// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core/Types.h"

#include "UI/Message.h"

namespace UE::CADKernel
{
class CADKERNEL_API FConsole
{
public:

	virtual ~FConsole()
	{
	}

	virtual void Print(const TCHAR* Texte, EVerboseLevel VerboseLevel = EVerboseLevel::NoVerbose)
	{}

};

} // namespace UE::CADKernel

