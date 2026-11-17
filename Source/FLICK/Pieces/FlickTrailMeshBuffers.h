#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

// Per-puck scratch storage. Reset retains capacity between rendered frames;
// buffers contain no UObject references and are not part of replay/physics state.
struct FFlickTrailMeshBuffers
{
	TArray<FVector> Vertices, Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Indices;
	void Reset(int32 VertexCapacity, int32 IndexCapacity);
	void Pair(const FVector& Center, const FVector& Side, float Width, FLinearColor Color, float U);
	void Quad(const FVector& Center, const FVector& Right, const FVector& Up, float Size, float Height, FLinearColor Color);
	void Upload(UProceduralMeshComponent* Mesh, int32 Section) const;
};
