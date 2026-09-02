#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

struct FLICK_API FFlickSeriesRoundResult
{
	int32 Player1RoundsWon = 0;
	int32 Player2RoundsWon = 0;
	EFlickTeam RoundWinner = EFlickTeam::None;
	EFlickTeam SeriesWinner = EFlickTeam::None;
	bool bRoundDraw = false;
	bool bSeriesDraw = false;
	bool bSeriesComplete = false;
};

namespace FlickSeriesRules
{
	constexpr int32 KnockoutRoundsToWin = 3;
	constexpr int32 SingleBoardRoundsToWin = 1;

	FLICK_API int32 GetMaximumRounds(int32 RoundsToWin);
	FLICK_API EFlickTeam GetStartingTeam(int32 RoundNumber);
	FLICK_API FFlickSeriesRoundResult ApplyRoundOutcome(
		EFlickMatchOutcome Outcome,
		int32 Player1RoundsWon,
		int32 Player2RoundsWon,
		int32 RoundsToWin,
		int32 CompletedRoundNumber,
		int32 Player1Score,
		int32 Player2Score);
}
