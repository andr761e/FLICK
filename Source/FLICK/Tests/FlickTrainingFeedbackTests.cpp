#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/FlickVisualSettings.h"

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
    // Enter bot training with either a menu spectator role or a stale opponent
    // private seat. Ownership must follow the human, never the current turn.
    auto* Controller = World->SpawnActor<AFlickPlayerController>();
    Controller->SetAsLocalPlayerController();
    World->AddController(Controller);
    auto* Player = World->SpawnActor<AFlickPlayerState>();
    Controller->PlayerState = Player;
    const int32 PreviousIndicatorStyle = FlickVisualSettings::GetPuckIndicatorStyle();
    Mode->bTutorialMode = false;
    for (EFlickTeam PreviousTeam : {EFlickTeam::None, EFlickTeam::Player2})
    for (EFlickMatchVariant Variant : {EFlickMatchVariant::Classic, EFlickMatchVariant::Bob})
    {
        Player->SetTeam(PreviousTeam);
        Player->SetTeamPlayerSlot(2);
        Player->SetPrivateControlledSlots({EncodePrivatePlayerSlot(EFlickTeam::Player2, 0)});
        Mode->SelectedMatchVariant = Variant;
        Mode->MatchmakingPlayersPerTeam = 1;
        Mode->BeginTrainingActivity(true);
        TestEqual(TEXT("Bot training assigns the human to Player 1"), Player->GetTeam(), EFlickTeam::Player1);
        TestEqual(TEXT("Bot training assigns human slot zero"), Player->GetTeamPlayerSlot(), 0);
        TestFalse(TEXT("Bot training clears stale private ownership"), Player->ControlsPrivateSlot(EFlickTeam::Player2, 0));
        for (EFlickTeam Turn : {EFlickTeam::Player1, EFlickTeam::Player2})
        for (int32 Style : {0, 1, 2})
        {
            State->SetCurrentTeam(Turn);
            State->SetMatchPhase(Turn == EFlickTeam::Player1 ? EFlickMatchPhase::KickoffPlanning : EFlickMatchPhase::Aiming);
            FlickVisualSettings::SetPuckIndicatorStyle(Style);
            TestEqual(TEXT("Local team does not change on the bot's turn"), Controller->GetLocalTeam(), EFlickTeam::Player1);
            int32 OwnCount = 0, BotCount = 0;
            for (AFlickPiece* Piece : Mode->Pieces)
            {
                const bool bOwn = Piece->GetTeam() == EFlickTeam::Player1;
                bOwn ? ++OwnCount : ++BotCount;
                TestEqual(TEXT("Only human pucks are locally owned"), Controller->OwnsPieceLocally(Piece), bOwn);
                Piece->Tick(.016f);
                for (UStaticMeshComponent* Component : TInlineComponentArray<UStaticMeshComponent*>(Piece))
                    if (bOwn && (Component->GetName() == TEXT("SelectionHalo") || Component->GetName().StartsWith(TEXT("TeamIndicatorDash"))))
                        TestFalse(TEXT("Human pucks have no ownership ring in any indicator style"), Component->IsVisible());
                for (UTextRenderComponent* Label : TInlineComponentArray<UTextRenderComponent*>(Piece))
                    if (Label->GetName() == TEXT("TeamIndicatorLabel"))
                    {
                        if (bOwn) TestFalse(TEXT("Human pucks have no opponent label"), Label->IsVisible());
                        else if (Style == 2) TestEqual(TEXT("Bot pucks remain marked FOE"), Label->Text.ToString(), FString(TEXT("FOE")));
                    }
            }
            TestTrue(TEXT("Regression exercised actual human and bot formations"), OwnCount > 0 && BotCount > 0);
        }
    }
    FlickVisualSettings::SetPuckIndicatorStyle(PreviousIndicatorStyle);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
