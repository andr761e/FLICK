#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickTestArena.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FlickModeRules.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickArenaConceptTest, "FLICK.Arena.ConceptPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickArenaConceptTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Arena = World->SpawnActor<AFlickTestArena>();
	if (!TestNotNull(TEXT("Test arena"), Arena))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	for (const int32 TeamSize : {1, 2, 3, 1})
	{
		const float Radius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize);
		Arena->InitializeTestArena(Radius, 50.f, 250.f, TeamSize);
		TestTrue(FString::Printf(TEXT("Concept applies to %dv%d"), TeamSize, TeamSize), Arena->bConceptApplied);
		TestTrue(TEXT("Reduced guide set is built"), Arena->ConceptGuides->GetInstanceCount() > 0);
		TestEqual(TEXT("Every designed socket has a dark channel"), Arena->ConceptSocketDark->GetInstanceCount(), Arena->GetPossibleLocationCount());
		TestEqual(TEXT("Every designed socket has four metal frame bars"), Arena->ConceptSocketMetal->GetInstanceCount(), Arena->GetPossibleLocationCount() * 4);
		FTransform Guide;
		Arena->ConceptGuides->GetInstanceTransform(0, Guide);
		FTransform OtherAxis, RingGuide;
		Arena->ConceptGuides->GetInstanceTransform(1, OtherAxis);
		Arena->ConceptGuides->GetInstanceTransform(2, RingGuide);
		TestTrue(TEXT("Cross axes and rings have distinct non-overlapping visual depths"),
			OtherAxis.GetLocation().Z - Guide.GetLocation().Z > .05f
			&& Guide.GetLocation().Z - RingGuide.GetLocation().Z > .05f);
		TestTrue(TEXT("Guides scale with the current arena radius"), FMath::IsNearlyEqual(Guide.GetScale3D().X * 100.f, Radius * 1.88f, .01f));
		FTransform Rim;
		Arena->ConceptWarmRim->GetInstanceTransform(0, Rim);
		TestTrue(TEXT("Warm rim follows the current arena edge"), FMath::IsNearlyEqual(FVector2D(Rim.GetLocation()).Size(), Radius * .998f, .01f));
		for (int32 Index = 0; Index < Arena->GetPossibleLocationCount(); ++Index)
		{
			FTransform Socket;
			Arena->ConceptSocketDark->GetInstanceTransform(Index, Socket);
			TestTrue(TEXT("Socket dressing follows the authoritative layout"),
				FVector2D(Socket.GetLocation()).Equals(Arena->PossibleDividerCenters[Index], .01f));
			TestTrue(TEXT("Socket dressing follows the actual divider length"),
				FMath::IsNearlyEqual(Socket.GetScale3D().X * 100.f,
					Arena->DividerBaseMeshes[Index]->GetRelativeScale3D().X * Arena->DividerLength + 4.f, .01f));
		}
		TestEqual(TEXT("Guides cannot collide with pucks"), Arena->ConceptGuides->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		for (const auto& Housing : Arena->ZoneOuterMeshes)
		{
			if (!Housing->IsVisible()) continue;
			TestEqual(TEXT("Switch housing retains its authored height-to-diameter proportions"),
				Housing->GetRelativeScale3D().Z, Housing->GetRelativeScale3D().X);
			TestTrue(TEXT("Complete switch housing clears the floor, not only its top face"),
				Housing->GetRelativeLocation().Z + Housing->GetStaticMesh()->GetBounds().Origin.Z * Housing->GetRelativeScale3D().Z
				- Housing->GetStaticMesh()->GetBounds().BoxExtent.Z * Housing->GetRelativeScale3D().Z > Arena->SurfaceZ + .5f);
			for (int32 Slot = 0; Slot < Housing->GetNumMaterials(); ++Slot)
			{
				UMaterialInterface* Material = Housing->GetMaterial(Slot);
				if (!Material || Slot != Housing->GetMaterialIndex(TEXT("05_Deep_Recess"))) continue;
				TestTrue(TEXT("Switch backing uses the authored shader, not the primitive fallback"),
					Material->GetMaterial() == Housing->GetStaticMesh()->GetMaterial(Slot)->GetMaterial());
				float Offset = 0.f;
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RenderLayerOffset")), Offset);
				TestEqual(TEXT("Switch backing cannot overlap the exposed metal rim"), Offset, -3.f);
			}
		}
		for (auto* Component : {Arena->ConceptSocketMetal.Get(), Arena->ConceptSocketDark.Get(), Arena->ConceptWarmRim.Get()})
			TestEqual(TEXT("Socket/rim dressing cannot collide"), Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TInlineComponentArray<UPrimitiveComponent*> Components(Arena);
		TArray<FTransform> Transforms;
		TArray<ECollisionEnabled::Type> Collision;
		for (auto* Component : Components)
		{
			Transforms.Add(Component->GetComponentTransform());
			Collision.Add(Component->GetCollisionEnabled());
		}
		Arena->SetMenuPresentationEnabled(true);
		TestTrue(TEXT("Menu presentation is enabled"), Arena->bMenuMaterialsEnabled);
		Arena->SetMenuPresentationEnabled(false);
		TestTrue(TEXT("New arena appearance remains active after leaving menus"), Arena->bConceptApplied && Arena->ConceptGuides->IsVisible());
		for (UMaterialInstanceDynamic* Material : Arena->ConceptMaterials)
			TestFalse(TEXT("Concept MIDs do not have unsupported MID parents"), Material->Parent->IsA<UMaterialInstanceDynamic>());
		for (int32 Index = 0; Index < Components.Num(); ++Index)
		{
			TestTrue(TEXT("Menu presentation preserves component transforms"), Components[Index]->GetComponentTransform().Equals(Transforms[Index]));
			TestEqual(TEXT("Menu presentation preserves collision modes"), Components[Index]->GetCollisionEnabled(), Collision[Index]);
		}
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
