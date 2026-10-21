#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickArenaLighting.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/FlickLightingSettings.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Player/FlickCameraPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBobLightingTest, "FLICK.Visuals.BobLightingTheme",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBobLightingTest::RunTest(const FString& Parameters)
{
	using namespace FlickArenaLighting;
	using namespace FlickLightingSettings;
	// Never change the developer's saved lighting preferences while testing.
	const FString TestIni = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("BobLighting-") + FGuid::NewGuid().ToString() + TEXT(".ini"));
	const FString PreviousIni = GGameUserSettingsIni;
	GGameUserSettingsIni = TestIni;
	GConfig->Add(TestIni, FConfigFile());
	ON_SCOPE_EXIT
	{
		GGameUserSettingsIni = PreviousIni;
		GConfig->UnloadFile(TestIni);
		IFileManager::Get().Delete(*TestIni);
	};

	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("BOB lighting world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };

	FParameters Bob;
	Bob.bTestArenaMode = false;
	Bob.bClassicArenaLighting = false;
	Bob.bBobArenaLighting = true;
	Bob.bPremiumArena = false;
	Bob.ArenaRadius = 620.f;
	Bob.ArenaSurfaceZ = 250.f;
	FRig Rig;
	Configure(World, Rig, Bob);
	if (!TestNotNull(TEXT("BOB key"), Rig.TestPuckKeyLight.Get())
		|| !TestNotNull(TEXT("BOB rim"), Rig.TestPuckRimLight.Get())
		|| !TestNotNull(TEXT("BOB fill"), Rig.ArenaFillLight.Get())
		|| !TestNotNull(TEXT("BOB warm rail"), Rig.Player1AccentLight.Get())
		|| !TestNotNull(TEXT("BOB cool rail"), Rig.Player2AccentLight.Get())
		|| !TestNotNull(TEXT("BOB sky"), Rig.SkyLightActor.Get())
		|| !TestNotNull(TEXT("BOB directional"), Rig.DirectionalLightActor.Get())) return false;
	const FRig Original = Rig;
	URectLightComponent* Key = Rig.TestPuckKeyLight->RectLightComponent;
	URectLightComponent* Rim = Rig.TestPuckRimLight->RectLightComponent;
	UPointLightComponent* Fill = Rig.ArenaFillLight->PointLightComponent;
	UPointLightComponent* Warm = Rig.Player1AccentLight->PointLightComponent;
	UPointLightComponent* Cool = Rig.Player2AccentLight->PointLightComponent;

	const auto CheckCounts = [&](int32 ExpectedPoints, int32 ExpectedRects)
	{
		int32 Points = 0, Rects = 0;
		for (TActorIterator<APointLight> It(World); It; ++It) ++Points;
		for (TActorIterator<ARectLight> It(World); It; ++It) ++Rects;
		TestEqual(TEXT("BOB never stacks duplicate point fixtures"), Points, ExpectedPoints);
		TestEqual(TEXT("BOB never stacks duplicate softboxes"), Rects, ExpectedRects);
	};
	const auto CheckPose = [&](float Scale, float SurfaceZ)
	{
		TestTrue(TEXT("BOB key scales from the source board and playing plane"),
			Rig.TestPuckKeyLight->GetActorLocation().Equals(FVector(-250.f * Scale, 1250.f * Scale, SurfaceZ + 850.f * Scale), .01f));
		TestTrue(TEXT("BOB rim scales from the source board and playing plane"),
			Rig.TestPuckRimLight->GetActorLocation().Equals(FVector(500.f * Scale, 600.f * Scale, SurfaceZ + 750.f * Scale), .01f));
		TestTrue(TEXT("BOB fill remains overhead and scales above the playing plane"),
			Rig.ArenaFillLight->GetActorLocation().Equals(FVector(0.f, 0.f, SurfaceZ + 850.f * Scale), .01f));
		TestEqual(TEXT("BOB key source width scales"), Key->SourceWidth, 900.f * Scale);
		TestEqual(TEXT("BOB key source height scales"), Key->SourceHeight, 700.f * Scale);
		TestEqual(TEXT("BOB rim source width scales"), Rim->SourceWidth, 650.f * Scale);
		TestEqual(TEXT("BOB rim source height scales"), Rim->SourceHeight, 500.f * Scale);
		TestEqual(TEXT("BOB key range remains bounded"), Key->AttenuationRadius, 2400.f * Scale);
		TestEqual(TEXT("BOB rim range remains bounded"), Rim->AttenuationRadius, 1900.f * Scale);
		TestEqual(TEXT("BOB fill range scales"), Fill->AttenuationRadius, 1800.f * Scale);
		TestEqual(TEXT("BOB fill reflection source scales"), Fill->SourceRadius, 400.f * Scale);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const APointLight* Actor = Index == 0 ? Rig.Player1AccentLight.Get() : Rig.Player2AccentLight.Get();
			const UPointLightComponent* Light = Actor->PointLightComponent;
			TestTrue(TEXT("BOB rail spill follows its actual side fixture below the playing plane"),
				Actor->GetActorLocation().Equals(FVector((Index == 0 ? 730.f : -730.f) * Scale, 0.f, SurfaceZ - 14.f * Scale), .01f));
			TestTrue(TEXT("BOB capsule runs along the side rail, not vertically"),
				FMath::Abs(FVector::DotProduct(Actor->GetActorUpVector(), FVector::YAxisVector)) > .999f);
			TestTrue(TEXT("BOB rail power uses explicit physical units"), Light->IntensityUnits == ELightUnits::Lumens);
			TestEqual(TEXT("BOB rail source length scales"), Light->SourceLength, 760.f * Scale);
			TestEqual(TEXT("BOB rail source radius scales"), Light->SourceRadius, 6.f * Scale);
			TestEqual(TEXT("BOB rail softness scales"), Light->SoftSourceRadius, 30.f * Scale);
			TestEqual(TEXT("BOB rail spill has a short, scaled range"), Light->AttenuationRadius, 500.f * Scale);
			TestFalse(TEXT("BOB rail lights add no dynamic shadow passes"), Light->CastShadows);
			TestEqual(TEXT("BOB rail spill has no unstable emissive/indirect proxy"), Light->IndirectLightingIntensity, 0.f);
			TestEqual(TEXT("BOB rail lights add no volumetric passes"), Light->VolumetricScatteringIntensity, 0.f);
			TestFalse(TEXT("BOB rail lights do not replicate cosmetic state"), Actor->GetIsReplicated());
		}
	};
	const auto CheckPower = [&](float Scale, float Ambient, float FillPreference, float KeyPreference,
		float RimPreference, float AccentPreference, float Direct, float Highlights)
	{
		const float Area = FMath::Square(Scale);
		TestTrue(TEXT("BOB key power follows preferences and source-board area"),
			FMath::IsNearlyEqual(Key->Intensity, Bob.BobKeyLightLumens * Area * KeyPreference, .02f));
		TestTrue(TEXT("BOB rim power follows preferences and source-board area"),
			FMath::IsNearlyEqual(Rim->Intensity, Bob.BobRimLightLumens * Area * RimPreference, .02f));
		TestTrue(TEXT("BOB fill follows preferences and source-board area"),
			FMath::IsNearlyEqual(Fill->Intensity, Bob.BobFillLightLumens * Area * FillPreference, .02f));
		TestTrue(TEXT("BOB amber side power follows the accent preference"),
			FMath::IsNearlyEqual(Warm->Intensity, Bob.BobWarmRailLightLumens * Area * AccentPreference, .02f));
		TestTrue(TEXT("BOB cyan side power follows the accent preference"),
			FMath::IsNearlyEqual(Cool->Intensity, Bob.BobCoolRailLightLumens * Area * AccentPreference, .02f));
		TestEqual(TEXT("BOB sky follows the ambient preference"), Rig.SkyLightActor->GetLightComponent()->Intensity, Bob.BobSkyIntensity * Ambient);
		TestEqual(TEXT("BOB overhead light follows the direct preference"), Rig.DirectionalLightActor->GetLightComponent()->Intensity, Bob.BobDirectionalIntensity * Direct);
		TestEqual(TEXT("BOB key reflections follow the saved reflection control"), Key->SpecularScale, Highlights);
		TestEqual(TEXT("BOB rim reflections follow the saved reflection control"), Rim->SpecularScale, Highlights);
		TestEqual(TEXT("BOB rail reflections stay soft rather than exposed bars"), Warm->SpecularScale, .35f * Highlights);
		TestEqual(TEXT("BOB cool rail uses the same reflection preference"), Cool->SpecularScale, .35f * Highlights);
		TestEqual(TEXT("BOB fill remains diffuse-only"), Fill->SpecularScale, 0.f);
	};

	CheckCounts(3, 2);
	CheckPose(1.f, 250.f);
	CheckPower(1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, .35f);
	TestTrue(TEXT("BOB key uses lumens"), Key->IntensityUnits == ELightUnits::Lumens);
	TestTrue(TEXT("BOB rim uses lumens"), Rim->IntensityUnits == ELightUnits::Lumens);
	TestTrue(TEXT("BOB fill uses lumens"), Fill->IntensityUnits == ELightUnits::Lumens);
	TestFalse(TEXT("BOB broad key adds no shadow array"), Key->CastShadows);
	TestFalse(TEXT("BOB broad rim adds no shadow array"), Rim->CastShadows);
	TestFalse(TEXT("BOB broad fill adds no shadow array"), Fill->CastShadows);
	TestTrue(TEXT("Amber fixture is not whitened into a neutral flood"), Warm->GetLightColor().R > Warm->GetLightColor().B * 2.f);
	TestTrue(TEXT("Cyan fixture is not whitened into a neutral flood"), Cool->GetLightColor().B > Cool->GetLightColor().R * 2.f);
	for (const auto& Actor : Rig.RimSpillLights)
		TestNull(TEXT("A fresh BOB scene never spawns a Knockout rim ring"), Actor.Get());
	for (const auto& Actor : Rig.StadiumWarmLights)
		TestNull(TEXT("A fresh BOB scene needs no extra stadium lights"), Actor.Get());

	// Exercise lookup as well as the host's cached rig. Both must reuse actors.
	Rig = FindRig(World);
	Configure(World, Rig, Bob);
	Configure(World, Rig, Bob);
	TestTrue(TEXT("BOB reapplication reuses the entire original fixture set"),
		Rig.TestPuckKeyLight == Original.TestPuckKeyLight && Rig.TestPuckRimLight == Original.TestPuckRimLight
		&& Rig.ArenaFillLight == Original.ArenaFillLight && Rig.Player1AccentLight == Original.Player1AccentLight
		&& Rig.Player2AccentLight == Original.Player2AccentLight && Rig.SkyLightActor == Original.SkyLightActor
		&& Rig.DirectionalLightActor == Original.DirectionalLightActor);
	CheckCounts(3, 2);
	SetValue(EScene::Gameplay, EControl::Ambient, 1.5f);
	SetValue(EScene::Gameplay, EControl::Fill, 1.8f);
	SetValue(EScene::Gameplay, EControl::Key, .6f);
	SetValue(EScene::Gameplay, EControl::Rim, .8f);
	SetValue(EScene::Gameplay, EControl::Accents, 1.2f);
	SetValue(EScene::Gameplay, EControl::Direct, .5f);
	SetValue(EScene::Gameplay, EControl::Highlights, .2f);
	for (const float Scale : {.85f, 1.f, 1.3f})
	{
		Bob.ArenaRadius = 620.f * Scale;
		Bob.ArenaSurfaceZ = 372.f;
		Configure(World, Rig, Bob);
		Configure(World, Rig, Bob);
		CheckPose(Scale, 372.f);
		CheckPower(Scale, 1.5f, 1.8f, .6f, .8f, 1.2f, .5f, .2f);
	}
	for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
		SetValue(EScene::Gameplay, static_cast<EControl>(Index), 0.f);
	Configure(World, Rig, Bob);
	CheckPower(1.3f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
	ResetToDefaults();

	// Reusing BOB's lumen-valued fixtures must not alter Knockout's unitless rig.
	FParameters Knockout;
	Configure(World, Rig, Knockout);
	TestTrue(TEXT("Returning to Knockout restores key intensity units"), Key->IntensityUnits == ELightUnits::Unitless);
	TestTrue(TEXT("Returning to Knockout restores rim intensity units"), Rim->IntensityUnits == ELightUnits::Unitless);
	TestTrue(TEXT("Returning to Knockout restores fill intensity units"), Fill->IntensityUnits == ELightUnits::Unitless);
	for (const UPointLightComponent* Accent : {Warm, Cool})
	{
		TestTrue(TEXT("Returning to Knockout restores accent intensity units"), Accent->IntensityUnits == ELightUnits::Unitless);
		TestEqual(TEXT("BOB rail length cannot leak into Knockout accents"), Accent->SourceLength, 0.f);
		TestEqual(TEXT("BOB rail softness cannot leak into Knockout accents"), Accent->SoftSourceRadius, 0.f);
	}
	TestEqual(TEXT("Knockout key retains its original intensity"), Key->Intensity,
		Knockout.TestPuckKeyLightIntensity * Knockout.OneVsOneGameplaySoftboxMultiplier);
	TestEqual(TEXT("Knockout fill retains its original intensity"), Fill->Intensity, Knockout.OneVsOneFillLightIntensity);
	CheckCounts(15, 6);
	Bob.ArenaRadius = 620.f;
	Bob.ArenaSurfaceZ = 250.f;
	Configure(World, Rig, Bob);
	CheckPose(1.f, 250.f);
	CheckPower(1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, .35f);
	for (const auto& Actor : Rig.RimSpillLights)
		if (TestNotNull(TEXT("Knockout rim lights remain reusable"), Actor.Get()))
			TestEqual(TEXT("Knockout rim spill never lights the BOB scene"), Actor->PointLightComponent->Intensity, 0.f);
	for (const auto& Actor : Rig.StadiumWarmLights)
		if (TestNotNull(TEXT("Knockout stadium lights remain reusable"), Actor.Get()))
			TestEqual(TEXT("Knockout stadium lights never leak into BOB"), Actor->RectLightComponent->Intensity, 0.f);
	CheckCounts(15, 6);

	AFlickCameraPawn* Camera = World->SpawnActor<AFlickCameraPawn>();
	if (TestNotNull(TEXT("BOB view camera"), Camera))
	{
		const UCameraComponent* View = Camera->FindComponentByClass<UCameraComponent>();
		if (TestNotNull(TEXT("BOB camera component"), View))
		{
			Camera->SetMenuPresentation(false);
			Camera->SetTestArenaPresentation(false);
			Camera->SetOneVsOneArenaPresentation(false);
			Camera->SetBobGameplayFraming(false);
			const FPostProcessSettings Ordinary = View->PostProcessSettings;
			Camera->SetTestArenaPresentation(true);
			Camera->SetTestArenaPresentation(false);
			Camera->SetBobGameplayFraming(true);
			const auto& Post = View->PostProcessSettings;
			TestEqual(TEXT("BOB bloom is restrained around the existing LEDs"), Post.BloomIntensity, .25f);
			TestEqual(TEXT("BOB bloom starts above the polished board's ordinary highlights"), Post.BloomThreshold, 1.15f);
			TestEqual(TEXT("BOB exposure preserves warm ivory and dark walnut contrast"), Post.AutoExposureBias, -.65f);
			TestEqual(TEXT("BOB does not add artificial grain to material detail"), Post.FilmGrainIntensity, 0.f);
			TestTrue(TEXT("BOB keeps a neutral gamma"), Post.ColorGamma == FVector4(1.f, 1.f, 1.f, 1.f));
			TestTrue(TEXT("BOB uses gentle contrast instead of crushing the walnut frame"), Post.ColorContrast == FVector4(1.1f, 1.1f, 1.1f, 1.f));
			TestEqual(TEXT("BOB occlusion remains restrained"), Post.AmbientOcclusionIntensity, 1.05f);
			TestFalse(TEXT("BOB never inherits a costly Lumen quality override"), Post.bOverride_LumenReflectionQuality);
			TestFalse(TEXT("BOB leaves screen-space reflection quality under player settings"), Post.bOverride_ScreenSpaceReflectionQuality);
			TestFalse(TEXT("BOB leaves GI trace quality under player settings"), Post.bOverride_LumenFinalGatherScreenTraces);
			TestFalse(TEXT("BOB leaves reflection trace quality under player settings"), Post.bOverride_LumenReflectionsScreenTraces);
			Camera->SetBobGameplayFraming(false);
			TestEqual(TEXT("Leaving BOB restores ordinary bloom"), Post.BloomIntensity, Ordinary.BloomIntensity);
			TestEqual(TEXT("Leaving BOB restores ordinary grain"), Post.FilmGrainIntensity, Ordinary.FilmGrainIntensity);
			TestTrue(TEXT("Leaving BOB restores ordinary gamma"), Post.ColorGamma == Ordinary.ColorGamma);
			TestTrue(TEXT("Leaving BOB restores ordinary contrast"), Post.ColorContrast == Ordinary.ColorContrast);
			TestEqual(TEXT("Leaving BOB restores ordinary occlusion"), Post.AmbientOcclusionIntensity, Ordinary.AmbientOcclusionIntensity);
		}
	}
	return true;
}
#endif
