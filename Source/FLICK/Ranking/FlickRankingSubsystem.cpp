#include "Ranking/FlickRankingSubsystem.h"

#include "Core/FlickLog.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Ranking/FlickRankingService.h"
#include "Misc/ConfigCacheIni.h"

void UFlickRankingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RankingService = MakeUnique<FConfigFlickRankingService>(ResolveAccountId(), ResolveSeasonId());
}

FString UFlickRankingSubsystem::ResolveSeasonId() const
{
	FString SeasonId = TEXT("PRESEASON");
	GConfig->GetString(TEXT("FLICK.RankedBackend"), TEXT("SeasonId"), SeasonId, GGameIni);
	return SeasonId.IsEmpty() ? TEXT("PRESEASON") : SeasonId.Left(64);
}

void UFlickRankingSubsystem::Deinitialize()
{
	RankingService.Reset();
	Super::Deinitialize();
}

FString UFlickRankingSubsystem::ResolveAccountId() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	return UserId.IsValid() ? UserId->ToString() : TEXT("LOCAL_DEVELOPMENT");
}

FFlickRankProgress UFlickRankingSubsystem::GetProgress(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam) const
{
	return RankingService
		? RankingService->LoadProgress(Variant, PlayersPerTeam)
		: FFlickRankProgress();
}

int32 UFlickRankingSubsystem::GetQueueRating(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam) const
{
	return GetProgress(Variant, PlayersPerTeam).Rating;
}

FString UFlickRankingSubsystem::GetProgressLabel(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam) const
{
	return FlickRankRules::GetProgressLabel(GetProgress(Variant, PlayersPerTeam));
}

void UFlickRankingSubsystem::BeginRankedMatch(
	const FString& MatchId,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 OpponentRating)
{
	if (RankingService && !MatchId.IsEmpty())
	{
		RankingService->SavePendingMatch(MatchId, Variant, PlayersPerTeam, OpponentRating);
		LastRatingUpdate = FFlickRatingUpdate();
	}
}

void UFlickRankingSubsystem::ApplyTrustedProgress(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const FFlickRankProgress& Progress)
{
	if (RankingService)
	{
		RankingService->CacheAuthoritativeProgress(Variant, PlayersPerTeam, Progress);
	}
}

bool UFlickRankingSubsystem::ApplyTrustedUpdate(const FFlickRatingUpdate& Update)
{
	if (!RankingService || !Update.bAccepted || Update.MatchId.IsEmpty())
	{
		return false;
	}
	RankingService->CacheAuthoritativeUpdate(Update);
	LastRatingUpdate = Update;
	UE_LOG(
		LogFlick,
		Log,
		TEXT("TRUSTED_RANKED_SNAPSHOT: match=%s delta=%+d rating=%d"),
		*Update.MatchId,
		Update.RatingDelta,
		Update.NewRating);
	return true;
}

bool UFlickRankingSubsystem::HasRatingUpdateForMatch(const FString& MatchId) const
{
	return LastRatingUpdate.bAccepted && LastRatingUpdate.MatchId == MatchId;
}
