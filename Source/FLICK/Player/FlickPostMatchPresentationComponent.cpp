#include "Player/FlickPostMatchPresentationComponent.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickCameraPawn.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

UFlickPostMatchPresentationComponent::UFlickPostMatchPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

float UFlickPostMatchPresentationComponent::GetElapsed() const
{
	const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
	const float Elapsed = State ? FMath::Max(0.0f, State->GetServerWorldTimeSeconds() - MatchStart) : 0.0f;
	return ContinuedAt >= 0.0f ? FMath::Max(Elapsed, ShowcaseSeconds + Elapsed - ContinuedAt) : Elapsed;
}

FVector UFlickPostMatchPresentationComponent::GetWinnerPosition(const int32 Slot) const
{
	const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
	return FVector((Slot - ((State ? State->PlayersPerTeam : 1) - 1) * 0.5f) * 190.0f, 0.0f,
		(State ? State->ArenaSurfaceZ : 250.0f) + 24.0f);
}

void UFlickPostMatchPresentationComponent::BuildWinner(const int32 Slot)
{
	const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
	if (!State || State->WinnerTeam == EFlickTeam::None) return;
	AFlickPiece* Source = nullptr;
	for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
	{
		if (It->GetTeam() == State->WinnerTeam && It->GetOwningPlayerSlot() == Slot)
		{
			Source = *It;
			if (Source->GetArchetype() == EFlickPieceArchetype::Standard || Source->IsBobStriker()) break;
		}
	}
	if (!Source) return; // Late-replicating pieces are retried on the next tick.
	UStaticMeshComponent* Mesh = Source->GetPresentationMesh();
	if (!Mesh || !Mesh->GetStaticMesh()) return;
	AActor* Display = GetWorld()->SpawnActor<AActor>();
	if (!Display) return;
	Display->SetReplicates(false);
	Display->SetActorTickEnabled(false);
	UStaticMeshComponent* Copy = NewObject<UStaticMeshComponent>(Display);
	Display->SetRootComponent(Copy);
	Display->AddInstanceComponent(Copy);
	Copy->SetStaticMesh(Mesh->GetStaticMesh());
	for (int32 Material = 0; Material < Mesh->GetNumMaterials(); ++Material)
		Copy->SetMaterial(Material, Mesh->GetMaterial(Material));
	Copy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Copy->SetGenerateOverlapEvents(false);
	Copy->RegisterComponent();
	// Copy the visual mesh's authored scale, not an additional gameplay cylinder.
	Copy->SetWorldScale3D(Mesh->GetComponentScale());
	const FVector Position = GetWinnerPosition(Slot);
	const float HalfHeight = Copy->Bounds.BoxExtent.Z;
	const float MeshBottom = Copy->Bounds.Origin.Z - HalfHeight;
	Display->SetActorLocation(FVector(Position.X, Position.Y, State->ArenaSurfaceZ + 2.0f - MeshBottom));
	DisplayPucks[Slot] = Display;
}

void UFlickPostMatchPresentationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	AFlickPlayerController* Controller = Cast<AFlickPlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController()) return;
	const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
	const AFlickGameMode* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	const bool bShouldPresent = State && State->bSeriesComplete && State->MatchPhase == EFlickMatchPhase::RoundOver
		&& !Controller->IsCinematicReplayPresentationActive()
		&& (!Mode || (Mode->GetFrontendScreen() == EFlickFrontendScreen::Playing && !Mode->IsCinematicReplayActive()));
	if (!bShouldPresent) { if (bActive) ClearPresentation(); return; }
	if (!bActive || MatchId != State->MatchId || MatchStart != State->PostMatchStartServerTime)
	{
		ClearPresentation();
		bActive = true;
		MatchId = State->MatchId;
		MatchStart = State->PostMatchStartServerTime;
		DisplayPucks.SetNum(State->PlayersPerTeam);
		for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
		{
			TInlineComponentArray<UPrimitiveComponent*> Components(*It);
			for (UPrimitiveComponent* Component : Components)
				HiddenComponents.Add(Component, Component->bHiddenInGame);
		}
	}
	for (const auto& Entry : HiddenComponents)
		if (UPrimitiveComponent* Component = Entry.Key.Get()) Component->SetHiddenInGame(true);
	for (int32 Slot = 0; Slot < DisplayPucks.Num(); ++Slot)
		if (!IsValid(DisplayPucks[Slot])) BuildWinner(Slot);
	if (bContinueRequested && ContinuedAt < 0.0f)
		ContinuedAt = FMath::Max(0.0f, State->GetServerWorldTimeSeconds() - MatchStart);
	if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(Controller->GetPawn()))
		Camera->SetPostMatchPresentation(true, FMath::Clamp((GetElapsed() - ShowcaseSeconds) / PullbackSeconds, 0.0f, 1.0f),
			State->PlayersPerTeam, State->ArenaSurfaceZ);
}

void UFlickPostMatchPresentationComponent::ClearPresentation()
{
	for (AActor* Display : DisplayPucks) if (IsValid(Display)) Display->Destroy();
	DisplayPucks.Reset();
	for (const auto& Entry : HiddenComponents)
		if (UPrimitiveComponent* Component = Entry.Key.Get()) Component->SetHiddenInGame(Entry.Value);
	HiddenComponents.Reset();
	if (AFlickPlayerController* Controller = Cast<AFlickPlayerController>(GetOwner()))
		if (AFlickCameraPawn* Camera = Cast<AFlickCameraPawn>(Controller->GetPawn()))
			Camera->SetPostMatchPresentation(false, 0.0f, 1, 250.0f);
	bActive = false;
	bContinueRequested = false;
	ContinuedAt = -1.0f;
}

void UFlickPostMatchPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearPresentation();
	Super::EndPlay(Reason);
}
