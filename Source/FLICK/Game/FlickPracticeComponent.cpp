#include "Game/FlickPracticeComponent.h"
#include "Game/FlickGameModePrivate.h"
#include "Core/FlickPracticeCatalog.h"
#include "Feedback/FlickCosmeticGeometry.h"
#include "Feedback/FlickCosmeticMaterial.h"

UFlickPracticeComponent::UFlickPracticeComponent() { PrimaryComponentTick.bCanEverTick = false; }

bool AFlickGameMode::IsPracticeMode() const { return IsTutorialMode() && Practice && Practice->IsRunning(); }

void AFlickGameMode::StartPractice(int32 Category, int32 Difficulty)
{
	if (!FlickPracticeCatalog::IsValid(Category, Difficulty) || GetNetMode() != NM_Standalone
		|| bNetworkMatchRequested || bPartyRequested || bMatchmakingRequested) return;
	SelectedMatchVariant = Category == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
	MatchmakingPlayersPerTeam = 1; bTutorialMode = true; bTutorialCompleted = false;
	Practice->Stop(); BeginTrainingActivity(false); Practice->Start(Category, Difficulty);
}

void UFlickPracticeComponent::Stop()
{
	Category = INDEX_NONE; TransitionRemaining = 0; bComplete = false;
	if (MarkerActor) MarkerActor->Destroy();
	Marker = nullptr; TargetMarker = nullptr; MarkerActor = nullptr;
}

void UFlickPracticeComponent::Start(int32 InCategory, int32 InDifficulty)
{
	if (!FlickPracticeCatalog::IsValid(InCategory, InDifficulty)) return;
	Stop(); Category = InCategory; Difficulty = InDifficulty;
	if (auto* Mode = Cast<AFlickGameMode>(GetOwner()); Mode && Mode->TestArenaActor)
		Mode->TestArenaActor->SetTrainingLayoutSeed(LayoutSeed);
	Restart();
}

void UFlickPracticeComponent::Restart()
{
	if (!IsRunning()) return;
	Challenge = Score = 0; bComplete = false; Results.Reset(); SetupChallenge();
}

void UFlickPracticeComponent::ShowMarker(const FVector& Location, float Radius, bool bTargetMarker)
{
	auto& Ring = bTargetMarker ? TargetMarker : Marker;
	if (!Ring)
	{
		// GameMode is an invisible AInfo: use a visible presentation-only host.
		if (!MarkerActor) MarkerActor = GetWorld()->SpawnActor<AActor>();
		if (!MarkerActor) return;
		MarkerActor->SetActorEnableCollision(false);
		Ring = NewObject<UProceduralMeshComponent>(MarkerActor);
		if (!bTargetMarker) MarkerActor->SetRootComponent(Ring);
		else { Ring->SetupAttachment(Marker); Ring->SetAbsolute(true, true, true); }
		MarkerActor->AddInstanceComponent(Ring);
		Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ring->SetGenerateOverlapEvents(false); Ring->SetCastShadow(false); Ring->SetCanEverAffectNavigation(false);
		Ring->RegisterComponent();
		FlickCosmeticGeometry::BuildRing(*Ring, 0);
		auto* Material = FlickCosmeticMaterial::CreateGlow(this);
		FlickCosmeticMaterial::SetGlow(Material, FLinearColor(.55f, 1.f, .04f));
		Ring->SetMaterial(0, Material);
	}
	Ring->SetWorldLocation(Location); Ring->SetWorldScale3D(FVector(Radius, Radius, 1)); Ring->SetVisibility(true);
}

void UFlickPracticeComponent::SetupChallenge()
{
	auto* Mode = Cast<AFlickGameMode>(GetOwner());
	if (!Mode || !Mode->IsTutorialMode() || !IsRunning()) return;
	Mode->ClearControllerAiming(); Mode->DestroyPieces(); Mode->ResetShotClock();
	if (Mode->TestArenaActor) Mode->TestArenaActor->ResetMechanisms();
	Mode->ResolutionElapsed = Mode->SettledElapsed = 0;
	Mode->bPlayer1BobStrikerPocketed = Mode->bPlayer2BobStrikerPocketed = false;
	TransitionRemaining = 0; Feedback.Reset(); ShooterId = TargetId = GuardId = SwitchIndex = INDEX_NONE;
	int32 Id = 2001;
	const auto Spawn = [Mode, &Id](EFlickTeam Team, EFlickPieceArchetype Type, FVector2D Position, bool bStriker = false)
	{
		const float Height = Mode->ArenaSurfaceZ + Mode->PieceThickness * FlickPieceArchetypeRules::Get(Type).ThicknessMultiplier * .5f + 3.f;
		return Mode->SpawnPiece(Team, Id++, FVector(Position.X, Position.Y, Height), Type, bStriker, 0);
	};
	static const float Angles[] = {-22.f, 0.f, 28.f, -36.f, 14.f};
	const float Angle = FMath::DegreesToRadians(Angles[Challenge]);
	const FVector2D Direction(FMath::Sin(Angle), FMath::Cos(Angle));
	const FVector2D Side(Direction.Y, -Direction.X);
	AFlickPiece* Shooter = nullptr;
	AFlickPiece* Target = nullptr;
	AFlickPiece* Guard = nullptr;
	FVector MarkerPosition(0, 0, Mode->ArenaSurfaceZ + 3.f);
	float MarkerRadius = 50;
	if (Category == 0)
	{
		ZoneCenter = Side * ((Challenge - 2) * 35.f);
		ZoneRadius = FMath::Lerp(FoundationZoneRadius, ExpertZoneRadius, Difficulty * .5f);
		Shooter = Spawn(EFlickTeam::Player1, EFlickPieceArchetype::Standard, ZoneCenter - Direction * (260.f + Difficulty * 90.f + Challenge * 15.f));
		MarkerPosition.X = ZoneCenter.X; MarkerPosition.Y = ZoneCenter.Y; MarkerRadius = ZoneRadius;
	}
	else if (Category == 1)
	{
		const FVector2D TargetPosition = Direction * (Mode->ArenaRadius - 75.f - Difficulty * 55.f);
		Shooter = Spawn(EFlickTeam::Player1, EFlickPieceArchetype::Striker, TargetPosition - Direction * (240.f + Difficulty * 65.f) + Side * (Difficulty * (Challenge % 2 ? 35.f : -35.f)));
		Target = Spawn(EFlickTeam::Player2, EFlickPieceArchetype::Compact, TargetPosition);
		if (Difficulty > 0) Guard = Spawn(EFlickTeam::Player1, EFlickPieceArchetype::Standard, TargetPosition + Side * 115.f - Direction * 55.f);
		MarkerPosition.X = TargetPosition.X; MarkerPosition.Y = TargetPosition.Y; MarkerRadius = Target ? Target->GetPieceRadius() + 8.f : 50;
	}
	else if (Category == 2 && Mode->TestArenaActor && Mode->TestArenaActor->GetMechanismCount() > 0)
	{
		SwitchIndex = (Challenge * 3 + Difficulty) % Mode->TestArenaActor->GetMechanismCount();
		MarkerPosition = Mode->TestArenaActor->GetSwitchWorldCenter(SwitchIndex); MarkerPosition.Z = Mode->ArenaSurfaceZ + 3;
		const FVector2D SwitchPosition(MarkerPosition.X, MarkerPosition.Y);
		const FVector2D Outward = SwitchPosition.GetSafeNormal();
		const FVector2D Tangent(Outward.Y, -Outward.X);
		Shooter = Spawn(EFlickTeam::Player1, EFlickPieceArchetype::Bouncer, SwitchPosition - Outward * (210.f + Difficulty * 65.f) + Tangent * (Difficulty * (Challenge % 2 ? 60.f : -60.f)));
		MarkerRadius = Mode->TestArenaActor->ControlZoneRadius;
	}
	else if (Category == 3 && Mode->BobArenaActor)
	{
		MarkerPosition = Mode->BobArenaActor->GetPocketWorldLocation(Challenge % 4); MarkerPosition.Z = Mode->ArenaSurfaceZ + 3;
		const FVector2D Pocket(MarkerPosition.X, MarkerPosition.Y);
		const FVector2D Inward = -Pocket.GetSafeNormal();
		const FVector2D Tangent(Inward.Y, -Inward.X);
		const FVector2D TargetPosition = Pocket + Inward * (110.f + Difficulty * 80.f);
		Shooter = Spawn(EFlickTeam::Player1, EFlickPieceArchetype::Standard, TargetPosition + Inward * (200.f + Challenge * 12.f) + Tangent * (Difficulty * (Challenge % 2 ? 65.f : -65.f)), true);
		Target = Spawn(EFlickTeam::Player1, EFlickPieceArchetype::Standard, TargetPosition);
		Mode->Player1BobStriker = Shooter;
		MarkerRadius = Mode->BobArenaActor->GetPocketRadius();
	}
	ShooterId = Shooter ? Shooter->GetPieceId() : INDEX_NONE;
	TargetId = Target ? Target->GetPieceId() : INDEX_NONE; GuardId = Guard ? Guard->GetPieceId() : INDEX_NONE;
	ShowMarker(MarkerPosition, MarkerRadius);
	if (Category == 3 && Target)
	{
		FVector TargetLocation = Target->GetActorLocation(); TargetLocation.Z = Mode->ArenaSurfaceZ + 3;
		ShowMarker(TargetLocation, Target->GetPieceRadius() + 10, true);
	}
	if (auto* State = Mode->GetFlickGameState())
	{
		State->SetPuckArrivalState(false); State->ClearDramaticEvent(); State->AccoladeFeedEvents.Reset();
		State->TurnNumber = Challenge + 1; State->SetCurrentTeam(EFlickTeam::Player1); State->SetCurrentTeamPlayerSlot(0);
		State->SetMatchPhase(EFlickMatchPhase::Aiming); Mode->UpdateGameStateCounts();
	}
	Mode->SetCameraViewForTeam(EFlickTeam::Player1, true);
}

void UFlickPracticeComponent::ResolveShot()
{
	auto* Mode = Cast<AFlickGameMode>(GetOwner());
	if (!Mode || !IsRunning() || bComplete || TransitionRemaining > 0) return;
	const bool bShooterLost = Mode->ResolutionEliminatedPieceIds.Contains(ShooterId) || Mode->bPlayer1BobStrikerPocketed;
	const bool bGuardLost = GuardId != INDEX_NONE && Mode->ResolutionEliminatedPieceIds.Contains(GuardId);
	bool bSuccess = false;
	if (Category == 0)
	{
		for (const AFlickPiece* Piece : Mode->Pieces)
			if (IsValid(Piece) && Piece->GetPieceId() == ShooterId && Piece->IsActive())
				// The whole gameplay footprint must fit, not just the puck's centre.
				bSuccess = FVector2D::Distance(FVector2D(Piece->GetActorLocation()), ZoneCenter)
					+ Piece->GetPieceRadius() <= ZoneRadius;
	}
	else if (Category == 2)
		bSuccess = SwitchIndex >= 0 && (Mode->ResolutionActivatedSwitchMask & (1 << SwitchIndex)) != 0
			&& (Difficulty < 2 || Mode->ResolutionActivatedSwitchMask == (1 << SwitchIndex));
	else if (Category == 3)
	{
		// BOB removes captured pucks from active play, rather than recording a
		// circular-arena knockout in ResolutionEliminatedPieceIds.
		for (const AFlickPiece* Piece : Mode->Pieces)
			if (IsValid(Piece) && Piece->GetPieceId() == TargetId) bSuccess = !Piece->IsActive();
	}
	else bSuccess = TargetId != INDEX_NONE && Mode->ResolutionEliminatedPieceIds.Contains(TargetId);
	bSuccess &= !bShooterLost && !bGuardLost;
	Results.Add(bSuccess ? 1 : 0); Score += bSuccess;
	Feedback = bSuccess ? TEXT("PASS — clean shot!") : bShooterLost ? TEXT("MISS — keep your shooting puck on the board.")
		: bGuardLost ? TEXT("MISS — protect the friendly puck.") : Category == 0 ? TEXT("MISS — the entire puck must stop inside the green circle.")
		: Category == 2 ? TEXT("MISS — cross the marked switch; Expert requires only that switch.")
		: Category == 3 ? TEXT("MISS — aim through the marked puck toward a pocket.") : TEXT("MISS — push the marked opponent over the edge.");
	TransitionRemaining = FMath::Max(.5f, ResultDisplaySeconds);
	if (auto* State = Mode->GetFlickGameState()) State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
}

void UFlickPracticeComponent::Update(float DeltaSeconds)
{
	if (!IsRunning() || bComplete || TransitionRemaining <= 0) return;
	TransitionRemaining = FMath::Max(0.f, TransitionRemaining - DeltaSeconds);
	if (TransitionRemaining > 0) return;
	if (++Challenge < FlickPracticeCatalog::ChallengesPerRun) { SetupChallenge(); return; }
	bComplete = true; if (Marker) Marker->SetVisibility(false);
	if (TargetMarker) TargetMarker->SetVisibility(false);
	FlickPracticeCatalog::RecordScore(Category, Difficulty, Score);
}

FString UFlickPracticeComponent::GetObjective() const
{
	if (bComplete) return Score == 5 ? TEXT("PERFECT RUN! This pack is mastered.") : TEXT("Run complete. Replay the pack to improve your best score.");
	return Category == 0 ? TEXT("Stop your entire puck inside the green circle with one shot.")
		: Category == 1 ? TEXT("Knock out the marked opponent. Keep every friendly puck safe.")
		: Category == 2 ? (Difficulty == 2 ? TEXT("Activate only the marked switch and stay on the board.") : TEXT("Activate the marked switch and stay on the board."))
		: TEXT("Pocket the marked puck without pocketing your striker.");
}

FString UFlickPracticeComponent::GetFeedback() const
{
	if (bComplete) return FString::Printf(TEXT("Score %d / 5  |  Best %d / 5. Restart replays all five shots."), Score, FlickPracticeCatalog::BestScore(Category, Difficulty));
	return !Feedback.IsEmpty() ? Feedback : TEXT("One attempt per challenge. Restart begins a fresh run. Best scores save after all five shots.");
}
