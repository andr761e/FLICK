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
