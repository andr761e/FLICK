#include "Game/FlickGameModePrivate.h"

using namespace FlickGameModePrivate;

void AFlickGameMode::BeginResolutionTracking(
	const EFlickTeam ShootingTeam,
	const bool bSimultaneousShot,
	AFlickPiece* ShotPiece)
{
	if (bCinematicReplayActive)
	{
		FinishCinematicRoundReplay(false);
	}
	RoundReplayFrames.Reset();
	ReplayShotSetups.Reset();
	LastReplayCaptureTime = -100.0f;
	bReplayPlayedForResolution = false;
	ResolutionElapsed = 0.0f;
	ResolutionPlayer1Eliminated = 0;
	ResolutionPlayer2Eliminated = 0;
	ResolutionImpactCount = 0;
	ResolutionShootingTeam = ShootingTeam;
	bResolutionWasSimultaneous = bSimultaneousShot;
	ResolutionShotPieceId = ShotPiece ? ShotPiece->GetPieceId() : INDEX_NONE;
	ResolutionShotStart = ShotPiece
		? FVector2D(ShotPiece->GetActorLocation().X, ShotPiece->GetActorLocation().Y)
		: FVector2D::ZeroVector;
	ResolutionActivatedSwitchMask = 0;
	ResolutionNewlyRaisedDividerMask = 0;
	ResolutionInitialPieceLocations.Reset();
	ResolutionContactDepths.Reset();
	ResolutionDirectContactPieceIds.Reset();
	ResolutionEliminatedPieceIds.Reset();
	ResolutionDividerContactPieceIds.Reset();
	ResolutionNewDividerContactPieceIds.Reset();
	ResolutionFirstImpactTimes.Reset();
	ResolutionFirstOpponentImpactTimes.Reset();
	ResolutionEliminationTimes.Reset();
	ReplayPresentedEliminationPieceIds.Reset();
	ReplayPrimaryFocusPieceId = INDEX_NONE;
	ReplayKnockoutFocusPieceId = INDEX_NONE;
	ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		bResolutionBuzzerRelease = !bSimultaneousShot
			&& FlickGameState->bShotClockActive
			&& FlickGameState->GetShotClockTimeRemaining() <= BuzzerBeaterTimeThreshold;
		FlickGameState->ClearDramaticEvent();
	}
	else
	{
		bResolutionBuzzerRelease = false;
	}
	for (const AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive())
		{
			const FVector Location = Piece->GetActorLocation();
			ResolutionInitialPieceLocations.Add(Piece->GetPieceId(), FVector2D(Location.X, Location.Y));
		}
	}
	if (ResolutionShotPieceId != INDEX_NONE)
	{
		ResolutionContactDepths.Add(ResolutionShotPieceId, 0);
	}
	CaptureRoundReplayFrame(true);
}

float AFlickGameMode::GetCinematicReplayProgress() const
{
	return CinematicReplayPlaybackDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(CinematicReplayElapsed / CinematicReplayPlaybackDuration, 0.0f, 1.0f)
		: 0.0f;
}

bool AFlickGameMode::IsCinematicReplayPullbackActive() const
{
	return bCinematicReplayActive && CinematicReplayElapsed < ReplayPullbackDuration;
}

float AFlickGameMode::GetCinematicReplayPullbackAlpha() const
{
	if (!bCinematicReplayActive)
	{
		return 0.0f;
	}
	if (!IsCinematicReplayPullbackActive())
	{
		return 1.0f;
	}
	const float BuildDuration = FMath::Max(ReplayPullbackDuration * 0.7f, 0.01f);
	const float Alpha = FMath::Clamp(CinematicReplayElapsed / BuildDuration, 0.0f, 1.0f);
	return Alpha * Alpha * (3.0f - 2.0f * Alpha);
}

const AFlickPiece* AFlickGameMode::GetCinematicReplayShotPiece(const int32 ShotIndex) const
{
	return ReplayShotSetups.IsValidIndex(ShotIndex) ? ReplayShotSetups[ShotIndex].Piece.Get() : nullptr;
}

FVector AFlickGameMode::GetCinematicReplayShotDirection(const int32 ShotIndex) const
{
	return ReplayShotSetups.IsValidIndex(ShotIndex) ? ReplayShotSetups[ShotIndex].Direction : FVector::ZeroVector;
}

float AFlickGameMode::GetCinematicReplayShotPower(const int32 ShotIndex) const
{
	return ReplayShotSetups.IsValidIndex(ShotIndex) ? ReplayShotSetups[ShotIndex].Power : 0.0f;
}

void AFlickGameMode::CaptureTestArenaControlZones()
{
	if (bTestArenaMode && TestArenaActor && IsValid(TestArenaActor))
	{
		TestArenaActor->BeginControlZoneTracking(Pieces);
	}
}

void AFlickGameMode::TrackTestArenaControlZones(const float DeltaSeconds)
{
	if (bTestArenaMode && IsValid(TestArenaActor))
	{
		uint16 DeployedMechanisms = 0;
		ResolutionActivatedSwitchMask |= TestArenaActor->TrackControlZoneCrossings(Pieces, DeltaSeconds, DeployedMechanisms);
		for (int32 Index = 0; Index < TestArenaActor->GetMechanismCount(); ++Index)
		{
			const uint16 Bit = static_cast<uint16>(1 << Index);
			if ((DeployedMechanisms & Bit) != 0 && TestArenaActor->IsDividerRaised(Index))
			{
				ResolutionNewlyRaisedDividerMask |= Bit;
			}
		}
	}
}

void AFlickGameMode::ResolveTestArenaControlZones()
{
	if (bTestArenaMode && IsValid(TestArenaActor))
	{
		TestArenaActor->CommitPendingControlZoneToggles(Pieces);
	}
}

void AFlickGameMode::CaptureRoundReplayFrame(const bool bForce)
{
	if (!bTestArenaMode || bCinematicReplayActive || !TestArenaActor || !IsValid(TestArenaActor))
	{
		return;
	}

	const float CaptureInterval = 1.0f / FMath::Max(ReplayCaptureRate, 1.0f);
	if (!bForce && ResolutionElapsed - LastReplayCaptureTime < CaptureInterval)
	{
		return;
	}
	if (bForce && !RoundReplayFrames.IsEmpty()
		&& FMath::IsNearlyEqual(RoundReplayFrames.Last().Time, ResolutionElapsed, KINDA_SMALL_NUMBER))
	{
		RoundReplayFrames.Pop(EAllowShrinking::No);
	}

	FFlickRoundReplayFrame& Frame = RoundReplayFrames.AddDefaulted_GetRef();
	Frame.Time = FMath::Max(0.0f, ResolutionElapsed);
	Frame.RaisedDividerMask = TestArenaActor->GetRaisedDividerMask();
	Frame.Pieces.Reserve(Pieces.Num());
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece))
		{
			continue;
		}
		FFlickReplayPieceState& PieceState = Frame.Pieces.AddDefaulted_GetRef();
		PieceState.Piece = Piece;
		PieceState.Transform = Piece->GetActorTransform();
		PieceState.bVisible = Piece->IsActive();
	}
	LastReplayCaptureTime = Frame.Time;
}

void AFlickGameMode::BeginCinematicRoundReplay(const EFlickMatchOutcome Outcome)
{
	if (!bTestArenaMode || bCinematicReplayActive || bReplayPlayedForResolution
		|| !TestArenaActor || !IsValid(TestArenaActor) || RoundReplayFrames.Num() < 2
		|| (Outcome != EFlickMatchOutcome::Player1Wins && Outcome != EFlickMatchOutcome::Player2Wins))
	{
		return;
	}

	const float FirstTime = RoundReplayFrames[0].Time;
	const float LastTime = RoundReplayFrames.Last().Time;
	CinematicReplaySourceStart = FirstTime;
	CinematicReplaySourceDuration = FMath::Max(LastTime - FirstTime, 0.01f);
	CinematicReplayMotionDuration = FMath::Clamp(
		CinematicReplaySourceDuration / FMath::Clamp(ReplaySlowMotionRate, 0.25f, 1.0f),
		1.25f,
		FMath::Max(ReplayMaximumPlaybackDuration, 2.0f));
	CinematicReplayPlaybackDuration = ReplayPullbackDuration + CinematicReplayMotionDuration;
	CinematicReplayElapsed = 0.0f;
	PendingReplayOutcome = Outcome;
	bCinematicReplayActive = true;
	bReplayPlayedForResolution = true;
	bReplayLaunchCuePlayed = false;
	ReplayPresentedEliminationPieceIds.Reset();
	const EFlickTeam WinningTeam = Outcome == EFlickMatchOutcome::Player1Wins
		? EFlickTeam::Player1 : EFlickTeam::Player2;
	const EFlickTeam LosingTeam = GetOpposingTeam(WinningTeam);
	ReplayPrimaryFocusPieceId = ResolutionShotPieceId;
	if (ReplayPrimaryFocusPieceId == INDEX_NONE)
	{
		const FFlickReplayShotSetup* WinningShot = ReplayShotSetups.FindByPredicate([WinningTeam](const FFlickReplayShotSetup& Shot)
		{
			return Shot.Piece.IsValid() && Shot.Piece->GetTeam() == WinningTeam;
		});
		if (!WinningShot && !ReplayShotSetups.IsEmpty())
		{
			WinningShot = &ReplayShotSetups[0];
		}
		ReplayPrimaryFocusPieceId = WinningShot && WinningShot->Piece.IsValid()
			? WinningShot->Piece->GetPieceId() : INDEX_NONE;
	}

	float LatestLosingElimination = -1.0f;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || Piece->GetTeam() != LosingTeam
			|| !ResolutionEliminatedPieceIds.Contains(Piece->GetPieceId()))
		{
			continue;
		}
		const float EliminationTime = ResolutionEliminationTimes.FindRef(Piece->GetPieceId());
		if (EliminationTime >= LatestLosingElimination)
		{
			LatestLosingElimination = EliminationTime;
			ReplayKnockoutFocusPieceId = Piece->GetPieceId();
		}
	}

	const FFlickReplayShotSetup* SelfKnockoutShot = ReplayShotSetups.FindByPredicate([this, LosingTeam](const FFlickReplayShotSetup& Shot)
	{
		if (!Shot.Piece.IsValid() || Shot.Piece->GetTeam() != LosingTeam
			|| !ResolutionEliminatedPieceIds.Contains(Shot.Piece->GetPieceId()))
		{
			return false;
		}
		return !bResolutionWasSimultaneous
			|| !ResolutionFirstOpponentImpactTimes.Contains(Shot.Piece->GetPieceId());
	});
	if (SelfKnockoutShot && SelfKnockoutShot->Piece.IsValid())
	{
		ReplayPrimaryFocusPieceId = SelfKnockoutShot->Piece->GetPieceId();
	}

	const AFlickPiece* PrimaryPiece = nullptr;
	if (const TObjectPtr<AFlickPiece>* Entry = Pieces.FindByPredicate([this](const TObjectPtr<AFlickPiece>& Piece)
	{
		return Piece && Piece->GetPieceId() == ReplayPrimaryFocusPieceId;
	}))
	{
		PrimaryPiece = Entry->Get();
	}
	const bool bSelfKnockout = PrimaryPiece
		&& PrimaryPiece->GetTeam() == LosingTeam
		&& ResolutionEliminatedPieceIds.Contains(ReplayPrimaryFocusPieceId);
	if (bSelfKnockout)
	{
		ReplayKnockoutFocusPieceId = ReplayPrimaryFocusPieceId;
		ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	}
	else if (ReplayKnockoutFocusPieceId != INDEX_NONE
		&& ReplayKnockoutFocusPieceId != ReplayPrimaryFocusPieceId)
	{
		const float* FirstImpact = ResolutionFirstImpactTimes.Find(ReplayKnockoutFocusPieceId);
		ReplayFocusSwitchSourceTime = FirstImpact
			? *FirstImpact
			: FMath::Max(FirstTime, LatestLosingElimination - 0.25f);
		ReplayFocusSwitchSourceTime = FMath::Clamp(ReplayFocusSwitchSourceTime, FirstTime, LastTime);
	}

	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->BeginReplayPresentation();
		}
	}
	TestArenaActor->BeginReplayPresentation();
	ApplyCinematicReplayTime(CinematicReplaySourceStart);

	FVector InitialFocus = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
	if (const FFlickReplayPieceState* ShotState = RoundReplayFrames[0].Pieces.FindByPredicate([this](const FFlickReplayPieceState& State)
	{
		return State.Piece.IsValid() && State.Piece->GetPieceId() == ReplayPrimaryFocusPieceId;
	}))
	{
		InitialFocus = ShotState->Transform.GetLocation();
	}
	const EFlickTeam ReplayTeam = ResolutionShootingTeam != EFlickTeam::None
		? ResolutionShootingTeam
		: Outcome == EFlickMatchOutcome::Player1Wins ? EFlickTeam::Player1 : EFlickTeam::Player2;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get()))
		{
			Controller->BeginCinematicReplayFromServer(InitialFocus, ReplayTeam);
		}
	}
	if (AudioDirector)
	{
		AudioDirector->PlayReplayMusic(CinematicReplayPlaybackDuration, ReplayTeam);
	}
	ClearControllerAiming();
	PushHudEvent(TEXT("REPLAY  //  ROUND-WINNING SHOT"), FLinearColor::White, CinematicReplayPlaybackDuration);
	UE_LOG(LogFlick, Log, TEXT("Test arena replay started: %.2f second pullback and %.2f seconds of shot playback"),
		ReplayPullbackDuration, CinematicReplayMotionDuration);
	UE_LOG(LogFlick, Log, TEXT("Replay camera narrative: primary=%d knockout=%d switch=%.2f self_ko=%d"),
		ReplayPrimaryFocusPieceId,
		ReplayKnockoutFocusPieceId,
		ReplayFocusSwitchSourceTime,
		bSelfKnockout ? 1 : 0);
}

void AFlickGameMode::UpdateCinematicRoundReplay(const float DeltaSeconds)
{
	if (!bCinematicReplayActive)
	{
		return;
	}

	CinematicReplayElapsed += FMath::Max(0.0f, DeltaSeconds);
	if (CinematicReplayElapsed < ReplayPullbackDuration)
	{
		ApplyCinematicReplayTime(CinematicReplaySourceStart);
	}
	else
	{
		if (!bReplayLaunchCuePlayed)
		{
			bReplayLaunchCuePlayed = true;
			if (AudioDirector)
			{
				for (const FFlickReplayShotSetup& ReplayShot : ReplayShotSetups)
				{
					if (const AFlickPiece* Piece = ReplayShot.Piece.Get())
					{
						AudioDirector->PlayLaunch(
							Piece->GetArchetype(),
							ReplayShot.Power,
							Piece->GetActorLocation() + FVector(0.0f, 0.0f, PieceThickness * 0.65f));
					}
				}
			}
		}
		const float MotionProgress = CinematicReplayMotionDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp((CinematicReplayElapsed - ReplayPullbackDuration) / CinematicReplayMotionDuration, 0.0f, 1.0f)
			: 1.0f;
		ApplyCinematicReplayTime(CinematicReplaySourceStart + CinematicReplaySourceDuration * MotionProgress);
	}
	if (CinematicReplayElapsed >= CinematicReplayPlaybackDuration)
	{
		FinishCinematicRoundReplay(true);
	}
}

void AFlickGameMode::ApplyCinematicReplayTime(const float SourceTime)
{
	if (RoundReplayFrames.IsEmpty())
	{
		return;
	}

	int32 UpperIndex = 0;
	while (UpperIndex < RoundReplayFrames.Num() && RoundReplayFrames[UpperIndex].Time < SourceTime)
	{
		++UpperIndex;
	}
	UpperIndex = FMath::Clamp(UpperIndex, 0, RoundReplayFrames.Num() - 1);
	const int32 LowerIndex = FMath::Max(0, UpperIndex - 1);
	const FFlickRoundReplayFrame& LowerFrame = RoundReplayFrames[LowerIndex];
	const FFlickRoundReplayFrame& UpperFrame = RoundReplayFrames[UpperIndex];
	const float FrameSpan = UpperFrame.Time - LowerFrame.Time;
	const float Alpha = FrameSpan > KINDA_SMALL_NUMBER
		? FMath::Clamp((SourceTime - LowerFrame.Time) / FrameSpan, 0.0f, 1.0f)
		: 0.0f;

	const bool bFollowKnockout = ReplayKnockoutFocusPieceId != INDEX_NONE
		&& SourceTime >= ReplayFocusSwitchSourceTime;
	const int32 FocusPieceId = bFollowKnockout
		? ReplayKnockoutFocusPieceId
		: ReplayPrimaryFocusPieceId;
	FVector ReplayFocus = FVector::ZeroVector;
	FVector VisibleCenter = FVector::ZeroVector;
	int32 VisiblePieceCount = 0;
	bool bFoundFocusPiece = false;
	for (const FFlickReplayPieceState& LowerState : LowerFrame.Pieces)
	{
		AFlickPiece* Piece = LowerState.Piece.Get();
		if (!Piece || !IsValid(Piece))
		{
			continue;
		}
		const FFlickReplayPieceState* UpperState = UpperFrame.Pieces.FindByPredicate([Piece](const FFlickReplayPieceState& State)
		{
			return State.Piece.Get() == Piece;
		});
		const FTransform& EndTransform = UpperState ? UpperState->Transform : LowerState.Transform;
		FTransform BlendedTransform;
		BlendedTransform.Blend(LowerState.Transform, EndTransform, Alpha);
		bool bVisible = LowerState.bVisible;
		if (const float* EliminationTime = ResolutionEliminationTimes.Find(Piece->GetPieceId()))
		{
			bVisible = SourceTime < *EliminationTime;
		}
		Piece->ApplyReplayPresentation(BlendedTransform, bVisible);
		if (Piece->GetPieceId() == FocusPieceId)
		{
			ReplayFocus = BlendedTransform.GetLocation();
			bFoundFocusPiece = true;
		}
		if (bVisible)
		{
			VisibleCenter += BlendedTransform.GetLocation();
			++VisiblePieceCount;
		}
	}

	// Live elimination feedback is transient and has expired by the time the
	// replay starts. Recreate it exactly once when replay time crosses each
	// recorded ring-out, without re-running elimination gameplay or scoring.
	for (const TPair<int32, float>& Elimination : ResolutionEliminationTimes)
	{
		if (SourceTime < Elimination.Value
			|| ReplayPresentedEliminationPieceIds.Contains(Elimination.Key))
		{
			continue;
		}

		AFlickPiece* EliminatedPiece = nullptr;
		for (AFlickPiece* Piece : Pieces)
		{
			if (Piece && IsValid(Piece) && Piece->GetPieceId() == Elimination.Key)
			{
				EliminatedPiece = Piece;
				break;
			}
		}
		if (!EliminatedPiece)
		{
			continue;
		}

		const FVector ArenaLocation = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
		const float ActiveArenaRadius = ArenaActor ? ArenaActor->GetRadius() : ArenaRadius;
		FVector EdgeDirection(
			EliminatedPiece->GetActorLocation().X - ArenaLocation.X,
			EliminatedPiece->GetActorLocation().Y - ArenaLocation.Y,
			0.0f);
		if (!EdgeDirection.Normalize())
		{
			EdgeDirection = FVector::ForwardVector;
		}
		const FVector FeedbackLocation = ArenaLocation
			+ EdgeDirection * (ActiveArenaRadius - 20.0f)
			+ FVector(0.0f, 0.0f, ArenaSurfaceZ + 22.0f - ArenaLocation.Z);
		SpawnWorldFeedback(
			FeedbackLocation,
			GetTeamColor(EliminatedPiece->GetTeam()),
			EFlickFeedbackKind::Elimination,
			1.0f,
			EdgeDirection);
		ReplayPresentedEliminationPieceIds.Add(Elimination.Key);
	}
	if (!bFoundFocusPiece && VisiblePieceCount > 0)
	{
		ReplayFocus = VisibleCenter / static_cast<float>(VisiblePieceCount);
	}
	else if (!bFoundFocusPiece)
	{
		ReplayFocus = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
	}
	ReplayFocus.Z = ArenaSurfaceZ + PieceThickness;
	if (TestArenaActor)
	{
		TestArenaActor->ApplyReplayDividerState(LowerFrame.RaisedDividerMask);
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get()))
		{
			Controller->UpdateCinematicReplayFromServer(
				ReplayFocus,
				GetCinematicReplayProgress(),
				GetCinematicReplayPullbackAlpha());
		}
	}
}

void AFlickGameMode::FinishCinematicRoundReplay(const bool bCompleteRound)
{
	if (!bCinematicReplayActive)
	{
		return;
	}

	const EFlickMatchOutcome CompletedOutcome = PendingReplayOutcome;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->EndReplayPresentation();
		}
	}
	if (TestArenaActor && IsValid(TestArenaActor))
	{
		TestArenaActor->EndReplayPresentation();
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get()))
		{
			Controller->EndCinematicReplayFromServer();
		}
	}
	if (AudioDirector)
	{
		AudioDirector->StopReplayMusic();
	}

	bCinematicReplayActive = false;
	CinematicReplayElapsed = 0.0f;
	CinematicReplaySourceStart = 0.0f;
	CinematicReplaySourceDuration = 0.0f;
	CinematicReplayMotionDuration = 0.0f;
	CinematicReplayPlaybackDuration = 0.0f;
	PendingReplayOutcome = EFlickMatchOutcome::Continue;
	RoundReplayFrames.Reset();
	ReplayShotSetups.Reset();
	ReplayPresentedEliminationPieceIds.Reset();
	bReplayLaunchCuePlayed = false;
	ReplayPrimaryFocusPieceId = INDEX_NONE;
	ReplayKnockoutFocusPieceId = INDEX_NONE;
	ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	if (bCompleteRound)
	{
		CompleteRoundForOutcome(CompletedOutcome);
	}
}

void AFlickGameMode::PresentDramaticResolutionEvent()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsBobMode())
	{
		return;
	}

	const FFlickDramaticEventResult Result = EvaluateDramaticEvent(
		ResolutionPlayer1Eliminated,
		ResolutionPlayer2Eliminated,
		CountActivePieces(EFlickTeam::Player1),
		CountActivePieces(EFlickTeam::Player2),
		ResolutionImpactCount,
		DramaticChainImpactThreshold,
		ResolutionShootingTeam,
		bResolutionWasSimultaneous);
	if (!Result.IsValid())
	{
		return;
	}

	int32 BonusPoints = 0;
	TArray<EFlickTeam, TInlineAllocator<2>> AwardedTeams;
	switch (Result.Event)
	{
	case EFlickDramaticEvent::Trade:
		BonusPoints = TradeBonusPoints;
		AwardedTeams.Add(EFlickTeam::Player1);
		AwardedTeams.Add(EFlickTeam::Player2);
		break;
	case EFlickDramaticEvent::DoubleKnockout:
		BonusPoints = DoubleKnockoutBonusPoints;
		if (Result.HighlightedTeam != EFlickTeam::None)
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::MultiKnockout:
		BonusPoints = MultiKnockoutBonusPerPuck * Result.Value;
		if (Result.HighlightedTeam == EFlickTeam::None)
		{
			AwardedTeams.Add(EFlickTeam::Player1);
			AwardedTeams.Add(EFlickTeam::Player2);
		}
		else
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::LastPuckStanding:
		BonusPoints = LastPuckStandingBonusPoints;
		if (Result.HighlightedTeam != EFlickTeam::None)
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::ChainReaction:
		BonusPoints = ChainReactionBonusPoints;
		if (Result.HighlightedTeam != EFlickTeam::None)
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::SelfKnockout:
	case EFlickDramaticEvent::None:
	default:
		break;
	}

	BonusPoints = FMath::Max(0, BonusPoints);
	for (const EFlickTeam AwardedTeam : AwardedTeams)
	{
		const int32 PlayerSlot = FlickGameState->GetLastShootingPlayerSlot(AwardedTeam);
		if (Result.Event == EFlickDramaticEvent::DoubleKnockout)
		{
			FlickGameState->RecordPlayerDoubleKnockout(AwardedTeam, PlayerSlot);
		}
		FlickGameState->RecordPlayerBonus(
			AwardedTeam,
			PlayerSlot,
			BonusPoints);
	}

	FlickGameState->ShowDramaticEvent(
		Result.Event,
		Result.HighlightedTeam,
		Result.Value,
		ResolutionImpactCount,
		BonusPoints,
		DramaticEventDuration);
}

void AFlickGameMode::PresentShotAccolades()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsBobMode() || bResolutionWasSimultaneous
		|| ResolutionShootingTeam == EFlickTeam::None || ResolutionShotPieceId == INDEX_NONE)
	{
		return;
	}

	const bool bShooterIsPlayer1 = ResolutionShootingTeam == EFlickTeam::Player1;
	const int32 OpponentEliminated = bShooterIsPlayer1
		? ResolutionPlayer2Eliminated : ResolutionPlayer1Eliminated;
	const int32 OwnEliminated = bShooterIsPlayer1
		? ResolutionPlayer1Eliminated : ResolutionPlayer2Eliminated;
	const EFlickTeam OpponentTeam = GetOpposingTeam(ResolutionShootingTeam);

	bool bDominoKnockout = false;
	bool bLongRangeKnockout = false;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || Piece->GetTeam() != OpponentTeam
			|| !ResolutionEliminatedPieceIds.Contains(Piece->GetPieceId()))
		{
			continue;
		}
		if (const int32* ContactDepth = ResolutionContactDepths.Find(Piece->GetPieceId()))
		{
			bDominoKnockout |= *ContactDepth >= 2
				&& !ResolutionDirectContactPieceIds.Contains(Piece->GetPieceId());
		}
		if (const FVector2D* InitialTargetLocation = ResolutionInitialPieceLocations.Find(Piece->GetPieceId()))
		{
			bLongRangeKnockout |= FVector2D::Distance(ResolutionShotStart, *InitialTargetLocation)
				>= LongRangeKnockoutDistance;
		}
	}

	bool bPrecisionStop = false;
	const TObjectPtr<AFlickPiece>* ShotPieceEntry = Pieces.FindByPredicate([this](const TObjectPtr<AFlickPiece>& Piece)
	{
		return Piece && Piece->GetPieceId() == ResolutionShotPieceId;
	});
	if (const AFlickPiece* ShotPiece = ShotPieceEntry ? ShotPieceEntry->Get() : nullptr)
	{
		if (IsValid(ShotPiece) && ShotPiece->IsActive())
		{
			const FVector ArenaLocation = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
			const float Radius = FVector2D::Distance(
				FVector2D(ShotPiece->GetActorLocation().X, ShotPiece->GetActorLocation().Y),
				FVector2D(ArenaLocation.X, ArenaLocation.Y));
			const float ActiveRadius = ArenaActor ? ArenaActor->GetRadius() : ArenaRadius;
			bPrecisionStop = Radius >= ActiveRadius * PrecisionStopMinimumRadiusFraction
				&& Radius <= ActiveRadius * PrecisionStopMaximumRadiusFraction;
		}
	}

	FFlickShotAccoladeContext Context;
	Context.OpponentEliminated = OpponentEliminated;
	Context.OwnEliminated = OwnEliminated;
	Context.OpponentRemaining = CountActivePieces(OpponentTeam);
	Context.OwnRemaining = CountActivePieces(ResolutionShootingTeam);
	Context.DirectlyContactedPucks = ResolutionDirectContactPieceIds.Num();
	Context.bSwitchActivated = ResolutionActivatedSwitchMask != 0;
	Context.bShotPieceHitDivider = ResolutionDividerContactPieceIds.Contains(ResolutionShotPieceId);
	Context.bNewDividerAffectedPlay = !ResolutionNewDividerContactPieceIds.IsEmpty();
	Context.bDominoKnockout = bDominoKnockout;
	Context.bLongRangeKnockout = bLongRangeKnockout;
	Context.bShotPieceSurvivedNearEdge = bPrecisionStop;
	Context.bBuzzerRelease = bResolutionBuzzerRelease;

	const int32 ShootingPlayerSlot = FlickGameState->GetLastShootingPlayerSlot(ResolutionShootingTeam);
	for (const EFlickAccolade Accolade : FlickAccoladeRules::EvaluateShot(Context))
	{
		const int32 BonusPoints = GetFlickAccoladeBonusPoints(Accolade);
		FlickGameState->RecordPlayerAccolade(
			ResolutionShootingTeam,
			ShootingPlayerSlot,
			Accolade,
			BonusPoints);
		FlickGameState->ShowAccolade(Accolade, ResolutionShootingTeam, BonusPoints);
	}
}

void AFlickGameMode::AwardFlawlessRound(const EFlickMatchOutcome Outcome)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsBobMode())
	{
		return;
	}
	const EFlickTeam WinningTeam = Outcome == EFlickMatchOutcome::Player1Wins
		? EFlickTeam::Player1
		: Outcome == EFlickMatchOutcome::Player2Wins ? EFlickTeam::Player2 : EFlickTeam::None;
	if (WinningTeam == EFlickTeam::None
		|| !FlickAccoladeRules::IsFlawlessRound(
			CountActivePieces(WinningTeam),
			CurrentStartingPiecesPerTeam))
	{
		return;
	}
	for (int32 PlayerSlot = 0; PlayerSlot < CurrentPlayersPerTeam; ++PlayerSlot)
	{
		FlickGameState->RecordPlayerAccolade(WinningTeam, PlayerSlot, EFlickAccolade::FlawlessRound);
	}
	FlickGameState->ShowAccolade(EFlickAccolade::FlawlessRound, WinningTeam);
}

void AFlickGameMode::UpdateEliminations()
{
	bool bAnyEliminated = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		const FVector PieceLocation = Piece->GetActorLocation();
		const FVector ArenaLocation = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
		const float ActiveArenaRadius = ArenaActor ? ArenaActor->GetRadius() : ArenaRadius;
		const bool bBelowKillPlane = PieceLocation.Z <= KillZ;
		const bool bOutsideTabletop = ArenaActor && FlickModeRules::IsPieceOutsideCircularTabletop(
			PieceLocation,
			Piece->GetActorUpVector(),
			Piece->GetPieceRadius(),
			Piece->GetPieceThickness(),
			ArenaLocation,
			ActiveArenaRadius,
			ArenaSurfaceZ,
			KnockoutBoundsTolerance);
		if (bBelowKillPlane || bOutsideTabletop)
		{
			ResolutionEliminatedPieceIds.Add(Piece->GetPieceId());
			ResolutionEliminationTimes.FindOrAdd(Piece->GetPieceId()) = ResolutionElapsed;
			const EFlickTeam EliminatedTeam = Piece->GetTeam();
			if (EliminatedTeam == EFlickTeam::Player1)
			{
				++ResolutionPlayer1Eliminated;
			}
			else if (EliminatedTeam == EFlickTeam::Player2)
			{
				++ResolutionPlayer2Eliminated;
			}
			FVector EdgeDirection(
				PieceLocation.X - ArenaLocation.X,
				PieceLocation.Y - ArenaLocation.Y,
				0.0f);
			if (!EdgeDirection.Normalize())
			{
				EdgeDirection = FVector::ForwardVector;
			}
			const FVector FeedbackLocation = ArenaLocation
				+ EdgeDirection * (ActiveArenaRadius - 20.0f)
				+ FVector(0.0f, 0.0f, ArenaSurfaceZ + 22.0f - ArenaLocation.Z);

			SpawnWorldFeedback(
				FeedbackLocation,
				GetTeamColor(EliminatedTeam),
				EFlickFeedbackKind::Elimination,
				1.0f,
				EdgeDirection);
			if (AFlickGameState* FlickGameState = GetFlickGameState())
			{
				FlickGameState->RecordElimination(EliminatedTeam);
				const EFlickTeam CreditingTeam = GetOpposingTeam(EliminatedTeam);
				if (bResolvingKickoff || FlickGameState->LastShotTeam == CreditingTeam)
				{
					FlickGameState->RecordPlayerKnockout(
						CreditingTeam,
						FlickGameState->GetLastShootingPlayerSlot(CreditingTeam));
				}
			}
			PushHudEvent(
				FString::Printf(TEXT("PLAYER %d PUCK OUT"), GetTeamNumber(EliminatedTeam)),
				GetTeamColor(EliminatedTeam),
				2.2f);
			AddCameraFeedback(0.32f);
			AddControllerFeedback(0.78f, 0.24f);
			if (AudioDirector)
			{
				AudioDirector->PlayRingOut(FeedbackLocation);
			}
			Piece->Eliminate();
			bAnyEliminated = true;
		}
	}

	if (bAnyEliminated)
	{
		UpdateGameStateCounts();
	}
}

void AFlickGameMode::UpdateBobPockets()
{
	if (!BobArenaActor)
	{
		return;
	}

	bool bPocketedAnyPiece = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		if (!BobArenaActor->IsCapturedByPocket(Piece->GetActorLocation(), Piece->GetPieceRadius()))
		{
			continue;
		}

		const FVector PocketLocation(
			Piece->GetActorLocation().X,
			Piece->GetActorLocation().Y,
			ArenaSurfaceZ + 8.0f);
		if (Piece->IsBobStriker())
		{
			const bool bShooterPocketedOwnStriker = GetFlickGameState()
				&& GetFlickGameState()->CurrentTeam == Piece->GetTeam();
			if (Piece->GetTeam() == EFlickTeam::Player1)
			{
				bPlayer1BobStrikerPocketed = true;
			}
			else if (Piece->GetTeam() == EFlickTeam::Player2)
			{
				bPlayer2BobStrikerPocketed = true;
			}
			PushHudEvent(
				bShooterPocketedOwnStriker
					? TEXT("STRIKER POCKETED  |  PENALTY")
					: FString::Printf(TEXT("PLAYER %d STRIKER RETURNED"), GetTeamNumber(Piece->GetTeam())),
				bShooterPocketedOwnStriker
					? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f)
					: GetTeamColor(Piece->GetTeam()),
				2.4f);
		}
		else
		{
			const EFlickTeam ScoringTeam = Piece->GetTeam();
			if (AFlickGameState* FlickGameState = GetFlickGameState())
			{
				FlickGameState->RecordElimination(ScoringTeam);
				if (FlickGameState->LastShotTeam == ScoringTeam)
				{
					FlickGameState->RecordPlayerKnockout(
						ScoringTeam,
						FlickGameState->GetLastShootingPlayerSlot(ScoringTeam));
				}
			}
			PushHudEvent(
				FString::Printf(TEXT("PLAYER %d PUCK POCKETED"), GetTeamNumber(ScoringTeam)),
				GetTeamColor(ScoringTeam),
				2.0f);
		}

		SpawnWorldFeedback(
			PocketLocation,
			Piece->IsBobStriker() ? FLinearColor::White : GetTeamColor(Piece->GetTeam()),
			EFlickFeedbackKind::Elimination,
			0.82f);
		if (AudioDirector)
		{
			AudioDirector->PlayRingOut(PocketLocation);
		}
		Piece->Eliminate();
		bPocketedAnyPiece = true;
	}

	if (bPocketedAnyPiece)
	{
		AddCameraFeedback(0.2f);
		AddControllerFeedback(0.55f, 0.2f);
		UpdateGameStateCounts();
	}
}

void AFlickGameMode::UpdateBobPieceStability()
{
	if (!BobArenaActor)
	{
		return;
	}

	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive()
			|| Piece->GetLinearVelocity().Size() > BobSelfRightingMaxLinearSpeed
			|| Piece->GetAngularVelocityDegrees().Size() > BobSelfRightingMaxAngularSpeed
			|| !BobArenaActor->IsSafeForTabletopSelfRighting(
				Piece->GetActorLocation(),
				Piece->GetPieceRadius()))
		{
			continue;
		}

		Piece->ApplyTabletopSelfRighting(
			BobSelfRightingTorque,
			BobSelfRightingDamping,
			BobSelfRightingMinimumTilt);
	}
}

bool AFlickGameMode::IsPieceSafeOnClassicTabletop(const AFlickPiece* Piece) const
{
	if (!Piece || !IsValid(Piece) || !Piece->IsActive() || !ArenaActor)
	{
		return false;
	}
	const FVector ArenaLocation = ArenaActor->GetActorLocation();
	const FVector PieceLocation = Piece->GetActorLocation();
	const float SafeRadius = FMath::Max(
		0.0f,
		ArenaActor->GetRadius() - Piece->GetPieceRadius() * 1.35f);
	return FVector2D::DistSquared(
		FVector2D(PieceLocation.X, PieceLocation.Y),
		FVector2D(ArenaLocation.X, ArenaLocation.Y)) <= FMath::Square(SafeRadius)
		&& PieceLocation.Z >= ArenaSurfaceZ - Piece->GetPieceThickness();
}

void AFlickGameMode::UpdateClassicPieceStability()
{
	for (AFlickPiece* Piece : Pieces)
	{
		if (!IsPieceSafeOnClassicTabletop(Piece))
		{
			continue;
		}
		Piece->ApplyTabletopFlightContainment(
			ArenaSurfaceZ,
			TabletopMaximumUpwardSpeed,
			TabletopDownwardAcceleration);
		if (Piece->GetLinearVelocity().Size() <= BobSelfRightingMaxLinearSpeed
			&& Piece->GetAngularVelocityDegrees().Size() <= BobSelfRightingMaxAngularSpeed)
		{
			Piece->ApplyTabletopSelfRighting(
				TabletopSelfRightingTorque,
				TabletopSelfRightingDamping,
				TabletopSelfRightingMinimumTilt);
		}
	}
}

void AFlickGameMode::SettleClassicPiecesOnTabletop()
{
	for (AFlickPiece* Piece : Pieces)
	{
		if (IsPieceSafeOnClassicTabletop(Piece))
		{
			Piece->SettleFlatOnTabletop(ArenaSurfaceZ);
		}
	}
}

void AFlickGameMode::UpdateGameStateCounts() const
{
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetActivePieceCounts(
			CountActivePieces(EFlickTeam::Player1),
			CountActivePieces(EFlickTeam::Player2));
	}
}

bool AFlickGameMode::AreActivePiecesSettled() const
{
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		if (Piece->GetLinearVelocity().Size() > SleepLinearVelocityThreshold
			|| Piece->GetAngularVelocityDegrees().Size() > SleepAngularVelocityThreshold)
		{
			return false;
		}
	}
	return true;
}

bool AFlickGameMode::ApplyResolutionTimeoutCleanup()
{
	bool bHasMeaningfulMotion = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		const float LinearSpeed = Piece->GetLinearVelocity().Size();
		const float AngularSpeed = Piece->GetAngularVelocityDegrees().Size();
		if (LinearSpeed > SleepLinearVelocityThreshold * 6.0f || AngularSpeed > SleepAngularVelocityThreshold * 4.0f)
		{
			bHasMeaningfulMotion = true;
			continue;
		}

		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			RootPrimitive->PutRigidBodyToSleep();
		}
	}

	if (bHasMeaningfulMotion)
	{
		ResolutionElapsed = MaximumResolutionDuration * 0.5f;
		UE_LOG(LogFlick, Warning, TEXT("Resolution timeout reached, but at least one puck is still moving meaningfully. Extending resolution."));
		return false;
	}

	UE_LOG(LogFlick, Warning, TEXT("Resolution timeout cleanup used"));
	return true;
}

void AFlickGameMode::FinishPhysicsResolution(const bool bUsedTimeout)
{
	if (IsBobMode())
	{
		UpdateBobPockets();
	}
	else
	{
		UpdateEliminations();
		SettleClassicPiecesOnTabletop();
	}
	UpdateGameStateCounts();
	PresentDramaticResolutionEvent();
	PresentShotAccolades();
	ResolveTestArenaControlZones();
	CaptureRoundReplayFrame(true);
	// Send the authority's final sleeping/resting transforms immediately. This
	// prevents a remote physics proxy from keeping a locally divergent tilt after
	// the server has already declared the shot settled.
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->ForceNetUpdate();
		}
	}

	if (bLogPhysicsResolution)
	{
		UE_LOG(
			LogFlick,
			Log,
			TEXT("Physics resolved in %.2f s%s"),
			ResolutionElapsed,
			bUsedTimeout ? TEXT(" using timeout cleanup") : TEXT(""));
	}

	CheckWinOrAdvanceTurn();
}

void AFlickGameMode::CheckWinOrAdvanceTurn()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}
	if (IsTutorialMode())
	{
		ResolveTutorialShot();
		return;
	}
	if (IsFreePlayTraining())
	{
		ResolveTrainingTurn();
		return;
	}
	if (IsBobMode())
	{
		ResolveBobTurn();
		return;
	}
	const bool bCompletedKickoff = bResolvingKickoff;
	bResolvingKickoff = false;

	const int32 Player1Count = CountActivePieces(EFlickTeam::Player1);
	const int32 Player2Count = CountActivePieces(EFlickTeam::Player2);
	FlickGameState->SetActivePieceCounts(Player1Count, Player2Count);

	const EFlickMatchOutcome Outcome = EvaluateMatchOutcome(Player1Count, Player2Count);
	switch (Outcome)
	{
	case EFlickMatchOutcome::Draw:
	case EFlickMatchOutcome::Player2Wins:
	case EFlickMatchOutcome::Player1Wins:
		if (bTestArenaMode && Outcome != EFlickMatchOutcome::Draw && !bReplayPlayedForResolution)
		{
			BeginCinematicRoundReplay(Outcome);
			if (bCinematicReplayActive)
			{
				return;
			}
		}
		CompleteRoundForOutcome(Outcome);
		return;
	case EFlickMatchOutcome::Continue:
	default:
		break;
	}

	// A kickoff is neutral: after both shots resolve, the round's designated
	// starting team takes the first normal turn (blue in odd rounds, orange in even).
	FlickGameState->SetCurrentTeam(bCompletedKickoff
		? FlickGameState->RoundStartingTeam
		: GetOpposingTeam(FlickGameState->CurrentTeam));
	ActivateNextPlayerForTeam(FlickGameState->CurrentTeam);
	FlickGameState->AdvanceTurn();
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	SetCameraViewForTeam(FlickGameState->CurrentTeam);
	if (bCompletedKickoff)
	{
		PushHudEvent(
			TEXT("KICKOFF COMPLETE"),
			GetTeamColor(FlickGameState->CurrentTeam),
			1.5f);
	}
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(FlickGameState->CurrentTeam);
	}
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;

	UE_LOG(LogFlick, Log, TEXT("Turn changed to %s"), *GetTeamDisplayName(FlickGameState->CurrentTeam));
}

void AFlickGameMode::ResolveTrainingTurn()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}

	if (bPlayer1BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player1);
	}
	if (bPlayer2BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player2);
	}
	bPlayer1BobStrikerPocketed = false;
	bPlayer2BobStrikerPocketed = false;
	bResolvingKickoff = false;
	for (int32 PieceIndex = Pieces.Num() - 1; PieceIndex >= 0; --PieceIndex)
	{
		AFlickPiece* Piece = Pieces[PieceIndex];
		if (Piece && IsValid(Piece) && Piece->IsActive())
		{
			continue;
		}
		if (Piece && IsValid(Piece))
		{
			Piece->Destroy();
		}
		Pieces.RemoveAtSwap(PieceIndex, 1, EAllowShrinking::No);
	}
	UpdateGameStateCounts();

	FlickGameState->AdvanceTurn();
	FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
	ActivateNextPlayerForTeam(EFlickTeam::Player1);
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	SetCameraViewForTeam(EFlickTeam::Player1);
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(EFlickTeam::Player1);
	}
	UE_LOG(LogFlick, Log, TEXT("Training shot resolved; Player 1 remains active"));
}

void AFlickGameMode::CaptureTrainingResetSnapshot()
{
	if (!IsFreePlayTraining())
	{
		return;
	}

	TrainingResetSnapshot.Reset();
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		FFlickTrainingPieceSnapshot& Snapshot = TrainingResetSnapshot.AddDefaulted_GetRef();
		Snapshot.Team = Piece->GetTeam();
		Snapshot.Archetype = Piece->GetArchetype();
		Snapshot.Location = Piece->GetActorLocation();
		Snapshot.PieceId = Piece->GetPieceId();
		Snapshot.OwningPlayerSlot = Piece->GetOwningPlayerSlot();
		Snapshot.bBobStriker = Piece->IsBobStriker();
		Snapshot.bShowPlayerIdentity = Piece->ShowsPlayerIdentity();
	}
	bHasTrainingResetSnapshot = true;
	UE_LOG(LogFlick, Log, TEXT("Saved training reset setup with %d pucks"), TrainingResetSnapshot.Num());
}

void AFlickGameMode::RestoreTrainingResetSnapshot()
{
	if (!bTrainingMode || !bHasTrainingResetSnapshot)
	{
		return;
	}

	for (const FFlickTrainingPieceSnapshot& Snapshot : TrainingResetSnapshot)
	{
		AFlickPiece* Piece = SpawnPiece(
			Snapshot.Team,
			Snapshot.PieceId,
			Snapshot.Location,
			Snapshot.Archetype,
			Snapshot.bBobStriker,
			Snapshot.OwningPlayerSlot,
			Snapshot.bShowPlayerIdentity);
		if (!Piece)
		{
			continue;
		}
		if (Snapshot.bBobStriker)
		{
			if (Snapshot.Team == EFlickTeam::Player1)
			{
				Player1BobStriker = Piece;
			}
			else if (Snapshot.Team == EFlickTeam::Player2)
			{
				Player2BobStriker = Piece;
			}
		}
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
		{
			Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Primitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			Primitive->PutRigidBodyToSleep();
		}
	}
}

void AFlickGameMode::ResetTrainingBoard()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!bTrainingMode || !FlickGameState)
	{
		return;
	}

	ClearControllerAiming();
	DestroyPieces();
	if (bHasTrainingResetSnapshot)
	{
		RestoreTrainingResetSnapshot();
	}
	else
	{
		SpawnPieces();
		CaptureTrainingResetSnapshot();
	}
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	LastImpactFeedbackTime = -100.0f;
	LastStrongImpactEventTime = -100.0f;
	Player1NextPlayerSlot = 0;
	Player2NextPlayerSlot = 0;
	FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
	FlickGameState->SetCurrentTeamPlayerSlot(0);
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	UpdateGameStateCounts();
	SetCameraViewForTeam(EFlickTeam::Player1);
	PushHudEvent(TEXT("TRAINING SETUP RESTORED"), FLinearColor(0.2f, 0.78f, 0.5f, 1.0f), 2.0f);
	UE_LOG(LogFlick, Log, TEXT("Training board reset to saved setup"));
}

void AFlickGameMode::ResolveBobTurn()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}

	const EFlickTeam ShootingTeam = FlickGameState->CurrentTeam;
	const bool bShootingStrikerPocketed = ShootingTeam == EFlickTeam::Player1
		? bPlayer1BobStrikerPocketed
		: bPlayer2BobStrikerPocketed;
	if (bShootingStrikerPocketed)
	{
		const bool bRestoredPuck = RestoreBobPenaltyPiece(ShootingTeam);
		PushHudEvent(
			bRestoredPuck ? TEXT("PENALTY  |  OWN PUCK RETURNED") : TEXT("PENALTY  |  TURN LOST"),
			FLinearColor(1.0f, 0.72f, 0.12f, 1.0f),
			2.2f);
	}
	if (bPlayer1BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player1);
	}
	if (bPlayer2BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player2);
	}
	bPlayer1BobStrikerPocketed = false;
	bPlayer2BobStrikerPocketed = false;
	UpdateGameStateCounts();

	const int32 Player1Remaining = CountActivePieces(EFlickTeam::Player1);
	const int32 Player2Remaining = CountActivePieces(EFlickTeam::Player2);
	const EFlickMatchOutcome Outcome = FlickBobRules::EvaluateOutcome(
		Player1Remaining,
		Player2Remaining,
		ShootingTeam);
	if (Outcome != EFlickMatchOutcome::Continue)
	{
		CompleteRoundForOutcome(Outcome);
		return;
	}

	FlickGameState->SetCurrentTeam(GetOpposingTeam(ShootingTeam));
	ActivateNextPlayerForTeam(FlickGameState->CurrentTeam);
	FlickGameState->AdvanceTurn();
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	SetCameraViewForTeam(FlickGameState->CurrentTeam);
	PushHudEvent(
		FString::Printf(TEXT("PLAYER %d STRIKER ACTIVE"), GetTeamNumber(FlickGameState->CurrentTeam)),
		GetTeamColor(FlickGameState->CurrentTeam),
		1.6f);
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(FlickGameState->CurrentTeam);
	}
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	UE_LOG(
		LogFlick,
		Log,
		TEXT("BOB turn changed to %s. Both strikers remain at their settled positions."),
		*GetTeamDisplayName(FlickGameState->CurrentTeam));
}

void AFlickGameMode::CompleteRoundForOutcome(const EFlickMatchOutcome Outcome)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}
	if (!IsBobMode())
	{
		AwardFlawlessRound(Outcome);
		AwardRoundSurvivalPoints();
	}
	ResetShotClock();
	FlickGameState->CompleteRound(Outcome);
	SetCameraViewForTeam(EFlickTeam::Player1, true);
	FlickGameState->SetRoundAdvanceTimerState(
		!FlickGameState->bSeriesComplete,
		RoundAdvanceTimeLimit);
	if (FlickGameState->bSeriesComplete)
	{
		const EFlickMatchOutcome FinalOutcome = FlickGameState->WinnerTeam == EFlickTeam::Player1
			? EFlickMatchOutcome::Player1Wins
			: FlickGameState->WinnerTeam == EFlickTeam::Player2
				? EFlickMatchOutcome::Player2Wins
				: EFlickMatchOutcome::Draw;
		FlickGameState->FinalizeAuthoritativeMatch(FinalOutcome);
		DispatchRankedMatchResults();
		if (UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem())
		{
			Coordinator->NotifyServerMatchComplete(FinalOutcome, false);
		}
		UE_LOG(
			LogFlick,
			Log,
			TEXT("AUTHORITATIVE_MATCH_RESULT: id=%s outcome=%d forfeit=0"),
			*FlickGameState->MatchId,
			static_cast<int32>(FinalOutcome));
	}
	if (AudioDirector)
	{
		AudioDirector->PlayRoundResult(
			FlickGameState->WinnerTeam,
			FlickGameState->bDraw,
			FlickGameState->bSeriesComplete);
	}
	if (FlickGameState->bDraw)
	{
		PushHudEvent(
			TEXT("ROUND DRAW"),
			FLinearColor(1.0f, 0.8f, 0.15f, 1.0f),
			3.0f);
	}
	else
	{
		const FString ResultMessage = IsBobMode()
			? FString::Printf(TEXT("PLAYER %d CLEARS BOB"), GetTeamNumber(FlickGameState->WinnerTeam))
			: FlickGameState->bSeriesComplete
				? FString::Printf(TEXT("PLAYER %d WINS THE MATCH"), GetTeamNumber(FlickGameState->WinnerTeam))
				: FString::Printf(TEXT("PLAYER %d TAKES ROUND %d"), GetTeamNumber(FlickGameState->WinnerTeam), FlickGameState->RoundNumber);
		PushHudEvent(ResultMessage, GetTeamColor(FlickGameState->WinnerTeam), 3.0f);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s complete. Rounds %d-%d, scoreboard %d-%d%s"),
		IsBobMode() ? TEXT("BOB board") : *FString::Printf(TEXT("Round %d"), FlickGameState->RoundNumber),
		FlickGameState->Player1RoundsWon,
		FlickGameState->Player2RoundsWon,
		FlickGameState->GetTeamScore(EFlickTeam::Player1),
		FlickGameState->GetTeamScore(EFlickTeam::Player2),
		FlickGameState->bSeriesComplete ? TEXT(" (match complete)") : TEXT(""));
}

void AFlickGameMode::AwardRoundSurvivalPoints() const
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}

	int32 SurvivorCounts[2][3] = {};
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive() || Piece->IsBobStriker())
		{
			continue;
		}
		const int32 TeamIndex = Piece->GetTeam() == EFlickTeam::Player1
			? 0
			: Piece->GetTeam() == EFlickTeam::Player2 ? 1 : INDEX_NONE;
		const int32 PlayerSlot = Piece->GetOwningPlayerSlot();
		if (TeamIndex != INDEX_NONE && PlayerSlot >= 0 && PlayerSlot < 3)
		{
			++SurvivorCounts[TeamIndex][PlayerSlot];
		}
	}

	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		const EFlickTeam Team = TeamIndex == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2;
		for (int32 PlayerSlot = 0; PlayerSlot < 3; ++PlayerSlot)
		{
			FlickGameState->RecordPlayerSurvivingPucks(
				Team,
				PlayerSlot,
				SurvivorCounts[TeamIndex][PlayerSlot]);
		}
	}
}

AFlickPiece* AFlickGameMode::GetBobStriker(const EFlickTeam Team) const
{
	return Team == EFlickTeam::Player1
		? Player1BobStriker.Get()
		: Team == EFlickTeam::Player2
			? Player2BobStriker.Get()
			: nullptr;
}

void AFlickGameMode::ResetBobStrikerForTeam(const EFlickTeam Team)
{
	AFlickPiece* BobStriker = GetBobStriker(Team);
	if (!BobStriker || !BobArenaActor || Team == EFlickTeam::None)
	{
		return;
	}
	ResetBobPieceAt(
		BobStriker,
		Team,
		BobArenaActor->GetStrikerStart(Team, PieceThickness),
		true);
}

bool AFlickGameMode::RestoreBobPenaltyPiece(const EFlickTeam Team)
{
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && !Piece->IsBobStriker()
			&& Piece->GetTeam() == Team && Piece->IsEliminated())
		{
			ResetBobPieceAt(Piece, Team, FindBobRespawnLocation(), false);
			UpdateGameStateCounts();
			return true;
		}
	}
	return false;
}

FVector AFlickGameMode::FindBobRespawnLocation() const
{
	const float CandidateSpacing = PieceRadius * 2.55f;
	for (int32 RingIndex = 0; RingIndex <= 4; ++RingIndex)
	{
		const int32 CandidateCount = RingIndex == 0 ? 1 : RingIndex * 8;
		for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount; ++CandidateIndex)
		{
			const float Angle = CandidateCount == 1
				? 0.0f
				: 2.0f * PI * static_cast<float>(CandidateIndex) / CandidateCount;
			const FVector Candidate(
				FMath::Cos(Angle) * CandidateSpacing * RingIndex,
				FMath::Sin(Angle) * CandidateSpacing * RingIndex,
				ArenaSurfaceZ + PieceThickness * 0.5f + 3.0f);
			bool bBlocked = false;
			for (const AFlickPiece* OtherPiece : Pieces)
			{
				if (!OtherPiece || !IsValid(OtherPiece) || !OtherPiece->IsActive())
				{
					continue;
				}
				const FVector Delta = OtherPiece->GetActorLocation() - Candidate;
				if (FVector2D(Delta.X, Delta.Y).Size() < PieceRadius + OtherPiece->GetPieceRadius() + 7.0f)
				{
					bBlocked = true;
					break;
				}
			}
			if (!bBlocked && (!BobArenaActor || !BobArenaActor->IsInsidePocket(Candidate)))
			{
				return Candidate;
			}
		}
	}
	return FVector(0.0f, 0.0f, ArenaSurfaceZ + PieceThickness * 0.5f + 3.0f);
}

void AFlickGameMode::ResetBobPieceAt(
	AFlickPiece* Piece,
	const EFlickTeam Team,
	const FVector& Location,
	const bool bIsStriker) const
{
	if (!Piece)
	{
		return;
	}
	if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
	{
		Primitive->SetSimulatePhysics(false);
	}
	Piece->SetActorLocationAndRotation(
		Location,
		FRotator::ZeroRotator,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Piece->InitializePiece(
		Team,
		Piece->GetPieceId(),
		PieceRadius,
		PieceThickness,
		EFlickPieceArchetype::Standard,
		bIsStriker,
		Piece->GetOwningPlayerSlot());
}

void AFlickGameMode::SpawnWorldFeedback(
	const FVector& Location,
	const FLinearColor& Color,
	const EFlickFeedbackKind FeedbackKind,
	const float Strength,
	const FVector& BiasDirection) const
{
	if (!GetWorld() || !AreImpactEffectsEnabled())
	{
		return;
	}

	AFlickWorldFeedback* Feedback = GetWorld()->SpawnActor<AFlickWorldFeedback>(
		AFlickWorldFeedback::StaticClass(), Location, FRotator::ZeroRotator);
	if (Feedback)
	{
		Feedback->InitializeFeedback(FeedbackKind, Color, Strength, BiasDirection);
	}
}

void AFlickGameMode::PushHudEvent(
	const FString& Message,
	const FLinearColor& Color,
	const float Duration) const
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		if (AFlickHUD* FlickHUD = Cast<AFlickHUD>(PlayerController->GetHUD()))
		{
			FlickHUD->PushEventMessage(Message, Color, Duration);
		}
	}
}

AFlickPiece* AFlickGameMode::SpawnPiece(
	const EFlickTeam Team,
	const int32 PieceId,
	const FVector& Location,
	const EFlickPieceArchetype Archetype,
	const bool bIsBobStriker,
	const int32 OwningPlayerSlot,
	const bool bShowPlayerIdentity)
{
	AFlickPiece* Piece = GetWorld()->SpawnActor<AFlickPiece>(AFlickPiece::StaticClass(), Location, FRotator::ZeroRotator);
	if (!Piece)
	{
		return nullptr;
	}

	const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(Archetype);
	Piece->PieceMassKg = PieceMassKg * ArchetypeRules.MassMultiplier;
	Piece->PieceFriction = PieceFriction * ArchetypeRules.FrictionMultiplier;
	Piece->PieceRestitution = PieceRestitution * ArchetypeRules.RestitutionMultiplier;
	Piece->LinearDamping = LinearDamping * ArchetypeRules.LinearDampingMultiplier;
	Piece->AngularDamping = AngularDamping * ArchetypeRules.AngularDampingMultiplier;
	Piece->LaunchSpeedMultiplier = ArchetypeRules.LaunchSpeedMultiplier;
	Piece->CenterOfMassOffsetZ = PieceThickness
		* ArchetypeRules.ThicknessMultiplier
		* ArchetypeRules.CenterOfMassHeightFraction;
	Piece->bAllowEdgeTipping = bAllowEdgeTipping;
	Piece->bUseContinuousCollisionDetection = bUseContinuousCollisionDetection;
	Piece->PositionSolverIterations = PositionSolverIterations;
	Piece->VelocitySolverIterations = VelocitySolverIterations;
	Piece->InitializePiece(
		Team,
		PieceId,
		PieceRadius * ArchetypeRules.RadiusMultiplier,
		PieceThickness * ArchetypeRules.ThicknessMultiplier,
		Archetype,
		bIsBobStriker,
		OwningPlayerSlot,
		bShowPlayerIdentity);
	if (bTestArenaMode || IsBobMode())
	{
		Piece->EnableTestArenaVisuals();
	}
	const AFlickGameState* State = GetFlickGameState();
	if (bPregamePreviewActive && !(State && State->bPuckArrivalActive))
	{
		Piece->SetPregamePreview(true);
	}
	if (bPregamePreviewActive || (State && State->bPuckArrivalActive))
	{
		Piece->BeginArrival(PuckArrivalDuration);
	}
	Pieces.Add(Piece);
	return Piece;
}

UFlickGameInstance* AFlickGameMode::GetFlickGameInstance() const
{
	return GetGameInstance<UFlickGameInstance>();
}

UFlickSessionSubsystem* AFlickGameMode::GetFlickSessionSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickSessionSubsystem>() : nullptr;
}

UFlickMatchmakingCoordinatorSubsystem* AFlickGameMode::GetFlickMatchmakingCoordinatorSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>() : nullptr;
}

UFlickRankingSubsystem* AFlickGameMode::GetFlickRankingSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickRankingSubsystem>() : nullptr;
}

UFlickRankedBackendSubsystem* AFlickGameMode::GetFlickRankedBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickRankedBackendSubsystem>() : nullptr;
}
