#include "Core/FlickMatchmakingRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickMatchmakingCompatibilityTest,
	"FLICK.Matchmaking.Compatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMatchmakingCompatibilityTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Trios requires six players"), FlickMatchmakingRules::GetRequiredPlayerCount(3), 6);
	TestTrue(TEXT("Two-player party fits doubles"), FlickMatchmakingRules::IsPartySizeValid(2, 2));
	TestFalse(TEXT("Three-player party cannot queue doubles"), FlickMatchmakingRules::IsPartySizeValid(3, 2));
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
	return true;
}

#endif
