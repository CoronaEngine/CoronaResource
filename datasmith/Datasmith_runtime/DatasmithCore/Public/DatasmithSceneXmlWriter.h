// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

#define UE_API DATASMITHCORE_API

class FArchive;
class IDatasmithScene;

class FDatasmithSceneXmlWriter
{
public:
	UE_API void Serialize( TSharedRef< IDatasmithScene > DatasmithScene, FArchive& Archive );
};

#undef UE_API
