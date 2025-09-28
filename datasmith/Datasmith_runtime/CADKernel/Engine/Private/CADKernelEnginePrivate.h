// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#if PLATFORM_DESKTOP
#include "CADKernelEngine.h"
#include "MeshUtilities.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace UE::CADKernel
{
	class FModel;

	struct FTessellationContext;

	namespace MeshUtilities
	{
		class FMeshWrapperAbstract;
	}

	namespace Private
	{

		template<typename MeshType>
		bool Tessellate(UParametricSurfaceData& Data, MeshType& MeshOut)
		{
			using namespace UE::CADKernel;

			bool bTessellationSuccessful = false;

			FTessellationContext Context(Data.GetModelParameters(), Data.GetMeshParameters(), Data.GetLastTessellationSettings());

			if (Context.TessellationSettings.bUseCADKernel)
			{
				TSharedPtr<FModel> Model = Data.GetModel();
				if (Model.IsValid())
				{
					bTessellationSuccessful = FCADKernelUtilities::Tessellate(*Model, Context, MeshOut);
				}
			}
			else if (FTechSoftLibrary::Initialize())
			{
				A3DRiRepresentationItem* Representation = Data.GetRepresentation();
				bTessellationSuccessful = FTechSoftUtilities::Tessellate(Representation, Context, MeshOut);
			}

			return bTessellationSuccessful;
		}

		template<typename MeshType>
		bool Retessellate(UParametricSurfaceData& Data, const FCADKernelRetessellationSettings& Settings, MeshType& MeshOut)
		{
			using namespace UE::CADKernel;
			using namespace UE::CADKernel::MeshUtilities;

			bool bTessellationSuccessful = false;

			FTessellationContext Context(Data.GetModelParameters(), Data.GetMeshParameters(), Settings);

			if (Settings.RetessellationRule == ECADKernelRetessellationRule::SkipDeletedFaces)
			{
				GetExistingFaceGroups(MeshOut, Context.FaceGroupsToExtract);
			}

			if (Context.TessellationSettings.bUseCADKernel)
			{
				TSharedPtr<FModel> Model = Data.GetModel();
				if (Model.IsValid())
				{
					bTessellationSuccessful = FCADKernelUtilities::Tessellate(*Model, Context, MeshOut, true);
				}
			}
			else if(FTechSoftLibrary::Initialize())
			{
				if (A3DRiRepresentationItem* Representation = Data.GetRepresentation())
				{
					bTessellationSuccessful = FTechSoftUtilities::Tessellate(Representation, Context, MeshOut, true);
				}
			}

			return bTessellationSuccessful;
		}

		bool Tessellate(FModel& Model, const UE::CADKernel::FTessellationContext& Context, UE::CADKernel::MeshUtilities::FMeshWrapperAbstract& MeshWrapper, bool bEmptyMesh = false);

		// For future use
		bool GetFaceTrimmingCurves(const UE::CADKernel::FModel& Model, const UE::CADKernel::FTopologicalFace& Face, TArray<TArray<TArray<FVector>>>& CurvesOut);
		bool GetFaceTrimming2DPolylines(const UE::CADKernel::FModel& Model, const UE::CADKernel::FTopologicalFace& Face, TArray<TArray<FVector2d>>& PolylinesOut);
		bool GetFaceTrimming3DPolylines(const UE::CADKernel::FModel& Model, const UE::CADKernel::FTopologicalFace& Face, TArray<TArray<FVector>>& PolylinesOut);

		bool AddModelMesh(const UE::CADKernel::FModelMesh& ModelMesh, UE::CADKernel::MeshUtilities::FMeshWrapperAbstract& MeshWrapper);
		bool ToRawData(TSharedPtr<UE::CADKernel::FModel>& Model, TArray<uint8>& RawDataOut);
	}
}
#endif
