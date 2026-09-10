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
class AFlickTestArena;
class UPrimitiveComponent;
class AFlickWorldFeedback;
class ADirectionalLight;
class APointLight;
class ARectLight;
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

struct FFlickLockedKickoffShot
{
	TWeakObjectPtr<AFlickPiece> Piece;
	FVector Direction = FVector::ZeroVector;
	float Power = 0.0f;
	EFlickTeam Team = EFlickTeam::None;
	int32 PlayerSlot = INDEX_NONE;
};

struct FFlickReplayPieceState
{
	TWeakObjectPtr<AFlickPiece> Piece;
	FTransform Transform = FTransform::Identity;
	bool bVisible = true;
};

struct FFlickRoundReplayFrame
{
	float Time = 0.0f;
	uint16 RaisedDividerMask = 0;
	TArray<FFlickReplayPieceState> Pieces;
};

struct FFlickReplayShotSetup
{
	TWeakObjectPtr<AFlickPiece> Piece;
	FVector Direction = FVector::ZeroVector;
	float Power = 0.0f;
};

USTRUCT(BlueprintType)
struct FFlickBotDifficultySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ThinkDelay = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float MinimumPower = 0.48f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float MaximumPower = 0.89f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float AimErrorDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float PowerVariation = 0.055f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float DecisionNoise = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DividerAwareness = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BankShotSkill = 0.12f;
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
	void NotifyArenaImpact(AFlickPiece* Piece, UPrimitiveComponent* OtherComponent, const FVector& ImpactLocation, float ImpactVelocityChange);

	EFlickFrontendScreen GetFrontendScreen() const { return FrontendScreen; }
	EFlickFrontendScreen GetSettingsReturnScreen() const { return SettingsReturnScreen; }
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
	bool IsTutorialMode() const { return bTrainingMode && bTutorialMode; }
	bool IsFreePlayTraining() const { return bTrainingMode && !bTrainingBotMatch && !bTutorialMode; }
	bool IsTrainingEditMode() const { return bTrainingMode && bTrainingEditMode; }
	int32 GetTutorialStageNumber() const { return TutorialStageIndex + 1; }
	int32 GetTutorialStageCount() const { return TutorialStageTotal; }
	FString GetTutorialTitle() const;
	FString GetTutorialObjective() const;
	FString GetTutorialHint() const;
	bool IsTutorialComplete() const { return bTutorialCompleted; }
	bool IsTutorialTransitioning() const { return TutorialTransitionRemaining > 0.0f; }
	bool IsTestArenaMode() const { return bTestArenaMode; }
	bool IsCinematicReplayActive() const { return bCinematicReplayActive; }
	bool IsCinematicReplayPullbackActive() const;
	float GetCinematicReplayProgress() const;
	float GetCinematicReplayPullbackAlpha() const;
	int32 GetCinematicReplayShotCount() const { return ReplayShotSetups.Num(); }
	const AFlickPiece* GetCinematicReplayShotPiece(int32 ShotIndex) const;
	FVector GetCinematicReplayShotDirection(int32 ShotIndex) const;
	float GetCinematicReplayShotPower(int32 ShotIndex) const;
	const AFlickTestArena* GetTestArena() const { return TestArenaActor; }
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
	/** Returns the next eligible team/player in the actual alternating turn order. */
	bool GetNextScheduledTurn(EFlickTeam& OutTeam, int32& OutPlayerSlot) const;
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
	EFlickPieceArchetype GetClassLoadoutPiece(EFlickLineupPreset Preset, int32 SlotIndex) const;
	EFlickLineupPreset GetPlayerClass(EFlickTeam Team, int32 PlayerSlot, bool bUseDraft = false) const;
	int32 GetClassSelectionPlayersPerTeam() const;
	bool IsChangingClassForNextRound() const { return bClassSelectionForNextRound; }
	bool IsPreparingTrainingBotMatch() const { return bClassSelectionStartsTrainingBotMatch; }
	bool HasPendingClassChanges() const { return bHasPendingClassChanges; }
	bool CanOpenClassChange() const;
	int32 GetCurrentStartingPiecesPerTeam() const { return CurrentStartingPiecesPerTeam; }
	EFlickMatchVariant GetLoadoutEditingVariant() const { return LoadoutEditingVariant; }
	EFlickLineupPreset GetLoadoutEditingPreset() const { return LoadoutEditingPreset; }
	int32 GetLoadoutEditingPieceCount() const;
	int32 GetCurrentRoundsToWin() const { return CurrentRoundsToWin; }
	bool IsBobMode() const { return ActiveMatchVariant == EFlickMatchVariant::Bob; }
	bool DoesSelectedModeSupportLoadouts() const;
	bool CanChangeCameraView() const;
	bool IsAimGuideEnabled() const;
	bool AreImpactEffectsEnabled() const;
	bool IsControlOverviewEnabled() const;
	EFlickBotDifficulty GetBotDifficulty() const;
	FString GetBotDifficultyLabel() const { return GetBotDifficultyName(GetBotDifficulty()); }
	float GetCameraShakeIntensity() const;
	float GetMasterVolume() const;
	float GetEffectsVolume() const;
	float GetInterfaceVolume() const;
	float GetFreeCameraLookSensitivity() const;
	float GetFreeCameraMoveSensitivity() const;
	FString GetClassName(EFlickLineupPreset Preset) const;
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
	void SelectLoadoutEditingPreset(EFlickLineupPreset Preset);
	void OpenItemShop();
	void CloseItemShop();
	void OpenProfile();
	void CloseProfile();
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
	void StartTutorialMode();
	void ToggleTrainingEditMode();
	void SetTrainingPlacementTeam(EFlickTeam Team);
	void CycleTrainingPlacementArchetype(int32 Direction);
	bool PlaceTrainingPuck(const FVector& WorldLocation);
	bool ToggleTrainingDivider(const FVector& WorldLocation);
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
	void SetControlOverviewEnabled(bool bEnabled);
	void CycleBotDifficulty(int32 Direction);
	void SetCameraShakeIntensity(float Intensity);
	void SetMasterVolume(float Volume);
	void SetEffectsVolume(float Volume);
	void SetInterfaceVolume(float Volume);
	void SetFreeCameraLookSensitivity(float Sensitivity);
	void SetFreeCameraMoveSensitivity(float Sensitivity);
	void SetClassName(EFlickLineupPreset Preset, const FString& Name);
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
	bool HasLockedKickoffShot() const { return !LockedKickoffShots.IsEmpty(); }
	const AFlickPiece* GetLockedKickoffPiece() const;
	FVector GetLockedKickoffDirection() const;
	float GetLockedKickoffPower() const;
	int32 CountActivePieces(EFlickTeam Team) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaThickness = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaSurfaceZ = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float KillZ = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float KnockoutBoundsTolerance = 2.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Feedback", meta = (ClampMin = "2", ClampMax = "12"))
	int32 DramaticChainImpactThreshold = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Accolades", meta = (ClampMin = "100.0", ClampMax = "1300.0"))
	float LongRangeKnockoutDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Accolades", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float BuzzerBeaterTimeThreshold = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Accolades", meta = (ClampMin = "0.5", ClampMax = "0.98"))
	float PrecisionStopMinimumRadiusFraction = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Accolades", meta = (ClampMin = "0.6", ClampMax = "1.0"))
	float PrecisionStopMaximumRadiusFraction = 0.93f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Feedback", meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float DramaticEventDuration = 2.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 TradeBonusPoints = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 DoubleKnockoutBonusPoints = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 MultiKnockoutBonusPerPuck = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 LastPuckStandingBonusPoints = 25;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 ChainReactionBonusPoints = 10;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot")
	FFlickBotDifficultySettings TrainingBotEasySettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot")
	FFlickBotDifficultySettings TrainingBotNormalSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot")
	FFlickBotDifficultySettings TrainingBotHardSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Training|Bot")
	FFlickBotDifficultySettings TrainingBotExpertSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers", meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float ShotTimeLimit = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers", meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float InitialClassSelectionTimeLimit = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers", meta = (ClampMin = "3.0", ClampMax = "60.0"))
	float RoundAdvanceTimeLimit = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Debug")
	bool bLogPhysicsResolution = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation", meta = (ClampMin = "2.0"))
	float MenuPreviewDuration = 6.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation", meta = (ClampMin = "0.35", ClampMax = "2.0"))
	float MenuPreviewTransitionDuration = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Replay", meta = (ClampMin = "12.0", ClampMax = "60.0"))
	float ReplayCaptureRate = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Replay", meta = (ClampMin = "0.25", ClampMax = "1.0"))
	float ReplaySlowMotionRate = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Replay", meta = (ClampMin = "2.0", ClampMax = "10.0"))
	float ReplayMaximumPlaybackDuration = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Replay", meta = (ClampMin = "0.5", ClampMax = "4.0"))
	float ReplayPullbackDuration = 1.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Test Arena", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float TestPuckKeyLightIntensity = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Test Arena", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float TestPuckRimLightIntensity = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Frontend", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float FrontendArenaDirectionalLightMultiplier = 1.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Frontend", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float FrontendArenaSkyLightMultiplier = 1.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Presentation|Frontend", meta = (ClampMin = "1.0", ClampMax = "2.0"))
	float FrontendArenaFillLightMultiplier = 1.18f;

private:
	void SpawnCameraIfNeeded();
	void SpawnAudioIfNeeded();
	void SpawnLightingIfNeeded();
	void SpawnArenaIfNeeded();
	void SpawnPieces();
	void SpawnBobPieces();
	void BeginSelectedMatch();
	void BeginTrainingActivity(bool bAgainstBot);
	void SetupTutorialStage(int32 StageIndex);
	void ResolveTutorialShot();
	void UpdateTutorial(float DeltaSeconds);
	void UpdateTrainingBot(float DeltaSeconds);
	void UpdateShotClock();
	void UpdateInitialClassSelectionTimer(float DeltaSeconds);
	void UpdateNetworkClassSelectionTimer();
	void UpdateRoundAdvanceTimer();
	bool TryExecuteTrainingBotShot();
	void ResetTrainingBotThinking();
	const FFlickBotDifficultySettings& GetTrainingBotDifficultySettings() const;
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
	void ReleaseKickoffShots();
	void ResetKickoffState();
	void BeginResolutionTracking(EFlickTeam ShootingTeam, bool bSimultaneousShot, AFlickPiece* ShotPiece = nullptr);
	void CaptureTestArenaControlZones();
	void TrackTestArenaControlZones(float DeltaSeconds);
	void ResolveTestArenaControlZones();
	void CaptureRoundReplayFrame(bool bForce = false);
	void BeginCinematicRoundReplay(EFlickMatchOutcome Outcome);
	void UpdateCinematicRoundReplay(float DeltaSeconds);
	void FinishCinematicRoundReplay(bool bCompleteRound);
	void ApplyCinematicReplayTime(float SourceTime);
	void PresentDramaticResolutionEvent();
	void PresentShotAccolades();
	void AwardFlawlessRound(EFlickMatchOutcome Outcome);
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
	TObjectPtr<AFlickTestArena> TestArenaActor;

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
	TObjectPtr<ARectLight> TestPuckKeyLight;

	UPROPERTY()
	TObjectPtr<ARectLight> TestPuckRimLight;

	UPROPERTY()
	TArray<TObjectPtr<AFlickPiece>> Pieces;

	UPROPERTY()
	TObjectPtr<AFlickPiece> Player1BobStriker;

	UPROPERTY()
	TObjectPtr<AFlickPiece> Player2BobStriker;

	TArray<FFlickLockedKickoffShot> LockedKickoffShots;
	bool bResolvingKickoff = false;
	int32 ResolutionPlayer1Eliminated = 0;
	int32 ResolutionPlayer2Eliminated = 0;
	int32 ResolutionImpactCount = 0;
	EFlickTeam ResolutionShootingTeam = EFlickTeam::None;
	bool bResolutionWasSimultaneous = false;
	int32 ResolutionShotPieceId = INDEX_NONE;
	FVector2D ResolutionShotStart = FVector2D::ZeroVector;
	bool bResolutionBuzzerRelease = false;
	uint16 ResolutionActivatedSwitchMask = 0;
	uint16 ResolutionNewlyRaisedDividerMask = 0;
	TMap<int32, FVector2D> ResolutionInitialPieceLocations;
	TMap<int32, int32> ResolutionContactDepths;
	TSet<int32> ResolutionDirectContactPieceIds;
	TSet<int32> ResolutionEliminatedPieceIds;
	TSet<int32> ResolutionDividerContactPieceIds;
	TSet<int32> ResolutionNewDividerContactPieceIds;
	TMap<int32, float> ResolutionFirstImpactTimes;
	TMap<int32, float> ResolutionFirstOpponentImpactTimes;
	TMap<int32, float> ResolutionEliminationTimes;
	TSet<int32> ReplayPresentedEliminationPieceIds;
	TArray<FFlickRoundReplayFrame> RoundReplayFrames;
	TArray<FFlickReplayShotSetup> ReplayShotSetups;
	float LastReplayCaptureTime = -100.0f;
	float CinematicReplayElapsed = 0.0f;
	float CinematicReplaySourceStart = 0.0f;
	float CinematicReplaySourceDuration = 0.0f;
	float CinematicReplayMotionDuration = 0.0f;
	float CinematicReplayPlaybackDuration = 0.0f;
	float ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	int32 ReplayPrimaryFocusPieceId = INDEX_NONE;
	int32 ReplayKnockoutFocusPieceId = INDEX_NONE;
	EFlickMatchOutcome PendingReplayOutcome = EFlickMatchOutcome::Continue;
	bool bCinematicReplayActive = false;
	bool bReplayPlayedForResolution = false;
	bool bReplayLaunchCuePlayed = false;

	EFlickFrontendScreen FrontendScreen = EFlickFrontendScreen::MainMenu;
	EFlickFrontendScreen SettingsReturnScreen = EFlickFrontendScreen::MainMenu;
	EFlickFrontendScreen LoadoutReturnScreen = EFlickFrontendScreen::MainMenu;
	EFlickFrontendScreen ClassSelectionReturnScreen = EFlickFrontendScreen::ModeSelect;
	EFlickMatchVariant SelectedMatchVariant = EFlickMatchVariant::Classic;
	EFlickMatchVariant ActiveMatchVariant = EFlickMatchVariant::Classic;
	EFlickMatchVariant LoadoutEditingVariant = EFlickMatchVariant::Classic;
	EFlickLineupPreset LoadoutEditingPreset = EFlickLineupPreset::Balanced;
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
	bool bTutorialMode = false;
	bool bTutorialCompleted = false;
	bool bTutorialAdvancePending = false;
	static constexpr int32 TutorialStageTotal = 4;
	int32 TutorialStageIndex = 0;
	int32 TutorialShotPieceId = INDEX_NONE;
	int32 TutorialTargetPieceId = INDEX_NONE;
	float TutorialTransitionRemaining = 0.0f;
	bool bClassSelectionStartsTrainingBotMatch = false;
	bool bTestArenaMode = false;
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
