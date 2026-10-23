#if WITH_DEV_AUTOMATION_TESTS
#include "Camera/CameraComponent.h"
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
		TestEqual(TEXT("1v1 gameplay uses a slightly brighter broad fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 1.0f);
		TestTrue(TEXT("Fill remains overhead and centred"), FillActor->GetActorLocation().Equals(FVector(0.0f, 0.0f, 1150.0f)));
		TestTrue(TEXT("Whole board remains comfortably inside fill falloff"),
			Fill->AttenuationRadius > 1.8f * FVector(650.0f, 0.0f, Mode->ArenaSurfaceZ - 1150.0f).Size());
		TestTrue(TEXT("Large fill source avoids a pinprick reflection"), Fill->SourceRadius >= 400.0f);
		TestFalse(TEXT("Fill does not add expensive dynamic shadows"), Fill->CastShadows);
		TestEqual(TEXT("Neutral fill is diffuse-only, with reflections left to the softboxes"), Fill->SpecularScale, 0.0f);
		TestTrue(TEXT("Both gameplay softboxes are overhead, not low edge spotlights"),
			KeyActor->GetActorLocation().Z >= 800.0f && RimActor->GetActorLocation().Z >= 800.0f);
		const FlickArenaLighting::FParameters DefaultRig;
		TestTrue(TEXT("Cool fixture is cyan-blue rather than a white ceiling reflection"),
			Key->GetLightColor().B > Key->GetLightColor().R * 2.f);
		TestTrue(TEXT("Warm fixture remains distinctly amber"), Rim->GetLightColor().R > Rim->GetLightColor().B * 2.f);
		TestTrue(TEXT("Cool source is a continuous elongated stadium strip"), Key->SourceWidth > Key->SourceHeight * 3.f);
		TestTrue(TEXT("Warm source is a continuous elongated stadium strip"), Rim->SourceWidth > Rim->SourceHeight * 3.f);
		const auto InitialRimSpillLights = FlickArenaLighting::FindRig(World).RimSpillLights;
		const auto CheckRimSpill = [&](float Radius, float SurfaceZ, float WarmPreference, float ReflectionPreference)
		{
			const float Scale = Radius / 650.f;
			const auto Rig = FlickArenaLighting::FindRig(World);
			for (int32 Index = 0; Index < Rig.RimSpillLights.Num(); ++Index)
			{
				const auto* Actor = Rig.RimSpillLights[Index].Get();
				if (!TestNotNull(TEXT("Every rim segment has one local spill source"), Actor)) continue;
				TestTrue(TEXT("Rim sources retain their identity across formats and settings"), Actor == InitialRimSpillLights[Index]);
				TestTrue(TEXT("Rim source has a stable lookup tag"),
					Actor->ActorHasTag(FName(*FString::Printf(TEXT("FLICK.RimSpill%d"), Index))));
				TestFalse(TEXT("Rim sources are cosmetic, not replicated gameplay actors"), Actor->GetIsReplicated());
				const FVector Position = Actor->GetActorLocation();
				TestTrue(TEXT("Warm spill stays just outside the arena skirt"),
					FMath::IsNearlyEqual(Position.Size2D(), Radius + 5.f * Scale, .01f));
				TestTrue(TEXT("Warm spill stays below the playing plane"),
					FMath::IsNearlyEqual(Position.Z, SurfaceZ - 18.f * Scale, .01f));
				const FVector Radial = FVector(Position.X, Position.Y, 0.f).GetSafeNormal();
				TestTrue(TEXT("Elongated rim sources run tangentially around the edge"),
					FMath::IsNearlyZero(FVector::DotProduct(Actor->GetActorUpVector(), Radial), .001f)
					&& FMath::IsNearlyZero(Actor->GetActorUpVector().Z, .001f));
				const UPointLightComponent* Light = Actor->PointLightComponent;
				TestEqual(TEXT("Rim range scales locally rather than flooding the arena"), Light->AttenuationRadius, 450.f * Scale);
				TestEqual(TEXT("Rim source width scales with arena size"), Light->SourceRadius, 15.f * Scale);
				TestEqual(TEXT("Rim reflection softness scales with arena size"), Light->SoftSourceRadius, 32.f * Scale);
				TestEqual(TEXT("Rim source length scales with arena size"), Light->SourceLength, 140.f * Scale);
				TestEqual(TEXT("Rim spill is stable direct light, without a second stochastic indirect bounce"), Light->IndirectLightingIntensity, 0.f);
				TestFalse(TEXT("Rim spill adds no dynamic shadow passes"), Light->CastShadows);
				TestTrue(TEXT("Warm spill power follows the warm preference and board area"),
					FMath::IsNearlyEqual(Light->Intensity, DefaultRig.RimSpillLightLumens * FMath::Square(Scale) * WarmPreference, .01f));
				TestEqual(TEXT("Rim reflections follow the reflection preference without exposed source glare"), Light->SpecularScale, ReflectionPreference * .12f);
			}
		};
		CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ,
			GetValue(EScene::Gameplay, EControl::Rim), GetValue(EScene::Gameplay, EControl::Highlights));
		TestEqual(TEXT("Key reflection source spans the deck instead of a narrow card"), Key->SourceWidth, DefaultRig.SwitchyardKeySourceWidth);
		TestEqual(TEXT("Key reflection source is broad on both axes"), Key->SourceHeight, DefaultRig.SwitchyardKeySourceHeight);
		TestEqual(TEXT("Warm reflection source is broad on both axes"), Rim->SourceHeight, DefaultRig.SwitchyardRimSourceHeight);
		const float GameplaySky = Sky->Intensity;
		Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Menu retains the same neutral ambient coverage"), Sky->Intensity, GameplaySky);
		TestEqual(TEXT("Menu retains its original broad overhead fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity);
		TestEqual(TEXT("Menu keeps its own stronger softbox highlights"), Key->Intensity,
			Mode->TestPuckKeyLightIntensity * Mode->OneVsOneMenuSoftboxMultiplier);
		TestEqual(TEXT("Menu softboxes default to restrained glare"), Key->SpecularScale, 0.35f);
		TestEqual(TEXT("Gameplay softbox multiplier matches the menu"), Mode->OneVsOneGameplaySoftboxMultiplier, 250.0f);
		for (const int32 TeamSize : {1, 2, 3})
		{
			Mode->CurrentPlayersPerTeam = TeamSize;
			Mode->ArenaRadius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, TeamSize);
			Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
			Mode->SpawnLightingIfNeeded();
			const float MenuKey = Key->Intensity, MenuRim = Rim->Intensity, MenuFill = Fill->Intensity;
			const float MenuDirect = Mode->DirectionalLightActor->GetLightComponent()->Intensity;
			const FVector MenuKeyLocation = KeyActor->GetActorLocation();
			const float MenuKeyWidth = Key->SourceWidth, MenuRimHeight = Rim->SourceHeight;
			Mode->FrontendScreen = EFlickFrontendScreen::Playing;
			Mode->SpawnLightingIfNeeded();
			TestEqual(TEXT("All formats share menu/gameplay key power"), Key->Intensity, MenuKey);
			TestEqual(TEXT("All formats share menu/gameplay rim power"), Rim->Intensity, MenuRim);
			TestEqual(TEXT("All formats share menu/gameplay fill power"), Fill->Intensity, MenuFill);
			TestEqual(TEXT("All formats share menu/gameplay direct power"), Mode->DirectionalLightActor->GetLightComponent()->Intensity, MenuDirect);
			TestTrue(TEXT("All formats share menu/gameplay softbox placement"), KeyActor->GetActorLocation().Equals(MenuKeyLocation));
			TestEqual(TEXT("All formats share menu/gameplay key source area"), Key->SourceWidth, MenuKeyWidth);
			TestEqual(TEXT("All formats share menu/gameplay warm source area"), Rim->SourceHeight, MenuRimHeight);
			const float Scale = Mode->ArenaRadius / 650.f;
			TestEqual(TEXT("Large boards retain broad key coverage"), Key->SourceWidth, DefaultRig.SwitchyardKeySourceWidth * Scale);
			TestEqual(TEXT("Large boards retain broad warm coverage"), Rim->SourceHeight, DefaultRig.SwitchyardRimSourceHeight * Scale);
			CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ,
				GetValue(EScene::Gameplay, EControl::Rim), GetValue(EScene::Gameplay, EControl::Highlights));
		}
		Mode->CurrentPlayersPerTeam = 1;
		Mode->ArenaRadius = 650.f;
		Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
		Mode->SpawnLightingIfNeeded();
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
			const FRotator LightFrame(0.0f, FMath::RadiansToDegrees(FMath::Atan2(View.Y, View.X)) + 90.0f, 0.0f);
			TestTrue(TEXT("Camera tick actually moves the softboxes, not just the setup helper"),
				KeyActor->GetActorLocation().Equals(LightFrame.RotateVector(FVector(-100.0f, 1600.0f, 950.0f)), 0.01f));
			const auto* ViewCamera = Camera->FindComponentByClass<UCameraComponent>();
			if (TestNotNull(TEXT("Runtime view camera"), ViewCamera))
			{
				const auto& Post = ViewCamera->PostProcessSettings;
				const auto CheckStableTrace = [&]()
				{
					TestTrue(TEXT("Switchyard uses the standard reconstruction budget without extra samples"),
						Post.bOverride_LumenReflectionQuality && Post.LumenReflectionQuality == 1.0f);
					TestTrue(TEXT("Switchyard uses stable scene-traced GI in the menu and game"),
						Post.bOverride_LumenFinalGatherScreenTraces && !Post.LumenFinalGatherScreenTraces);
					TestTrue(TEXT("Switchyard keeps Lumen reflections without screen-edge radiance switching"),
						Post.bOverride_LumenReflectionsScreenTraces && !Post.LumenReflectionsScreenTraces);
				};
				CheckStableTrace();
				Camera->SetMenuPresentation(false);
				Camera->SetTestArenaPresentation(true);
				CheckStableTrace();
				Camera->SetTestArenaPresentation(false);
				Camera->SetOneVsOneArenaPresentation(false);
				Camera->SetBobGameplayFraming(true);
				TestFalse(TEXT("BOB retains its original GI trace path"), Post.bOverride_LumenFinalGatherScreenTraces);
				TestFalse(TEXT("BOB retains its original reflection trace path"), Post.bOverride_LumenReflectionsScreenTraces);
				Camera->SetBobGameplayFraming(false);
				Camera->SetOneVsOneArenaPresentation(true);
				Camera->SetMenuPresentation(true);
			}
		}
		Mode->FrontendScreen = EFlickFrontendScreen::Settings;
		Mode->SettingsReturnScreen = EFlickFrontendScreen::MainMenu;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Opening settings from the menu preserves its preview lights"), Key->Intensity,
			Mode->TestPuckKeyLightIntensity * Mode->OneVsOneMenuSoftboxMultiplier);
		Mode->SettingsReturnScreen = EFlickFrontendScreen::Paused;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Opening settings from a match preserves gameplay fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 1.0f);
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
		const auto ArchitecturalRig = FlickArenaLighting::FindRig(World);
		CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ, .8f, .2f);
		int32 WarmFixtureCount = 0;
		for (TActorIterator<ARectLight> It(World); It; ++It)
			if (It->GetName().StartsWith(TEXT("RectLight")))
				for (int32 Index = 0; Index < 4; ++Index)
					if (It->ActorHasTag(FName(*FString::Printf(TEXT("FLICK.StadiumWarm%d"), Index)))) ++WarmFixtureCount;
		TestEqual(TEXT("Repeated lighting applications never duplicate stadium fixtures"), WarmFixtureCount, 4);
		for (const auto& Fixture : ArchitecturalRig.StadiumWarmLights)
			if (TestNotNull(TEXT("Each seating bank has one warm source"), Fixture.Get()))
			{
				TestFalse(TEXT("Seating spill never adds dynamic shadow passes"), Fixture->RectLightComponent->CastShadows);
				TestTrue(TEXT("Stadium sources sit above the playing plane so the deck can reflect them"),
					Fixture->GetActorLocation().Z > Mode->ArenaSurfaceZ + 300.f);
				TestTrue(TEXT("Seating warmth follows the saved warm-light preference"),
					FMath::IsNearlyEqual(Fixture->RectLightComponent->Intensity, DefaultRig.StadiumWarmLightLumens * .8f, .1f));
			}
		TestEqual(TEXT("Ambient preference applies live"), Sky->Intensity, 1.05f * Mode->OneVsOneSkyLightMultiplier * 1.5f);
		TestEqual(TEXT("Fill preference applies without compounding"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f);
		TestEqual(TEXT("Cool softbox preference applies"), Key->Intensity, Mode->TestPuckKeyLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier * 0.5f);
		TestEqual(TEXT("Warm softbox preference applies"), Rim->Intensity, Mode->TestPuckRimLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier * 0.8f);
		TestEqual(TEXT("Team accent preference applies"), Mode->Player1AccentLight->PointLightComponent->Intensity, Mode->MenuAccentLightIntensity * 2.0f);
		TestEqual(TEXT("Overhead preference applies"), Mode->DirectionalLightActor->GetLightComponent()->Intensity, 1.45f * 0.68f * 0.5f);
		TestEqual(TEXT("Reflection preference applies without changing diffuse fill"), Key->SpecularScale, 0.2f);
		SetValue(EScene::Gameplay, EControl::Highlights, 0.f);
		Mode->SpawnLightingIfNeeded();
		CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ, .8f, 0.f);
		SetValue(EScene::Gameplay, EControl::Highlights, .2f);
		SetValue(EScene::Gameplay, EControl::Rim, 0.f);
		Mode->SpawnLightingIfNeeded();
		CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ, 0.f, .2f);
		SetValue(EScene::Gameplay, EControl::Rim, .8f);
		Mode->SpawnLightingIfNeeded();
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
			TestEqual(TEXT("Party frontend retains the shared preset"), Fill->Intensity, 300000.0f);
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
				TestTrue(TEXT("Remote-client larger formats retain the shared preset"),
					FMath::IsNearlyEqual(Fill->Intensity, 300000.0f * FMath::Square(Scale), 0.1f));
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
		TestEqual(TEXT("Gameplay fill changes carry back into the menu"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f);

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
			TestTrue(TEXT("Larger formats share the off-axis key height"),
				FMath::IsNearlyEqual(KeyActor->GetActorLocation().Z, 250.f + 700.f * Scale, .01f));
			TestTrue(TEXT("Larger formats scale saved softbox power with board area"),
				FMath::IsNearlyEqual(Key->Intensity, Mode->TestPuckKeyLightIntensity * Mode->OneVsOneGameplaySoftboxMultiplier * 0.5f * FMath::Square(Scale), 0.1f));
			TestEqual(TEXT("Larger formats scale the far-rim softbox range"), Key->AttenuationRadius, 2500.0f * Scale);
			TestEqual(TEXT("Larger formats share reflection preference"), Key->SpecularScale, 0.2f);
			CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ, .8f, .2f);
			Mode->FrontendScreen = EFlickFrontendScreen::MainMenu;
			Mode->SpawnLightingIfNeeded();
			TestEqual(TEXT("Larger format menus retain shared ambient settings"), Sky->Intensity, 1.05f * Mode->OneVsOneSkyLightMultiplier * 1.5f);
			TestTrue(TEXT("Larger format menus retain shared fill settings"),
				FMath::IsNearlyEqual(Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f * FMath::Square(Scale), 0.1f));
			TestEqual(TEXT("Larger format menus retain shared reflection settings"), Key->SpecularScale, 0.2f);
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
				const FRotator Frame(0.0f, FMath::RadiansToDegrees(FMath::Atan2(View.Y, View.X)) + 90.0f, 0.0f);
				FVector Expected = Frame.RotateVector(FVector(-100.0f, 1600.0f, 950.0f) * Scale);
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
		TestEqual(TEXT("BOB fill uses saved preferences without compounding"), Fill->Intensity, DefaultRig.BobFillLightLumens * 2.0f);
		TestEqual(TEXT("BOB ambient uses saved preferences"), Sky->Intensity, DefaultRig.BobSkyIntensity * 1.5f);
		TestEqual(TEXT("BOB key uses saved preferences"), Key->Intensity, DefaultRig.BobKeyLightLumens * 0.5f);
		TestEqual(TEXT("BOB rim uses saved preferences"), Rim->Intensity, DefaultRig.BobRimLightLumens * 0.8f);
		TestEqual(TEXT("BOB accents use saved preferences"), Mode->Player1AccentLight->PointLightComponent->Intensity, DefaultRig.BobWarmRailLightLumens * 2.0f);
		TestEqual(TEXT("BOB direct light uses saved preferences"), Mode->DirectionalLightActor->GetLightComponent()->Intensity, DefaultRig.BobDirectionalIntensity * 0.5f);
		TestEqual(TEXT("BOB highlights use saved preferences"), Key->SpecularScale, 0.2f);
		TestEqual(TEXT("BOB fill does not create reflection glare"), Fill->SpecularScale, 0.0f);
		for (const auto& Actor : FlickArenaLighting::FindRig(World).RimSpillLights)
			if (TestNotNull(TEXT("Switching to BOB retains reusable rim sources"), Actor.Get()))
				TestEqual(TEXT("Switchyard rim spill never illuminates the BOB arena"), Actor->PointLightComponent->Intensity, 0.f);
		if (ClientState && ClientController)
		{
			ClientState->ActiveMatchVariant = EFlickMatchVariant::Bob;
			ClientState->bPartyActive = false;
			ClientState->SetMatchPhase(EFlickMatchPhase::Aiming);
			ClientController->RefreshLocalLighting();
			TestEqual(TEXT("BOB remote-client path applies the local fill preference"), Fill->Intensity, DefaultRig.BobFillLightLumens * 2.0f);
			TestEqual(TEXT("BOB remote-client path applies the local key preference"), Key->Intensity, DefaultRig.BobKeyLightLumens * 0.5f);
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
		TestEqual(TEXT("BOB reset restores the shared gameplay fill default"), Fill->Intensity, DefaultRig.BobFillLightLumens);
		SetValue(EScene::Gameplay, EControl::Fill, 2.0f);
		Mode->bTestArenaMode = true;
		Mode->ActiveMatchVariant = EFlickMatchVariant::Classic;
		Mode->CurrentPlayersPerTeam = 1;
		Mode->ArenaRadius = 650.0f;
		Mode->FrontendScreen = EFlickFrontendScreen::Playing;
		Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Returning to 1v1 restores its custom fill"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 2.0f);
		ResetToDefaults(); Mode->SpawnLightingIfNeeded();
		TestEqual(TEXT("Reset applies live"), Fill->Intensity, Mode->OneVsOneFillLightIntensity * 1.0f);
		CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ,
			GetValue(EScene::Gameplay, EControl::Rim), GetValue(EScene::Gameplay, EControl::Highlights));
		const FlickArenaLighting::FRig Found = FlickArenaLighting::FindRig(World);
		TestTrue(TEXT("Client-side lookup reuses the same rig"), Found.ArenaFillLight == FillActor && Found.TestPuckKeyLight == KeyActor);
		TestTrue(TEXT("Existing light actors are reused across formats and screens"),
			Mode->ArenaFillLight == FillActor && Mode->TestPuckKeyLight == KeyActor
			&& Mode->TestPuckRimLight == RimActor && Mode->SkyLightActor == SkyActor);
		auto NonPremiumRig = Found;
		auto NonPremiumParameters = DefaultRig;
		NonPremiumParameters.bPremiumArena = false;
		FlickArenaLighting::Configure(World, NonPremiumRig, NonPremiumParameters);
		for (const auto& Actor : NonPremiumRig.RimSpillLights)
			if (Actor)
				TestEqual(TEXT("Non-premium scenes disable the local rim ring"), Actor->PointLightComponent->Intensity, 0.f);
		Mode->SpawnLightingIfNeeded();
		CheckRimSpill(Mode->ArenaRadius, Mode->ArenaSurfaceZ,
			GetValue(EScene::Gameplay, EControl::Rim), GetValue(EScene::Gameplay, EControl::Highlights));
		int32 PointLights = 0, RectLights = 0;
		for (TActorIterator<APointLight> It(World); It; ++It) ++PointLights;
		for (TActorIterator<ARectLight> It(World); It; ++It) ++RectLights;
		TestEqual(TEXT("One rig contains three broad lights and exactly twelve local rim sources"), PointLights, 15);
		TestEqual(TEXT("One rig contains two softboxes and four seating sources"), RectLights, 6);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickEpicReflectionBudgetTest, "FLICK.Visuals.EpicReflectionBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickEpicReflectionBudgetTest::RunTest(const FString& Parameters)
{
	FConfigFile Config;
	const FString ConfigPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultScalability.ini"));
	if (!TestTrue(TEXT("Project reflection scalability config exists"),
		IFileManager::Get().FileExists(*ConfigPath))) return false;
	Config.Read(ConfigPath);
	int32 Factor = 0, Checkerboard = 0;
	TestTrue(TEXT("Epic explicitly sets a reflection tracing factor"),
		Config.GetInt(TEXT("ReflectionQuality@3"), TEXT("r.Lumen.Reflections.DownsampleFactor"), Factor));
	TestTrue(TEXT("Epic explicitly enables checkerboard reconstruction"),
		Config.GetInt(TEXT("ReflectionQuality@3"), TEXT("r.Lumen.Reflections.DownsampleCheckerboard"), Checkerboard));
	TestEqual(TEXT("Epic traces half the pixels, not a quarter-resolution grid"), Factor, 2);
	TestEqual(TEXT("Epic retains full vertical reflection tracing resolution"), Checkerboard, 1);
	for (const TCHAR* Section : {TEXT("ReflectionQuality@0"), TEXT("ReflectionQuality@1"),
		TEXT("ReflectionQuality@2"), TEXT("ReflectionQuality@Cine")})
	{
		Checkerboard = -1;
		TestTrue(TEXT("Other presets explicitly reset the checkerboard state"),
			Config.GetInt(Section, TEXT("r.Lumen.Reflections.DownsampleCheckerboard"), Checkerboard));
		TestEqual(FString::Printf(TEXT("%s does not inherit Epic checkerboarding"), Section), Checkerboard, 0);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLightingRigLookupTest, "FLICK.Visuals.LightingRigLookup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickLightingRigLookupTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Lookup test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	TestNull(TEXT("Missing fixtures remain absent"), FlickArenaLighting::FindRig(World).ArenaFillLight.Get());
	auto* Accent = World->SpawnActor<APointLight>();
	Accent->Tags = {TEXT("FLICK.Player1AccentLight"), TEXT("FLICK.Player2AccentLight")};
	auto* Fill = World->SpawnActor<APointLight>();
	Fill->Tags.Add(TEXT("FLICK.ArenaFillLight"));
	auto* RimSpill = World->SpawnActor<APointLight>();
	RimSpill->Tags = {TEXT("FLICK.RimSpill0"), TEXT("FLICK.RimSpill11")};
	auto* Duplicate = World->SpawnActor<APointLight>();
	Duplicate->Tags = {TEXT("FLICK.Player1AccentLight"), TEXT("FLICK.Player2AccentLight"), TEXT("FLICK.ArenaFillLight"),
		TEXT("FLICK.RimSpill0"), TEXT("FLICK.RimSpill11")};
	auto* Key = World->SpawnActor<ARectLight>();
	Key->Tags.Add(TEXT("FLICK.TestPuckKeyLight"));
	auto* Rim = World->SpawnActor<ARectLight>();
	Rim->Tags.Add(TEXT("FLICK.TestPuckRimLight"));
	auto* DuplicateRect = World->SpawnActor<ARectLight>();
	DuplicateRect->Tags = {TEXT("FLICK.TestPuckKeyLight"), TEXT("FLICK.TestPuckRimLight")};
	const auto Rig = FlickArenaLighting::FindRig(World);
	TestTrue(TEXT("Multi-tag actor can supply both accents"), Rig.Player1AccentLight == Accent && Rig.Player2AccentLight == Accent);
	TestTrue(TEXT("First matching fill wins over duplicates"), Rig.ArenaFillLight == Fill);
	TestTrue(TEXT("First matching rim sources win over duplicates at both ends of the tag range"),
		Rig.RimSpillLights[0] == RimSpill && Rig.RimSpillLights[11] == RimSpill);
	TestNull(TEXT("Lookup never creates an absent rim source"), Rig.RimSpillLights[1].Get());
	TestTrue(TEXT("First matching softboxes win over duplicates"), Rig.TestPuckKeyLight == Key && Rig.TestPuckRimLight == Rim);
	TestNull(TEXT("Lookup never creates absent directional fixtures"), Rig.DirectionalLightActor.Get());
#if WITH_EDITOR
	// Exercise the real dedicated-server guard without opening a network socket.
	const auto CountActors = [World]()
	{
		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It) ++Count;
		return Count;
	};
	const int32 ExistingActors = CountActors();
	World->WorldType = EWorldType::PIE;
	World->SetPlayInEditorInitialNetMode(NM_DedicatedServer);
	TestEqual(TEXT("Fixture represents a dedicated server"), World->GetNetMode(), NM_DedicatedServer);
	FlickArenaLighting::FRig DedicatedRig;
	FlickArenaLighting::Configure(World, DedicatedRig, FlickArenaLighting::FParameters());
	TestNull(TEXT("Dedicated servers never create or configure a presentation fill"), DedicatedRig.ArenaFillLight.Get());
	TestNull(TEXT("Dedicated servers never create presentation softboxes"), DedicatedRig.TestPuckKeyLight.Get());
	for (const auto& Source : DedicatedRig.RimSpillLights)
		TestNull(TEXT("Dedicated servers never create rim sources"), Source.Get());
	TestEqual(TEXT("Dedicated lighting configuration never spawns cosmetic actors"), CountActors(), ExistingActors);
	World->WorldType = EWorldType::Game;
#endif
	return true;
}
#endif
