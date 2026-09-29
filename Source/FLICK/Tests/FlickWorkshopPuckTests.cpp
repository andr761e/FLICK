#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickCameraPawn.h"
#include "Arena/FlickTestArena.h"
#include "Core/FlickPieceArchetypeRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickWorkshopPuckTest, "FLICK.Visuals.WorkshopPucks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPuckSkinTest, "FLICK.Visuals.PuckSkins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickTestArenaExposureTest, "FLICK.Visuals.TestArenaExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickMenuPresentationTest, "FLICK.Visuals.MenuPresentationIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickMenuArenaTest, "FLICK.Visuals.MenuArenaIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMenuArenaTest::RunTest(const FString& Parameters)
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
	Arena->InitializeTestArena(650.0f, 50.0f, 250.0f);
	UStaticMeshComponent* Art = nullptr;
	TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
	for (UStaticMeshComponent* Component : Components)
		if (Component->GetFName() == TEXT("WorkshopArenaMesh")) Art = Component;
	if (TestNotNull(TEXT("Arena visual mesh"), Art))
	{
		TArray<UMaterialInterface*> Originals;
		for (int32 Index = 0; Index < Art->GetNumMaterials(); ++Index) Originals.Add(Art->GetMaterial(Index));
		Arena->SetMenuPresentationEnabled(true);
		bool bCheckedFloor = false;
		bool bCheckedLight = false;
		for (int32 Index = 0; Index < Originals.Num(); ++Index)
		{
			UMaterialInterface* Original = Originals[Index];
			UMaterialInterface* MenuMaterial = Art->GetMaterial(Index);
			TestTrue(TEXT("Menu uses a transient material, not an asset edit"), MenuMaterial != Original);
			if (auto* Dynamic = Cast<UMaterialInstanceDynamic>(MenuMaterial))
			{
				TestNotNull(TEXT("Menu material has a renderable authored parent"), Dynamic->Parent.Get());
				TestFalse(TEXT("Menu never parents a dynamic material to another dynamic material"),
					Dynamic->Parent && Dynamic->Parent->IsA<UMaterialInstanceDynamic>());
			}
			if (!Original || !MenuMaterial) continue;
			float Emission = 0.0f;
			Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), Emission);
			if (Emission > 0.1f)
			{
				float MenuEmission = 0.0f;
				MenuMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), MenuEmission);
				TestEqual(TEXT("Menu increases authored arena LEDs"), MenuEmission, Emission * 4.0f);
				bCheckedLight = true;
			}
			if (Original->GetName().Contains(TEXT("Arena_Surface")))
			{
				float Roughness = 0.0f;
				MenuMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
				TestEqual(TEXT("Menu deck has a restrained specular finish"), Roughness, Arena->OneVsOneMenuDeckRoughness);
				bCheckedFloor = true;
			}
		}
		TestTrue(TEXT("Checked the actual deck material"), bCheckedFloor);
		TestTrue(TEXT("Checked actual emissive material"), bCheckedLight);
		Arena->SetMenuPresentationEnabled(false);
		for (int32 Index = 0; Index < Originals.Num(); ++Index)
			TestTrue(TEXT("Leaving menu restores each original material"), Art->GetMaterial(Index) == Originals[Index]);
		TestEqual(TEXT("Menu finish cannot add collision"), Art->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

bool FFlickMenuPresentationTest::RunTest(const FString& Parameters)
{
	AFlickCameraPawn* Pawn = NewObject<AFlickCameraPawn>();
	UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Camera component"), Camera)) return false;
	Pawn->SetTestArenaPresentation(true);
	const float GameplayBloom = Camera->PostProcessSettings.BloomIntensity;
	const FVector4 GameplayContrast = Camera->PostProcessSettings.ColorContrast;
	Pawn->SetMenuPresentation(true);
	Pawn->SetMenuOrbitEnabled(true);
	TestEqual(TEXT("Main menu uses its tuned bloom"), Camera->PostProcessSettings.BloomIntensity, Pawn->MenuBloomIntensity);
	TestTrue(TEXT("Main menu enables restrained depth of field"), Camera->PostProcessSettings.bOverride_DepthOfFieldFstop);
	TestTrue(TEXT("Main menu has richer color"), Camera->PostProcessSettings.ColorSaturation.X > 1.02f);
	Pawn->SetMenuOrbitEnabled(false);
	TestEqual(TEXT("Other frontend screens restore bloom"), Camera->PostProcessSettings.BloomIntensity, GameplayBloom);
	TestFalse(TEXT("Other frontend screens do not inherit menu blur"), Camera->PostProcessSettings.bOverride_DepthOfFieldFstop);
	Pawn->SetMenuOrbitEnabled(true);
	Pawn->SetMenuPresentation(false);
	TestEqual(TEXT("Gameplay restores bloom"), Camera->PostProcessSettings.BloomIntensity, GameplayBloom);
	TestEqual(TEXT("Gameplay restores contrast"), Camera->PostProcessSettings.ColorContrast, GameplayContrast);
	TestFalse(TEXT("Gameplay disables menu depth of field"), Camera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance);
	return true;
}

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
			Piece->SetPuckSkin(Team == EFlickTeam::Player2 ? 1 : 0);
			Piece->EnableTestArenaVisuals();
			TestTrue(TEXT("Test puck loads its imported mesh"), Piece->HasTestArenaVisuals());
			TestTrue(TEXT("Physics mesh preserved"), Root->GetStaticMesh() == CollisionMesh);
			TestEqual(TEXT("Physics mass preserved"), Root->GetMass(), Mass);
			TestTrue(TEXT("Root still simulates physics"), Root->IsSimulatingPhysics());
			if (Index == 0 && Team == EFlickTeam::Player1)
			{
				const FVector StartingLocation = Piece->GetActorLocation();
				Piece->SetPregamePreview(true);
				TestFalse(TEXT("Confirmed lineup preview cannot move under physics"), Root->IsSimulatingPhysics());
				TestTrue(TEXT("Confirmed lineup preview cannot collide"),
					Root->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
				Piece->SetPregamePreview(false);
				TestTrue(TEXT("Physics resumes for the live match"), Root->IsSimulatingPhysics());
				TestTrue(TEXT("Preview did not alter the puck starting position"),
					Piece->GetActorLocation().Equals(StartingLocation, 0.1f));
			}
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

	AFlickPiece* BobStriker = World->SpawnActor<AFlickPiece>();
	if (TestNotNull(TEXT("Spawned BOB striker"), BobStriker))
	{
		BobStriker->InitializePiece(
			EFlickTeam::Player2, 99, 24.0f, 14.0f,
			EFlickPieceArchetype::Standard, true, 1);
		BobStriker->EnableTestArenaVisuals();
		TestTrue(TEXT("BOB striker uses the premium Standard puck mesh"),
			BobStriker->HasTestArenaVisuals());
		TestEqual(TEXT("BOB striker retains its distinct player identity"),
			BobStriker->GetOwningPlayerSlot(), 1);
		TInlineComponentArray<UStaticMeshComponent*> BobComponents(BobStriker);
		for (UStaticMeshComponent* Visual : BobComponents)
		{
			if (!Visual || Visual->GetFName() != TEXT("WorkshopMesh") || !Visual->GetStaticMesh()) continue;
			const FVector Extent = Visual->GetStaticMesh()->GetBounds().BoxExtent * Visual->GetComponentScale();
			TestTrue(TEXT("BOB Standard art is scaled to its smaller physics radius"),
				FMath::Max(Extent.X, Extent.Y) <= BobStriker->GetPieceRadius() + 0.2f);
			TestTrue(TEXT("BOB Standard art is scaled to its smaller physics height"),
				Extent.Z * 2.0f <= BobStriker->GetPieceThickness() * 1.2f);
			int32 DynamicMaterialCount = 0;
			for (int32 Slot = 0; Slot < Visual->GetNumMaterials(); ++Slot)
			{
				DynamicMaterialCount += Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(Slot)) ? 1 : 0;
			}
			TestTrue(TEXT("BOB striker has distinct dynamic crown and light materials"),
				DynamicMaterialCount >= 4);
		}
		for (UTextRenderComponent* Text : TInlineComponentArray<UTextRenderComponent*>(BobStriker))
			TestFalse(TEXT("No obsolete P-number component remains"), Text->GetFName() == TEXT("PlayerLabel"));
		BobStriker->Destroy();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

bool FFlickPuckSkinTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Cosmetic runtime world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	TArray<UStaticMesh*> BaseMeshes;
	BaseMeshes.SetNumZeroed(FlickPieceArchetypeRules::ArchetypeCount);
	for (int32 PlayerSlot = 0; PlayerSlot < 3; ++PlayerSlot)
	for (EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
	for (int32 Skin = 0; Skin < 2; ++Skin)
	for (int32 Type = 0; Type < FlickPieceArchetypeRules::ArchetypeCount; ++Type)
	{
		AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
		Piece->InitializePiece(Team, 1, 45, 20, static_cast<EFlickPieceArchetype>(Type), false, PlayerSlot);
		Piece->SetPuckSkin(Skin);
		Piece->EnableTestArenaVisuals();
		TestTrue(TEXT("Ownership independent of skin"), Piece->GetTeam() == Team);
		TestEqual(TEXT("Requested cosmetic applied"), Piece->GetPuckSkin(), Skin);
		TInlineComponentArray<UStaticMeshComponent*> Components(Piece);
		for (UStaticMeshComponent* Visual : Components)
		{
			if (Visual->GetFName() == TEXT("SelectionHalo"))
			{
				TestTrue(TEXT("Team marker remains visible without selection"), Visual->IsVisible());
				if (auto* Marker = Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(0)))
				{
					const FLinearColor Colour = Marker->K2_GetVectorParameterValue(TEXT("Color"));
					TestTrue(TEXT("Ownership marker follows team, not skin"), Team == EFlickTeam::Player2 ? Colour.R > Colour.B : Colour.B > Colour.R);
				}
			}
			if (Visual->GetFName() != TEXT("WorkshopMesh")) continue;
			TestNotNull(TEXT("Base cosmetic mesh"), Visual->GetStaticMesh().Get());
			TestTrue(TEXT("Cosmetic does not own collision"), Visual->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
			if (BaseMeshes[Type]) TestTrue(TEXT("All players and both skins reuse the base mesh"), BaseMeshes[Type] == Visual->GetStaticMesh());
			BaseMeshes[Type] = Visual->GetStaticMesh();
			for (const FStaticMaterial& Material : Visual->GetStaticMesh()->GetStaticMaterials())
				TestFalse(TEXT("No baked player-number sections"), Material.MaterialSlotName.ToString().Contains(TEXT("identity"), ESearchCase::IgnoreCase));
			int32 Lights = 0;
			for (int32 Slot = 0; Slot < Visual->GetNumMaterials(); ++Slot)
			{
				if (UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Visual->GetMaterial(Slot)))
				{
					const FLinearColor Color = Dynamic->K2_GetVectorParameterValue(TEXT("TeamColor"));
					TestTrue(TEXT("LED colour follows chosen skin, never ownership"), Skin == 1 ? Color.R > Color.B : Color.B > Color.R);
					++Lights;
				}
			}
			TestEqual(TEXT("Type symbol and rim both retained"), Lights, 2);
		}
		Piece->Destroy();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
