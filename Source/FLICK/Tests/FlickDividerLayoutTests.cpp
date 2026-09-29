#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickTestArena.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FlickModeRules.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickDividerRimClearanceTest, "FLICK.Arena.DividerRimClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickDividerRimClearanceTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickTestArena* Arena = World->SpawnActor<AFlickTestArena>();
	if (!TestNotNull(TEXT("Test arena"), Arena))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
	const auto FindComponent = [&Components](const FString& Name) -> UStaticMeshComponent*
	{
		for (UStaticMeshComponent* Component : Components)
			if (Component->GetName() == Name) return Component;
		return nullptr;
	};
	const auto CheckFootprint = [this, Arena](UStaticMeshComponent* Component, const float Radius)
	{
		UStaticMesh* Mesh = Component ? Component->GetStaticMesh().Get() : nullptr;
		if (!TestNotNull(TEXT("Divider assembly mesh"), Mesh)) return;
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		// Check the real imported mesh's oriented corners, including the wider
		// sockets. Center + collider width alone misses overlap at the slot ends.
		for (const float XSign : {-1.0f, 1.0f})
			for (const float YSign : {-1.0f, 1.0f})
			{
				const FVector Corner = Component->GetComponentTransform().TransformPosition(
					Bounds.Origin + FVector(XSign * Bounds.BoxExtent.X, YSign * Bounds.BoxExtent.Y, 0.0f));
				const FVector Offset = Corner - Arena->GetActorLocation();
				TestTrue(FString::Printf(TEXT("%s keeps at least 5 cm clear of the authored rim"), *Component->GetName()),
					FVector2D(Offset.X, Offset.Y).Size() <= Radius * 0.96f - 5.0f);
			}
	};
	for (const int32 TeamSize : {1, 2, 3})
	{
		const float Radius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize);
		Arena->InitializeTestArena(Radius, 50.0f, 250.0f, TeamSize);
		TestEqual(TEXT("Moving dividers does not resize the arena"), Arena->GetRadius(), Radius);
		for (int32 Index = 0; Index < Arena->GetPossibleLocationCount(); ++Index)
		{
			UStaticMeshComponent* Socket = FindComponent(FString::Printf(TEXT("DividerSocket_%02d"), Index));
			if (!TestNotNull(TEXT("Designed socket"), Socket)) continue;
			const float Angle = FMath::DegreesToRadians(Index * (360.0f / Arena->GetPossibleLocationCount()));
			const FVector2D Radial(FMath::Cos(Angle), FMath::Sin(Angle));
			const float OldFraction = Index % 2 == 0 ? Arena->OuterDividerRadiusFraction : 0.82f;
			const FVector2D Expected = Radial * Radius * (OldFraction - Arena->DividerRadialInsetFraction);
			const FVector Center = Socket->GetRelativeLocation();
			TestTrue(TEXT("Every active or dormant slot moves radially inward by the shared inset"),
				FVector2D(Center.X, Center.Y).Equals(Expected, 0.01f));
			TestTrue(TEXT("Socket depth stays flush with the floor"), FMath::IsNearlyEqual(Center.Z, Arena->GetSurfaceZ()));
			CheckFootprint(Socket, Radius);
		}
		Arena->BeginReplayPresentation();
		Arena->ApplyReplayDividerState(static_cast<uint16>((1 << Arena->GetMechanismCount()) - 1));
		for (int32 Index = 0; Index < Arena->GetMechanismCount(); ++Index)
		{
			UStaticMeshComponent* Collider = FindComponent(FString::Printf(TEXT("EdgeDivider_%02d"), Index));
			UStaticMeshComponent* Art = FindComponent(FString::Printf(TEXT("WorkshopDivider_%02d"), Index));
			if (!TestNotNull(TEXT("Divider collider"), Collider) || !TestNotNull(TEXT("Divider art"), Art)) continue;
			const FVector Center = Arena->GetDividerWorldCenter(Index);
			for (UStaticMeshComponent* Component : {Collider, Art})
			{
				TestTrue(TEXT("Collider and raised divider art share the authoritative center"),
					FVector2D(Component->GetComponentLocation()).Equals(FVector2D(Center), 0.01f));
				CheckFootprint(Component, Radius);
			}
			TestEqual(TEXT("Raised divider retains simple authoritative collision"),
				Collider->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
			TestEqual(TEXT("Divider art remains non-colliding"), Art->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestTrue(TEXT("Raised divider art remains visible"), Art->IsVisible());
			TestTrue(TEXT("Collider length and thickness are unchanged"), Collider->GetRelativeScale3D().Equals(
				FVector(Arena->GetDividerLength(Index) / 100.0f, Arena->DividerThickness / 100.0f, Arena->DividerHeight / 100.0f), 0.001f));
		}
		Arena->EndReplayPresentation();
		for (int32 Index = 0; Index < Arena->GetMechanismCount(); ++Index)
			TestFalse(TEXT("Replay ends with the original lowered divider state"), Arena->IsDividerRaised(Index));
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
