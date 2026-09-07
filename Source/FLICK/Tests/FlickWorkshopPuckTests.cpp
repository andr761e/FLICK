#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pieces/FlickPiece.h"
#include "Core/FlickPieceArchetypeRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickWorkshopPuckTest, "FLICK.Visuals.WorkshopPucks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickWorkshopPuckTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	for (int32 Index = 0; Index < FlickPieceArchetypeRules::ArchetypeCount; ++Index)
	{
		UStaticMesh* SharedMesh = nullptr;
		for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
		{
			AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
			if (!TestNotNull(TEXT("Spawned puck"), Piece)) continue;
			const auto Archetype = static_cast<EFlickPieceArchetype>(Index);
			const auto& Rules = FlickPieceArchetypeRules::Get(Archetype);
			Piece->InitializePiece(Team, Index, 45.0f * Rules.RadiusMultiplier, 20.0f * Rules.ThicknessMultiplier, Archetype);
			auto* Root = Cast<UStaticMeshComponent>(Piece->GetRootComponent());
			UStaticMesh* CollisionMesh = Root->GetStaticMesh();
			const float Mass = Root->GetMass();
			TestFalse(TEXT("Ordinary puck does not load workshop art"), Piece->HasTestArenaVisuals());
			Piece->EnableTestArenaVisuals();
			TestTrue(TEXT("Test puck loads its imported mesh"), Piece->HasTestArenaVisuals());
			TestTrue(TEXT("Physics mesh preserved"), Root->GetStaticMesh() == CollisionMesh);
			TestEqual(TEXT("Physics mass preserved"), Root->GetMass(), Mass);
			TestTrue(TEXT("Root still simulates physics"), Root->IsSimulatingPhysics());
			UPointLightComponent* AccentLight = Piece->FindComponentByClass<UPointLightComponent>();
			TestNotNull(TEXT("Workshop puck has a local accent light"), AccentLight);
			if (AccentLight)
			{
				TestTrue(TEXT("Puck accent light remains visible with legacy art hidden"), AccentLight->IsVisible());
				TestTrue(TEXT("Puck accent is visible but restrained"),
					AccentLight->Intensity >= 15.0f && AccentLight->Intensity <= 40.0f);
				TestTrue(TEXT("Puck accent carries a controlled reflective highlight"),
					AccentLight->SpecularScale >= 0.25f && AccentLight->SpecularScale <= 0.40f);
			}
			TInlineComponentArray<UStaticMeshComponent*> Components(Piece);
			for (auto* Visual : Components)
			{
				if (Visual->GetFName() != TEXT("WorkshopMesh")) continue;
				TestTrue(TEXT("Cosmetic mesh has no collision"), Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				if (SharedMesh) TestTrue(TEXT("Teams share one mesh asset"), Visual->GetStaticMesh() == SharedMesh);
				SharedMesh = Visual->GetStaticMesh();
				if (!SharedMesh) continue;
				const FVector Extent = SharedMesh->GetBounds().BoxExtent * Visual->GetComponentScale();
				TestTrue(TEXT("Visual fits tuned radius"), FMath::Max(Extent.X, Extent.Y) <= Piece->GetPieceRadius() + .2f);
				const float VisualHeight = Extent.Z * 2.0f;
				TestTrue(TEXT("Visual height remains near collider"),
					VisualHeight >= Piece->GetPieceThickness() * 0.75f
					&& VisualHeight <= Piece->GetPieceThickness() * 1.20f);
				if (Archetype == EFlickPieceArchetype::Toppler || Archetype == EFlickPieceArchetype::Heavy)
				{
					TestTrue(TEXT("Tall classes exceed collider silhouette"), VisualHeight > Piece->GetPieceThickness());
				}
				if (Archetype == EFlickPieceArchetype::Compact || Archetype == EFlickPieceArchetype::Striker)
				{
					TestTrue(TEXT("Low classes sit below collider silhouette"), VisualHeight < Piece->GetPieceThickness() * 0.9f);
				}
				bool bFoundTeamMaterial = false;
				for (int32 Slot = 0; Slot < Visual->GetNumMaterials(); ++Slot)
				{
					if (auto* Dynamic = Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(Slot)))
					{
						const FLinearColor Color = Dynamic->K2_GetVectorParameterValue(TEXT("TeamColor"));
						TestTrue(TEXT("Team accent matches owner"), Team == EFlickTeam::Player2 ? Color.R > Color.B : Color.B > Color.R);
						bFoundTeamMaterial = true;
					}
				}
				TestTrue(TEXT("Team material found"), bFoundTeamMaterial);
				Piece->SetSelected(true);
				TestTrue(TEXT("Selection keeps imported mesh visible"), Visual->IsVisible());
				TestFalse(TEXT("Selection does not restore old body"), Root->IsVisible());
			}
			Piece->Destroy();
		}
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
