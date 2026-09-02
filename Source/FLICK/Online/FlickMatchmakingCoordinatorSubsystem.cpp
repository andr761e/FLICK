#include "Online/FlickMatchmakingCoordinatorSubsystem.h"

#include "Core/FlickLog.h"
#include "Dom/JsonObject.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* CoordinatorSection = TEXT("FLICK.MatchmakingCoordinator");

	FString CleanCoordinatorValue(FString Value, const int32 MaximumLength)
	{
		Value.TrimStartAndEndInline();
		return Value.Left(MaximumLength);
	}

	int32 ReadJsonInt(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, const int32 DefaultValue)
	{
		double Value = DefaultValue;
		return Json.IsValid() && Json->TryGetNumberField(Field, Value)
			? FMath::RoundToInt(Value)
			: DefaultValue;
	}
}

void UFlickMatchmakingCoordinatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FString ModeValue = TEXT("SessionFallback");
	GConfig->GetString(CoordinatorSection, TEXT("Mode"), ModeValue, GGameIni);
	if (ModeValue.Equals(TEXT("RemoteHttp"), ESearchCase::IgnoreCase))
	{
		Mode = EFlickCoordinatorMode::RemoteHttp;
	}
	else if (ModeValue.Equals(TEXT("LocalHttp"), ESearchCase::IgnoreCase))
	{
		Mode = EFlickCoordinatorMode::LocalHttp;
	}
	GConfig->GetString(CoordinatorSection, TEXT("BaseUrl"), BaseUrl, GGameIni);
	GConfig->GetString(CoordinatorSection, TEXT("Region"), Region, GGameIni);
	GConfig->GetString(CoordinatorSection, TEXT("SteamTicketAudience"), SteamTicketAudience, GGameIni);
	GConfig->GetString(CoordinatorSection, TEXT("ServerApiKeyEnvironmentVariable"), ServerApiKeyEnvironmentVariable, GGameIni);
	GConfig->GetBool(CoordinatorSection, TEXT("bRequireSteamAuthentication"), bRequireSteamAuthentication, GGameIni);
	GConfig->GetFloat(CoordinatorSection, TEXT("PollIntervalSeconds"), PollIntervalSeconds, GGameIni);
	GConfig->GetFloat(CoordinatorSection, TEXT("HeartbeatIntervalSeconds"), HeartbeatIntervalSeconds, GGameIni);
	GConfig->GetFloat(CoordinatorSection, TEXT("RequestTimeoutSeconds"), RequestTimeoutSeconds, GGameIni);
	GConfig->GetFloat(CoordinatorSection, TEXT("ReconnectGraceSeconds"), ReconnectGraceSeconds, GGameIni);

	FString CommandLineUrl;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickCoordinatorUrl="), CommandLineUrl))
	{
		BaseUrl = MoveTemp(CommandLineUrl);
		Mode = EFlickCoordinatorMode::LocalHttp;
	}

	BaseUrl = CleanCoordinatorValue(MoveTemp(BaseUrl), 512);
	BaseUrl.RemoveFromEnd(TEXT("/"));
	Region = CleanCoordinatorValue(MoveTemp(Region), 32);
	SteamTicketAudience = CleanCoordinatorValue(MoveTemp(SteamTicketAudience), 128);
	ServerApiKeyEnvironmentVariable = CleanCoordinatorValue(MoveTemp(ServerApiKeyEnvironmentVariable), 128);
	ServerApiKey = FPlatformMisc::GetEnvironmentVariable(*ServerApiKeyEnvironmentVariable);
	ServerId = FPlatformMisc::GetEnvironmentVariable(TEXT("FLICK_SERVER_ID"));
	FParse::Value(FCommandLine::Get(), TEXT("FlickServerId="), ServerId);
	PollIntervalSeconds = FMath::Clamp(PollIntervalSeconds, 0.25f, 10.0f);
	HeartbeatIntervalSeconds = FMath::Clamp(HeartbeatIntervalSeconds, 1.0f, 30.0f);
	RequestTimeoutSeconds = FMath::Clamp(RequestTimeoutSeconds, 2.0f, 30.0f);
	ReconnectGraceSeconds = FMath::Clamp(ReconnectGraceSeconds, 10.0f, 180.0f);

#if UE_BUILD_SHIPPING
	Mode = EFlickCoordinatorMode::RemoteHttp;
	bRequireSteamAuthentication = true;
#endif

	if (ShouldUseCoordinator() && BaseUrl.IsEmpty())
	{
		Mode = EFlickCoordinatorMode::SessionFallback;
		StatusMessage = TEXT("COORDINATOR URL MISSING - USING STEAM SESSION MATCHMAKING");
	}
#if UE_BUILD_SHIPPING
	else if (!BaseUrl.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase))
	{
		Mode = EFlickCoordinatorMode::SessionFallback;
		StatusMessage = TEXT("SHIPPING COORDINATOR REQUIRES HTTPS");
	}
#endif
	else if (ShouldUseCoordinator())
	{
		StatusMessage = FString::Printf(TEXT("COORDINATOR READY / %s"), *Region.ToUpper());
	}

	UE_LOG(LogFlick, Log, TEXT("MATCH_COORDINATOR: %s"), *StatusMessage);
}

void UFlickMatchmakingCoordinatorSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimer);
		World->GetTimerManager().ClearTimer(HeartbeatTimer);
	}
	QueueTicketId.Reset();
	HeartbeatMatchId.Reset();
	Super::Deinitialize();
}

bool UFlickMatchmakingCoordinatorSubsystem::IsQueueActive() const
{
	return QueueState == EFlickCoordinatorQueueState::Submitting
		|| QueueState == EFlickCoordinatorQueueState::Searching
		|| QueueState == EFlickCoordinatorQueueState::Cancelling;
}

bool UFlickMatchmakingCoordinatorSubsystem::HasReconnectReservation() const
{
	return QueueState == EFlickCoordinatorQueueState::Allocated
		&& !LastAllocation.Address.IsEmpty()
		&& !LastAllocation.MatchId.IsEmpty()
		&& !LocalReservation.Token.IsEmpty();
}

bool UFlickMatchmakingCoordinatorSubsystem::QueueParty(
	const FFlickCoordinatorQueueRequest& Request,
	FFlickCoordinatorQueueCallback Callback)
{
	if (!ShouldUseCoordinator() || BaseUrl.IsEmpty() || IsQueueActive() || Request.Members.IsEmpty())
	{
		return false;
	}
	ActiveRequest = Request;
	ActiveRequest.Region = Region.IsEmpty() ? TEXT("auto") : Request.Region;
	QueueTicketId.Reset();
	LastAllocation = FFlickCoordinatorAllocation();
	LocalReservation = FFlickCoordinatorReservation();
	ReconnectAttempts = 0;
	SetState(EFlickCoordinatorQueueState::Submitting, TEXT("CONTACTING MATCHMAKING COORDINATOR..."));

	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("party_id"), Request.PartyId);
	Json->SetStringField(TEXT("region"), ActiveRequest.Region);
	Json->SetNumberField(TEXT("variant"), static_cast<int32>(Request.Variant));
	Json->SetNumberField(TEXT("players_per_team"), Request.PlayersPerTeam);
	Json->SetBoolField(TEXT("ranked"), Request.bRanked);
	TArray<TSharedPtr<FJsonValue>> Members;
	for (const FFlickCoordinatorPartyMember& Member : Request.Members)
	{
		const TSharedRef<FJsonObject> MemberJson = MakeShared<FJsonObject>();
		MemberJson->SetStringField(TEXT("account_id"), Member.AccountId);
		MemberJson->SetStringField(TEXT("display_name"), Member.DisplayName);
		MemberJson->SetStringField(TEXT("steam_ticket"), Member.SteamAuthTicket);
		MemberJson->SetNumberField(TEXT("party_slot"), Member.PartySlot);
		MemberJson->SetNumberField(TEXT("rating_snapshot"), Member.Rating);
		Members.Add(MakeShared<FJsonValueObject>(MemberJson));
	}
	Json->SetArrayField(TEXT("members"), MoveTemp(Members));

	SendJsonRequest(TEXT("POST"), TEXT("/v1/matchmaking/tickets"), Json, false,
		[this, Callback = MoveTemp(Callback)](const bool bSuccess, const TSharedPtr<FJsonObject>& Response, const FString& Error) mutable
		{
			if (!bSuccess || !Response.IsValid() || !Response->TryGetStringField(TEXT("ticket_id"), QueueTicketId))
			{
				SetState(EFlickCoordinatorQueueState::Error,
					Error.IsEmpty() ? TEXT("MATCHMAKING COORDINATOR REJECTED THE QUEUE") : Error.ToUpper());
				Callback(false, Error);
				return;
			}
			SetState(EFlickCoordinatorQueueState::Searching, TEXT("SEARCHING FOR A DEDICATED MATCH..."));
			Callback(true, FString());
			SchedulePoll(0.15f);
		});
	return true;
}

bool UFlickMatchmakingCoordinatorSubsystem::CancelQueue()
{
	if (!IsQueueActive() || QueueTicketId.IsEmpty())
	{
		return false;
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimer);
	}
	bRequestInFlight = false;
	SetState(EFlickCoordinatorQueueState::Cancelling, TEXT("LEAVING DEDICATED QUEUE..."));
	const FString TicketPath = FString::Printf(
		TEXT("/v1/matchmaking/tickets/%s"),
		*FGenericPlatformHttp::UrlEncode(QueueTicketId));
	SendJsonRequest(TEXT("DELETE"), TicketPath, nullptr, false,
		[this](const bool bSuccess, const TSharedPtr<FJsonObject>&, const FString& Error)
		{
			QueueTicketId.Reset();
			SetState(
				bSuccess ? EFlickCoordinatorQueueState::Idle : EFlickCoordinatorQueueState::Error,
				bSuccess ? TEXT("MATCHMAKING CANCELLED") : Error.ToUpper());
		});
	return true;
}

void UFlickMatchmakingCoordinatorSubsystem::PollQueue()
{
	if (QueueState != EFlickCoordinatorQueueState::Searching || QueueTicketId.IsEmpty() || bRequestInFlight)
	{
		return;
	}
	bRequestInFlight = true;
	const FString TicketPath = FString::Printf(
		TEXT("/v1/matchmaking/tickets/%s"),
		*FGenericPlatformHttp::UrlEncode(QueueTicketId));
	SendJsonRequest(TEXT("GET"), TicketPath, nullptr, false,
		[this](const bool bSuccess, const TSharedPtr<FJsonObject>& Response, const FString& Error)
		{
			bRequestInFlight = false;
			if (!bSuccess || !Response.IsValid())
			{
				SetState(EFlickCoordinatorQueueState::Error,
					Error.IsEmpty() ? TEXT("MATCHMAKING COORDINATOR BECAME UNAVAILABLE") : Error.ToUpper());
				return;
			}
			FString Status;
			Response->TryGetStringField(TEXT("status"), Status);
			if (Status.Equals(TEXT("allocated"), ESearchCase::IgnoreCase))
			{
				FFlickCoordinatorAllocation Allocation;
				if (!ParseAllocation(Response, Allocation))
				{
					SetState(EFlickCoordinatorQueueState::Error, TEXT("COORDINATOR RETURNED AN INVALID ALLOCATION"));
					return;
				}
				LastAllocation = Allocation;
				SetState(EFlickCoordinatorQueueState::Allocated, TEXT("MATCH FOUND - CONNECTING TO DEDICATED SERVER"));
				OnAllocated.Broadcast(LastAllocation);
				return;
			}
			if (Status.Equals(TEXT("cancelled"), ESearchCase::IgnoreCase)
				|| Status.Equals(TEXT("expired"), ESearchCase::IgnoreCase))
			{
				QueueTicketId.Reset();
				SetState(EFlickCoordinatorQueueState::Idle, TEXT("MATCHMAKING TICKET EXPIRED"));
				return;
			}
			if (Status.Equals(TEXT("error"), ESearchCase::IgnoreCase))
			{
				FString AllocationError;
				Response->TryGetStringField(TEXT("error"), AllocationError);
				SetState(
					EFlickCoordinatorQueueState::Error,
					AllocationError.IsEmpty() ? TEXT("DEDICATED SERVER ALLOCATION FAILED") : AllocationError.ToUpper());
				return;
			}
			SchedulePoll();
		});
}

void UFlickMatchmakingCoordinatorSubsystem::SchedulePoll(const float DelaySeconds)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PollTimer,
			this,
			&UFlickMatchmakingCoordinatorSubsystem::PollQueue,
			DelaySeconds >= 0.0f ? DelaySeconds : PollIntervalSeconds,
			false);
	}
}

void UFlickMatchmakingCoordinatorSubsystem::VerifyReservation(
	const FString& MatchId,
	const FString& AccountId,
	const FString& ReservationToken,
	FFlickCoordinatorReservationCallback Callback)
{
	FFlickCoordinatorReservationResult ImmediateFailure;
	if (!ShouldUseCoordinator() || MatchId.IsEmpty() || AccountId.IsEmpty() || ReservationToken.IsEmpty())
	{
		ImmediateFailure.Error = TEXT("Reservation credentials are incomplete.");
		Callback(ImmediateFailure);
		return;
	}
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("match_id"), MatchId);
	Json->SetStringField(TEXT("account_id"), AccountId);
	Json->SetStringField(TEXT("reservation_token"), ReservationToken);
	SendJsonRequest(TEXT("POST"), TEXT("/v1/matchmaking/reservations/verify"), Json, true,
		[Callback = MoveTemp(Callback)](const bool bSuccess, const TSharedPtr<FJsonObject>& Response, const FString& Error) mutable
		{
			FFlickCoordinatorReservationResult Result;
			if (bSuccess && Response.IsValid())
			{
				Response->TryGetBoolField(TEXT("accepted"), Result.bAccepted);
				Response->TryGetStringField(TEXT("account_id"), Result.AccountId);
				Result.Team = static_cast<EFlickTeam>(FMath::Clamp(ReadJsonInt(Response, TEXT("team"), 0), 0, 2));
				Result.PlayerSlot = FMath::Clamp(ReadJsonInt(Response, TEXT("player_slot"), INDEX_NONE), INDEX_NONE, 2);
				Result.ReconnectDeadlineUnixTime = static_cast<int64>(ReadJsonInt(Response, TEXT("reconnect_deadline_unix"), 0));
			}
			if (!Result.bAccepted)
			{
				Result.Error = Error.IsEmpty() ? TEXT("The server rejected the match reservation.") : Error;
			}
			Callback(Result);
		});
}

void UFlickMatchmakingCoordinatorSubsystem::AdoptLocalReservation(
	const FFlickCoordinatorAllocation& Allocation,
	const FFlickCoordinatorReservation& Reservation,
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bPartyLeader)
{
	LastAllocation = Allocation;
	LocalReservation = Reservation;
	LocalPartyId = PartyId.Left(64);
	LocalPartySlot = FMath::Clamp(PartySlot, 0, 2);
	LocalPartySize = FMath::Clamp(PartySize, 1, 3);
	bLocalPartyLeader = bPartyLeader;
	QueueState = EFlickCoordinatorQueueState::Allocated;
	ReconnectAttempts = 0;
}

FString UFlickMatchmakingCoordinatorSubsystem::BuildTravelAddress() const
{
	FString Address = LastAllocation.Address;
	Address += FString::Printf(
		TEXT("?FlickNetworkMatch?FlickMatchmaking%s?FlickCoordinatorMatchId=%s?FlickAccountId=%s?FlickReservation=%s?FlickTeam=%d?FlickPlayerSlot=%d"),
		LastAllocation.bRanked ? TEXT("?FlickRanked") : TEXT(""),
		*FGenericPlatformHttp::UrlEncode(LastAllocation.MatchId),
		*FGenericPlatformHttp::UrlEncode(LocalReservation.AccountId),
		*FGenericPlatformHttp::UrlEncode(LocalReservation.Token),
		static_cast<int32>(LocalReservation.Team),
		LocalReservation.PlayerSlot);
	if (!LocalPartyId.IsEmpty())
	{
		Address += FString::Printf(
			TEXT("?FlickPartyId=%s?FlickPartySlot=%d?FlickPartySize=%d?FlickPartyLeader=%d"),
			*FGenericPlatformHttp::UrlEncode(LocalPartyId),
			LocalPartySlot,
			LocalPartySize,
			bLocalPartyLeader ? 1 : 0);
	}
	return Address;
}

bool UFlickMatchmakingCoordinatorSubsystem::TryReconnect(APlayerController* LocalController)
{
	if (!LocalController || !HasReconnectReservation() || ReconnectAttempts >= 3)
	{
		return false;
	}
	++ReconnectAttempts;
	StatusMessage = FString::Printf(TEXT("RECONNECTING TO MATCH - ATTEMPT %d/3"), ReconnectAttempts);
	OnCoordinatorChanged.Broadcast();
	LocalController->ClientTravel(BuildTravelAddress(), ETravelType::TRAVEL_Absolute);
	return true;
}

bool UFlickMatchmakingCoordinatorSubsystem::TravelToAllocatedMatch(APlayerController* LocalController)
{
	if (!LocalController || !HasReconnectReservation())
	{
		return false;
	}
	ReconnectAttempts = 0;
	LocalController->ClientTravel(BuildTravelAddress(), ETravelType::TRAVEL_Absolute);
	return true;
}

void UFlickMatchmakingCoordinatorSubsystem::ClearReconnectReservation()
{
	LastAllocation = FFlickCoordinatorAllocation();
	LocalReservation = FFlickCoordinatorReservation();
	QueueTicketId.Reset();
	ReconnectAttempts = 0;
	if (QueueState == EFlickCoordinatorQueueState::Allocated)
	{
		SetState(EFlickCoordinatorQueueState::Idle, TEXT("COORDINATOR READY"));
	}
}

void UFlickMatchmakingCoordinatorSubsystem::BeginServerHeartbeat(const FString& MatchId)
{
	if (!ShouldUseCoordinator() || MatchId.IsEmpty() || !IsRunningDedicatedServer())
	{
		return;
	}
	HeartbeatMatchId = MatchId;
	bCompletionReported = false;
	UE_LOG(
		LogFlick,
		Log,
		TEXT("COORDINATOR_SERVER_READY: match=%s server=%s"),
		*HeartbeatMatchId,
		*ServerId);
	SendHeartbeat();
}

void UFlickMatchmakingCoordinatorSubsystem::NotifyServerMatchComplete(
	const EFlickMatchOutcome Outcome,
	const bool bForfeit)
{
	if (HeartbeatMatchId.IsEmpty() || bCompletionReported)
	{
		return;
	}
	bCompletionReported = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HeartbeatTimer);
	}
	const FString CompletedMatchId = HeartbeatMatchId;
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("match_id"), CompletedMatchId);
	Json->SetStringField(TEXT("server_id"), ServerId);
	Json->SetNumberField(TEXT("outcome"), static_cast<int32>(Outcome));
	Json->SetBoolField(TEXT("forfeit"), bForfeit);
	Json->SetNumberField(TEXT("completed_unix_time"), FDateTime::UtcNow().ToUnixTimestamp());
	SendJsonRequest(
		TEXT("POST"),
		FString::Printf(
			TEXT("/v1/servers/matches/%s/complete"),
			*FGenericPlatformHttp::UrlEncode(CompletedMatchId)),
		Json,
		true,
		[CompletedMatchId](const bool bSuccess, const TSharedPtr<FJsonObject>&, const FString& Error)
		{
			if (bSuccess)
			{
				UE_LOG(LogFlick, Log, TEXT("COORDINATOR_MATCH_COMPLETE: match=%s accepted=1"), *CompletedMatchId);
			}
			else
			{
				UE_LOG(LogFlick, Warning, TEXT("COORDINATOR_MATCH_COMPLETE: match=%s accepted=0 error=%s"), *CompletedMatchId, *Error);
			}
		});
}

void UFlickMatchmakingCoordinatorSubsystem::SendHeartbeat()
{
	if (HeartbeatMatchId.IsEmpty())
	{
		return;
	}
	const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("match_id"), HeartbeatMatchId);
	Json->SetStringField(TEXT("server_id"), ServerId);
	Json->SetNumberField(TEXT("unix_time"), FDateTime::UtcNow().ToUnixTimestamp());
	SendJsonRequest(TEXT("POST"), TEXT("/v1/servers/heartbeat"), Json, true,
		[this](const bool bSuccess, const TSharedPtr<FJsonObject>&, const FString& Error)
		{
			if (!bSuccess)
			{
				UE_LOG(LogFlick, Warning, TEXT("COORDINATOR_HEARTBEAT_FAILED: %s"), *Error);
			}
			ScheduleHeartbeat();
		});
}

void UFlickMatchmakingCoordinatorSubsystem::ScheduleHeartbeat()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HeartbeatTimer,
			this,
			&UFlickMatchmakingCoordinatorSubsystem::SendHeartbeat,
			HeartbeatIntervalSeconds,
			false);
	}
}

void UFlickMatchmakingCoordinatorSubsystem::SetState(
	const EFlickCoordinatorQueueState NewState,
	const FString& Message)
{
	QueueState = NewState;
	StatusMessage = Message;
	UE_LOG(LogFlick, Log, TEXT("MATCH_COORDINATOR: %s"), *StatusMessage);
	OnCoordinatorChanged.Broadcast();
}

void UFlickMatchmakingCoordinatorSubsystem::SendJsonRequest(
	const FString& Verb,
	const FString& Path,
	const TSharedPtr<FJsonObject>& Body,
	const bool bServerRequest,
	FJsonResponseCallback Callback)
{
	FString SerializedBody;
	if (Body.IsValid())
	{
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SerializedBody);
		FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);
	}
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + Path);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	if (!SerializedBody.IsEmpty())
	{
		Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		Request->SetContentAsString(SerializedBody);
	}
	if (bServerRequest)
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ServerApiKey));
		Request->SetHeader(TEXT("X-Flick-Server-Id"), ServerId);
	}
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->OnProcessRequestComplete().BindWeakLambda(this,
		[Callback = MoveTemp(Callback)](FHttpRequestPtr, FHttpResponsePtr Response, const bool bConnectedSuccessfully) mutable
		{
			const int32 ResponseCode = Response.IsValid() ? Response->GetResponseCode() : 0;
			const bool bHttpSuccess = bConnectedSuccessfully && ResponseCode >= 200 && ResponseCode < 300;
			TSharedPtr<FJsonObject> Json;
			if (Response.IsValid() && !Response->GetContentAsString().IsEmpty())
			{
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
				FJsonSerializer::Deserialize(Reader, Json);
			}
			FString Error;
			if (!bHttpSuccess)
			{
				if (Json.IsValid())
				{
					Json->TryGetStringField(TEXT("error"), Error);
				}
				if (Error.IsEmpty())
				{
					Error = FString::Printf(TEXT("Coordinator request failed with HTTP %d."), ResponseCode);
				}
			}
			Callback(bHttpSuccess, Json, Error);
		});
	if (!Request->ProcessRequest())
	{
		Callback(false, nullptr, TEXT("The coordinator request could not be started."));
	}
}

bool UFlickMatchmakingCoordinatorSubsystem::ParseAllocation(
	const TSharedPtr<FJsonObject>& Json,
	FFlickCoordinatorAllocation& OutAllocation) const
{
	const TSharedPtr<FJsonObject>* AllocationJson = nullptr;
	if (!Json.IsValid() || !Json->TryGetObjectField(TEXT("allocation"), AllocationJson)
		|| !AllocationJson || !AllocationJson->IsValid())
	{
		return false;
	}
	(*AllocationJson)->TryGetStringField(TEXT("match_id"), OutAllocation.MatchId);
	(*AllocationJson)->TryGetStringField(TEXT("server_id"), OutAllocation.ServerId);
	(*AllocationJson)->TryGetStringField(TEXT("address"), OutAllocation.Address);
	OutAllocation.Variant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(FMath::Clamp(
		ReadJsonInt(*AllocationJson, TEXT("variant"), 0), 0, 2)));
	OutAllocation.PlayersPerTeam = FMath::Clamp(
		ReadJsonInt(*AllocationJson, TEXT("players_per_team"), 1), 1, 3);
	(*AllocationJson)->TryGetBoolField(TEXT("ranked"), OutAllocation.bRanked);
	OutAllocation.ExpiresUnixTime = ReadJsonInt(*AllocationJson, TEXT("expires_unix"), 0);
	const TArray<TSharedPtr<FJsonValue>>* Reservations = nullptr;
	if (!(*AllocationJson)->TryGetArrayField(TEXT("reservations"), Reservations) || !Reservations)
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Reservations)
	{
		const TSharedPtr<FJsonObject> ReservationJson = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!ReservationJson.IsValid())
		{
			return false;
		}
		FFlickCoordinatorReservation Reservation;
		ReservationJson->TryGetStringField(TEXT("account_id"), Reservation.AccountId);
		ReservationJson->TryGetStringField(TEXT("token"), Reservation.Token);
		Reservation.Team = static_cast<EFlickTeam>(FMath::Clamp(
			ReadJsonInt(ReservationJson, TEXT("team"), 0), 0, 2));
		Reservation.PlayerSlot = FMath::Clamp(
			ReadJsonInt(ReservationJson, TEXT("player_slot"), INDEX_NONE), INDEX_NONE, 2);
		if (Reservation.AccountId.IsEmpty() || Reservation.Token.IsEmpty()
			|| Reservation.Team == EFlickTeam::None || Reservation.PlayerSlot == INDEX_NONE)
		{
			return false;
		}
		OutAllocation.Reservations.Add(MoveTemp(Reservation));
	}
	return !OutAllocation.MatchId.IsEmpty() && !OutAllocation.Address.IsEmpty()
		&& OutAllocation.Reservations.Num() > 0;
}
