#pragma once

#include "CoreMinimal.h"
#include "Core/FlickRankRules.h"
#include "Core/FlickTypes.h"
#include "GameFramework/PlayerController.h"
#include "Utility/FlickLaunchMath.h"
#include "FlickPlayerController.generated.h"

class AFlickGameMode;
class AFlickGameState;
class AFlickPiece;

UCLASS()
class FLICK_API AFlickPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFlickPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	void ClearAiming();
	bool IsAimingShot() const { return bAimingShot; }
	float GetAimPowerPercent() const { return CurrentLaunchResult.NormalizedPower * 100.0f; }
	const AFlickPiece* GetSelectedPiece() const { return SelectedPiece; }
	const FFlickLaunchResult& GetAimResult() const { return CurrentLaunchResult; }
	bool HasAimCursorPoint() const { return bHasAimCursorPoint; }
	FVector GetAimCursorWorldPoint() const { return AimCursorWorldPoint; }
	bool HasPredictedContact() const { return bHasPredictedContact; }
	FVector GetPredictedContactWorldPoint() const { return PredictedContactWorldPoint; }
	const AFlickPiece* GetPredictedContactPiece() const { return PredictedContactPiece; }
	float GetAimGuideDistance() const { return AimGuideDistance; }
	bool IsScoreboardVisible() const { return bScoreboardVisible; }
	EFlickTeam GetLocalTeam() const;
	bool CanSelectPieceLocally(const AFlickPiece* Piece) const;
	void RequestRestartMatch();
	void RequestNextRound();
	void ToggleLobbyReady();
	void RequestStartNetworkMatch();
	void RequestSelectClass(EFlickLineupPreset Preset);
	void RequestConfirmClass();
	void RequestTogglePrivateMatchSlot(EFlickTeam Team, int32 PlayerSlot);
	void RequestPrivateMatchSpectate();
	void LeaveNetworkSession();
	void ReturnToFrontendFromServer(bool bClearPartyIdentity = false);
	void SetPersistentPartyIdentityFromServer(const FString& PartyId, int32 PartySlot, int32 PartySize, bool bLeader);
	void BeginPartyMatchMigrationFromServer(const FString& TargetSessionId, const FString& PartyId, int32 PartySlot, int32 PartySize, bool bLeader);
	void BeginPartyRestoreFromServer(bool bLeader);
	void RecordRecentPlayer(const FString& UserId, const FString& DisplayName);
	void BeginRankedMatchFromServer(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating);
	void RequestRankedAuthenticationFromServer(const FString& TicketType);
	void RequestCoordinatorAuthenticationFromServer(const FString& TicketType);
	void TravelToCoordinatorMatchFromServer(
		const FString& MatchId,
		const FString& ServerId,
		const FString& Address,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		bool bRanked,
		int64 ExpiresUnixTime,
		const FString& AccountId,
		const FString& ReservationToken,
		EFlickTeam Team,
		int32 PlayerSlot,
		const FString& PartyId,
		int32 PartySlot,
		int32 PartySize,
		bool bPartyLeader);
	void ApplyTrustedRankedProgressFromServer(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		const FFlickRankProgress& Progress);
	void ApplyTrustedRankedUpdateFromServer(const FFlickRatingUpdate& Update);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Debug")
	bool bDrawAimDebug = false;

private:
	void HandlePrimaryPressed();
	void HandlePrimaryReleased();
	void HandleGamepadPrimaryPressed();
	void HandleGamepadPrimaryReleased();
	void HandlePreviousPiecePressed();
	void HandleNextPiecePressed();
	void HandleCameraElevationUpPressed();
	void HandleCameraElevationDownPressed();
	void HandleScoreboardPressed();
	void HandleScoreboardReleased();
	void HandleSecondaryPressed();
	void HandleCancelPressed();
	void HandleRestartPressed();
	void HandleTrainingEditorTogglePressed();
	void HandleTrainingOwnPuckPressed();
	void HandleTrainingTargetPuckPressed();
	void HandleTrainingClearPressed();
	void HandleTrainingRemovePressed();
	void UpdateAimFromCursor();
	void UpdateAimFromGamepad();
	void UpdatePredictedContact();
	void UpdateHoveredPiece();
	void ClearHoveredPiece();
	void RefreshGamepadFocus();
	void CycleGamepadPiece(int32 Direction);
	void SetGamepadFocusedPiece(AFlickPiece* Piece);
	void ClearGamepadFocus();
	void UpdateCareerStatsTracking(const AFlickGameState* FlickGameState);
#if !UE_BUILD_SHIPPING
	void BeginGamepadSmokeTest();
	void CompleteGamepadSmokeTest();
#endif
	void DrawAimDebug() const;
	bool IsGameplayActive() const;
	float GetArenaSurfaceZ() const;
	float GetMaxDragDistance() const;
	float GetMinDragDistance() const;
	float GetPowerExponent() const;
	void SubmitLaunch(AFlickPiece* Piece, const FVector& Direction, float NormalizedPower);
	void UpdateLocalCameraOrbit(float DeltaSeconds);
	void AdjustLocalCameraElevation(int32 Direction);
	AFlickPiece* FindPieceUnderCursor() const;
	bool GetCursorPointOnArenaPlane(FVector& OutWorldPoint) const;
	AFlickGameMode* GetFlickGameMode() const;
	AFlickGameState* GetFlickGameState() const;

	UFUNCTION(Server, Reliable)
	void ServerTryLaunchPiece(int32 PieceId, FVector_NetQuantizeNormal Direction, float NormalizedPower);

	UFUNCTION(Server, Reliable)
	void ServerRequestRestartMatch();

	UFUNCTION(Server, Reliable)
	void ServerRequestNextRound();

	UFUNCTION(Server, Reliable)
	void ServerSetLobbyReady(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerRequestStartNetworkMatch();

	UFUNCTION(Server, Reliable)
	void ServerSelectNetworkClass(EFlickLineupPreset Preset);

	UFUNCTION(Server, Reliable)
	void ServerConfirmNetworkClass();

	UFUNCTION(Server, Reliable)
	void ServerTogglePrivateMatchSlot(EFlickTeam Team, int32 PlayerSlot);

	UFUNCTION(Server, Reliable)
	void ServerSetPrivateMatchSpectating();

	UFUNCTION(Server, Reliable)
	void ServerSubmitRankedAuthentication(const FString& SteamAuthTicket);

	UFUNCTION(Server, Reliable)
	void ServerSubmitCoordinatorAuthentication(const FString& SteamAuthTicket);

	UFUNCTION(Client, Reliable)
	void ClientReturnToFrontend(bool bClearPartyIdentity);

	UFUNCTION(Client, Reliable)
	void ClientSetPersistentPartyIdentity(const FString& PartyId, int32 PartySlot, int32 PartySize, bool bLeader);

	UFUNCTION(Client, Reliable)
	void ClientBeginPartyMatchMigration(const FString& TargetSessionId, const FString& PartyId, int32 PartySlot, int32 PartySize, bool bLeader);

	UFUNCTION(Client, Reliable)
	void ClientBeginPartyRestore(bool bLeader);

	UFUNCTION(Client, Reliable)
	void ClientRecordRecentPlayer(const FString& UserId, const FString& DisplayName);

	UFUNCTION(Client, Reliable)
	void ClientBeginRankedMatch(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OpponentRating);

	UFUNCTION(Client, Reliable)
	void ClientRequestRankedAuthentication(const FString& TicketType);

	UFUNCTION(Client, Reliable)
	void ClientRequestCoordinatorAuthentication(const FString& TicketType);

	UFUNCTION(Client, Reliable)
	void ClientTravelToCoordinatorMatch(
		const FString& MatchId,
		const FString& ServerId,
		const FString& Address,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		bool bRanked,
		int64 ExpiresUnixTime,
		const FString& AccountId,
		const FString& ReservationToken,
		EFlickTeam Team,
		int32 PlayerSlot,
		const FString& PartyId,
		int32 PartySlot,
		int32 PartySize,
		bool bPartyLeader);

	UFUNCTION(Client, Reliable)
	void ClientApplyTrustedRankedProgress(
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 Rating,
		int32 MatchesPlayed,
		int32 Wins,
		int32 Losses,
		int32 Draws);

	UFUNCTION(Client, Reliable)
	void ClientApplyTrustedRankedUpdate(
		const FString& MatchId,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		int32 OldRating,
		int32 NewRating,
		int32 MatchesPlayed,
		uint8 OldTier,
		uint8 NewTier,
		int32 OldDivision,
		int32 NewDivision,
		bool bForfeit);

	UPROPERTY()
	TObjectPtr<AFlickPiece> SelectedPiece;

	UPROPERTY()
	TObjectPtr<AFlickPiece> HoveredPiece;

	UPROPERTY()
	TObjectPtr<AFlickPiece> PredictedContactPiece;

	UPROPERTY()
	TObjectPtr<AFlickPiece> GamepadFocusedPiece;

	UPROPERTY()
	TObjectPtr<AFlickPiece> TrainingDraggedPiece;

	FFlickLaunchResult CurrentLaunchResult;
	FVector AimCursorWorldPoint = FVector::ZeroVector;
	FVector PredictedContactWorldPoint = FVector::ZeroVector;
	float AimGuideDistance = 0.0f;
	bool bAimingShot = false;
	bool bHasAimCursorPoint = false;
	bool bHasPredictedContact = false;
	bool bGamepadAiming = false;
	bool bUsingGamepad = false;
	bool bScoreboardVisible = false;
	bool bInitializedNetworkCamera = false;
	bool bNetworkAutoShotRequested = false;
	bool bNetworkAutoShotSubmitted = false;
	bool bNetworkAutoReadyRequested = false;
	bool bNetworkAutoReadySubmitted = false;
	bool bNetworkAutoStartSubmitted = false;
	bool bNetworkAutoClassSubmitted = false;
	bool bCareerStatsRecordedForCurrentSeries = false;
	float NetworkGameplayElapsed = 0.0f;
#if !UE_BUILD_SHIPPING
	bool bUseGamepadAimOverride = false;
	bool bGamepadSmokeAimWasValid = false;
	FVector2D GamepadAimOverride = FVector2D::ZeroVector;
#endif

	UPROPERTY(EditAnywhere, Category = "FLICK|Controller", meta = (ClampMin = "0.0", ClampMax = "0.9"))
	float GamepadAimDeadZone = 0.18f;
};
