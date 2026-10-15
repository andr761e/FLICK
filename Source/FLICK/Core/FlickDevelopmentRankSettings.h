#pragma once

#include "Core/FlickRankRules.h"

// Presentation-only overrides. Never call this from queueing or settlement code.
namespace FlickDevelopmentRankSettings
{
	TOptional<int32> GetRating(EFlickMatchVariant Variant, int32 PlayersPerTeam, const FString& Ini = FString());
	FFlickRankProgress ApplyToProgress(EFlickMatchVariant Variant, int32 PlayersPerTeam,
		const FFlickRankProgress& Actual, const FString& Ini = FString());

#if !UE_BUILD_SHIPPING
	void SetRating(EFlickMatchVariant Variant, int32 PlayersPerTeam, int32 Rating, const FString& Ini = FString());
	void ClearRating(EFlickMatchVariant Variant, int32 PlayersPerTeam, const FString& Ini = FString());
	void ClearAll(const FString& Ini = FString());
#endif
}
