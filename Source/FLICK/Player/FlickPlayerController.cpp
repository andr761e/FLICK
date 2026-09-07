#include "Player/FlickPlayerController.h"

#include "Core/FlickLog.h"
#include "DrawDebugHelpers.h"
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
	bShouldPerformFullTickWhenPaused = true;
}

void AFlickPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bNetworkAutoShotRequested = FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkAutoShot"));
	bNetworkAutoReadyRequested = FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkAutoReady"));

}

void AFlickPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AFlickPlayerController::HandlePrimaryPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AFlickPlayerController::HandlePrimaryReleased).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AFlickPlayerController::HandleSecondaryPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AFlickPlayerController::HandleCancelPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AFlickPlayerController::HandleRestartPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AFlickPlayerController::HandleTrainingEditorTogglePressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AFlickPlayerController::HandleTrainingOwnPuckPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AFlickPlayerController::HandleTrainingTargetPuckPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AFlickPlayerController::HandleTrainingClearPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &AFlickPlayerController::HandleTrainingRemovePressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AFlickPlayerController::HandleCameraElevationUpPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AFlickPlayerController::HandleCameraElevationDownPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AFlickPlayerController::HandleCameraResetPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AFlickPlayerController::HandleScoreboardPressed).bExecuteWhenPaused = true;
	InputComponent->BindKey(EKeys::Tab, IE_Released, this, &AFlickPlayerController::HandleScoreboardReleased).bExecuteWhenPaused = true;
}

void AFlickPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	const AFlickGameState* NetworkState = GetFlickGameState();
	UpdateCareerStatsTracking(NetworkState);
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
	UpdateLocalCameraOrbit(DeltaTime);

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
			CameraPawn && LocalTeam != EFlickTeam::None)
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

	if (bAimingShot)
	{
		UpdateAimFromCursor();
		DrawAimDebug();
		CurrentMouseCursor = EMouseCursor::Crosshairs;
	}
	else
	{
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
		FlickGameState->bRankedMatch);
	bCareerStatsRecordedForCurrentSeries = true;
}

void AFlickPlayerController::ClearAiming()
{
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
	if (TrainingDraggedPiece)
	{
		if (FlickGameMode)
		{
			FlickGameMode->FinishTrainingPuckMove(TrainingDraggedPiece);
		}
		TrainingDraggedPiece = nullptr;
	}
	if (!FlickGameMode)
	{
		ClearAiming();
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

void AFlickPlayerController::HandleTrainingEditorTogglePressed()
{
	AFlickGameMode* FlickGameMode = GetFlickGameMode();
	if (!FlickGameMode || !FlickGameMode->IsTrainingMode()
		|| FlickGameMode->GetFrontendScreen() != EFlickFrontendScreen::Playing)
	{
		return;
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
	AFlickPiece* NewHoveredPiece = FindPieceUnderCursor();
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

bool AFlickPlayerController::CanSelectPieceLocally(const AFlickPiece* Piece) const
{
	if (const AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		return FlickGameMode->CanSelectPieceForController(this, Piece);
	}

	const AFlickGameState* FlickGameState = GetFlickGameState();
	const AFlickPlayerState* LocalPlayerState = GetPlayerState<AFlickPlayerState>();
	const bool bControlsPrivateTurn = FlickGameState
		&& LocalPlayerState
		&& LocalPlayerState->ControlsPrivateSlot(
			FlickGameState->CurrentTeam,
			FlickGameState->CurrentTeamPlayerSlot);
	return Piece
		&& FlickGameState
		&& LocalPlayerState
		&& FlickGameState->IsGameplayActive()
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
		&& (bControlsPrivateTurn
			|| (GetLocalTeam() == FlickGameState->CurrentTeam
				&& LocalPlayerState->GetTeamPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot))
		&& Piece->IsSelectableBy(GetLocalTeam())
		&& (FlickGameState->ActiveMatchVariant == EFlickMatchVariant::Bob
			|| (bControlsPrivateTurn
				? Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot
				: Piece->GetOwningPlayerSlot() == LocalPlayerState->GetTeamPlayerSlot()))
		&& (FlickGameState->ActiveMatchVariant != EFlickMatchVariant::Bob || Piece->IsBobStriker());
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

void AFlickPlayerController::RequestRestartMatch()
{
	if (!IsGameplayActive())
	{
		return;
	}
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->RestartMatch();
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
	if (FlickGameState && FlickGameState->bNetworkClassSelectionActive)
	{
		if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
		{
			FlickGameMode->SetNetworkPlayerClass(this, Preset);
		}
		else
		{
			ServerSelectNetworkClass(Preset);
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
	if (FlickGameState && FlickGameState->bNetworkClassSelectionActive)
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
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetPrivateMatchSpectating(this);
	}
	else
	{
		ServerSetPrivateMatchSpectating();
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
	if (DeltaSeconds <= 0.0f || !IsGameplayActive())
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
		(IsInputKeyDown(EKeys::Q) ? 1.0f : 0.0f)
		- (IsInputKeyDown(EKeys::E) ? 1.0f : 0.0f);

	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn());
		CameraPawn && !FMath::IsNearlyZero(Direction))
	{
		CameraPawn->RotateGameplayOrbit(Direction, DeltaSeconds);
		ClearHoveredPiece();
	}
}

void AFlickPlayerController::AdjustLocalCameraElevation(const int32 Direction)
{
	if (Direction == 0 || !IsGameplayActive())
	{
		return;
	}

	if (AFlickCameraPawn* CameraPawn = Cast<AFlickCameraPawn>(GetPawn()))
	{
		ClearHoveredPiece();
		CameraPawn->AdjustGameplayElevation(Direction);
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
		FlickGameMode->RestartMatch();
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

void AFlickPlayerController::ServerSelectNetworkClass_Implementation(const EFlickLineupPreset Preset)
{
	if (AFlickGameMode* FlickGameMode = GetFlickGameMode())
	{
		FlickGameMode->SetNetworkPlayerClass(this, Preset);
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

AFlickPiece* AFlickPlayerController::FindPieceUnderCursor() const
{
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
