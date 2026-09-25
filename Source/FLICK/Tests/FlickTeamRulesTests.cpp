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
	for (const int32 TeamSize : {2, 3})
	{
		for (int32 Round = 1; Round <= TeamSize * 2 + 2; ++Round)
		{
			const int32 OpeningSeat = (Round - 1) % (TeamSize * 2);
			const EFlickTeam OpeningTeam = OpeningSeat % 2 == 0
				? EFlickTeam::Player1 : EFlickTeam::Player2;
			int32 BlueSlot = FlickTeamRules::GetRoundOpeningPlayerSlot(
				Round, EFlickTeam::Player1, TeamSize);
			int32 OrangeSlot = FlickTeamRules::GetRoundOpeningPlayerSlot(
				Round, EFlickTeam::Player2, TeamSize);
			EFlickTeam CurrentTeam = OpeningTeam;
			for (int32 Turn = 0; Turn < TeamSize * 2; ++Turn)
			{
				const int32 ExpectedSeat = (OpeningSeat + Turn) % (TeamSize * 2);
				const EFlickTeam ExpectedTeam = ExpectedSeat % 2 == 0
					? EFlickTeam::Player1 : EFlickTeam::Player2;
				const int32 ActualSlot = CurrentTeam == EFlickTeam::Player1 ? BlueSlot : OrangeSlot;
				TestEqual(TEXT("Turn team follows interleaved cycle"), CurrentTeam, ExpectedTeam);
				TestEqual(TEXT("Turn slot follows interleaved cycle"), ActualSlot, ExpectedSeat / 2);
				if (CurrentTeam == EFlickTeam::Player1)
				{
					BlueSlot = FlickTeamRules::AdvancePlayerSlot(BlueSlot, TeamSize);
					CurrentTeam = EFlickTeam::Player2;
				}
				else
				{
					OrangeSlot = FlickTeamRules::AdvancePlayerSlot(OrangeSlot, TeamSize);
					CurrentTeam = EFlickTeam::Player1;
				}
			}
		}
	}
	TestTrue(TEXT("Matching slot is active"), FlickTeamRules::IsActivePlayerSlot(1, 1, 3));
	TestFalse(TEXT("Different slot is inactive"), FlickTeamRules::IsActivePlayerSlot(0, 1, 3));
	TestTrue(TEXT("Private match accepts one human controlling several slots"),
		FlickTeamRules::HasRequiredClassConfirmationCount(true, 1, 3));
	TestFalse(TEXT("Private match still requires at least one participant"),
		FlickTeamRules::HasRequiredClassConfirmationCount(true, 0, 3));
	TestFalse(TEXT("Public doubles waits for all four players"),
		FlickTeamRules::HasRequiredClassConfirmationCount(false, 3, 2));
	TestTrue(TEXT("Public doubles accepts its complete roster"),
		FlickTeamRules::HasRequiredClassConfirmationCount(false, 4, 2));
	return true;
}

#endif
