#include "Core/FlickSeriesRules.h"

EFlickTeam FlickSeriesRules::GetStartingTeam(const int32 RoundNumber)
{
	return FMath::Max(1, RoundNumber) % 2 == 0
		? EFlickTeam::Player2
		: EFlickTeam::Player1;
}

FFlickSeriesRoundResult FlickSeriesRules::ApplyRoundOutcome(
	const EFlickMatchOutcome Outcome,
	const int32 Player1RoundsWon,
	const int32 Player2RoundsWon,
	const int32 RoundsToWin)
{
	FFlickSeriesRoundResult Result;
	Result.Player1RoundsWon = FMath::Max(0, Player1RoundsWon);
	Result.Player2RoundsWon = FMath::Max(0, Player2RoundsWon);

	switch (Outcome)
	{
	case EFlickMatchOutcome::Player1Wins:
		++Result.Player1RoundsWon;
		Result.RoundWinner = EFlickTeam::Player1;
		break;
	case EFlickMatchOutcome::Player2Wins:
		++Result.Player2RoundsWon;
		Result.RoundWinner = EFlickTeam::Player2;
		break;
	case EFlickMatchOutcome::Draw:
		Result.bRoundDraw = true;
		break;
	case EFlickMatchOutcome::Continue:
	default:
		break;
	}

	const int32 SafeRoundsToWin = FMath::Max(1, RoundsToWin);
	if (Result.Player1RoundsWon >= SafeRoundsToWin)
	{
		Result.bSeriesComplete = true;
		Result.SeriesWinner = EFlickTeam::Player1;
	}
	else if (Result.Player2RoundsWon >= SafeRoundsToWin)
	{
		Result.bSeriesComplete = true;
		Result.SeriesWinner = EFlickTeam::Player2;
	}
	return Result;
}
