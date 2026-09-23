#include "Core/FlickMatchmakingRules.h"
#include "Core/FlickLineupRules.h"
#include "Core/FlickPrivateMatchRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickMatchmakingCompatibilityTest,
	"FLICK.Matchmaking.Compatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMatchmakingCompatibilityTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Trios requires six players"), FlickMatchmakingRules::GetRequiredPlayerCount(3), 6);
	TestTrue(TEXT("Solo party fits 1v1 and BOB"), FlickMatchmakingRules::IsPartySizeValid(1, 1));
	TestFalse(TEXT("Two-player party cannot queue 1v1 or BOB"), FlickMatchmakingRules::IsPartySizeValid(2, 1));
	TestTrue(TEXT("Two-player party fits doubles"), FlickMatchmakingRules::IsPartySizeValid(2, 2));
	TestFalse(TEXT("Three-player party cannot queue doubles"), FlickMatchmakingRules::IsPartySizeValid(3, 2));
	TestTrue(TEXT("Three-player party fits trios"), FlickMatchmakingRules::IsPartySizeValid(3, 3));
	TestFalse(TEXT("Four-player party cannot queue a public playlist"), FlickMatchmakingRules::IsPartySizeValid(4, 3));
	TestTrue(
		TEXT("Matching queue is compatible"),
		FlickMatchmakingRules::IsCompatibleLobby(
			EFlickMatchVariant::Classic, 2, 1,
			EFlickMatchVariant::Classic, 2, 2, true));
	TestFalse(
		TEXT("Different arena is rejected"),
		FlickMatchmakingRules::IsCompatibleLobby(
			EFlickMatchVariant::Bob, 2, 1,
			EFlickMatchVariant::Classic, 2, 2, true));
	TestFalse(
		TEXT("Locked queue is rejected"),
		FlickMatchmakingRules::IsCompatibleLobby(
			EFlickMatchVariant::Classic, 3, 1,
			EFlickMatchVariant::Classic, 3, 3, false));
	TestFalse(
		TEXT("Total open seats are rejected when neither team can hold the joining party"),
		FlickMatchmakingRules::IsCompatibleLobby(
			EFlickMatchVariant::Classic, 3, 2,
			EFlickMatchVariant::Classic, 3, 2, true, 2));
	TestEqual(
		TEXT("A complete visiting doubles party stays together on Team 2"),
		FlickMatchmakingRules::ChooseTeamForPremade(2, 0, 2, 2),
		EFlickTeam::Player2);
	TestEqual(
		TEXT("A visiting trios party fits opposite a solo host"),
		FlickMatchmakingRules::ChooseTeamForPremade(1, 0, 3, 3),
		EFlickTeam::Player2);
	TestEqual(
		TEXT("A premade is rejected when neither team has contiguous capacity"),
		FlickMatchmakingRules::ChooseTeamForPremade(2, 2, 3, 2),
		EFlickTeam::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPrivateMatchSlotEncodingTest,
	"FLICK.PrivateMatch.SlotEncoding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPrivateMatchSlotEncodingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Private parties support six users"), FlickMaximumPartyMembers, 6);
	TestEqual(TEXT("Blue Player 1 uses slot zero"), EncodePrivatePlayerSlot(EFlickTeam::Player1, 0), 0);
	TestEqual(TEXT("Blue Player 3 stays on the blue half"), EncodePrivatePlayerSlot(EFlickTeam::Player1, 2), 2);
	TestEqual(TEXT("Orange Player 1 starts the orange half"), EncodePrivatePlayerSlot(EFlickTeam::Player2, 0), 3);
	TestEqual(TEXT("Orange Player 3 uses the final slot"), EncodePrivatePlayerSlot(EFlickTeam::Player2, 2), 5);
	TestEqual(TEXT("Spectators do not encode as players"), EncodePrivatePlayerSlot(EFlickTeam::None, 0), INDEX_NONE);
	TestEqual(TEXT("Out-of-range slots are rejected"), EncodePrivatePlayerSlot(EFlickTeam::Player1, 3), INDEX_NONE);
	TestFalse(TEXT("A four-player party cannot enter public trios"), FlickMatchmakingRules::IsPartySizeValid(4, 3));

	FFlickPrivateMatchSettings Settings;
	Settings.Variant = EFlickMatchVariant::Classic;
	Settings.PlayersPerTeam = 1;
	FlickPrivateMatchRules::CycleMode(Settings, 1);
	TestEqual(TEXT("Private mode advances from 1v1 to 2v2"), Settings.PlayersPerTeam, 2);
	FlickPrivateMatchRules::CycleMode(Settings, 1);
	FlickPrivateMatchRules::CycleMode(Settings, 1);
	TestEqual(TEXT("Private mode advances from 3v3 to BOB"), Settings.Variant, EFlickMatchVariant::Bob);
	TestEqual(TEXT("BOB always reserves one player per side"), FlickPrivateMatchRules::GetRequiredPlayingSlots(Settings), 2);
	FlickPrivateMatchRules::CycleMode(Settings, -1);
	TestEqual(TEXT("Cycling backward from BOB returns to 3v3"), Settings.PlayersPerTeam, 3);
	const TArray<EFlickPieceArchetype> ValidLineup = {
		EFlickPieceArchetype::Standard,
		EFlickPieceArchetype::Heavy,
		EFlickPieceArchetype::Striker,
		EFlickPieceArchetype::Grippy};
	TestTrue(TEXT("Four distinct archetypes form a legal network lineup"), FlickLineupRules::IsValid(ValidLineup));
	TArray<EFlickPieceArchetype> DuplicateLineup = ValidLineup;
	DuplicateLineup[3] = EFlickPieceArchetype::Standard;
	TestFalse(TEXT("Duplicate archetypes are rejected at the network boundary"), FlickLineupRules::IsValid(DuplicateLineup));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPrivateMatchSettingsTest,
	"FLICK.PrivateMatch.Settings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPrivateMatchSettingsTest::RunTest(const FString& Parameters)
{
	FFlickPrivateMatchSettings Settings;
	Settings.Variant = EFlickMatchVariant::Classic;
	Settings.PlayersPerTeam = 99;
	Settings.RoundsToWin = 0;
	Settings.ArenaScale = 5.0f;
	Settings.FrictionScale = 0.0f;
	Settings.LaunchSpeedScale = 9.0f;
	Settings.RestitutionScale = 0.0f;
	FlickPrivateMatchRules::Normalize(Settings);
	TestEqual(TEXT("Classic team size is capped at three"), Settings.PlayersPerTeam, 3);
	TestEqual(TEXT("Round target cannot fall below one"), Settings.RoundsToWin, 1);
	TestEqual(TEXT("Arena size is clamped"), Settings.ArenaScale, 1.3f);
	TestEqual(TEXT("Friction is clamped"), Settings.FrictionScale, 0.5f);
	TestEqual(TEXT("Launch power is clamped"), Settings.LaunchSpeedScale, 1.5f);
	TestEqual(TEXT("Bounce is clamped"), Settings.RestitutionScale, 0.5f);
	TestEqual(TEXT("Three-versus-three requires six playing slots"), FlickPrivateMatchRules::GetRequiredPlayingSlots(Settings), 6);

	Settings.Variant = EFlickMatchVariant::Bob;
	Settings.PlayersPerTeam = 3;
	FlickPrivateMatchRules::Normalize(Settings);
	TestEqual(TEXT("BOB always uses one player per side"), Settings.PlayersPerTeam, 1);
	TestEqual(TEXT("BOB needs two playing slots"), FlickPrivateMatchRules::GetRequiredPlayingSlots(Settings), 2);
	FlickPrivateMatchRules::CycleMode(Settings, 1);
	TestEqual(TEXT("Cycling forward from BOB wraps to classic"), Settings.Variant, EFlickMatchVariant::Classic);
	TestEqual(TEXT("Wrapped mode begins at 1v1"), Settings.PlayersPerTeam, 1);
	return true;
}

#endif
