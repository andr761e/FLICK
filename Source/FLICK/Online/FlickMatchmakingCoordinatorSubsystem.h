#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Online/FlickMatchmakingCoordinatorTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlickMatchmakingCoordinatorSubsystem.generated.h"

class APlayerController;
class FJsonObject;
class IHttpResponse;

DECLARE_MULTICAST_DELEGATE(FOnFlickCoordinatorChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFlickCoordinatorAllocated, const FFlickCoordinatorAllocation&);

UCLASS()
class FLICK_API UFlickMatchmakingCoordinatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool ShouldUseCoordinator() const { return Mode != EFlickCoordinatorMode::SessionFallback; }
	bool RequiresSteamTickets() const { return bRequireSteamAuthentication; }
	bool IsQueueActive() const;
	bool HasReconnectReservation() const;
	EFlickCoordinatorQueueState GetQueueState() const { return QueueState; }
	const FString& GetStatusMessage() const { return StatusMessage; }
	const FString& GetSteamTicketAudience() const { return SteamTicketAudience; }
	float GetReconnectGraceSeconds() const { return ReconnectGraceSeconds; }
	const FFlickCoordinatorAllocation& GetLastAllocation() const { return LastAllocation; }

	bool QueueParty(const FFlickCoordinatorQueueRequest& Request, FFlickCoordinatorQueueCallback Callback);
	bool CancelQueue();
	void VerifyReservation(
		const FString& MatchId,
		const FString& AccountId,
		const FString& ReservationToken,
		FFlickCoordinatorReservationCallback Callback);
	void AdoptLocalReservation(
		const FFlickCoordinatorAllocation& Allocation,
		const FFlickCoordinatorReservation& Reservation,
		const FString& PartyId,
		int32 PartySlot,
		int32 PartySize,
		bool bPartyLeader);
	bool TravelToAllocatedMatch(APlayerController* LocalController);
	bool TryReconnect(APlayerController* LocalController);
	void ClearReconnectReservation();
	void BeginServerHeartbeat(const FString& MatchId);
	void NotifyServerMatchComplete(EFlickMatchOutcome Outcome, bool bForfeit);

	FOnFlickCoordinatorChanged OnCoordinatorChanged;
	FOnFlickCoordinatorAllocated OnAllocated;

private:
	using FJsonResponseCallback = TFunction<void(bool, const TSharedPtr<FJsonObject>&, const FString&)>;

	void PollQueue();
	void SchedulePoll(float DelaySeconds = -1.0f);
	void SendHeartbeat();
	void ScheduleHeartbeat();
	void SetState(EFlickCoordinatorQueueState NewState, const FString& Message);
	void SendJsonRequest(
		const FString& Verb,
		const FString& Path,
		const TSharedPtr<FJsonObject>& Body,
		bool bServerRequest,
		FJsonResponseCallback Callback);
	bool ParseAllocation(const TSharedPtr<FJsonObject>& Json, FFlickCoordinatorAllocation& OutAllocation) const;
	FString BuildTravelAddress() const;

	EFlickCoordinatorMode Mode = EFlickCoordinatorMode::SessionFallback;
	EFlickCoordinatorQueueState QueueState = EFlickCoordinatorQueueState::Idle;
	FString BaseUrl;
	FString Region = TEXT("auto");
	FString SteamTicketAudience = TEXT("FLICK");
	FString ServerApiKeyEnvironmentVariable = TEXT("FLICK_BACKEND_SERVER_KEY");
	FString ServerApiKey;
	FString ServerId;
	FString QueueTicketId;
	FString StatusMessage = TEXT("STEAM SESSION MATCHMAKING READY");
	FString HeartbeatMatchId;
	float PollIntervalSeconds = 1.0f;
	float HeartbeatIntervalSeconds = 5.0f;
	float RequestTimeoutSeconds = 10.0f;
	float ReconnectGraceSeconds = 45.0f;
	bool bRequireSteamAuthentication = false;
	bool bRequestInFlight = false;
	bool bCompletionReported = false;
	FFlickCoordinatorQueueRequest ActiveRequest;
	FFlickCoordinatorAllocation LastAllocation;
	FFlickCoordinatorReservation LocalReservation;
	FString LocalPartyId;
	int32 LocalPartySlot = 0;
	int32 LocalPartySize = 1;
	bool bLocalPartyLeader = true;
	int32 ReconnectAttempts = 0;
	FTimerHandle PollTimer;
	FTimerHandle HeartbeatTimer;
};
