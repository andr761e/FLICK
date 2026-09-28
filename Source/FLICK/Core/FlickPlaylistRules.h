#pragma once

#include "CoreMinimal.h"

namespace FlickPlaylistRules
{
	inline bool CountsForCareer(const bool bSeriesComplete, const bool bMatchmade, const bool bPrivateMatch)
	{
		return bSeriesComplete && bMatchmade && !bPrivateMatch;
	}

	inline bool CanPauseWorld(
		const bool bTraining,
		const bool bStandalone,
		const bool bPrivateMatch,
		const bool bMatchmaking,
		const bool bDedicatedServer,
		const int32 HumanPlayers)
	{
		return !bDedicatedServer
			&& ((bTraining && bStandalone)
				|| (bPrivateMatch && !bMatchmaking && HumanPlayers == 1));
	}
}
