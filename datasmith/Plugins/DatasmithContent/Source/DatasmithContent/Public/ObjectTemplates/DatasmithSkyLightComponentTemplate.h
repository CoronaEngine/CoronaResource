// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ObjectTemplates/DatasmithObjectTemplate.h"

#include "Components/SkyLightComponent.h"

#include "DatasmithSkyLightComponentTemplate.generated.h"

#define UE_API DATASMITHCONTENT_API

UCLASS(MinimalAPI)
class UDatasmithSkyLightComponentTemplate : public UDatasmithObjectTemplate
{
	GENERATED_BODY()

public:
	UE_API UDatasmithSkyLightComponentTemplate();

	UPROPERTY()
	TEnumAsByte< ESkyLightSourceType > SourceType;

	UPROPERTY()
	int32 CubemapResolution;

	UPROPERTY()
	TObjectPtr<class UTextureCube> Cubemap;

	UE_API virtual UObject* UpdateObject( UObject* Destination, bool bForce = false ) override;
	UE_API virtual void Load( const UObject* Source ) override;
	UE_API virtual bool Equals( const UDatasmithObjectTemplate* Other ) const override;
};

#undef UE_API
