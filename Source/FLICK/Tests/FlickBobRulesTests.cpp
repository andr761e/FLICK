#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickBobRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickBobRulesTest,
	"FLICK.MatchRules.BobOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBobRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("BOB continues while both colors remain"),
		FlickBobRules::EvaluateOutcome(3, 4, EFlickTeam::Player1),
		EFlickMatchOutcome::Continue);
	TestEqual(
		TEXT("Player 1 wins by clearing their own color"),
		FlickBobRules::EvaluateOutcome(0, 4, EFlickTeam::Player2),
		EFlickMatchOutcome::Player1Wins);
	TestEqual(
		TEXT("Player 2 wins by clearing their own color"),
		FlickBobRules::EvaluateOutcome(5, 0, EFlickTeam::Player1),
		EFlickMatchOutcome::Player2Wins);
	TestEqual(
		TEXT("The shooter wins if both colors clear in one collision chain"),
		FlickBobRules::EvaluateOutcome(0, 0, EFlickTeam::Player2),
		EFlickMatchOutcome::Player2Wins);

	return true;
}

#endif
