#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickSeriesRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickSeriesRulesTest,
	"FLICK.MatchRules.FirstToThreeSeries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickSeriesRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Player 1 opens odd rounds"), FlickSeriesRules::GetStartingTeam(1), EFlickTeam::Player1);
	TestEqual(TEXT("Player 2 opens even rounds"), FlickSeriesRules::GetStartingTeam(2), EFlickTeam::Player2);
	TestEqual(TEXT("Opening team alternates again"), FlickSeriesRules::GetStartingTeam(3), EFlickTeam::Player1);

	const FFlickSeriesRoundResult FirstWin = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player1Wins, 0, 0, 3);
	TestEqual(TEXT("First win records one round"), FirstWin.Player1RoundsWon, 1);
	TestFalse(TEXT("One round does not complete the series"), FirstWin.bSeriesComplete);

	const FFlickSeriesRoundResult SecondWin = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player1Wins, 1, 1, 3);
	TestEqual(TEXT("Second win records another round"), SecondWin.Player1RoundsWon, 2);
	TestFalse(TEXT("Two round wins do not complete first to three"), SecondWin.bSeriesComplete);

	const FFlickSeriesRoundResult MatchWin = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player1Wins, 2, 2, 3);
	TestEqual(TEXT("Third win records the winning point"), MatchWin.Player1RoundsWon, 3);
	TestTrue(TEXT("Three round wins complete the match"), MatchWin.bSeriesComplete);

	const FFlickSeriesRoundResult Draw = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Draw, 2, 2, 3);
	TestTrue(TEXT("A simultaneous ring-out remains a draw"), Draw.bRoundDraw);
	TestEqual(TEXT("Draw does not award Player 1"), Draw.Player1RoundsWon, 2);
	TestEqual(TEXT("Draw does not award Player 2"), Draw.Player2RoundsWon, 2);
	TestFalse(TEXT("A tied draw does not complete the series"), Draw.bSeriesComplete);

	const FFlickSeriesRoundResult LateDraw = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Draw, 2, 1, 3);
	TestFalse(TEXT("Any number of drawn rounds cannot end the match"), LateDraw.bSeriesComplete);
	TestEqual(TEXT("A drawn round has no series winner"), LateDraw.SeriesWinner, EFlickTeam::None);

	const FFlickSeriesRoundResult Player2Win = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player2Wins, 2, 2, 3);
	TestTrue(TEXT("Player 2 also wins immediately on their third round"), Player2Win.bSeriesComplete);
	TestEqual(TEXT("Player 2 is recorded as series winner"), Player2Win.SeriesWinner, EFlickTeam::Player2);

	return true;
}

#endif
