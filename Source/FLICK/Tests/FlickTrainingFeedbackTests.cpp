#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickTrainingFeedbackTest, "FLICK.Training.TutorialFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickTrainingFeedbackTest::RunTest(const FString& Parameters)
{
    const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Settings);
    if (!TestNotNull(TEXT("Training test world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    AFlickGameState* State = World->SpawnActor<AFlickGameState>();
    World->SetGameState(State);
    AFlickGameMode* Mode = World->SpawnActor<AFlickGameMode>();
    Mode->GameState = State;
    Mode->bTrainingMode = true;
    Mode->bTutorialMode = true;
    Mode->ArenaSurfaceZ = 250;

    Mode->SetupTutorialStage(0);
    TestFalse(TEXT("Tutorial does not assume cosmetic skin colours"), Mode->GetTutorialObjective().Contains(TEXT("orange")));
    Mode->ResolveTutorialShot();
    TestTrue(TEXT("Missed contact explains the target"), Mode->GetTutorialHint().Contains(TEXT("opposing puck")));
    for (const float Y : {-250.0f, 250.0f, 0.0f})
    {
        Mode->SetupTutorialStage(1);
        AFlickPiece* Piece = Mode->Pieces[0];
        Piece->SetActorLocation(FVector(0, Y, 263), false, nullptr, ETeleportType::TeleportPhysics);
        Mode->ResolveTutorialShot();
        TestTrue(TEXT("Power feedback describes the result"), Mode->GetTutorialHint().Contains(
            Y < 0 ? TEXT("more power") : Y > 0 ? TEXT("less power") : TEXT("Nice shot")));
        TestEqual(TEXT("Only a puck in the center completes the lesson"), Mode->bTutorialAdvancePending, Y == 0);
    }
    Mode->SetupTutorialStage(2);
    Mode->ResolutionEliminatedPieceIds.Add(Mode->TutorialShotPieceId);
    Mode->ResolveTutorialShot();
    TestTrue(TEXT("Self-knockout explains survival"), Mode->GetTutorialHint().Contains(TEXT("Keep your Striker")));
    Mode->SetupTutorialStage(3);
    TestTrue(TEXT("A retry clears old result feedback"), Mode->TutorialFeedback.IsEmpty());
    Mode->ResolutionActivatedSwitchMask = 0;
    Mode->ResolveTutorialShot();
    TestTrue(TEXT("Missed switch explains where to aim"), Mode->GetTutorialHint().Contains(TEXT("switch dot")));
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
