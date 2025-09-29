// Copyright Epic Games, Inc. All Rights Reserved.
#include "Topo/TopologicalShapeEntity.h"

namespace UE::CADKernel
{

void FTopologicalShapeEntity::CompleteMetaDataWithHostMetaData()
{
	if (HostedBy != nullptr)
	{
		Dictionary.CompleteDictionary(HostedBy->GetMetaDataDictionary());
	}
}

}
