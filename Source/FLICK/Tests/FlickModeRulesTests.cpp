#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickModeRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickModeRulesTest,
	"FLICK.MatchRules.ModeConfigurations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickModeRulesTest::RunTest(const FString& Parameters)
{
	const FFlickModeRules& Classic = FlickModeRules::Get(EFlickMatchVariant::Classic);
	const FFlickModeRules& Bob = FlickModeRules::Get(EFlickMatchVariant::Bob);
	const FFlickModeRules& LegacyThreePuck = FlickModeRules::Get(EFlickMatchVariant::Blitz);

	TestEqual(TEXT("1v1 Knockout starts with four pieces per player"), Classic.StartingPiecesPerTeam, 4);
	TestEqual(TEXT("Doubles gives every player four pieces"), FlickModeRules::GetStartingPiecesPerTeam(EFlickMatchVariant::Classic, 2), 8);
	TestEqual(TEXT("Trios gives every player four pieces"), FlickModeRules::GetStartingPiecesPerTeam(EFlickMatchVariant::Classic, 3), 12);
	TestTrue(TEXT("Doubles arena is larger than singles"), FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, 2) > FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, 1));
	TestEqual(TEXT("Trios and doubles use the same arena size"), FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, 3), FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, 2));
	const float MinimumPieceSpacing = Classic.PieceRadius * 2.0f * 1.18f + 7.0f;
	for (const int32 TeamSize : {2, 3})
	{
		const float FormationDistance = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize) * 0.45f;
		const TArray<FVector2D> Formation = FlickModeRules::BuildMultiplayerFormationPositions(
			EFlickMatchVariant::Classic,
			TeamSize,
			FormationDistance);
		TestEqual(
			*FString::Printf(TEXT("%dv%d formation contains four pucks per player"), TeamSize, TeamSize),
			Formation.Num(),
			TeamSize * Classic.StartingPiecesPerTeam);
		for (int32 PlayerSlot = 0; PlayerSlot < TeamSize; ++PlayerSlot)
		{
			const int32 FirstPiece = PlayerSlot * Classic.StartingPiecesPerTeam;
			TestTrue(
				*FString::Printf(TEXT("%dv%d uses two concentric formation rows"), TeamSize, TeamSize),
				FMath::IsNearlyEqual(Formation[FirstPiece].Size(), Formation[FirstPiece + 1].Size(), 0.1f)
					&& FMath::IsNearlyEqual(Formation[FirstPiece + 2].Size(), Formation[FirstPiece + 3].Size(), 0.1f)
					&& Formation[FirstPiece].Size() < Formation[FirstPiece + 2].Size());
		}
		for (int32 FirstPiece = 0; FirstPiece < Formation.Num(); ++FirstPiece)
		{
			for (int32 SecondPiece = FirstPiece + 1; SecondPiece < Formation.Num(); ++SecondPiece)
			{
				TestTrue(
					*FString::Printf(TEXT("%dv%d curved formation pucks do not overlap"), TeamSize, TeamSize),
					FVector2D::Distance(Formation[FirstPiece], Formation[SecondPiece]) >= MinimumPieceSpacing);
			}
		}
	}
	TestEqual(TEXT("Legacy three-puck selections migrate to four-puck Knockout"), LegacyThreePuck.StartingPiecesPerTeam, 4);
	TestEqual(TEXT("Knockout is first to three rounds"), Classic.RoundsToWin, 3);
	TestEqual(TEXT("Legacy Knockout data preserves the series length"), LegacyThreePuck.RoundsToWin, 3);
	TestEqual(TEXT("BOB starts with twelve objective pucks per team"), Bob.StartingPiecesPerTeam, 12);
	TestEqual(TEXT("BOB is a single-board match"), Bob.RoundsToWin, 1);
	TestEqual(TEXT("BOB uses its isolated ruleset"), Bob.Ruleset, EFlickRuleset::Bob);
	TestTrue(TEXT("Knockout playlists use the promoted Switchyard arena"), Classic.bUseSwitchyardArena);
	TestFalse(TEXT("BOB retains its dedicated pocket board"), Bob.bUseSwitchyardArena);
	TestFalse(TEXT("BOB disables simultaneous Knockout kickoff"), Bob.bUseSimultaneousKickoff);
	TestFalse(TEXT("BOB disables special-puck loadouts"), Bob.bSupportsLoadouts);
	TestTrue(TEXT("BOB pucks use less surface friction than Knockout"), Bob.PieceFriction < Classic.PieceFriction);
	TestTrue(TEXT("BOB pucks retain momentum longer than Knockout"), Bob.LinearDamping < Classic.LinearDamping);
	TestTrue(TEXT("BOB allows longer resolution for the smoother board"), Bob.MaximumResolutionDuration > Classic.MaximumResolutionDuration);
	TestEqual(
		TEXT("Knockout builds one spawn offset per piece"),
		FlickModeRules::BuildFormationOffsets(Classic.StartingPiecesPerTeam).Num()
			+ FlickModeRules::BuildFormationOffsets(LegacyThreePuck.StartingPiecesPerTeam).Num(),
		Classic.StartingPiecesPerTeam + LegacyThreePuck.StartingPiecesPerTeam);

	const FVector ArenaLocation = FVector::ZeroVector;
	const float SurfaceZ = 250.0f;
	const float ArenaRadius = Classic.ArenaRadius;
	const float PuckRadius = Classic.PieceRadius;
	const float PuckThickness = Classic.PieceThickness;
	const float BoundsTolerance = 2.0f;
	TestFalse(
		TEXT("An upright puck may legitimately overhang the arena edge"),
		FlickModeRules::IsPieceOutsideCircularTabletop(
			FVector(ArenaRadius + PuckRadius - 1.0f, 0.0f, SurfaceZ + PuckThickness * 0.5f),
			FVector::UpVector,
			PuckRadius,
			PuckThickness,
			ArenaLocation,
			ArenaRadius,
			SurfaceZ,
			BoundsTolerance));
	TestTrue(
		TEXT("An upright puck completely beyond the arena edge is knocked out"),
		FlickModeRules::IsPieceOutsideCircularTabletop(
			FVector(ArenaRadius + PuckRadius + BoundsTolerance + 1.0f, 0.0f, SurfaceZ + PuckThickness * 0.5f),
			FVector::UpVector,
			PuckRadius,
			PuckThickness,
			ArenaLocation,
			ArenaRadius,
			SurfaceZ,
			BoundsTolerance));
	TestFalse(
		TEXT("A side-tipped puck touching the tabletop remains in play"),
		FlickModeRules::IsPieceOutsideCircularTabletop(
			FVector(ArenaRadius + PuckThickness * 0.5f, 0.0f, SurfaceZ + PuckRadius),
			FVector::ForwardVector,
			PuckRadius,
			PuckThickness,
			ArenaLocation,
			ArenaRadius,
			SurfaceZ,
			BoundsTolerance));
	TestTrue(
		TEXT("A side-tipped puck clear of the tabletop is knocked out"),
		FlickModeRules::IsPieceOutsideCircularTabletop(
			FVector(ArenaRadius + PuckThickness * 0.5f + BoundsTolerance + 1.0f, 0.0f, SurfaceZ + PuckRadius),
			FVector::ForwardVector,
			PuckRadius,
			PuckThickness,
			ArenaLocation,
			ArenaRadius,
			SurfaceZ,
			BoundsTolerance));
	TestTrue(
		TEXT("A puck completely below the tabletop is knocked out even near the rim"),
		FlickModeRules::IsPieceOutsideCircularTabletop(
			FVector(0.0f, 0.0f, SurfaceZ - PuckThickness * 0.5f - BoundsTolerance - 1.0f),
			FVector::UpVector,
			PuckRadius,
			PuckThickness,
			ArenaLocation,
			ArenaRadius,
			SurfaceZ,
			BoundsTolerance));

	return true;
}

#endif
