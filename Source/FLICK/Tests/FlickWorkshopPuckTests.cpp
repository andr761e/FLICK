#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickCameraPawn.h"
#include "Core/FlickPieceArchetypeRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickWorkshopPuckTest, "FLICK.Visuals.WorkshopPucks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPlayerIdentityPuckTest, "FLICK.Visuals.PlayerIdentityPucks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickTestArenaExposureTest, "FLICK.Visuals.TestArenaExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickTestArenaExposureTest::RunTest(const FString& Parameters)
{
	AFlickCameraPawn* CameraPawn = NewObject<AFlickCameraPawn>();
	if (!TestNotNull(TEXT("Camera pawn"), CameraPawn)) return false;
	UCameraComponent* Camera = CameraPawn->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Camera component"), Camera)) return false;

	CameraPawn->SetTestArenaPresentation(true);
	TestTrue(TEXT("Test arena fixes minimum exposure"),
		Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness);
	TestTrue(TEXT("Test arena fixes maximum exposure"),
		Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness);
	TestEqual(TEXT("Fixed exposure has no adaptation range"),
		Camera->PostProcessSettings.AutoExposureMinBrightness,
		Camera->PostProcessSettings.AutoExposureMaxBrightness);
	TestTrue(TEXT("Test arena bloom is more localized than classic bloom"),
		Camera->PostProcessSettings.BloomIntensity < CameraPawn->ClassicBloomIntensity
		&& Camera->PostProcessSettings.BloomThreshold > CameraPawn->ClassicBloomThreshold
		&& Camera->PostProcessSettings.bOverride_Bloom6Size
		&& Camera->PostProcessSettings.Bloom6Size < 64.0f);
	CameraPawn->ResetGameplayView(0, true);
	TestEqual(TEXT("Test arena starts on its lower tactical elevation"),
		CameraPawn->GetGameplayElevationAngle(), CameraPawn->TestArenaTacticalGameplayElevation);
	CameraPawn->AdjustGameplayElevation(-1);
	TestEqual(TEXT("Test arena can scroll lower than the classic low view"),
		CameraPawn->GetGameplayElevationAngle(), CameraPawn->TestArenaLowGameplayElevation);
	CameraPawn->AdjustGameplayElevation(1);
	CameraPawn->AdjustGameplayElevation(1);
	TestEqual(TEXT("Test arena overview is deliberately restrained"),
		CameraPawn->GetGameplayElevationAngle(), CameraPawn->TestArenaOverviewGameplayElevation);
	TestTrue(TEXT("Test arena low view is below classic low view"),
		CameraPawn->TestArenaLowGameplayElevation < CameraPawn->LowGameplayElevation);
	TestTrue(TEXT("Test arena high view is below classic overview"),
		CameraPawn->TestArenaOverviewGameplayElevation < CameraPawn->OverviewGameplayElevation);

	CameraPawn->SetTestArenaPresentation(false);
	TestFalse(TEXT("Other arenas restore adaptive minimum exposure"),
		Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness);
	TestFalse(TEXT("Other arenas restore adaptive maximum exposure"),
		Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness);
	TestFalse(TEXT("Other arenas restore the default bloom kernel"),
		Camera->PostProcessSettings.bOverride_Bloom6Size);
	return true;
}

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
					AccentLight->Intensity >= 8.0f && AccentLight->Intensity <= 24.0f);
				TestTrue(TEXT("Puck accent carries a controlled reflective highlight"),
					AccentLight->SpecularScale >= 0.25f && AccentLight->SpecularScale <= 0.40f);
				TestTrue(TEXT("Puck accent glow remains localized"),
					AccentLight->AttenuationRadius <= 68.0f);
			}
			TInlineComponentArray<UStaticMeshComponent*> Components(Piece);
			for (auto* Visual : Components)
			{
				if (Visual->GetFName() != TEXT("WorkshopMesh")) continue;
				TestTrue(TEXT("Cosmetic mesh has no collision"), Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				if (SharedMesh)
				{
					TestTrue(TEXT("Teams share identical high-detail geometry"), Visual->GetStaticMesh() == SharedMesh);
				}
				SharedMesh = Visual->GetStaticMesh();
				if (!SharedMesh) continue;
				const bool bHighDetailPuck = SharedMesh->GetPathName().Contains(TEXT("HighDetail"))
					|| SharedMesh->GetPathName().Contains(TEXT("PrototypeStandard"));
				TestTrue(TEXT("Every test-arena archetype uses its high-detail asset"), bHighDetailPuck);
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
				int32 TeamMaterialCount = 0;
				for (int32 Slot = 0; Slot < Visual->GetNumMaterials(); ++Slot)
				{
					if (auto* Dynamic = Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(Slot)))
					{
						const FLinearColor Color = Dynamic->K2_GetVectorParameterValue(TEXT("TeamColor"));
						TestTrue(TEXT("Team accent matches owner"), Team == EFlickTeam::Player2 ? Color.R > Color.B : Color.B > Color.R);
						const FLinearColor BaseColor = Dynamic->K2_GetVectorParameterValue(TEXT("BaseColor"));
						TestTrue(TEXT("Unlit diffuser tint matches owner"),
							Team == EFlickTeam::Player2 ? BaseColor.R > BaseColor.B : BaseColor.B > BaseColor.R);
						TestTrue(TEXT("Team LED emission remains gameplay-readable"),
							Dynamic->K2_GetScalarParameterValue(TEXT("Emission"))
								>= (bHighDetailPuck ? 5.5f : 9.0f));
						bFoundTeamMaterial = true;
						++TeamMaterialCount;
					}
				}
				TestTrue(TEXT("Team material found"), bFoundTeamMaterial);
				TestEqual(TEXT("Diffuser and emblem are independently team tinted"), TeamMaterialCount, 2);
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

bool FFlickPlayerIdentityPuckTest::RunTest(const FString& Parameters)
{
	static const TCHAR* ArchetypeNames[] = {
		TEXT("Standard"), TEXT("Heavy"), TEXT("Striker"), TEXT("Grippy"), TEXT("Slider"),
		TEXT("Blocker"), TEXT("Compact"), TEXT("Bouncer"), TEXT("Toppler")};
	static const TCHAR* IdentityNames[] = {TEXT("P1"), TEXT("P2"), TEXT("P3")};

	for (const TCHAR* Identity : IdentityNames)
	{
		for (const TCHAR* Archetype : ArchetypeNames)
		{
			const FString MeshPath = FString::Printf(
				TEXT("/Game/TestArena/Pucks/HighDetail/PlayerIdentity/%s/SM_Puck_%s_%s_HighDetail.SM_Puck_%s_%s_HighDetail"),
				Identity, Archetype, Identity, Archetype, Identity);
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
			if (!TestNotNull(*FString::Printf(TEXT("%s %s mesh"), Identity, Archetype), Mesh))
			{
				continue;
			}
			TestEqual(TEXT("Identity puck preserves seven authored material sections"),
				Mesh->GetStaticMaterials().Num(), 7);

			int32 PlayerIdentitySlotCount = 0;
			int32 TeamLightSlotCount = 0;
			for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
			{
				const FString SlotName = Slot.MaterialSlotName.ToString();
				if (SlotName.Contains(TEXT("Player_identity"), ESearchCase::IgnoreCase)
					|| SlotName.Contains(TEXT("Player identity"), ESearchCase::IgnoreCase))
				{
					++PlayerIdentitySlotCount;
					TestTrue(TEXT("Player-number material is independent from team colour"),
						Slot.MaterialInterface
						&& Slot.MaterialInterface->GetPathName().Contains(FString::Printf(TEXT("MI_Player_%s"), Identity)));
				}
				if (SlotName.Contains(TEXT("Cyan_light_diffuser"), ESearchCase::IgnoreCase)
					|| SlotName.Contains(TEXT("Cyan light diffuser"), ESearchCase::IgnoreCase))
				{
					++TeamLightSlotCount;
				}
			}
			TestEqual(TEXT("Puck has exactly one player-number section"), PlayerIdentitySlotCount, 1);
			TestEqual(TEXT("Puck has exactly one team-light section"), TeamLightSlotCount, 1);
		}
	}

	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Player identity runtime world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);

	for (int32 PlayerSlot = 0; PlayerSlot < 3; ++PlayerSlot)
	{
		UStaticMesh* SharedIdentityMesh = nullptr;
		for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
		{
			AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
			if (!TestNotNull(TEXT("Spawned identity puck"), Piece)) continue;
			Piece->InitializePiece(
				Team, PlayerSlot + 1, 45.0f, 20.0f, EFlickPieceArchetype::Standard,
				false, PlayerSlot, true);
			Piece->EnableTestArenaVisuals();

			UStaticMeshComponent* IdentityVisual = nullptr;
			TInlineComponentArray<UStaticMeshComponent*> Components(Piece);
			for (UStaticMeshComponent* Component : Components)
			{
				if (Component && Component->GetFName() == TEXT("WorkshopMesh"))
				{
					IdentityVisual = Component;
					break;
				}
			}
			if (!TestNotNull(TEXT("Runtime identity visual"), IdentityVisual))
			{
				Piece->Destroy();
				continue;
			}
			TestTrue(TEXT("Runtime selected the requested P-number mesh"),
				IdentityVisual->GetStaticMesh()
				&& IdentityVisual->GetStaticMesh()->GetPathName().Contains(
					FString::Printf(TEXT("P%d_HighDetail"), PlayerSlot + 1)));
			if (SharedIdentityMesh)
			{
				TestTrue(TEXT("Blue and orange share the same player-identity geometry"),
					IdentityVisual->GetStaticMesh() == SharedIdentityMesh);
			}
			SharedIdentityMesh = IdentityVisual->GetStaticMesh();

			int32 DynamicTeamMaterialCount = 0;
			for (int32 MaterialIndex = 0; MaterialIndex < IdentityVisual->GetNumMaterials(); ++MaterialIndex)
			{
				if (UMaterialInstanceDynamic* Dynamic =
					Cast<UMaterialInstanceDynamic>(IdentityVisual->GetMaterial(MaterialIndex)))
				{
					const FLinearColor TeamColor = Dynamic->K2_GetVectorParameterValue(TEXT("TeamColor"));
					TestTrue(TEXT("Runtime team light matches the puck team"),
						Team == EFlickTeam::Player2 ? TeamColor.R > TeamColor.B : TeamColor.B > TeamColor.R);
					++DynamicTeamMaterialCount;
				}
			}
			TestEqual(TEXT("Only the team-light section is dynamically recoloured"), DynamicTeamMaterialCount, 1);
			Piece->Destroy();
		}
	}

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
