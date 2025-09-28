// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#ifdef USE_TECHSOFT_SDK

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

THIRD_PARTY_INCLUDES_START
#pragma push_macro("TEXT")
#pragma warning(push)
#pragma warning(disable:4191) // unsafe sprintf
#include "A3DSDKIncludes.h"
#pragma warning(pop)
#pragma pop_macro("TEXT")
THIRD_PARTY_INCLUDES_END

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define WITH_HOOPS
#endif
