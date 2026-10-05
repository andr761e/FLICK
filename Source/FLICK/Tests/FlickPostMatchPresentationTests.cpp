#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPostMatchPresentationComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPostMatchPresentationTest, "FLICK.Match.PostMatchArenaPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPostMatchPresentationTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("World"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickGameState* State = World->SpawnActor<AFlickGameState>();
	World->SetGameState(State);
	AFlickPlayerController* Controller = World->SpawnActor<AFlickPlayerController>();
	Controller->SetAsLocalPlayerController();
	World->AddController(Controller);
	Controller->Possess(World->SpawnActor<AFlickCameraPawn>());
	auto* View = Controller->FindComponentByClass<UFlickPostMatchPresentationComponent>();
	TestNotNull(TEXT("Local controller has presentation component"), View);
	if (View)
	{
		for (int32 Count = 1; Count <= 3; ++Count)
		{
			State->SetTeamFormat(Count);
			State->ResetSeriesState(3);
			State->MatchId = FString::Printf(TEXT("presentation-%d"), Count);
			State->ArenaSurfaceZ = 250.0f;
			TArray<AFlickPiece*> Pieces;
			TArray<FTransform> Transforms;
			for (int32 Slot = 0; Slot < Count; ++Slot)
			{
				AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
				Piece->InitializePiece(EFlickTeam::Player2, Slot + 1, 46, 36, EFlickPieceArchetype::Standard, false, Slot);
				Piece->SetPregamePreview(true);
				Piece->SetActorLocation(FVector(Slot * 150, 200, 300));
				Pieces.Add(Piece);
				Transforms.Add(Piece->GetActorTransform());
			}
			State->bSeriesComplete = true;
			State->WinnerTeam = EFlickTeam::Player2;
			State->MatchPhase = EFlickMatchPhase::RoundOver;
			State->PostMatchStartServerTime = State->GetServerWorldTimeSeconds();
			View->TickComponent(.016f, LEVELTICK_All, nullptr);
			TestFalse(TEXT("Winner showcase precedes summary"), View->ShowsDetails());
			int32 DisplayCount = 0;
			for (TActorIterator<AActor> It(World); It; ++It)
				if (It->GetClass() == AActor::StaticClass() && It->FindComponentByClass<UStaticMeshComponent>())
				{
					++DisplayCount;
					TestFalse(TEXT("Display pucks never replicate"), It->GetIsReplicated());
					TestEqual(TEXT("Display pucks have no collision"), It->FindComponentByClass<UStaticMeshComponent>()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
				}
			TestEqual(TEXT("One visual-only puck per winning player"), DisplayCount, Count);
			for (int32 Slot = 0; Slot < Count; ++Slot)
				TestTrue(TEXT("Presentation preserves gameplay transforms"), Pieces[Slot]->GetActorTransform().Equals(Transforms[Slot]));
			View->ContinueToSummary();
			View->TickComponent(.016f, LEVELTICK_All, nullptr);
			TestFalse(TEXT("Continue still allows camera pullback before menu"), View->ShowsDetails());
			// Re-entering gameplay removes all local copies and restores visibility.
			State->bSeriesComplete = false;
			View->TickComponent(.016f, LEVELTICK_All, nullptr);
			for (AFlickPiece* Piece : Pieces)
			{
				TestFalse(TEXT("Original mesh is no longer presentation-hidden"), Piece->GetPresentationMesh()->bHiddenInGame);
				Piece->Destroy();
			}
		}
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
