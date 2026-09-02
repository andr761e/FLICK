#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

enum class EFlickCoordinatorMode : uint8
{
	SessionFallback,
	LocalHttp,
	RemoteHttp
};

enum class EFlickCoordinatorQueueState : uint8
{
	Idle,
	Submitting,
	Searching,
	Allocated,
	Cancelling,
	Error
};

struct FFlickCoordinatorPartyMember
{
	FString AccountId;
	FString DisplayName;
	FString SteamAuthTicket;
	int32 PartySlot = 0;
	int32 Rating = 1000;
};

struct FFlickCoordinatorQueueRequest
{
	FString PartyId;
	FString Region = TEXT("auto");
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;
	int32 PlayersPerTeam = 1;
	bool bRanked = false;
	TArray<FFlickCoordinatorPartyMember> Members;
};

struct FFlickCoordinatorReservation
{
	FString AccountId;
	FString Token;
	EFlickTeam Team = EFlickTeam::None;
	int32 PlayerSlot = INDEX_NONE;
};

struct FFlickCoordinatorAllocation
{
	FString MatchId;
	FString ServerId;
	FString Address;
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;
	int32 PlayersPerTeam = 1;
	bool bRanked = false;
	int64 ExpiresUnixTime = 0;
	TArray<FFlickCoordinatorReservation> Reservations;
};

struct FFlickCoordinatorReservationResult
{
	bool bAccepted = false;
	FString AccountId;
	EFlickTeam Team = EFlickTeam::None;
	int32 PlayerSlot = INDEX_NONE;
	int64 ReconnectDeadlineUnixTime = 0;
	FString Error;
};

using FFlickCoordinatorQueueCallback = TFunction<void(bool, const FString&)>;
using FFlickCoordinatorReservationCallback = TFunction<void(const FFlickCoordinatorReservationResult&)>;

