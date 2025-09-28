// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ObjectTemplates/DatasmithSceneComponentTemplate.h"

#include "DatasmithStaticMeshComponentTemplate.generated.h"

#define UE_API DATASMITHCONTENT_API

UCLASS(MinimalAPI)
class UDatasmithStaticMeshComponentTemplate : public UDatasmithObjectTemplate
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TObjectPtr<class UStaticMesh> StaticMesh;

	UPROPERTY()
	TArray< TObjectPtr<class UMaterialInterface> > OverrideMaterials;

	UE_API virtual UObject* UpdateObject( UObject* Destination, bool bForce = false ) override;
	UE_API virtual void Load( const UObject* Source ) override;
	UE_API virtual bool Equals( const UDatasmithObjectTemplate* Other ) const override;
};

#undef UE_API
