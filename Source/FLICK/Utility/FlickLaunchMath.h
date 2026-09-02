#pragma once

#include "CoreMinimal.h"

struct FFlickLaunchResult
{
	bool bValidShot = false;
	FVector Direction = FVector::ZeroVector;
	float NormalizedPower = 0.0f;
	float DragDistance = 0.0f;
};

namespace FlickLaunchMath
{
	FFlickLaunchResult CalculateLaunch(
		const FVector& PieceLocation,
		const FVector& CursorWorldLocation,
		float MaxDragDistance,
		float MinDragDistance,
		float PowerExponent);
}
