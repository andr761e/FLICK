#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickSeriesRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickSeriesRulesTest,
	"FLICK.MatchRules.BestOfFiveSeries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickSeriesRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Player 1 opens odd rounds"), FlickSeriesRules::GetStartingTeam(1), EFlickTeam::Player1);
	TestEqual(TEXT("Player 2 opens even rounds"), FlickSeriesRules::GetStartingTeam(2), EFlickTeam::Player2);
	TestEqual(TEXT("Opening team alternates again"), FlickSeriesRules::GetStartingTeam(3), EFlickTeam::Player1);

	const FFlickSeriesRoundResult FirstWin = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player1Wins, 0, 0, 3, 1, 100, 50);
	TestEqual(TEXT("First win records one round"), FirstWin.Player1RoundsWon, 1);
	TestFalse(TEXT("One round does not complete the series"), FirstWin.bSeriesComplete);

	const FFlickSeriesRoundResult SecondWin = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player1Wins, 1, 1, 3, 3, 250, 200);
	TestEqual(TEXT("Second win records another round"), SecondWin.Player1RoundsWon, 2);
	TestFalse(TEXT("Two round wins do not complete Best of 5"), SecondWin.bSeriesComplete);

	const FFlickSeriesRoundResult MatchWin = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Player1Wins, 2, 2, 3, 5, 400, 500);
	TestEqual(TEXT("Third win records the winning point"), MatchWin.Player1RoundsWon, 3);
	TestTrue(TEXT("Three round wins complete Best of 5"), MatchWin.bSeriesComplete);

	const FFlickSeriesRoundResult Draw = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Draw, 2, 2, 3, 4, 400, 500);
	TestTrue(TEXT("A simultaneous ring-out remains a draw"), Draw.bRoundDraw);
	TestEqual(TEXT("Draw does not award Player 1"), Draw.Player1RoundsWon, 2);
	TestEqual(TEXT("Draw does not award Player 2"), Draw.Player2RoundsWon, 2);
	TestFalse(TEXT("A tied draw does not complete the series"), Draw.bSeriesComplete);

	const FFlickSeriesRoundResult RoundLeadAtLimit = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Draw, 2, 1, 3, 5, 200, 900);
	TestTrue(TEXT("The fifth round always completes Best of 5"), RoundLeadAtLimit.bSeriesComplete);
	TestEqual(TEXT("Round wins take priority at the limit"), RoundLeadAtLimit.SeriesWinner, EFlickTeam::Player1);

	const FFlickSeriesRoundResult ScoreTiebreak = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Draw, 1, 1, 3, 5, 650, 700);
	TestTrue(TEXT("A tied fifth-round series completes"), ScoreTiebreak.bSeriesComplete);
	TestEqual(TEXT("Score breaks a tied series"), ScoreTiebreak.SeriesWinner, EFlickTeam::Player2);

	const FFlickSeriesRoundResult ExactTie = FlickSeriesRules::ApplyRoundOutcome(
		EFlickMatchOutcome::Draw, 0, 0, 3, 5, 500, 500);
	TestTrue(TEXT("An exact fifth-round tie completes"), ExactTie.bSeriesComplete);
	TestTrue(TEXT("Equal rounds and score remain a match draw"), ExactTie.bSeriesDraw);
	TestEqual(TEXT("An exact tie has no series winner"), ExactTie.SeriesWinner, EFlickTeam::None);
	TestEqual(TEXT("Best of 5 has a five-round ceiling"), FlickSeriesRules::GetMaximumRounds(3), 5);

	return true;
}

#endif
