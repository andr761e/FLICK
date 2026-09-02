#pragma once

#include "CoreMinimal.h"
#include "Core/FlickRankRules.h"
#include "Ranking/FlickRankingService.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlickRankingSubsystem.generated.h"

UCLASS()
class FLICK_API UFlickRankingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	FFlickRankProgress GetProgress(EFlickMatchVariant Variant, int32 PlayersPerTeam) const;
	int32 GetQueueRating(EFlickMatchVariant Variant, int32 PlayersPerTeam) const;
	FString GetProgressLabel(EFlickMatchVariant Variant, int32 PlayersPerTeam) const;
	void BeginRankedMatch(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating);
	void ApplyTrustedProgress(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		const FFlickRankProgress& Progress);
	bool ApplyTrustedUpdate(const FFlickRatingUpdate& Update);
	const FFlickRatingUpdate& GetLastRatingUpdate() const { return LastRatingUpdate; }
	bool HasRatingUpdateForMatch(const FString& MatchId) const;

private:
	FString ResolveAccountId() const;
	FString ResolveSeasonId() const;

	TUniquePtr<IFlickRankingService> RankingService;
	FFlickRatingUpdate LastRatingUpdate;
};
