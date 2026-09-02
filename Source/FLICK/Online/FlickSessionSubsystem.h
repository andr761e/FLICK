#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlickSessionSubsystem.generated.h"

class FOnlineSessionSearch;
class FOnlineSessionSearchResult;
class UFlickMatchmakingCoordinatorSubsystem;

enum class EFlickSessionState : uint8
{
	Idle,
	Creating,
	Searching,
	Joining,
	Queued,
	InSession,
	Destroying,
	Error
};

enum class EFlickSessionPurpose : uint8
{
	Match,
	Party,
	Matchmaking
};

struct FFlickSessionBrowserEntry
{
	FString OwnerName;
	FString SessionId;
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;
	int32 CurrentPlayers = 0;
	int32 MaximumPlayers = 0;
	int32 PingMilliseconds = 0;
	int32 SearchResultIndex = INDEX_NONE;
	int32 PlayersPerTeam = 1;
	int32 QueueRating = 1000;
	bool bMatchmaking = false;
	bool bRanked = false;
};

struct FFlickSocialPlayerEntry
{
	FString DisplayName;
	FString UserId;
	FString Status;
	bool bOnline = false;
	bool bPlayingFlick = false;
	bool bJoinable = false;
};

struct FFlickRecentPlayerEntry
{
	FString DisplayName;
	FString UserId;
	FDateTime LastEncounteredUtc;
};

DECLARE_MULTICAST_DELEGATE(FOnFlickSessionsChanged);

UCLASS()
class FLICK_API UFlickSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool HostSession(EFlickMatchVariant Variant, int32 MaximumPlayers = 2);
	bool FindSessions();
	bool JoinSession(int32 ResultIndex);
	bool UpdateSessionVariant(EFlickMatchVariant Variant);
	bool ConvertPartyToPrivateMatch(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	bool StartMatchmaking(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 PartySize = 1,
		bool bRanked = false,
		int32 QueueRating = 1000);
	bool ConvertPartyToMatchmakingQueue(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 PartySize,
		bool bRanked = false,
		int32 QueueRating = 1000);
	bool ConvertMatchmakingToParty(int32 MaximumPartyMembers = FlickMaximumPartyMembers);
	bool LockMatchmakingLobby();
	bool ReopenMatchmakingLobby();
	bool CancelMatchmaking(bool bReturnToFrontend = false);
	bool LeaveSession(bool bReturnToFrontend = true);
	bool BeginPartyMatchMigration(const FString& TargetSessionId, const FString& PartyId, int32 PartySlot, int32 PartySize, bool bLeader);
	bool RestorePersistentParty(bool bLeader);
	void SetPersistentPartyIdentity(const FString& PartyId, int32 PartySlot, int32 PartySize, bool bLeader);
	void ClearPersistentPartyIdentity();
	bool OpenInviteOverlay();
	bool RefreshFriends();
	bool InviteFriendToParty(int32 FriendIndex);
	bool InviteRecentPlayerToParty(int32 RecentPlayerIndex);
	bool SendPendingPartyInvite();
	void RecordRecentPlayer(const FString& UserId, const FString& DisplayName);

	bool IsSteamAvailable() const;
	bool HasActiveSession() const;
	bool IsPartySession() const { return ActivePurpose == EFlickSessionPurpose::Party && HasActiveSession(); }
	bool HasPersistentPartyIdentity() const { return !PersistentPartyId.IsEmpty() && PersistentPartySlot != INDEX_NONE; }
	bool IsPersistentPartyLeader() const { return HasPersistentPartyIdentity() && bPersistentPartyLeader; }
	const FString& GetPersistentPartyId() const { return PersistentPartyId; }
	int32 GetPersistentPartySlot() const { return PersistentPartySlot; }
	int32 GetPersistentPartySize() const { return PersistentPartySize; }
	bool IsMatchmakingActive() const;
	int32 GetMatchmakingPlayersPerTeam() const { return PendingPlayersPerTeam; }
	int32 GetMatchmakingPartySize() const { return PendingPartySize; }
	bool IsRankedMatchmaking() const { return bPendingRanked; }
	int32 GetPendingQueueRating() const { return PendingQueueRating; }
	int32 GetRankedSearchRange() const { return CurrentRankedSearchRange; }
	EFlickSessionState GetState() const;
	const FString& GetStatusMessage() const;
	const TArray<FFlickSessionBrowserEntry>& GetBrowserEntries() const { return BrowserEntries; }
	const TArray<FFlickSocialPlayerEntry>& GetFriends() const { return Friends; }
	const TArray<FFlickRecentPlayerEntry>& GetRecentPlayers() const { return RecentPlayers; }
	FString GetOnlineServiceName() const;
	FString GetLocalDisplayName() const;

	FOnFlickSessionsChanged OnSessionsChanged;

private:
	bool RegisterOnlineDelegates();
	bool BeginCreateSession(EFlickMatchVariant Variant, int32 MaximumPlayers, EFlickSessionPurpose Purpose);
	bool JoinSearchResult(const FOnlineSessionSearchResult& SearchResult);
	bool OpenCurrentPartyAsMatchmakingQueue();
	bool BeginTargetSessionSearch();
	bool BeginPartyRestoreSearch();
	void ScheduleRankedSearchRetry();
	void SchedulePartyRestoreRetry();
	bool ShouldLeadPartyRestorationAfterFailure() const;
	bool InvitePlayerToParty(const FString& UserId, const FString& DisplayName);
	bool SendPartyInvite(const FString& FriendUserId, const FString& FriendDisplayName);
	void LoadRecentPlayers();
	void SaveRecentPlayers() const;
	void BeginDestroySession(bool bReturnToFrontend);
	void TravelToFrontend();
	void SetState(EFlickSessionState NewState, const FString& Message);
	void ClearOnlineDelegates();

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleReadFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorString);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleInviteAccepted(
		bool bWasSuccessful,
		int32 ControllerId,
		FUniqueNetIdPtr UserId,
		const FOnlineSessionSearchResult& InviteResult);
	void HandleNetworkFailure(
		UWorld* World,
		class UNetDriver* NetDriver,
		ENetworkFailure::Type FailureType,
		const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	TSharedPtr<FOnlineSessionSearch> ActiveSearch;
	TSharedPtr<FOnlineSessionSearchResult> PendingJoinResult;
	TArray<FFlickSessionBrowserEntry> BrowserEntries;
	TArray<FFlickSocialPlayerEntry> Friends;
	TArray<FFlickRecentPlayerEntry> RecentPlayers;
	EFlickSessionState State = EFlickSessionState::Idle;
	FString StatusMessage = TEXT("STEAM SESSION SERVICE READY");
	EFlickMatchVariant PendingHostVariant = EFlickMatchVariant::Classic;
	int32 PendingMaximumPlayers = 2;
	int32 PendingPlayersPerTeam = 1;
	int32 PendingPartySize = 1;
	int32 PendingQueueRating = 1000;
	int32 MatchmakingSearchAttempt = 0;
	int32 CurrentRankedSearchRange = 100;
	EFlickSessionPurpose PendingPurpose = EFlickSessionPurpose::Match;
	EFlickSessionPurpose ActivePurpose = EFlickSessionPurpose::Match;
	FString PendingPartyInviteUserId;
	FString PendingPartyInviteDisplayName;
	bool bHostAfterDestroy = false;
	bool bJoinAfterDestroy = false;
	bool bReturnAfterDestroy = false;
	bool bMatchmakingActive = false;
	bool bMatchmakingSearch = false;
	bool bPendingRanked = false;
	bool bRankedRetryScheduled = false;
	bool bPartyMatchmakingSearch = false;
	bool bTargetSessionSearch = false;
	bool bFindTargetAfterDestroy = false;
	bool bPartyRestoreSearch = false;
	bool bIgnoreNextFindCompletion = false;
	bool bRestorePartyAfterDestroy = false;
	bool bRestoreAsPartyLeader = false;
	FString PendingTargetSessionId;
	FString PersistentPartyId;
	int32 PersistentPartySlot = INDEX_NONE;
	int32 PersistentPartySize = 0;
	bool bPersistentPartyLeader = false;
	int32 PartyRestoreAttempts = 0;

	FDelegateHandle CreateSessionHandle;
	FDelegateHandle FindSessionsHandle;
	FDelegateHandle JoinSessionHandle;
	FDelegateHandle DestroySessionHandle;
	FDelegateHandle InviteAcceptedHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;
};
