#pragma once

#include "CoreMinimal.h"

namespace FlickTeamRules
{
	constexpr int32 MinimumPlayersPerTeam = 1;
	constexpr int32 MaximumPlayersPerTeam = 3;

	FLICK_API int32 ClampPlayersPerTeam(int32 PlayersPerTeam);
	FLICK_API int32 GetPieceOwnerSlot(int32 PieceIndex, int32 PlayersPerTeam);
	FLICK_API int32 GetSimultaneousKickoffShotCount(int32 PlayersPerTeam);
	FLICK_API int32 AdvancePlayerSlot(int32 CurrentSlot, int32 PlayersPerTeam);
	FLICK_API bool IsActivePlayerSlot(int32 PlayerSlot, int32 CurrentPlayerSlot, int32 PlayersPerTeam);
	FLICK_API bool HasRequiredClassConfirmationCount(
		bool bPrivateMatch,
		int32 ConfirmedParticipantCount,
		int32 PlayersPerTeam);
}
