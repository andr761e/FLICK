#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

enum class EFlickRuleset : uint8
{
	Knockout,
	Bob
};

struct FLICK_API FFlickModeRules
{
	// Knockout treats this as the number of pieces owned by each player.
	// BOB remains a single-player-per-side ruleset and uses it as the team total.
	int32 StartingPiecesPerTeam = 4;
	int32 RoundsToWin = 3;
	EFlickRuleset Ruleset = EFlickRuleset::Knockout;
	bool bUseSimultaneousKickoff = true;
	bool bSupportsLoadouts = true;
	float ArenaRadius = 650.0f;
	float MultiplayerArenaRadiusGrowth = 165.0f;
	float FormationArcHalfAngleDegrees = 60.0f;
	float FormationRowSpacing = 130.0f;
	float PieceRadius = 45.0f;
	float PieceThickness = 20.0f;
	float MaxLaunchSpeed = 1450.0f;
	float MaxDragDistance = 280.0f;
	float PieceFriction = 0.12f;
	float PieceRestitution = 0.32f;
	float LinearDamping = 0.55f;
	float AngularDamping = 1.8f;
	float MaximumResolutionDuration = 8.0f;
};

namespace FlickModeRules
{
	FLICK_API const FFlickModeRules& Get(EFlickMatchVariant Variant);
	FLICK_API int32 GetStartingPiecesPerTeam(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	FLICK_API float GetArenaRadius(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	FLICK_API bool IsPieceOutsideCircularTabletop(
		const FVector& PieceLocation,
		const FVector& PieceUpVector,
		float PieceRadius,
		float PieceThickness,
		const FVector& ArenaLocation,
		float ArenaRadius,
		float ArenaSurfaceZ,
		float ClearanceTolerance);
	FLICK_API TArray<FVector2D> BuildMultiplayerFormationPositions(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		float FormationDistance);
	FLICK_API TArray<FVector2D> BuildFormationOffsets(int32 PiecesPerTeam);
}
