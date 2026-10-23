#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "HAL/IConsoleManager.h"
#include "Arena/FlickBobArena.h"
#include "Arena/FlickTestArena.h"
#include "Core/FlickModeRules.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPhysicsRateTest, "FLICK.Physics.RenderRateIndependence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPhysicsRateTest::RunTest(const FString& Parameters)
{
    const UPhysicsSettings* Settings = UPhysicsSettings::Get();
    TestTrue(TEXT("The project uses fixed-rate Chaos"), Settings->bTickPhysicsAsync);
    TestTrue(TEXT("Simulation remains 120 Hz, not the render rate"),
        FMath::IsNearlyEqual(Settings->AsyncFixedTimeStepSize, 1.f / 120.f, 1.e-7f));
    TestEqual(TEXT("Fixed-step accumulator/interpolation remains enabled"),
        IConsoleManager::Get().FindConsoleVariable(TEXT("p.UseAsyncInterpolation"))->GetInt(), 1);
    TestEqual(TEXT("Physics catches up instead of dropping steps under load"),
        IConsoleManager::Get().FindConsoleVariable(TEXT("p.AsyncPhysicsBlockMode"))->GetInt(), 1);

    for (int32 Scenario = 0; Scenario < 4; ++Scenario)
    {
        TArray<FVector> Reference;
        for (const int32 Rate : {120, 40, 60, 144, 240, 400, 0})
        {
            const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false)
                .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
            UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
                ERHIFeatureLevel::Num, &Init);
            if (!TestNotNull(TEXT("Rate-test world"), World)) return false;
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
            if (Scenario == 2)
            {
                auto* Arena = World->SpawnActor<AFlickTestArena>();
                Arena->InitializeTestArena(650, 50, 250);
                Arena->SetTrainingLayoutSeed(1337);
            }
            else
            {
                auto* Arena = World->SpawnActor<AFlickBobArena>();
                Arena->InitializeArena(620, 54, 250);
            }
            const auto& Rules = FlickModeRules::Get(Scenario == 2 ? EFlickMatchVariant::Classic : EFlickMatchVariant::Bob);
            TArray<AFlickPiece*> Pieces;
            for (int32 Index = 0; Index < 2; ++Index)
            {
                FVector Start = Scenario != 1
                    ? FVector(Index == 0 ? -230 : 0, Index == 0 ? 0 : 18, 250 + Rules.PieceThickness * .5f)
                    : FVector(Index == 0 ? 400 : -200, Index == 0 ? 80 : -160, 250 + Rules.PieceThickness * .5f);
                const FRotator Rotation = Scenario == 3 && Index == 1 ? FRotator(25, 0, 0) : FRotator::ZeroRotator;
                if (Scenario == 3 && Index == 1)
                    Start.Z = 250 + Rules.PieceRadius * FMath::Sin(FMath::DegreesToRadians(25.f))
                        + Rules.PieceThickness * .5f * FMath::Cos(FMath::DegreesToRadians(25.f));
                auto* Piece = World->SpawnActor<AFlickPiece>(Start, Rotation);
                Piece->InitializePiece(Index == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2,
                    Index, Rules.PieceRadius, Rules.PieceThickness, EFlickPieceArchetype::Standard);
                Piece->PieceFriction = Rules.PieceFriction;
                Piece->PieceRestitution = Rules.PieceRestitution;
                Piece->LinearDamping = Rules.LinearDamping;
                Piece->AngularDamping = Rules.AngularDamping;
                Piece->ApplyPhysicsSettings();
                Pieces.Add(Piece);
            }
            World->BeginPlay();
            // All bodies start asleep at exactly the same poses. Queue one identical
            // impulse before stepping; no input timing or variable-frame forces.
            for (AFlickPiece* Piece : Pieces)
                CastChecked<UPrimitiveComponent>(Piece->GetRootComponent())->PutAllRigidBodiesToSleep();
            Pieces[0]->Launch(FVector::ForwardVector, 1.f, Scenario != 1 ? 650.f : 900.f);
            double Time = 0;
            int32 Frame = 0;
            const float Jitter[] = {1.f/40.f, 1.f/400.f, 1.f/55.f, 1.f/144.f};
            while (Time < 12.0 - 1.e-7)
            {
                const float Dt = FMath::Min(static_cast<double>(Rate ? 1.f / Rate : Jitter[Frame % 4]), 12.0 - Time);
                ++GFrameCounter;
                World->Tick(LEVELTICK_All, Dt);
                Time += Dt;
                ++Frame;
            }
            TArray<FVector> End;
            for (AFlickPiece* Piece : Pieces) End.Add(Piece->GetActorLocation());
            if (Reference.IsEmpty()) Reference = End;
            for (int32 Index = 0; Index < End.Num(); ++Index)
            {
                const float Error = FVector::Distance(End[Index], Reference[Index]);
                AddInfo(FString::Printf(TEXT("Scenario %d, render %d FPS (0=jitter), puck %d: endpoint error %.4f cm, speed %.3f"),
                    Scenario, Rate, Index, Error, Pieces[Index]->GetLinearVelocity().Size()));
                TestTrue(TEXT("Same collision outcome within 1 cm across render cadences"), Error < 1.f);
                TestTrue(TEXT("No tunnelling through a rail or unexpected knockout"), FMath::Abs(End[Index].X) < (Scenario == 2 ? 650.f : 620.f));
                TestTrue(TEXT("No tunnelling through the board"), End[Index].Z > 245.f);
            }
            if (Scenario != 1)
                TestTrue(TEXT("The second puck actually participated in a collision"), End[1].X > 35.f);
            else
                TestTrue(TEXT("The launched puck actually rebounded from the rail"), End[0].X < 400.f);
        }
    }
    return true;
}
#endif
