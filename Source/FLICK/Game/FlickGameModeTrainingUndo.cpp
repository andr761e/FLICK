#include "Game/FlickGameModePrivate.h"

bool AFlickGameMode::CanUndoTrainingShot() const
{
	const AFlickGameState* State = GetFlickGameState();
	return HasAuthority() && GetNetMode() == NM_Standalone && IsFreePlayTraining()
		&& !bTrainingEditMode && bHasTrainingUndoSnapshot && !bCinematicReplayActive
		&& FrontendScreen == EFlickFrontendScreen::Playing && State && !State->bPuckArrivalActive
		&& (State->MatchPhase == EFlickMatchPhase::Aiming || State->MatchPhase == EFlickMatchPhase::ResolvingPhysics);
}

void AFlickGameMode::CaptureTrainingUndoSnapshot()
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State || !IsFreePlayTraining() || GetNetMode() != NM_Standalone) return;
	TrainingUndoSnapshot = FFlickTrainingUndoSnapshot();
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!IsValid(Piece) || !Piece->IsActive()) continue;
		FFlickTrainingUndoPiece& Entry = TrainingUndoSnapshot.Pieces.AddDefaulted_GetRef();
		Entry.Team = Piece->GetTeam();
		Entry.Archetype = Piece->GetArchetype();
		Entry.PieceId = Piece->GetPieceId();
		Entry.OwningPlayerSlot = Piece->GetOwningPlayerSlot();
		Entry.bBobStriker = Piece->IsBobStriker();
		Entry.PuckSkin = Piece->GetPuckSkin();
		Entry.Transform = Piece->GetActorTransform();
		Entry.LinearVelocity = Piece->GetLinearVelocity();
		Entry.AngularVelocity = Piece->GetAngularVelocityDegrees();
		for (int32 Effect = 0; Effect < 3; ++Effect) Entry.Effects.Add(Piece->GetPuckEffect(Effect));
		if (auto* Body = Cast<UPrimitiveComponent>(Piece->GetRootComponent())) Entry.bAwake = Body->IsAnyRigidBodyAwake();
	}
	TrainingUndoSnapshot.DividerMask = TestArenaActor ? TestArenaActor->GetRaisedDividerMask() : 0;
	TrainingUndoSnapshot.PlayerStats = State->PlayerMatchStats;
	TrainingUndoSnapshot.TurnNumber = State->TurnNumber;
	TrainingUndoSnapshot.Player1Shots = State->Player1ShotsTaken;
	TrainingUndoSnapshot.Player2Shots = State->Player2ShotsTaken;
	TrainingUndoSnapshot.CurrentSlot = State->CurrentTeamPlayerSlot;
	TrainingUndoSnapshot.Player1NextSlot = Player1NextPlayerSlot;
	TrainingUndoSnapshot.Player2NextSlot = Player2NextPlayerSlot;
	bHasTrainingUndoSnapshot = true;
}

bool AFlickGameMode::UndoTrainingShot()
{
	if (!CanUndoTrainingShot()) return false;
	// DestroyPieces invalidates history on every board rebuild. Take this one-step copy first.
	const FFlickTrainingUndoSnapshot Snapshot = TrainingUndoSnapshot;
	ClearControllerAiming();
	DestroyPieces();
	for (TActorIterator<AFlickWorldFeedback> It(GetWorld()); It; ++It) It->Destroy();
	if (TestArenaActor) TestArenaActor->RestoreTrainingMechanisms(Snapshot.DividerMask);
	AFlickGameState* State = GetFlickGameState();
	for (const FFlickTrainingUndoPiece& Entry : Snapshot.Pieces)
	{
		AFlickPiece* Piece = SpawnPiece(Entry.Team, Entry.PieceId, Entry.Transform.GetLocation(), Entry.Archetype, Entry.bBobStriker, Entry.OwningPlayerSlot);
		if (!Piece) continue;
		Piece->SetPuckSkin(Entry.PuckSkin);
		Piece->SetPuckEffects(Entry.Effects);
		Piece->SetActorTransform(Entry.Transform, false, nullptr, ETeleportType::TeleportPhysics);
		if (auto* Body = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
		{
			Body->SetPhysicsLinearVelocity(Entry.LinearVelocity);
			Body->SetPhysicsAngularVelocityInDegrees(Entry.AngularVelocity);
			if (Entry.bAwake) Body->WakeAllRigidBodies(); else Body->PutAllRigidBodiesToSleep();
		}
		if (Entry.bBobStriker)
		{
			if (Entry.Team == EFlickTeam::Player1) Player1BobStriker = Piece;
			else if (Entry.Team == EFlickTeam::Player2) Player2BobStriker = Piece;
		}
	}
	BeginResolutionTracking(EFlickTeam::None, false, nullptr);
	SettledElapsed = 0;
	LastImpactFeedbackTime = LastStrongImpactEventTime = -100.f;
	Player1NextPlayerSlot = Snapshot.Player1NextSlot;
	Player2NextPlayerSlot = Snapshot.Player2NextSlot;
	State->PlayerMatchStats = Snapshot.PlayerStats;
	State->TurnNumber = Snapshot.TurnNumber;
	State->Player1ShotsTaken = Snapshot.Player1Shots;
	State->Player2ShotsTaken = Snapshot.Player2Shots;
	State->LastShotTeam = EFlickTeam::None;
	State->LastShotPieceId = INDEX_NONE;
	State->LastShotPower = 0;
	State->ImpactsThisShot = State->Player1EliminatedThisShot = State->Player2EliminatedThisShot = 0;
	State->StrongestImpactThisShot = 0;
	State->AccoladeFeedEvents.Reset();
	State->SetCurrentTeam(EFlickTeam::Player1);
	State->SetCurrentTeamPlayerSlot(Snapshot.CurrentSlot);
	State->SetMatchPhase(EFlickMatchPhase::Aiming);
	UpdateGameStateCounts();
	SetCameraViewForTeam(EFlickTeam::Player1);
	PushHudEvent(TEXT("LAST SHOT UNDONE"), FLinearColor(.2f, .78f, .5f, 1.f), 1.5f);
	return true;
}
