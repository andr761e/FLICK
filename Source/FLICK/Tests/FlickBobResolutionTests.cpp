#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Arena/FlickBobArena.h"
#include "Core/FlickModeRules.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBobResolutionReliabilityTest, "FLICK.BOB.ResolutionReliability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBobResolutionReliabilityTest::RunTest(const FString& Parameters)
{
    AddExpectedMessagePlain(TEXT("Resolution timeout cleanup used"), ELogVerbosity::Warning);
    AddExpectedMessagePlain(TEXT("Resolution timeout reached, but at least one puck is still moving meaningfully. Extending resolution."), ELogVerbosity::Warning);
    AddExpectedMessagePlain(TEXT("BOB recovered escaped puck 2 without awarding a pocket"), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 2);
    AddExpectedMessagePlain(TEXT("BOB recovered escaped puck 3 without awarding a pocket"), ELogVerbosity::Warning);
    const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Resolution world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* State = World->SpawnActor<AFlickGameState>();
    World->SetGameState(State);
    auto* Mode = World->SpawnActor<AFlickGameMode>();
    Mode->GameState = State;
    Mode->ActiveMatchVariant = EFlickMatchVariant::Bob;
    Mode->bTestArenaMode = false;
    Mode->bPregamePreviewActive = false;
    Mode->ArenaRadius = 620.f;
    Mode->ArenaSurfaceZ = 250.f;
    Mode->FrontendScreen = EFlickFrontendScreen::Playing;
    Mode->CurrentPlayersPerTeam = 1;
    Mode->SpawnArenaIfNeeded();
    const auto& Rules = FlickModeRules::Get(EFlickMatchVariant::Bob);
    Mode->PieceRadius = Rules.PieceRadius;
    Mode->PieceThickness = Rules.PieceThickness;
    AFlickPiece* Bottom = Mode->SpawnPiece(EFlickTeam::Player1, 1,
        FVector(0, 0, 250 + Rules.PieceThickness * .5f), EFlickPieceArchetype::Standard);
    AFlickPiece* Top = Mode->SpawnPiece(EFlickTeam::Player2, 2,
        FVector(0, 0, 250 + Rules.PieceThickness * 1.5f), EFlickPieceArchetype::Standard);
    for (AFlickPiece* Piece : {Bottom, Top})
        CastChecked<UPrimitiveComponent>(Piece->GetRootComponent())->PutAllRigidBodiesToSleep();
    // Ensure Chaos broadphase/query data has received the registered bodies.
    for (int32 Step = 0; Step < 8; ++Step) { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f / 120.f); }
    TestTrue(TEXT("A puck resting on another puck has real support"), Mode->HasRestingSupport(Top));
    TestTrue(TEXT("A resting stack cannot hold the turn in Puck in motion"), Mode->AreActivePiecesSettled());
    State->SetCurrentTeam(EFlickTeam::Player1);
    State->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);
    Mode->Tick(Mode->SettledDuration + .05f);
    TestEqual(TEXT("The real resolution tick returns a supported BOB stack to aiming"),
        State->MatchPhase, EFlickMatchPhase::Aiming);
    TestTrue(TEXT("Timeout can settle a supported stack rather than extend forever"), Mode->ApplyResolutionTimeoutCleanup());
    auto* Body = CastChecked<UPrimitiveComponent>(Top->GetRootComponent());
    Body->SetPhysicsLinearVelocity(FVector(300, 0, 0));
    TestFalse(TEXT("Genuine motion still blocks resolution"), Mode->AreActivePiecesSettled());
    TestFalse(TEXT("Timeout must not freeze a genuinely moving puck"), Mode->ApplyResolutionTimeoutCleanup());
    Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Top->SetActorLocation(FVector(0, 0, 350), false, nullptr, ETeleportType::TeleportPhysics);
    TestFalse(TEXT("A stationary airborne apex still cannot end the turn"), Mode->AreActivePiecesSettled());
    Top->SetActorLocation(FVector(850, 0, 100), false, nullptr, ETeleportType::TeleportPhysics);
    const int32 Count = Mode->CountActivePieces(EFlickTeam::Player2);
    Mode->UpdateBobPockets();
    TestTrue(TEXT("A puck outside the table is recovered, not allowed to fall forever"), Top->GetActorLocation().Z > 250);
    TestEqual(TEXT("Recovery cannot award a pocket"), Mode->CountActivePieces(EFlickTeam::Player2), Count);
    TestTrue(TEXT("Recovery cannot eliminate an objective puck"), Top->IsActive());
    TestTrue(TEXT("Recovery picks an unoccupied position"), FVector2D::Distance(
        FVector2D(Top->GetActorLocation()), FVector2D(Bottom->GetActorLocation())) > Rules.PieceRadius * 2);
    Top->SetActorLocation(FVector(850, 0, 204), false, nullptr, ETeleportType::TeleportPhysics);
    Mode->UpdateBobPockets();
    TestTrue(TEXT("A puck that lands on the surrounding stadium floor is also recovered"), Top->GetActorLocation().Z > 250);
    AFlickPiece* Striker = Mode->SpawnPiece(EFlickTeam::Player2, 3, FVector(-850, 0, 100),
        EFlickPieceArchetype::Standard, true);
    Mode->UpdateBobPockets();
    TestTrue(TEXT("Striker recovery keeps its striker identity and team"),
        Striker->IsActive() && Striker->IsBobStriker() && Striker->GetTeam() == EFlickTeam::Player2);
    TestFalse(TEXT("An escaped striker is not incorrectly penalized as pocketed"), Mode->bPlayer2BobStrikerPocketed);

    const float CaptureZ = Mode->BobArenaActor->SurfaceZ - Mode->BobArenaActor->PocketCaptureDepth;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FVector Pocket = Mode->BobArenaActor->GetPocketWorldLocation(Index);
        const FVector From(Pocket.X, Pocket.Y, CaptureZ + 1);
        const FVector To(Pocket.X + 150, Pocket.Y, CaptureZ - 30);
        TestFalse(TEXT("Fast path fixture misses the endpoint-only check"),
            Mode->BobArenaActor->IsCapturedByPocket(To, Rules.PieceRadius));
        TestTrue(TEXT("Capture-plane sweep does not miss a fast puck at low FPS"),
            Mode->BobArenaActor->IsCapturedByPocketOnPath(From, To, Rules.PieceRadius));
        TestFalse(TEXT("Crossing above the board cannot score a pocket"),
            Mode->BobArenaActor->IsCapturedByPocketOnPath(From + FVector(0,0,100), To + FVector(0,0,100), Rules.PieceRadius));
    }
    return true;
}
#endif
