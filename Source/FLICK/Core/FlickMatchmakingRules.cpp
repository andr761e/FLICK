#include "Core/FlickMatchmakingRules.h"

#include "Core/FlickTeamRules.h"

int32 FlickMatchmakingRules::GetRequiredPlayerCount(const int32 PlayersPerTeam)
{
	return FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam) * 2;
}

bool FlickMatchmakingRules::IsPartySizeValid(
	const int32 PartySize,
	const int32 PlayersPerTeam)
{
	return PartySize >= 1
		&& PartySize <= FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
}

bool FlickMatchmakingRules::IsCompatibleLobby(
	const EFlickMatchVariant RequestedVariant,
	const int32 RequestedPlayersPerTeam,
	const int32 JoiningPartySize,
	const EFlickMatchVariant LobbyVariant,
	const int32 LobbyPlayersPerTeam,
	const int32 LobbyOpenConnections,
	const bool bLobbyAcceptingPlayers,
	const int32 LobbyHostPartySize)
{
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(LobbyPlayersPerTeam);
	const int32 CurrentPlayers = FMath::Clamp(
		GetRequiredPlayerCount(TeamSize) - FMath::Max(0, LobbyOpenConnections),
		0,
		GetRequiredPlayerCount(TeamSize));
	const int32 HostPartySize = FMath::Clamp(LobbyHostPartySize, 1, FMath::Max(1, CurrentPlayers));
	const int32 AdditionalPlayers = FMath::Max(0, CurrentPlayers - HostPartySize);
	const int32 EstimatedPlayer2Count = FMath::Min(AdditionalPlayers, TeamSize);
	const int32 EstimatedPlayer1Count = FMath::Min(
		TeamSize,
		HostPartySize + FMath::Max(0, AdditionalPlayers - TeamSize));

	return bLobbyAcceptingPlayers
		&& NormalizeMatchVariant(RequestedVariant) == NormalizeMatchVariant(LobbyVariant)
		&& FlickTeamRules::ClampPlayersPerTeam(RequestedPlayersPerTeam)
			== TeamSize
		&& IsPartySizeValid(JoiningPartySize, RequestedPlayersPerTeam)
		&& LobbyOpenConnections >= JoiningPartySize
		&& ChooseTeamForPremade(
			EstimatedPlayer1Count,
			EstimatedPlayer2Count,
			TeamSize,
			JoiningPartySize) != EFlickTeam::None;
}

EFlickTeam FlickMatchmakingRules::ChooseTeamForPremade(
	const int32 Player1Count,
	const int32 Player2Count,
	const int32 PlayersPerTeam,
	const int32 PartySize)
{
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	const int32 SafePartySize = FMath::Clamp(PartySize, 1, TeamSize);
	if (FMath::Max(0, Player2Count) + SafePartySize <= TeamSize)
	{
		return EFlickTeam::Player2;
	}
	if (FMath::Max(0, Player1Count) + SafePartySize <= TeamSize)
	{
		return EFlickTeam::Player1;
	}
	return EFlickTeam::None;
}
