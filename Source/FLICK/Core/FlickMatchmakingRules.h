#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

namespace FlickMatchmakingRules
{
	FLICK_API int32 GetRequiredPlayerCount(int32 PlayersPerTeam);
	FLICK_API bool IsPartySizeValid(int32 PartySize, int32 PlayersPerTeam);
	FLICK_API bool IsCompatibleLobby(
		EFlickMatchVariant RequestedVariant,
		int32 RequestedPlayersPerTeam,
		int32 JoiningPartySize,
		EFlickMatchVariant LobbyVariant,
		int32 LobbyPlayersPerTeam,
		int32 LobbyOpenConnections,
		bool bLobbyAcceptingPlayers,
		int32 LobbyHostPartySize = 1);
	FLICK_API EFlickTeam ChooseTeamForPremade(
		int32 Player1Count,
		int32 Player2Count,
		int32 PlayersPerTeam,
		int32 PartySize);
}
