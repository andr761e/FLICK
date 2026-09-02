#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickMatchRulesTest,
	"FLICK.MatchRules.Outcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMatchRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Both teams with pieces continues"),
		EvaluateMatchOutcome(4, 4),
		EFlickMatchOutcome::Continue);
	TestEqual(
		TEXT("Player 1 wins when Player 2 has no pieces"),
		EvaluateMatchOutcome(2, 0),
		EFlickMatchOutcome::Player1Wins);
	TestEqual(
		TEXT("Player 2 wins when Player 1 has no pieces"),
		EvaluateMatchOutcome(0, 3),
		EFlickMatchOutcome::Player2Wins);
	TestEqual(
		TEXT("Both teams reaching zero is a draw"),
		EvaluateMatchOutcome(0, 0),
		EFlickMatchOutcome::Draw);
	TestEqual(
		TEXT("Counts below zero are treated as empty for defensive robustness"),
		EvaluateMatchOutcome(-1, -1),
		EFlickMatchOutcome::Draw);

	return true;
}

#endif
