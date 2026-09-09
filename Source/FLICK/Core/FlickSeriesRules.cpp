#include "Core/FlickSeriesRules.h"

int32 FlickSeriesRules::GetMaximumRounds(const int32 RoundsToWin)
{
	return FMath::Max(1, RoundsToWin) * 2 - 1;
}

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
	const int32 RoundsToWin,
	const int32 CompletedRoundNumber,
	const int32 Player1Score,
	const int32 Player2Score)
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
	const int32 SafeCompletedRound = FMath::Max(1, CompletedRoundNumber);
	const int32 RemainingRounds = FMath::Max(0, GetMaximumRounds(SafeRoundsToWin) - SafeCompletedRound);
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
	else if (Result.Player1RoundsWon > Result.Player2RoundsWon + RemainingRounds)
	{
		// Drawn rounds still consume the finite best-of series. End as soon as the
		// trailing player can no longer equal the leader, even below rounds-to-win.
		Result.bSeriesComplete = true;
		Result.SeriesWinner = EFlickTeam::Player1;
	}
	else if (Result.Player2RoundsWon > Result.Player1RoundsWon + RemainingRounds)
	{
		Result.bSeriesComplete = true;
		Result.SeriesWinner = EFlickTeam::Player2;
	}
	else if (SafeCompletedRound >= GetMaximumRounds(SafeRoundsToWin))
	{
		Result.bSeriesComplete = true;
		if (Result.Player1RoundsWon != Result.Player2RoundsWon)
		{
			Result.SeriesWinner = Result.Player1RoundsWon > Result.Player2RoundsWon
				? EFlickTeam::Player1
				: EFlickTeam::Player2;
		}
		else if (Player1Score != Player2Score)
		{
			Result.SeriesWinner = Player1Score > Player2Score
				? EFlickTeam::Player1
				: EFlickTeam::Player2;
		}
		else
		{
			Result.bSeriesDraw = true;
		}
	}
	return Result;
}
