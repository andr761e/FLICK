#include "Player/FlickPlayerController.h"
#include "Arena/FlickArenaLighting.h"
#include "Arena/FlickTestArena.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickControlBindings.h"
#include "Core/FlickCosmeticCatalog.h"

#include "Core/FlickLog.h"
#include "DrawDebugHelpers.h"
#include "Debug/FlickPhysicsDiagnosticsComponent.h"
#include "Player/FlickPostMatchPresentationComponent.h"
#include "Player/FlickPrivateSpectatorComponent.h"
#include "Player/FlickTeamPingComponent.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "InputCoreTypes.h"
#include "Math/RotationMatrix.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Online/FlickMatchmakingCoordinatorSubsystem.h"
#include "Core/FlickPlaylistRules.h"
#include "Online/FlickSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPlayerState.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "TimerManager.h"
#include "UI/FlickHUD.h"

AFlickPlayerController::AFlickPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	PrimaryActorTick.bCanEverTick = true;
	PhysicsDiagnostics = CreateDefaultSubobject<UFlickPhysicsDiagnosticsComponent>(TEXT("PhysicsDiagnostics"));
	PostMatchPresentation = CreateDefaultSubobject<UFlickPostMatchPresentationComponent>(TEXT("PostMatchPresentation"));
	PrivateSpectator = CreateDefaultSubobject<UFlickPrivateSpectatorComponent>(TEXT("PrivateSpectator"));
	TeamPings = CreateDefaultSubobject<UFlickTeamPingComponent>(TEXT("TeamPings"));
	bShouldPerformFullTickWhenPaused = true;
}

void AFlickPlayerController::SetGameplayCameraTeamFromServer(const EFlickTeam Team, const bool bSnap)
{
	if (IsLocalController())
	{
		ClientSetGameplayCameraTeam_Implementation(Team, bSnap);
		return;
	}
	ClientSetGameplayCameraTeam(Team, bSnap);
}

void AFlickPlayerController::HandleReplaySkipPressed()
{
	if (!bCinematicReplayPresentationActive) return;
	if (const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>())
	{
		ServerRequestReplaySkip(State->ReplaySerial);
	}
}

void AFlickPlayerController::ServerRequestReplaySkip_Implementation(const int32 ReplaySerial)
{
	if (AFlickGameMode* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>())
	{
		Mode->RequestReplaySkip(this, ReplaySerial);
	}
}

void AFlickPlayerController::BeginCinematicReplayFromServer(const FVector& InitialFocus, const EFlickTeam ShootingTeam)
{
	if (IsLocalController())
	{
		ClientBeginCinematicReplay_Implementation(InitialFocus, ShootingTeam);
		return;
	}
	ClientBeginCinematicReplay(InitialFocus, ShootingTeam);
}

void AFlickPlayerController::UpdateCinematicReplayFromServer(
	const FVector& Focus,
	const float NormalizedProgress,
	const float PullbackAlpha)
{
	if (IsLocalController())
	{
		ClientUpdateCinematicReplay_Implementation(Focus, NormalizedProgress, PullbackAlpha);
		return;
	}
	ClientUpdateCinematicReplay(Focus, NormalizedProgress, PullbackAlpha);
}

void AFlickPlayerController::EndCinematicReplayFromServer()
{
	if (IsLocalController())
	{
		ClientEndCinematicReplay_Implementation();
		return;
	}
	ClientEndCinematicReplay();
}

void AFlickPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyFrontendInputMode();
	bNetworkAutoShotRequested = FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkAutoShot"));
	bNetworkAutoReadyRequested = FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkAutoReady"));

}

void AFlickPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();

	if (!IsGameplayActive())
	{
		ApplyFrontendInputMode();
	}
}

void AFlickPlayerController::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
	bPuckSkinsSubmitted = false;

	if (!IsGameplayActive())
	{
		ApplyFrontendInputMode();
	}
}

void AFlickPlayerController::ApplyFrontendInputMode()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AFlickPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	RefreshControlBindings();
}

void AFlickPlayerController::RefreshControlBindings()
{
	if (!InputComponent) return;
	InputComponent->KeyBindings.Reset();
	const auto Key = [](const TCHAR* Id) { return FlickControlBindings::GetKey(Id); };
	InputComponent->BindKey(Key(TEXT("ReplaySkip")), IE_Pressed, this, &AFlickPlayerController::HandleReplaySkipPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Shoot")), IE_Pressed, this, &AFlickPlayerController::HandlePrimaryPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("TeamPing")), IE_Pressed, this, &AFlickPlayerController::HandleTeamPingPressed);
	InputComponent->BindKey(Key(TEXT("Shoot")), IE_Released, this, &AFlickPlayerController::HandlePrimaryReleased).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Secondary")), IE_Pressed, this, &AFlickPlayerController::HandleSecondaryPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Menu")), IE_Pressed, this, &AFlickPlayerController::HandleCancelPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Restart")), IE_Pressed, this, &AFlickPlayerController::HandleRestartPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Editor")), IE_Pressed, this, &AFlickPlayerController::HandleTrainingEditorTogglePressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("OwnPuck")), IE_Pressed, this, &AFlickPlayerController::HandleTrainingOwnPuckPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("TargetPuck")), IE_Pressed, this, &AFlickPlayerController::HandleTrainingTargetPuckPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Clear")), IE_Pressed, this, &AFlickPlayerController::HandleTrainingClearPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("FreeCamera")), IE_Pressed, this, &AFlickPlayerController::HandleFreeCameraTogglePressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Remove")), IE_Pressed, this, &AFlickPlayerController::HandleTrainingRemovePressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("TopView")), IE_Pressed, this, &AFlickPlayerController::HandleTopDownViewPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("CameraUp")), IE_Pressed, this, &AFlickPlayerController::HandleCameraElevationUpPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("CameraDown")), IE_Pressed, this, &AFlickPlayerController::HandleCameraElevationDownPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("CameraReset")), IE_Pressed, this, &AFlickPlayerController::HandleCameraResetPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Scoreboard")), IE_Pressed, this, &AFlickPlayerController::HandleScoreboardPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("Scoreboard")), IE_Released, this, &AFlickPlayerController::HandleScoreboardReleased).bExecuteWhenPaused = true;
	InputComponent->BindKey(Key(TEXT("OrbitLeft")), IE_Pressed, this, &AFlickPlayerController::HandleSpectatorPreviousPressed);
	InputComponent->BindKey(Key(TEXT("OrbitRight")), IE_Pressed, this, &AFlickPlayerController::HandleSpectatorNextPressed);
}

void AFlickPlayerController::RefreshLocalLighting()
{
	if (!IsLocalController()) return;
	if (AFlickGameMode* Mode = GetFlickGameMode())
	{
		Mode->RefreshLightingSettings();
		return;
	}
	const AFlickGameState* State = GetFlickGameState();
	if (!State) return;
	FlickArenaLighting::FParameters Parameters;
	Parameters.bClassicArenaLighting = State->ActiveMatchVariant == EFlickMatchVariant::Classic;
	Parameters.bBobArenaLighting = State->ActiveMatchVariant == EFlickMatchVariant::Bob;
	Parameters.bTestArenaMode = FlickModeRules::Get(State->ActiveMatchVariant).bUseSwitchyardArena;
	Parameters.bPremiumArena = Parameters.bTestArenaMode;
	Parameters.bPremiumMenu = State->bPartyActive && !State->IsGameplayActive() && !State->bNetworkLobbyActive
		&& !State->bPrivateMatchActive && !State->bPrivateMatchLobbyActive && !State->bNetworkClassSelectionActive;
	Parameters.bFrontendShowcase = Parameters.bPremiumMenu || !State->IsGameplayActive();
	Parameters.ArenaRadius = FlickModeRules::GetArenaRadius(State->ActiveMatchVariant, State->PlayersPerTeam);
	if (Parameters.bClassicArenaLighting && (State->bPrivateMatchActive || State->bPrivateMatchLobbyActive))
		Parameters.ArenaRadius *= FMath::Clamp(State->PrivateMatchSettings.ArenaScale, 0.85f, 1.3f);
	Parameters.ArenaSurfaceZ = State->ArenaSurfaceZ;
	FlickArenaLighting::FRig Rig = FlickArenaLighting::FindRig(GetWorld());
	FlickArenaLighting::Configure(GetWorld(), Rig, Parameters);
}

void AFlickPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (IsLocalController() && !bPuckSkinsSubmitted && GetPlayerState<AFlickPlayerState>()) SubmitLocalPuckSkins();
	InspectedPiece.Reset();
	const AFlickGameState* NetworkState = GetFlickGameState();
	// Remote players own their light rig locally. Only update on presentation changes,
	// never scan or recreate fixtures each frame, and never send preferences to the host.
	if (IsLocalController() && GetNetMode() == NM_Client && NetworkState)
	{
		const int32 Context = static_cast<int32>(NetworkState->ActiveMatchVariant)
			| (NetworkState->PlayersPerTeam << 4) | (NetworkState->bPartyActive << 8)
			| (NetworkState->bNetworkLobbyActive << 9) | (NetworkState->bPrivateMatchActive << 10)
			| (NetworkState->bPrivateMatchLobbyActive << 11) | (NetworkState->bNetworkClassSelectionActive << 12)
			| (NetworkState->IsGameplayActive() << 13);
		if (LocalLightingContext != Context || LocalLightingArenaScale != NetworkState->PrivateMatchSettings.ArenaScale)
		{
			LocalLightingContext = Context;
			LocalLightingArenaScale = NetworkState->PrivateMatchSettings.ArenaScale;
			RefreshLocalLighting();
		}
	}
	const bool bPrivateMatchNowActive = NetworkState && NetworkState->bPrivateMatchActive;
	if (bPrivateMatchNowActive && !bObservedPrivateMatchActive)
	{
		bPrivateSpectateChosen = false;
		bPrivateTeamMenuOpen = false;
		bInitializedNetworkCamera = false;
	}
	bObservedPrivateMatchActive = bPrivateMatchNowActive;
	UpdateCareerStatsTracking(NetworkState);
	const AFlickGameMode* PresentationMode = GetFlickGameMode();
	if (IsLocalController() && NetworkState && NetworkState->bSeriesComplete
		&& NetworkState->MatchPhase == EFlickMatchPhase::RoundOver
		&& !bCinematicReplayPresentationActive
		&& (!PresentationMode || PresentationMode->GetFrontendScreen() == EFlickFrontendScreen::Playing))
	{
		// A private spectator may finish the match in free-camera/game-only input.
		// Restore the pointer before the post-match actions appear, on every client.
		if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(GetPawn()); Camera && Camera->IsFreeCameraEnabled())
		{
			Camera->SetFreeCameraEnabled(false);
			SetFreeCameraInputMode(false);
		}
		bPrivateTeamMenuOpen = false;
		bScoreboardVisible = false;
		ClearAiming();
		ClearHoveredPiece();
		bShowMouseCursor = true;
		CurrentMouseCursor = EMouseCursor::Default;
		return;
	}
	if (bNetworkAutoReadyRequested && NetworkState && NetworkState->bNetworkLobbyActive)
	{
		const AFlickPlayerState* FlickPlayerState = GetPlayerState<AFlickPlayerState>();
		if (!bNetworkAutoReadySubmitted && FlickPlayerState && FlickPlayerState->GetTeam() != EFlickTeam::None)
		{
			bNetworkAutoReadySubmitted = true;
			ToggleLobbyReady();
			UE_LOG(LogFlick, Log, TEXT("NETWORK_AUTO_READY_SUBMITTED: %s"), *GetTeamDisplayName(FlickPlayerState->GetTeam()));
		}
		if (!bNetworkAutoStartSubmitted)
		{
			if (AFlickGameMode* FlickGameMode = GetFlickGameMode(); FlickGameMode && FlickGameMode->CanStartNetworkMatch())
			{
				bNetworkAutoStartSubmitted = true;
				RequestStartNetworkMatch();
			}
		}
	}
	if (NetworkState && NetworkState->bNetworkClassSelectionActive)
	{
		if (!bNetworkClassLineupSubmitted)
		{
			bNetworkClassLineupSubmitted = true;
			const AFlickPlayerState* LocalState = GetPlayerState<AFlickPlayerState>();
			RequestSelectClass(LocalState
				? LocalState->GetNetworkSelectedClass()
				: EFlickLineupPreset::Balanced);
		}
	}
	else
	{
		bNetworkClassLineupSubmitted = false;
	}
	if (bNetworkAutoReadyRequested && NetworkState
		&& NetworkState->bNetworkClassSelectionActive && !bNetworkAutoClassSubmitted)
	{
		bNetworkAutoClassSubmitted = true;
		RequestSelectClass(GetLocalTeam() == EFlickTeam::Player2
			? EFlickLineupPreset::Speed
			: EFlickLineupPreset::Power);
		RequestConfirmClass();
		UE_LOG(LogFlick, Log, TEXT("NETWORK_AUTO_CLASS_CONFIRMED: %s"), *GetTeamDisplayName(GetLocalTeam()));
	}
	if (!IsGameplayActive())
	{
		if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
			CameraPawn && CameraPawn->IsFreeCameraEnabled())
		{
			CameraPawn->SetFreeCameraEnabled(false);
			SetFreeCameraInputMode(false);
		}
		if (TrainingDraggedPiece)
		{
			if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
			{
				FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
			}
			TrainingDraggedPiece = nullptr;
		}
		NetworkGameplayElapsed = 0.0f;
		bScoreboardVisible = false;
		ClearAiming();
		bShowMouseCursor = true;
		CurrentMouseCursor = EMouseCursor::Default;
		return;
	}
	NetworkGameplayElapsed += DeltaTime;
	if (UpdateFreeCamera(DeltaTime))
	{
		return;
	}
	UpdateLocalCameraOrbit(DeltaTime);
	if (IsFollowingPrivatePlayer())
	{
		if (auto* Camera = Cast<AFlickCameraPawn>(GetPawn())) Camera->SetMenuPresentation(false);
		bInitializedNetworkCamera = true;
		ClearAiming();
		ClearHoveredPiece();
		bShowMouseCursor = true;
		CurrentMouseCursor = EMouseCursor::Default;
		return;
	}

	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	if (TrainingDraggedPiece && (!FlickGameMode || !FlickGameMode->IsTrainingEditMode()))
	{
		if (FlickGameMode)
		{
			FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
		}
		TrainingDraggedPiece = nullptr;
	}
	if (FlickGameMode
		&& FlickGameMode->IsTrainingEditMode()
		&& FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing)
	{
		bShowMouseCursor = true;
		bScoreboardVisible = false;
		if (TrainingDraggedPiece)
		{
			FVector CursorPoint;
			if (GetCursorPointOnArenaPlane(CursorPoint))
			{
				FlickGameMode->MoveTrainingPuck(TrainingDraggedPiece, CursorPoint);
			}
			CurrentMouseCursor = EMouseCursor::GrabHandClosed;
		}
		else
		{
			UpdateHoveredPiece();
			CurrentMouseCursor = HoveredPiece ? EMouseCursor::GrabHand : EMouseCursor::Crosshairs;
		}
		return;
	}

	if (!bInitializedNetworkCamera && GetNetMode() != NM_Standalone)
	{
		const EFlickTeam LocalTeam = GetLocalTeam();
		if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
			CameraPawn && (LocalTeam != EFlickTeam::None
				|| (NetworkState && NetworkState->bPrivateMatchActive)))
		{
			CameraPawn->SetMenuPresentation(false);
			CameraPawn->SetGameplayViewIndex(LocalTeam == EFlickTeam::Player2 ? 2 : 0, true);
			bInitializedNetworkCamera = true;
		}
	}

	if (bNetworkAutoShotRequested && !bNetworkAutoShotSubmitted && NetworkGameplayElapsed >= 2.0f)
	{
		for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
		{
			AFlickPiece* Piece = *It;
			if (!CanSelectPieceLocally(Piece))
			{
				continue;
			}
			FVector Direction = -Piece->GetActorLocation();
			Direction.Z = 0.0f;
			SubmitLaunch(Piece, Direction.GetSafeNormal(), 0.28f);
			bNetworkAutoShotSubmitted = true;
			UE_LOG(
				LogFlick,
				Log,
				TEXT("NETWORK_AUTO_SHOT_SUBMITTED: %s Piece %d"),
				*GetTeamDisplayName(GetLocalTeam()),
				Piece->GetPieceId());
			break;
		}
	}

	if (bAimingShot && !CanSelectPieceLocally(SelectedPiece))
	{
		ClearAiming();
	}
	if (bAimingShot)
	{
		UpdateAimFromCursor();
		if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
		{
			const FVector PieceLocation = SelectedPiece ? SelectedPiece->GetActorLocation() : FVector::ZeroVector;
			const FVector FocusPoint = bHasAimCursorPoint
				? FMath::Lerp(PieceLocation, AimCursorWorldPoint, 0.35f)
				: PieceLocation;
			CameraPawn->SetAimPresentation(true, FocusPoint);
		}
		DrawAimDebug();
		CurrentMouseCursor = EMouseCursor::Crosshairs;
	}
	else
	{
		if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
		{
			CameraPawn->SetAimPresentation(false);
		}
		UpdateHoveredPiece();
		CurrentMouseCursor = HoveredPiece ? EMouseCursor::Hand : EMouseCursor::Default;
	}
}

void AFlickPlayerController::UpdateCareerStatsTracking(const AFlickGameState* FlickGameState)
{
	if (!IsLocalController())
	{
		return;
	}
	if (!FlickGameState || !FlickGameState->bSeriesComplete)
	{
		bCareerStatsRecordedForCurrentSeries = false;
		return;
	}
	if (bCareerStatsRecordedForCurrentSeries)
	{
		return;
	}
	// Training, private matches, and locally hosted sessions never advance the
	// playlist career or its challenges. Matchmaking state is replicated by the
	// authority and remains active through the completed series.
	if (!FlickPlaylistRules::CountsForCareer(
		FlickGameState->bSeriesComplete,
		FlickGameState->bMatchmakingLobby,
		FlickGameState->bPrivateMatchActive))
	{
		bCareerStatsRecordedForCurrentSeries = true;
		return;
	}

	const AFlickPlayerState* LocalPlayerState = GetPlayerState<AFlickPlayerState>();
	if (!LocalPlayerState
		|| LocalPlayerState->GetTeam() == EFlickTeam::None
		|| LocalPlayerState->GetTeamPlayerSlot() < 0)
	{
		return;
	}

	const FFlickPlayerMatchStats* MatchStats = FlickGameState->FindPlayerMatchStats(
		LocalPlayerState->GetTeam(),
		LocalPlayerState->GetTeamPlayerSlot());
	UFlickGameInstance* FlickGameInstance = Cast<UFlickGameInstance>(GetGameInstance());
	if (!MatchStats || !FlickGameInstance)
	{
		return;
	}

	const bool bWon = !FlickGameState->bDraw
		&& FlickGameState->WinnerTeam == LocalPlayerState->GetTeam();
	FlickGameInstance->RecordCompletedMatch(
		MatchStats->Score,
		MatchStats->Knockouts,
		MatchStats->DoubleKnockouts,
		MatchStats->Shots,
		MatchStats->AccoladeCounts,
		bWon,
		FlickGameState->bDraw,
		FlickGameState->ActiveMatchVariant,
		FlickGameState->PlayersPerTeam,
		FlickGameState->bRankedMatch,
		FlickGameState->bMatchmakingLobby);
	bCareerStatsRecordedForCurrentSeries = true;
}

void AFlickPlayerController::ClearAiming()
{
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		CameraPawn->SetAimPresentation(false);
	}
	ClearHoveredPiece();
	if (SelectedPiece)
	{
		SelectedPiece->SetSelected(false);
	}

	SelectedPiece = nullptr;
	CurrentLaunchResult = FFlickLaunchResult();
	AimCursorWorldPoint = FVector::ZeroVector;
	PredictedContactWorldPoint = FVector::ZeroVector;
	PredictedContactPiece = nullptr;
	AimGuideDistance = 0.0f;
	bAimingShot = false;
	bHasAimCursorPoint = false;
	bHasPredictedContact = false;
}

void AFlickPlayerController::HandleTeamPingPressed()
{
	if (!TeamPings) return;
	if (AFlickPiece* Piece = FindPieceUnderCursor())
	{
		TeamPings->TryPing(Piece);
		return; // Never ping a switch through a puck, including friendly pucks.
	}
	FHitResult Hit;
	if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit)) return;
	if (const auto* Arena = Cast<AFlickTestArena>(Hit.GetActor()))
		TeamPings->TryPing(nullptr, Arena->FindSwitchAtWorldLocation(Hit.ImpactPoint));
}

void AFlickPlayerController::HandlePrimaryPressed()
{
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!IsGameplayActive()
		|| (FlickGameState && FlickGameState->MatchPhase == EFlickMatchPhase::RoundOver))
	{
		ClearAiming();
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (GetMousePosition(MouseX, MouseY))
		{
			if (AFlickHUD* FlickHUD = Cast<AFlickHUD>(GetHUD()))
			{
				FlickHUD->HandleMenuClick(FVector2D(MouseX, MouseY));
			}
		}
		return;
	}
	if (FlickGameMode && FlickGameMode->IsTrainingEditMode())
	{
		ClearAiming();
		ClearHoveredPiece();
		if (AFlickPiece* HitPiece = FindPieceUnderCursor();
			HitPiece && FlickGameMode->BeginTrainingPuckMove(HitPiece))
		{
			TrainingDraggedPiece = HitPiece;
			return;
		}

		FVector CursorPoint;
		if (GetCursorPointOnArenaPlane(CursorPoint))
		{
			if (FlickGameMode->ToggleTrainingDivider(CursorPoint))
			{
				return;
			}
			FlickGameMode->PlaceTrainingPuck(CursorPoint);
		}
		return;
	}

	ClearAiming();
	ClearHoveredPiece();

	AFlickPiece* HitPiece = FindPieceUnderCursor();

	if (!CanSelectPieceLocally(HitPiece))
	{
		return;
	}

	SelectedPiece = HitPiece;
	SelectedPiece->SetSelected(true);
	bAimingShot = true;
	UpdateAimFromCursor();
	if (FlickGameMode)
	{
		FlickGameMode->NotifyPieceSelected(SelectedPiece);
	}
}

void AFlickPlayerController::HandlePrimaryReleased()
{
	if (!IsGameplayActive())
	{
		return;
	}
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode
		&& FlickGameMode->IsTrainingEditMode()
		&& FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing)
	{
		if (TrainingDraggedPiece)
		{
			FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
			TrainingDraggedPiece = nullptr;
		}
		return;
	}

	if (!bAimingShot || !SelectedPiece)
	{
		ClearAiming();
		return;
	}

	UpdateAimFromCursor();
	if (CurrentLaunchResult.bValidShot)
	{
		SubmitLaunch(SelectedPiece, CurrentLaunchResult.Direction, CurrentLaunchResult.NormalizedPower);
	}

	ClearAiming();
}

void AFlickPlayerController::HandleCameraElevationUpPressed()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode
		&& FlickGameMode->IsTrainingEditMode()
		&& FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing)
	{
		FlickGameMode->CycleTrainingPlacementArchetype(1);
		return;
	}
	AdjustLocalCameraElevation(1);
}

void AFlickPlayerController::HandleCameraElevationDownPressed()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode
		&& FlickGameMode->IsTrainingEditMode()
		&& FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing)
	{
		FlickGameMode->CycleTrainingPlacementArchetype(-1);
		return;
	}
	AdjustLocalCameraElevation(-1);
}

void AFlickPlayerController::HandleCameraResetPressed()
{
	if (IsFollowingPrivatePlayer()) return;
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	if (!IsGameplayActive()
		|| !FlickGameMode
		|| !FlickGameMode->CanChangeCameraView()
		|| FlickGameMode->IsCinematicReplayActive())
	{
		return;
	}

	EFlickTeam ViewTeam = GetLocalTeam();
	if (ViewTeam == EFlickTeam::None)
	{
		if (const AFlickGameState* FlickGameState = GetFlickGameState())
		{
			ViewTeam = FlickGameState->CurrentTeam;
		}
	}

	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		ClearHoveredPiece();
		CameraPawn->ResetGameplayView(
			ViewTeam == EFlickTeam::Player2 ? 2 : 0,
			!FlickGameMode->IsTrainingEditMode());
	}
}

void AFlickPlayerController::HandleTopDownViewPressed()
{
	if (IsFollowingPrivatePlayer()) return;
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	if (!IsGameplayActive() || (FlickGameMode && (!FlickGameMode->CanChangeCameraView()
		|| FlickGameMode->IsCinematicReplayActive()))) return;
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		ClearHoveredPiece();
		CameraPawn->ToggleTopDownView();
	}
}

void AFlickPlayerController::HandleScoreboardPressed()
{
	const AFlickGameMode* FlickGameMode = GetFlickGameMode();
	bScoreboardVisible = IsGameplayActive()
		&& (!FlickGameMode || !FlickGameMode->IsFreePlayTraining());
}

void AFlickPlayerController::HandleScoreboardReleased()
{
	bScoreboardVisible = false;
}

void AFlickPlayerController::HandleSecondaryPressed()
{
	// Right click is deliberately scoped to cancelling a prepared mouse shot.
	// It must not double as pause or camera input.
	if (bAimingShot)
	{
		ClearAiming();
	}
}

void AFlickPlayerController::HandleCancelPressed()
{
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
		CameraPawn && CameraPawn->IsFreeCameraEnabled())
	{
		CameraPawn->SetFreeCameraEnabled(false);
		SetFreeCameraInputMode(false);
		if (IsPrivateMatchSpectator() && PrivateSpectator) PrivateSpectator->CycleTarget(0);
		return;
	}
	if (TrainingDraggedPiece)
	{
		if (FlickGameMode)
		{
			FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
		}
		TrainingDraggedPiece = nullptr;
	}
	if (const AFlickGameState* State = GetFlickGameState();
		State && State->bPrivateMatchAssignmentActive)
	{
		bPrivateTeamMenuOpen = true;
		return;
	}
	if (!FlickGameMode)
	{
		const AFlickGameState* State = GetFlickGameState();
		if (State && State->bPrivateMatchActive)
		{
			bPrivateTeamMenuOpen = !bPrivateTeamMenuOpen;
		}
		else ClearAiming();
		return;
	}

	switch (FlickGameMode->GetFrontendScreen())
	{
	case EFlickFrontendScreen::NetworkLobby:
		FlickGameMode->CancelNetworkLobby();
		break;
	case EFlickFrontendScreen::OnlineBrowser:
		FlickGameMode->CloseOnlineBrowser();
		break;
	case EFlickFrontendScreen::ModeSelect:
		FlickGameMode->CloseModeSelect();
		break;
	case EFlickFrontendScreen::PrivateMatch:
		FlickGameMode->ClosePrivateMatchSetup();
		break;
	case EFlickFrontendScreen::Loadout:
		FlickGameMode->CloseLoadout();
		break;
	case EFlickFrontendScreen::ClassSelect:
		FlickGameMode->CancelClassSelection();
		break;
	case EFlickFrontendScreen::ItemShop:
		FlickGameMode->CloseItemShop();
		break;
	case EFlickFrontendScreen::Profile:
		FlickGameMode->CloseProfile();
		break;
	case EFlickFrontendScreen::Settings:
		FlickGameMode->CloseSettings();
		break;
	case EFlickFrontendScreen::Paused:
		FlickGameMode->TogglePauseMenu();
		break;
	case EFlickFrontendScreen::Playing:
		if (bAimingShot)
		{
			ClearAiming();
		}
		else
		{
			FlickGameMode->TogglePauseMenu();
		}
		break;
	case EFlickFrontendScreen::MainMenu:
	default:
		break;
	}
}

void AFlickPlayerController::HandleRestartPressed()
{
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
		CameraPawn && CameraPawn->IsFreeCameraEnabled())
	{
		CameraPawn->SetFreeCameraEnabled(false);
		SetFreeCameraInputMode(false);
	}
	if (TrainingDraggedPiece)
	{
		if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
		{
			FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
		}
		TrainingDraggedPiece = nullptr;
	}
	RequestRestartMatch();
}

bool AFlickPlayerController::IsFreeCameraActive() const
{
	const AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
	return CameraPawn && CameraPawn->IsFreeCameraEnabled();
}

void AFlickPlayerController::HandleTrainingEditorTogglePressed()
{
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	if (!FlickGameMode || !FlickGameMode->IsTrainingMode()
		|| FlickGameMode->GetFrontendScreen() != EFlickFrontendScreen::Playing)
	{
		return;
	}
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
		CameraPawn && CameraPawn->IsFreeCameraEnabled())
	{
		CameraPawn->SetFreeCameraEnabled(false);
		SetFreeCameraInputMode(false);
	}
	if (TrainingDraggedPiece)
	{
		FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
		TrainingDraggedPiece = nullptr;
	}
	ClearAiming();
	ClearHoveredPiece();
	FlickGameMode->ToggleTrainingEditMode();
}

void AFlickPlayerController::HandleTrainingOwnPuckPressed()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode && FlickGameMode->IsTrainingMode())
	{
		FlickGameMode->SetTrainingPlacementTeam(EFlickTeam::Player1);
	}
}

void AFlickPlayerController::HandleTrainingTargetPuckPressed()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode && FlickGameMode->IsTrainingMode())
	{
		FlickGameMode->SetTrainingPlacementTeam(EFlickTeam::Player2);
	}
}

void AFlickPlayerController::HandleTrainingClearPressed()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode && FlickGameMode->IsTrainingEditMode())
	{
		if (TrainingDraggedPiece)
		{
			FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
			TrainingDraggedPiece = nullptr;
		}
		FlickGameMode->ClearTrainingPucks();
		ClearHoveredPiece();
	}
}

void AFlickPlayerController::HandleTrainingRemovePressed()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode();
		FlickGameMode && FlickGameMode->IsTrainingEditMode())
	{
		AFlickPiece* PieceToRemove = TrainingDraggedPiece
			? TrainingDraggedPiece.Get()
			: FindPieceUnderCursor();
		TrainingDraggedPiece = nullptr;
		ClearHoveredPiece();
		FlickGameMode->RemoveTrainingPuck(PieceToRemove);
	}
}

void AFlickPlayerController::HandleFreeCameraTogglePressed()
{
	if (const AFlickGameState* State = GetFlickGameState();
		State && State->bPrivateMatchActive && GetLocalTeam() == EFlickTeam::None)
	{
		TogglePrivateSpectatorFreeCamera();
		return;
	}
	if (const AFlickGameState* State = GetFlickGameState(); State && State->bPrivateMatchActive)
	{
		ClearAiming();
		bPrivateTeamMenuOpen = !bPrivateTeamMenuOpen;
		return;
	}
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
	if (!FlickGameMode || !CameraPawn || !IsGameplayActive()
		|| !FlickGameMode->IsFreePlayTraining()
		|| FlickGameMode->GetFrontendScreen() != EFlickFrontendScreen::Playing)
	{
		return;
	}

	ClearAiming();
	ClearHoveredPiece();
	const bool bEnable = !CameraPawn->IsFreeCameraEnabled();
	CameraPawn->SetFreeCameraEnabled(bEnable);
	SetFreeCameraInputMode(bEnable);
}

void AFlickPlayerController::SetFreeCameraInputMode(const bool bEnabled)
{
	bShowMouseCursor = !bEnabled;
	bEnableClickEvents = !bEnabled;
	bEnableMouseOverEvents = !bEnabled;
	if (bEnabled)
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}

bool AFlickPlayerController::UpdateFreeCamera(const float DeltaSeconds)
{
	AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
	if (!CameraPawn || !CameraPawn->IsFreeCameraEnabled())
	{
		return false;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	GetInputMouseDelta(MouseX, MouseY);
	if (const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>())
	{
		CameraPawn->SetFreeCameraSensitivity(
			Instance->GetFreeCameraLookSensitivity(),
			Instance->GetFreeCameraMoveSensitivity());
	}
	const float Forward = (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveForward"))) ? 1.0f : 0.0f)
		- (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveBack"))) ? 1.0f : 0.0f);
	const float Right = (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveRight"))) ? 1.0f : 0.0f)
		- (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveLeft"))) ? 1.0f : 0.0f);
	const float Up = (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveUp"))) ? 1.0f : 0.0f)
		- (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveDown")))
			|| IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveDownAlt"))) ? 1.0f : 0.0f);
	const bool bBoost = IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveBoost")))
		|| IsInputKeyDown(FlickControlBindings::GetKey(TEXT("MoveBoostAlt")));
	CameraPawn->AddFreeCameraInput(Forward, Right, Up, FVector2D(MouseX, MouseY), bBoost, DeltaSeconds);
	CurrentMouseCursor = EMouseCursor::None;
	return true;
}

void AFlickPlayerController::UpdateAimFromCursor()
{
	CurrentLaunchResult = FFlickLaunchResult();
	bHasAimCursorPoint = false;
	if (!SelectedPiece)
	{
		return;
	}

	FVector CursorPoint = FVector::ZeroVector;
	if (!GetCursorPointOnArenaPlane(CursorPoint))
	{
		return;
	}
	if (const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>())
	{
		const float SensitivityScale = FMath::Lerp(0.4f, 1.6f, Instance->GetShotMouseSensitivity());
		CursorPoint = SelectedPiece->GetActorLocation()
			+ (CursorPoint - SelectedPiece->GetActorLocation()) * SensitivityScale;
	}
	AimCursorWorldPoint = CursorPoint;
	bHasAimCursorPoint = true;

	CurrentLaunchResult = FlickLaunchMath::CalculateLaunch(
		SelectedPiece->GetActorLocation(),
		CursorPoint,
		GetMaxDragDistance(),
		GetMinDragDistance(),
		GetPowerExponent());
	UpdatePredictedContact();
}

void AFlickPlayerController::UpdatePredictedContact()
{
	bHasPredictedContact = false;
	PredictedContactPiece = nullptr;
	PredictedContactWorldPoint = FVector::ZeroVector;
	AimGuideDistance = 0.0f;
	if (!SelectedPiece || !CurrentLaunchResult.bValidShot || !GetWorld())
	{
		return;
	}

	const float SpeedScale = SelectedPiece->GetLaunchSpeedMultiplier();
	AimGuideDistance = FMath::Lerp(260.0f, 920.0f, CurrentLaunchResult.NormalizedPower) * SpeedScale;
	const FVector Start = SelectedPiece->GetActorLocation()
		+ CurrentLaunchResult.Direction * (SelectedPiece->GetPieceRadius() + 3.0f);
	const FVector End = Start + CurrentLaunchResult.Direction * AimGuideDistance;

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FlickContactPreview), false, SelectedPiece);
	FHitResult Hit;
	const bool bHit = GetWorld()->SweepSingleByObjectType(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ObjectTypes,
		FCollisionShape::MakeSphere(SelectedPiece->GetPieceRadius() * 0.82f),
		QueryParams);
	AFlickPiece* HitPiece = bHit ? Cast<AFlickPiece>(Hit.GetActor()) : nullptr;
	if (!HitPiece || !HitPiece->IsActive())
	{
		return;
	}

	bHasPredictedContact = true;
	PredictedContactPiece = HitPiece;
	PredictedContactWorldPoint = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
	AimGuideDistance = FVector::Distance(Start, Hit.Location);
}

void AFlickPlayerController::UpdateHoveredPiece()
{
	if (bCinematicReplayPresentationActive) { ApplyHoveredPiece(nullptr); return; }
	ApplyHoveredPiece(FindPieceUnderCursor());
}

void AFlickPlayerController::ApplyHoveredPiece(AFlickPiece* NewHoveredPiece)
{
	InspectedPiece = NewHoveredPiece && NewHoveredPiece->IsActive() ? NewHoveredPiece : nullptr;
	const AFlickGameMode* FlickGameMode = GetFlickGameMode();
	const bool bCanEditHoveredPiece = FlickGameMode
		&& FlickGameMode->IsTrainingEditMode()
		&& NewHoveredPiece
		&& NewHoveredPiece->IsActive();
	if (!bCanEditHoveredPiece && !CanSelectPieceLocally(NewHoveredPiece))
	{
		NewHoveredPiece = nullptr;
	}

	if (HoveredPiece == NewHoveredPiece)
	{
		return;
	}

	ClearHoveredPiece();
	HoveredPiece = NewHoveredPiece;
	if (HoveredPiece)
	{
		HoveredPiece->SetHovered(true);
	}
}

void AFlickPlayerController::ClearHoveredPiece()
{
	if (HoveredPiece)
	{
		HoveredPiece->SetHovered(false);
	}
	HoveredPiece = nullptr;
}

void AFlickPlayerController::DrawAimDebug() const
{
	if (!bDrawAimDebug || !SelectedPiece || !GetWorld())
	{
		return;
	}

	const FVector PieceLocation = SelectedPiece->GetActorLocation();
	DrawDebugSphere(GetWorld(), PieceLocation + FVector(0.0f, 0.0f, 35.0f), 58.0f, 32, FColor::Yellow, false, 0.0f, 0, 2.0f);

	if (!CurrentLaunchResult.bValidShot)
	{
		return;
	}

	const float ArrowLength = 120.0f + CurrentLaunchResult.NormalizedPower * 280.0f;
	const FVector Start = PieceLocation + FVector(0.0f, 0.0f, 45.0f);
	const FVector End = Start + CurrentLaunchResult.Direction * ArrowLength;
	const FColor ArrowColor = CurrentLaunchResult.NormalizedPower >= 0.98f ? FColor::Red : FColor::Green;
	DrawDebugDirectionalArrow(GetWorld(), Start, End, 36.0f, ArrowColor, false, 0.0f, 0, 5.0f);
}

bool AFlickPlayerController::GetCursorPointOnArenaPlane(FVector& OutWorldPoint) const
{
	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection) || FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float PlaneZ = GetArenaSurfaceZ();
	const float T = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
	if (T < 0.0f)
	{
		return false;
	}

	OutWorldPoint = WorldLocation + WorldDirection * T;
	return true;
}

EFlickTeam AFlickPlayerController::GetLocalTeam() const
{
	const AFlickPlayerState* FlickPlayerState = GetPlayerState<AFlickPlayerState>();
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (FlickPlayerState && FlickGameState
		&& FlickPlayerState->ControlsPrivateSlot(
			FlickGameState->CurrentTeam,
			FlickGameState->CurrentTeamPlayerSlot))
	{
		return FlickGameState->CurrentTeam;
	}
	return FlickPlayerState ? FlickPlayerState->GetTeam() : EFlickTeam::None;
}

bool AFlickPlayerController::OwnsPieceLocally(const AFlickPiece* Piece) const
{
	const AFlickGameState* State = GetFlickGameState();
	const AFlickPlayerState* Local = GetPlayerState<AFlickPlayerState>();
	if (!Piece || !State || !Local || Piece->GetTeam() == EFlickTeam::None) return false;
	return State->bPrivateMatchActive
		? Local->ControlsPrivateSlot(Piece->GetTeam(), Piece->GetOwningPlayerSlot())
		: Local->GetTeam() == Piece->GetTeam()
			&& Local->GetTeamPlayerSlot() == Piece->GetOwningPlayerSlot();
}

bool AFlickPlayerController::CanSelectPieceLocally(const AFlickPiece* Piece) const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->CanSelectPieceForController(this, Piece);
	}

	const AFlickGameState* FlickGameState = GetFlickGameState();
	const AFlickPlayerState* LocalPlayerState = GetPlayerState<AFlickPlayerState>();
	if (!Piece || !FlickGameState || !LocalPlayerState
		|| !FlickGameState->IsGameplayActive()
		|| (FlickGameState->ActiveMatchVariant == EFlickMatchVariant::Bob && !Piece->IsBobStriker()))
	{
		return false;
	}
	if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		return Piece->IsSelectableBy(Piece->GetTeam())
			&& (FlickGameState->bPrivateMatchActive
				? LocalPlayerState->ControlsPrivateSlot(Piece->GetTeam(), Piece->GetOwningPlayerSlot())
				: LocalPlayerState->GetTeam() == Piece->GetTeam()
					&& LocalPlayerState->GetTeamPlayerSlot() == Piece->GetOwningPlayerSlot());
	}
	if (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
		|| !Piece->IsSelectableBy(FlickGameState->CurrentTeam)
		|| (FlickGameState->ActiveMatchVariant != EFlickMatchVariant::Bob
			&& Piece->GetOwningPlayerSlot() != FlickGameState->CurrentTeamPlayerSlot))
	{
		return false;
	}
	return FlickGameState->bPrivateMatchActive
		? LocalPlayerState->ControlsPrivateSlot(
			FlickGameState->CurrentTeam, FlickGameState->CurrentTeamPlayerSlot)
		: LocalPlayerState->GetTeam() == FlickGameState->CurrentTeam
			&& LocalPlayerState->GetTeamPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot;
}

void AFlickPlayerController::SubmitLocalPuckSkins()
{
	if (!IsLocalController()) return;
	ServerSetPuckSkins(FlickCosmeticCatalog::LoadPuckSkins());
	SubmitLocalPuckEffects();
	bPuckSkinsSubmitted = GetPlayerState<AFlickPlayerState>() != nullptr;
}

void AFlickPlayerController::SubmitLocalPuckEffects()
{
	if (IsLocalController()) ServerSetPuckEffects(FlickCosmeticCatalog::LoadPuckEffects());
}

void AFlickPlayerController::ServerSetPuckEffects_Implementation(const TArray<int32>& Effects)
{
	AFlickPlayerState* State = GetPlayerState<AFlickPlayerState>();
	if (!State || !State->SetPuckEffects(Effects)) return;
	for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
	{
		if (State->ControlsPrivateSlot(It->GetTeam(), It->GetOwningPlayerSlot())
			|| (State->GetTeam() == It->GetTeam() && State->GetTeamPlayerSlot() == It->GetOwningPlayerSlot()))
		{
			It->SetPuckEffects(Effects);
		}
	}
}

void AFlickPlayerController::ServerSetPuckSkins_Implementation(const TArray<int32>& Skins)
{
	AFlickPlayerState* State = GetPlayerState<AFlickPlayerState>();
	if (!State || !State->SetPuckSkins(Skins)) return;
	for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
	{
		if (State->ControlsPrivateSlot(It->GetTeam(), It->GetOwningPlayerSlot())
			|| (State->GetTeam() == It->GetTeam() && State->GetTeamPlayerSlot() == It->GetOwningPlayerSlot()))
		{
			It->SetPuckSkin(State->GetPuckSkin(It->GetArchetype()));
		}
	}
}

bool AFlickPlayerController::IsGameplayActive() const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing;
	}
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState && FlickGameState->IsGameplayActive();
}

float AFlickPlayerController::GetArenaSurfaceZ() const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->GetArenaSurfaceZ();
	}
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->ArenaSurfaceZ : 250.0f;
}

float AFlickPlayerController::GetMaxDragDistance() const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->GetMaxDragDistance();
	}
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->MaxDragDistance : 280.0f;
}

float AFlickPlayerController::GetMinDragDistance() const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->GetMinDragDistance();
	}
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->MinDragDistance : 15.0f;
}

float AFlickPlayerController::GetPowerExponent() const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->GetPowerExponent();
	}
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->PowerExponent : 1.2f;
}

void AFlickPlayerController::SubmitLaunch(
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->TryLaunchPieceForController(this, Piece, Direction, NormalizedPower);
		return;
	}
	ServerTryLaunchPiece(Piece ? Piece->GetPieceId() : INDEX_NONE, Direction.GetSafeNormal(), NormalizedPower);
}

void AFlickPlayerController::RequestRematch(const bool bChangeLineup)
{
 if (AFlickGameMode* Mode = GetFlickGameMode()) Mode->RequestRematch(this, bChangeLineup);
 else ServerRequestRematch(bChangeLineup);
}

void AFlickPlayerController::ServerRequestRematch_Implementation(const bool bChangeLineup)
{
 if (AFlickGameMode* Mode = GetFlickGameMode()) Mode->RequestRematch(this, bChangeLineup);
}

void AFlickPlayerController::RequestRestartMatch()
{
	if (!IsGameplayActive())
	{
		return;
	}
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		if (GetFlickGameState() && GetFlickGameState()->bSeriesComplete) FlickGameMode->RequestRematch(this, false);
		else FlickGameMode->RestartMatch();
	}
	else
	{
		ServerRequestRestartMatch();
	}
}

void AFlickPlayerController::RequestNextRound()
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || FlickGameState->MatchPhase != EFlickMatchPhase::RoundOver
		|| FlickGameState->bSeriesComplete)
	{
		return;
	}
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->StartNextRound();
	}
	else
	{
		ServerRequestNextRound();
	}
}

void AFlickPlayerController::ToggleLobbyReady()
{
	const AFlickPlayerState* FlickPlayerState = GetPlayerState<AFlickPlayerState>();
	if (!FlickPlayerState)
	{
		return;
	}
	const bool bNewReady = !FlickPlayerState->IsLobbyReady();
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetLobbyReady(this, bNewReady);
	}
	else
	{
		ServerSetLobbyReady(bNewReady);
	}
}

void AFlickPlayerController::RequestStartNetworkMatch()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->StartNetworkMatch(this);
	}
	else
	{
		ServerRequestStartNetworkMatch();
	}
}

void AFlickPlayerController::RequestSelectClass(const EFlickLineupPreset Preset)
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (FlickGameState && (FlickGameState->bNetworkClassSelectionActive || FlickGameState->bPrivateMatchActive))
	{
		TArray<EFlickPieceArchetype> Lineup;
		if (const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>())
		{
			for (int32 PieceSlot = 0; PieceSlot < 4; ++PieceSlot)
			{
				Lineup.Add(Instance->GetClassLoadoutPiece(Preset, PieceSlot));
			}
		}
		if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
		{
			FlickGameMode->SetNetworkPlayerClass(this, Preset, Lineup);
		}
		else
		{
			ServerSelectNetworkClass(Preset, Lineup);
		}
		return;
	}
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SelectPlayerClass(EFlickTeam::Player1, 0, Preset);
	}
}

void AFlickPlayerController::RequestConfirmClass()
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (FlickGameState && (FlickGameState->bNetworkClassSelectionActive
		|| FlickGameState->bPrivateMatchAssignmentActive))
	{
		if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
		{
			FlickGameMode->ConfirmNetworkPlayerClass(this);
		}
		else
		{
			ServerConfirmNetworkClass();
		}
		return;
	}
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->ConfirmClassSelection();
	}
}

void AFlickPlayerController::RequestTogglePrivateMatchSlot(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	bPrivateTeamMenuOpen = false;
	bPrivateSpectateChosen = false;
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->TogglePrivateMatchSlot(this, Team, PlayerSlot);
	}
	else
	{
		ServerTogglePrivateMatchSlot(Team, PlayerSlot);
	}
}

void AFlickPlayerController::RequestPrivateMatchSpectate()
{
	bPrivateTeamMenuOpen = false;
	bPrivateSpectateChosen = true;
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetPrivateMatchSpectating(this);
	}
	else
	{
		ServerSetPrivateMatchSpectating();
	}
	// On remote clients the role update may arrive later; the component starts
	// following once the chosen spectator role has replicated.
}

bool AFlickPlayerController::ShouldShowPrivateTeamMenu() const
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State || !State->bPrivateMatchActive) return false;
	if (State->bSeriesComplete && State->MatchPhase == EFlickMatchPhase::RoundOver) return false;
	const AFlickPlayerState* LocalState = GetPlayerState<AFlickPlayerState>();
	return bPrivateTeamMenuOpen || (LocalState && !LocalState->HasChosenPrivateRole());
}

void AFlickPlayerController::CyclePrivateSpectatorPlayer(const int32 Direction)
{
	if (!PrivateSpectator || !IsPrivateMatchSpectator() || IsCinematicReplayPresentationActive()) return;
	PrivateSpectator->CycleTarget(Direction);
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		bPrivateSpectateChosen = true;
		CameraPawn->SetFreeCameraEnabled(false);
		SetFreeCameraInputMode(false);
		bPrivateTeamMenuOpen = false;
	}
}

FString AFlickPlayerController::GetPrivateSpectatorTargetName() const
{
	return PrivateSpectator ? PrivateSpectator->GetTargetName() : TEXT("SELECT PLAYER OR BOT");
}

bool AFlickPlayerController::IsPrivateMatchSpectator() const
{
	return PrivateSpectator && PrivateSpectator->IsSpectator();
}

bool AFlickPlayerController::IsFollowingPrivatePlayer() const
{
	return PrivateSpectator && PrivateSpectator->IsFollowing();
}

EFlickTeam AFlickPlayerController::GetPrivateSpectatorTargetTeam() const
{
	return PrivateSpectator ? PrivateSpectator->GetTargetTeam() : EFlickTeam::None;
}

void AFlickPlayerController::HandleSpectatorPreviousPressed()
{
	if (IsPrivateMatchSpectator() && !ShouldShowPrivateTeamMenu() && !IsScoreboardVisible()) CyclePrivateSpectatorPlayer(-1);
}

void AFlickPlayerController::HandleSpectatorNextPressed()
{
	if (IsPrivateMatchSpectator() && !ShouldShowPrivateTeamMenu() && !IsScoreboardVisible()) CyclePrivateSpectatorPlayer(1);
}

void AFlickPlayerController::TogglePrivateSpectatorFreeCamera()
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State || !State->bPrivateMatchActive || State->bSeriesComplete || GetLocalTeam() != EFlickTeam::None) return;
	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		bPrivateSpectateChosen = true;
		const bool bEnable = !CameraPawn->IsFreeCameraEnabled();
		if (PrivateSpectator && bEnable) PrivateSpectator->StopFollowing();
		CameraPawn->SetFreeCameraEnabled(bEnable);
		SetFreeCameraInputMode(bEnable);
		bPrivateTeamMenuOpen = false;
		if (!bEnable && PrivateSpectator) PrivateSpectator->CycleTarget(0);
	}
}

void AFlickPlayerController::LeaveNetworkSession()
{
	ClearAiming();
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->CancelNetworkLobby();
		return;
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickSessionSubsystem* Sessions = GameInstance->GetSubsystem<UFlickSessionSubsystem>())
		{
			if (Sessions->IsPartySession())
			{
				Sessions->LeaveSession(true);
				return;
			}
			if (Sessions->HasPersistentPartyIdentity())
			{
				Sessions->RestorePersistentParty(Sessions->IsPersistentPartyLeader());
				return;
			}
			Sessions->LeaveSession(true);
			return;
		}
	}
	ClientTravel(TEXT("/Engine/Maps/Templates/OpenWorld"), ETravelType::TRAVEL_Absolute);
}

void AFlickPlayerController::ReturnToFrontendFromServer(const bool bClearPartyIdentity)
{
	ClientReturnToFrontend(bClearPartyIdentity);
}

void AFlickPlayerController::SetPersistentPartyIdentityFromServer(
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bLeader)
{
	ClientSetPersistentPartyIdentity(PartyId, PartySlot, PartySize, bLeader);
}

void AFlickPlayerController::BeginPartyMatchMigrationFromServer(
	const FString& TargetSessionId,
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bLeader)
{
	ClientBeginPartyMatchMigration(TargetSessionId, PartyId, PartySlot, PartySize, bLeader);
}

void AFlickPlayerController::BeginPartyRestoreFromServer(const bool bLeader)
{
	ClientBeginPartyRestore(bLeader);
}

void AFlickPlayerController::RecordRecentPlayer(const FString& UserId, const FString& DisplayName)
{
	ClientRecordRecentPlayer(UserId, DisplayName);
}

void AFlickPlayerController::BeginRankedMatchFromServer(
	const FString& MatchId,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 OpponentRating)
{
	ClientBeginRankedMatch(MatchId, Variant, PlayersPerTeam, OpponentRating);
}

void AFlickPlayerController::RequestRankedAuthenticationFromServer(const FString& TicketType)
{
	ClientRequestRankedAuthentication(TicketType);
}

void AFlickPlayerController::RequestCoordinatorAuthenticationFromServer(const FString& TicketType)
{
	ClientRequestCoordinatorAuthentication(TicketType);
}

void AFlickPlayerController::TravelToCoordinatorMatchFromServer(
	const FString& MatchId,
	const FString& ServerId,
	const FString& Address,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const bool bRanked,
	const int64 ExpiresUnixTime,
	const FString& AccountId,
	const FString& ReservationToken,
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bPartyLeader)
{
	ClientTravelToCoordinatorMatch(
		MatchId,
		ServerId,
		Address,
		Variant,
		PlayersPerTeam,
		bRanked,
		ExpiresUnixTime,
		AccountId,
		ReservationToken,
		Team,
		PlayerSlot,
		PartyId,
		PartySlot,
		PartySize,
		bPartyLeader);
}

void AFlickPlayerController::ApplyTrustedRankedProgressFromServer(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const FFlickRankProgress& Progress)
{
	ClientApplyTrustedRankedProgress(
		Variant,
		PlayersPerTeam,
		Progress.Rating,
		Progress.MatchesPlayed,
		Progress.Wins,
		Progress.Losses,
		Progress.Draws);
}

void AFlickPlayerController::ApplyTrustedRankedUpdateFromServer(const FFlickRatingUpdate& Update)
{
	ClientApplyTrustedRankedUpdate(
		Update.MatchId,
		Update.Variant,
		Update.PlayersPerTeam,
		Update.OldRating,
		Update.NewRating,
		Update.MatchesPlayed,
		static_cast<uint8>(Update.OldTier),
		static_cast<uint8>(Update.NewTier),
		Update.OldDivision,
		Update.NewDivision,
		Update.bForfeit);
}

void AFlickPlayerController::UpdateLocalCameraOrbit(const float DeltaSeconds)
{
	if (DeltaSeconds <= 0.0f || !IsGameplayActive() || IsFollowingPrivatePlayer())
	{
		return;
	}

	const AFlickGameMode* FlickGameMode = GetFlickGameMode();
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState
		|| (FlickGameMode && FlickGameMode->IsCinematicReplayActive())
		|| (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning
			&& FlickGameState->MatchPhase != EFlickMatchPhase::ResolvingPhysics))
	{
		return;
	}

	const float Direction =
		(IsInputKeyDown(FlickControlBindings::GetKey(TEXT("OrbitLeft"))) ? 1.0f : 0.0f)
		- (IsInputKeyDown(FlickControlBindings::GetKey(TEXT("OrbitRight"))) ? 1.0f : 0.0f);

	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
		CameraPawn && !FMath::IsNearlyZero(Direction))
	{
		const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>();
		CameraPawn->RotateGameplayOrbit(
			Direction,
			DeltaSeconds,
			Instance ? Instance->GetGameplayCameraSensitivity() : 0.35f);
		ClearHoveredPiece();
	}
}

void AFlickPlayerController::AdjustLocalCameraElevation(const int32 Direction)
{
	if (Direction == 0 || !IsGameplayActive() || IsFollowingPrivatePlayer())
	{
		return;
	}

	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		ClearHoveredPiece();
		const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>();
		CameraPawn->AdjustGameplayElevationFine(
			Direction,
			Instance ? Instance->GetGameplayCameraSensitivity() : 0.35f);
	}
}

void AFlickPlayerController::ServerTryLaunchPiece_Implementation(
	const int32 PieceId,
	const FVector_NetQuantizeNormal Direction,
	const float NormalizedPower)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->TryLaunchPieceByIdForController(this, PieceId, Direction, NormalizedPower);
	}
}

void AFlickPlayerController::ServerRequestRestartMatch_Implementation()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		if (GetFlickGameState() && GetFlickGameState()->bSeriesComplete) FlickGameMode->RequestRematch(this, false);
		else if (GetNetMode() == NM_Standalone) FlickGameMode->RestartMatch();
	}
}

void AFlickPlayerController::ServerRequestNextRound_Implementation()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->StartNextRound();
	}
}

void AFlickPlayerController::ServerSetLobbyReady_Implementation(const bool bReady)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetLobbyReady(this, bReady);
	}
}

void AFlickPlayerController::ServerRequestStartNetworkMatch_Implementation()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->StartNetworkMatch(this);
	}
}

void AFlickPlayerController::ServerSelectNetworkClass_Implementation(
	const EFlickLineupPreset Preset,
	const TArray<EFlickPieceArchetype>& Lineup)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetNetworkPlayerClass(this, Preset, Lineup);
	}
}

void AFlickPlayerController::ServerConfirmNetworkClass_Implementation()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->ConfirmNetworkPlayerClass(this);
	}
}

void AFlickPlayerController::ServerTogglePrivateMatchSlot_Implementation(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->TogglePrivateMatchSlot(this, Team, PlayerSlot);
	}
}

void AFlickPlayerController::ServerSetPrivateMatchSpectating_Implementation()
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetPrivateMatchSpectating(this);
	}
}

void AFlickPlayerController::ServerSubmitRankedAuthentication_Implementation(const FString& SteamAuthTicket)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SubmitRankedAuthentication(this, SteamAuthTicket.Left(8192));
	}
}

void AFlickPlayerController::ServerSubmitCoordinatorAuthentication_Implementation(const FString& SteamAuthTicket)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SubmitCoordinatorAuthentication(this, SteamAuthTicket.Left(8192));
	}
}

void AFlickPlayerController::ClientReturnToFrontend_Implementation(const bool bClearPartyIdentity)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickSessionSubsystem* Sessions = GameInstance->GetSubsystem<UFlickSessionSubsystem>())
		{
			if (bClearPartyIdentity)
			{
				Sessions->ClearPersistentPartyIdentity();
			}
			Sessions->LeaveSession(true);
			return;
		}
	}
	ClientTravel(TEXT("/Engine/Maps/Templates/OpenWorld"), ETravelType::TRAVEL_Absolute);
}

void AFlickPlayerController::ClientSetPersistentPartyIdentity_Implementation(
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bLeader)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickSessionSubsystem* Sessions = GameInstance->GetSubsystem<UFlickSessionSubsystem>())
		{
			Sessions->SetPersistentPartyIdentity(PartyId, PartySlot, PartySize, bLeader);
		}
	}
}

void AFlickPlayerController::ClientBeginPartyMatchMigration_Implementation(
	const FString& TargetSessionId,
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bLeader)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickSessionSubsystem* Sessions = GameInstance->GetSubsystem<UFlickSessionSubsystem>())
		{
			Sessions->BeginPartyMatchMigration(TargetSessionId, PartyId, PartySlot, PartySize, bLeader);
		}
	}
}

void AFlickPlayerController::ClientBeginPartyRestore_Implementation(const bool bLeader)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickSessionSubsystem* Sessions = GameInstance->GetSubsystem<UFlickSessionSubsystem>())
		{
			Sessions->RestorePersistentParty(bLeader);
		}
	}
}

void AFlickPlayerController::ClientRecordRecentPlayer_Implementation(
	const FString& UserId,
	const FString& DisplayName)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickSessionSubsystem* Sessions = GameInstance->GetSubsystem<UFlickSessionSubsystem>())
		{
			Sessions->RecordRecentPlayer(UserId, DisplayName);
		}
	}
}

void AFlickPlayerController::ClientBeginRankedMatch_Implementation(
	const FString& MatchId,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 OpponentRating)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickRankingSubsystem* Ranking = GameInstance->GetSubsystem<UFlickRankingSubsystem>())
		{
			Ranking->BeginRankedMatch(MatchId, Variant, PlayersPerTeam, OpponentRating);
		}
	}
}

void AFlickPlayerController::ClientRequestRankedAuthentication_Implementation(const FString& TicketType)
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid())
	{
		ServerSubmitRankedAuthentication(FString());
		return;
	}
	Identity->GetLinkedAccountAuthToken(
		0,
		TicketType,
		IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateWeakLambda(
			this,
			[this](const int32 LocalUserNum, const bool bSuccess, const FExternalAuthToken& Token)
			{
				FString SerializedTicket = Token.TokenString;
				if (SerializedTicket.IsEmpty() && Token.TokenData.Num() > 0)
				{
					SerializedTicket = FBase64::Encode(Token.TokenData);
				}
				if (bSuccess && !SerializedTicket.IsEmpty())
				{
					UE_LOG(LogFlick, Log, TEXT("RANKED_STEAM_TICKET: local_user=%d available=1"), LocalUserNum);
				}
				else
				{
					UE_LOG(LogFlick, Warning, TEXT("RANKED_STEAM_TICKET: local_user=%d available=0"), LocalUserNum);
				}
				ServerSubmitRankedAuthentication(bSuccess ? SerializedTicket : FString());
			}));
}

void AFlickPlayerController::ClientRequestCoordinatorAuthentication_Implementation(const FString& TicketType)
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	if (!Identity.IsValid())
	{
		ServerSubmitCoordinatorAuthentication(FString());
		return;
	}
	Identity->GetLinkedAccountAuthToken(
		0,
		TicketType,
		IOnlineIdentity::FOnGetLinkedAccountAuthTokenCompleteDelegate::CreateWeakLambda(
			this,
			[this](const int32 LocalUserNum, const bool bSuccess, const FExternalAuthToken& Token)
			{
				FString SerializedTicket = Token.TokenString;
				if (SerializedTicket.IsEmpty() && Token.TokenData.Num() > 0)
				{
					SerializedTicket = FBase64::Encode(Token.TokenData);
				}
				if (bSuccess && !SerializedTicket.IsEmpty())
				{
					UE_LOG(LogFlick, Log, TEXT("COORDINATOR_STEAM_TICKET: local_user=%d available=1"), LocalUserNum);
				}
				else
				{
					UE_LOG(LogFlick, Warning, TEXT("COORDINATOR_STEAM_TICKET: local_user=%d available=0"), LocalUserNum);
				}
				ServerSubmitCoordinatorAuthentication(bSuccess ? SerializedTicket : FString());
			}));
}

void AFlickPlayerController::ClientTravelToCoordinatorMatch_Implementation(
	const FString& MatchId,
	const FString& ServerId,
	const FString& Address,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const bool bRanked,
	const int64 ExpiresUnixTime,
	const FString& AccountId,
	const FString& ReservationToken,
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bPartyLeader)
{
	UGameInstance* GameInstance = GetGameInstance();
	UFlickMatchmakingCoordinatorSubsystem* Coordinator = GameInstance
		? GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>()
		: nullptr;
	if (!Coordinator)
	{
		return;
	}
	FFlickCoordinatorAllocation Allocation;
	Allocation.MatchId = MatchId;
	Allocation.ServerId = ServerId;
	Allocation.Address = Address;
	Allocation.Variant = Variant;
	Allocation.PlayersPerTeam = FMath::Clamp(PlayersPerTeam, 1, 3);
	Allocation.bRanked = bRanked;
	Allocation.ExpiresUnixTime = ExpiresUnixTime;
	FFlickCoordinatorReservation Reservation;
	Reservation.AccountId = AccountId;
	Reservation.Token = ReservationToken;
	Reservation.Team = Team;
	Reservation.PlayerSlot = FMath::Clamp(PlayerSlot, 0, 2);
	Allocation.Reservations.Add(Reservation);
	Coordinator->AdoptLocalReservation(
		Allocation,
		Reservation,
		PartyId,
		PartySlot,
		PartySize,
		bPartyLeader);
	Coordinator->TravelToAllocatedMatch(this);
}

void AFlickPlayerController::ClientApplyTrustedRankedProgress_Implementation(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 Rating,
	const int32 MatchesPlayed,
	const int32 Wins,
	const int32 Losses,
	const int32 Draws)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickRankingSubsystem* Ranking = GameInstance->GetSubsystem<UFlickRankingSubsystem>())
		{
			FFlickRankProgress Progress;
			Progress.Rating = FMath::Clamp(Rating, 0, 3000);
			Progress.MatchesPlayed = FMath::Max(0, MatchesPlayed);
			Progress.Wins = FMath::Max(0, Wins);
			Progress.Losses = FMath::Max(0, Losses);
			Progress.Draws = FMath::Max(0, Draws);
			Ranking->ApplyTrustedProgress(Variant, PlayersPerTeam, Progress);
		}
	}
}

void AFlickPlayerController::ClientApplyTrustedRankedUpdate_Implementation(
	const FString& MatchId,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 OldRating,
	const int32 NewRating,
	const int32 MatchesPlayed,
	const uint8 OldTier,
	const uint8 NewTier,
	const int32 OldDivision,
	const int32 NewDivision,
	const bool bForfeit)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickRankingSubsystem* Ranking = GameInstance->GetSubsystem<UFlickRankingSubsystem>())
		{
			FFlickRatingUpdate Update;
			Update.MatchId = MatchId;
			Update.Variant = Variant;
			Update.PlayersPerTeam = FMath::Clamp(PlayersPerTeam, 1, 3);
			Update.OldRating = FMath::Clamp(OldRating, 0, 3000);
			Update.NewRating = FMath::Clamp(NewRating, 0, 3000);
			Update.RatingDelta = Update.NewRating - Update.OldRating;
			Update.MatchesPlayed = FMath::Max(0, MatchesPlayed);
			Update.OldTier = static_cast<EFlickRankTier>(FMath::Clamp<int32>(
				OldTier,
				static_cast<int32>(EFlickRankTier::Unranked),
				static_cast<int32>(EFlickRankTier::GrandChampion)));
			Update.NewTier = static_cast<EFlickRankTier>(FMath::Clamp<int32>(
				NewTier,
				static_cast<int32>(EFlickRankTier::Unranked),
				static_cast<int32>(EFlickRankTier::GrandChampion)));
			Update.OldDivision = FMath::Clamp(OldDivision, 0, 4);
			Update.NewDivision = FMath::Clamp(NewDivision, 0, 4);
			Update.bForfeit = bForfeit;
			Update.bAccepted = true;
			Ranking->ApplyTrustedUpdate(Update);
		}
	}
}

void AFlickPlayerController::ClientSetGameplayCameraTeam_Implementation(
	const EFlickTeam Team,
	const bool bSnap)
{
	if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(GetPawn()); Camera && Team != EFlickTeam::None)
	{
		const int32 ViewIndex = Team == EFlickTeam::Player2 ? 2 : 0;
		if (bSnap) Camera->ResetRoundView(ViewIndex);
		else Camera->SetGameplayViewIndex(ViewIndex, false);
	}
}

void AFlickPlayerController::ClientBeginCinematicReplay_Implementation(
	const FVector InitialFocus,
	const EFlickTeam ShootingTeam)
{
	bCinematicReplayPresentationActive = true;
	CinematicReplayPresentationProgress = 0.0f;
	CinematicReplayPresentationPullbackAlpha = 0.0f;
	if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(GetPawn()))
	{
		Camera->BeginCinematicReplay(InitialFocus, ShootingTeam);
	}
}

void AFlickPlayerController::ClientUpdateCinematicReplay_Implementation(
	const FVector Focus,
	const float NormalizedProgress,
	const float PullbackAlpha)
{
	bCinematicReplayPresentationActive = true;
	CinematicReplayPresentationProgress = FMath::Clamp(NormalizedProgress, 0.0f, 1.0f);
	CinematicReplayPresentationPullbackAlpha = FMath::Clamp(PullbackAlpha, 0.0f, 1.0f);
	if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(GetPawn()))
	{
		Camera->UpdateCinematicReplay(Focus, NormalizedProgress, PullbackAlpha);
	}
}

void AFlickPlayerController::ClientEndCinematicReplay_Implementation()
{
	bCinematicReplayPresentationActive = false;
	CinematicReplayPresentationProgress = 0.0f;
	CinematicReplayPresentationPullbackAlpha = 0.0f;
	if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(GetPawn()))
	{
		Camera->EndCinematicReplay();
	}
}

AFlickPiece* AFlickPlayerController::FindPieceUnderCursor() const
{
#if !UE_BUILD_SHIPPING
	// Exercise the real inspection/filter/UI path in offscreen presentation QA.
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickPuckHoverPreview")))
	{
		for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
			if (It->IsActive() && It->GetTeam() != GetLocalTeam()) return *It;
	}
#endif
	FHitResult HitResult;
	if (!GetHitResultUnderCursorByChannel(
		UEngineTypes::ConvertToTraceType(ECC_Visibility),
		false,
		HitResult))
	{
		return nullptr;
	}

	if (AFlickPiece* HitPiece = Cast<AFlickPiece>(HitResult.GetActor()))
	{
		return HitPiece;
	}

	return HitResult.GetComponent()
		? Cast<AFlickPiece>(HitResult.GetComponent()->GetOwner())
		: nullptr;
}

AFlickGameMode* AFlickPlayerController::GetFlickGameMode() const
{
	return GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr;
}

AFlickGameState* AFlickPlayerController::GetFlickGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
}
