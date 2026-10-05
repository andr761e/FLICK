#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

// Presentation-only geometry. Never enables collision or participates in physics.
namespace FlickCosmeticGeometry
{
	inline void BuildRing(UProceduralMeshComponent& Mesh, const int32 Section, const int32 Sides = 96)
	{
		TArray<FVector> Vertices, Normals;
		TArray<FVector2D> UVs;
		TArray<int32> Indices;
		for (int32 Index = 0; Index <= Sides; ++Index)
		{
			const float Angle = 2.0f * PI * Index / Sides;
			const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			Vertices.Add(Direction * 0.975f);
			Vertices.Add(Direction * 1.025f);
			Normals.Append({FVector::UpVector, FVector::UpVector});
			UVs.Append({FVector2D(static_cast<float>(Index) / Sides, 0.0f), FVector2D(static_cast<float>(Index) / Sides, 1.0f)});
			if (Index < Sides)
			{
				const int32 A = Index * 2;
				// Both faces: portals also remain visible from low cinematic cameras.
				Indices.Append({A, A + 2, A + 1, A + 1, A + 2, A + 3,
					A + 1, A + 2, A, A + 3, A + 2, A + 1});
			}
		}
		Mesh.CreateMeshSection(Section, Vertices, Indices, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	}
}
