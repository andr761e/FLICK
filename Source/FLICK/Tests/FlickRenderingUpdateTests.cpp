#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRenderingUpdateTest, "FLICK.Visuals.PuckRenderUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRenderingUpdateTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Render update world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	for (int32 Type = 0; Type < 9; ++Type)
	{
		auto* A = World->SpawnActor<AFlickPiece>();
		auto* B = World->SpawnActor<AFlickPiece>();
		for (auto* Piece : {A, B})
		{
			Piece->InitializePiece(EFlickTeam::Player1, Piece == A ? 1 : 2, 45.f, 20.f, static_cast<EFlickPieceArchetype>(Type));
			Piece->EnableTestArenaVisuals();
			Piece->Tick(.016f);
		}
		auto* MeshA = A->GetPresentationMesh();
		auto* MeshB = B->GetPresentationMesh();
		if (!TestNotNull(TEXT("First detailed puck"), MeshA) || !TestNotNull(TEXT("Second detailed puck"), MeshB)) continue;
		const auto Emissions = [](const UStaticMeshComponent* Mesh)
		{
			TArray<float> Values;
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				float Value = 0;
				Mesh->GetMaterial(Slot)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), Value);
				Values.Add(Value);
			}
			return Values;
		};
		const auto IdleB = Emissions(MeshB);
		const float IdleLight = B->FindComponentByClass<UPointLightComponent>()->Intensity;
		for (int32 Slot = 0; Slot < MeshA->GetNumMaterials(); ++Slot)
		{
			auto* MaterialA = Cast<UMaterialInstanceDynamic>(MeshA->GetMaterial(Slot));
			auto* MaterialB = Cast<UMaterialInstanceDynamic>(MeshB->GetMaterial(Slot));
			const FString Role = MeshA->GetStaticMesh()->GetStaticMaterials()[Slot].MaterialSlotName.ToString();
			if (Role.Contains(TEXT("05_Team")) || Role.Contains(TEXT("Cyan")))
				TestTrue(TEXT("Mutable team LEDs belong to each puck"), MaterialA && MaterialB && MaterialA != MaterialB);
		}
		A->SetSelected(true);
		UStaticMeshComponent* Halo = nullptr;
		for (auto* Component : TInlineComponentArray<UStaticMeshComponent*>(A))
			if (Component->GetFName() == TEXT("SelectionHalo")) Halo = Component;
		if (TestNotNull(TEXT("Selection halo"), Halo))
		{
			const FVector BeforePulse = Halo->GetRelativeScale3D();
			A->Tick(.1f);
			TestFalse(TEXT("Selected halo still animates between appearance changes"), Halo->GetRelativeScale3D().Equals(BeforePulse));
		}
		A->PlayImpactFlash(1.f);
		const auto BrightA = Emissions(MeshA);
		A->Tick(.1f);
		TestTrue(TEXT("Hit flash continues fading while selected"), Emissions(MeshA) != BrightA);
		TestTrue(TEXT("Selection and hit flashes do not change the other puck"), Emissions(MeshB) == IdleB);
		TestEqual(TEXT("Other puck's accent stays idle"), B->FindComponentByClass<UPointLightComponent>()->Intensity, IdleLight);
		B->SetHovered(true);
		TestTrue(TEXT("Hover still highlights the puck"), B->FindComponentByClass<UPointLightComponent>()->Intensity > IdleLight);
		B->SetHovered(false);
		TestEqual(TEXT("Leaving hover restores idle light"), B->FindComponentByClass<UPointLightComponent>()->Intensity, IdleLight);
		A->SetPuckSkin(1);
		A->SetPuckSkin(0);
		TestTrue(TEXT("Skin changes do not recolour another puck's LEDs"), Emissions(MeshB) == IdleB);
		A->Destroy();
		B->Destroy();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
