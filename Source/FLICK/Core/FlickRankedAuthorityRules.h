#pragma once

#include "CoreMinimal.h"
#include "Ranking/FlickRankedBackendTypes.h"

namespace FlickRankedAuthorityRules
{
	bool ValidateMatchRequest(
		const FFlickRankedMatchRequest& Request,
		int32 MaximumTeamRatingSpread,
		FString& OutError);
	int32 GetAverageTeamRating(
		const TArray<FFlickRankedParticipant>& Participants,
		EFlickTeam Team);
	EFlickRankMatchResult GetPlayerResult(EFlickTeam Team, EFlickMatchOutcome Outcome);
}

