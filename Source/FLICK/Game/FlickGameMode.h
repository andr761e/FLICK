#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/GameModeBase.h"
#include "Ranking/FlickRankedBackendTypes.h"
#include "FlickGameMode.generated.h"

class AFlickArena;
class AFlickAudioDirector;
class AFlickBobArena;
class AFlickCameraPawn;
class AFlickGameState;
class AFlickPiece;
class AFlickPlayerState;
class AFlickWorldFeedback;
class ADirectionalLight;
class APointLight;
class ASkyLight;
class UFlickGameInstance;
class UFlickMatchmakingCoordinatorSubsystem;
class UFlickRankedBackendSubsystem;
class UFlickRankingSubsystem;
class UFlickSessionSubsystem;
enum class EFlickFeedbackKind : uint8;
struct FFlickCoordinatorAllocation;

struct FFlickTrainingPieceSnapshot
{
	EFlickTeam Team = EFlickTeam::None;
	EFlickPieceArchetype Archetype = EFlickPieceArchetype::Standard;
	FVector Location = FVector::ZeroVector;
	int32 PieceId = 0;
	int32 OwningPlayerSlot = 0;
	bool bBobStriker = false;
	bool bShowPlayerIdentity = false;
};

UCLASS()
class FLICK_API AFlickGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFlickGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual FString InitNewPlayer(
		APlayerController* NewPlayerController,
		const FUniqueNetIdRepl& UniqueId,
		const FString& Options,
		const FString& Portal = TEXT("")) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	bool CanSelectPiece(const AFlickPiece* Piece) const;
	bool CanSelectPieceForController(const APlayerController* RequestingPlayer, const AFlickPiece* Piece) const;
	bool TryLaunchPiece(AFlickPiece* Piece, const FVector& Direction, float NormalizedPower);
	bool TryLaunchPieceForController(
		APlayerController* RequestingPlayer,
		AFlickPiece* Piece,
		const FVector& Direction,
		float NormalizedPower);
	bool TryLaunchPieceByIdForController(
		APlayerController* RequestingPlayer,
		int32 PieceId,
		const FVector& Direction,
		float NormalizedPower);
	void NotifyPieceSelected(const AFlickPiece* Piece) const;
	void NotifyPieceImpact(
		AFlickPiece* Piece,
		AFlickPiece* OtherPiece,
		const FVector& ImpactLocation,
		float ImpactVelocityChange);

	EFlickFrontendScreen GetFrontendScreen() const { return FrontendScreen; }
	EFlickMatchVariant GetSelectedMatchVariant() const { return SelectedMatchVariant; }
	EFlickMatchVariant GetActiveMatchVariant() const { return ActiveMatchVariant; }
	bool IsWaitingForNetworkPlayer() const { return bNetworkMatchRequested && !bNetworkMatchStarted; }
	bool IsNetworkLobby() const { return bNetworkMatchRequested && !bNetworkMatchStarted; }
	bool IsNetworkSession() const { return bNetworkMatchRequested; }
	bool IsPartySession() const { return bPartyRequested; }
	bool IsMatchmakingSession() const { return bMatchmakingRequested; }
	bool IsRankedMatch() const { return bRankedRequested; }
	bool IsTrainingMode() const { return bTrainingMode; }
	bool IsTrainingBotMatch() const { return bTrainingMode && bTrainingBotMatch; }
	bool IsFreePlayTraining() const { return bTrainingMode && !bTrainingBotMatch; }
	bool IsTrainingEditMode() const { return bTrainingMode && bTrainingEditMode; }
	float GetShotTimeRemaining() const;
	float GetShotClockFraction(EFlickTeam Team) const;
	float GetInitialClassSelectionTimeRemaining() const { return FMath::Max(0.0f, InitialClassSelectionTimeRemaining); }
	float GetRoundAdvanceTimeRemaining() const;
	EFlickTeam GetTrainingPlacementTeam() const { return TrainingPlacementTeam; }
	EFlickPieceArchetype GetTrainingPlacementArchetype() const { return TrainingPlacementArchetype; }
	bool IsRankedQueueSelected() const { return bRankedQueueSelected; }
	bool IsPrivateMatchSetup() const { return bPrivateMatchSetupActive; }
	bool IsPrivateMatch() const { return bPrivateMatchActive; }
	const FFlickPrivateMatchSettings& GetPrivateMatchSettings() const { return PrivateMatchSettings; }
	int32 GetPlayersPerTeam() const { return CurrentPlayersPerTeam; }
	int32 GetMatchmakingPlayersPerTeam() const { return MatchmakingPlayersPerTeam; }
	int32 GetPartyMemberCount() const;
	AFlickPlayerState* GetPartyMember(int32 PartySlot) const;
	bool CanOpenPartyTeamLobby(int32 PlayersPerTeam) const;
	void OpenPartyTeamLobby(int32 PlayersPerTeam);
	bool CanQueuePartyForMatchmaking(int32 PlayersPerTeam) const;
	void QueuePartyForMatchmaking(int32 PlayersPerTeam);
	void PreparePartyMigrationToMatch(const FString& TargetSessionId);
	bool CanStartNetworkMatch() const;
	bool AreRankedPlayersAuthenticated() const;
	AFlickPlayerState* GetLobbyPlayer(EFlickTeam Team) const;
	AFlickPlayerState* GetLobbyPlayer(EFlickTeam Team, int32 TeamPlayerSlot) const;
	int32 GetLobbyPlayerCount(EFlickTeam Team) const;
	AFlickPlayerState* GetPrivateSlotOwner(EFlickTeam Team, int32 PlayerSlot) const;
	bool CanStartPrivateMatch() const;
	float GetMenuPreviewAlpha() const
	{
		return MenuPreviewDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp(MenuPreviewElapsed / MenuPreviewDuration, 0.0f, 1.0f)
			: 0.0f;
	}
	float GetMenuPreviewTransitionOpacity() const
	{
		if (!bMenuPreviewTransitionActive || MenuPreviewTransitionDuration <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}
		const float Alpha = FMath::Clamp(
			MenuPreviewTransitionElapsed / MenuPreviewTransitionDuration,
			0.0f,
			1.0f);
		return FMath::Sin(Alpha * PI) * 0.78f;
	}
	EFlickPieceArchetype GetLoadoutPiece(EFlickTeam Team, int32 SlotIndex) const;
	EFlickLineupPreset GetLoadoutPreset(EFlickTeam Team) const;
	EFlickLineupPreset GetPlayerClass(EFlickTeam Team, int32 PlayerSlot, bool bUseDraft = false) const;
	int32 GetClassSelectionPlayersPerTeam() const;
	bool IsChangingClassForNextRound() const { return bClassSelectionForNextRound; }
	bool IsPreparingTrainingBotMatch() const { return bClassSelectionStartsTrainingBotMatch; }
	bool HasPendingClassChanges() const { return bHasPendingClassChanges; }
	bool CanOpenClassChange() const;
	int32 GetCurrentStartingPiecesPerTeam() const { return CurrentStartingPiecesPerTeam; }
	EFlickMatchVariant GetLoadoutEditingVariant() const { return LoadoutEditingVariant; }
	int32 GetLoadoutEditingPieceCount() const;
	int32 GetCurrentRoundsToWin() const { return CurrentRoundsToWin; }
	bool IsBobMode() const { return ActiveMatchVariant == EFlickMatchVariant::Bob; }
	bool DoesSelectedModeSupportLoadouts() const;
	bool CanChangeCameraView() const;
	bool IsAimGuideEnabled() const;
	bool AreImpactEffectsEnabled() const;
	float GetCameraShakeIntensity() const;
	float GetMasterVolume() const;
	float GetEffectsVolume() const;
	float GetInterfaceVolume() const;
	bool IsVSyncEnabled() const;
	FString GetWindowModeLabel() const;
	FString GetResolutionLabel() const;

	void SelectMatchVariant(EFlickMatchVariant Variant);
	void OpenModeSelect();
	void CloseModeSelect();
	void OpenOnlineBrowser();
	void CloseOnlineBrowser();
	void OpenLoadout();
	void CloseLoadout();
	void SelectLoadoutEditingVariant(EFlickMatchVariant Variant);
	void OpenItemShop();
	void CloseItemShop();
	void CycleLoadoutPiece(EFlickTeam Team, int32 SlotIndex, int32 Direction);
	void SetLoadoutPiece(EFlickTeam Team, int32 SlotIndex, EFlickPieceArchetype Archetype);
	void ApplyLoadoutPreset(EFlickTeam Team, EFlickLineupPreset Preset);
	void StartSelectedMatch();
	void OpenPrivateMatchSetup();
	void ClosePrivateMatchSetup();
	void CyclePrivateMatchSetting(EFlickPrivateMatchSetting Setting, int32 Direction);
	void TogglePrivateMatchSlot(APlayerController* RequestingPlayer, EFlickTeam Team, int32 PlayerSlot);
	void SetPrivateMatchSpectating(APlayerController* RequestingPlayer);
	void StartPrivateMatch(APlayerController* RequestingPlayer);
	void SelectPlayerClass(EFlickTeam Team, int32 PlayerSlot, EFlickLineupPreset Preset);
	void ConfirmClassSelection();
	void SetNetworkPlayerClass(APlayerController* RequestingPlayer, EFlickLineupPreset Preset);
	void ConfirmNetworkPlayerClass(APlayerController* RequestingPlayer);
	void CancelClassSelection();
	void OpenClassChange();
	void StartTrainingMode();
	void StartTrainingBotMatch();
	void ToggleTrainingEditMode();
	void SetTrainingPlacementTeam(EFlickTeam Team);
	void CycleTrainingPlacementArchetype(int32 Direction);
	bool PlaceTrainingPuck(const FVector& WorldLocation);
	bool BeginTrainingPuckMove(AFlickPiece* Piece);
	bool MoveTrainingPuck(AFlickPiece* Piece, const FVector& WorldLocation);
	void FinishTrainingPuckMove(AFlickPiece* Piece);
	bool RemoveTrainingPuck(AFlickPiece* Piece);
	void ClearTrainingPucks();
	void HostOnlineNetworkMatch();
	void CycleMatchmakingPlayersPerTeam(int32 Direction);
	void SetMatchmakingPlayersPerTeam(int32 PlayersPerTeam);
	void ToggleRankedQueue();
	void SetRankedQueueSelected(bool bRanked);
	void StartSelectedMatchmaking();
	void StartUnrankedMatchmaking();
	void CancelUnrankedMatchmaking();
	void RemovePartyMember(int32 PartySlot);
	void DisbandParty();
	void HostLocalNetworkMatch();
	void JoinLocalNetworkMatch(const FString& Address = TEXT("127.0.0.1"));
	void SetLobbyReady(APlayerController* RequestingPlayer, bool bReady);
	void StartNetworkMatch(APlayerController* RequestingPlayer);
	void SubmitRankedAuthentication(APlayerController* RequestingPlayer, const FString& SteamAuthTicket);
	void SubmitCoordinatorAuthentication(APlayerController* RequestingPlayer, const FString& SteamAuthTicket);
	void CancelNetworkLobby();
	void ReturnToNetworkLobby();
	void StartNextRound();
	void ReturnToLoadout();
	void TogglePauseMenu();
	void OpenSettings();
	void CloseSettings();
	void ReturnToMainMenu();
	void QuitGame();
	void SetAimGuideEnabled(bool bEnabled);
	void SetImpactEffectsEnabled(bool bEnabled);
	void SetCameraShakeIntensity(float Intensity);
	void SetMasterVolume(float Volume);
	void SetEffectsVolume(float Volume);
	void SetInterfaceVolume(float Volume);
	void PlayMenuSound(bool bConfirm) const;
	UFlickSessionSubsystem* GetFlickSessionSubsystem() const;
	UFlickMatchmakingCoordinatorSubsystem* GetFlickMatchmakingCoordinatorSubsystem() const;
	UFlickRankingSubsystem* GetFlickRankingSubsystem() const;
	void ToggleVSync();
	void CycleWindowMode(int32 Direction);
	void CycleResolution(int32 Direction);
	void ApplyDisplaySettings();

	UFUNCTION(BlueprintCallable, Category = "FLICK|Match")
	void RestartMatch();

	AFlickGameState* GetFlickGameState() const;
	float GetArenaSurfaceZ() const { return ArenaSurfaceZ; }
	float GetMaxDragDistance() const { return MaxDragDistance; }
	float GetMaxLaunchSpeed() const { return MaxLaunchSpeed; }
	float GetMinDragDistance() const { return MinDragDistance; }
	float GetPowerExponent() const { return PowerExponent; }
	float GetResolutionElapsed() const { return ResolutionElapsed; }
	float GetMaximumResolutionDuration() const { return MaximumResolutionDuration; }
	bool IsResolvingKickoff() const { return bResolvingKickoff; }
	bool HasLockedKickoffShot() const { return LockedKickoffPiece != nullptr; }
	const AFlickPiece* GetLockedKickoffPiece() const { return LockedKickoffPiece; }
	FVector GetLockedKickoffDirection() const { return LockedKickoffDirection; }
	float GetLockedKickoffPower() const { return LockedKickoffPower; }
	int32 CountActivePieces(EFlickTeam Team) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaThickness = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaSurfaceZ = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float KillZ = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena", meta = (ClampMin = "0.2", ClampMax = "0.7"))
	float FormationRadiusFraction = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceRadius = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceThickness = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceMassKg = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceFriction = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceRestitution = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float LinearDamping = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float AngularDamping = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Launch")
	float MaxLaunchSpeed = 1450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Launch")
	float MaxDragDistance = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Launch")
	float MinDragDistance = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Launch")
	float PowerExponent = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	bool bUseSimultaneousKickoff = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Resolution")
	float SleepLinearVelocityThreshold = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Resolution")
	float SleepAngularVelocityThreshold = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Resolution")
	float SettledDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Resolution")
	float MaximumResolutionDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Feedback")
	float MinimumImpactFeedback = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Feedback")
	float StrongImpactFeedback = 820.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics")
	bool bAllowEdgeTipping = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics")
	bool bUseContinuousCollisionDetection = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics", meta = (ClampMin = "1", ClampMax = "255"))
	int32 PositionSolverIterations = 12;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics", meta = (ClampMin = "1", ClampMax = "255"))
	int32 VelocitySolverIterations = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|BOB", meta = (ClampMin = "0.0", ClampMax = "80.0"))
	float BobSelfRightingTorque = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|BOB", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float BobSelfRightingDamping = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|BOB", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float BobSelfRightingMinimumTilt = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|BOB", meta = (ClampMin = "0.0"))
	float BobSelfRightingMaxLinearSpeed = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|BOB", meta = (ClampMin = "0.0"))
	float BobSelfRightingMaxAngularSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|Tabletop", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float TabletopMaximumUpwardSpeed = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|Tabletop", meta = (ClampMin = "0.0", ClampMax = "2000.0"))
	float TabletopDownwardAcceleration = 620.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|Tabletop", meta = (ClampMin = "0.0", ClampMax = "80.0"))
	float TabletopSelfRightingTorque = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|Tabletop", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float TabletopSelfRightingDamping = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics|Tabletop", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float TabletopSelfRightingMinimumTilt = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training", meta = (ClampMin = "2", ClampMax = "96"))
	int32 MaxTrainingPieces = 48;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training", meta = (ClampMin = "0.0", ClampMax = "40.0"))
	float TrainingPlacementGap = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training", meta = (ClampMin = "0.0", ClampMax = "80.0"))
	float TrainingArenaInset = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training", meta = (ClampMin = "-30.0", ClampMax = "30.0"))
	float TrainingEditCameraElevation = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float TrainingBotThinkDelay = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers", meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float ShotTimeLimit = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers", meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float InitialClassSelectionTimeLimit = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers", meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float RoundAdvanceTimeLimit = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float TrainingBotMinimumPower = 0.52f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float TrainingBotMaximumPower = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "12.0"))
	float TrainingBotAimErrorDegrees = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float TrainingBotPowerVariation = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Debug")
	bool bLogPhysicsResolution = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation", meta = (ClampMin = "2.0"))
	float MenuPreviewDuration = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation", meta = (ClampMin = "0.35", ClampMax = "2.0"))
	float MenuPreviewTransitionDuration = 0.9f;

private:
	void SpawnCameraIfNeeded();
	void SpawnAudioIfNeeded();
	void SpawnLightingIfNeeded();
	void SpawnArenaIfNeeded();
	void SpawnPieces();
	void SpawnBobPieces();
	void BeginSelectedMatch();
	void BeginTrainingActivity(bool bAgainstBot);
	void UpdateTrainingBot(float DeltaSeconds);
	void UpdateShotClock();
	void UpdateInitialClassSelectionTimer(float DeltaSeconds);
	void UpdateNetworkClassSelectionTimer();
	void UpdateRoundAdvanceTimer();
	bool TryExecuteTrainingBotShot();
	void ResetTrainingBotThinking();
	void ResetShotClock();
	void ExpireCurrentShot();
	void PrepareClassSelection(bool bForNextRound);
	void BeginNetworkClassSelection();
	void FinalizeNetworkClassSelection();
	bool AreNetworkClassesConfirmed() const;
	void EnsureActivePlayerClasses(int32 PlayersPerTeam);
	void RandomizeOtherLocalPlayerClasses(int32 PlayersPerTeam);
	EFlickLineupPreset PickRandomPlayerClass() const;
	void ApplyPendingPlayerClasses();
	TArray<AFlickPlayerState*> GetPrivateMatchParticipants() const;
	void AutoAssignPrivateMatchSlots();
	void RefreshPrivatePrimaryAssignments();
	void ResetPrivateMatchReadiness();
	void PushPrivateMatchState();
	EFlickPieceArchetype GetPlayerClassPiece(EFlickTeam Team, int32 PlayerSlot, int32 PieceSlot) const;
	void DestroyPieces();
	void StartMatch();
	void TryStartNetworkMatch();
	void ResetLobbyReadiness();
	void RestorePremadePartyAfterMatch();
	void FinalizeDisconnectedPlayerForfeit(const AFlickPlayerState* ExitingState);
	void FinalizeDisconnectedTeamForfeit(EFlickTeam ExitingTeam);
	void BeginRankedMatchForPlayers();
	void DispatchRankedMatchResults();
	void RegisterRankedMatchThenStart();
	bool BeginCoordinatorQueue(int32 PlayersPerTeam);
	void TrySubmitCoordinatorQueue();
	void HandleCoordinatorAllocation(const FFlickCoordinatorAllocation& Allocation);
	void VerifyCoordinatorReservation(
		APlayerController* Player,
		const FString& AccountId,
		const FString& ReservationToken);
	bool AreCoordinatorReservationsVerified() const;
	void CompleteNetworkMatchStart(const FString& MatchId);
	FFlickRankedMatchRequest BuildRankedMatchRequest(const FString& MatchId) const;
	int32 GetAverageRankedRating(EFlickTeam Team) const;
	void UpdateReplicatedMatchmakingState(bool bTimedOut = false) const;
	int32 GetPartyMemberCountForId(const FString& PartyId) const;
	bool FindPremadeTeamAndSlot(
		const FString& PartyId,
		int32 PartySlot,
		int32 PartySize,
		EFlickTeam& OutTeam,
		int32& OutPlayerSlot);
	EFlickTeam FindAvailableTeam(const APlayerController* PlayerToIgnore = nullptr) const;
	bool FindAvailableTeamAndSlot(
		const APlayerController* PlayerToIgnore,
		EFlickTeam& OutTeam,
		int32& OutPlayerSlot) const;
	void AdvanceCompletedPlayerTurn(EFlickTeam Team, int32 CompletedPlayerSlot);
	void ActivateNextPlayerForTeam(EFlickTeam Team);
	int32 FindEligiblePlayerSlot(EFlickTeam Team, int32 StartingSlot) const;
	bool HasActivePieceForPlayer(EFlickTeam Team, int32 TeamPlayerSlot) const;
	int32& GetNextPlayerSlot(EFlickTeam Team);
	int32 GetNextPlayerSlot(EFlickTeam Team) const;
	void RebuildMatch();
	void BeginOpeningPhase();
	bool ExecuteValidatedLaunch(AFlickPiece* Piece, const FVector& Direction, float NormalizedPower);
	bool LockKickoffShot(AFlickPiece* Piece, const FVector& Direction, float NormalizedPower, bool bAnnounce = true);
	void ReleaseKickoffPair(AFlickPiece* SecondPiece, const FVector& SecondDirection, float SecondPower);
	void ResetKickoffState();
	void ApplySelectedMatchConfiguration();
	void ApplyMatchConfiguration(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	void UpdateMainMenuPreview(float DeltaSeconds);
	void ShowModePreview(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	void CommitModePreview(EFlickMatchVariant Variant, int32 PlayersPerTeam);
	void SetCameraForFrontend();
	void SetCameraViewForTeam(EFlickTeam Team, bool bSnap = false);
	void ClearControllerAiming() const;
	void AddCameraFeedback(float Strength) const;
	void AddControllerFeedback(float Strength, float Duration) const;
	void UpdateEliminations();
	void UpdateBobPockets();
	void UpdateBobPieceStability();
	void UpdateClassicPieceStability();
	bool IsPieceSafeOnClassicTabletop(const AFlickPiece* Piece) const;
	void SettleClassicPiecesOnTabletop();
	void UpdateGameStateCounts() const;
	void AwardRoundSurvivalPoints() const;
	bool AreActivePiecesSettled() const;
	bool ApplyResolutionTimeoutCleanup();
	void FinishPhysicsResolution(bool bUsedTimeout);
	void CheckWinOrAdvanceTurn();
	void ResolveTrainingTurn();
	void ResetTrainingBoard();
	void CaptureTrainingResetSnapshot();
	void RestoreTrainingResetSnapshot();
	bool CanEditTrainingBoard() const;
	bool ResolveTrainingPlacement(
		const FVector& RequestedWorldLocation,
		float Radius,
		const AFlickPiece* IgnoredPiece,
		FVector& OutWorldLocation) const;
	int32 AllocateTrainingPieceId() const;
	void ResolveBobTurn();
	void CompleteRoundForOutcome(EFlickMatchOutcome Outcome);
	AFlickPiece* GetBobStriker(EFlickTeam Team) const;
	void ResetBobStrikerForTeam(EFlickTeam Team);
	bool RestoreBobPenaltyPiece(EFlickTeam Team);
	FVector FindBobRespawnLocation() const;
	void ResetBobPieceAt(AFlickPiece* Piece, EFlickTeam Team, const FVector& Location, bool bIsStriker) const;
	void SpawnWorldFeedback(
		const FVector& Location,
		const FLinearColor& Color,
		EFlickFeedbackKind FeedbackKind,
		float Strength,
		const FVector& BiasDirection = FVector::ZeroVector) const;
	void PushHudEvent(const FString& Message, const FLinearColor& Color, float Duration = 1.8f) const;
	AFlickPiece* SpawnPiece(
		EFlickTeam Team,
		int32 PieceId,
		const FVector& Location,
		EFlickPieceArchetype Archetype,
		bool bIsBobStriker = false,
		int32 OwningPlayerSlot = 0,
		bool bShowPlayerIdentity = false);
	UFlickGameInstance* GetFlickGameInstance() const;
	UFlickRankedBackendSubsystem* GetFlickRankedBackendSubsystem() const;

	UPROPERTY()
	TObjectPtr<AFlickArena> ArenaActor;

	UPROPERTY()
	TObjectPtr<AFlickBobArena> BobArenaActor;

	UPROPERTY()
	TObjectPtr<AFlickAudioDirector> AudioDirector;

	UPROPERTY()
	TObjectPtr<AFlickCameraPawn> CameraPawn;

	UPROPERTY()
	TObjectPtr<ADirectionalLight> DirectionalLightActor;

	UPROPERTY()
	TObjectPtr<ASkyLight> SkyLightActor;

	UPROPERTY()
	TObjectPtr<APointLight> Player1AccentLight;

	UPROPERTY()
	TObjectPtr<APointLight> Player2AccentLight;

	UPROPERTY()
	TObjectPtr<APointLight> ArenaFillLight;

	UPROPERTY()
	TArray<TObjectPtr<AFlickPiece>> Pieces;

	UPROPERTY()
	TObjectPtr<AFlickPiece> Player1BobStriker;

	UPROPERTY()
	TObjectPtr<AFlickPiece> Player2BobStriker;

	UPROPERTY()
	TObjectPtr<AFlickPiece> LockedKickoffPiece;

	FVector LockedKickoffDirection = FVector::ZeroVector;
	float LockedKickoffPower = 0.0f;
	EFlickTeam LockedKickoffTeam = EFlickTeam::None;
	bool bResolvingKickoff = false;

	EFlickFrontendScreen FrontendScreen = EFlickFrontendScreen::MainMenu;
	EFlickFrontendScreen SettingsReturnScreen = EFlickFrontendScreen::MainMenu;
	EFlickFrontendScreen LoadoutReturnScreen = EFlickFrontendScreen::MainMenu;
	EFlickFrontendScreen ClassSelectionReturnScreen = EFlickFrontendScreen::ModeSelect;
	EFlickMatchVariant SelectedMatchVariant = EFlickMatchVariant::Classic;
	EFlickMatchVariant ActiveMatchVariant = EFlickMatchVariant::Classic;
	EFlickMatchVariant LoadoutEditingVariant = EFlickMatchVariant::Classic;
	EFlickMatchVariant MenuPreviewVariant = EFlickMatchVariant::Classic;
	EFlickMatchVariant PendingMenuPreviewVariant = EFlickMatchVariant::Classic;
	int32 MenuPreviewPlayersPerTeam = 1;
	int32 PendingMenuPreviewPlayersPerTeam = 1;
	int32 CurrentStartingPiecesPerTeam = 4;
	int32 CurrentRoundsToWin = 3;
	int32 CurrentPlayersPerTeam = 1;
	int32 MatchmakingPlayersPerTeam = 1;
	int32 Player1NextPlayerSlot = 0;
	int32 Player2NextPlayerSlot = 0;
	bool bCurrentModeSupportsLoadouts = true;
	bool bPlayer1BobStrikerPocketed = false;
	bool bPlayer2BobStrikerPocketed = false;
	bool bTrainingMode = false;
	bool bTrainingEditMode = false;
	bool bTrainingBotMatch = false;
	bool bClassSelectionStartsTrainingBotMatch = false;
	bool bPrivateMatchSetupActive = false;
	bool bPrivateMatchActive = false;
	FFlickPrivateMatchSettings PrivateMatchSettings;
	EFlickMatchVariant PrivateMatchReturnVariant = EFlickMatchVariant::Classic;
	int32 PrivateMatchReturnPlayersPerTeam = 1;
	EFlickTeam TrainingPlacementTeam = EFlickTeam::Player1;
	EFlickPieceArchetype TrainingPlacementArchetype = EFlickPieceArchetype::Standard;
	float TrainingPreviousCameraElevation = 0.0f;
	float TrainingBotThinkElapsed = 0.0f;
	bool bTrainingBotThinkingAnnounced = false;
	bool bShotClockTrackingActive = false;
	EFlickTeam ShotClockTrackedTeam = EFlickTeam::None;
	int32 ShotClockTrackedPlayerSlot = INDEX_NONE;
	EFlickMatchPhase ShotClockTrackedPhase = EFlickMatchPhase::WaitingToStart;
	float InitialClassSelectionTimeRemaining = 10.0f;
	bool bInitialClassSelectionTimerActive = false;
	FRandomStream TrainingBotRandom;
	bool bHasTrainingResetSnapshot = false;
	TArray<FFlickTrainingPieceSnapshot> TrainingResetSnapshot;
	bool bPlayerClassesActiveForMatch = false;
	bool bClassSelectionForNextRound = false;
	bool bHasPendingClassChanges = false;
	TArray<EFlickLineupPreset> Player1ActiveClasses;
	TArray<EFlickLineupPreset> Player2ActiveClasses;
	TArray<EFlickLineupPreset> Player1PendingClasses;
	TArray<EFlickLineupPreset> Player2PendingClasses;
	TArray<EFlickLineupPreset> Player1ClassDraft;
	TArray<EFlickLineupPreset> Player2ClassDraft;

	float ResolutionElapsed = 0.0f;
	float SettledElapsed = 0.0f;
	float MenuPreviewElapsed = 0.0f;
	float MenuPreviewTransitionElapsed = 0.0f;
	float LastImpactFeedbackTime = -100.0f;
	float LastStrongImpactEventTime = -100.0f;
	bool bMenuPreviewTransitionActive = false;
	bool bMenuPreviewSwapApplied = false;
	bool bNetworkMatchRequested = false;
	bool bNetworkMatchStarted = false;
	bool bPartyRequested = false;
	bool bMatchmakingRequested = false;
	bool bRankedRequested = false;
	bool bRankedQueueSelected = false;
	bool bRankedResultsDispatched = false;
	bool bRankedMatchRegistrationPending = false;
	bool bHasActiveRankedMatchRequest = false;
	FFlickRankedMatchRequest ActiveRankedMatchRequest;
	int32 RankedQueueRating = 1000;
	bool bVariantProvidedByTravel = false;
	bool bCoordinatorQueuePending = false;
	int32 PendingCoordinatorTeamSize = 1;
	FString CoordinatorMatchId;
	FDelegateHandle CoordinatorAllocatedHandle;
	bool bMatchmakingLobbyLocked = false;
	float MatchmakingQueueElapsed = 0.0f;
	float MatchmakingTimeoutSeconds = 90.0f;
	float MatchFoundConfirmationElapsed = 0.0f;
	float MatchFoundConfirmationTimeoutSeconds = 30.0f;
	TMap<FString, EFlickTeam> DisconnectedPlayerTeams;
	TMap<FString, int32> DisconnectedPlayerSlots;
	TMap<APlayerController*, FString> IncomingPartyIds;
	TMap<APlayerController*, int32> IncomingPartySlots;
	TMap<APlayerController*, int32> IncomingPartySizes;
	TMap<APlayerController*, bool> IncomingPartyLeaders;
	TMap<APlayerController*, int32> IncomingRankedRatings;
	TMap<APlayerController*, FString> IncomingCoordinatorAccountIds;
	TMap<APlayerController*, FString> IncomingCoordinatorReservations;
	TMap<APlayerController*, EFlickTeam> IncomingCoordinatorTeams;
	TMap<APlayerController*, int32> IncomingCoordinatorSlots;
	TMap<APlayerController*, FString> CoordinatorAuthenticationTickets;
	TMap<APlayerController*, FString> CoordinatorQueueAccountIds;
	TSet<APlayerController*> VerifiedCoordinatorPlayers;
	TMap<APlayerController*, int32> PlayerRankedRatings;
	TMap<FString, EFlickTeam> PremadePartyTeams;
	TMap<FString, TArray<int32>> PremadePartySlots;
};
