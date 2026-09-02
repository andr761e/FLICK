#include "Core/FlickBobRules.h"

EFlickMatchOutcome FlickBobRules::EvaluateOutcome(
	const int32 Player1Remaining,
	const int32 Player2Remaining,
	const EFlickTeam ShootingTeam)
{
	if (Player1Remaining <= 0 && Player2Remaining <= 0)
	{
		return ShootingTeam == EFlickTeam::Player2
			? EFlickMatchOutcome::Player2Wins
			: EFlickMatchOutcome::Player1Wins;
	}
	if (Player1Remaining <= 0)
	{
		return EFlickMatchOutcome::Player1Wins;
	}
	if (Player2Remaining <= 0)
	{
		return EFlickMatchOutcome::Player2Wins;
	}
	return EFlickMatchOutcome::Continue;
}
