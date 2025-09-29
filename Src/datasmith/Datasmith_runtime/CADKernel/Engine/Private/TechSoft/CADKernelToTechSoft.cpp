// Copyright Epic Games, Inc. All Rights Reserved.

#include "CADKernelEngine.h"

#if PLATFORM_DESKTOP
#ifdef WITH_HOOPS
#include "TechSoftUniqueObjectImpl.h"

namespace UE::CADKernel
{
	A3DRiRepresentationItem* FTechSoftUtilities::CADKernelToTechSoft(TSharedPtr<FModel>& Model)
	{
		if (!FTechSoftLibrary::IsInitialized())
		{
			return nullptr;
		}

		return nullptr;
	}
}
#else
namespace UE::CADKernel
{
	A3DRiRepresentationItem* FTechSoftUtilities::CADKernelToTechSoft(TSharedPtr<FModel>& Model)
	{
		return nullptr;
	}
}
#endif
#endif

