// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ObjectTemplates/DatasmithObjectTemplate.h"

#include "DatasmithDecalComponentTemplate.generated.h"

#define UE_API DATASMITHCONTENT_API

UCLASS(MinimalAPI)
class UDatasmithDecalComponentTemplate : public UDatasmithObjectTemplate
{
	GENERATED_BODY()

public:
	UE_API UDatasmithDecalComponentTemplate();

	UPROPERTY()
	int32 SortOrder;

	UPROPERTY()
	FVector DecalSize;

	UPROPERTY()
	TObjectPtr<class UMaterialInterface> Material;

	UE_API virtual UObject* UpdateObject( UObject* Destination, bool bForce = false ) override;
	UE_API virtual void Load( const UObject* Source ) override;
	UE_API virtual bool Equals( const UDatasmithObjectTemplate* Other ) const override;
};

#undef UE_API
