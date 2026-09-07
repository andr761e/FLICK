#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Arena/FlickTestArena.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickWorkshopArenaTest, "FLICK.Visuals.WorkshopArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickWorkshopArenaTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
	UWorld* World = UWorld::CreateWorld(
		EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickTestArena* Arena = World->SpawnActor<AFlickTestArena>();
	if (!TestNotNull(TEXT("Test arena"), Arena))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	Arena->InitializeTestArena(650.0f, 50.0f, 250.0f);
	for (int32 Index = 0; Index < Arena->GetMechanismCount(); ++Index)
	{
		const FVector RelativeCenter = Arena->GetDividerWorldCenter(Index) - Arena->GetActorLocation();
		const float SocketOuterRadius = FVector2D(RelativeCenter.X, RelativeCenter.Y).Size()
			+ (Arena->GetDividerCollisionThickness() + 8.0f) * 0.5f;
		TestTrue(TEXT("Active divider sockets preserve a visible gap to the rim"),
			SocketOuterRadius <= 650.0f * 0.96f);
	}

	UStaticMeshComponent* StaticArt = nullptr;
	UStaticMeshComponent* FloorCollider = nullptr;
	UStaticMeshComponent* DividerCollider = nullptr;
	UStaticMeshComponent* DividerArt = nullptr;
	int32 SocketCount = 0;
	TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component->GetFName() == TEXT("WorkshopArenaMesh")) StaticArt = Component;
		if (Component->GetFName() == TEXT("ArenaMesh")) FloorCollider = Component;
		if (Component->GetFName() == TEXT("EdgeDivider_00")) DividerCollider = Component;
		if (Component->GetFName() == TEXT("WorkshopDivider_00")) DividerArt = Component;
		if (Component->GetName().StartsWith(TEXT("DividerSocket_"))) ++SocketCount;
	}

	TestNotNull(TEXT("Imported arena presentation component"), StaticArt);
	TestNotNull(TEXT("Original floor collider"), FloorCollider);
	TestNotNull(TEXT("Simple divider collider"), DividerCollider);
	TestNotNull(TEXT("Imported divider presentation component"), DividerArt);
	TestEqual(TEXT("All designed sockets remain present"), SocketCount, AFlickTestArena::PossibleLocationCount);
	if (StaticArt && StaticArt->GetStaticMesh())
	{
		const FVector Size = StaticArt->GetStaticMesh()->GetBounds().BoxExtent * 2.0f;
		TestTrue(TEXT("Imported arena diameter is 1300 cm"),
			FMath::IsNearlyEqual(Size.X, 1300.0f, 1.0f) && FMath::IsNearlyEqual(Size.Y, 1300.0f, 1.0f));
		TestEqual(TEXT("Arena art cannot collide"), StaticArt->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestTrue(TEXT("Imported arena presentation is visible"), StaticArt->IsVisible());
	}
	else
	{
		AddError(TEXT("Workshop arena mesh asset was not loaded"));
	}
	if (FloorCollider)
	{
		TestEqual(TEXT("Original floor collision remains authoritative"),
			FloorCollider->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		TestFalse(TEXT("Original generated floor presentation is hidden"), FloorCollider->IsVisible());
	}

	Arena->BeginReplayPresentation();
	Arena->ApplyReplayDividerState(1);
	if (DividerCollider && DividerArt)
	{
		TestEqual(TEXT("Raised divider retains simple collision"),
			DividerCollider->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		TestFalse(TEXT("Collider mesh stays invisible"), DividerCollider->IsVisible());
		TestTrue(TEXT("Imported divider becomes visible"), DividerArt->IsVisible());
		TestEqual(TEXT("Imported divider cannot collide"),
			DividerArt->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		const FVector ColliderLocation = DividerCollider->GetRelativeLocation();
		const FVector ArtLocation = DividerArt->GetRelativeLocation();
		TestTrue(TEXT("Visual and collider share XY placement"),
			FVector2D(ColliderLocation.X, ColliderLocation.Y).Equals(
				FVector2D(ArtLocation.X, ArtLocation.Y), 0.1f));
	}

	Arena->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
