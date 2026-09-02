#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

enum class EFlickRankMatchResult : uint8
{
	Loss,
	Draw,
	Win
};

struct FFlickRankProgress
{
	int32 Rating = 1000;
	int32 MatchesPlayed = 0;
	int32 Wins = 0;
	int32 Losses = 0;
	int32 Draws = 0;
};

struct FFlickRatingUpdate
{
	FString MatchId;
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;
	int32 PlayersPerTeam = 1;
	int32 OldRating = 1000;
	int32 NewRating = 1000;
	int32 RatingDelta = 0;
	int32 MatchesPlayed = 0;
	EFlickRankTier OldTier = EFlickRankTier::Unranked;
	EFlickRankTier NewTier = EFlickRankTier::Unranked;
	int32 OldDivision = 0;
	int32 NewDivision = 0;
	bool bAccepted = false;
	bool bForfeit = false;
};

namespace FlickRankRules
{
	constexpr int32 PlacementMatchCount = 5;
	constexpr int32 DefaultRating = 1000;
	constexpr int32 MinimumRating = 0;
	constexpr int32 MaximumRating = 3000;

	FString MakePlaylistKey(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	int32 GetSearchRangeForAttempt(int32 SearchAttempt);
	int32 CalculateRatingDelta(
		int32 Rating,
		int32 OpponentRating,
		EFlickRankMatchResult Result,
		bool bInPlacements);
	EFlickRankTier GetTier(const FFlickRankProgress& Progress);
	int32 GetDivision(const FFlickRankProgress& Progress);
	int32 GetPlacementsRemaining(const FFlickRankProgress& Progress);
	FString GetTierName(EFlickRankTier Tier);
	FString GetDivisionName(int32 Division);
	FString GetRankLabel(const FFlickRankProgress& Progress);
	FString GetProgressLabel(const FFlickRankProgress& Progress);
}
