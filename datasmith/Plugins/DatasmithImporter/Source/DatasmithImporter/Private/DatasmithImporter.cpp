// ============================================================================
//  文件: DatasmithImporter.cpp
//  作用: Datasmith 导入器核心实现。负责协调“读取翻译器(SceneTranslator)”加载
//        Datasmith 场景元素(网格/纹理/材质/Actor/动画/变体集等)，并分阶段执行：
//        1) 过滤需要导入/重导入的元素 (FilterElementsToImport)
//        2) 原始数据载入(可能并行) + 临时资产(位于 Transient/ 临时包)创建
//        3) 资源元数据写入与引用跟踪
//        4) Finalize 阶段: 迁移到最终包(重命名/软引用重映射/模板迁移/构建静态网格等)
//        5) Actor / 层级生成与后期修正 (引用修复、层信息、材质编译、组件重新注册)
//
//  设计要点:
//    - ImportContext: 贯穿所有阶段的“状态容器” (选项、包路径、映射表、进度反馈、取消标记)
//    - 先资源, 后 Actor；资源内部按依赖顺序: 纹理 -> 材质函数 -> 材质 -> 静态网格 -> 动画/变体
//    - 临时包 (TransientFolderPath) 中生成的对象在 Finalize 时迁移到 RootFolderPath
//    - ReferencesToRemap / SoftObjectPath 重定向保障重命名后引用一致
//    - 并行: 网格加载、纹理数据抓取使用 UE::Tasks；材质/网格最终构建仍在主线程顺序化
//    - 重导入: 通过 AssetImportData + 元素 Hash 判断是否跳过未变化资产
//
//  修改说明:
//    - 本次仅添加中文注释 (不改任何逻辑 / 标识符 / 宏)。
// ============================================================================

// Copyright Epic Games, Inc. All Rights Reserved.

#include "DatasmithImporter.h"

#include "DatasmithActorImporter.h"
#include "DatasmithAdditionalData.h"
#include "DatasmithAnimationElements.h"
#include "DatasmithAssetImportData.h"
#include "DatasmithAssetUserData.h"
#include "DatasmithCameraImporter.h"
#include "DatasmithCloth.h"  // UE_DEPRECATED(5.5, "The experimental Cloth importer is no longer supported.")
#include "DatasmithImportContext.h"
#include "DatasmithImporterModule.h"
#include "DatasmithLevelSequenceImporter.h"
#include "DatasmithLevelVariantSetsImporter.h"
#include "DatasmithLightImporter.h"
#include "DatasmithMaterialExpressions.h"
#include "DatasmithMaterialImporter.h"
#include "DatasmithPayload.h"
#include "DatasmithPostProcessImporter.h"
#include "DatasmithScene.h"
#include "DatasmithSceneActor.h"
#include "DatasmithSceneFactory.h"
#include "DatasmithStaticMeshImporter.h"
#include "DatasmithTextureImporter.h"
#include "DatasmithTranslator.h"
#include "DatasmithUtils.h"
#include "Engine/Texture2D.h"
#include "IDatasmithSceneElements.h"
#include "LevelVariantSets.h"
#include "ObjectTemplates/DatasmithObjectTemplate.h"
#include "Utility/DatasmithImporterImpl.h"
#include "Utility/DatasmithImporterUtils.h"
#include "Utility/DatasmithTextureResize.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "CineCameraComponent.h"
#include "ComponentReregisterContext.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Containers/Map.h"
#include "CoreMinimal.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorLevelUtils.h"
#include "Engine/Engine.h"
#include "Engine/RendererSettings.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Features/IModularFeatures.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Landscape.h"
#include "Layers/LayersSubsystem.h"
#include "LevelSequence.h"
#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialCachedData.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/UObjectToken.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "Serialization/ArchiveReplaceObjectRef.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"
#include "Settings/EditorExperimentalSettings.h"
#include "SkinnedAssetCompiler.h"
#include "SourceControlOperations.h"
#include "Tasks/Task.h"
#include "Templates/UniquePtr.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"
#include "UnrealEdGlobals.h"

extern UNREALED_API UEditorEngine* GEditor;

#define LOCTEXT_NAMESPACE "DatasmithImporter"

namespace UE::DatasmithImporter::Private
{
	/**
	 * 函数: MakeChangeToExpressionsNameForMaterialFunctions
	 * 目的: 解决材质函数被嵌套引用时, 参数(表达式)名称冲突问题, 确保最终生成的顶层材质中
	 *       每种表达式类型 (EDatasmithMaterialExpressionType) 下的参数名唯一。
	 * 核心思路:
	 *   1) 构建材料函数与其引用者拓扑(调用方在 FunctionMaterialAndTheirReferencer 中)
	 *   2) 逆序遍历 (确保被依赖的函数先处理, 顶层材质后处理保留其原生名称优先级)
	 *   3) 对每个表达式名做唯一性检测 (向引用链与顶层已使用集合递归查询)
	 *   4) 冲突时使用 FDatasmithUniqueNameProvider 生成新名字并写回表达式
	 *   5) Generic 类型集合额外记录所有非 Generic 的名字以防后续推断类型时冲突
	 * 复杂点: 递归检测引用关系 + 按表达式类型区分唯一名空间
	 */
	void MakeChangeToExpressionsNameForMaterialFunctions(const TArray<FDatasmithImporterUtils::FFunctionAndMaterialsThatUseIt>& FunctionMaterialAndTheirReferencer)
	{
		// Contains the name of the parameters for a material that isn't use as a function and the set of names used the by material functions it call
		TMap<TSharedPtr<IDatasmithUEPbrMaterialElement>,TArray<TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>>> TopMaterialAndNamesUsedByType;
		TMap<EDatasmithMaterialExpressionType, TUniquePtr<FDatasmithUniqueNameProvider>> ExpressionTypeToUniqueNameProvider;
		using FFunctionAndMaterialsThatUseIt = FDatasmithImporterUtils::FFunctionAndMaterialsThatUseIt;

		TMap<TSharedPtr<IDatasmithUEPbrMaterialElement>, int32> MaterialToIndexInArray;
		MaterialToIndexInArray.Reserve( FunctionMaterialAndTheirReferencer.Num() );

		// Check if the name was already used in this function or in one of its referencers
		TFunction<bool (uint32, const TSharedPtr<IDatasmithUEPbrMaterialElement>&, uint32, FName, EDatasmithMaterialExpressionType)> CheckIfNameIsUsed;
		CheckIfNameIsUsed = [&TopMaterialAndNamesUsedByType, &MaterialToIndexInArray, &FunctionMaterialAndTheirReferencer, &CheckIfNameIsUsed] (uint32 MaterialHash, const TSharedPtr<IDatasmithUEPbrMaterialElement>& Material, uint32 ExpressionHash, FName Expression, EDatasmithMaterialExpressionType ExpressionType)
			{
				if ( int32* Index = MaterialToIndexInArray.FindByHash( MaterialHash, Material ) )
				{
					for ( const TSharedPtr<IDatasmithUEPbrMaterialElement>& Referencer : FunctionMaterialAndTheirReferencer[*Index].Value )
					{
						if ( CheckIfNameIsUsed( GetTypeHash( Referencer ), Referencer, ExpressionHash, Expression, ExpressionType ) )
						{
							return true;
						}
					}
				}
				else if ( const TArray<TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>>* NameUsedArray = TopMaterialAndNamesUsedByType.FindByHash( MaterialHash, Material ) )
				// Only check if we are in a material that aren't referenced elsewhere
				{
					for ( const TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>& NamesUsed : *NameUsedArray )
					{
						if ( NamesUsed->Contains( TPair<FName, EDatasmithMaterialExpressionType>( Expression, ExpressionType ) ) )
						{
							return true;
						}
					}
				}

				return false;
			};

		// Make sure that the parameter have a unique name in the materials were they are use
		auto HandleExpressionNameForMaterialFunction =
			[&ExpressionTypeToUniqueNameProvider, &CheckIfNameIsUsed] (uint32 MaterialHash, const TSharedPtr<IDatasmithUEPbrMaterialElement>& Material, FName Expression, EDatasmithMaterialExpressionType ExpressionType, int32 ExpressionIndex, TSet<TPair<FName, EDatasmithMaterialExpressionType>>& NameUsed)
			{
				TUniquePtr<FDatasmithUniqueNameProvider>& UniqueNameProvider = ExpressionTypeToUniqueNameProvider.FindOrAdd( ExpressionType );
				if ( !UniqueNameProvider )
				{
					UniqueNameProvider = MakeUnique<FDatasmithUniqueNameProvider>();
				}

				FString ChosenName;

				if ( CheckIfNameIsUsed( MaterialHash, Material, GetTypeHash( Expression ), Expression, ExpressionType ) )
				{
					// Make the name unique
					ChosenName = UniqueNameProvider->GenerateUniqueName( Expression.ToString() );
					Material->GetExpression( ExpressionIndex )->SetName( *ChosenName );
				}
				else
				{
					// Keep track of the used names
					ChosenName = Expression.ToString();
					UniqueNameProvider->AddExistingName( ChosenName );
				}

				NameUsed.Add( TPair<FName, EDatasmithMaterialExpressionType>( *ChosenName, ExpressionType ) );

				if ( ExpressionType != EDatasmithMaterialExpressionType::Generic )
				{
					// Always add to the generic, we don't know what type of parameter a generic expression would be
					TUniquePtr<FDatasmithUniqueNameProvider>& GenericUniqueNameProvider = ExpressionTypeToUniqueNameProvider.FindOrAdd( EDatasmithMaterialExpressionType::Generic );
					if ( !GenericUniqueNameProvider )
					{
						GenericUniqueNameProvider = MakeUnique<FDatasmithUniqueNameProvider>();
					}

					GenericUniqueNameProvider->AddExistingName( ChosenName );
					NameUsed.Add( TPair<FName, EDatasmithMaterialExpressionType>( *ChosenName, EDatasmithMaterialExpressionType::Generic ) );
				}
			};

		TFunction<void (const TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>&, uint32, const TSharedPtr<IDatasmithUEPbrMaterialElement>&)> PushUsedNamesIntoTopLevelMaterial;
		PushUsedNamesIntoTopLevelMaterial = [&MaterialToIndexInArray, &TopMaterialAndNamesUsedByType, &FunctionMaterialAndTheirReferencer, &PushUsedNamesIntoTopLevelMaterial] (const TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>& UsedNames, uint32 MaterialHash, const TSharedPtr<IDatasmithUEPbrMaterialElement>& Material)
			{
				if ( int32* Index = MaterialToIndexInArray.FindByHash( MaterialHash, Material ) )
				{
					for ( const TSharedPtr<IDatasmithUEPbrMaterialElement>& Referencer : FunctionMaterialAndTheirReferencer[*Index].Value )
					{
						PushUsedNamesIntoTopLevelMaterial( UsedNames, GetTypeHash( Referencer ), Referencer );
					}
				}
				else
				{
					TopMaterialAndNamesUsedByType.FindOrAddByHash( MaterialHash, Material ).Add( UsedNames );
				}
			};


		TSet<TSharedPtr<IDatasmithUEPbrMaterialElement>> VisitedMaterials;
		// Plus one because there is at least one top material that isn't used as a function
		VisitedMaterials.Reserve( FunctionMaterialAndTheirReferencer.Num() + 1 );

		/**
			* We expect the independents function to be at the start of the array
			* We want the top level function to conserve their name in case of conflicted naming of the parameters.
			*/
		for ( int32 Index = FunctionMaterialAndTheirReferencer.Num()-1; Index >= 0; Index-- )
		{
			const FFunctionAndMaterialsThatUseIt& FunctionAndReferencer = FunctionMaterialAndTheirReferencer[Index];
			const TSharedPtr<IDatasmithUEPbrMaterialElement>& CurrentElement = FunctionAndReferencer.Key;
			uint32 CurrentElementHash = GetTypeHash( CurrentElement );
			MaterialToIndexInArray.AddByHash( CurrentElementHash, CurrentElement, Index );

			// Populate the name used by the Referencers
			for ( const TSharedPtr<IDatasmithUEPbrMaterialElement>& Referencer : FunctionAndReferencer.Value )
			{
				uint32  ReferencerHash = GetTypeHash( Referencer );

				if ( !VisitedMaterials.ContainsByHash( ReferencerHash, Referencer ) )
				{
					TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>> UsedNames = MakeShared<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>();

					FDatasmithMaterialExpressions::ForEachParamsNameInMaterial( Referencer,
						[ReferencerHash, &Referencer, &HandleExpressionNameForMaterialFunction, &UsedNames] (FName Expression, const EDatasmithMaterialExpressionType& ExpressionType, int32 Index)
						{
							HandleExpressionNameForMaterialFunction( ReferencerHash, Referencer, Expression, ExpressionType, Index, UsedNames.Get() );
						});
					VisitedMaterials.AddByHash( ReferencerHash, Referencer );
					PushUsedNamesIntoTopLevelMaterial( UsedNames, ReferencerHash, Referencer );
				}
			}

			// For the current element
			TSharedRef<TSet<TPair<FName, EDatasmithMaterialExpressionType>>> UsedNames = MakeShared<TSet<TPair<FName, EDatasmithMaterialExpressionType>>>();

			TFunction<void(FName Expression, const EDatasmithMaterialExpressionType& ExpressionType, int32 Index)> ForEachParameter =
				[CurrentElementHash, &CurrentElement, &UsedNames, &HandleExpressionNameForMaterialFunction](FName Expression, const EDatasmithMaterialExpressionType& ExpressionType, int32 Index)
				{
					HandleExpressionNameForMaterialFunction( CurrentElementHash, CurrentElement, Expression, ExpressionType, Index, UsedNames.Get() );
				};

			FDatasmithMaterialExpressions::ForEachParamsNameInMaterial( CurrentElement,
				[CurrentElementHash, &CurrentElement, &UsedNames, &HandleExpressionNameForMaterialFunction] (FName Expression, const EDatasmithMaterialExpressionType& ExpressionType, int32 Index)
				{
					HandleExpressionNameForMaterialFunction( CurrentElementHash, CurrentElement, Expression, ExpressionType, Index, UsedNames.Get() );
				});
			VisitedMaterials.AddByHash( CurrentElementHash, CurrentElement );
			PushUsedNamesIntoTopLevelMaterial( UsedNames, CurrentElementHash, CurrentElement );
		}
	}
} // namespace UE::DatasmithImporter::Private

// ============================= 静态网格导入 =============================
// ImportStaticMeshes
// 作用: 遍历过滤后的网格元素, 进行并行预加载(若翻译器支持), 再顺序生成 UStaticMesh 临时对象,
//       最终批量设置 Lightmap 权重与构建参数。仅创建(暂不 Build)。
// 参数: ImportContext - 全局导入上下文(含过滤场景、翻译器、选项、进度等)
// 注意: 并行阶段只收集 payload; 第二阶段才调用 ImportStaticMesh 实际生成资产。
void FDatasmithImporter::ImportStaticMeshes( FDatasmithImportContext& ImportContext )
{
	const int32 StaticMeshesCount = ImportContext.FilteredScene->GetMeshesCount();

	if ( !ImportContext.Options->BaseOptions.bIncludeGeometry || StaticMeshesCount == 0 )
	{
		return;
	}

	if (!ImportContext.AssetsContext.StaticMeshesFinalPackage || ImportContext.AssetsContext.StaticMeshesFinalPackage->GetFName() == NAME_None || ImportContext.SceneTranslator == nullptr)
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportStaticMeshes);

	TUniquePtr<FScopedSlowTask> ProgressPtr;

	if ( ImportContext.FeedbackContext )
	{
		ProgressPtr = MakeUnique<FScopedSlowTask>(StaticMeshesCount, LOCTEXT("ImportStaticMeshes", "Importing Static Meshes..."), true, *ImportContext.FeedbackContext );
		ProgressPtr->MakeDialog(true);
	}

	TMap<TSharedRef<IDatasmithMeshElement>, UE::Tasks::TTask<FDatasmithMeshElementPayload*>> MeshElementPayloads;

	FDatasmithTranslatorCapabilities TranslatorCapabilities;
	ImportContext.SceneTranslator->Initialize(TranslatorCapabilities);

	// Parallelize loading by doing a first pass to send translator loading into async task
	if (TranslatorCapabilities.bParallelLoadStaticMeshSupported)
	{
		for (int32 MeshIndex = 0; MeshIndex < StaticMeshesCount && !ImportContext.bUserCancelled; ++MeshIndex)
		{
			if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
			{
				ImportContext.bUserCancelled = true;
			}

			TSharedRef<IDatasmithMeshElement> MeshElement = ImportContext.FilteredScene->GetMesh( MeshIndex ).ToSharedRef();

			auto& ImportedStaticMesh = ImportContext.ImportedStaticMeshes.FindOrAdd( MeshElement );

			// We still have factories that are importing the UStaticMesh on their own, so check if it's already imported here
			if (ImportedStaticMesh == nullptr)
			{
				// Parallel loading from the translator using futures
				MeshElementPayloads.Add(
					MeshElement,
					UE::Tasks::Launch(UE_SOURCE_LOCATION,
						[&ImportContext, MeshElement]() -> FDatasmithMeshElementPayload*
						{
							if (ImportContext.bUserCancelled)
							{
								return nullptr;
							}

							TRACE_CPUPROFILER_EVENT_SCOPE(LoadStaticMesh);
							TUniquePtr<FDatasmithMeshElementPayload> MeshPayload = MakeUnique<FDatasmithMeshElementPayload>();
							return ImportContext.SceneTranslator->LoadStaticMesh(MeshElement, *MeshPayload) ? MeshPayload.Release() : nullptr;
						}
					)
				);
			}
		}
	}

	FScopedSlowTask* Progress = ProgressPtr.Get();

	// This pass will wait on the futures we got from the first pass async tasks
	for ( int32 MeshIndex = 0; MeshIndex < StaticMeshesCount && !ImportContext.bUserCancelled; ++MeshIndex )
	{
		if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
		{
			ImportContext.bUserCancelled = true;
		}

		TSharedRef< IDatasmithMeshElement > MeshElement = ImportContext.FilteredScene->GetMesh( MeshIndex ).ToSharedRef();

		FDatasmithImporterImpl::ReportProgress( Progress, 1.f, FText::FromString( FString::Printf( TEXT("Importing static mesh %d/%d (%s) ..."), MeshIndex + 1, StaticMeshesCount, MeshElement->GetLabel() ) ) );

		UStaticMesh* ExistingStaticMesh = nullptr;

		if (ImportContext.SceneAsset)
		{
			TSoftObjectPtr< UStaticMesh >* ExistingStaticMeshPtr = ImportContext.SceneAsset->StaticMeshes.Find( MeshElement->GetName() );

			if (ExistingStaticMeshPtr)
			{
				ExistingStaticMesh = ExistingStaticMeshPtr->LoadSynchronous();
			}
		}

		// #ueent_todo rewrite in N passes:
		//  - GetDestination (find or create StaticMesh, duplicate, flags and context etc)
		//  - Import (Import data in simple memory repr (eg. TArray<FMeshDescription>)
		//  - Set (fill UStaticMesh with imported data)
		UE::Tasks::TTask<FDatasmithMeshElementPayload*> MeshPayload;
		if (MeshElementPayloads.RemoveAndCopyValue(MeshElement, MeshPayload))
		{
			TUniquePtr<FDatasmithMeshElementPayload> MeshPayloadPtr(MeshPayload.GetResult());
			if (MeshPayloadPtr)
			{
				ImportStaticMesh(ImportContext, MeshElement, ExistingStaticMesh, MeshPayloadPtr.Get());
			}
		}
		else
		{
			ImportStaticMesh(ImportContext, MeshElement, ExistingStaticMesh, nullptr);
		}

		ImportContext.ImportedStaticMeshesByName.Add(MeshElement->GetName(), MeshElement);
	}

	//Just make sure there is no async task left running in case of a cancellation
	for (TPair<TSharedRef<IDatasmithMeshElement>, UE::Tasks::TTask<FDatasmithMeshElementPayload*>>& Kvp : MeshElementPayloads)
	{
		// Wait for the result and delete it when getting out of scope
		TUniquePtr<FDatasmithMeshElementPayload> MeshPayloadPtr(Kvp.Value.GetResult());
	}

	TMap< TSharedRef< IDatasmithMeshElement >, float > LightmapWeights = FDatasmithStaticMeshImporter::CalculateMeshesLightmapWeights( ImportContext.Scene.ToSharedRef() );

	for ( auto& ImportedStaticMeshPair : ImportContext.ImportedStaticMeshes )
	{
		FDatasmithStaticMeshImporter::SetupStaticMesh( ImportContext.AssetsContext, ImportedStaticMeshPair.Key, ImportedStaticMeshPair.Value, ImportContext.Options->BaseOptions.StaticMeshOptions, LightmapWeights[ ImportedStaticMeshPair.Key ] );
	}
}

// ImportClothes (已废弃)
// 作用: 旧实验性布料导入逻辑, 5.5 标记为弃用; 仍保留以兼容旧工程。
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void FDatasmithImporter::ImportClothes(FDatasmithImportContext& ImportContext)  // UE_DEPRECATED(5.5, "The experimental Cloth importer is no longer supported.")
{
	const int32 ClothesCount = ImportContext.FilteredScene->GetClothesCount();

	if (!ImportContext.Options->BaseOptions.bIncludeGeometry
	 || ClothesCount == 0
	 || ImportContext.SceneTranslator == nullptr
	 || !ImportContext.AssetsContext.StaticMeshesFinalPackage
	 || ImportContext.AssetsContext.StaticMeshesFinalPackage->GetFName() == NAME_None)
	{
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportClothes);

	// Locate a cloth asset provider
	const TArray<IDatasmithClothFactoryClassesProvider*> ClothFactoryClassesProviders = IModularFeatures::Get().GetModularFeatureImplementations<IDatasmithClothFactoryClassesProvider>(IDatasmithClothFactoryClassesProvider::FeatureName);
	if (!ClothFactoryClassesProviders.Num() || !ClothFactoryClassesProviders[0])
	{
		UE_LOG(LogDatasmithImport, Warning, TEXT("No cloth factory class provider found, cloth import skipped."));
		return;
	}
	const UDatasmithClothAssetFactory* const ClothAssetFactory = ClothFactoryClassesProviders[0]->GetClothAssetFactoryClass().GetDefaultObject();
	if (!ClothAssetFactory)
	{
		UE_LOG(LogDatasmithImport, Warning, TEXT("Invalid (null) cloth asset factory, cloth import skipped."));
		return;
	}
	if (ClothFactoryClassesProviders.Num() > 1)
	{
		UE_LOG(LogDatasmithImport, Warning, TEXT("Multiple cloth asset providers found, only the cloth asset type from provider %s will be imported."), *ClothFactoryClassesProviders[0]->GetName().ToString());
	}

	TUniquePtr<FScopedSlowTask> ProgressPtr;
	if (ImportContext.FeedbackContext)
	{
		ProgressPtr = MakeUnique<FScopedSlowTask>(ClothesCount, LOCTEXT("ImportClothes", "Importing Clothes..."), true, *ImportContext.FeedbackContext);
		ProgressPtr->MakeDialog(true);
	}

	FScopedSlowTask* Progress = ProgressPtr.Get();

	for (int32 ClothIndex = 0; ClothIndex < ClothesCount && !ImportContext.bUserCancelled; ++ClothIndex)
	{
		ImportContext.bUserCancelled = FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext);

		TSharedPtr<IDatasmithClothElement> ClothElementPtr = ImportContext.FilteredScene->GetCloth(ClothIndex);
		if (!ClothElementPtr)
		{
			continue;
		}
		TSharedRef<IDatasmithClothElement> ClothElement = ClothElementPtr.ToSharedRef();

		FDatasmithImporterImpl::ReportProgress(Progress, 1.f, FText::FromString(FString::Printf(TEXT("Importing cloth %d/%d (%s) ..."), ClothIndex + 1, ClothesCount, ClothElement->GetLabel())));

		int32 MaxNameCharCount = FDatasmithImporterUtils::GetAssetNameMaxCharCount(ImportContext.AssetsContext.StaticMeshesFinalPackage.Get());
		FString ClothName = ImportContext.AssetsContext.StaticMeshNameProvider.GenerateUniqueName(ClothElement->GetLabel(), MaxNameCharCount);
		ClothName = ObjectTools::SanitizeObjectName(ClothName);

		FDatasmithClothElementPayload Payload;
		// #ue_ds_cloth_todo: async parallel support
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportClothes:TranslatorLoadCloth);
			if (!ImportContext.SceneTranslator->LoadCloth(ClothElement, Payload))
			{
				UE_LOG(LogDatasmithImport, Warning, TEXT("Cloth %s: load failed, translator issue."), ClothElement->GetName());
				return;
			}

			if (Payload.Cloth.Patterns.IsEmpty())
			{
				UE_LOG(LogDatasmithImport, Warning, TEXT("Cloth %s: no patterns, import skipped."), ClothElement->GetName());
				return;
			}
		}
		FDatasmithCloth DsCloth = MoveTemp(Payload.Cloth);

		UPackage* ImportPackage = ImportContext.AssetsContext.StaticMeshesImportPackage.Get();
		EObjectFlags ObjectFlags = ImportContext.ObjectFlags & ~RF_Public; // not RF_Public yet, the publicized asset will be.

		// TODO: Use FCollectionPropertyFacade instead, see \Engine\Source\Runtime\Experimental\Chaos\Public\Chaos\CollectionPropertyFacade.h
		//TArray<UObject*> ClothPresetAssets;

		//for (const FDatasmithClothPresetPropertySet& PropertySet : DsCloth.PropertySets)
		//{
		//	FString PresetAssetName = ObjectTools::SanitizeObjectName(PropertySet.SetName);
		//	if (UChaosClothPreset* PropertySetAsset = NewObject<UChaosClothPreset>(ImportPackage, *PresetAssetName, ObjectFlags))
		//	{
		//		ClothPresetAssets.Add(PropertySetAsset);
		//		for (const FDatasmithClothPresetProperty& Property : PropertySet.Properties)
		//		{
		//			// #ue_ds_cloth_todo fill property values, min/max,
		//			PropertySetAsset->AddProperty(Property.Name, EChaosClothPresetPropertyType::String);
		//		}
		//	}
		//}

		UObject* ExistingCloth = nullptr;
		if (ImportContext.SceneAsset)
		{
			if (TSoftObjectPtr<UObject>* ExistingAssetPtr = ImportContext.SceneAsset->Clothes.Find(ClothElement->GetName()))
			{
				ExistingCloth = ExistingAssetPtr->LoadSynchronous();
			}
		}
		UObject* ClothAsset = nullptr;
		if (ExistingCloth)
		{
			if (ExistingCloth->GetOuter() != ImportPackage)
			{
				// Temporary flag to skip PostLoad during DuplicateObject
				ExistingCloth->SetFlags(RF_ArchetypeObject);
				ClothAsset = ClothAssetFactory->DuplicateClothAsset(ExistingCloth, ImportPackage, *ClothName);
				ExistingCloth->ClearFlags(RF_ArchetypeObject);
				ClothAsset->ClearFlags(RF_ArchetypeObject);

				IDatasmithImporterModule::Get().ResetOverrides(ClothAsset);
			}
			else
			{
				ClothAsset = ExistingCloth;
			}

			ExistingCloth->SetFlags(ObjectFlags);
		}
		else
		{
			ClothAsset = ClothAssetFactory->CreateClothAsset(ImportPackage, *ClothName, ObjectFlags);
		}

		if (!ensure(ClothAsset))
		{
			continue;
		}

		ClothAssetFactory->InitializeClothAsset(ClothAsset, DsCloth);

		ImportContext.ImportedClothes.Add(ClothElement, ClothAsset);

		// TODO: Use FCollectionPropertyFacade instead, see \Engine\Source\Runtime\Experimental\Chaos\Public\Chaos\CollectionPropertyFacade.h
		//for (UObject* ClothPresetAsset : ClothPresetAssets)
		//{
		//	ImportContext.ImportedClothPresets.Add(ClothPresetAsset);
		//}
	}
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

// ImportStaticMesh
// 作用: 对单个 MeshElement 执行载入/转换 -> 创建 UStaticMesh -> 生成 ImportData / 元数据
// 参数: MeshElement - 场景网格描述; ExistingStaticMesh - 可能的旧资产; MeshPayload - 已并行加载数据
// 返回: 导入或复用的 UStaticMesh 指针
UStaticMesh* FDatasmithImporter::ImportStaticMesh( FDatasmithImportContext& ImportContext, TSharedRef< IDatasmithMeshElement > MeshElement, UStaticMesh* ExistingStaticMesh, FDatasmithMeshElementPayload* MeshPayload)
{
	if ( !ImportContext.AssetsContext.StaticMeshesFinalPackage || ImportContext.AssetsContext.StaticMeshesFinalPackage->GetFName() == NAME_None || ImportContext.SceneTranslator == nullptr)
	{
		return nullptr;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportStaticMesh);

	auto& ImportedStaticMesh = ImportContext.ImportedStaticMeshes.FindOrAdd( MeshElement );

	TArray<UDatasmithAdditionalData*> AdditionalData;

	if ( ImportedStaticMesh == nullptr ) // We still have factories that are importing the UStaticMesh on their own, so check if it's already imported here
	{
		FDatasmithMeshElementPayload LocalMeshPayload;
		if (MeshPayload == nullptr)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(LoadStaticMesh);
			ImportContext.SceneTranslator->LoadStaticMesh(MeshElement, LocalMeshPayload);
			MeshPayload = &LocalMeshPayload;
		}

		ImportedStaticMesh = FDatasmithStaticMeshImporter::ImportStaticMesh( MeshElement, *MeshPayload, ImportContext.ObjectFlags & ~RF_Public, ImportContext.Options->BaseOptions.StaticMeshOptions, ImportContext.AssetsContext, ExistingStaticMesh );
		AdditionalData = MoveTemp(MeshPayload->AdditionalData);

		// Make sure the garbage collector can collect additional data allocated on other thread
		for (UDatasmithAdditionalData* Data : AdditionalData)
		{
			if (Data)
			{
				Data->ClearInternalFlags(EInternalObjectFlags::Async);
			}
		}

		// Creation of static mesh failed, remove it from the list of importer mesh elements
		if (ImportedStaticMesh == nullptr)
		{
			ImportContext.ImportedStaticMeshes.Remove(MeshElement);
			return nullptr;
		}
	}

	CreateStaticMeshAssetImportData( ImportContext, MeshElement, ImportedStaticMesh, AdditionalData );

	ImportMetaDataForObject( ImportContext, MeshElement, ImportedStaticMesh );

	if ( MeshElement->GetLightmapSourceUV() >= MAX_MESH_TEXTURE_COORDS_MD )
	{
		FFormatNamedArguments FormatArgs;
		FormatArgs.Add(TEXT("SourceUV"), FText::FromString(FString::FromInt(MeshElement->GetLightmapSourceUV())));
		FormatArgs.Add(TEXT("MeshName"), FText::FromName(ImportedStaticMesh->GetFName()));
		ImportContext.LogError(FText::Format(LOCTEXT("InvalidLightmapSourceUVError", "The lightmap source UV '{SourceUV}' used for the lightmap UV generation of the mesh '{MeshName}' is invalid."), FormatArgs));
	}

	return ImportedStaticMesh;
}


// FinalizeStaticMesh
// 作用: 将临时(Transient)静态网格迁移到目标包路径并可选 Build (此处通常延迟构建集中处理)
// 参数: SourceStaticMesh - 源静态网格; StaticMeshesFolderPath - 目标文件夹路径; ExistingStaticMesh - 可能的旧资产;
//        ReferencesToRemap - 引用重映射表; bBuild - 是否在迁移后立即构建
UStaticMesh* FDatasmithImporter::FinalizeStaticMesh( UStaticMesh* SourceStaticMesh, const TCHAR* StaticMeshesFolderPath, UStaticMesh* ExistingStaticMesh, TMap< UObject*, UObject* >* ReferencesToRemap, bool bBuild)
{
	UStaticMesh* DestinationStaticMesh = Cast< UStaticMesh >( FDatasmithImporterImpl::FinalizeAsset( SourceStaticMesh, StaticMeshesFolderPath, ExistingStaticMesh, ReferencesToRemap ) );

	if (bBuild)
	{
		FDatasmithStaticMeshImporter::BuildStaticMesh(DestinationStaticMesh);
	}

	return DestinationStaticMesh;
}

// CreateStaticMeshAssetImportData
// 作用: 为静态网格创建/更新 ImportData, 写入源文件路径与 Hash, 同时附加 AdditionalData
void FDatasmithImporter::CreateStaticMeshAssetImportData(FDatasmithImportContext& InContext, TSharedRef< IDatasmithMeshElement > MeshElement, UStaticMesh* ImportedStaticMesh, TArray<UDatasmithAdditionalData*>& AdditionalData)
{
	UDatasmithStaticMeshImportData::DefaultOptionsPair ImportOptions = UDatasmithStaticMeshImportData::DefaultOptionsPair( InContext.Options->BaseOptions.StaticMeshOptions, InContext.Options->BaseOptions.AssetOptions );

	UDatasmithStaticMeshImportData* MeshImportData = UDatasmithStaticMeshImportData::GetImportDataForStaticMesh( ImportedStaticMesh, ImportOptions );

	if ( MeshImportData )
	{
		// Update the import data source file and set the mesh hash
		// #ueent_todo FH: piggybacking off of the SourceData file hash for now, until we have custom derived AssetImportData properly serialize to the AssetRegistry
		FMD5Hash Hash = MeshElement->CalculateElementHash( false );
		MeshImportData->Update( InContext.Options->FilePath, &Hash );
		MeshImportData->DatasmithImportInfo = FDatasmithImportInfo(InContext.Options->SourceUri, InContext.Options->SourceHash);

		// Set the final outer // #ueent_review: propagate flags of outer?
		for (UDatasmithAdditionalData* Data: AdditionalData)
		{
			Data->Rename(nullptr, MeshImportData);
		}
		MeshImportData->AdditionalData = MoveTemp(AdditionalData);
	}
}

// ImportTextures
// 作用: 纹理导入主流程: 过滤 -> 异步抓取原始数据 -> 同步创建 UTexture -> 元数据写入。
// 并发: 使用 UE::Tasks 抓取文件内容/字节; IES 纹理特殊同步路径。
void FDatasmithImporter::ImportTextures( FDatasmithImportContext& ImportContext )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportTextures);

	FDatasmithImporterImpl::SetTexturesMode( ImportContext );

	const int32 TexturesCount = ImportContext.FilteredScene->GetTexturesCount();

	TUniquePtr<FScopedSlowTask> ProgressPtr;

	if ( ImportContext.FeedbackContext )
	{
		ProgressPtr = MakeUnique<FScopedSlowTask>( (float)TexturesCount, LOCTEXT("ImportingTextures", "Importing Textures..."), true, *ImportContext.FeedbackContext );
		ProgressPtr->MakeDialog(true);
	}

	if (ImportContext.Options->TextureConflictPolicy != EDatasmithImportAssetConflictPolicy::Ignore && TexturesCount > 0)
	{
		FDatasmithTextureImporter DatasmithTextureImporter(ImportContext);

		TArray<TSharedPtr< IDatasmithTextureElement >> FilteredTextureElements;
		for ( int32 i = 0; i < TexturesCount; i++ )
		{
			TSharedPtr< IDatasmithTextureElement > TextureElement = ImportContext.FilteredScene->GetTexture(i);

			if ( !TextureElement )
			{
				continue;
			}

			FilteredTextureElements.Add(TextureElement);
		}

		FDatasmithTextureResize::Initialize();

		// Try to import textures async first, this does not work with DatasmithImportFactory as it creates a deadlock on the main thread.
		const bool bInterchangeImport = false; /*GetDefault<UEditorExperimentalSettings>()->bEnableInterchangeFramework*/
		if ( bInterchangeImport )
		{
			for ( int32 TextureIndex = 0; TextureIndex < FilteredTextureElements.Num(); TextureIndex++ )
			{
				if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
				{
					ImportContext.bUserCancelled = true;
				}

				if (ImportContext.bUserCancelled)
				{
					break;
				}

				// Skip asynchronous creation for IES textures as it is not supported.
				// The creation of such textures will synchronously happen later in the call to FDatasmithTextureImporter::CreateTexture
				if (FilteredTextureElements[TextureIndex]->GetTextureMode() == EDatasmithTextureMode::Ies)
				{
					continue;
				}

				UE::Interchange::FAssetImportResultRef FutureTexture = DatasmithTextureImporter.CreateTextureAsync( FilteredTextureElements[TextureIndex] );

				if ( FutureTexture->IsValid() )
				{
					FutureTexture->OnDone(
						[ TextureElement = FilteredTextureElements[TextureIndex].ToSharedRef(), &ImportContext ]( UE::Interchange::FImportResult& AssetImportResults )
						{
							ImportMetaDataForObject( ImportContext, TextureElement, AssetImportResults.GetFirstAssetOfClass( UTexture::StaticClass() ) );
						}
					);

					ImportContext.ImportedTextures.Add( FilteredTextureElements[TextureIndex].ToSharedRef(), MoveTemp( FutureTexture ) );
				}
			}
		}

		struct FAsyncData
		{
			FString       Extension;
			TArray<uint8> TextureData;
			UE::Tasks::TTask<bool> Result;
		};
		TArray<FAsyncData> AsyncData;

		AsyncData.SetNum(FilteredTextureElements.Num());

		for ( int32 TextureIndex = 0; TextureIndex < FilteredTextureElements.Num(); TextureIndex++ )
		{
			if ( ImportContext.ImportedTextures.Contains( FilteredTextureElements[TextureIndex].ToSharedRef() ) )
			{
				continue;
			}

			if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
			{
				ImportContext.bUserCancelled = true;
			}

			AsyncData[TextureIndex].Result =
				UE::Tasks::Launch(UE_SOURCE_LOCATION,
					[&ImportContext, &AsyncData, &FilteredTextureElements, &DatasmithTextureImporter, TextureIndex]()
					{
						if (ImportContext.bUserCancelled)
						{
							return false;
						}

						if (FilteredTextureElements[TextureIndex]->GetTextureMode() == EDatasmithTextureMode::Ies)
						{
							return true;
						}

						return DatasmithTextureImporter.GetTextureData(FilteredTextureElements[TextureIndex], AsyncData[TextureIndex].TextureData, AsyncData[TextureIndex].Extension);
					}
				);
		}

		// Avoid a call to IsValid for each item
		FScopedSlowTask* Progress = ProgressPtr.Get();

		for ( int32 TextureIndex = 0; TextureIndex < FilteredTextureElements.Num(); TextureIndex++ )
		{
			if ( ImportContext.ImportedTextures.Contains( FilteredTextureElements[TextureIndex].ToSharedRef() ) )
			{
				continue;
			}

			if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
			{
				ImportContext.bUserCancelled = true;
			}

			if ( ImportContext.bUserCancelled )
			{
				// If operation has been canceled, just wait for other threads to also cancel
				AsyncData[TextureIndex].Result.Wait();
			}
			else
			{
				const TSharedPtr< IDatasmithTextureElement >& TextureElement = FilteredTextureElements[TextureIndex];

				FDatasmithImporterImpl::ReportProgress( Progress, 1.f, FText::FromString( FString::Printf( TEXT("Importing texture %d/%d (%s) ..."), TextureIndex + 1, FilteredTextureElements.Num(), TextureElement->GetLabel() ) ) );

				UTexture* ExistingTexture = nullptr;

				if ( ImportContext.SceneAsset )
				{
					TSoftObjectPtr< UTexture >* ExistingTexturePtr = ImportContext.SceneAsset->Textures.Find( TextureElement->GetName() );

					if ( ExistingTexturePtr )
					{
						ExistingTexture = ExistingTexturePtr->LoadSynchronous();
					}
				}

				if (AsyncData[TextureIndex].Result.GetResult())
				{
					ImportTexture( ImportContext, DatasmithTextureImporter, TextureElement.ToSharedRef(), ExistingTexture, AsyncData[TextureIndex].TextureData, AsyncData[TextureIndex].Extension );
				}
			}

			// Release memory as soon as possible
			AsyncData[TextureIndex].TextureData.Empty();
		}
	}
}

// ImportTexture
// 作用: 基于已获取的字节数据创建单个纹理并登记到 ImportContext.ImportedTextures
UTexture* FDatasmithImporter::ImportTexture( FDatasmithImportContext& ImportContext, FDatasmithTextureImporter& DatasmithTextureImporter, TSharedRef< IDatasmithTextureElement > TextureElement, UTexture* ExistingTexture, const TArray<uint8>& TextureData, const FString& Extension )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportTexture);

	UTexture* ImportedTexture = DatasmithTextureImporter.CreateTexture( TextureElement, TextureData, Extension );

	if ( !ImportedTexture )
	{
		return nullptr;
	}

	UE::Interchange::FAssetImportResultRef& AssetImportResults = ImportContext.ImportedTextures.Add( TextureElement, MakeShared< UE::Interchange::FImportResult, ESPMode::ThreadSafe >() );
	AssetImportResults->AddImportedObject( ImportedTexture );
	AssetImportResults->SetDone();

	ImportMetaDataForObject( ImportContext, TextureElement, ImportedTexture );

	return Cast< UTexture >( ImportedTexture );
}

// FinalizeTexture
// 作用: 纹理迁移至最终包并处理软引用重映射。
UTexture* FDatasmithImporter::FinalizeTexture( UTexture* SourceTexture, const TCHAR* TexturesFolderPath, UTexture* ExistingTexture, TMap< UObject*, UObject* >* ReferencesToRemap )
{
	return Cast< UTexture >( FDatasmithImporterImpl::FinalizeAsset( SourceTexture, TexturesFolderPath, ExistingTexture, ReferencesToRemap ) );
}

// ImportMaterials
// 作用: 先导入所有被引用的材质函数(递归拓扑排序), 再导入普通材质, 记录构建需求并标记需重新注册组件。
// 注意: 使用 FGlobalComponentReregisterContext 避免旧材质资源仍被渲染线程持有导致崩溃。
void FDatasmithImporter::ImportMaterials( FDatasmithImportContext& ImportContext )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportMaterials);

	if ( ImportContext.Options->MaterialConflictPolicy != EDatasmithImportAssetConflictPolicy::Ignore && ImportContext.FilteredScene->GetMaterialsCount() > 0 )
	{
		IDatasmithShaderElement::bUseRealisticFresnel = ( ImportContext.Options->MaterialQuality == EDatasmithImportMaterialQuality::UseRealFresnelCurves );
		IDatasmithShaderElement::bDisableReflectionFresnel = ( ImportContext.Options->MaterialQuality == EDatasmithImportMaterialQuality::UseNoFresnelCurves );

		//Import referenced materials as MaterialFunctions first
		{
			using FFunctionAndMaterialsThatUseIt = FDatasmithImporterUtils::FFunctionAndMaterialsThatUseIt;
			TArray<FFunctionAndMaterialsThatUseIt> Functions = FDatasmithImporterUtils::GetOrderedListOfMaterialsReferencedByMaterials( ImportContext.FilteredScene );

			UE::DatasmithImporter::Private::MakeChangeToExpressionsNameForMaterialFunctions(Functions);

			for (const FFunctionAndMaterialsThatUseIt& FunctionAndMaterials : Functions)
			{
				ImportMaterialFunction(ImportContext, FunctionAndMaterials.Key.ToSharedRef());
			}
		}

		ImportContext.AssetsContext.MaterialsRequirements.Empty( ImportContext.FilteredScene->GetMaterialsCount() );

		for (FDatasmithImporterUtils::FDatasmithMaterialImportIterator It(ImportContext); It; ++It)
		{
			TSharedRef< IDatasmithBaseMaterialElement > MaterialElement = It.Value().ToSharedRef();

			UMaterialInterface* ExistingMaterial = nullptr;

			if ( ImportContext.SceneAsset )
			{
				TSoftObjectPtr< UMaterialInterface >* ExistingMaterialPtr = ImportContext.SceneAsset->Materials.Find( MaterialElement->GetName() );

				if ( ExistingMaterialPtr )
				{
					ExistingMaterial = ExistingMaterialPtr->LoadSynchronous();
				}
			}

			ImportMaterial( ImportContext, MaterialElement, ExistingMaterial );
		}

		// IMPORTANT: FGlobalComponentReregisterContext destructor will de-register and re-register all UActorComponent present in the world
		// Consequently, all static meshes will stop using the FMaterialResource of the original materials on de-registration
		// and will use the new FMaterialResource created on re-registration.
		// Otherwise, the editor will crash on redraw
		FGlobalComponentReregisterContext RecreateComponents;
	}
}

// ImportMaterialFunction / FinalizeMaterialFunction
// 作用: 创建材质函数临时资产并在 Finalize 阶段迁移 + PostEdit 触发编译。
UMaterialFunction* FDatasmithImporter::ImportMaterialFunction(FDatasmithImportContext& ImportContext, TSharedRef< IDatasmithBaseMaterialElement > MaterialElement)
{
	UMaterialFunction* ImportedMaterialFunction = FDatasmithMaterialImporter::CreateMaterialFunction( ImportContext, MaterialElement );

	if (!ImportedMaterialFunction )
	{
		return nullptr;
	}

	ImportContext.ImportedMaterialFunctions.Add( MaterialElement ) = ImportedMaterialFunction;

	return ImportedMaterialFunction;
}

UMaterialFunction* FDatasmithImporter::FinalizeMaterialFunction(UObject* SourceMaterialFunction, const TCHAR* MaterialFunctionsFolderPath,
	UMaterialFunction* ExistingMaterialFunction, TMap< UObject*, UObject* >* ReferencesToRemap)
{
	UMaterialFunction* MaterialFunction = Cast< UMaterialFunction >( FDatasmithImporterImpl::FinalizeAsset( SourceMaterialFunction, MaterialFunctionsFolderPath, ExistingMaterialFunction, ReferencesToRemap ) );

	MaterialFunction->PreEditChange( nullptr );
	MaterialFunction->PostEditChange();

	return MaterialFunction;
}

// ImportMaterial / FinalizeMaterial
// 作用: 创建材质或材质实例, 设置 ImportData 与元数据; Finalize 时递归处理父材质并编译表达式。
UMaterialInterface* FDatasmithImporter::ImportMaterial( FDatasmithImportContext& ImportContext,
	TSharedRef< IDatasmithBaseMaterialElement > MaterialElement, UMaterialInterface* ExistingMaterial )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportMaterial);

	UMaterialInterface* ImportedMaterial = FDatasmithMaterialImporter::CreateMaterial( ImportContext, MaterialElement, ExistingMaterial );

	if ( !ImportedMaterial )
	{
		return nullptr;
	}

	if (GetDefault<URendererSettings>()->bEnableVirtualTextureOpacityMask == false)
	{
		//Virtual textures are not supported in the OpacityMask slot, convert any textures back to a regular texture.
		TArray<UTexture*> OutOpacityMaskTextures;
		if (ImportedMaterial->GetTexturesInPropertyChain(MP_OpacityMask, OutOpacityMaskTextures, nullptr, nullptr))
		{
			for (UTexture* CurrentTexture : OutOpacityMaskTextures)
			{
				UTexture2D* Texture2D = Cast<UTexture2D>(CurrentTexture);
				if (Texture2D && Texture2D->VirtualTextureStreaming)
				{
					ImportContext.AssetsContext.VirtualTexturesToConvert.Add(Texture2D);
				}
			}
		}
	}

	UDatasmithAssetImportData* AssetImportData = Cast< UDatasmithAssetImportData >(ImportedMaterial->AssetImportData);

	if (!AssetImportData)
	{
		AssetImportData = NewObject< UDatasmithAssetImportData >(ImportedMaterial);
		ImportedMaterial->AssetImportData = AssetImportData;
	}

	AssetImportData->Update(ImportContext.Options->FilePath, ImportContext.FileHash.IsValid() ? &ImportContext.FileHash : nullptr);
	AssetImportData->AssetImportOptions = ImportContext.Options->BaseOptions.AssetOptions;
	AssetImportData->DatasmithImportInfo = FDatasmithImportInfo(ImportContext.Options->SourceUri, ImportContext.Options->SourceHash);

	// Record requirements on mesh building for this material
	ImportContext.AssetsContext.MaterialsRequirements.Add( MaterialElement->GetName(), FDatasmithMaterialImporter::GetMaterialRequirements( ImportedMaterial ) );
	ImportContext.ImportedMaterials.Add( MaterialElement ) = ImportedMaterial;

	ImportMetaDataForObject( ImportContext, MaterialElement, ImportedMaterial );

	return ImportedMaterial;
}

UObject* FDatasmithImporter::FinalizeMaterial( UObject* SourceMaterial, const TCHAR* MaterialFolderPath, const TCHAR* TransientPackagePath, const TCHAR* RootFolderPath, UMaterialInterface* ExistingMaterial, TMap< UObject*, UObject* >* ReferencesToRemap, FMaterialUpdateContext* MaterialUpdateContext)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::FinalizeMaterial);

	// Finalizing the material instance might add a remapping for the instance parent property so make sure we have a remapping map available
	TOptional< TMap< UObject*, UObject* > > ReferencesToRemapLocal;
	if ( !ReferencesToRemap )
	{
		ReferencesToRemapLocal.Emplace();
		ReferencesToRemap = &ReferencesToRemapLocal.GetValue();
	}

	if ( UMaterialInstance* SourceMaterialInstance = Cast< UMaterialInstance >( SourceMaterial ) )
	{
		if ( UMaterialInterface* SourceMaterialParent = SourceMaterialInstance->Parent )
		{
			// Do not finalize parent material more than once by verifying it is not already in ReferencesToRemap
			if (!ReferencesToRemap->Contains(SourceMaterialParent))
			{
				FString SourceMaterialPath = SourceMaterialInstance->GetOutermost()->GetName();
				FString SourceParentPath = SourceMaterialParent->GetOutermost()->GetName();

				if ( SourceParentPath.StartsWith( TransientPackagePath ) )
				{
					// Simply finalize the source parent material.
					// Note that the parent material will be overridden on the existing material instance
					const FString DestinationParentPath = SourceParentPath.Replace( TransientPackagePath, RootFolderPath, ESearchCase::CaseSensitive );

					FinalizeMaterial( SourceMaterialParent, *DestinationParentPath, TransientPackagePath, RootFolderPath, nullptr, ReferencesToRemap );
				}
			}
		}
	}

	UMaterialEditingLibrary::DeleteAllMaterialExpressions( Cast< UMaterial >( ExistingMaterial ) );

	UObject* DestinationMaterial = FDatasmithImporterImpl::FinalizeAsset( SourceMaterial, MaterialFolderPath, ExistingMaterial, ReferencesToRemap );

	FDatasmithImporterImpl::CompileMaterial( DestinationMaterial, MaterialUpdateContext);

	return DestinationMaterial;
}

// ImportActors
// 作用: 生成 Import 场景 Actor 容器(ADatasmithSceneActor), 深度遍历场景描述生成 Actor/组件树。
// 特点: 分两趟 — 第一趟创建 Actor, 第二趟对 Camera 等需要引用信息的做 PostImport。
void FDatasmithImporter::ImportActors( FDatasmithImportContext& ImportContext )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportActors);

	/**
	 * Hot fix for reimport issues UE-71655. A temporary created actor might have the same object path as the previously deleted actor.
	 * This code below won't be needed when UE-71695 is fixed. This should be in 4.23.
	 */
	TArray< ADatasmithSceneActor* > SceneActors = FDatasmithImporterUtils::FindSceneActors( ImportContext.ActorsContext.FinalWorld, ImportContext.SceneAsset );
	for ( ADatasmithSceneActor* SceneActor : SceneActors )
	{
		if ( !SceneActor )
		{
			continue;
		}

		if ( ImportContext.SceneAsset == SceneActor->Scene && SceneActor->GetLevel() == ImportContext.ActorsContext.FinalWorld->GetCurrentLevel())
		{
			for ( TPair< FName, TSoftObjectPtr< AActor > >& Pair : SceneActor->RelatedActors )
			{
				// Try to load the actor. If we can't reset the soft object ptr
				if ( !Pair.Value.LoadSynchronous() )
				{
					Pair.Value.Reset();
				}
			}
		}
	}
	// end of the hotfix

	ADatasmithSceneActor* ImportSceneActor = ImportContext.ActorsContext.ImportSceneActor;

	// Create a scene actor to import with if we don't have one
	if ( !ImportSceneActor )
	{
		// Create a the import scene actor for the import context
		ImportSceneActor = FDatasmithImporterUtils::CreateImportSceneActor( ImportContext, FTransform::Identity );
	}

	const int32 ActorsCount = ImportContext.Scene->GetActorsCount();

	TUniquePtr<FScopedSlowTask> ProgressPtr;

	if ( ImportContext.FeedbackContext )
	{
		ProgressPtr = MakeUnique<FScopedSlowTask>( (float)ActorsCount, LOCTEXT("ImportActors", "Spawning actors..."), true, *ImportContext.FeedbackContext );
		ProgressPtr->MakeDialog(true);
	}

	FScopedSlowTask* Progress = ProgressPtr.Get();

	if ( ImportSceneActor )
	{
		ImportContext.Hierarchy.Push( ImportSceneActor->GetRootComponent() );

		FDatasmithActorUniqueLabelProvider UniqueNameProvider;

		for (int32 i = 0; i < ActorsCount && !ImportContext.bUserCancelled; ++i)
		{
			if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
			{
				ImportContext.bUserCancelled = true;
			}

			TSharedPtr< IDatasmithActorElement > ActorElement = ImportContext.Scene->GetActor(i);

			if ( ActorElement.IsValid() )
			{
				FDatasmithImporterImpl::ReportProgress( Progress, 1.f, FText::FromString( FString::Printf( TEXT("Spawning actor %d/%d (%s) ..."), ( i + 1 ), ActorsCount, ActorElement->GetLabel() ) ) );

				if ( ActorElement->IsAComponent() )
				{
					ImportActorAsComponent( ImportContext, ActorElement.ToSharedRef(), ImportSceneActor, UniqueNameProvider );
				}
				else
				{
					ImportActor( ImportContext, ActorElement.ToSharedRef() );
				}
			}
		}

		// After all actors were imported, perform a post import step so that any dependencies can be resolved
		for (int32 i = 0; i < ActorsCount && !ImportContext.bUserCancelled; ++i)
		{
			if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
			{
				ImportContext.bUserCancelled = true;
			}

			TSharedPtr< IDatasmithActorElement > ActorElement = ImportContext.Scene->GetActor(i);

			if ( ActorElement.IsValid() && ActorElement->IsA( EDatasmithElementType::Camera ) )
			{
				FDatasmithCameraImporter::PostImportCameraActor( StaticCastSharedRef< IDatasmithCameraActorElement >( ActorElement.ToSharedRef() ), ImportContext );
			}
		}

		ImportSceneActor->Scene = ImportContext.SceneAsset;

		ImportContext.Hierarchy.Pop();
	}

	// Sky
	if( ImportContext.Scene->GetUsePhysicalSky() )
	{
		AActor* SkyActor = FDatasmithLightImporter::CreatePhysicalSky(ImportContext);
	}

	if ( ImportContext.bUserCancelled )
	{
		FDatasmithImporterImpl::DeleteImportSceneActorIfNeeded(ImportContext.ActorsContext, true );
	}
}

// ImportActor / ImportActorAsComponent
// 作用: 根据元素类型分派到各专用 Importer (静态网格/灯光/相机/地形/后处理等) 生成 Actor 或组件。
// 递归: 深度优先构建层级, 未导入元素(策略忽略)记录到 NonImportedDatasmithActors。
AActor* FDatasmithImporter::ImportActor( FDatasmithImportContext& ImportContext, const TSharedRef< IDatasmithActorElement >& ActorElement )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::ImportActor);

	FDatasmithActorUniqueLabelProvider UniqueNameProvider;

	AActor* ImportedActor = nullptr;
	if (ActorElement->IsA(EDatasmithElementType::HierarchicalInstanceStaticMesh))
	{
		ImportedActor =  FDatasmithActorImporter::ImportHierarchicalInstancedStaticMeshAsActor( ImportContext, StaticCastSharedRef< IDatasmithHierarchicalInstancedStaticMeshActorElement >( ActorElement ), UniqueNameProvider );
	}
	else if (ActorElement->IsA(EDatasmithElementType::StaticMeshActor))
	{
		ImportedActor = FDatasmithActorImporter::ImportStaticMeshActor( ImportContext, StaticCastSharedRef< IDatasmithMeshActorElement >( ActorElement ) );
	}
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	else if (ActorElement->IsA(EDatasmithElementType::ClothActor))  // UE_DEPRECATED(5.5, "The experimental Cloth importer is no longer supported.")
	{
		ImportedActor = FDatasmithActorImporter::ImportClothActor(ImportContext, StaticCastSharedRef<IDatasmithClothActorElement>( ActorElement ) );
	}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	else if (ActorElement->IsA(EDatasmithElementType::EnvironmentLight))
	{
		ImportedActor = FDatasmithActorImporter::ImportEnvironment( ImportContext, StaticCastSharedRef< IDatasmithEnvironmentElement >( ActorElement ) );
	}
	else if (ActorElement->IsA(EDatasmithElementType::Light))
	{
		ImportedActor = FDatasmithActorImporter::ImportLightActor( ImportContext, StaticCastSharedRef< IDatasmithLightActorElement >(ActorElement) );
	}
	else if (ActorElement->IsA(EDatasmithElementType::Camera))
	{
		ImportedActor = FDatasmithActorImporter::ImportCameraActor( ImportContext, StaticCastSharedRef< IDatasmithCameraActorElement >(ActorElement) );
	}
	else if (ActorElement->IsA(EDatasmithElementType::Decal))
	{
		ImportedActor = FDatasmithActorImporter::ImportDecalActor( ImportContext, StaticCastSharedRef< IDatasmithDecalActorElement >(ActorElement), UniqueNameProvider );
	}
	else if (ActorElement->IsA(EDatasmithElementType::CustomActor))
	{
		ImportedActor = FDatasmithActorImporter::ImportCustomActor( ImportContext, StaticCastSharedRef< IDatasmithCustomActorElement >(ActorElement), UniqueNameProvider );
	}
	else if (ActorElement->IsA(EDatasmithElementType::Landscape))
	{
		ImportedActor = FDatasmithActorImporter::ImportLandscapeActor( ImportContext, StaticCastSharedRef< IDatasmithLandscapeElement >(ActorElement) );
	}
	else if (ActorElement->IsA(EDatasmithElementType::PostProcessVolume))
	{
		ImportedActor = FDatasmithPostProcessImporter::ImportPostProcessVolume( StaticCastSharedRef< IDatasmithPostProcessVolumeElement >( ActorElement ), ImportContext, ImportContext.Options->OtherActorImportPolicy );
	}
	else
	{
		ImportedActor = FDatasmithActorImporter::ImportBaseActor( ImportContext, ActorElement );
	}

	if ( ImportedActor ) // It's possible that we didn't import an actor (ie: the user doesn't want to import the cameras), in that case, we'll skip it in the hierarchy
	{
		ImportContext.Hierarchy.Push( ImportedActor->GetRootComponent() );
		ImportMetaDataForObject(ImportContext, ActorElement, ImportedActor);

		ImportContext.AddImportedActor(ImportedActor);

		if (ADatasmithSceneActor* DatasmithSceneActor = ImportContext.ActorsContext.ImportSceneActor)
		{
			DatasmithSceneActor->RelatedActors.FindOrAdd(ActorElement->GetName()) = ImportedActor;
		}
	}
	else
	{
		ImportContext.ActorsContext.NonImportedDatasmithActors.Add( ActorElement->GetName() );
	}

	for (int32 i = 0; i < ActorElement->GetChildrenCount() && !ImportContext.bUserCancelled; ++i)
	{
		if (FDatasmithImporterImpl::HasUserCancelledTask(ImportContext.FeedbackContext))
		{
			ImportContext.bUserCancelled = true;
		}

		const TSharedPtr< IDatasmithActorElement >& ChildActorElement = ActorElement->GetChild(i);

		if ( ChildActorElement.IsValid() )
		{
			if (!ChildActorElement->IsAComponent() )
			{
				ImportActor( ImportContext, ChildActorElement.ToSharedRef() );
			}
			else if ( ImportedActor ) // Don't import the components of an actor that we didn't import
			{
				ImportActorAsComponent( ImportContext, ChildActorElement.ToSharedRef(), ImportedActor, UniqueNameProvider );
			}
		}
	}

	if ( ImportedActor )
	{
		ImportContext.Hierarchy.Pop();
	}

	return ImportedActor;
}

// ImportActorAsComponent
// 作用: 将 ActorElement 作为组件导入到指定根 Actor 下, 生成场景组件并登记到 ImportContext。
// 递归: 深度优先导入所有子级。
void FDatasmithImporter::ImportActorAsComponent(FDatasmithImportContext& ImportContext, const TSharedRef< IDatasmithActorElement >& ActorElement, AActor* InRootActor, FDatasmithActorUniqueLabelProvider& UniqueNameProvider)
{
	if (!InRootActor)
	{
		return;
	}

	USceneComponent* SceneComponent = nullptr;

	if (ActorElement->IsA(EDatasmithElementType::HierarchicalInstanceStaticMesh))
	{
		TSharedRef< IDatasmithHierarchicalInstancedStaticMeshActorElement > HierarchicalInstancedStaticMeshElement = StaticCastSharedRef< IDatasmithHierarchicalInstancedStaticMeshActorElement >(ActorElement);
		SceneComponent = FDatasmithActorImporter::ImportHierarchicalInstancedStaticMeshComponent(ImportContext, HierarchicalInstancedStaticMeshElement, InRootActor, UniqueNameProvider);
	}
	else if (ActorElement->IsA(EDatasmithElementType::StaticMeshActor))
	{
		TSharedRef< IDatasmithMeshActorElement > MeshActorElement = StaticCastSharedRef< IDatasmithMeshActorElement >(ActorElement);
		SceneComponent = FDatasmithActorImporter::ImportStaticMeshComponent(ImportContext, MeshActorElement, InRootActor, UniqueNameProvider);
	}
	else if (ActorElement->IsA(EDatasmithElementType::Light))
	{
		if (ImportContext.Options->LightImportPolicy == EDatasmithImportActorPolicy::Ignore)
		{
			return;
		}

		SceneComponent = FDatasmithLightImporter::ImportLightComponent(StaticCastSharedRef< IDatasmithLightActorElement >(ActorElement), ImportContext, InRootActor, UniqueNameProvider);
	}
	else if (ActorElement->IsA(EDatasmithElementType::Camera))
	{
		if (ImportContext.Options->CameraImportPolicy == EDatasmithImportActorPolicy::Ignore)
		{
			return;
		}

		SceneComponent = FDatasmithCameraImporter::ImportCineCameraComponent(StaticCastSharedRef< IDatasmithCameraActorElement >(ActorElement), ImportContext, InRootActor, UniqueNameProvider);
	}
	else
	{
		SceneComponent = FDatasmithActorImporter::ImportBaseActorAsComponent(ImportContext, ActorElement, InRootActor, UniqueNameProvider);
	}

	if (SceneComponent)
	{
		ImportContext.AddSceneComponent(SceneComponent->GetName(), SceneComponent);
		ImportMetaDataForObject(ImportContext, ActorElement, SceneComponent);
	}
	else
	{
		ImportContext.ActorsContext.NonImportedDatasmithActors.Add(ActorElement->GetName());
	}

	for (int32 i = 0; i < ActorElement->GetChildrenCount(); ++i)
	{
		if (SceneComponent) // If we didn't import the current component, skip it in the hierarchy
		{
			ImportContext.Hierarchy.Push(SceneComponent);
		}

		ImportActorAsComponent(ImportContext, ActorElement->GetChild(i).ToSharedRef(), InRootActor, UniqueNameProvider);

		if (SceneComponent)
		{
			ImportContext.Hierarchy.Pop();
		}
	}
}

// FinalizeActors
// 作用: 将临时 ImportSceneActor 中的 RelatedActors 迁移/合并到最终关卡 SceneActor。
// 步骤: 去除未导入 Actor -> 复制/迁移模板与组件 -> 修复引用 -> 软路径重定向 -> 层同步。
void FDatasmithImporter::FinalizeActors( FDatasmithImportContext& ImportContext, TMap< UObject*, UObject* >* AssetReferencesToRemap )
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FDatasmithImporter::FinalizeActors);

	if ( !ImportContext.bUserCancelled )
	{
		// Ensure a proper setup for the finalize of the actors
		auto& ImportSceneActor = ImportContext.ActorsContext.ImportSceneActor;
		TSet< ADatasmithSceneActor* >& FinalSceneActors = ImportContext.ActorsContext.FinalSceneActors;

		if ( !ImportContext.ActorsContext.FinalWorld )
		{
			ImportContext.ActorsContext.FinalWorld = ImportContext.ActorsContext.ImportWorld;
		}
		else if ( !ImportContext.bIsAReimport && ImportSceneActor )
		{
				//Create a new datasmith scene actor in the final level
				FActorSpawnParameters SpawnParameters;
				SpawnParameters.Template = ImportSceneActor;
				ADatasmithSceneActor* DestinationSceneActor = ImportContext.ActorsContext.FinalWorld->SpawnActor< ADatasmithSceneActor >(SpawnParameters);

				// Name new destination ADatasmithSceneActor to the DatasmithScene's name
				DestinationSceneActor->SetActorLabel( ImportContext.Scene->GetName() );
				DestinationSceneActor->MarkPackageDirty();
				DestinationSceneActor->RelatedActors.Reset();

				// Workaround for UE-94255. We should be able to remove this when UE-76028 is fixed
				TArray<UObject*> SubObjects;
				GetObjectsWithOuter( DestinationSceneActor, SubObjects );
				for ( UObject* SubObject : SubObjects )
				{
					if ( UAssetUserData* AssetUserData = Cast<UAssetUserData>( SubObject ) )
					{
						AssetUserData->SetFlags( RF_Public );
					}
				}

				FinalSceneActors.Empty( 1 );
				FinalSceneActors.Add( DestinationSceneActor );
		}

		if( FinalSceneActors.Num() == 0 )
		{
			if ( ImportContext.bIsAReimport )
			{
				FinalSceneActors.Append( FDatasmithImporterUtils::FindSceneActors( ImportContext.ActorsContext.FinalWorld, ImportContext.SceneAsset ) );
				FinalSceneActors.Remove( ImportSceneActor );
			}
			else
			{
				FinalSceneActors.Add( ImportSceneActor );
			}
		}

		for ( AActor* Actor : FinalSceneActors )
		{
			check(Actor->GetWorld() == ImportContext.ActorsContext.FinalWorld);
		}


		// Do the finalization for each actor from each FinalSceneActor
		TMap< FSoftObjectPath, FSoftObjectPath > RenamedActorsMap;
		TSet< FName > LayersUsedByActors;
		const bool bShouldSpawnNonExistingActors = !ImportContext.bIsAReimport || ImportContext.Options->ReimportOptions.bRespawnDeletedActors;

		TArray<uint8> ReusableBuffer;
		for ( ADatasmithSceneActor* DestinationSceneActor : FinalSceneActors )
		{
			if ( !DestinationSceneActor )
			{
				continue;
			}

			if ( ImportSceneActor->Scene != DestinationSceneActor->Scene  || DestinationSceneActor->GetLevel() != ImportContext.ActorsContext.FinalWorld->GetCurrentLevel())
			{
				continue;
			}

			// In order to allow modification on components owned by DestinationSceneActor, unregister all of them
			DestinationSceneActor->UnregisterAllComponents( /* bForReregister = */true);

			ImportContext.ActorsContext.CurrentTargetedScene = DestinationSceneActor;

			if ( ImportSceneActor != DestinationSceneActor )
			{
				// Before we delete the non imported actors, remove the old actor labels from the unique name provider
				// as we don't care if the source labels clash with labels from actors that will be deleted or replaced on reimport
				for (const TPair< FName, TSoftObjectPtr< AActor > >& ActorPair : DestinationSceneActor->RelatedActors)
				{
					if ( AActor* DestActor = ActorPair.Value.Get() )
					{
						ImportContext.ActorsContext.UniqueNameProvider.RemoveExistingName(DestActor->GetActorLabel());
					}
				}

				FDatasmithImporterUtils::DeleteNonImportedDatasmithElementFromSceneActor( *ImportSceneActor, *DestinationSceneActor, ImportContext.ActorsContext.NonImportedDatasmithActors );


				// Update destination actor's metadata
				if (IInterface_AssetUserData* SourceAssetUserData = Cast<IInterface_AssetUserData>(ImportSceneActor->GetRootComponent()))
				{
					if (UDatasmithAssetUserData* SourceDatasmithUserData = SourceAssetUserData->GetAssetUserData<UDatasmithAssetUserData>())
					{
						USceneComponent* SceneComponent = DestinationSceneActor->GetRootComponent();

						if (IInterface_AssetUserData* AssetUserData = Cast<IInterface_AssetUserData>(SceneComponent))
						{
							UDatasmithAssetUserData* DatasmithUserData = AssetUserData->GetAssetUserData<UDatasmithAssetUserData>();

							if (!DatasmithUserData)
							{
								DatasmithUserData = NewObject<UDatasmithAssetUserData>(SceneComponent, NAME_None, RF_Public | RF_Transactional);
								AssetUserData->AddAssetUserData(DatasmithUserData);
							}

							DatasmithUserData->MetaData.Append(SourceDatasmithUserData->MetaData);
						}
					}
				}
			}

			// Add Actor info to the remap info
			TMap< UObject*, UObject* > PerSceneActorReferencesToRemap = AssetReferencesToRemap ? *AssetReferencesToRemap : TMap< UObject*, UObject* >();
			PerSceneActorReferencesToRemap.Add( ImportSceneActor ) = DestinationSceneActor;
			PerSceneActorReferencesToRemap.Add( ImportSceneActor->GetRootComponent() ) = DestinationSceneActor->GetRootComponent();

			// #ueent_todo order of actors matters for ReferencesFix + re-parenting
			for ( const TPair< FName, TSoftObjectPtr< AActor > >& SourceActorPair : ImportSceneActor->RelatedActors )
			{
				AActor* SourceActor = SourceActorPair.Value.Get();
				if ( SourceActor == nullptr )
				{
					continue;
				}

				const bool bActorIsRelatedToDestinationScene = DestinationSceneActor->RelatedActors.Contains( SourceActorPair.Key );
				TSoftObjectPtr< AActor >& ExistingActorPtr = DestinationSceneActor->RelatedActors.FindOrAdd( SourceActorPair.Key );
				const bool bShouldFinalizeActor = bShouldSpawnNonExistingActors || !bActorIsRelatedToDestinationScene || ( ExistingActorPtr.Get() && !ExistingActorPtr.Get()->IsPendingKillPending() );

				if ( bShouldFinalizeActor )
				{
					// Remember the original source path as FinalizeActor may set SourceActor's label, which apparently can also change its Name and package path
					FSoftObjectPath OriginalSourcePath = FSoftObjectPath(SourceActor);
					AActor* DestinationActor = FinalizeActor( ImportContext, *SourceActor, ExistingActorPtr.Get(), PerSceneActorReferencesToRemap, ReusableBuffer );
					RenamedActorsMap.Add(OriginalSourcePath, FSoftObjectPath(DestinationActor));
					LayersUsedByActors.Append(DestinationActor->Layers);
					ExistingActorPtr = DestinationActor;
				}
			}

			for (  const TPair< FName, TSoftObjectPtr< AActor > >& DestinationActorPair : DestinationSceneActor->RelatedActors )
			{
				if ( AActor* Actor = DestinationActorPair.Value.Get() )
				{
					FDatasmithImporterImpl::FixReferencesForObject( Actor, PerSceneActorReferencesToRemap );
				}
			}

			// Modification is completed, re-register all components owned by DestinationSceneActor
			DestinationSceneActor->RegisterAllComponents();
		}

		// Add the missing layers to the final world
		FDatasmithImporterUtils::AddUniqueLayersToWorld( ImportContext.ActorsContext.FinalWorld, LayersUsedByActors );

		// Fixed the soft object paths that were pointing to our pre-finalized actors.
		TArray< UPackage* > PackagesToFix;

		for ( const TPair< FName, TSoftObjectPtr< ULevelSequence > >& LevelSequencePair : ImportContext.SceneAsset->LevelSequences )
		{
			if (LevelSequencePair.Value)
			{
				PackagesToFix.Add( LevelSequencePair.Value->GetOutermost() );
			}
		}

		for ( const TPair< FName, TSoftObjectPtr< ULevelVariantSets > >& LevelVariantSetsPair : ImportContext.SceneAsset->LevelVariantSets )
		{
			if (LevelVariantSetsPair.Value)
			{
				PackagesToFix.Add( LevelVariantSetsPair.Value->GetOutermost() );
			}
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked< FAssetToolsModule >(TEXT("AssetTools"));
		AssetToolsModule.Get().RenameReferencingSoftObjectPaths( PackagesToFix, RenamedActorsMap );
	}


	if ( ImportContext.ShouldImportActors() )
	{
		FinalizeActors( ImportContext, &ReferencesToRemap );
	}

	// Everything has been finalized, make sure the UDatasmithScene is set to dirty
	if ( ImportContext.SceneAsset )
	{
		ImportContext.SceneAsset->MarkPackageDirty();
	}

	FGlobalComponentReregisterContext RecreateComponents;

	// Flush the transaction buffer (eg. avoid corrupted hierarchies after repeated undo actions)
	// This is an aggressive workaround while we don't properly support undo history.
	GEditor->ResetTransaction(LOCTEXT("Reset Transaction Buffer", "Datasmith Import Finalization"));
}

#undef LOCTEXT_NAMESPACE
