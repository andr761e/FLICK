#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBobGameplayTest, "FLICK.BOB.BotTurnIntegration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBobGameplayTest::RunTest(const FString& Parameters)
{
    const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
    if (!TestNotNull(TEXT("BOB test world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    AFlickGameState* State = World->SpawnActor<AFlickGameState>();
    World->SetGameState(State);
    AFlickGameMode* Mode = World->SpawnActor<AFlickGameMode>();
    Mode->GameState = State;
    Mode->ActiveMatchVariant = EFlickMatchVariant::Bob;
    Mode->bTestArenaMode = false;
    Mode->bPregamePreviewActive = false;
    Mode->ArenaRadius = 620.0f;
    Mode->ArenaSurfaceZ = 250.0f;
    Mode->FrontendScreen = EFlickFrontendScreen::Playing;
    Mode->CurrentPlayersPerTeam = 1;
    Mode->SpawnArenaIfNeeded();
    Mode->SpawnBobPieces();
    TestEqual(TEXT("Each team's single striker belongs to slot zero"), Mode->Player2BobStriker->GetOwningPlayerSlot(), 0);
    for (const bool bPrivate : {false, true})
    {
        Mode->bTrainingMode = !bPrivate;
        Mode->bTrainingBotMatch = !bPrivate;
        Mode->bPrivateMatchActive = bPrivate;
        for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
        {
            if (!bPrivate && Team == EFlickTeam::Player1) continue;
            State->SetCurrentTeam(Team);
            State->SetCurrentTeamPlayerSlot(0);
            State->SetMatchPhase(EFlickMatchPhase::Aiming);
            Mode->ResetTrainingBotThinking();
            Mode->UpdateTrainingBot(5.0f);
            TestEqual(bPrivate ? TEXT("An empty private BOB seat launches a bot shot") : TEXT("BOB vs Bot launches its orange striker"),
                State->MatchPhase, EFlickMatchPhase::ResolvingPhysics);
            TestEqual(TEXT("The bot uses its own striker, not an objective puck"), State->LastShotPieceId,
                Mode->GetBobStriker(Team)->GetPieceId());
        }
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
