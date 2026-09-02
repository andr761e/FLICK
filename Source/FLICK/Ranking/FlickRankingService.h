#pragma once

#include "CoreMinimal.h"
#include "Core/FlickRankRules.h"

class IFlickRankingService
{
public:
	virtual ~IFlickRankingService() = default;

	virtual FFlickRankProgress LoadProgress(EFlickMatchVariant Variant, int32 PlayersPerTeam) const = 0;
	virtual bool SubmitMatchResult(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating,
		EFlickRankMatchResult Result,
		bool bForfeit,
		FFlickRatingUpdate& OutUpdate) = 0;
	virtual void SavePendingMatch(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating) = 0;
	virtual bool LoadPendingMatch(
		FString& OutMatchId,
		EFlickMatchVariant& OutVariant,
		int32& OutPlayersPerTeam,
		int32& OutOpponentRating) const = 0;
	virtual void ClearPendingMatch(const FString& MatchId) = 0;
	virtual void CacheAuthoritativeUpdate(const FFlickRatingUpdate& Update) = 0;
	virtual void CacheAuthoritativeProgress(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		const FFlickRankProgress& Progress) = 0;
};

class FConfigFlickRankingService final : public IFlickRankingService
{
public:
	explicit FConfigFlickRankingService(FString InAccountId, FString InSeasonId = TEXT("PRESEASON"));

	virtual FFlickRankProgress LoadProgress(EFlickMatchVariant Variant, int32 PlayersPerTeam) const override;
	virtual bool SubmitMatchResult(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating,
		EFlickRankMatchResult Result,
		bool bForfeit,
		FFlickRatingUpdate& OutUpdate) override;
	virtual void SavePendingMatch(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating) override;
	virtual bool LoadPendingMatch(
		FString& OutMatchId,
		EFlickMatchVariant& OutVariant,
		int32& OutPlayersPerTeam,
		int32& OutOpponentRating) const override;
	virtual void ClearPendingMatch(const FString& MatchId) override;
	virtual void CacheAuthoritativeUpdate(const FFlickRatingUpdate& Update) override;
	virtual void CacheAuthoritativeProgress(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		const FFlickRankProgress& Progress) override;

private:
	FString GetPlaylistSection(EFlickMatchVariant Variant, int32 PlayersPerTeam) const;
	void SaveProgress(EFlickMatchVariant Variant, int32 PlayersPerTeam, const FFlickRankProgress& Progress) const;
	bool HasProcessedMatch(const FString& Section, const FString& MatchId) const;
	void RecordProcessedMatch(const FString& Section, const FString& MatchId) const;

	FString AccountId;
	FString SeasonId;
};
