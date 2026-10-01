#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Arena/FlickBobArena.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBobSurfaceTest, "FLICK.BOB.PlanarSurfaceCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBobSurfaceTest::RunTest(const FString& Parameters)
{
    const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
        ERHIFeatureLevel::Num, &Settings);
    if (!TestNotNull(TEXT("Collision test world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    AFlickBobArena* Arena = World->SpawnActor<AFlickBobArena>();
    Arena->InitializeArena(620.0f, 54.0f, 250.0f);
    TArray<UPrimitiveComponent*> Components;
    Arena->GetComponents(Components);
    for (UPrimitiveComponent* Component : Components)
    {
        if (Component->GetName() != TEXT("FloorCollision") && !Component->GetName().StartsWith(TEXT("Rail_")))
            TestEqual(*FString::Printf(TEXT("Artwork %s cannot collide"), *Component->GetName()),
                Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
    }
    int32 RaisedHits = 0, MissingHits = 0;
    float MaxHeight = -MAX_flt;
    for (int32 X = -590; X <= 590; X += 10)
    for (int32 Y = -590; Y <= 590; Y += 10)
    {
        const FVector Point(X, Y, 250.0f);
        if (Arena->IsInsidePocket(Point)) continue;
        FHitResult Hit;
        if (!World->LineTraceSingleByChannel(Hit, Point + FVector(0, 0, 30),
            Point - FVector(0, 0, 60), ECC_WorldStatic)) ++MissingHits;
        else
        {
            MaxHeight = FMath::Max(MaxHeight, static_cast<float>(Hit.ImpactPoint.Z));
            if (Hit.ImpactPoint.Z > 250.05f)
            {
                if (++RaisedHits <= 5) AddInfo(FString::Printf(TEXT("Raised hit: %s at %s"),
                    *GetNameSafe(Hit.GetComponent()), *Hit.ImpactPoint.ToString()));
            }
        }
    }
    AddInfo(FString::Printf(TEXT("BOB sampled floor: maxZ %.6f, raised %d, missing %d"), MaxHeight, RaisedHits, MissingHits));
    TestEqual(TEXT("No raised collision on pocket bezels or edge artwork"), RaisedHits, 0);
    TestEqual(TEXT("Continuous support outside pocket openings"), MissingHits, 0);
    World->BeginPlay();
    // Traverse the narrow support between a pocket and the outer rail as well
    // as the inner pocket border. These cross UCX seams and authored linework.
    const FVector Starts[] = {FVector(350, 370, 261), FVector(350, 570, 261), FVector(-570, -350, 261)};
    const FVector Directions[] = {FVector(1, 0, 0), FVector(1, 0, 0), FVector(0, -1, 0)};
    const auto StepPhysics = [&]()
    {
        ++GFrameCounter; // Tick functions must see a fresh frame in this isolated test world.
        World->Tick(LEVELTICK_All, 1.0f / 120.0f);
    };
    for (int32 Corner = 0; Corner < 4; ++Corner)
    for (int32 Path = 0; Path < UE_ARRAY_COUNT(Starts); ++Path)
    {
        const FRotator Rotation(0, 90 * Corner, 0);
        const FVector Start = Rotation.RotateVector(Starts[Path]);
        const FVector Direction = Rotation.RotateVector(Directions[Path]);
        AFlickPiece* Piece = World->SpawnActor<AFlickPiece>(Start, FRotator::ZeroRotator);
        Piece->InitializePiece(EFlickTeam::Player1, 1, 35, 20, EFlickPieceArchetype::Standard);
        for (int32 Frame = 0; Frame < 60; ++Frame) StepPhysics();
        Piece->Launch(Direction, 1.0f, 300.0f);
        float MaxLift = 0, MaxDeviation = 0;
        for (int32 Frame = 0; Frame < 90; ++Frame)
        {
            StepPhysics();
            const FVector Location = Piece->GetActorLocation();
            MaxLift = FMath::Max(MaxLift, static_cast<float>(Location.Z - 260.0f));
            const FVector Side = FVector::CrossProduct(Direction, FVector::UpVector);
            MaxDeviation = FMath::Max(MaxDeviation, static_cast<float>(FMath::Abs(FVector::DotProduct(Location - Start, Side))));
        }
        AddInfo(FString::Printf(TEXT("BOB corner %d path %d: end %s lift %.3f deviation %.3f"),
            Corner, Path, *Piece->GetActorLocation().ToString(), MaxLift, MaxDeviation));
        TestTrue(TEXT("Seams cannot kick a sliding puck upward"), MaxLift < 2.0f);
        TestTrue(TEXT("Seams cannot redirect a sliding puck"), MaxDeviation < 1.0f);
        TestTrue(TEXT("Physics actually advanced along the tested path"),
            FVector::DotProduct(Piece->GetActorLocation() - Start, Direction) > 100.0f);
        Piece->Destroy();
    }
    for (int32 Pocket = 0; Pocket < 4; ++Pocket)
    {
        const FVector Center = Arena->GetPocketWorldLocation(Pocket);
        FHitResult Hit;
        TestFalse(TEXT("Pocket has no invisible floor or decorative bottom collider"),
            World->LineTraceSingleByChannel(Hit, Center + FVector(0, 0, 30), Center - FVector(0, 0, 60), ECC_WorldStatic));
        const FVector Direction(0, FMath::Sign(Center.Y), 0);
        const FVector Start = Center - Direction * 120.0f + FVector(0, 0, 11);
        AFlickPiece* Piece = World->SpawnActor<AFlickPiece>(Start, FRotator::ZeroRotator);
        Piece->InitializePiece(EFlickTeam::Player1, 1, 35, 20, EFlickPieceArchetype::Standard);
        for (int32 Frame = 0; Frame < 60; ++Frame) StepPhysics();
        Piece->Launch(Direction, 1.0f, 300.0f);
        for (int32 Frame = 0; Frame < 90; ++Frame) StepPhysics();
        AddInfo(FString::Printf(TEXT("BOB pocket %d: puck Z %.3f"), Pocket, Piece->GetActorLocation().Z));
        TestTrue(TEXT("Puck falls physically below pocket capture depth"),
            Piece->GetActorLocation().Z < Arena->GetSurfaceZ() - Arena->PocketCaptureDepth);
        Piece->Destroy();
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
