#pragma once

#include "CoreMinimal.h"
#include "FlickSpectatorView.generated.h"

// Presentation only. Never used to aim, validate shots, or move physics bodies.
USTRUCT()
struct FFlickSpectatorView
{
	GENERATED_BODY()

	UPROPERTY() FVector_NetQuantize10 Location = FVector::ZeroVector;
	UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
	UPROPERTY() float FieldOfView = 49.2f;
	UPROPERTY() bool bValid = false;

	bool IsSafe() const
	{
		return !Location.ContainsNaN() && !Rotation.ContainsNaN()
			&& Location.GetAbsMax() < 15000.0 && FMath::IsFinite(FieldOfView)
			&& FieldOfView >= 20.0f && FieldOfView <= 120.0f;
	}
};
