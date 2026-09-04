#include "Core/FlickTeamRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickTeamRulesOwnershipTest,
	"FLICK.TeamRules.PieceOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickTeamRulesOwnershipTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Singles kickoff launches two pucks"), FlickTeamRules::GetSimultaneousKickoffShotCount(1), 2);
	TestEqual(TEXT("Doubles kickoff launches four pucks"), FlickTeamRules::GetSimultaneousKickoffShotCount(2), 4);
	TestEqual(TEXT("Trios kickoff launches six pucks"), FlickTeamRules::GetSimultaneousKickoffShotCount(3), 6);
	TestEqual(TEXT("Doubles piece 1"), FlickTeamRules::GetPieceOwnerSlot(0, 2), 0);
	TestEqual(TEXT("Doubles piece 2"), FlickTeamRules::GetPieceOwnerSlot(1, 2), 1);
	TestEqual(TEXT("Doubles piece 3"), FlickTeamRules::GetPieceOwnerSlot(2, 2), 0);
	TestEqual(TEXT("Trios piece 3"), FlickTeamRules::GetPieceOwnerSlot(2, 3), 2);
	TestEqual(TEXT("Trios piece 4"), FlickTeamRules::GetPieceOwnerSlot(3, 3), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickTeamRulesRotationTest,
	"FLICK.TeamRules.TurnRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickTeamRulesRotationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Doubles advances to slot 2"), FlickTeamRules::AdvancePlayerSlot(0, 2), 1);
	TestEqual(TEXT("Doubles wraps"), FlickTeamRules::AdvancePlayerSlot(1, 2), 0);
	TestEqual(TEXT("Trios advances"), FlickTeamRules::AdvancePlayerSlot(1, 3), 2);
	TestEqual(TEXT("Trios wraps"), FlickTeamRules::AdvancePlayerSlot(2, 3), 0);
	TestTrue(TEXT("Matching slot is active"), FlickTeamRules::IsActivePlayerSlot(1, 1, 3));
	TestFalse(TEXT("Different slot is inactive"), FlickTeamRules::IsActivePlayerSlot(0, 1, 3));
	return true;
}

#endif
