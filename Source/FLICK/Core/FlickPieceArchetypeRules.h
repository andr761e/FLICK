#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

struct FLICK_API FFlickPieceArchetypeRules
{
	float MassMultiplier = 1.0f;
	float FrictionMultiplier = 1.0f;
	float RestitutionMultiplier = 1.0f;
	float LinearDampingMultiplier = 1.0f;
	float AngularDampingMultiplier = 1.0f;
	float LaunchSpeedMultiplier = 1.0f;
	float RadiusMultiplier = 1.0f;
	float ThicknessMultiplier = 1.0f;
	float CenterOfMassHeightFraction = 0.0f;
	FLinearColor AccentColor = FLinearColor::White;
	FString ClassLabel = TEXT("CORE");
	FString Summary = TEXT("BALANCED | RELIABLE");
	FString Strengths = TEXT("RELIABILITY | FLEXIBILITY");
	FString Weaknesses = TEXT("NO SPECIALTY");
};

struct FLICK_API FFlickPieceDisplayStats
{
	float Speed = 0.5f;
	float Weight = 0.5f;
	float Impact = 0.5f;
	float Control = 0.5f;
	float Coast = 0.5f;
	float Stability = 0.5f;
};

namespace FlickPieceArchetypeRules
{
	inline constexpr int32 ArchetypeCount = static_cast<int32>(EFlickPieceArchetype::Toppler) + 1;
	FLICK_API const FFlickPieceArchetypeRules& Get(EFlickPieceArchetype Archetype);
	FLICK_API FFlickPieceDisplayStats GetDisplayStats(EFlickPieceArchetype Archetype);
	FLICK_API const TArray<EFlickPieceArchetype>& GetPreset(EFlickLineupPreset Preset);
	FLICK_API EFlickPieceArchetype Cycle(EFlickPieceArchetype Archetype, int32 Direction);
}
