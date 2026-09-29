#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickLightingSettings.h"
#include "Core/FlickModeRules.h"
#include "Arena/FlickArenaLighting.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Game/FlickGameMode.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPlayerController.h"
#include "Game/FlickGameState.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickOneVsOneLightingTest, "FLICK.Visuals.KnockoutLightingTheme",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickOneVsOneLightingTest::RunTest(const FString& Parameters)
{
	using namespace FlickLightingSettings;
	const FString TestIni = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("LightingRig-") + FGuid::NewGuid().ToString() + TEXT(".ini"));
	const FString PreviousIni = GGameUserSettingsIni;
	GGameUserSettingsIni = TestIni;
	GConfig->Add(TestIni, FConfigFile());
	ON_SCOPE_EXIT { GGameUserSettingsIni = PreviousIni; GConfig->UnloadFile(TestIni); IFileManager::Get().Delete(*TestIni); };
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickGameMode* Mode = World->SpawnActor<AFlickGameMode>();
	if (!TestNotNull(TEXT("Game mode"), Mode))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	Mode->bTestArenaMode = true;
	Mode->FrontendScreen = EFlickFrontendScreen::Playing;
	Mode->CurrentPlayersPerTeam = 1;
	Mode->ArenaRadius = 650.0f;
	Mode->ArenaSurfaceZ = 250.0f;
	Mode->SpawnLightingIfNeeded();
	APointLight* FillActor = Mode->ArenaFillLight;
	ARectLight* KeyActor = Mode->TestPuckKeyLight;
	ARectLight* RimActor = Mode->TestPuckRimLight;
	ASkyLight* SkyActor = Mode->SkyLightActor;
	if (TestNotNull(TEXT("Fill"), FillActor) && TestNotNull(TEXT("Key"), KeyActor)
		&& TestNotNull(TEXT("Rim"), RimActor) && TestNotNull(TEXT("Sky"), SkyActor))
	{
		UPointLightComponent* Fill = FillActor->PointLightComponent;
		URectLightComponent* Key = KeyActor->RectLightComponent;
		URectLightComponent* Rim = RimActor->RectLightComponent;
		USkyLightComponent* Sky = SkyActor->GetLightComponent();
		TestEqual(TEXT("1v1 gains evenly distributed reflected environment light"), Sky->Intensity,
			1.05f * Mode->OneVsOneSkyLightMultiplier);
		TestEqual(TEXT("1v1 gameplay uses a slightly brighter broad fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 1.2f);
		TestTrue(TEXT("Fill remains overhead and centred"), FillActor->GetActorLocation().Equals(FVector(0.0f, 0.0f, 1150.0f)));
		TestTrue(TEXT("Whole board remains comfortably inside fill falloff"),
			Fill->AttenuationRadius > 1.8f * FVector(650.0f, 0.0f, Mode->ArenaSurfaceZ - 1150.0f).Size());
		TestTrue(TEXT("Large fill source avoids a pinprick reflection"), Fill->SourceRadius >= 400.0f);
		TestFalse(TEXT("Fill does not add expensive dynamic shadows"), Fill->CastShadows);
		TestEqual(TEXT("Neutral fill is diffuse-only, with reflections left to the softboxes"), Fill->SpecularScale, 0.0f);
		TestTrue(TEXT("Both gameplay softboxes are overhead, not low edge spotlights"),
			KeyActor->GetActorLocation().Z >= 1000.0f && RimActor->GetActorLocation().Z >= 1000.0f);
		TestTrue(TEXT("Gameplay sources spread highlights broadly"), Key->SourceWidth >= 500.0f && Rim->SourceHeight >= 400.0f);
		const float GameplaySky = Sky->Intensity;
		Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Menu retains the same neutral ambient coverage"), Sky->Intensity, GameplaySky);
		TestEqual(TEXT("Menu retains its original broad overhead fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity);
		TestEqual(TEXT("Menu keeps its own stronger softbox highlights"), Key->Intensity,
			Mode->TestPuckKeyLightIntensity * Mode->OneVsOneMenuSoftboxMultiplier);
		TestEqual(TEXT("Menu softboxes default to restrained glare"), Key->SpecularScale, 0.35f);
		const FVector InitialCamera(170.0f, -1600.0f, 1140.0f);
		FlickArenaLighting::UpdateMenuSoftboxes(KeyActor, RimActor, InitialCamera, 650.0f, 250.0f);
		const FVector InitialKey = KeyActor->GetActorLocation();
		const FVector InitialRim = RimActor->GetActorLocation();
		const FRotator Orbit(0.0f, 90.0f, 0.0f);
		FlickArenaLighting::UpdateMenuSoftboxes(KeyActor, RimActor, Orbit.RotateVector(InitialCamera), 650.0f, 250.0f);
		TestTrue(TEXT("Cool softbox maintains its offset as the menu orbits"), KeyActor->GetActorLocation().Equals(Orbit.RotateVector(InitialKey), 0.01f));
		TestTrue(TEXT("Warm softbox maintains its offset as the menu orbits"), RimActor->GetActorLocation().Equals(Orbit.RotateVector(InitialRim), 0.01f));
		TestTrue(TEXT("Menu softboxes still aim at the deck centre"),
			KeyActor->GetActorForwardVector().Equals((FVector(0.0f, 0.0f, 250.0f) - KeyActor->GetActorLocation()).GetSafeNormal(), 0.01f));
		AFlickCameraPawn* Camera = World->SpawnActor<AFlickCameraPawn>();
		if (TestNotNull(TEXT("Menu camera"), Camera))
		{
			Camera->SetOneVsOneArenaPresentation(true);
			Camera->SetMenuPresentation(true);
			Camera->SetMenuOrbitEnabled(true);
			Camera->ApplyCameraSettings();
			Camera->Tick(1.0f);
			const FVector View = Camera->GetActorLocation();
			const FRotator LightFrame(0.0f, FMath::RadiansToDegrees(FMath::Atan2(View.Y, View.X)) + 56.0f, 0.0f);
			TestTrue(TEXT("Camera tick actually moves the softboxes, not just the setup helper"),
				KeyActor->GetActorLocation().Equals(LightFrame.RotateVector(FVector(-100.0f, 900.0f, 950.0f)), 0.01f));
		}
		Mode->FrontendScreen = EFlickFrontendScreen::Settings;
		Mode->SettingsReturnScreen = EFlickFrontendScreen::MainMenu;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Opening settings from the menu preserves its preview lights"), Key->Intensity,
			Mode->TestPuckKeyLightIntensity * Mode->OneVsOneMenuSoftboxMultiplier);
		Mode->SettingsReturnScreen = EFlickFrontendScreen::Paused;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Opening settings from a match preserves gameplay fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 1.2f);
		TestEqual(TEXT("Opening settings from a match preserves gameplay softboxes"), Key->Intensity,
			Mode->TestPuckKeyLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier);
		// Each preference changes its own light, and applying twice cannot compound values.
		SetValue(EScene::Gameplay, EControl::Ambient, 1.5f);
		SetValue(EScene::Gameplay, EControl::Fill, 2.0f);
		SetValue(EScene::Gameplay, EControl::Key, 0.5f);
		SetValue(EScene::Gameplay, EControl::Rim, 0.8f);
		SetValue(EScene::Gameplay, EControl::Accents, 2.0f);
		SetValue(EScene::Gameplay, EControl::Direct, 0.5f);
		SetValue(EScene::Gameplay, EControl::Highlights, 0.2f);
		Mode->SpawnLightingIfNeeded(); Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Ambient preference applies live"), Sky->Intensity, 1.05f * Mode->OneVsOneSkyLightMultiplier * 1.5f);
		TestEqual(TEXT("Fill preference applies without compounding"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f);
		TestEqual(TEXT("Cool softbox preference applies"), Key->Intensity, Mode->TestPuckKeyLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier * 0.5f);
		TestEqual(TEXT("Warm softbox preference applies"), Rim->Intensity, Mode->TestPuckRimLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier * 0.8f);
		TestEqual(TEXT("Team accent preference applies"), Mode->Player1AccentLight->PointLightComponent->Intensity, 380.0f);
		TestEqual(TEXT("Overhead preference applies"), Mode->DirectionalLightActor->GetLightComponent()->Intensity, 1.45f * 0.5f);
		TestEqual(TEXT("Reflection preference applies without changing diffuse fill"), Key->SpecularScale, 0.2f);
		// Exercise the same local-controller path used without an authoritative game mode.
		AFlickGameState* ClientState = World->SpawnActor<AFlickGameState>();
		AFlickPlayerController* ClientController = World->SpawnActor<AFlickPlayerController>();
		if (TestNotNull(TEXT("Client presentation state"), ClientState) && TestNotNull(TEXT("Local presentation controller"), ClientController))
		{
			World->SetGameState(ClientState);
			ClientController->SetAsLocalPlayerController();
			TestTrue(TEXT("Fixture represents a local player"), ClientController->IsLocalController());
			TestNull(TEXT("Fixture uses the client path without an authoritative game mode"), World->GetAuthGameMode());
			ClientState->bPartyActive = true;
			ClientState->SetMatchPhase(EFlickMatchPhase::Aiming);
			ClientController->RefreshLocalLighting();
			TestEqual(TEXT("A party in a live match uses gameplay lights, not menu lights"), Fill->Intensity, 300000.0f);
			ClientState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
			ClientController->RefreshLocalLighting();
			TestEqual(TEXT("Party frontend uses the local menu preset"), Fill->Intensity, 150000.0f);
			TestTrue(TEXT("Local client application reuses existing fixtures"), FlickArenaLighting::FindRig(World).ArenaFillLight == FillActor);
			for (const int32 TeamSize : {2, 3})
			{
				ClientState->PlayersPerTeam = TeamSize;
				ClientState->SetMatchPhase(EFlickMatchPhase::Aiming);
				ClientController->RefreshLocalLighting();
				const float Scale = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize) / 650.0f;
				TestTrue(TEXT("Remote-client larger formats use the same saved gameplay preset and area scaling"),
					FMath::IsNearlyEqual(Fill->Intensity, 300000.0f * FMath::Square(Scale), 0.1f));
				ClientState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
				ClientController->RefreshLocalLighting();
				TestTrue(TEXT("Remote-client larger formats keep menu/gameplay presets separate"),
					FMath::IsNearlyEqual(Fill->Intensity, 150000.0f * FMath::Square(Scale), 0.1f));
			}
			ClientState->bPrivateMatchActive = true;
			ClientState->PrivateMatchSettings.ArenaScale = 1.3f;
			ClientState->SetMatchPhase(EFlickMatchPhase::Aiming);
			ClientController->RefreshLocalLighting();
			const float PrivateScale = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, 3) * 1.3f / 650.0f;
			TestTrue(TEXT("Private arena-size adjustments preserve the lighting density"),
				FMath::IsNearlyEqual(Fill->Intensity, 300000.0f * FMath::Square(PrivateScale), 0.1f));
			ClientState->bPrivateMatchActive = false;
			ClientState->PlayersPerTeam = 1;
		}
		Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Custom gameplay fill cannot leak into the menu"), Fill->Intensity, Mode->OneVsOneFillLightIntensity);

		for (const int32 TeamSize : {2, 3})
		{
			Mode->CurrentPlayersPerTeam = TeamSize;
			Mode->ArenaRadius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize);
			const float Scale = Mode->ArenaRadius / 650.0f;
			Mode->FrontendScreen = EFlickFrontendScreen::Playing;
			Mode->SpawnLightingIfNeeded();
			TestEqual(TEXT("Larger formats share the saved neutral ambient response"), Sky->Intensity, 1.05f * Mode->OneVsOneSkyLightMultiplier * 1.5f);
			TestTrue(TEXT("Larger formats scale broad fill power with board area"),
				FMath::IsNearlyEqual(Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f * FMath::Square(Scale), 0.1f));
			TestEqual(TEXT("Larger formats scale broad fill source radius"), Fill->SourceRadius, 480.0f * Scale);
			TestEqual(TEXT("Larger formats scale broad fill falloff"), Fill->AttenuationRadius, 2200.0f * Scale);
			TestEqual(TEXT("Larger formats keep fill diffuse-only"), Fill->SpecularScale, 0.0f);
			TestTrue(TEXT("Larger formats scale fill height above the deck rather than world origin"),
				FillActor->GetActorLocation().Equals(FVector(0.0f, 0.0f, 250.0f + 900.0f * Scale), 0.01f));
			TestTrue(TEXT("Larger formats share the broad overhead key placement"),
				KeyActor->GetActorLocation().Equals(FVector(-360.0f * Scale, -280.0f * Scale, 250.0f + 800.0f * Scale), 0.01f));
			TestTrue(TEXT("Larger formats scale saved softbox power with board area"),
				FMath::IsNearlyEqual(Key->Intensity, Mode->TestPuckKeyLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier * 0.5f * FMath::Square(Scale), 0.1f));
			TestEqual(TEXT("Larger formats scale softbox range"), Key->AttenuationRadius, 1850.0f * Scale);
			TestEqual(TEXT("Larger formats share reflection preference"), Key->SpecularScale, 0.2f);
			Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
			Mode->SpawnLightingIfNeeded();
			TestEqual(TEXT("Larger format menus use their own saved ambient preset"), Sky->Intensity, 1.05f * Mode->OneVsOneSkyLightMultiplier);
			TestTrue(TEXT("Larger format menus use their own saved fill preset"),
				FMath::IsNearlyEqual(Fill->Intensity, Mode->OneVsOneFillLightIntensity * FMath::Square(Scale), 0.1f));
			TestEqual(TEXT("Larger format menus retain restrained reflections"), Key->SpecularScale, 0.35f);
			// The same camera-following rig must stay off-axis on the larger board.
			FlickArenaLighting::UpdateMenuSoftboxes(KeyActor, RimActor, InitialCamera, Mode->ArenaRadius, 250.0f);
			const FVector LargerKey = KeyActor->GetActorLocation();
			FlickArenaLighting::UpdateMenuSoftboxes(KeyActor, RimActor, Orbit.RotateVector(InitialCamera), Mode->ArenaRadius, 250.0f);
			TestTrue(TEXT("Larger menu softboxes keep their off-axis angle throughout the orbit"),
				KeyActor->GetActorLocation().Equals(Orbit.RotateVector(LargerKey), 0.01f));
			if (Camera && ClientState)
			{
				ClientState->PlayersPerTeam = TeamSize;
				Camera->SetTestArenaPresentation(true);
				Camera->SetOneVsOneArenaPresentation(false);
				Camera->Tick(1.0f);
				const FVector View = Camera->GetActorLocation();
				const FRotator Frame(0.0f, FMath::RadiansToDegrees(FMath::Atan2(View.Y, View.X)) + 56.0f, 0.0f);
				FVector Expected = Frame.RotateVector(FVector(-100.0f, 900.0f, 950.0f) * Scale);
				Expected.Z = 250.0f + 700.0f * Scale;
				TestTrue(TEXT("Larger-format camera tick uses its actual arena size for softbox tracking"),
					KeyActor->GetActorLocation().Equals(Expected, 0.01f));
			}
		}
		// BOB applies the same preferences to its own warmer baseline rig.
		Mode->bTestArenaMode = false;
		Mode->ActiveMatchVariant = EFlickMatchVariant::Bob;
		Mode->ArenaRadius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Bob, 1);
		Mode->FrontendScreen = EFlickFrontendScreen::Playing;
		Mode->SpawnLightingIfNeeded();
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("BOB fill uses saved preferences without compounding"), Fill->Intensity, 520.0f);
		TestEqual(TEXT("BOB ambient uses saved preferences"), Sky->Intensity, 0.55f * 1.5f);
		TestEqual(TEXT("BOB key uses saved preferences"), Key->Intensity, Mode->TestPuckKeyLightIntensity * 12.0f * 0.5f);
		TestEqual(TEXT("BOB rim uses saved preferences"), Rim->Intensity, Mode->TestPuckRimLightIntensity * 12.0f * 0.8f);
		TestEqual(TEXT("BOB accents use saved preferences"), Mode->Player1AccentLight->PointLightComponent->Intensity, 410.0f);
		TestEqual(TEXT("BOB direct light uses saved preferences"), Mode->DirectionalLightActor->GetLightComponent()->Intensity, 1.32f * 0.5f);
		TestEqual(TEXT("BOB highlights use saved preferences"), Key->SpecularScale, 0.2f);
		TestEqual(TEXT("BOB fill does not create reflection glare"), Fill->SpecularScale, 0.0f);
		if (ClientState && ClientController)
		{
			ClientState->ActiveMatchVariant = EFlickMatchVariant::Bob;
			ClientState->bPartyActive = false;
			ClientState->SetMatchPhase(EFlickMatchPhase::Aiming);
			ClientController->RefreshLocalLighting();
			TestEqual(TEXT("BOB remote-client path applies the local fill preference"), Fill->Intensity, 520.0f);
			TestEqual(TEXT("BOB remote-client path applies the local key preference"), Key->Intensity, 1560.0f);
			ClientState->ActiveMatchVariant = EFlickMatchVariant::Classic;
		}
		for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
			SetValue(EScene::Gameplay, static_cast<EControl>(Index), 0.0f);
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("BOB fill can be disabled"), Fill->Intensity, 0.0f);
		TestEqual(TEXT("BOB ambient can be disabled"), Sky->Intensity, 0.0f);
		TestEqual(TEXT("BOB key can be disabled"), Key->Intensity, 0.0f);
		TestEqual(TEXT("BOB rim can be disabled"), Rim->Intensity, 0.0f);
		TestEqual(TEXT("BOB accents can be disabled"), Mode->Player1AccentLight->PointLightComponent->Intensity, 0.0f);
		TestEqual(TEXT("BOB direct light can be disabled"), Mode->DirectionalLightActor->GetLightComponent()->Intensity, 0.0f);
		TestEqual(TEXT("BOB highlights can be disabled"), Key->SpecularScale, 0.0f);
		ResetToDefaults(); Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("BOB reset restores the shared gameplay fill default"), Fill->Intensity, 260.0f * 1.2f);
		SetValue(EScene::Gameplay, EControl::Fill, 2.0f);
		Mode->bTestArenaMode = true;
		Mode->ActiveMatchVariant = EFlickMatchVariant::Classic;
		Mode->CurrentPlayersPerTeam = 1;
		Mode->ArenaRadius = 650.0f;
		Mode->FrontendScreen = EFlickFrontendScreen::Playing;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Returning to 1v1 restores its custom fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f);
		ResetToDefaults(); Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Reset applies live"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 1.2f);
		const FlickArenaLighting::FRig Found = FlickArenaLighting::FindRig(World);
		TestTrue(TEXT("Client-side lookup reuses the same rig"), Found.ArenaFillLight == FillActor && Found.TestPuckKeyLight == KeyActor);
		TestTrue(TEXT("Existing light actors are reused across formats and screens"),
			Mode->ArenaFillLight == FillActor && Mode->TestPuckKeyLight == KeyActor
			&& Mode->TestPuckRimLight == RimActor && Mode->SkyLightActor == SkyActor);
		int32 PointLights = 0, RectLights = 0;
		for (TActorIterator<APointLight> It(World); It; ++It) ++PointLights;
		for (TActorIterator<ARectLight> It(World); It; ++It) ++RectLights;
		TestEqual(TEXT("No extra point lights were added"), PointLights, 3);
		TestEqual(TEXT("No extra softboxes were added"), RectLights, 2);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
