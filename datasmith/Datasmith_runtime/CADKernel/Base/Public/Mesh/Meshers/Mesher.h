// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core/Chrono.h"
#include "Core/Types.h"

namespace UE::CADKernel
{
class FModelMesh;
class FTopologicalShapeEntity;

class CADKERNEL_API FMesher
{
protected:

	double GeometricTolerance;
	bool bThinZoneMeshing = false;

	FModelMesh& MeshModel;

public:

	FMesher(FModelMesh& InMeshModel, double GeometricTolerance, bool bActivateThinZoneMeshing);

	void MeshEntities(TArray<FTopologicalShapeEntity*>& InEntities);

	void MeshEntity(FTopologicalShapeEntity& InEntity)
	{
		TArray<FTopologicalShapeEntity*> Entities;
		Entities.Add(&InEntity);
		MeshEntities(Entities);
	}
};

} // namespace UE::CADKernel

