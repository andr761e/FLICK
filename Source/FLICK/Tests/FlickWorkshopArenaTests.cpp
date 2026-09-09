#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Arena/FlickBobArena.h"
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
	TestEqual(TEXT("1v1 active divider count"), Arena->GetMechanismCount(), 8);
	TestEqual(TEXT("1v1 designed location count"), Arena->GetPossibleLocationCount(), 20);
	for (int32 Index = 0; Index < Arena->GetMechanismCount(); ++Index)
	{
		const FVector RelativeCenter = Arena->GetDividerWorldCenter(Index) - Arena->GetActorLocation();
		const float SocketOuterRadius = FVector2D(RelativeCenter.X, RelativeCenter.Y).Size()
			+ (Arena->GetDividerCollisionThickness() + 8.0f) * 0.5f;
		TestTrue(TEXT("Active divider sockets preserve a visible gap to the rim"),
			SocketOuterRadius <= 650.0f * 0.96f);
	}

	UStaticMeshComponent* StaticArt = nullptr;
	UStaticMeshComponent* StadiumStructure = nullptr;
	UStaticMeshComponent* StadiumLights = nullptr;
	UStaticMeshComponent* FloorCollider = nullptr;
	UStaticMeshComponent* DividerCollider = nullptr;
	UStaticMeshComponent* DividerArt = nullptr;
	UStaticMeshComponent* SwitchArt = nullptr;
	UStaticMeshComponent* SwitchDotArt = nullptr;
	UStaticMeshComponent* SignalTraceArt = nullptr;
	UStaticMeshComponent* DormantSocketArt = nullptr;
	int32 SocketCount = 0;
	TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component->GetFName() == TEXT("WorkshopArenaMesh")) StaticArt = Component;
		if (Component->GetFName() == TEXT("StadiumStructureMesh")) StadiumStructure = Component;
		if (Component->GetFName() == TEXT("StadiumLightsMesh")) StadiumLights = Component;
		if (Component->GetFName() == TEXT("ArenaMesh")) FloorCollider = Component;
		if (Component->GetFName() == TEXT("EdgeDivider_00")) DividerCollider = Component;
		if (Component->GetFName() == TEXT("WorkshopDivider_00")) DividerArt = Component;
		if (Component->GetFName() == TEXT("ControlSwitchOuter_00")) SwitchArt = Component;
		if (Component->GetFName() == TEXT("ControlSwitchDot_00")) SwitchDotArt = Component;
		if (Component->GetFName() == TEXT("ControlSignalTrace_00")) SignalTraceArt = Component;
		if (Component->GetFName() == TEXT("DividerSocket_00")) DormantSocketArt = Component;
		if (Component->GetName().StartsWith(TEXT("DividerSocket_"))) ++SocketCount;
	}

	TestNotNull(TEXT("Imported arena presentation component"), StaticArt);
	TestNotNull(TEXT("Imported stadium structure component"), StadiumStructure);
	TestNotNull(TEXT("Imported stadium lighting component"), StadiumLights);
	TestNotNull(TEXT("Original floor collider"), FloorCollider);
	TestNotNull(TEXT("Simple divider collider"), DividerCollider);
	TestNotNull(TEXT("Imported divider presentation component"), DividerArt);
	TestNotNull(TEXT("Imported switch presentation component"), SwitchArt);
	TestNotNull(TEXT("Imported switch-dot presentation component"), SwitchDotArt);
	TestNotNull(TEXT("Imported signal-trace presentation component"), SignalTraceArt);
	TestNotNull(TEXT("Imported dormant-divider socket component"), DormantSocketArt);
	TestEqual(TEXT("All maximum-format socket components remain present"),
		SocketCount, AFlickTestArena::MaxPossibleLocationCount);

	Arena->InitializeTestArena(815.0f, 50.0f, 250.0f, 2);
	TestEqual(TEXT("2v2 active divider count"), Arena->GetMechanismCount(), 12);
	TestEqual(TEXT("2v2 designed location count"), Arena->GetPossibleLocationCount(), 28);
	Arena->InitializeTestArena(815.0f, 50.0f, 250.0f, 3);
	TestEqual(TEXT("3v3 active divider count"), Arena->GetMechanismCount(), 14);
	TestEqual(TEXT("3v3 designed location count"), Arena->GetPossibleLocationCount(), 36);
	Arena->BeginReplayPresentation();
	Arena->ApplyReplayDividerState(static_cast<uint16>(1 << 13));
	TestTrue(TEXT("Replay state preserves divider fourteen"), Arena->IsDividerRaised(13));
	Arena->EndReplayPresentation();
	Arena->InitializeTestArena(650.0f, 50.0f, 250.0f, 1);
	if (StaticArt && StaticArt->GetStaticMesh())
	{
		const FVector Size = StaticArt->GetStaticMesh()->GetBounds().BoxExtent * 2.0f;
		TestTrue(TEXT("Imported arena diameter is 1300 cm"),
			FMath::IsNearlyEqual(Size.X, 1300.0f, 1.0f) && FMath::IsNearlyEqual(Size.Y, 1300.0f, 1.0f));
		TestTrue(TEXT("Decorative arena trim stays flush with the authoritative surface"),
			StaticArt->GetStaticMesh()->GetBounds().BoxExtent.Z
				+ StaticArt->GetStaticMesh()->GetBounds().Origin.Z <= 3.0f);
		TestEqual(TEXT("Arena art cannot collide"), StaticArt->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestTrue(TEXT("Imported arena presentation is visible"), StaticArt->IsVisible());
		TestTrue(TEXT("Premium arena preserves its layered material separation"),
			StaticArt->GetStaticMesh()->GetStaticMaterials().Num() >= 12);
	}
	else
	{
		AddError(TEXT("Workshop arena mesh asset was not loaded"));
	}
	for (UStaticMeshComponent* StadiumComponent : { StadiumStructure, StadiumLights })
	{
		if (!StadiumComponent || !StadiumComponent->GetStaticMesh())
		{
			AddError(TEXT("Stadium presentation asset was not loaded"));
			continue;
		}
		TestEqual(TEXT("Stadium presentation cannot affect puck physics"),
			StadiumComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestTrue(TEXT("Stadium presentation is visible in the Test arena"),
			StadiumComponent->IsVisible());
		TestFalse(TEXT("Stadium shell cannot shadow the gameplay lighting rig"),
			StadiumComponent->CastShadow);
		TestTrue(TEXT("Stadium shares the play-surface origin"),
			FMath::IsNearlyEqual(StadiumComponent->GetRelativeLocation().Z, Arena->GetSurfaceZ(), 0.01f));
		TestTrue(TEXT("Stadium uses the authored 650 cm opening scale"),
			StadiumComponent->GetRelativeScale3D().Equals(FVector(1.0f), 0.001f));
	}
	if (StadiumStructure && StadiumStructure->GetStaticMesh())
	{
		const FVector StadiumSize = StadiumStructure->GetStaticMesh()->GetBounds().BoxExtent * 2.0f;
		TestTrue(TEXT("Stadium surrounds rather than overlaps the arena"),
			StadiumSize.X >= 3100.0f && StadiumSize.Y >= 3100.0f);
		TestTrue(TEXT("Stadium structure preserves layered material separation"),
			StadiumStructure->GetStaticMesh()->GetStaticMaterials().Num() >= 5);
	}
	if (StadiumLights && StadiumLights->GetStaticMesh())
	{
		TestTrue(TEXT("Stadium keeps warm, cyan, and orange lighting separate"),
			StadiumLights->GetStaticMesh()->GetStaticMaterials().Num() >= 3);
	}
	if (FloorCollider)
	{
		TestEqual(TEXT("Original floor collision remains authoritative"),
			FloorCollider->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		TestTrue(TEXT("Authoritative floor collider remains solid to the arena edge"),
			FMath::IsNearlyEqual(FloorCollider->GetComponentScale().X, 13.0f, 0.01f)
			&& FMath::IsNearlyEqual(FloorCollider->GetComponentScale().Y, 13.0f, 0.01f));
		TestFalse(TEXT("Original generated floor presentation is hidden"), FloorCollider->IsVisible());
	}
	for (UStaticMeshComponent* FlushMechanism : {
		SwitchArt, SwitchDotArt, SignalTraceArt, DormantSocketArt})
	{
		if (!FlushMechanism) continue;
		const bool bIsSwitchGraphic = FlushMechanism != DormantSocketArt;
		const float ExpectedVisualZ = Arena->GetSurfaceZ() + (bIsSwitchGraphic ? 0.35f : 0.0f);
		TestTrue(TEXT("Flush mechanism uses its stable visual depth layer"),
			FMath::IsNearlyEqual(FlushMechanism->GetRelativeLocation().Z, ExpectedVisualZ, 0.01f));
		TestEqual(TEXT("Flush mechanism delegates solidity to the arena floor"),
			FlushMechanism->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		bool bUsesDedicatedFlushMaterial = false;
		for (int32 MaterialIndex = 0; MaterialIndex < FlushMechanism->GetNumMaterials(); ++MaterialIndex)
		{
			const UMaterialInterface* Material = FlushMechanism->GetMaterial(MaterialIndex);
			bUsesDedicatedFlushMaterial |= Material
				&& Material->GetName().StartsWith(TEXT("MI_Flush_"));
		}
		TestTrue(TEXT("Closed mechanism uses dedicated anti-flicker materials"),
			bUsesDedicatedFlushMaterial);
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
		TestTrue(TEXT("Premium divider preserves its layered material separation"),
			DividerArt->GetStaticMesh() && DividerArt->GetStaticMesh()->GetStaticMaterials().Num() >= 7);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickHighDetailBobArenaTest, "FLICK.Visuals.HighDetailBobArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickHighDetailBobArenaTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
	UWorld* World = UWorld::CreateWorld(
		EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("BOB test world"), World))
	{
		return false;
	}
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickBobArena* Arena = World->SpawnActor<AFlickBobArena>();
	if (!TestNotNull(TEXT("BOB arena"), Arena))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	Arena->InitializeArena(620.0f, 50.0f, 250.0f);

	UStaticMeshComponent* HighDetailArt = nullptr;
	UStaticMeshComponent* BoardCollider = nullptr;
	UStaticMeshComponent* PocketedBoardCollider = nullptr;
	UStaticMeshComponent* RailCollider = nullptr;
	TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component->GetFName() == TEXT("HighDetailArenaMesh")) HighDetailArt = Component;
		if (Component->GetFName() == TEXT("BoardBase")) BoardCollider = Component;
		if (Component->GetFName() == TEXT("BoardCollision_0")) PocketedBoardCollider = Component;
		if (Component->GetFName() == TEXT("Rail_0")) RailCollider = Component;
	}

	TestNotNull(TEXT("Imported high-detail BOB presentation"), HighDetailArt);
	TestNotNull(TEXT("Original BOB board presentation"), BoardCollider);
	TestNotNull(TEXT("Pocketed BOB board collider"), PocketedBoardCollider);
	TestNotNull(TEXT("Original BOB rail collider"), RailCollider);
	if (HighDetailArt && HighDetailArt->GetStaticMesh())
	{
		const FVector Size = HighDetailArt->GetStaticMesh()->GetBounds().BoxExtent * 2.0f;
		TestTrue(TEXT("BOB art keeps the authoritative 1308 cm outer span"),
			FMath::IsNearlyEqual(Size.X, 1308.0f, 1.0f)
			&& FMath::IsNearlyEqual(Size.Y, 1308.0f, 1.0f));
		TestEqual(TEXT("BOB art cannot affect puck physics"),
			HighDetailArt->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestTrue(TEXT("High-detail BOB presentation is visible"), HighDetailArt->IsVisible());
		TestTrue(TEXT("BOB materials preserve surface, wood, metal, markings, and pockets"),
			HighDetailArt->GetStaticMesh()->GetStaticMaterials().Num() >= 9);
	}
	else
	{
		AddError(TEXT("High-detail BOB mesh asset was not loaded"));
	}
	if (BoardCollider)
	{
		TestEqual(TEXT("Solid legacy slab no longer blocks the pocket openings"),
			BoardCollider->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}
	for (UStaticMeshComponent* Collider : { PocketedBoardCollider, RailCollider })
	{
		if (!Collider) continue;
		TestEqual(TEXT("Legacy BOB collision remains authoritative"),
			Collider->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		TestFalse(TEXT("Legacy BOB collision presentation is hidden"), Collider->IsVisible());
	}
	const FVector Pocket = Arena->GetPocketWorldLocation(0);
	TestFalse(TEXT("Puck is not captured before it descends into the cup"),
		Arena->IsCapturedByPocket(FVector(Pocket.X, Pocket.Y, 250.0f), 24.0f));
	TestTrue(TEXT("Puck is captured after it visibly falls below the tabletop"),
		Arena->IsCapturedByPocket(FVector(Pocket.X, Pocket.Y, 240.0f), 24.0f));

	Arena->Destroy();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
