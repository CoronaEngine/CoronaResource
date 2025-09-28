// Copyright Epic Games, Inc. All Rights Reserved.

#include "CADKernelEngine.h"

#if PLATFORM_DESKTOP
#include "CADKernelEnginePrivate.h"
#include "MeshUtilities.h"

#include "Core/Session.h"
#include "Topo/Body.h"
#include "Topo/Model.h"
#include "Topo/Shell.h"
#include "Topo/TopologicalFace.h"

#include "CADKernelEngineLog.h"

#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogCADKernelEngine);

namespace UE::CADKernel
{
	using namespace UE::CADKernel::MeshUtilities;

	static bool GUseEngine = false;
	FAutoConsoleVariableRef GCADKernelDebugUseEngine(
		TEXT("CADKernel.Debug.UseEngine"),
		GUseEngine,
		TEXT(""),
		ECVF_Default);

	FTessellationContext::FTessellationContext(const FCADKernelModelParameters& InModelParams, const FCADKernelMeshParameters& InMeshParams, const FCADKernelRetessellationSettings& Settings)
	{
		ModelParams = InModelParams;
		MeshParams = InMeshParams;
		TessellationSettings = Settings;
		bResolveTJunctions = TessellationSettings.bResolveTJunctions;
	}

	bool FCADKernelUtilities::Save(TSharedPtr<UE::CADKernel::FModel>& Model, const FString& FilePath)
	{
		TSharedPtr<FSession> Session = FEntity::MakeShared<FSession>(0.01);
		Session->GetModel().Copy(*Model);

		return Session->SaveDatabase(*FilePath);
	}

	bool FCADKernelUtilities::Load(TSharedPtr<UE::CADKernel::FModel>& Model, const FString& FilePath)
	{
		TSharedPtr<FSession> Session = FEntity::MakeShared<FSession>(0.01);

		if (Session->LoadDatabase(*FilePath))
		{
			Model->Copy(Session->GetModel());
			return true;
		}

		return false;
	}

	bool FCADKernelUtilities::Tessellate(UE::CADKernel::FModel& Model, const FTessellationContext& Context, FMeshDescription& Mesh, bool bEmptyMesh)
	{
		using namespace UE::CADKernel::MeshUtilities;

		TSharedPtr<FMeshWrapperAbstract> MeshWrapper = FMeshWrapperAbstract::MakeWrapper(Context, Mesh);
		return Private::Tessellate(Model, Context, *MeshWrapper, bEmptyMesh);
	}

	bool FCADKernelUtilities::Tessellate(UE::CADKernel::FModel& Model, const FTessellationContext& Context, UE::Geometry::FDynamicMesh3& Mesh, bool bEmptyMesh)
	{
		using namespace UE::CADKernel::MeshUtilities;

		TSharedPtr<FMeshWrapperAbstract> MeshWrapper = FMeshWrapperAbstract::MakeWrapper(Context, Mesh);
		return Private::Tessellate(Model, Context, *MeshWrapper, bEmptyMesh);
	}
}
#endif
