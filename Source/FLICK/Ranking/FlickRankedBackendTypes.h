#pragma once

#include "CoreMinimal.h"
#include "Core/FlickRankRules.h"

enum class EFlickRankedBackendMode : uint8
{
	LocalDevelopment,
	RemoteHttp
};

struct FFlickRankedParticipant
{
	FString AccountId;
	EFlickTeam Team = EFlickTeam::None;
	int32 PlayerSlot = INDEX_NONE;
	int32 Rating = FlickRankRules::DefaultRating;
};

struct FFlickRankedAuthenticationResult
{
	bool bAuthenticated = false;
	FString AccountId;
	FFlickRankProgress Progress;
	FString Error;
};

struct FFlickRankedMatchRequest
{
	FString MatchId;
	FString SeasonId;
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;
	int32 PlayersPerTeam = 1;
	int64 StartedUnixTime = 0;
	TArray<FFlickRankedParticipant> Participants;
};

struct FFlickRankedMatchResultRequest
{
	FFlickRankedMatchRequest Match;
	EFlickMatchOutcome Outcome = EFlickMatchOutcome::Continue;
	bool bForfeit = false;
	int64 CompletedUnixTime = 0;
};

struct FFlickRankedPlayerUpdate
{
	FString AccountId;
	FFlickRatingUpdate RatingUpdate;
};

struct FFlickRankedSettlementResult
{
	bool bAccepted = false;
	bool bDuplicate = false;
	FString Error;
	TArray<FFlickRankedPlayerUpdate> PlayerUpdates;
};

using FFlickRankedAuthenticationCallback = TFunction<void(const FFlickRankedAuthenticationResult&)>;
using FFlickRankedMatchRegistrationCallback = TFunction<void(bool, const FString&)>;
using FFlickRankedSettlementCallback = TFunction<void(const FFlickRankedSettlementResult&)>;

