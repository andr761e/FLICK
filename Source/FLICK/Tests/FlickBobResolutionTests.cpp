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
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

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
    Top->SetActorRotation(FRotator(25, 12, 0), ETeleportType::TeleportPhysics);
    auto* RestingBody = CastChecked<UPrimitiveComponent>(Top->GetRootComponent());
    RestingBody->PutAllRigidBodiesToSleep();
    for (int32 Frame = 0; Frame < 120; ++Frame) Mode->UpdateBobPieceStability();
    TestFalse(TEXT("BOB self-righting cannot repeatedly wake a settled tilted rack coin"), RestingBody->IsAnyRigidBodyAwake());
    Top->SetActorRotation(FRotator::ZeroRotator, ETeleportType::TeleportPhysics);
    RestingBody->PutAllRigidBodiesToSleep();
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
    // Reproduce entering the real hole, then drifting past its footprint before
    // the old deep-plane check. Both sparse and dense rendered sampling must score.
    int32 NextId = 20;
    for (int32 PocketIndex = 0; PocketIndex < 4; ++PocketIndex)
    for (int32 FPS : {40, 400})
    {
        const FVector Pocket = Mode->BobArenaActor->GetPocketWorldLocation(PocketIndex);
        const FVector From(Pocket.X, Pocket.Y, Mode->BobArenaActor->SurfaceZ + 5.f);
        const FVector To = From + FVector(160, 0, -55);
        auto* Coin = Mode->SpawnPiece(EFlickTeam::Player1, NextId++, From, EFlickPieceArchetype::Standard);
        const int32 Before = State->Player1EliminatedThisShot;
        Mode->UpdateBobPockets();
        for (int32 Frame = 1; Frame <= FPS && Coin->IsActive(); ++Frame)
        {
            Coin->SetActorLocation(FMath::Lerp(From, To, static_cast<float>(Frame) / FPS), false, nullptr, ETeleportType::TeleportPhysics);
            Mode->UpdateBobPockets();
        }
        TestFalse(TEXT("A coin falling through the lip is scored instead of respawned"), Coin->IsActive());
        Mode->UpdateBobPockets();
        TestEqual(TEXT("Pocket scores exactly once at both 40 and 400 FPS"), State->Player1EliminatedThisShot, Before + 1);
    }
    const FVector Pocket = Mode->BobArenaActor->GetPocketWorldLocation(0);
    const FVector Above(Pocket.X, Pocket.Y, Mode->BobArenaActor->SurfaceZ + 5.f);
    auto* Bounce = Mode->SpawnPiece(EFlickTeam::Player1, NextId++, Above, EFlickPieceArchetype::Standard);
    Mode->UpdateBobPockets();
    Bounce->SetActorLocation(Above - FVector(0,0,10), false, nullptr, ETeleportType::TeleportPhysics);
    Mode->UpdateBobPockets();
    TestTrue(TEXT("Shallow lip entry does not score until the capture depth"), Bounce->IsActive());
    Bounce->SetActorLocation(Above, false, nullptr, ETeleportType::TeleportPhysics);
    Mode->UpdateBobPockets();
    TestTrue(TEXT("Returning above the bed clears pending pocket entry"), !Mode->BobPocketEnteredPieces.Contains(Bounce->GetPieceId()));
    TestFalse(TEXT("Flying horizontally over a hole is not entry"), Mode->BobArenaActor->HasEnteredPocketOnPath(Above, Above + FVector(160,0,0)));
    TestFalse(TEXT("Rising through the lip is not entry"), Mode->BobArenaActor->HasEnteredPocketOnPath(Above - FVector(0,0,10), Above));
    return true;
}

namespace
{
    struct FBobRackReviewState
    {
        TWeakObjectPtr<UWorld> World;
        TWeakObjectPtr<AFlickPiece> Striker;
        FVector StartPosition = FVector::ZeroVector;
        float StartTime = 0;
    };

    class FBobRackMotionReview final : public IAutomationLatentCommand
    {
    public:
        FBobRackMotionReview(FAutomationTestBase* InTest, TSharedRef<FBobRackReviewState> InState)
            : Test(InTest), State(InState) {}
        bool Update() override
        {
            UWorld* World = State->World.Get();
            if (!World) { Test->AddError(TEXT("Rack review has no live world")); return true; }
            const float Elapsed = World->GetTimeSeconds() - State->StartTime;
            if (Elapsed > 2.f)
            {
                for (TActorIterator<AFlickPiece> It(World); It; ++It)
                {
                    if (!It->IsActive() || It->GetLinearVelocity().Size() > 10.f) continue;
                    FVector Angular = It->GetAngularVelocityDegrees(); Angular.Z = 0;
                    const int32 Id = It->GetPieceId();
                    const FVector Previous = PreviousAngular.FindRef(Id);
                    if (Angular.Size() > 5.f && Previous.Size() > 5.f
                        && FVector::DotProduct(Angular.GetSafeNormal(), Previous.GetSafeNormal()) < -.5f)
                        ++Reversals.FindOrAdd(Id);
                    PreviousAngular.Add(Id, Angular);
                }
            }
            if (Shot < 3 && Elapsed >= (Shot == 0 ? 1.f : Shot == 1 ? 4.f : 7.5f))
            {
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()
                    / FString::Printf(TEXT("BOBRackReview/shot-%d.png"), Shot++), true, false);
            }
            if (Elapsed < 8.f) return false;
            int32 MaximumReversals = 0;
            for (const auto& Pair : Reversals) MaximumReversals = FMath::Max(MaximumReversals, Pair.Value);
            Test->AddInfo(FString::Printf(TEXT("BOB rack break: worst repeated rocking reversals=%d"), MaximumReversals));
            for (const auto& Pair : Reversals)
                if (Pair.Value > 16) Test->AddInfo(FString::Printf(TEXT("Rocking puck %d: %d reversals"), Pair.Key, Pair.Value));
            Test->TestTrue(TEXT("A settled rack coin does not oscillate perpetually"), MaximumReversals <= 16);
            Test->TestTrue(TEXT("The break still produces genuine physical motion"), State->Striker.IsValid()
                && FVector::Dist2D(State->Striker->GetActorLocation(), State->StartPosition) > 150.f);
            return true;
        }
    private:
        FAutomationTestBase* Test;
        TSharedRef<FBobRackReviewState> State;
        TMap<int32, FVector> PreviousAngular;
        TMap<int32, int32> Reversals;
        int32 Shot = 0;
    };
}

// Opt-in: exercises a real break with the production fixed-step physics and rig.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBobRackReviewTest, "FLICK.BOB.RackMotionReview",
    EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FFlickBobRackReviewTest::RunTest(const FString&)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("FlickBobRackReview"))) return true;
    const auto Review = MakeShared<FBobRackReviewState>();
    ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Review]()
    {
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            UWorld* World = Context.World();
            if (Context.WorldType != EWorldType::Game || !World) continue;
            auto* Mode = World->GetAuthGameMode<AFlickGameMode>();
            auto* State = World->GetGameState<AFlickGameState>();
            if (!Mode || !State) continue;
            // Reproduce the trailer's blue opening break even if the unattended
            // shot clock advanced while the renderer was warming up.
            State->SetCurrentTeam(EFlickTeam::Player1);
            if (FParse::Param(FCommandLine::Get(), TEXT("FlickBobReviewNoAssist")))
                Mode->BobSelfRightingMaxLinearSpeed = -1.f;
            for (TActorIterator<AFlickPiece> It(World); It; ++It)
            {
                if (!It->IsBobStriker() || It->GetTeam() != State->CurrentTeam) continue;
                Review->World = World; Review->Striker = *It;
                Review->StartPosition = It->GetActorLocation(); Review->StartTime = World->GetTimeSeconds();
                FVector Direction = -Review->StartPosition; Direction.Z = 0;
                float Power = .82f;
                FParse::Value(FCommandLine::Get(), TEXT("FlickBobReviewPower="), Power);
                TestTrue(TEXT("Real BOB break launches"), Mode->TryLaunchPiece(*It, Direction.GetSafeNormal(), FMath::Clamp(Power, .1f, 1.f)));
                return;
            }
        }
        AddError(TEXT("Start the rack review with -FlickBobPreview and an isolated UserDir"));
    }, 4.f));
    ADD_LATENT_AUTOMATION_COMMAND(FBobRackMotionReview(this, Review));
    return true;
}
#endif
