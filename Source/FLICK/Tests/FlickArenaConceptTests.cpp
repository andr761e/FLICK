#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickTestArena.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FlickModeRules.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
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
	const int32 DefaultLensCount = Arena->RimLensCount;
	const float DefaultLensEmission = Arena->RimLensEmission;
	const auto CheckParameter = [this](const UMaterialInterface* Material, const TCHAR* Name, float Expected)
	{
		float Value = -1.f;
		const FString Context = FString::Printf(TEXT("%s: %s"), *Material->GetName(), Name);
		TestTrue(Context + TEXT(" exists on the runtime material"),
			Material->GetScalarParameterValue(FMaterialParameterInfo(Name), Value));
		TestEqual(Context + TEXT(" follows the current arena presentation"), Value, Expected);
	};
	for (const int32 TeamSize : {1, 2, 3, 1})
	{
		const float Radius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize);
		// Exercise cached MID refresh with non-default values, then return to 1v1.
		Arena->RimLensCount = TeamSize == 2 ? 22 : DefaultLensCount;
		Arena->RimLensEmission = TeamSize == 3 ? DefaultLensEmission * .75f : DefaultLensEmission;
		Arena->InitializeTestArena(Radius, 50.f, 250.f, TeamSize);
		const auto CheckPatternScale = [&](const UMaterialInterface* Material)
		{
			CheckParameter(Material, TEXT("ArenaRadius"), Radius);
			CheckParameter(Material, TEXT("ArenaScale"), Arena->ArenaThickness / 50.f);
			CheckParameter(Material, TEXT("ArenaSurfaceZ"), Arena->SurfaceZ);
			CheckParameter(Material, TEXT("RimLensCount"), static_cast<float>(FMath::Clamp(Arena->RimLensCount, 12, 40)));
			CheckParameter(Material, TEXT("RimLensStrength"), Arena->RimLensEmission);
		};
		TestTrue(FString::Printf(TEXT("Concept applies to %dv%d"), TeamSize, TeamSize), Arena->bConceptApplied);
		TestTrue(TEXT("Reflections trace the same offset deck as the raster view"),
			Arena->WorkshopArenaMesh->EvaluateWorldPositionOffsetInRayTracing());
		for (const auto* Insert : {Arena->RimLensBodies.Get(), Arena->RimLensCaps.Get(), Arena->RimHousingBodies.Get(), Arena->RimHousingCaps.Get()})
		{
			TestFalse(TEXT("Material lenses never overlap a second near-coplanar light/edge mesh"), Insert->IsVisible());
			TestEqual(TEXT("Material-only rim needs no duplicate insert instances"), Insert->GetInstanceCount(), 0);
		}
		int32 CollarMaterialCount = 0;
		for (int32 Slot = 0; Slot < Arena->WorkshopArenaMesh->GetNumMaterials(); ++Slot)
		{
			const auto* Source = Arena->WorkshopArenaMesh->GetStaticMesh()->GetMaterial(Slot);
			if (Source && Source->GetName().Contains(TEXT("Rim_Polymer")))
			{
				++CollarMaterialCount;
				auto* Finish = Arena->WorkshopArenaMesh->GetMaterial(Slot);
				float Band = 0.f;
				Finish->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RimSideBand")), Band);
				TestEqual(TEXT("The existing rim sidewall carries the amber light segments"), Band, 1.f);
				CheckParameter(Finish, TEXT("RimCollarFinish"), 1.f);
				CheckParameter(Finish, TEXT("IndirectEmissionScale"), .15f);
				CheckPatternScale(Finish);
			}
			if (Source && Source->GetName().Contains(TEXT("Brushed_Titanium")))
			{
				++CollarMaterialCount;
				auto* Finish = Arena->WorkshopArenaMesh->GetMaterial(Slot);
				CheckParameter(Finish, TEXT("RimCollarFinish"), 1.f);
				CheckPatternScale(Finish);
			}
			if (!Source || !Source->GetName().Contains(TEXT("Arena_Surface"))) continue;
			auto* Finish = Arena->WorkshopArenaMesh->GetMaterial(Slot);
			TestEqual(TEXT("Deck uses the cooked architectural shader"), Finish->GetMaterial()->GetName(), FString(TEXT("M_ArchitecturalMetal")));
			CheckParameter(Finish, TEXT("RimCollarFinish"), 0.f);
			CheckParameter(Finish, TEXT("DeckFinish"), 1.f);
			CheckPatternScale(Finish);
			FLinearColor Color;
			Finish->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
			TestTrue(TEXT("Deck uses the tuneable neutral gunmetal reflectance tint"), Color.Equals(Arena->OneVsOneDeckTint));
			float Roughness = 1.f;
			Finish->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
			TestTrue(TEXT("Deck finish retains visible scene reflections"), Roughness <= .20f);
			float Lift = -1, Offset = 0, SourceOffset = 0;
			Finish->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SurfaceLift")), Lift);
			Finish->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RenderLayerOffset")), Offset);
			Source->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RenderLayerOffset")), SourceOffset);
			TestEqual(TEXT("Deck has no artificial emissive fill"), Lift, 0.f);
			TestEqual(TEXT("Finish preserves authored render depth"), Offset, SourceOffset);
		}
		TestTrue(TEXT("The authored arena contains opt-in collar materials"), CollarMaterialCount > 0);
		for (int32 Slot = 0; Slot < Arena->StadiumStructureMesh->GetNumMaterials(); ++Slot)
		{
			const auto* Material = Arena->StadiumStructureMesh->GetMaterial(Slot);
			if (!Material || Material->GetMaterial()->GetName() != TEXT("M_ArchitecturalMetal")) continue;
			CheckParameter(Material, TEXT("RimCollarFinish"), 0.f);
			CheckPatternScale(Material);
		}
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
				float Collar = 0.f;
				if (Material) Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RimCollarFinish")), Collar);
				TestEqual(TEXT("Arena collar lighting never opts switch housings or lenses in"), Collar, 0.f);
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
		for (int32 Slot = 0; Slot < Arena->WorkshopArenaMesh->GetNumMaterials(); ++Slot)
		{
			const auto* Source = Arena->WorkshopArenaMesh->GetStaticMesh()->GetMaterial(Slot);
			if (!Source || !Source->GetName().Contains(TEXT("Rim_Polymer"))) continue;
			const auto* Material = Arena->WorkshopArenaMesh->GetMaterial(Slot);
			CheckParameter(Material, TEXT("RimCollarFinish"), 1.f);
			CheckPatternScale(Material);
		}
		Arena->SetMenuPresentationEnabled(false);
		TestTrue(TEXT("New arena appearance remains active after leaving menus"), Arena->bConceptApplied && Arena->ConceptGuides->IsVisible());
		for (UMaterialInstanceDynamic* Material : Arena->ConceptMaterials)
			TestFalse(TEXT("Concept MIDs do not have unsupported MID parents"), Material->Parent->IsA<UMaterialInstanceDynamic>());
		for (int32 Index = 0; Index < Components.Num(); ++Index)
		{
			TestTrue(TEXT("Menu presentation preserves component transforms"), Components[Index]->GetComponentTransform().Equals(Transforms[Index]));
			TestEqual(TEXT("Menu presentation preserves collision modes"), Components[Index]->GetCollisionEnabled(), Collision[Index]);
		}
		// Client ticks must still animate both directions and leave replay frames
		// alone when redundant settled-transform updates are omitted.
		Arena->SetRole(ROLE_SimulatedProxy);
		Arena->RaisedDividerMask = 1;
		Arena->DividerLiftFractions[0] = .5f;
		Arena->ApplyMechanismState(false);
		const float RaisedZ = Arena->SurfaceZ + Arena->DividerHeight * .5f + 1.f;
		const auto CheckClientStep = [&](const float Target, const float Duration)
		{
			const float Expected = FMath::FInterpConstantTo(Arena->DividerLiftFractions[0], Target, .01f,
				1.f / FMath::Max(.05f, Duration));
			Arena->Tick(.01f);
			TestEqual(TEXT("Client retains exact divider interpolation"), Arena->DividerLiftFractions[0], Expected);
			TestTrue(TEXT("Moving client divider retains its exact transform"),
				FMath::IsNearlyEqual(Arena->DividerMeshes[0]->GetRelativeLocation().Z,
					RaisedZ - (1.f - Expected) * (Arena->DividerHeight + 2.f), .001f));
			Arena->Tick(10.f);
			const FTransform Settled = Arena->DividerMeshes[0]->GetRelativeTransform();
			Arena->Tick(.01f);
			TestEqual(TEXT("Client divider reaches its target"), Arena->DividerLiftFractions[0], Target);
			TestTrue(TEXT("Settled client divider stays unchanged"), Arena->DividerMeshes[0]->GetRelativeTransform().Equals(Settled));
		};
		CheckClientStep(1.f, Arena->DividerRiseDuration);
		Arena->RaisedDividerMask = 0;
		Arena->ApplyMechanismState(false);
		CheckClientStep(0.f, Arena->DividerRetractionDuration);
		Arena->BeginReplayPresentation();
		TArray<float> ReplayLifts = Arena->GetDividerLiftFractions();
		ReplayLifts[0] = .3f;
		Arena->ApplyReplayDividerState(1, ReplayLifts);
		Arena->Tick(.1f);
		TestEqual(TEXT("Client tick preserves recorded replay lift"), Arena->DividerLiftFractions[0], .3f);
		Arena->EndReplayPresentation();
		Arena->SetRole(ROLE_Authority);
		Arena->ResetMechanisms();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
