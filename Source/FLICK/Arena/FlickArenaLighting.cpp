#include "Arena/FlickArenaLighting.h"
#include "Core/FlickLightingSettings.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace FlickArenaLighting
{
	// This off-axis angle reproduces the calmer highlights seen after the old
	// startup orbit. Tracking the view prevents the softboxes crossing its mirror axis.
	void UpdateMenuSoftboxes(ARectLight* Key, ARectLight* Rim, const FVector& CameraLocation, float ArenaRadius, float ArenaSurfaceZ)
	{
		if (FVector2D(CameraLocation.X, CameraLocation.Y).IsNearlyZero()) return;
		const float CameraAzimuth = FMath::RadiansToDegrees(FMath::Atan2(CameraLocation.Y, CameraLocation.X));
		const FRotator Rotation(0.0f, CameraAzimuth + 56.0f, 0.0f);
		const float Scale = ArenaRadius / 650.0f;
		const auto Move = [&](ARectLight* Light, const FVector& Offset)
		{
			if (!IsValid(Light)) return;
			FVector Location = Rotation.RotateVector(Offset * Scale);
			Location.Z = ArenaSurfaceZ + (Offset.Z - 250.0f) * Scale;
			Light->SetActorLocation(Location);
			Light->SetActorRotation((FVector(0.0f, 0.0f, ArenaSurfaceZ) - Location).Rotation());
		};
		Move(Key, FVector(-100.0f, 900.0f, 950.0f));
		Move(Rim, FVector(350.0f, 800.0f, 800.0f));
	}

	// Tags identify only this rig; repeated applications never stack extra fixtures.
	FRig FindRig(UWorld* World)
	{
		FRig Rig;
		if (!World) return Rig;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.DirectionalLightActor"))) { Rig.DirectionalLightActor = *It; break; }
		for (TActorIterator<ASkyLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.SkyLightActor"))) { Rig.SkyLightActor = *It; break; }
		for (TActorIterator<APointLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.Player1AccentLight"))) { Rig.Player1AccentLight = *It; break; }
		for (TActorIterator<APointLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.Player2AccentLight"))) { Rig.Player2AccentLight = *It; break; }
		for (TActorIterator<APointLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.ArenaFillLight"))) { Rig.ArenaFillLight = *It; break; }
		for (TActorIterator<ARectLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.TestPuckKeyLight"))) { Rig.TestPuckKeyLight = *It; break; }
		for (TActorIterator<ARectLight> It(World); It; ++It)
			if (It->ActorHasTag(TEXT("FLICK.TestPuckRimLight"))) { Rig.TestPuckRimLight = *It; break; }
		return Rig;
	}

	void Configure(UWorld* World, FRig& Rig, const FParameters& Parameters)
	{
		if (!World || World->GetNetMode() == NM_DedicatedServer) return;
		const auto bTestArenaMode = Parameters.bTestArenaMode;
		const auto bClassicArenaLighting = Parameters.bClassicArenaLighting;
		const auto bBobArenaLighting = Parameters.bBobArenaLighting;
		const auto bFrontendShowcase = Parameters.bFrontendShowcase;
		const auto bPremiumMenu = Parameters.bPremiumMenu;
		const auto bPremiumArena = Parameters.bPremiumArena;
		const auto ArenaRadius = Parameters.ArenaRadius;
		const auto ArenaSurfaceZ = Parameters.ArenaSurfaceZ;
		const auto TestPuckKeyLightIntensity = Parameters.TestPuckKeyLightIntensity;
		const auto TestPuckRimLightIntensity = Parameters.TestPuckRimLightIntensity;
		const auto MenuAccentLightIntensity = Parameters.MenuAccentLightIntensity;
		const auto MenuSoftboxLightMultiplier = Parameters.MenuSoftboxLightMultiplier;
		const auto OneVsOneMenuSoftboxMultiplier = Parameters.OneVsOneMenuSoftboxMultiplier;
		const auto OneVsOneGameplaySoftboxMultiplier = Parameters.OneVsOneGameplaySoftboxMultiplier;
		const auto OneVsOneSkyLightMultiplier = Parameters.OneVsOneSkyLightMultiplier;
		const auto OneVsOneFillLightIntensity = Parameters.OneVsOneFillLightIntensity;
		const auto FrontendArenaDirectionalLightMultiplier = Parameters.FrontendArenaDirectionalLightMultiplier;
		const auto FrontendArenaSkyLightMultiplier = Parameters.FrontendArenaSkyLightMultiplier;
		const auto FrontendArenaFillLightMultiplier = Parameters.FrontendArenaFillLightMultiplier;
		auto& DirectionalLightActor = Rig.DirectionalLightActor;
		auto& SkyLightActor = Rig.SkyLightActor;
		auto& Player1AccentLight = Rig.Player1AccentLight;
		auto& Player2AccentLight = Rig.Player2AccentLight;
		auto& ArenaFillLight = Rig.ArenaFillLight;
		auto& TestPuckKeyLight = Rig.TestPuckKeyLight;
		auto& TestPuckRimLight = Rig.TestPuckRimLight;
		using namespace FlickLightingSettings;
		const EScene Scene = bPremiumMenu ? EScene::Menu : EScene::Gameplay;
		const auto Value = [bPremiumArena, bBobArenaLighting, Scene](EControl Control) { return bPremiumArena || bBobArenaLighting ? GetValue(Scene, Control) : 1.0f; };
		// Share the menu's base rig in Switchyard matches; preferences still use their own scene.
		const bool bShowcaseLighting = bPremiumMenu || bPremiumArena;
		const float Ambient = Value(EControl::Ambient), Fill = Value(EControl::Fill), Key = Value(EControl::Key);
		const float Rim = Value(EControl::Rim), Accents = Value(EControl::Accents), Direct = Value(EControl::Direct);
		const float Highlights = Value(EControl::Highlights);
		const float ArenaScale = ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius;
		// Fixture dimensions/distances scale with the board. Scale local light
		// power by area to preserve illumination instead of darkening large arenas.
		const float LocalPowerScale = bPremiumArena ? FMath::Square(ArenaScale) : 1.0f;
		const auto ScaleLocation = [=](const FVector& Location)
		{
			FVector Scaled = Location * ArenaScale;
			if (bPremiumArena || bBobArenaLighting) Scaled.Z = ArenaSurfaceZ + (Location.Z - 250.0f) * ArenaScale;
			return Scaled;
		};
		const float DirectionalMultiplier = bShowcaseLighting ? (bPremiumArena ? 0.68f : 0.90f) : bFrontendShowcase
			? FrontendArenaDirectionalLightMultiplier : 1.0f;
		// The satin-metal deck needs reflected light across its whole hemisphere,
		// not just a bright patch from one softbox. Shared by all Knockout formats.
		const float SkyMultiplier = bPremiumArena ? OneVsOneSkyLightMultiplier
			: bShowcaseLighting ? 0.78f : bFrontendShowcase ? FrontendArenaSkyLightMultiplier : 1.0f;
		const float FillMultiplier = bShowcaseLighting ? 0.58f : bFrontendShowcase ? FrontendArenaFillLightMultiplier : 1.0f;

		if (!DirectionalLightActor || !IsValid(DirectionalLightActor))
		{
			for (TActorIterator<ADirectionalLight> It(World); It; ++It)
			{
				DirectionalLightActor = *It;
				break;
			}
			if (!DirectionalLightActor)
			{
				DirectionalLightActor = World->SpawnActor<ADirectionalLight>(
					ADirectionalLight::StaticClass(),
					FVector(-300.0f, -500.0f, 900.0f),
					FRotator(-55.0f, -30.0f, 0.0f));
			}
		}

		if (!SkyLightActor || !IsValid(SkyLightActor))
		{
			for (TActorIterator<ASkyLight> It(World); It; ++It)
			{
				SkyLightActor = *It;
				break;
			}
			if (!SkyLightActor)
			{
				SkyLightActor = World->SpawnActor<ASkyLight>(
					ASkyLight::StaticClass(),
					FVector(0.0f, 0.0f, 900.0f),
					FRotator::ZeroRotator);
			}
		}

		if (UDirectionalLightComponent* Light = DirectionalLightActor
			? Cast<UDirectionalLightComponent>(DirectionalLightActor->GetLightComponent())
			: nullptr)
		{
			Light->SetMobility(EComponentMobility::Movable);
			DirectionalLightActor->SetActorRotation(FRotator(-72.0f, -25.0f, 0.0f));
			Light->SetLightColor(bTestArenaMode
				? FLinearColor(0.92f, 0.95f, 1.0f)
				: bClassicArenaLighting
					? FLinearColor(0.82f, 0.88f, 0.96f)
					: FLinearColor(0.9f, 0.94f, 1.0f));
			Light->SetIntensity(
				(bTestArenaMode ? 1.45f : bClassicArenaLighting ? 0.78f : bBobArenaLighting ? 1.32f : 1.15f)
				* DirectionalMultiplier * Direct);
			Light->SetLightSourceAngle(bTestArenaMode ? 5.0f : 3.0f);
			Light->SetSpecularScale(bShowcaseLighting ? Highlights : bTestArenaMode ? 0.60f * Highlights : bClassicArenaLighting ? 0.14f : 0.32f * Highlights);
			Light->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.72f : 0.8f);
			Light->SetCastShadows(true);
		}
		if (SkyLightActor && SkyLightActor->GetLightComponent())
		{
			auto* Sky = SkyLightActor->GetLightComponent();
			Sky->SetMobility(EComponentMobility::Movable);
			// Metallic workshop surfaces need an environment to reflect even on an empty map.
			if (bTestArenaMode || bShowcaseLighting)
			{
				if (auto* Environment = LoadObject<UTextureCube>(nullptr,
					TEXT("/Game/TestArena/Pucks/T_PuckEnvironment.T_PuckEnvironment")))
				{
					Sky->SourceType = SLS_SpecifiedCubemap;
					Sky->SetCubemap(Environment);
				}
			}
			else if (Sky->SourceType == SLS_SpecifiedCubemap && Sky->Cubemap
				&& Sky->Cubemap->GetPathName().StartsWith(TEXT("/Game/TestArena/Pucks/")))
			{
				Sky->SourceType = SLS_CapturedScene;
				Sky->SetCubemap(nullptr);
			}
			Sky->SetIntensity(
				(bTestArenaMode ? 1.05f : bClassicArenaLighting ? 0.34f : bBobArenaLighting ? 0.55f : 0.28f)
				* SkyMultiplier * Ambient);
		}

		const auto SpawnAccentLight = [&, bClassicArenaLighting, bBobArenaLighting, bFrontendShowcase, bShowcaseLighting](
			TObjectPtr<APointLight>& LightActor,
			const FVector& Location,
			const FLinearColor& Color)
		{
			if (!LightActor || !IsValid(LightActor))
			{
				LightActor = World->SpawnActor<APointLight>(
					APointLight::StaticClass(), Location, FRotator::ZeroRotator);
			}
			if (LightActor && LightActor->PointLightComponent)
			{
				UPointLightComponent* Light = LightActor->PointLightComponent;
				Light->SetMobility(EComponentMobility::Movable);
				LightActor->SetActorLocation(Location);
				// Preserve the neutral gameplay wash, but let the main-menu orbit show
				// the arena's team colors and material highlights without a screen tint.
				Light->SetLightColor(FMath::Lerp(
					Color, FLinearColor::White,
					bShowcaseLighting ? 0.06f : bTestArenaMode ? 0.38f : bClassicArenaLighting ? (bFrontendShowcase ? 0.52f : 0.88f) : 0.72f));
				Light->SetIntensity((bShowcaseLighting ? MenuAccentLightIntensity * Accents : bTestArenaMode ? 190.0f * Accents : bClassicArenaLighting ? (bFrontendShowcase ? 90.0f : 52.0f) : bBobArenaLighting ? 205.0f * Accents : 165.0f) * LocalPowerScale);
				Light->SetAttenuationRadius(
					(bTestArenaMode ? 700.0f : bClassicArenaLighting ? 720.0f : bBobArenaLighting ? 1500.0f : 820.0f)
						* ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
				Light->SetSourceRadius((bTestArenaMode ? 100.0f : bClassicArenaLighting ? 260.0f : 120.0f)
					* ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
				Light->SetSpecularScale(bShowcaseLighting ? Highlights : bTestArenaMode ? 0.52f * Highlights : bClassicArenaLighting ? (bFrontendShowcase ? 0.24f : 0.04f) : 0.48f * Highlights);
				Light->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.15f : 0.42f);
				Light->SetCastShadows(false);
			}
		};

		SpawnAccentLight(Player1AccentLight, ScaleLocation(FVector(0.0f, -690.0f, 620.0f)), GetTeamColor(EFlickTeam::Player1));
		SpawnAccentLight(Player2AccentLight, ScaleLocation(FVector(0.0f, 690.0f, 620.0f)), GetTeamColor(EFlickTeam::Player2));
		if (!ArenaFillLight || !IsValid(ArenaFillLight))
		{
			ArenaFillLight = World->SpawnActor<APointLight>(
				APointLight::StaticClass(),
				FVector(0.0f, 0.0f, 920.0f),
				FRotator::ZeroRotator);
		}
		if (ArenaFillLight && ArenaFillLight->PointLightComponent)
		{
			ArenaFillLight->PointLightComponent->SetMobility(EComponentMobility::Movable);
			ArenaFillLight->SetActorLocation(ScaleLocation(FVector(0.0f, 0.0f, bPremiumArena ? 1150.0f : 920.0f)));
			ArenaFillLight->PointLightComponent->SetLightColor(bPremiumArena ? FLinearColor(0.91f, 0.96f, 1.0f) : bClassicArenaLighting
				? FLinearColor(0.62f, 0.7f, 0.82f)
				: FLinearColor(0.72f, 0.78f, 0.88f));
			ArenaFillLight->PointLightComponent->SetIntensity(
				bPremiumArena ? OneVsOneFillLightIntensity * Fill * LocalPowerScale
				: (bTestArenaMode ? 440.0f : bClassicArenaLighting ? 112.0f : bBobArenaLighting ? 260.0f * Fill : 190.0f) * FillMultiplier);
			ArenaFillLight->PointLightComponent->SetAttenuationRadius(
				(bPremiumArena ? 2200.0f : bBobArenaLighting ? 2400.0f : 1280.0f) * ArenaScale);
			ArenaFillLight->PointLightComponent->SetSourceRadius((bPremiumArena ? 480.0f : bTestArenaMode ? 240.0f : 180.0f) * ArenaScale);
			// This is the broad neutral wash, not another mirrored highlight. Leave
			// the softboxes responsible for the satin-metal reflections.
			ArenaFillLight->PointLightComponent->SetSpecularScale(bPremiumArena || bBobArenaLighting ? 0.0f : bTestArenaMode ? 0.32f : bClassicArenaLighting ? 0.06f : 0.26f);
			ArenaFillLight->PointLightComponent->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.45f : 0.34f);
			ArenaFillLight->PointLightComponent->SetCastShadows(false);
		}

		// Large neutral cards create narrow, moving highlight bands on the prototype's
		// machined rings and graphite bevels. They are specular-first fixtures rather
		// than another arena flood. BOB uses a gentler version for its timber board.
		const auto ConfigurePuckSoftbox = [&, ArenaScale, bShowcaseLighting, bPremiumArena](
			TObjectPtr<ARectLight>& LightActor,
			const FVector& Location,
			const FLinearColor& Color,
			const float Intensity,
			const float Width,
			const float Height,
			const float PowerScale)
		{
			if (!LightActor || !IsValid(LightActor))
			{
				LightActor = World->SpawnActor<ARectLight>(
					ARectLight::StaticClass(), Location, FRotator::ZeroRotator);
			}
			if (!LightActor || !LightActor->RectLightComponent)
			{
				return;
			}
			URectLightComponent* Light = LightActor->RectLightComponent;
			Light->SetMobility(EComponentMobility::Movable);
			const FVector ScaledLocation = ScaleLocation(Location);
			LightActor->SetActorLocation(ScaledLocation);
			LightActor->SetActorRotation(
				(FVector(0.0f, 0.0f, bShowcaseLighting || bPremiumArena || bBobArenaLighting ? ArenaSurfaceZ : 45.0f * ArenaScale) - ScaledLocation).Rotation());
			Light->SetLightColor(Color);
			const float Multiplier = bPremiumArena
				? (bPremiumMenu ? OneVsOneMenuSoftboxMultiplier : OneVsOneGameplaySoftboxMultiplier)
				: (bShowcaseLighting ? MenuSoftboxLightMultiplier : bBobArenaLighting ? Parameters.BobSoftboxLightMultiplier : 1.0f);
			Light->SetIntensity(bShowcaseLighting || bTestArenaMode || bBobArenaLighting ? Intensity * Multiplier * PowerScale * LocalPowerScale : 0.0f);
			Light->SetAttenuationRadius((bPremiumArena || bBobArenaLighting ? 1850.0f : 1250.0f) * ArenaScale);
			Light->SetSourceWidth(Width * ArenaScale);
			Light->SetSourceHeight(Height * ArenaScale);
			Light->SetSpecularScale(Highlights);
			Light->SetIndirectLightingIntensity(0.05f);
			Light->SetCastShadows(false);
		};
		// Wide luminous panels, not narrow highlight cards. The menu UI used to
		// hide their overlapping mirror patches; a full-board gameplay view exposes
		// them. Keep total power but distribute reflected radiance over a larger area.
		ConfigurePuckSoftbox(TestPuckKeyLight,
			bShowcaseLighting ? (bPremiumArena ? FVector(-100.0f, 900.0f, 950.0f) : FVector(-380.0f, 420.0f, 820.0f))
				: bPremiumArena || bBobArenaLighting ? FVector(-360.0f, -280.0f, 1050.0f) : FVector(-620.0f, -420.0f, 560.0f),
			bPremiumArena ? FLinearColor(0.92f, 0.97f, 1.0f) : bShowcaseLighting ? FLinearColor(0.48f, 0.82f, 1.0f) : FLinearColor(0.82f, 0.91f, 1.0f),
			TestPuckKeyLightIntensity, bPremiumArena ? Parameters.SwitchyardKeySourceWidth : bBobArenaLighting ? (bShowcaseLighting ? 220.0f : 600.0f) : 460.0f,
			bPremiumArena ? Parameters.SwitchyardKeySourceHeight : bBobArenaLighting ? 580.0f : 170.0f, Key);
		ConfigurePuckSoftbox(TestPuckRimLight,
			bShowcaseLighting ? (bPremiumArena ? FVector(350.0f, 800.0f, 800.0f) : FVector(0.0f, 600.0f, 660.0f))
				: bPremiumArena || bBobArenaLighting ? FVector(360.0f, 300.0f, 1050.0f) : FVector(600.0f, 300.0f, 450.0f),
			bPremiumArena ? (bShowcaseLighting ? FLinearColor(1.0f, 0.66f, 0.35f) : FLinearColor(1.0f, 0.86f, 0.69f))
				: bShowcaseLighting ? FLinearColor(1.0f, 0.53f, 0.22f) : FLinearColor(1.0f, 0.86f, 0.72f),
			TestPuckRimLightIntensity, bPremiumArena ? Parameters.SwitchyardRimSourceWidth : bBobArenaLighting && !bShowcaseLighting ? 520.0f : 360.0f,
			bPremiumArena ? Parameters.SwitchyardRimSourceHeight : bBobArenaLighting && !bShowcaseLighting ? 420.0f : 130.0f, Rim);
		if (bPremiumArena && bShowcaseLighting)
			UpdateMenuSoftboxes(TestPuckKeyLight, TestPuckRimLight, FVector(170.0f, -1600.0f, 1140.0f), ArenaRadius, ArenaSurfaceZ);
		if (DirectionalLightActor) DirectionalLightActor->Tags.AddUnique(TEXT("FLICK.DirectionalLightActor"));
		if (SkyLightActor) SkyLightActor->Tags.AddUnique(TEXT("FLICK.SkyLightActor"));
		if (Player1AccentLight) Player1AccentLight->Tags.AddUnique(TEXT("FLICK.Player1AccentLight"));
		if (Player2AccentLight) Player2AccentLight->Tags.AddUnique(TEXT("FLICK.Player2AccentLight"));
		if (ArenaFillLight) ArenaFillLight->Tags.AddUnique(TEXT("FLICK.ArenaFillLight"));
		if (TestPuckKeyLight) TestPuckKeyLight->Tags.AddUnique(TEXT("FLICK.TestPuckKeyLight"));
		if (TestPuckRimLight) TestPuckRimLight->Tags.AddUnique(TEXT("FLICK.TestPuckRimLight"));
	}

}
