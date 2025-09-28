// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParametricSurfaceData.h"

#include "CADKernelEnginePrivate.h"
#include "MeshUtilities.h"
#include "TechSoft/TechSoftUtilities.h"

#include "Core/Session.h"
#include "Topo/Model.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/EnterpriseObjectVersion.h"

#ifdef WITH_HOOPS
bool FCADKernelTessellationSettings::bWithHoops = true;
#else
bool FCADKernelTessellationSettings::bWithHoops = false;
#endif

#define LOCTEXT_NAMESPACE "ParametricSurfaceData"

const double FUnitConverter::CentimeterToMillimeter = 10.;
const double FUnitConverter::MillimeterToCentimeter = 0.1;
const double FUnitConverter::CentimeterToMeter = 0.01;
const double FUnitConverter::MeterToCentimeter = 100.;
const double FUnitConverter::MillimeterToMeter = 0.001;
const double FUnitConverter::MeterToMillimeter = 1000.;

bool UParametricSurfaceData::SetFromFile(const TCHAR* FilePath, bool bForTechSoft)
{
	if (FPaths::FileExists(FilePath))
	{
		TArray<uint8> ByteArray;

		if (FFileHelper::LoadFileToArray(ByteArray, FilePath))
		{
			TArray<uint8>& RawData = bForTechSoft ? TechSoftRawData : CADKernelRawData;
			RawData = MoveTemp(ByteArray);
			return true;
		}
	}

	return false;
}

void UParametricSurfaceData::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FEnterpriseObjectVersion::GUID);

	if (Ar.IsLoading() && Ar.CustomVer(FEnterpriseObjectVersion::GUID) < FEnterpriseObjectVersion::AddedParametricSurfaceData)
	{
		// #cad_import_todo : Put code to deserialize UDatasmithParametricSurfaceData
		 return;
	}

	Super::Serialize(Ar);

	Ar << CADKernelRawData;
	Ar << TechSoftRawData;
}

TSharedPtr<UE::CADKernel::FModel> UParametricSurfaceData::GetModel()
{
#if PLATFORM_DESKTOP
	using namespace UE::CADKernel;

	if (CADKernelRawData.IsEmpty())
	{
		return {};
	}

	TSharedRef<FSession> Session = MakeShared<FSession>(LastTessellationSettings.GetGeometricTolerance(true));
	Session->AddDatabase(CADKernelRawData);

	return Session->GetModelAsShared();
#else
	return {};
#endif
}

A3DRiRepresentationItem* UParametricSurfaceData::GetRepresentation()
{
#ifdef WITH_HOOPS
	using namespace UE::CADKernel;

	if (TechSoftRawData.IsEmpty())
	{
		return {};
	}

	return FTechSoftUtilities::GetRepresentation(TechSoftRawData);
#else
	return nullptr;
#endif
}


bool UParametricSurfaceData::Tessellate(UE::Geometry::FDynamicMesh3& MeshOut)
{
#if PLATFORM_DESKTOP
	return UE::CADKernel::Private::Tessellate(*this, MeshOut);
#else
	return false;
#endif
}

bool UParametricSurfaceData::Tessellate(FMeshDescription& MeshOut)
{
#if PLATFORM_DESKTOP
	return UE::CADKernel::Private::Tessellate(*this, MeshOut);
#else
	return false;
#endif
}

bool UParametricSurfaceData::Retessellate(const FCADKernelRetessellationSettings& Settings, UE::Geometry::FDynamicMesh3& MeshOut)
{
#if PLATFORM_DESKTOP
	return UE::CADKernel::Private::Retessellate(*this, Settings, MeshOut);
#else
	return false;
#endif
}

bool UParametricSurfaceData::Retessellate(const FCADKernelRetessellationSettings& Settings, FMeshDescription& MeshOut)
{
#if PLATFORM_DESKTOP
	return UE::CADKernel::Private::Retessellate(*this, Settings, MeshOut);
#else
	return false;
#endif
}

bool UParametricSurfaceData::SetModel(TSharedPtr<UE::CADKernel::FModel>& Model, double UnitModelToCentimeter)
{
#if PLATFORM_DESKTOP
	ModelParameters.ModelUnitToCentimeter = UnitModelToCentimeter;
	return UE::CADKernel::Private::ToRawData(Model, CADKernelRawData);
#else
	return false;
#endif
}

bool UParametricSurfaceData::SetRepresentation(A3DRiRepresentationItem* Representation, int32 MaterialID, double UnitRepresentationToCentimeter)
{
#ifdef WITH_HOOPS
	ModelParameters.ModelUnitToCentimeter = UnitRepresentationToCentimeter;
	return UE::CADKernel::TechSoftUtilities::ToRawData(Representation, MaterialID, TechSoftRawData);
#else
	return false;
#endif
}

#undef LOCTEXT_NAMESPACE // "ParametricSurfaceData"
