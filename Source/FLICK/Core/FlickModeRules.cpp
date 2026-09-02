#include "Core/FlickModeRules.h"

#include "Core/FlickTeamRules.h"

namespace
{
	const FFlickModeRules ClassicRules;

	const FFlickModeRules BobRules = []
	{
		FFlickModeRules Rules;
		Rules.StartingPiecesPerTeam = 12;
		Rules.RoundsToWin = 1;
		Rules.Ruleset = EFlickRuleset::Bob;
		Rules.bUseSimultaneousKickoff = false;
		Rules.bSupportsLoadouts = false;
		Rules.ArenaRadius = 620.0f;
		Rules.PieceRadius = 24.0f;
		Rules.PieceThickness = 14.0f;
		Rules.MaxLaunchSpeed = 1320.0f;
		Rules.MaxDragDistance = 240.0f;
		Rules.PieceFriction = 0.05f;
		Rules.PieceRestitution = 0.28f;
		Rules.LinearDamping = 0.18f;
		Rules.AngularDamping = 1.35f;
		Rules.MaximumResolutionDuration = 10.0f;
		return Rules;
	}();

}

const FFlickModeRules& FlickModeRules::Get(const EFlickMatchVariant Variant)
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob:
		return BobRules;
	case EFlickMatchVariant::Classic:
	default:
		return ClassicRules;
	}
}

int32 FlickModeRules::GetStartingPiecesPerTeam(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam)
{
	const FFlickModeRules& Rules = Get(Variant);
	return NormalizeMatchVariant(Variant) == EFlickMatchVariant::Bob
		? Rules.StartingPiecesPerTeam
		: Rules.StartingPiecesPerTeam * FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
}

float FlickModeRules::GetArenaRadius(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam)
{
	const FFlickModeRules& Rules = Get(Variant);
	if (NormalizeMatchVariant(Variant) == EFlickMatchVariant::Bob)
	{
		return Rules.ArenaRadius;
	}
	const bool bUseMultiplayerArena = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam) > 1;
	return Rules.ArenaRadius
		+ (bUseMultiplayerArena ? Rules.MultiplayerArenaRadiusGrowth : 0.0f);
}

TArray<FVector2D> FlickModeRules::BuildMultiplayerFormationPositions(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const float FormationDistance)
{
	const FFlickModeRules& Rules = Get(Variant);
	const int32 TeamSize = NormalizeMatchVariant(Variant) == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	const int32 ColumnsPerRow = TeamSize * 2;
	TArray<FVector2D> Positions;
	Positions.Reserve(TeamSize * Rules.StartingPiecesPerTeam);

	// Each player owns two neighbouring columns across two concentric rows.
	// Ordering the results by player and then loadout slot keeps ownership and
	// loadout assignment stable while the whole team follows the arena curve.
	for (int32 PlayerSlot = 0; PlayerSlot < TeamSize; ++PlayerSlot)
	{
		for (int32 LoadoutSlot = 0; LoadoutSlot < Rules.StartingPiecesPerTeam; ++LoadoutSlot)
		{
			const int32 RowIndex = LoadoutSlot / 2;
			const int32 ColumnIndex = PlayerSlot * 2 + LoadoutSlot % 2;
			const float ColumnAlpha = ColumnsPerRow > 1
				? static_cast<float>(ColumnIndex) / static_cast<float>(ColumnsPerRow - 1)
				: 0.5f;
			const float AngleRadians = FMath::DegreesToRadians(FMath::Lerp(
				-Rules.FormationArcHalfAngleDegrees,
				Rules.FormationArcHalfAngleDegrees,
				ColumnAlpha));
			const float RowRadius = FormationDistance
				+ (static_cast<float>(RowIndex) - 0.5f) * Rules.FormationRowSpacing;
			Positions.Add(FVector2D(
				RowRadius * FMath::Sin(AngleRadians),
				RowRadius * FMath::Cos(AngleRadians)));
		}
	}

	return Positions;
}

TArray<FVector2D> FlickModeRules::BuildFormationOffsets(const int32 PiecesPerTeam)
{
	TArray<FVector2D> Offsets;
	Offsets.Reserve(FMath::Clamp(PiecesPerTeam, 1, 4));

	switch (PiecesPerTeam)
	{
	case 1:
		Offsets.Add(FVector2D::ZeroVector);
		break;
	case 2:
		Offsets.Add(FVector2D(-75.0f, 0.0f));
		Offsets.Add(FVector2D(75.0f, 0.0f));
		break;
	case 3:
		Offsets.Add(FVector2D(0.0f, -55.0f));
		Offsets.Add(FVector2D(-85.0f, 45.0f));
		Offsets.Add(FVector2D(85.0f, 45.0f));
		break;
	default:
		Offsets.Add(FVector2D(-88.0f, -60.0f));
		Offsets.Add(FVector2D(88.0f, -60.0f));
		Offsets.Add(FVector2D(-88.0f, 60.0f));
		Offsets.Add(FVector2D(88.0f, 60.0f));
		break;
	}

	return Offsets;
}
