#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Arena/FlickTestArena.h"
#include "Components/PrimitiveComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickTrainingUndoTest, "FLICK.Training.ShotUndo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickTrainingUndoTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Undo test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
	auto* Mode = World->SpawnActor<AFlickGameMode>(); Mode->GameState = State;
	Mode->bTrainingMode = true;
	Mode->FrontendScreen = EFlickFrontendScreen::Playing;
	Mode->ArenaSurfaceZ = 250;
	State->InitializePlayerMatchStats(1);
	State->SetMatchPhase(EFlickMatchPhase::Aiming);
	TestFalse(TEXT("No undo before a shot"), Mode->UndoTrainingShot());
	for (int32 Board = 0; Board < 3; ++Board)
	{
		const EFlickMatchVariant Variant = Board == 2 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
		Mode->DestroyPieces();
		Mode->SelectedMatchVariant = Variant;
		Mode->ActiveMatchVariant = Variant;
		Mode->bTestArenaMode = Board == 1;
		if (Mode->bTestArenaMode)
		{
			Mode->TestArenaActor = World->SpawnActor<AFlickTestArena>();
			Mode->TestArenaActor->InitializeTestArena(650, 50, 250, 1);
			Mode->TestArenaActor->RestoreTrainingMechanisms(1);
		}
		State->SetMatchPhase(EFlickMatchPhase::Aiming);
		AFlickPiece* Own = Mode->SpawnPiece(EFlickTeam::Player1, 10, FVector(-150, 0, 265), EFlickPieceArchetype::Standard, Variant == EFlickMatchVariant::Bob);
		AFlickPiece* Target = Mode->SpawnPiece(EFlickTeam::Player2, 20, FVector(150, 0, 265), EFlickPieceArchetype::Standard);
		Own->SetActorRotation(FRotator(0, 37, 0), ETeleportType::TeleportPhysics);
		Own->SetPuckSkin(1); Own->SetPuckEffects({1, 1, 1});
		const FTransform Before = Own->GetActorTransform();
		const float Radius = Own->GetPieceRadius(), Thickness = Own->GetPieceThickness();
		const int32 TurnBefore = State->TurnNumber, ShotsBefore = State->Player1ShotsTaken;
		Mode->CaptureTrainingResetSnapshot();
		TestTrue(TEXT("Free Play launch succeeds"), Mode->TryLaunchPiece(Own, FVector::ForwardVector, .5f));
		TestTrue(TEXT("Undo available during motion"), Mode->CanUndoTrainingShot());
		Own->SetActorLocation(FVector(500, 0, 300), false, nullptr, ETeleportType::TeleportPhysics);
		Target->Eliminate(); Target->Destroy(); Mode->Pieces.Remove(Target);
		State->RecordPlayerKnockout(EFlickTeam::Player1, 0);
		State->AdvanceTurn();
		if (Mode->TestArenaActor) Mode->TestArenaActor->RestoreTrainingMechanisms(2);
		TestTrue(TEXT("Undo restores the last shot"), Mode->UndoTrainingShot());
		TestEqual(TEXT("Eliminated puck returns"), Mode->Pieces.Num(), 2);
		AFlickPiece* Restored = Mode->Pieces[0];
		TestTrue(TEXT("Position and rotation restored"), Restored->GetActorTransform().Equals(Before, .01f));
		TestTrue(TEXT("Velocity restored to pre-shot state"), Restored->GetLinearVelocity().IsNearlyZero());
		TestEqual(TEXT("Puck ID preserved"), Restored->GetPieceId(), 10);
		TestEqual(TEXT("Skin preserved"), Restored->GetPuckSkin(), 1);
		TestEqual(TEXT("Effects preserved"), Restored->GetPuckEffect(0), 1);
		TestEqual(TEXT("Physics radius unchanged"), Restored->GetPieceRadius(), Radius);
		TestEqual(TEXT("Physics thickness unchanged"), Restored->GetPieceThickness(), Thickness);
		TestEqual(TEXT("Turn restored"), State->TurnNumber, TurnBefore);
		TestEqual(TEXT("Shot count restored"), State->Player1ShotsTaken, ShotsBefore);
		TestEqual(TEXT("Back in aiming phase"), State->MatchPhase, EFlickMatchPhase::Aiming);
		if (Variant == EFlickMatchVariant::Bob) TestEqual(TEXT("BOB striker reference restored"), Mode->Player1BobStriker.Get(), Restored);
		if (Mode->TestArenaActor) TestEqual(TEXT("Raised dividers restored"), Mode->TestArenaActor->GetRaisedDividerMask(), static_cast<uint16>(1));
		TestTrue(TEXT("Saved setup remains separate"), Mode->bHasTrainingResetSnapshot && Mode->TrainingResetSnapshot.Num() == 2);
		TestFalse(TEXT("Undo is one step, not repeated backwards"), Mode->CanUndoTrainingShot());
		Restored->SetActorLocation(FVector(-100, 50, 265), false, nullptr, ETeleportType::TeleportPhysics);
		const FVector NewStart = Restored->GetActorLocation();
		TestTrue(TEXT("Retry takes a fresh snapshot"), Mode->TryLaunchPiece(Restored, FVector::ForwardVector, .2f));
		State->SetMatchPhase(EFlickMatchPhase::Aiming);
		TestTrue(TEXT("Undo also works after settling"), Mode->UndoTrainingShot());
		TestTrue(TEXT("Most recent pre-shot board restored"), Mode->Pieces[0]->GetActorLocation().Equals(NewStart, .01f));
		Mode->TryLaunchPiece(Mode->Pieces[0], FVector::ForwardVector, .2f);
		State->SetMatchPhase(EFlickMatchPhase::Aiming);
		Mode->ToggleTrainingEditMode();
		TestFalse(TEXT("Entering editor invalidates old shot"), Mode->bHasTrainingUndoSnapshot);
		Mode->bTrainingEditMode = false;
		if (Mode->TestArenaActor) { Mode->TestArenaActor->Destroy(); Mode->TestArenaActor = nullptr; }
	}
	Mode->CaptureTrainingUndoSnapshot();
	Mode->bTrainingBotMatch = true;
	TestFalse(TEXT("Bot matches cannot undo"), Mode->UndoTrainingShot());
	Mode->bTrainingBotMatch = false; Mode->bTutorialMode = true;
	TestFalse(TEXT("Tutorial cannot undo"), Mode->UndoTrainingShot());
	Mode->bTutorialMode = false; Mode->bTrainingMode = false;
	TestFalse(TEXT("Normal matches cannot undo"), Mode->UndoTrainingShot());
	Mode->bTrainingMode = true; Mode->ResetTrainingBoard();
	TestFalse(TEXT("Saved setup reset invalidates shot history"), Mode->CanUndoTrainingShot());
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
