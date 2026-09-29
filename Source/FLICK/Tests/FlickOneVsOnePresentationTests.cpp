#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Arena/FlickTestArena.h"
#include "Player/FlickCameraPawn.h"
#include "Core/FlickModeRules.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickOneVsOneArenaTest, "FLICK.Visuals.KnockoutArenaTheme",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickOneVsOneCameraTest, "FLICK.Visuals.OneVsOneCameraIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickOneVsOneArenaTest::RunTest(const FString& Parameters)
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
	Arena->InitializeTestArena(650.0f, 50.0f, 250.0f, 1);
	UStaticMeshComponent* Deck = nullptr;
	UStaticMeshComponent* FirstSwitch = nullptr;
	TArray<UStaticMeshComponent*> Surrounds;
	const float RimSurfaceZ = Arena->GetSurfaceZ();
	int32 LensComponents = 0;
	TArray<UStaticMeshComponent*> Colliders;
	TArray<FTransform> ColliderTransforms;
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component->GetFName() == TEXT("WorkshopArenaMesh")) Deck = Component;
		if (Component->GetFName() == TEXT("ControlSwitchOuter_00")) FirstSwitch = Component;
		if (Component->GetName().StartsWith(TEXT("ControlSwitchOuter_"))
			|| Component->GetName().StartsWith(TEXT("DividerSocket_"))) Surrounds.Add(Component);
		if (Component->GetName().StartsWith(TEXT("OneVsOneRim")))
		{
			++LensComponents;
			TestTrue(TEXT("1v1 rim dressing is visible"), Component->IsVisible());
			TestEqual(TEXT("Rim dressing cannot collide with pucks"), Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestFalse(TEXT("Rim dressing cannot overlap or trigger switches"), Component->GetGenerateOverlapEvents());
			UMaterialInterface* Material = Component->GetMaterial(0);
			TestTrue(TEXT("Rim shader is enabled for instanced meshes, not a default-material fallback"),
				Material && Material->GetMaterial() && Material->GetMaterial()->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
			if (auto* Instances = Cast<UInstancedStaticMeshComponent>(Component))
			{
				TestEqual(TEXT("Instanced lens count matches tuning"), Instances->GetInstanceCount(),
					Arena->RimLensCount * (Component->GetName().Contains(TEXT("Caps")) ? 2 : 1));
				UStaticMesh* InsertMesh = Instances->GetStaticMesh();
				if (TestNotNull(TEXT("Flush insert mesh"), InsertMesh))
				{
					const FBoxSphereBounds Bounds = InsertMesh->GetBounds();
					for (int32 Index = 0; Index < Instances->GetInstanceCount(); ++Index)
					{
						FTransform Transform;
						TestTrue(TEXT("Flush insert transform exists"), Instances->GetInstanceTransform(Index, Transform));
						const float TopZ = Transform.TransformPosition(Bounds.Origin + FVector(0.0f, 0.0f, Bounds.BoxExtent.Z)).Z;
						const float BottomZ = Transform.TransformPosition(Bounds.Origin - FVector(0.0f, 0.0f, Bounds.BoxExtent.Z)).Z;
						TestTrue(TEXT("Insert top is flush with the authored rim, within a sub-millimetre depth separation"),
							TopZ >= RimSurfaceZ && TopZ <= RimSurfaceZ + 0.04f);
						TestTrue(TEXT("Light insert is recessed into the rim surface"), BottomZ < RimSurfaceZ);
						TestTrue(TEXT("Insert face points straight up rather than forming a raised tube"),
							Transform.GetRotation().GetUpVector().Equals(FVector::UpVector, 0.001f));
					}
				}
			}
		}
		if (Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			Colliders.Add(Component);
			ColliderTransforms.Add(Component->GetComponentTransform());
		}
	}
	TestEqual(TEXT("Four batched non-colliding rim components"), LensComponents, 4);
	int32 DarkBezelCount = 0;
	for (UStaticMeshComponent* Surround : Surrounds)
	{
		for (int32 Slot = 0; Slot < Surround->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Original = Surround->GetStaticMesh()->GetMaterial(Slot);
			UMaterialInterface* Material = Surround->GetMaterial(Slot);
			if (!Original || !Material || (!Original->GetName().Contains(TEXT("Brushed_Titanium"))
				&& !Original->GetName().Contains(TEXT("Accent_Metal")))) continue;
			FLinearColor Color, OriginalColor;
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
			Original->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), OriginalColor);
			TestTrue(TEXT("Switch/socket edges are darker gunmetal rather than pale silver"),
				Color.GetLuminance() < OriginalColor.GetLuminance() * 0.4f && Color.B <= 0.18f);
			float RenderOffset = 0.0f, OriginalOffset = 0.0f;
			Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RenderLayerOffset")), RenderOffset);
			Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("RenderLayerOffset")), OriginalOffset);
			TestEqual(TEXT("Darkening a bezel preserves its authored depth offset"), RenderOffset, OriginalOffset);
			++DarkBezelCount;
		}
	}
	TestEqual(TEXT("Every switch and socket has its existing metal bezel shaded"), DarkBezelCount,
		AFlickTestArena::MaxMechanismCount + AFlickTestArena::MaxPossibleLocationCount);
	UMaterialInterface* StateAccent = FirstSwitch
		? FirstSwitch->GetMaterial(FirstSwitch->GetMaterialIndex(TEXT("09_Switch_Accent"))) : nullptr;
	TestNotNull(TEXT("Switch state material remains available"), StateAccent);
	TestEqual(TEXT("1v1 board radius is unchanged"), Arena->GetRadius(), 650.0f);
	TArray<FVector> DividerLocations;
	for (int32 Index = 0; Index < Arena->GetMechanismCount(); ++Index) DividerLocations.Add(Arena->GetDividerWorldCenter(Index));
	Arena->SetMenuPresentationEnabled(true);
	Arena->SetMenuPresentationEnabled(false);
	for (int32 Index = 0; Index < Colliders.Num(); ++Index)
		TestTrue(TEXT("Presentation does not move any collider"), Colliders[Index]->GetComponentTransform().Equals(ColliderTransforms[Index]));
	for (int32 Index = 0; Index < DividerLocations.Num(); ++Index)
		TestTrue(TEXT("Presentation does not move dividers"), Arena->GetDividerWorldCenter(Index).Equals(DividerLocations[Index]));
	if (TestNotNull(TEXT("Imported deck"), Deck))
	{
		// Change format while menu materials are active. The shared finish stays
		// intact while the rim and stadium follow each board's authoritative size.
		Arena->SetMenuPresentationEnabled(true);
		for (const int32 TeamSize : {2, 3})
		{
			const float Radius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize);
			const float Scale = Radius / 650.0f;
			Arena->InitializeTestArena(Radius, 50.0f, 250.0f, TeamSize);
			Arena->SetMenuPresentationEnabled(false);
			for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot)
			{
				UMaterialInterface* Material = Deck->GetMaterial(Slot);
				if (!Material || !Material->GetName().Contains(TEXT("Arena_Surface"))) continue;
				FLinearColor Color;
				float Metallic = 0.0f, Roughness = 0.0f;
				Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Metallic")), Metallic);
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
				TestTrue(TEXT("Every format keeps the silver deck"), Color.Equals(FLinearColor(0.60f, 0.65f, 0.69f), 0.001f));
				TestEqual(TEXT("Every format keeps the same metallic response"), Metallic, Arena->OneVsOneDeckMetallic);
				TestEqual(TEXT("Every format keeps the same satin roughness"), Roughness, Arena->OneVsOneDeckRoughness);
			}
			for (UStaticMeshComponent* Surround : Surrounds)
				for (int32 Slot = 0; Slot < Surround->GetNumMaterials(); ++Slot)
				{
					UMaterialInterface* Source = Surround->GetStaticMesh()->GetMaterial(Slot);
					if (!Source || (!Source->GetName().Contains(TEXT("Brushed_Titanium"))
						&& !Source->GetName().Contains(TEXT("Accent_Metal")))) continue;
					FLinearColor Color;
					Surround->GetMaterial(Slot)->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
					TestTrue(TEXT("2v2/3v3 retain darker switch/socket bezels"), Color.Equals(FLinearColor(0.09f, 0.135f, 0.17f), 0.001f));
				}
			for (UStaticMeshComponent* Component : Components)
			{
				if (Component->GetName().StartsWith(TEXT("Stadium")))
				{
					TestTrue(TEXT("Stadium follows arena size"), Component->GetRelativeScale3D().Equals(FVector(Scale), 0.001f));
					TestEqual(TEXT("Stadium remains visual only"), Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
					for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
					{
						UMaterialInterface* Source = Component->GetStaticMesh()->GetMaterial(Slot);
						float OriginalEmission = 0.0f, Emission = 0.0f;
						Source->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), OriginalEmission);
						Component->GetMaterial(Slot)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), Emission);
						if (OriginalEmission > 0.1f)
							TestTrue(TEXT("All formats share the balanced warm/cyan stadium emitters"),
								FMath::IsNearlyEqual(Emission, OriginalEmission * (Source->GetName().Contains(TEXT("Warm")) ? 0.55f : 2.0f)));
					}
				}
				if (Component->GetName().StartsWith(TEXT("OneVsOneRim")))
				{
					TestTrue(TEXT("2v2/3v3 show the shared recessed rim lighting"), Component->IsVisible());
					auto* Instances = Cast<UInstancedStaticMeshComponent>(Component);
					if (!TestNotNull(TEXT("Batched rim"), Instances)) continue;
					TestEqual(TEXT("Format changes do not accumulate rim instances"), Instances->GetInstanceCount(),
						Arena->RimLensCount * (Component->GetName().Contains(TEXT("Caps")) ? 2 : 1));
					FTransform Transform;
					Instances->GetInstanceTransform(0, Transform);
					TestTrue(TEXT("Larger inserts follow the arena radius"),
						FMath::IsNearlyEqual(FVector2D(Transform.GetLocation()).Size(), Radius - 16.0f * Scale,
							Component->GetName().Contains(TEXT("Caps")) ? 4.0f : 0.01f));
					const FBoxSphereBounds Bounds = Instances->GetStaticMesh()->GetBounds();
					const float TopZ = Transform.TransformPosition(Bounds.Origin + FVector(0.0f, 0.0f, Bounds.BoxExtent.Z)).Z;
					TestTrue(TEXT("Larger light inserts stay flush, not raised lamps"), TopZ >= 250.0f && TopZ <= 250.0f + 0.04f * Scale);
				}
			}
			// Snapshot this format's gameplay geometry before switching presentation.
			TArray<FTransform> FormatColliders;
			for (UStaticMeshComponent* Collider : Colliders) FormatColliders.Add(Collider->GetComponentTransform());
			Arena->SetMenuPresentationEnabled(true);
			Arena->SetMenuPresentationEnabled(false);
			for (int32 Index = 0; Index < Colliders.Num(); ++Index)
				TestTrue(TEXT("Larger-format presentation never moves colliders"), Colliders[Index]->GetComponentTransform().Equals(FormatColliders[Index]));
			Arena->SetMenuPresentationEnabled(true);
		}
		Arena->InitializeTestArena(650.0f, 50.0f, 250.0f, 1);
		Arena->SetMenuPresentationEnabled(false);
		bool bFoundSilverFloor = false;
		for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Material = Deck->GetMaterial(Slot);
			if (Material && Material->GetName().Contains(TEXT("Arena_Surface")))
			{
				FLinearColor Color;
				Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
				TestTrue(TEXT("Returning to 1v1 restores the light silver finish"), Color.R > 0.5f && Color.B > 0.5f);
				float Metallic = 0.0f, Roughness = 0.0f, SurfaceLift = 0.0f;
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Metallic")), Metallic);
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SurfaceLift")), SurfaceLift);
				TestEqual(TEXT("Gameplay floor uses the tuned metallic response"), Metallic, Arena->OneVsOneDeckMetallic);
				TestEqual(TEXT("Gameplay floor remains satin rather than mirror smooth"), Roughness, Arena->OneVsOneDeckRoughness);
				TestTrue(TEXT("Self illumination does not flatten the metal lighting"), SurfaceLift <= 0.03f);
				Arena->SetMenuPresentationEnabled(true);
				Deck->GetMaterial(Slot)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Metallic")), Metallic);
				Deck->GetMaterial(Slot)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
				TestEqual(TEXT("Menu keeps the 1v1 metallic response instead of overwriting it"), Metallic, Arena->OneVsOneDeckMetallic);
				TestEqual(TEXT("Menu uses its own subtle reflection roughness"), Roughness, Arena->OneVsOneMenuDeckRoughness);
				Arena->SetMenuPresentationEnabled(false);
				bFoundSilverFloor = true;
			}
		}
		TestTrue(TEXT("Verified actual silver floor material"), bFoundSilverFloor);
	}
	if (FirstSwitch && StateAccent)
	{
		TestTrue(TEXT("Cosmetic/format changes never replace the state-owned switch material"),
			FirstSwitch->GetMaterial(FirstSwitch->GetMaterialIndex(TEXT("09_Switch_Accent"))) == StateAccent);
		Arena->BeginReplayPresentation();
		Arena->ApplyReplayDividerState(1);
		FLinearColor Color;
		StateAccent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
		FLinearColor RaisedColor = Arena->GetMechanismColor(0) * 0.92f;
		RaisedColor.A = 1.0f;
		TestTrue(TEXT("Raised switch still displays the authoritative mechanism colour"),
			Color.Equals(RaisedColor, 0.001f));
		Arena->EndReplayPresentation();
		StateAccent->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color);
		FLinearColor ClosedColor = Arena->GetMechanismColor(0) * 0.56f;
		ClosedColor.A = 1.0f;
		TestTrue(TEXT("Closed switch still displays the authoritative mechanism colour"), Color.Equals(ClosedColor, 0.001f));
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

bool FFlickOneVsOneCameraTest::RunTest(const FString& Parameters)
{
	AFlickCameraPawn* Pawn = NewObject<AFlickCameraPawn>();
	UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Camera"), Camera)) return false;
	Pawn->SetTestArenaPresentation(true);
	const float GameplayBloom = Camera->PostProcessSettings.BloomIntensity;
	Pawn->SetMenuPresentation(true);
	Pawn->SetMenuOrbitEnabled(true);
	const float OtherMenuFstop = Camera->PostProcessSettings.DepthOfFieldFstop;
	Pawn->SetOneVsOneArenaPresentation(true);
	TestEqual(TEXT("1v1 menu has tuned localized glow"), Camera->PostProcessSettings.BloomIntensity, Pawn->OneVsOneMenuBloomIntensity);
	TestEqual(TEXT("1v1 menu has tuned background focus"), Camera->PostProcessSettings.DepthOfFieldFstop, Pawn->OneVsOneMenuFstop);
	Pawn->SetMenuPresentation(false);
	TestEqual(TEXT("Gameplay does not inherit cinematic bloom"), Camera->PostProcessSettings.BloomIntensity, GameplayBloom);
	TestFalse(TEXT("Gameplay does not inherit cinematic depth of field"), Camera->PostProcessSettings.bOverride_DepthOfFieldFstop);
	TestFalse(TEXT("Gameplay does not inherit the cinematic sensor"), Camera->PostProcessSettings.bOverride_DepthOfFieldSensorWidth);
	Pawn->SetMenuPresentation(true);
	Pawn->SetOneVsOneArenaPresentation(false);
	TestEqual(TEXT("Other formats keep their existing menu bloom"), Camera->PostProcessSettings.BloomIntensity, Pawn->MenuBloomIntensity);
	TestEqual(TEXT("Other formats keep their existing menu focus"), Camera->PostProcessSettings.DepthOfFieldFstop, OtherMenuFstop);
	return true;
}
#endif
