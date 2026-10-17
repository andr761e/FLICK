#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickBobArena.h"
#include "Arena/FlickStadiumCrowd.h"
#include "Arena/FlickTestArena.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickStadiumCrowdTest, "FLICK.Arena.StadiumCrowd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickStadiumCrowdTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("World"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AActor* Arenas[] = {World->SpawnActor<AFlickTestArena>(), World->SpawnActor<AFlickBobArena>()};
	for (int32 Venue = 0; Venue < 2; ++Venue)
	{
		AActor* Arena = Arenas[Venue];
		if (!TestNotNull(TEXT("Arena"), Arena)) continue;
		auto* Crowd = Arena->FindComponentByClass<UFlickStadiumCrowd>();
		if (!TestNotNull(TEXT("Arena owns its crowd"), Crowd)) continue;
		TestFalse(TEXT("Stadium spectators are disabled by default"), Crowd->IsVisible());
		for (USceneComponent* Child : Crowd->GetAttachChildren())
		{
			TestFalse(TEXT("Disabled crowd batches are hidden"), Child->IsVisible());
			TestEqual(TEXT("Disabled stadium does not populate any spectators"), CastChecked<UInstancedStaticMeshComponent>(Child)->GetInstanceCount(), 0);
		}
		Crowd->Occupancy = 1.0f;
		Crowd->Configure(Venue == 1, 1.0f, 250.0f, true);
		TestEqual(TEXT("Only five geometry batches"), Crowd->GetAttachChildren().Num(), 5);
		int32 Parts = 0;
		for (USceneComponent* Child : Crowd->GetAttachChildren())
		{
			auto* Batch = Cast<UInstancedStaticMeshComponent>(Child);
			if (!TestNotNull(TEXT("Instanced batch"), Batch)) continue;
			TestTrue(TEXT("Nested crowd batch is registered"), Batch->IsRegistered());
			TestEqual(TEXT("No gameplay collision"), Batch->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestFalse(TEXT("No tick"), Batch->IsComponentTickEnabled());
			Parts += Batch->GetInstanceCount();
			for (int32 Index = 0; Index < Batch->GetInstanceCount(); ++Index)
			{
				FTransform Transform;
				Batch->GetInstanceTransform(Index, Transform);
				const float Radius = FVector2D(Transform.GetLocation()).Size();
				TestTrue(TEXT("All spectators stay outside the board"), Radius > (Venue == 1 ? 1800.0f : 1100.0f));
			}
		}
		TestEqual(TEXT("Every occupied seat has a complete seated figure"), Parts, (Venue == 1 ? 112 : 216) * 14);
		Crowd->Configure(Venue == 1, 1.5f, 250.0f, true);
		TestTrue(TEXT("Crowd follows venue scaling"), Crowd->GetRelativeScale3D().Equals(FVector(1.5f)));
		Crowd->Configure(Venue == 1, 1.0f, 250.0f, false);
		for (USceneComponent* Child : Crowd->GetAttachChildren()) TestFalse(TEXT("Fallback hides crowd"), Child->IsVisible());
		Crowd->Occupancy = 0.0f;
		Crowd->Configure(Venue == 1, 1.0f, 250.0f, true);
		for (USceneComponent* Child : Crowd->GetAttachChildren())
			TestEqual(TEXT("Zero occupancy clears instances"), CastChecked<UInstancedStaticMeshComponent>(Child)->GetInstanceCount(), 0);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
