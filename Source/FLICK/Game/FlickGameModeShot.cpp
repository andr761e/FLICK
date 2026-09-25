#include "Game/FlickGameModePrivate.h"

using namespace FlickGameModePrivate;

void AFlickGameMode::ResetTrainingBotThinking()
{
	TrainingBotThinkElapsed = 0.0f;
	bTrainingBotThinkingAnnounced = false;
}

void AFlickGameMode::ResetShotClock()
{
	bShotClockTrackingActive = false;
	ShotClockTrackedTeam = EFlickTeam::None;
	ShotClockTrackedPlayerSlot = INDEX_NONE;
	ShotClockTrackedPhase = EFlickMatchPhase::WaitingToStart;
	if (AFlickGameState* FlickGameState = GetFlickGameState(); FlickGameState && FlickGameState->bShotClockActive)
	{
		FlickGameState->SetShotClockState(false, ShotTimeLimit);
	}
}

float AFlickGameMode::GetShotClockFraction(const EFlickTeam Team) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->GetShotClockFraction(Team) : 1.0f;
}

float AFlickGameMode::GetRoundAdvanceTimeRemaining() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->GetRoundAdvanceTimeRemaining() : 0.0f;
}

void AFlickGameMode::UpdateInitialClassSelectionTimer(const float DeltaSeconds)
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bShouldRun = FrontendScreen == EFlickFrontendScreen::ClassSelect
		&& !bClassSelectionForNextRound
		&& !(FlickGameState && FlickGameState->bNetworkClassSelectionActive);
	if (!bShouldRun)
	{
		bInitialClassSelectionTimerActive = false;
		return;
	}
	if (!bInitialClassSelectionTimerActive)
	{
		InitialClassSelectionTimeRemaining = FMath::Max(3.0f, InitialClassSelectionTimeLimit);
		bInitialClassSelectionTimerActive = true;
	}
	InitialClassSelectionTimeRemaining = FMath::Max(
		0.0f,
		InitialClassSelectionTimeRemaining - FMath::Max(0.0f, DeltaSeconds));
	if (InitialClassSelectionTimeRemaining <= 0.0f)
	{
		bInitialClassSelectionTimerActive = false;
		ConfirmClassSelection();
	}
}

void AFlickGameMode::UpdateNetworkClassSelectionTimer()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !FlickGameState->bNetworkClassSelectionActive)
	{
		return;
	}
	if (AreNetworkClassesConfirmed()
		|| FlickGameState->GetNetworkClassSelectionTimeRemaining() <= 0.0f)
	{
		FinalizeNetworkClassSelection();
	}
}

void AFlickGameMode::UpdateRoundAdvanceTimer()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !FlickGameState->bRoundAdvanceTimerActive)
	{
		return;
	}
	if (FrontendScreen != EFlickFrontendScreen::Playing
		|| FlickGameState->MatchPhase != EFlickMatchPhase::RoundOver
		|| FlickGameState->bSeriesComplete)
	{
		FlickGameState->SetRoundAdvanceTimerState(false, RoundAdvanceTimeLimit);
		return;
	}
	if (FlickGameState->GetRoundAdvanceTimeRemaining() <= 0.0f)
	{
		FlickGameState->SetRoundAdvanceTimerState(false, RoundAdvanceTimeLimit);
		StartNextRound();
	}
}

void AFlickGameMode::UpdateShotClock()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bClockPhase = FlickGameState
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning);
	if (!FlickGameState || IsFreePlayTraining()
		|| FrontendScreen != EFlickFrontendScreen::Playing || !bClockPhase)
	{
		ResetShotClock();
		return;
	}

	const bool bTurnChanged = !bShotClockTrackingActive
		|| ShotClockTrackedTeam != FlickGameState->CurrentTeam
		|| ShotClockTrackedPlayerSlot != FlickGameState->CurrentTeamPlayerSlot
		|| ShotClockTrackedPhase != FlickGameState->MatchPhase;
	if (bTurnChanged)
	{
		bShotClockTrackingActive = true;
		ShotClockTrackedTeam = FlickGameState->CurrentTeam;
		ShotClockTrackedPlayerSlot = FlickGameState->CurrentTeamPlayerSlot;
		ShotClockTrackedPhase = FlickGameState->MatchPhase;
		FlickGameState->SetShotClockState(true, ShotTimeLimit);
	}

	if (FlickGameState->GetShotClockTimeRemaining() <= 0.0f)
	{
		if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
		{
			// Kickoff planning has one shared deadline. Commit a zero-power shot for
			// every player who has not submitted, then release all shots together.
			for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
			{
				for (int32 PlayerSlot = 0; PlayerSlot < CurrentPlayersPerTeam; ++PlayerSlot)
				{
					const bool bAlreadyLocked = LockedKickoffShots.ContainsByPredicate(
						[Team, PlayerSlot](const FFlickLockedKickoffShot& Shot)
						{
							return Shot.Team == Team && Shot.PlayerSlot == PlayerSlot;
						});
					if (bAlreadyLocked) continue;
					AFlickPiece* TimeoutPiece = nullptr;
					for (AFlickPiece* Candidate : Pieces)
					{
						if (Candidate && IsValid(Candidate) && Candidate->IsActive()
							&& Candidate->IsSelectableBy(Team) && Candidate->GetOwningPlayerSlot() == PlayerSlot)
						{
							TimeoutPiece = Candidate;
							break;
						}
					}
					if (TimeoutPiece)
					{
						FVector Direction = -TimeoutPiece->GetActorLocation(); Direction.Z = 0.0f;
						if (!Direction.Normalize()) Direction = FVector::ForwardVector;
						LockKickoffShot(TimeoutPiece, Direction, 0.0f, false);
					}
				}
			}
			if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning) ReleaseKickoffShots();
			ResetShotClock();
			return;
		}
		ExpireCurrentShot();
	}
}

void AFlickGameMode::ExpireCurrentShot()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsFreePlayTraining()
		|| FlickGameState->CurrentTeam == EFlickTeam::None
		|| (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning))
	{
		return;
	}

	ClearControllerAiming();
	const EFlickTeam ExpiredTeam = FlickGameState->CurrentTeam;
	const int32 ExpiredPlayerSlot = FlickGameState->CurrentTeamPlayerSlot;
	bool bTurnAdvanced = false;
	if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		AFlickPiece* WastedKickoffPiece = nullptr;
		for (AFlickPiece* Piece : Pieces)
		{
			if (Piece && IsValid(Piece) && Piece->IsActive()
				&& Piece->IsSelectableBy(ExpiredTeam)
				&& Piece->GetOwningPlayerSlot() == ExpiredPlayerSlot)
			{
				WastedKickoffPiece = Piece;
				break;
			}
		}
		if (WastedKickoffPiece)
		{
			FVector WastedDirection = -WastedKickoffPiece->GetActorLocation();
			WastedDirection.Z = 0.0f;
			if (!WastedDirection.Normalize())
			{
				WastedDirection = ExpiredTeam == EFlickTeam::Player1
					? FVector::ForwardVector : -FVector::ForwardVector;
			}
			bTurnAdvanced = LockKickoffShot(WastedKickoffPiece, WastedDirection, 0.0f, false);
		}
	}
	else
	{
		const EFlickTeam NextTeam = GetOpposingTeam(ExpiredTeam);
		FlickGameState->BeginShot(ExpiredTeam, INDEX_NONE, 0.0f);
		FlickGameState->RecordPlayerShot(ExpiredTeam, ExpiredPlayerSlot);
		AdvanceCompletedPlayerTurn(ExpiredTeam, ExpiredPlayerSlot);
		FlickGameState->SetCurrentTeam(NextTeam);
		ActivateNextPlayerForTeam(NextTeam);
		FlickGameState->AdvanceTurn();
		FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
		SetCameraViewForTeam(NextTeam);
		if (AudioDirector)
		{
			AudioDirector->PlayTurn(NextTeam);
		}
		bTurnAdvanced = true;
	}

	ResetShotClock();
	if (bTurnAdvanced)
	{
		ResetTrainingBotThinking();
		PushHudEvent(
			TEXT("SHOT CLOCK EXPIRED  |  TURN FORFEITED"),
			FLinearColor(1.0f, 0.34f, 0.12f, 1.0f),
			2.2f);
		UE_LOG(
			LogFlick,
			Log,
			TEXT("%s Player %d forfeited the shot because the 10-second clock expired"),
			*GetTeamDisplayName(ExpiredTeam),
			ExpiredPlayerSlot + 1);
	}
}

void AFlickGameMode::UpdateTrainingBot(const float DeltaSeconds)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	EFlickTeam BotTeam = EFlickTeam::None;
	int32 BotPlayerSlot = INDEX_NONE;
	if (FlickGameState && FrontendScreen == EFlickFrontendScreen::Playing)
	{
		const auto NeedsBot = [this, FlickGameState](const EFlickTeam Team, const int32 Slot)
		{
			const bool bBotSeat = IsTrainingBotMatch()
				? Team == EFlickTeam::Player2 && Slot == 0
				: bPrivateMatchActive && !GetPrivateSlotOwner(Team, Slot);
			return bBotSeat && (FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning
				|| !LockedKickoffShots.ContainsByPredicate([Team, Slot](const FFlickLockedKickoffShot& Shot)
			{
				return Shot.Team == Team && Shot.PlayerSlot == Slot;
			}));
		};
		if (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming)
		{
			BotTeam = FlickGameState->CurrentTeam;
			BotPlayerSlot = FlickGameState->CurrentTeamPlayerSlot;
			if (!NeedsBot(BotTeam, BotPlayerSlot)) BotTeam = EFlickTeam::None;
		}
		else if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
		{
			for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
			{
				for (int32 Slot = 0; Slot < CurrentPlayersPerTeam; ++Slot)
				{
					if (NeedsBot(Team, Slot))
					{
						BotTeam = Team;
						BotPlayerSlot = Slot;
						break;
					}
				}
				if (BotTeam != EFlickTeam::None) break;
			}
		}
	}
	const bool bBotCanPlan = BotTeam != EFlickTeam::None;
	if (!bBotCanPlan)
	{
		ResetTrainingBotThinking();
		return;
	}

	if (!bTrainingBotThinkingAnnounced)
	{
		bTrainingBotThinkingAnnounced = true;
		const float ThinkDelay = GetTrainingBotDifficultySettings().ThinkDelay;
		PushHudEvent(
			FString::Printf(TEXT("%s BOT IS LINING UP A SHOT"), *GetBotDifficultyLabel()),
			GetTeamColor(BotTeam),
			ThinkDelay);
	}
	const float ThinkDelay = GetTrainingBotDifficultySettings().ThinkDelay;
	TrainingBotThinkElapsed += DeltaSeconds;
	if (TrainingBotThinkElapsed < ThinkDelay)
	{
		return;
	}

	if (TryExecuteTrainingBotShot(BotTeam, BotPlayerSlot))
	{
		ResetTrainingBotThinking();
	}
	else
	{
		// A transient camera or state transition can briefly leave the board
		// unavailable. Retry shortly instead of stalling the match.
		TrainingBotThinkElapsed = FMath::Max(0.0f, ThinkDelay - 0.2f);
	}
}

const FFlickBotDifficultySettings& AFlickGameMode::GetTrainingBotDifficultySettings() const
{
	switch (GetBotDifficulty())
	{
	case EFlickBotDifficulty::Easy:
		return TrainingBotEasySettings;
	case EFlickBotDifficulty::Hard:
		return TrainingBotHardSettings;
	case EFlickBotDifficulty::Expert:
		return TrainingBotExpertSettings;
	case EFlickBotDifficulty::Normal:
	default:
		return TrainingBotNormalSettings;
	}
}

bool AFlickGameMode::TryExecuteTrainingBotShot(const EFlickTeam BotTeam, const int32 BotPlayerSlot)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if ((!IsTrainingBotMatch() && !bPrivateMatchActive) || !FlickGameState
		|| !((FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
				&& FlickGameState->CurrentTeam == BotTeam
				&& FlickGameState->CurrentTeamPlayerSlot == BotPlayerSlot)
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning))
	{
		return false;
	}

	TArray<FFlickBotPieceState> BotPieces;
	BotPieces.Reserve(Pieces.Num());
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive()
			|| (Piece->GetTeam() == BotTeam && Piece->GetOwningPlayerSlot() != BotPlayerSlot))
		{
			continue;
		}
		const FVector Location = Piece->GetActorLocation();
		BotPieces.Add({
			Piece->GetPieceId(),
			Piece->GetTeam(),
			FVector2D(Location.X, Location.Y),
			Piece->GetPieceRadius()});
	}

	const FFlickBotDifficultySettings& DifficultySettings = GetTrainingBotDifficultySettings();
	FFlickBotShotTuning BotTuning;
	BotTuning.ArenaRadius = ArenaRadius;
	BotTuning.MinimumPower = FMath::Min(DifficultySettings.MinimumPower, DifficultySettings.MaximumPower);
	BotTuning.MaximumPower = FMath::Max(DifficultySettings.MinimumPower, DifficultySettings.MaximumPower);
	BotTuning.AimErrorDegrees = FMath::Max(0.0f, DifficultySettings.AimErrorDegrees);
	BotTuning.PowerVariation = FMath::Max(0.0f, DifficultySettings.PowerVariation);
	BotTuning.DecisionNoise = FMath::Max(0.0f, DifficultySettings.DecisionNoise);
	BotTuning.DividerAwareness = FMath::Clamp(DifficultySettings.DividerAwareness, 0.0f, 1.0f);
	BotTuning.BankShotSkill = FMath::Clamp(DifficultySettings.BankShotSkill, 0.0f, 1.0f);
	if (bTestArenaMode && TestArenaActor && IsValid(TestArenaActor))
	{
		BotTuning.Dividers.Reserve(TestArenaActor->GetMechanismCount());
		for (int32 DividerIndex = 0; DividerIndex < TestArenaActor->GetMechanismCount(); ++DividerIndex)
		{
			const FVector DividerCenter = TestArenaActor->GetDividerWorldCenter(DividerIndex);
			const FVector SwitchCenter = TestArenaActor->GetSwitchWorldCenter(DividerIndex);
			FFlickBotDividerState& Divider = BotTuning.Dividers.AddDefaulted_GetRef();
			Divider.Center = FVector2D(DividerCenter.X, DividerCenter.Y);
			Divider.Tangent = TestArenaActor->GetDividerWorldTangent(DividerIndex);
			Divider.SwitchPosition = FVector2D(SwitchCenter.X, SwitchCenter.Y);
			Divider.HalfLength = TestArenaActor->GetDividerLength(DividerIndex) * 0.5f;
			Divider.HalfThickness = TestArenaActor->GetDividerCollisionThickness() * 0.5f;
			Divider.bRaised = TestArenaActor->IsDividerRaised(DividerIndex);
		}
	}
	FFlickBotShotPlan Plan;
	if (IsBobMode())
	{
		AFlickPiece* BobStriker = GetBobStriker(BotTeam);
		TArray<FVector2D> PocketPositions;
		if (BobArenaActor)
		{
			BotTuning.ArenaRadius = BobArenaActor->GetHalfExtent();
			PocketPositions.Reserve(4);
			for (int32 PocketIndex = 0; PocketIndex < 4; ++PocketIndex)
			{
				const FVector PocketLocation = BobArenaActor->GetPocketWorldLocation(PocketIndex);
				PocketPositions.Emplace(PocketLocation.X, PocketLocation.Y);
			}
		}
		Plan = FlickBotShotPlanner::PlanBobShot(
			BotPieces,
			BotTeam,
			BobStriker ? BobStriker->GetPieceId() : INDEX_NONE,
			PocketPositions,
			BotTuning,
			TrainingBotRandom);
	}
	else
	{
		Plan = FlickBotShotPlanner::PlanShot(
			BotPieces,
			BotTeam,
			BotTuning,
			TrainingBotRandom);
	}
	if (!Plan.IsValid())
	{
		UE_LOG(LogFlick, Warning, TEXT("Training bot could not find a valid shot"));
		return false;
	}

	AFlickPiece* Shooter = nullptr;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive()
			&& Piece->GetPieceId() == Plan.ShooterPieceId
			&& Piece->IsSelectableBy(BotTeam)
			&& Piece->GetOwningPlayerSlot() == BotPlayerSlot)
		{
			Shooter = Piece;
			break;
		}
	}
	if (!Shooter)
	{
		return false;
	}

	UE_LOG(
		LogFlick,
		Log,
		TEXT("TRAINING_BOT_SHOT: difficulty=%s shooter=%d target=%d power=%.2f score=%.2f"),
		*GetBotDifficultyLabel(),
		Plan.ShooterPieceId,
		Plan.TargetPieceId,
		Plan.NormalizedPower,
		Plan.Score);
	return ExecuteValidatedLaunch(
		Shooter,
		FVector(Plan.Direction.X, Plan.Direction.Y, 0.0f),
		Plan.NormalizedPower);
}

bool AFlickGameMode::CanSelectPiece(const AFlickPiece* Piece) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (IsTrainingBotMatch() && FlickGameState
		&& FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning
		&& Piece && Piece->GetTeam() != EFlickTeam::Player1)
	{
		return false;
	}
	if (IsTrainingBotMatch() && FlickGameState
		&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning
		&& FlickGameState->CurrentTeam == EFlickTeam::Player2)
	{
		return false;
	}
	const bool bCanUseEitherTrainingBobStriker = IsFreePlayTraining()
		&& IsBobMode()
		&& Piece
		&& Piece->IsBobStriker();
	const bool bCommonSelectionValid = FlickGameState
		&& FrontendScreen == EFlickFrontendScreen::Playing
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
		&& (!CameraPawn || !CameraPawn->IsGameplayViewTransitioning())
		&& Piece
		&& (bCanUseEitherTrainingBobStriker || FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning
			|| Piece->IsSelectableBy(FlickGameState->CurrentTeam))
		&& (IsFreePlayTraining() || IsBobMode() || Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot);
	return bCommonSelectionValid && (!IsBobMode() || Piece->IsBobStriker());
}

bool AFlickGameMode::CanSelectPieceForController(
	const APlayerController* RequestingPlayer,
	const AFlickPiece* Piece) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bCanUseEitherTrainingBobStriker = IsFreePlayTraining()
		&& IsBobMode()
		&& Piece
		&& Piece->IsBobStriker();
	if (!RequestingPlayer || !Piece || !FlickGameState
		|| (IsTrainingBotMatch()
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning
			&& FlickGameState->CurrentTeam == EFlickTeam::Player2)
		|| (IsTrainingBotMatch()
			&& FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning
			&& Piece->GetTeam() != EFlickTeam::Player1)
		|| FrontendScreen != EFlickFrontendScreen::Playing
		|| (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning)
		|| (!bCanUseEitherTrainingBobStriker
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning
			&& !Piece->IsSelectableBy(FlickGameState->CurrentTeam))
		|| (IsBobMode() && !Piece->IsBobStriker()))
	{
		return false;
	}
	if (bPrivateMatchActive
		&& GetPrivateSlotOwner(Piece->GetTeam(), Piece->GetOwningPlayerSlot())
			!= RequestingPlayer->GetPlayerState<AFlickPlayerState>())
	{
		return false;
	}

	// A solo private match can still run in standalone mode, but its empty seats
	// belong to bots. Only ordinary offline training uses the legacy local-seat rule.
	if (GetNetMode() == NM_Standalone && !bPrivateMatchActive)
	{
		return IsFreePlayTraining() || IsBobMode()
			|| Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot;
	}

	const AFlickPlayerState* FlickPlayerState = RequestingPlayer->GetPlayerState<AFlickPlayerState>();
	if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		if (!FlickPlayerState) return false;
		if (bPrivateMatchActive)
		{
			return FlickPlayerState->ControlsPrivateSlot(Piece->GetTeam(), Piece->GetOwningPlayerSlot());
		}
		return FlickPlayerState->GetTeam() == Piece->GetTeam()
			&& FlickPlayerState->GetTeamPlayerSlot() == Piece->GetOwningPlayerSlot();
	}
	if (bPrivateMatchActive)
	{
		return FlickPlayerState
			&& FlickPlayerState->ControlsPrivateSlot(
				FlickGameState->CurrentTeam,
				FlickGameState->CurrentTeamPlayerSlot)
			&& (IsBobMode() || Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot);
	}
	return FlickPlayerState
		&& FlickPlayerState->GetTeam() == FlickGameState->CurrentTeam
		&& FlickTeamRules::IsActivePlayerSlot(
			FlickPlayerState->GetTeamPlayerSlot(),
			FlickGameState->CurrentTeamPlayerSlot,
			CurrentPlayersPerTeam)
		&& (IsBobMode() || Piece->GetOwningPlayerSlot() == FlickPlayerState->GetTeamPlayerSlot());
}

bool AFlickGameMode::TryLaunchPiece(AFlickPiece* Piece, const FVector& Direction, const float NormalizedPower)
{
	if (!CanSelectPiece(Piece))
	{
		return false;
	}
	return ExecuteValidatedLaunch(Piece, Direction, NormalizedPower);
}

bool AFlickGameMode::ExecuteValidatedLaunch(
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!Piece || !FlickGameState)
	{
		return false;
	}
	if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		return LockKickoffShot(Piece, Direction, NormalizedPower);
	}

	const bool bFreePlayTraining = IsFreePlayTraining();
	const EFlickTeam ShootingTeam = bFreePlayTraining ? Piece->GetTeam() : FlickGameState->CurrentTeam;
	const int32 ShootingPlayerSlot = bFreePlayTraining
		? Piece->GetOwningPlayerSlot()
		: FlickGameState->CurrentTeamPlayerSlot;
	CaptureTestArenaControlZones();
	BeginResolutionTracking(ShootingTeam, false, Piece);
	FFlickReplayShotSetup& ReplayShot = ReplayShotSetups.AddDefaulted_GetRef();
	ReplayShot.Piece = Piece;
	ReplayShot.Direction = Direction.GetSafeNormal();
	ReplayShot.Power = FMath::Clamp(NormalizedPower, 0.0f, 1.0f);
	if (bFreePlayTraining)
	{
		FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
		FlickGameState->SetCurrentTeamPlayerSlot(ShootingPlayerSlot);
	}
	const FVector LaunchLocation = Piece->GetActorLocation() + FVector(0.0f, 0.0f, PieceThickness * 0.65f);
	FlickGameState->BeginShot(ShootingTeam, Piece->GetPieceId(), NormalizedPower);
	FlickGameState->RecordPlayerShot(ShootingTeam, ShootingPlayerSlot);
	AdvanceCompletedPlayerTurn(ShootingTeam, ShootingPlayerSlot);
	Piece->SetSelected(false);
	Piece->SetHovered(false);
	Piece->Launch(Direction, NormalizedPower, MaxLaunchSpeed);
	if (AudioDirector)
	{
		AudioDirector->PlayLaunch(Piece->GetArchetype(), NormalizedPower, LaunchLocation);
	}
	SpawnWorldFeedback(
		LaunchLocation,
		GetTeamColor(ShootingTeam),
		EFlickFeedbackKind::Launch,
		NormalizedPower,
		-Direction);
	AddCameraFeedback(FMath::Lerp(0.035f, 0.095f, NormalizedPower));
	AddControllerFeedback(FMath::Lerp(0.08f, 0.28f, NormalizedPower), 0.09f);

	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	FlickGameState->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);

	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s flicked Piece %d at power %.2f"),
		*GetTeamDisplayName(ShootingTeam),
		Piece->GetPieceId(),
		NormalizedPower);
	return true;
}

bool AFlickGameMode::TryLaunchPieceForController(
	APlayerController* RequestingPlayer,
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower)
{
	if (!CanSelectPieceForController(RequestingPlayer, Piece)
		|| Direction.ContainsNaN()
		|| Direction.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogFlick, Warning, TEXT("Rejected invalid or out-of-turn network flick request"));
		return false;
	}

	const bool bAccepted = ExecuteValidatedLaunch(
		Piece,
		Direction.GetSafeNormal2D(),
		FMath::Clamp(NormalizedPower, 0.0f, 1.0f));
	if (bAccepted)
	{
		UE_LOG(
			LogFlick,
			Log,
			TEXT("NETWORK_SHOT_ACCEPTED: %s Piece %d"),
			*GetTeamDisplayName(Piece->GetTeam()),
			Piece->GetPieceId());
	}
	return bAccepted;
}

bool AFlickGameMode::TryLaunchPieceByIdForController(
	APlayerController* RequestingPlayer,
	const int32 PieceId,
	const FVector& Direction,
	const float NormalizedPower)
{
	AFlickPiece* RequestedPiece = nullptr;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->GetPieceId() == PieceId)
		{
			RequestedPiece = Piece;
			break;
		}
	}
	return TryLaunchPieceForController(RequestingPlayer, RequestedPiece, Direction, NormalizedPower);
}

bool AFlickGameMode::LockKickoffShot(
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower,
	const bool bAnnounce)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!Piece || !FlickGameState || FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning)
	{
		return false;
	}

	FVector SafeDirection(Direction.X, Direction.Y, 0.0f);
	if (!SafeDirection.Normalize())
	{
		return false;
	}
	const float SafePower = FMath::Clamp(NormalizedPower, 0.0f, 1.0f);
	const EFlickTeam PlanningTeam = Piece->GetTeam();
	const int32 PlanningPlayerSlot = Piece->GetOwningPlayerSlot();
	if (Piece->GetTeam() != PlanningTeam
		|| Piece->GetOwningPlayerSlot() != PlanningPlayerSlot
		|| LockedKickoffShots.ContainsByPredicate([PlanningTeam, PlanningPlayerSlot](const FFlickLockedKickoffShot& Shot)
		{
			return Shot.Team == PlanningTeam && Shot.PlayerSlot == PlanningPlayerSlot;
		}))
	{
		return false;
	}

	FFlickLockedKickoffShot& LockedShot = LockedKickoffShots.AddDefaulted_GetRef();
	LockedShot.Piece = Piece;
	LockedShot.Direction = SafeDirection;
	LockedShot.Power = SafePower;
	LockedShot.Team = PlanningTeam;
	LockedShot.PlayerSlot = PlanningPlayerSlot;
	AdvanceCompletedPlayerTurn(PlanningTeam, PlanningPlayerSlot);
	Piece->SetSelected(false);
	Piece->SetHovered(false);

	const int32 RequiredShots = FlickTeamRules::GetSimultaneousKickoffShotCount(CurrentPlayersPerTeam);
	FlickGameState->SetKickoffProgress(LockedKickoffShots.Num(), RequiredShots);
	if (bAnnounce)
	{
		PushHudEvent(
			FString::Printf(
				TEXT("TEAM %d  /  PLAYER %d KICKOFF LOCKED  /  %d OF %d"),
				GetTeamNumber(PlanningTeam),
				PlanningPlayerSlot + 1,
				LockedKickoffShots.Num(),
				RequiredShots),
			GetTeamColor(PlanningTeam),
			1.6f);
	}
	if (AudioDirector)
	{
		AudioDirector->PlayUi(true);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s Player %d locked kickoff Piece %d at power %.2f (%d/%d)"),
		*GetTeamDisplayName(PlanningTeam),
		PlanningPlayerSlot + 1,
		Piece->GetPieceId(),
		SafePower,
		LockedKickoffShots.Num(),
		RequiredShots);

	if (LockedKickoffShots.Num() >= RequiredShots)
	{
		ReleaseKickoffShots();
		return true;
	}

	// Do not advance a global turn while planning. Every connected controller
	// may keep aiming its own assigned kickoff piece until it submits.
	return true;
}

void AFlickGameMode::ReleaseKickoffShots()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	const int32 RequiredShots = FlickTeamRules::GetSimultaneousKickoffShotCount(CurrentPlayersPerTeam);
	const bool bShotsValid = FlickGameState
		&& LockedKickoffShots.Num() == RequiredShots
		&& !LockedKickoffShots.ContainsByPredicate([](const FFlickLockedKickoffShot& Shot)
		{
			return !Shot.Piece.IsValid() || !Shot.Piece->IsActive();
		});
	if (!bShotsValid)
	{
		ResetKickoffState();
		BeginOpeningPhase();
		return;
	}

	CaptureTestArenaControlZones();
	BeginResolutionTracking(EFlickTeam::None, true, nullptr);
	float StrongestPower = 0.0f;
	for (const FFlickLockedKickoffShot& Shot : LockedKickoffShots)
	{
		AFlickPiece* Piece = Shot.Piece.Get();
		FFlickReplayShotSetup& ReplayShot = ReplayShotSetups.AddDefaulted_GetRef();
		ReplayShot.Piece = Piece;
		ReplayShot.Direction = Shot.Direction.GetSafeNormal();
		ReplayShot.Power = FMath::Clamp(Shot.Power, 0.0f, 1.0f);
		const FVector LaunchLocation = Piece->GetActorLocation() + FVector(0.0f, 0.0f, PieceThickness * 0.65f);
		FlickGameState->BeginShot(Shot.Team, Piece->GetPieceId(), Shot.Power);
		FlickGameState->RecordPlayerShot(Shot.Team, Shot.PlayerSlot);
		Piece->SetSelected(false);
		Piece->SetHovered(false);
		Piece->Launch(Shot.Direction, Shot.Power, MaxLaunchSpeed);
		if (AudioDirector)
		{
			AudioDirector->PlayLaunch(Piece->GetArchetype(), Shot.Power, LaunchLocation);
		}
		SpawnWorldFeedback(
			LaunchLocation,
			GetTeamColor(Shot.Team),
			EFlickFeedbackKind::Launch,
			Shot.Power,
			-Shot.Direction);
		StrongestPower = FMath::Max(StrongestPower, Shot.Power);
	}

	AddCameraFeedback(FMath::Lerp(0.07f, 0.16f, StrongestPower));
	AddControllerFeedback(FMath::Lerp(0.14f, 0.38f, StrongestPower), 0.14f);
	PushHudEvent(
		FString::Printf(TEXT("ALL %d KICKOFF SHOTS RELEASED"), RequiredShots),
		FLinearColor::White,
		1.9f);
	LockedKickoffShots.Reset();
	bResolvingKickoff = true;
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	FlickGameState->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);

	UE_LOG(LogFlick, Log, TEXT("Simultaneous kickoff released %d player shots"), RequiredShots);
}

void AFlickGameMode::NotifyPieceImpact(
	AFlickPiece* Piece,
	AFlickPiece* OtherPiece,
	const FVector& ImpactLocation,
	const float ImpactVelocityChange)
{
	if (!Piece || !OtherPiece || ImpactVelocityChange < 35.0f || !GetWorld())
	{
		return;
	}
	const int32 PieceId = Piece->GetPieceId();
	const int32 OtherPieceId = OtherPiece->GetPieceId();
	if (bTestArenaMode)
	{
		const AFlickGameState* State = GetFlickGameState();
		if (State && State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
		{
			if (!ResolutionFirstImpactTimes.Contains(PieceId))
			{
				ResolutionFirstImpactTimes.Add(PieceId, ResolutionElapsed);
			}
			if (!ResolutionFirstImpactTimes.Contains(OtherPieceId))
			{
				ResolutionFirstImpactTimes.Add(OtherPieceId, ResolutionElapsed);
			}
			if (Piece->GetTeam() != OtherPiece->GetTeam())
			{
				if (!ResolutionFirstOpponentImpactTimes.Contains(PieceId))
				{
					ResolutionFirstOpponentImpactTimes.Add(PieceId, ResolutionElapsed);
				}
				if (!ResolutionFirstOpponentImpactTimes.Contains(OtherPieceId))
				{
					ResolutionFirstOpponentImpactTimes.Add(OtherPieceId, ResolutionElapsed);
				}
			}
		}
	}
	if (PieceId == ResolutionShotPieceId)
	{
		ResolutionDirectContactPieceIds.Add(OtherPieceId);
	}
	else if (OtherPieceId == ResolutionShotPieceId)
	{
		ResolutionDirectContactPieceIds.Add(PieceId);
	}
	const int32* PieceDepth = ResolutionContactDepths.Find(PieceId);
	const int32* OtherDepth = ResolutionContactDepths.Find(OtherPieceId);
	if (PieceDepth && !OtherDepth)
	{
		ResolutionContactDepths.Add(OtherPieceId, *PieceDepth + 1);
	}
	else if (OtherDepth && !PieceDepth)
	{
		ResolutionContactDepths.Add(PieceId, *OtherDepth + 1);
	}
	if (ImpactVelocityChange < MinimumImpactFeedback)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastImpactFeedbackTime < 0.07f)
	{
		return;
	}
	LastImpactFeedbackTime = Now;

	const float Strength = FMath::Clamp(
		(ImpactVelocityChange - MinimumImpactFeedback)
		/ FMath::Max(StrongImpactFeedback - MinimumImpactFeedback, 1.0f),
		0.08f,
		1.0f);
	const FLinearColor ImpactColor = FLinearColor::LerpUsingHSV(
		GetTeamColor(Piece->GetTeam()),
		FLinearColor(1.0f, 0.82f, 0.2f, 1.0f),
		Strength * 0.7f);

	SpawnWorldFeedback(
		FVector(ImpactLocation.X, ImpactLocation.Y, FMath::Max(ImpactLocation.Z, ArenaSurfaceZ + 12.0f)),
		ImpactColor,
		EFlickFeedbackKind::Impact,
		Strength);
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->RecordImpact(ImpactVelocityChange);
		FlickGameState->RecordPlayerImpact(
			FlickGameState->LastShotTeam,
			FlickGameState->GetLastShootingPlayerSlot(FlickGameState->LastShotTeam));
	}
	++ResolutionImpactCount;
	AddCameraFeedback(FMath::Lerp(0.055f, 0.24f, Strength));
	AddControllerFeedback(FMath::Lerp(0.1f, 0.72f, Strength), FMath::Lerp(0.055f, 0.16f, Strength));
	if (AudioDirector)
	{
		AudioDirector->PlayImpact(
			Strength,
			Piece->PieceMassKg + OtherPiece->PieceMassKg,
			Piece->GetArchetype(),
			OtherPiece->GetArchetype(),
			ImpactLocation);
	}

	if (Strength >= 0.78f && Now - LastStrongImpactEventTime > 0.45f)
	{
		LastStrongImpactEventTime = Now;
		PushHudEvent(TEXT("HEAVY HIT"), ImpactColor, 1.1f);
	}
}

void AFlickGameMode::NotifyArenaImpact(
	AFlickPiece* Piece,
	UPrimitiveComponent* OtherComponent,
	const FVector& ImpactLocation,
	const float ImpactVelocityChange)
{
	if (!Piece || ImpactVelocityChange < 45.0f)
	{
		return;
	}
	if (bTestArenaMode && TestArenaActor && OtherComponent)
	{
		int32 DividerIndex = INDEX_NONE;
		if (TestArenaActor->FindDividerIndex(OtherComponent, DividerIndex))
		{
			ResolutionDividerContactPieceIds.Add(Piece->GetPieceId());
			const uint16 DividerBit = static_cast<uint16>(1 << DividerIndex);
			if ((ResolutionNewlyRaisedDividerMask & DividerBit) != 0)
			{
				ResolutionNewDividerContactPieceIds.Add(Piece->GetPieceId());
			}
		}
	}
	if (!AudioDirector)
	{
		return;
	}
	const float Strength = FMath::Clamp((ImpactVelocityChange - 45.0f) / 720.0f, 0.05f, 1.0f);
	AudioDirector->PlayRimImpact(
		Strength,
		Piece->PieceMassKg,
		Piece->GetArchetype(),
		ImpactLocation);
}

void AFlickGameMode::NotifyPieceSelected(const AFlickPiece* Piece) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!Piece || !FlickGameState)
	{
		return;
	}

	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s selected Piece %d"),
		*GetTeamDisplayName(FlickGameState->CurrentTeam),
		Piece->GetPieceId());
}

void AFlickGameMode::RestartMatch()
{
	if (FrontendScreen == EFlickFrontendScreen::MainMenu
		|| FrontendScreen == EFlickFrontendScreen::ModeSelect)
	{
		return;
	}
	if (bNetworkMatchRequested && bRankedRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Ranked restart rejected; return to the lobby to register a new authoritative match."));
		return;
	}
	if (IsTutorialMode())
	{
		UGameplayStatics::SetGamePaused(this, false);
		FrontendScreen = EFlickFrontendScreen::Playing;
		SetupTutorialStage(bTutorialCompleted ? 0 : TutorialStageIndex);
		return;
	}
	if (IsFreePlayTraining())
	{
		UGameplayStatics::SetGamePaused(this, false);
		FrontendScreen = EFlickFrontendScreen::Playing;
		ResetTrainingBoard();
		UE_LOG(LogFlick, Log, TEXT("Training board manually reset"));
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	FrontendScreen = EFlickFrontendScreen::Playing;
	ApplyPendingPlayerClasses();
	SetCameraForFrontend();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	if (bNetworkMatchRequested)
	{
		if (AFlickGameState* State = GetFlickGameState())
		{
			State->BeginAuthoritativeMatch(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
		}
		BeginRankedMatchForPlayers();
	}
	if (AudioDirector && GetFlickGameState())
	{
		AudioDirector->PlayTurn(GetFlickGameState()->CurrentTeam);
	}
	UE_LOG(LogFlick, Log, TEXT("Match restarted"));
}

void AFlickGameMode::OpenModeSelect()
{
	if (FrontendScreen == EFlickFrontendScreen::MainMenu
		|| FrontendScreen == EFlickFrontendScreen::ModeSelect)
	{
		FrontendScreen = EFlickFrontendScreen::ModeSelect;
		if (ActiveMatchVariant != SelectedMatchVariant
			|| CurrentPlayersPerTeam != MatchmakingPlayersPerTeam)
		{
			ShowModePreview(SelectedMatchVariant, MatchmakingPlayersPerTeam);
		}
		SetCameraForFrontend();
	}
}

void AFlickGameMode::CloseModeSelect()
{
	if (FrontendScreen == EFlickFrontendScreen::ModeSelect)
	{
		FrontendScreen = EFlickFrontendScreen::MainMenu;
		SetCameraForFrontend();
	}
}

void AFlickGameMode::OpenOnlineBrowser()
{
	if (bPartyRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Online browser is disabled while a pre-match party is active"));
		return;
	}
	if (FrontendScreen != EFlickFrontendScreen::ModeSelect
		&& FrontendScreen != EFlickFrontendScreen::OnlineBrowser)
	{
		return;
	}
	FrontendScreen = EFlickFrontendScreen::OnlineBrowser;
	SetCameraForFrontend();
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
	{
		Sessions->FindSessions();
	}
}

void AFlickGameMode::CloseOnlineBrowser()
{
	if (FrontendScreen == EFlickFrontendScreen::OnlineBrowser)
	{
		FrontendScreen = EFlickFrontendScreen::ModeSelect;
		SetCameraForFrontend();
	}
}

bool AFlickGameMode::DoesSelectedModeSupportLoadouts() const
{
	return FlickModeRules::Get(SelectedMatchVariant).bSupportsLoadouts;
}

int32 AFlickGameMode::GetLoadoutEditingPieceCount() const
{
	return FlickModeRules::Get(LoadoutEditingVariant).StartingPiecesPerTeam;
}

bool AFlickGameMode::CanChangeCameraView() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return CameraPawn
		&& FrontendScreen == EFlickFrontendScreen::Playing
		&& FlickGameState
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning
			|| FlickGameState->MatchPhase == EFlickMatchPhase::ResolvingPhysics);
}

bool AFlickGameMode::IsAimGuideEnabled() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return !FlickGameInstance || FlickGameInstance->IsAimGuideEnabled();
}

bool AFlickGameMode::AreImpactEffectsEnabled() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return !FlickGameInstance || FlickGameInstance->AreImpactEffectsEnabled();
}

bool AFlickGameMode::IsControlOverviewEnabled() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return !FlickGameInstance || FlickGameInstance->IsControlOverviewEnabled();
}

EFlickBotDifficulty AFlickGameMode::GetBotDifficulty() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetBotDifficulty() : EFlickBotDifficulty::Normal;
}

float AFlickGameMode::GetCameraShakeIntensity() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetCameraShakeIntensity() : 1.0f;
}

float AFlickGameMode::GetMasterVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetMasterVolume() : 1.0f;
}

float AFlickGameMode::GetEffectsVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetEffectsVolume() : 1.0f;
}

float AFlickGameMode::GetInterfaceVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetInterfaceVolume() : 1.0f;
}

bool AFlickGameMode::IsVSyncEnabled() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return Settings && Settings->IsVSyncEnabled();
}

FString AFlickGameMode::GetWindowModeLabel() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return TEXT("WINDOWED");
	}

	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen:
		return TEXT("FULLSCREEN");
	case EWindowMode::WindowedFullscreen:
		return TEXT("BORDERLESS");
	case EWindowMode::Windowed:
	default:
		return TEXT("WINDOWED");
	}
}

FString AFlickGameMode::GetResolutionLabel() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const FIntPoint Resolution = Settings ? Settings->GetScreenResolution() : FIntPoint(1280, 720);
	return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}

