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

	TestEqual(TEXT("One knockout on each side is a trade"), EvaluateDramaticEvent(1, 1, 3, 3, 2, 4, EFlickTeam::None, true).Event, EFlickDramaticEvent::Trade);
	const FFlickDramaticEventResult DoubleKnockout = EvaluateDramaticEvent(0, 2, 4, 2, 3, 4, EFlickTeam::Player1, false);
	TestEqual(TEXT("Two knockouts on one side are a double knockout"), DoubleKnockout.Event, EFlickDramaticEvent::DoubleKnockout);
	TestEqual(TEXT("The team causing the double knockout receives the highlight"), DoubleKnockout.HighlightedTeam, EFlickTeam::Player1);
	const FFlickDramaticEventResult MultiKnockout = EvaluateDramaticEvent(2, 1, 2, 3, 5, 4, EFlickTeam::None, true);
	TestEqual(TEXT("Three knockouts are a multi knockout"), MultiKnockout.Event, EFlickDramaticEvent::MultiKnockout);
	TestEqual(TEXT("The team losing fewer pucks receives the multi-knockout highlight"), MultiKnockout.HighlightedTeam, EFlickTeam::Player2);
	TestEqual(TEXT("Eliminating only your own puck is called out"), EvaluateDramaticEvent(1, 0, 3, 4, 1, 4, EFlickTeam::Player1, false).Event, EFlickDramaticEvent::SelfKnockout);
	TestEqual(TEXT("A one-puck victory is last puck standing"), EvaluateDramaticEvent(0, 1, 1, 0, 2, 4, EFlickTeam::Player1, false).Event, EFlickDramaticEvent::LastPuckStanding);
	TestEqual(TEXT("A long collision sequence is a chain reaction"), EvaluateDramaticEvent(0, 0, 4, 4, 5, 4, EFlickTeam::Player1, false).Event, EFlickDramaticEvent::ChainReaction);

	return true;
}

#endif
