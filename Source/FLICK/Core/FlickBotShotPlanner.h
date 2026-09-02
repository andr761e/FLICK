#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

struct FFlickBotPieceState
{
	int32 PieceId = INDEX_NONE;
	EFlickTeam Team = EFlickTeam::None;
	FVector2D Position = FVector2D::ZeroVector;
	float Radius = 0.0f;
};

struct FFlickBotShotTuning
{
	float ArenaRadius = 650.0f;
	float MinimumPower = 0.52f;
	float MaximumPower = 0.92f;
	float AimErrorDegrees = 1.35f;
	float PowerVariation = 0.025f;
};

struct FFlickBotShotPlan
{
	int32 ShooterPieceId = INDEX_NONE;
	int32 TargetPieceId = INDEX_NONE;
	FVector2D Direction = FVector2D::ZeroVector;
	float NormalizedPower = 0.0f;
	float Score = -BIG_NUMBER;

	bool IsValid() const
	{
		return ShooterPieceId != INDEX_NONE
			&& TargetPieceId != INDEX_NONE
			&& !Direction.IsNearlyZero();
	}
};

namespace FlickBotShotPlanner
{
	FLICK_API FFlickBotShotPlan PlanShot(
		const TArray<FFlickBotPieceState>& Pieces,
		EFlickTeam BotTeam,
		const FFlickBotShotTuning& Tuning,
		FRandomStream& RandomStream);
}
