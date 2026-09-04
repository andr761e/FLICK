#include "Ranking/FlickRankedBackendSubsystem.h"

#include "Core/FlickLog.h"
#include "Core/FlickRankedAuthorityRules.h"
#include "Dom/JsonObject.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Ranking/FlickRankingService.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* BackendSection = TEXT("FLICK.RankedBackend");
	constexpr int32 MaximumAuthTicketLength = 8192;

	FString SanitizeBackendToken(FString Value, const int32 MaximumLength)
	{
		Value.TrimStartAndEndInline();
		return Value.Left(MaximumLength);
	}

	int32 ReadRankedJsonInt(const TSharedPtr<FJsonObject>& Json, const TCHAR* Field, const int32 DefaultValue)
	{
		double Value = DefaultValue;
		return Json.IsValid() && Json->TryGetNumberField(Field, Value)
			? FMath::RoundToInt(Value)
			: DefaultValue;
	}
}

void UFlickRankedBackendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FString ModeValue = TEXT("LocalDevelopment");
	GConfig->GetString(BackendSection, TEXT("Mode"), ModeValue, GGameIni);
	Mode = ModeValue.Equals(TEXT("RemoteHttp"), ESearchCase::IgnoreCase)
		? EFlickRankedBackendMode::RemoteHttp
		: EFlickRankedBackendMode::LocalDevelopment;
	GConfig->GetString(BackendSection, TEXT("BaseUrl"), BaseUrl, GGameIni);
	GConfig->GetString(BackendSection, TEXT("ServerApiKeyEnvironmentVariable"), ServerApiKeyEnvironmentVariable, GGameIni);
	GConfig->GetString(BackendSection, TEXT("SeasonId"), SeasonId, GGameIni);
	GConfig->GetString(BackendSection, TEXT("SteamTicketAudience"), SteamTicketAudience, GGameIni);
	GConfig->GetBool(BackendSection, TEXT("bRequireDedicatedServer"), bRequireDedicatedServer, GGameIni);
	GConfig->GetBool(BackendSection, TEXT("bRequireSteamAuthentication"), bRequireSteamAuthentication, GGameIni);
	GConfig->GetInt(BackendSection, TEXT("MaximumTeamRatingSpread"), MaximumTeamRatingSpread, GGameIni);
	GConfig->GetFloat(BackendSection, TEXT("RequestTimeoutSeconds"), RequestTimeoutSeconds, GGameIni);
	FString CommandLineBaseUrl;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickRankedBackendUrl="), CommandLineBaseUrl))
	{
		BaseUrl = MoveTemp(CommandLineBaseUrl);
		Mode = EFlickRankedBackendMode::RemoteHttp;
	}

	BaseUrl = SanitizeBackendToken(MoveTemp(BaseUrl), 512);
	BaseUrl.RemoveFromEnd(TEXT("/"));
	SeasonId = SanitizeBackendToken(MoveTemp(SeasonId), 64);
	SteamTicketAudience = SanitizeBackendToken(MoveTemp(SteamTicketAudience), 128);
	ServerApiKeyEnvironmentVariable = SanitizeBackendToken(MoveTemp(ServerApiKeyEnvironmentVariable), 128);
	ServerApiKey = FPlatformMisc::GetEnvironmentVariable(*ServerApiKeyEnvironmentVariable);
	ServerId = FPlatformMisc::GetEnvironmentVariable(TEXT("FLICK_SERVER_ID"));
	if (ServerId.IsEmpty())
	{
		ServerId = FString::Printf(TEXT("local-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}
	MaximumTeamRatingSpread = FMath::Clamp(MaximumTeamRatingSpread, 0, FlickRankRules::MaximumRating);
	RequestTimeoutSeconds = FMath::Clamp(RequestTimeoutSeconds, 2.0f, 30.0f);

#if UE_BUILD_SHIPPING
	Mode = EFlickRankedBackendMode::RemoteHttp;
	bRequireDedicatedServer = true;
	bRequireSteamAuthentication = true;
#endif

	FString AuthorityError;
	if (CanRunRankedAuthority(AuthorityError))
	{
		UE_LOG(LogFlick, Log, TEXT("RANKED_BACKEND: %s"), *GetStatusLabel());
	}
	else
	{
		UE_LOG(LogFlick, Warning, TEXT("RANKED_BACKEND: %s (%s)"), *GetStatusLabel(), *AuthorityError);
	}
}

void UFlickRankedBackendSubsystem::Deinitialize()
{
	AuthenticatedAccounts.Reset();
	ActiveMatches.Reset();
	SettledMatches.Reset();
	Super::Deinitialize();
}

bool UFlickRankedBackendSubsystem::IsServerAuthority() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

bool UFlickRankedBackendSubsystem::CanRunRankedAuthority(FString& OutError) const
{
	OutError.Reset();
	if (!IsServerAuthority())
	{
		OutError = TEXT("ranked authority cannot run on a network client");
		return false;
	}
	if (bRequireDedicatedServer && !IsRunningDedicatedServer())
	{
		OutError = TEXT("this configuration requires a dedicated server");
		return false;
	}
	if (Mode == EFlickRankedBackendMode::RemoteHttp)
	{
		if (BaseUrl.IsEmpty())
		{
			OutError = TEXT("remote backend URL is not configured");
			return false;
		}
#if UE_BUILD_SHIPPING
		if (!BaseUrl.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase))
		{
			OutError = TEXT("shipping ranked backend URLs must use HTTPS");
			return false;
		}
#endif
		if (ServerApiKey.IsEmpty())
		{
			OutError = FString::Printf(TEXT("server API key environment variable %s is empty"), *ServerApiKeyEnvironmentVariable);
			return false;
		}
	}
	return true;
}

FString UFlickRankedBackendSubsystem::GetStatusLabel() const
{
	return Mode == EFlickRankedBackendMode::RemoteHttp
		? FString::Printf(TEXT("REMOTE / %s / %s"), *SeasonId, IsRunningDedicatedServer() ? TEXT("DEDICATED") : TEXT("LISTEN"))
		: FString::Printf(TEXT("LOCAL DEVELOPMENT / %s / SERVER-OWNED RATINGS"), *SeasonId);
}

void UFlickRankedBackendSubsystem::AuthenticatePlayer(
	const FString& ClaimedAccountId,
	const FString& SteamAuthTicket,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	FFlickRankedAuthenticationCallback Callback)
{
	FFlickRankedAuthenticationResult FailedResult;
	FString AuthorityError;
	if (!CanRunRankedAuthority(AuthorityError))
	{
		FailedResult.Error = MoveTemp(AuthorityError);
		Callback(FailedResult);
		return;
	}
	const FString AccountId = SanitizeBackendToken(ClaimedAccountId, 128);
	if (AccountId.IsEmpty())
	{
		FailedResult.Error = TEXT("The player has no online account identity.");
		Callback(FailedResult);
		return;
	}
	if (SteamAuthTicket.Len() > MaximumAuthTicketLength)
	{
		FailedResult.Error = TEXT("The Steam authentication ticket is too large.");
		Callback(FailedResult);
		return;
	}

	if (Mode == EFlickRankedBackendMode::LocalDevelopment)
	{
		FFlickRankedAuthenticationResult Result;
		Result.bAuthenticated = true;
		Result.AccountId = AccountId;
		FConfigFlickRankingService Service(AccountId, SeasonId);
		Result.Progress = Service.LoadProgress(Variant, PlayersPerTeam);
		AuthenticatedAccounts.Add(AccountId);
		Callback(Result);
		return;
	}
	if (bRequireSteamAuthentication && SteamAuthTicket.IsEmpty())
	{
		FailedResult.Error = TEXT("Steam did not provide a WebAPI authentication ticket.");
		Callback(FailedResult);
		return;
	}

	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("claimed_account_id"), AccountId);
	Body->SetStringField(TEXT("steam_ticket"), SteamAuthTicket);
	Body->SetStringField(TEXT("steam_ticket_audience"), SteamTicketAudience);
	Body->SetStringField(TEXT("season_id"), SeasonId);
	Body->SetStringField(TEXT("playlist"), FlickRankRules::MakePlaylistKey(Variant, PlayersPerTeam));
	SendJsonRequest(TEXT("POST"), TEXT("/v1/auth/steam"), Body, AccountId, 2,
		[this, AccountId, Callback = MoveTemp(Callback)](
			const bool bSuccess,
			const TSharedPtr<FJsonObject>& Json,
			const FString& Error) mutable
		{
			FFlickRankedAuthenticationResult Result;
			Result.Error = Error;
			bool bAuthenticated = false;
			if (bSuccess && Json.IsValid())
			{
				Json->TryGetBoolField(TEXT("authenticated"), bAuthenticated);
				Json->TryGetStringField(TEXT("account_id"), Result.AccountId);
				Result.bAuthenticated = bAuthenticated && Result.AccountId == AccountId;
				ParseProgress(Json, Result.Progress);
			}
			if (Result.bAuthenticated)
			{
				AuthenticatedAccounts.Add(Result.AccountId);
			}
			else if (Result.Error.IsEmpty())
			{
				Result.Error = TEXT("The ranked backend rejected the Steam identity.");
			}
			Callback(Result);
		});
}

void UFlickRankedBackendSubsystem::RegisterMatch(
	const FFlickRankedMatchRequest& Request,
	FFlickRankedMatchRegistrationCallback Callback)
{
	FString Error;
	if (!CanRunRankedAuthority(Error)
		|| !FlickRankedAuthorityRules::ValidateMatchRequest(Request, MaximumTeamRatingSpread, Error))
	{
		Callback(false, Error);
		return;
	}
	for (const FFlickRankedParticipant& Participant : Request.Participants)
	{
		if (!AuthenticatedAccounts.Contains(Participant.AccountId))
		{
			Callback(false, FString::Printf(TEXT("Player %s has not passed ranked authentication."), *Participant.AccountId));
			return;
		}
	}
	if (SettledMatches.Contains(Request.MatchId))
	{
		Callback(false, TEXT("The ranked match ID has already been settled."));
		return;
	}
	if (ActiveMatches.Contains(Request.MatchId))
	{
		Callback(true, FString());
		return;
	}
	if (Mode == EFlickRankedBackendMode::LocalDevelopment)
	{
		ActiveMatches.Add(Request.MatchId, Request);
		Callback(true, FString());
		return;
	}

	SendJsonRequest(TEXT("POST"), TEXT("/v1/ranked/matches"), MakeMatchJson(Request), Request.MatchId, 3,
		[this, Request, Callback = MoveTemp(Callback)](
			const bool bSuccess,
			const TSharedPtr<FJsonObject>& Json,
			const FString& RequestError) mutable
		{
			bool bAccepted = false;
			FString ReturnedMatchId;
			if (bSuccess && Json.IsValid())
			{
				Json->TryGetBoolField(TEXT("accepted"), bAccepted);
				Json->TryGetStringField(TEXT("match_id"), ReturnedMatchId);
			}
			bAccepted &= ReturnedMatchId == Request.MatchId;
			if (bAccepted)
			{
				ActiveMatches.Add(Request.MatchId, Request);
			}
			Callback(bAccepted, bAccepted ? FString() : RequestError.IsEmpty()
				? TEXT("The ranked backend rejected match registration.")
				: RequestError);
		});
}

void UFlickRankedBackendSubsystem::SubmitMatchResult(
	const FFlickRankedMatchResultRequest& Request,
	FFlickRankedSettlementCallback Callback)
{
	if (const FFlickRankedSettlementResult* Existing = SettledMatches.Find(Request.Match.MatchId))
	{
		FFlickRankedSettlementResult Duplicate = *Existing;
		Duplicate.bDuplicate = true;
		Callback(Duplicate);
		return;
	}
	FFlickRankedSettlementResult FailedResult;
	FString Error;
	if (!CanRunRankedAuthority(Error)
		|| Request.Outcome == EFlickMatchOutcome::Continue
		|| !FlickRankedAuthorityRules::ValidateMatchRequest(Request.Match, MaximumTeamRatingSpread, Error))
	{
		FailedResult.Error = Error.IsEmpty() ? TEXT("The ranked result is incomplete.") : Error;
		Callback(FailedResult);
		return;
	}
	if (!ActiveMatches.Contains(Request.Match.MatchId))
	{
		FailedResult.Error = TEXT("The ranked match was not registered by this server.");
		Callback(FailedResult);
		return;
	}
	if (Mode == EFlickRankedBackendMode::LocalDevelopment)
	{
		SubmitLocalMatchResult(Request, MoveTemp(Callback));
		return;
	}

	const TSharedRef<FJsonObject> Body = MakeMatchJson(Request.Match);
	Body->SetNumberField(TEXT("outcome"), static_cast<int32>(Request.Outcome));
	Body->SetBoolField(TEXT("forfeit"), Request.bForfeit);
	Body->SetNumberField(TEXT("completed_unix_time"), static_cast<double>(Request.CompletedUnixTime));
	SendJsonRequest(
		TEXT("POST"),
		FString::Printf(TEXT("/v1/ranked/matches/%s/result"), *FGenericPlatformHttp::UrlEncode(Request.Match.MatchId)),
		Body,
		Request.Match.MatchId + TEXT(":result"),
		4,
		[this, Request, Callback = MoveTemp(Callback)](
			const bool bSuccess,
			const TSharedPtr<FJsonObject>& Json,
			const FString& RequestError) mutable
		{
			FFlickRankedSettlementResult Result;
			if (!bSuccess || !ParseSettlement(Request, Json, Result))
			{
				Result.Error = RequestError.IsEmpty()
					? TEXT("The ranked backend returned an invalid settlement.")
					: RequestError;
				Callback(Result);
				return;
			}
			SettledMatches.Add(Request.Match.MatchId, Result);
			ActiveMatches.Remove(Request.Match.MatchId);
			Callback(Result);
		});
}

void UFlickRankedBackendSubsystem::SubmitLocalMatchResult(
	const FFlickRankedMatchResultRequest& Request,
	FFlickRankedSettlementCallback Callback)
{
	FFlickRankedSettlementResult Result;
	for (const FFlickRankedParticipant& Participant : Request.Match.Participants)
	{
		FConfigFlickRankingService Service(Participant.AccountId, Request.Match.SeasonId);
		FFlickRankedPlayerUpdate PlayerUpdate;
		PlayerUpdate.AccountId = Participant.AccountId;
		const int32 OpponentRating = FlickRankedAuthorityRules::GetAverageTeamRating(
			Request.Match.Participants,
			GetOpposingTeam(Participant.Team));
		if (!Service.SubmitMatchResult(
			Request.Match.MatchId,
			Request.Match.Variant,
			Request.Match.PlayersPerTeam,
			OpponentRating,
			FlickRankedAuthorityRules::GetPlayerResult(Participant.Team, Request.Outcome),
			Request.bForfeit,
			PlayerUpdate.RatingUpdate))
		{
			Result.Error = FString::Printf(TEXT("Could not settle local rank for %s."), *Participant.AccountId);
			Callback(Result);
			return;
		}
		Result.PlayerUpdates.Add(MoveTemp(PlayerUpdate));
	}
	Result.bAccepted = true;
	SettledMatches.Add(Request.Match.MatchId, Result);
	ActiveMatches.Remove(Request.Match.MatchId);
	Callback(Result);
}

TSharedRef<FJsonObject> UFlickRankedBackendSubsystem::MakeMatchJson(
	const FFlickRankedMatchRequest& Request) const
{
	const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
	Body->SetStringField(TEXT("match_id"), Request.MatchId);
	Body->SetStringField(TEXT("server_id"), ServerId);
	Body->SetStringField(TEXT("season_id"), Request.SeasonId);
	Body->SetStringField(TEXT("playlist"), FlickRankRules::MakePlaylistKey(Request.Variant, Request.PlayersPerTeam));
	Body->SetNumberField(TEXT("variant"), static_cast<int32>(Request.Variant));
	Body->SetNumberField(TEXT("players_per_team"), Request.PlayersPerTeam);
	Body->SetNumberField(TEXT("started_unix_time"), static_cast<double>(Request.StartedUnixTime));
	TArray<TSharedPtr<FJsonValue>> ParticipantValues;
	for (const FFlickRankedParticipant& Participant : Request.Participants)
	{
		const TSharedRef<FJsonObject> ParticipantJson = MakeShared<FJsonObject>();
		ParticipantJson->SetStringField(TEXT("account_id"), Participant.AccountId);
		ParticipantJson->SetNumberField(TEXT("team"), static_cast<int32>(Participant.Team));
		ParticipantJson->SetNumberField(TEXT("player_slot"), Participant.PlayerSlot);
		ParticipantJson->SetNumberField(TEXT("authenticated_rating"), Participant.Rating);
		ParticipantValues.Add(MakeShared<FJsonValueObject>(ParticipantJson));
	}
	Body->SetArrayField(TEXT("participants"), ParticipantValues);
	return Body;
}

bool UFlickRankedBackendSubsystem::ParseProgress(
	const TSharedPtr<FJsonObject>& Json,
	FFlickRankProgress& OutProgress) const
{
	if (!Json.IsValid())
	{
		return false;
	}
	const TSharedPtr<FJsonObject>* ProgressJson = nullptr;
	const TSharedPtr<FJsonObject> Source = Json->TryGetObjectField(TEXT("progress"), ProgressJson) && ProgressJson
		? *ProgressJson
		: Json;
	OutProgress.Rating = FMath::Clamp(ReadRankedJsonInt(Source, TEXT("rating"), FlickRankRules::DefaultRating), 0, 3000);
	OutProgress.MatchesPlayed = FMath::Max(0, ReadRankedJsonInt(Source, TEXT("matches_played"), 0));
	OutProgress.Wins = FMath::Max(0, ReadRankedJsonInt(Source, TEXT("wins"), 0));
	OutProgress.Losses = FMath::Max(0, ReadRankedJsonInt(Source, TEXT("losses"), 0));
	OutProgress.Draws = FMath::Max(0, ReadRankedJsonInt(Source, TEXT("draws"), 0));
	return true;
}

bool UFlickRankedBackendSubsystem::ParseSettlement(
	const FFlickRankedMatchResultRequest& Request,
	const TSharedPtr<FJsonObject>& Json,
	FFlickRankedSettlementResult& OutResult) const
{
	if (!Json.IsValid())
	{
		return false;
	}
	Json->TryGetBoolField(TEXT("accepted"), OutResult.bAccepted);
	Json->TryGetBoolField(TEXT("duplicate"), OutResult.bDuplicate);
	FString MatchId;
	Json->TryGetStringField(TEXT("match_id"), MatchId);
	const TArray<TSharedPtr<FJsonValue>>* Updates = nullptr;
	if (!OutResult.bAccepted || MatchId != Request.Match.MatchId
		|| !Json->TryGetArrayField(TEXT("updates"), Updates) || !Updates)
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Updates)
	{
		const TSharedPtr<FJsonObject> UpdateJson = Value.IsValid() ? Value->AsObject() : nullptr;
		if (!UpdateJson.IsValid())
		{
			return false;
		}
		FFlickRankedPlayerUpdate PlayerUpdate;
		UpdateJson->TryGetStringField(TEXT("account_id"), PlayerUpdate.AccountId);
		PlayerUpdate.RatingUpdate.MatchId = Request.Match.MatchId;
		PlayerUpdate.RatingUpdate.Variant = Request.Match.Variant;
		PlayerUpdate.RatingUpdate.PlayersPerTeam = Request.Match.PlayersPerTeam;
		PlayerUpdate.RatingUpdate.OldRating = ReadRankedJsonInt(UpdateJson, TEXT("old_rating"), 1000);
		PlayerUpdate.RatingUpdate.NewRating = ReadRankedJsonInt(UpdateJson, TEXT("new_rating"), 1000);
		PlayerUpdate.RatingUpdate.RatingDelta = PlayerUpdate.RatingUpdate.NewRating - PlayerUpdate.RatingUpdate.OldRating;
		PlayerUpdate.RatingUpdate.MatchesPlayed = ReadRankedJsonInt(UpdateJson, TEXT("matches_played"), 0);
		PlayerUpdate.RatingUpdate.OldTier = static_cast<EFlickRankTier>(FMath::Clamp(
			ReadRankedJsonInt(UpdateJson, TEXT("old_tier"), 0),
			static_cast<int32>(EFlickRankTier::Unranked),
			static_cast<int32>(EFlickRankTier::GrandChampion)));
		PlayerUpdate.RatingUpdate.NewTier = static_cast<EFlickRankTier>(FMath::Clamp(
			ReadRankedJsonInt(UpdateJson, TEXT("new_tier"), 0),
			static_cast<int32>(EFlickRankTier::Unranked),
			static_cast<int32>(EFlickRankTier::GrandChampion)));
		PlayerUpdate.RatingUpdate.OldDivision = FMath::Clamp(ReadRankedJsonInt(UpdateJson, TEXT("old_division"), 0), 0, 4);
		PlayerUpdate.RatingUpdate.NewDivision = FMath::Clamp(ReadRankedJsonInt(UpdateJson, TEXT("new_division"), 0), 0, 4);
		PlayerUpdate.RatingUpdate.bForfeit = Request.bForfeit;
		PlayerUpdate.RatingUpdate.bAccepted = true;
		if (PlayerUpdate.AccountId.IsEmpty())
		{
			return false;
		}
		OutResult.PlayerUpdates.Add(MoveTemp(PlayerUpdate));
	}
	return OutResult.PlayerUpdates.Num() == Request.Match.Participants.Num();
}

void UFlickRankedBackendSubsystem::SendJsonRequest(
	const FString& Verb,
	const FString& Path,
	const TSharedRef<FJsonObject>& Body,
	const FString& IdempotencyKey,
	const int32 AttemptsRemaining,
	FJsonResponseCallback Callback)
{
	FString SerializedBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SerializedBody);
	FJsonSerializer::Serialize(Body, Writer);
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BaseUrl + Path);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ServerApiKey));
	Request->SetHeader(TEXT("X-Flick-Server-Id"), ServerId);
	if (!IdempotencyKey.IsEmpty())
	{
		Request->SetHeader(TEXT("X-Idempotency-Key"), IdempotencyKey.Left(160));
	}
	Request->SetTimeout(RequestTimeoutSeconds);
	Request->SetContentAsString(SerializedBody);
	const TWeakObjectPtr<UFlickRankedBackendSubsystem> WeakThis(this);
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, Verb, Path, SerializedBody, IdempotencyKey, AttemptsRemaining, Callback](
			FHttpRequestPtr CompletedRequest,
			FHttpResponsePtr Response,
			const bool bConnectedSuccessfully)
		{
			if (UFlickRankedBackendSubsystem* Backend = WeakThis.Get())
			{
				Backend->HandleJsonResponse(
					CompletedRequest,
					Response,
					bConnectedSuccessfully,
					Verb,
					Path,
					SerializedBody,
					IdempotencyKey,
					AttemptsRemaining,
					Callback);
			}
		});
	if (!Request->ProcessRequest())
	{
		Callback(false, nullptr, TEXT("The ranked backend request could not be started."));
	}
}

void UFlickRankedBackendSubsystem::HandleJsonResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	const bool bConnectedSuccessfully,
	FString Verb,
	FString Path,
	FString SerializedBody,
	FString IdempotencyKey,
	const int32 AttemptsRemaining,
	FJsonResponseCallback Callback)
{
	const int32 ResponseCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	const bool bHttpSuccess = bConnectedSuccessfully && ResponseCode >= 200 && ResponseCode < 300;
	if (!bHttpSuccess && AttemptsRemaining > 1 && (ResponseCode == 0 || ResponseCode == 408 || ResponseCode == 429 || ResponseCode >= 500))
	{
		const float RetryDelay = static_cast<float>(5 - FMath::Min(4, AttemptsRemaining));
		if (UWorld* World = GetWorld())
		{
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(RetryTimer, [this, Verb, Path, SerializedBody, IdempotencyKey, AttemptsRemaining, Callback = MoveTemp(Callback)]() mutable
			{
				TSharedPtr<FJsonObject> Body;
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SerializedBody);
				if (FJsonSerializer::Deserialize(Reader, Body) && Body.IsValid())
				{
					SendJsonRequest(Verb, Path, Body.ToSharedRef(), IdempotencyKey, AttemptsRemaining - 1, MoveTemp(Callback));
				}
				else
				{
					Callback(false, nullptr, TEXT("Could not restore the ranked backend retry payload."));
				}
			}, RetryDelay, false);
			return;
		}
	}

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
			Error = FString::Printf(TEXT("Ranked backend request failed with HTTP %d."), ResponseCode);
		}
	}
	Callback(bHttpSuccess, Json, Error);
}
