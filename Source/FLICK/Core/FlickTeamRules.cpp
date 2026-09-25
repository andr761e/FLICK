#include "Core/FlickTeamRules.h"

int32 FlickTeamRules::ClampPlayersPerTeam(const int32 PlayersPerTeam)
{
	return FMath::Clamp(PlayersPerTeam, MinimumPlayersPerTeam, MaximumPlayersPerTeam);
}

int32 FlickTeamRules::GetPieceOwnerSlot(const int32 PieceIndex, const int32 PlayersPerTeam)
{
	const int32 ClampedTeamSize = ClampPlayersPerTeam(PlayersPerTeam);
	return FMath::Max(0, PieceIndex) % ClampedTeamSize;
}

int32 FlickTeamRules::GetSimultaneousKickoffShotCount(const int32 PlayersPerTeam)
{
	return ClampPlayersPerTeam(PlayersPerTeam) * 2;
}

int32 FlickTeamRules::AdvancePlayerSlot(const int32 CurrentSlot, const int32 PlayersPerTeam)
{
	const int32 ClampedTeamSize = ClampPlayersPerTeam(PlayersPerTeam);
	return (FMath::Max(0, CurrentSlot) + 1) % ClampedTeamSize;
}

int32 FlickTeamRules::GetRoundOpeningPlayerSlot(
	const int32 RoundNumber,
	const EFlickTeam Team,
	const int32 PlayersPerTeam)
{
	const int32 SafeRound = FMath::Max(1, RoundNumber);
	const int32 TeamSize = ClampPlayersPerTeam(PlayersPerTeam);
	// Blue 1, Orange 1, Blue 2, Orange 2... is one continuous cycle.
	// Each new round starts at the next seat, even when kickoff is simultaneous.
	return (Team == EFlickTeam::Player1 ? SafeRound / 2 : (SafeRound - 1) / 2) % TeamSize;
}

bool FlickTeamRules::IsActivePlayerSlot(
	const int32 PlayerSlot,
	const int32 CurrentPlayerSlot,
	const int32 PlayersPerTeam)
{
	const int32 ClampedTeamSize = ClampPlayersPerTeam(PlayersPerTeam);
	return PlayerSlot >= 0
		&& PlayerSlot < ClampedTeamSize
		&& PlayerSlot == FMath::Clamp(CurrentPlayerSlot, 0, ClampedTeamSize - 1);
}

bool FlickTeamRules::HasRequiredClassConfirmationCount(
	const bool bPrivateMatch,
	const int32 ConfirmedParticipantCount,
	const int32 PlayersPerTeam)
{
	if (ConfirmedParticipantCount <= 0)
	{
		return false;
	}
	return bPrivateMatch
		|| ConfirmedParticipantCount == ClampPlayersPerTeam(PlayersPerTeam) * 2;
}
