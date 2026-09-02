#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Ranking/FlickRankedBackendTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlickRankedBackendSubsystem.generated.h"

class FJsonObject;
class IHttpRequest;
class IHttpResponse;

UCLASS()
class FLICK_API UFlickRankedBackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	EFlickRankedBackendMode GetMode() const { return Mode; }
	const FString& GetSeasonId() const { return SeasonId; }
	const FString& GetSteamTicketAudience() const { return SteamTicketAudience; }
	int32 GetMaximumTeamRatingSpread() const { return MaximumTeamRatingSpread; }
	bool IsRemoteAuthorityEnabled() const { return Mode == EFlickRankedBackendMode::RemoteHttp; }
	bool CanRunRankedAuthority(FString& OutError) const;
	FString GetStatusLabel() const;

	void AuthenticatePlayer(
		const FString& ClaimedAccountId,
		const FString& SteamAuthTicket,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		FFlickRankedAuthenticationCallback Callback);
	void RegisterMatch(
		const FFlickRankedMatchRequest& Request,
		FFlickRankedMatchRegistrationCallback Callback);
	void SubmitMatchResult(
		const FFlickRankedMatchResultRequest& Request,
		FFlickRankedSettlementCallback Callback);

private:
	using FJsonResponseCallback = TFunction<void(bool, const TSharedPtr<FJsonObject>&, const FString&)>;

	void SendJsonRequest(
		const FString& Verb,
		const FString& Path,
		const TSharedRef<FJsonObject>& Body,
		const FString& IdempotencyKey,
		int32 AttemptsRemaining,
		FJsonResponseCallback Callback);
	void HandleJsonResponse(
		FHttpRequestPtr Request,
		FHttpResponsePtr Response,
		bool bConnectedSuccessfully,
		FString Verb,
		FString Path,
		FString SerializedBody,
		FString IdempotencyKey,
		int32 AttemptsRemaining,
		FJsonResponseCallback Callback);
	void SubmitLocalMatchResult(
		const FFlickRankedMatchResultRequest& Request,
		FFlickRankedSettlementCallback Callback);
	TSharedRef<FJsonObject> MakeMatchJson(const FFlickRankedMatchRequest& Request) const;
	bool ParseProgress(const TSharedPtr<FJsonObject>& Json, FFlickRankProgress& OutProgress) const;
	bool ParseSettlement(
		const FFlickRankedMatchResultRequest& Request,
		const TSharedPtr<FJsonObject>& Json,
		FFlickRankedSettlementResult& OutResult) const;
	bool IsServerAuthority() const;

	EFlickRankedBackendMode Mode = EFlickRankedBackendMode::LocalDevelopment;
	FString BaseUrl;
	FString ServerApiKeyEnvironmentVariable = TEXT("FLICK_BACKEND_SERVER_KEY");
	FString ServerApiKey;
	FString ServerId;
	FString SeasonId = TEXT("PRESEASON");
	FString SteamTicketAudience = TEXT("FLICK");
	bool bRequireDedicatedServer = false;
	bool bRequireSteamAuthentication = false;
	int32 MaximumTeamRatingSpread = 350;
	float RequestTimeoutSeconds = 10.0f;
	TSet<FString> AuthenticatedAccounts;
	TMap<FString, FFlickRankedMatchRequest> ActiveMatches;
	TMap<FString, FFlickRankedSettlementResult> SettledMatches;
};
