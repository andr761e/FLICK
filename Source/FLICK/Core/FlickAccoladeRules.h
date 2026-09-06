#pragma once

#include "Core/FlickTypes.h"

struct FFlickShotAccoladeContext
{
	int32 OpponentEliminated = 0;
	int32 OwnEliminated = 0;
	int32 OpponentRemaining = 0;
	int32 OwnRemaining = 0;
	int32 DirectlyContactedPucks = 0;
	bool bSwitchActivated = false;
	bool bShotPieceHitDivider = false;
	bool bNewDividerAffectedPlay = false;
	bool bDominoKnockout = false;
	bool bLongRangeKnockout = false;
	bool bShotPieceSurvivedNearEdge = false;
	bool bBuzzerRelease = false;
};

namespace FlickAccoladeRules
{
	inline TArray<EFlickAccolade> EvaluateShot(const FFlickShotAccoladeContext& Context)
	{
		TArray<EFlickAccolade> Result;
		if (Context.OpponentEliminated <= 0)
		{
			return Result;
		}

		if (Context.bSwitchActivated)
		{
			Result.Add(EFlickAccolade::SwitchKnockout);
		}
		if (Context.bShotPieceHitDivider)
		{
			Result.Add(EFlickAccolade::DividerBank);
		}
		if (Context.bNewDividerAffectedPlay)
		{
			Result.Add(EFlickAccolade::TrapShot);
		}
		if (Context.bDominoKnockout)
		{
			Result.Add(EFlickAccolade::DominoKnockout);
		}
		if (Context.OpponentRemaining == 0 && Context.OpponentEliminated >= 2)
		{
			Result.Add(EFlickAccolade::TeamWipeout);
		}
		if (Context.OwnEliminated == 1 && Context.OpponentEliminated == 1
			&& Context.OpponentRemaining == 0 && Context.OwnRemaining > 0)
		{
			Result.Add(EFlickAccolade::PerfectTrade);
		}
		if (Context.bLongRangeKnockout)
		{
			Result.Add(EFlickAccolade::LongRangeKnockout);
		}
		if (Context.DirectlyContactedPucks >= 3)
		{
			Result.Add(EFlickAccolade::Pinball);
		}
		if (Context.bShotPieceSurvivedNearEdge && Context.OwnEliminated == 0)
		{
			Result.Add(EFlickAccolade::PrecisionKnockout);
		}
		if (Context.bBuzzerRelease)
		{
			Result.Add(EFlickAccolade::BuzzerBeater);
		}
		return Result;
	}

	inline bool IsFlawlessRound(
		const int32 WinningTeamRemaining,
		const int32 StartingPiecesPerTeam)
	{
		return StartingPiecesPerTeam > 0 && WinningTeamRemaining >= StartingPiecesPerTeam;
	}
}
