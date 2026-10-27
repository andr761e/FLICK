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
	namespace
	{
		const FName PuckWideStudioTag(TEXT("FLICK.PuckWideStudio"));
		const FName PuckGameplayStudioTag(TEXT("FLICK.PuckGameplayStudio"));
		FName RimSpillTag(int32 Index)
		{
			static const auto Tags = []
			{
				TStaticArray<FName, 12> Result;
				for (int32 Slot = 0; Slot < Result.Num(); ++Slot)
					Result[Slot] = FName(*FString::Printf(TEXT("FLICK.RimSpill%d"), Slot));
				return Result;
			}();
			return Tags[Index];
		}
	}
	// Keep the broad key opposite the view so its highlight stays on the far
	// center of the deck instead of washing across the left half during orbit.
	void UpdateMenuSoftboxes(ARectLight* Key, ARectLight* Rim, const FVector& CameraLocation, float ArenaRadius, float ArenaSurfaceZ, ARectLight* SecondKey)
	{
		if (FVector2D(CameraLocation.X, CameraLocation.Y).IsNearlyZero()) return;
		const float CameraAzimuth = FMath::RadiansToDegrees(FMath::Atan2(CameraLocation.Y, CameraLocation.X));
		const FRotator Rotation(0.0f, CameraAzimuth + 90.0f, 0.0f);
		const float Scale = ArenaRadius / 650.0f;
		const auto Move = [&](ARectLight* Light, const FVector& Offset)
		{
			if (!IsValid(Light)) return;
			FVector Location = Rotation.RotateVector(Offset * Scale);
			Location.Z = ArenaSurfaceZ + (Offset.Z - 250.0f) * Scale;
			Light->SetActorLocation(Location);
			Light->SetActorRotation((FVector(0.0f, 0.0f, ArenaSurfaceZ) - Location).Rotation());
		};
		const FParameters Defaults;
		Move(Key, Defaults.DeckKeyOffset);
		Move(SecondKey, Defaults.DeckSecondKeyOffset);
		Move(Rim, Defaults.DeckWarmOffset);
	}

	FQuat MakePuckStudioFrame(const FRotator& View, const FRotator& ReferenceView,
		float ReferenceAzimuth, float HighlightYaw, bool bReflectElevation)
	{
		const FQuat AuthoredFrame = FRotator(0.f, View.Yaw - ReferenceView.Yaw
			+ ReferenceAzimuth + 90.f + HighlightYaw, 0.f).Quaternion();
		if (!bReflectElevation)
			return FRotator(FMath::Clamp(View.Pitch, -65.f, -10.f), View.Yaw, 0.f).Quaternion()
				* ReferenceView.Quaternion().Inverse()
				* FRotator(0.f, ReferenceAzimuth + 90.f + HighlightYaw, 0.f).Quaternion();
		// Aim the actual key source at the reflection of the viewing ray, rather
		// than rotating an arbitrary reference vector. This accounts for the
		// authored key's side offset as well as its height above the puck plane.
		const FVector ToEye = -FRotator(FMath::Clamp(View.Pitch, -85.f, -5.f), View.Yaw, 0.f).Vector();
		const FVector ReflectedEye(-ToEye.X, -ToEye.Y, ToEye.Z);
		const FVector KeyDirection = FRotator(0.f, HighlightYaw, 0.f).RotateVector(ReflectedEye);
		const FParameters Defaults;
		const FVector LocalKey(Defaults.PuckKeyOffset.X, Defaults.PuckKeyOffset.Y, Defaults.PuckKeyOffset.Z - 250.f);
		return FQuat::FindBetweenNormals(AuthoredFrame.RotateVector(LocalKey).GetSafeNormal(), KeyDirection) * AuthoredFrame;
	}

	void UpdatePuckStudioLights(ARectLight* Key, ARectLight* Fill, const FVector& CameraLocation, float ArenaScale, float ArenaSurfaceZ, const FQuat* ProductFrame, bool bGameplayReflection)
	{
		if (!ProductFrame && FVector2D(CameraLocation.X, CameraLocation.Y).IsNearlyZero()) return;
		const float Azimuth = FMath::RadiansToDegrees(FMath::Atan2(CameraLocation.Y, CameraLocation.X));
		const FRotator Frame(0.f, Azimuth + 90.f, 0.f);
		const FParameters Defaults;
		const float Coverage = ProductFrame ? Defaults.PuckGameplayCoverageScale : 1.f;
		const auto Move = [&](ARectLight* Actor, const FVector& Offset, bool bFill)
		{
			if (!IsValid(Actor)) return;
			if (ProductFrame) Actor->Tags.AddUnique(PuckWideStudioTag);
			else Actor->Tags.Remove(PuckWideStudioTag);
			if (bGameplayReflection) Actor->Tags.AddUnique(PuckGameplayStudioTag);
			else Actor->Tags.Remove(PuckGameplayStudioTag);
			const FVector Local(Offset.X, Offset.Y, Offset.Z - 250.f);
			const FVector Position = FVector(0, 0, ArenaSurfaceZ)
				+ (ProductFrame ? ProductFrame->RotateVector(Local) : Frame.RotateVector(Local)) * ArenaScale * Coverage;
			Actor->SetActorLocation(Position);
			Actor->SetActorRotation((FVector(0.f, 0.f, ArenaSurfaceZ) - Position).Rotation());
			if (auto* Light = Actor->RectLightComponent.Get())
			{
				// Scale power by area: same centre illumination, much less falloff
				// variation across the playing surface. Still just two shared lights.
				Light->SetIntensity((bFill ? Defaults.PuckFillLightLumens : Defaults.PuckFinishLightLumens)
					* FMath::Square(ArenaScale * Coverage));
				Light->SetSpecularScale(bFill ? .15f : bGameplayReflection ? Defaults.PuckGameplayKeySpecularScale : .55f);
				Light->SetSourceWidth((bFill ? 900.f : 1100.f) * ArenaScale * Coverage);
				Light->SetSourceHeight((bFill ? 600.f : 650.f) * ArenaScale * Coverage);
				Light->SetAttenuationRadius(2200.f * ArenaScale * Coverage);
			}
		};
		Move(Key, Defaults.PuckKeyOffset, false);
		Move(Fill, Defaults.PuckFillOffset, true);
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
		// One pass per light type, retaining the first match for each tag.
		for (TActorIterator<APointLight> It(World); It; ++It)
		{
			if (!Rig.Player1AccentLight && It->ActorHasTag(TEXT("FLICK.Player1AccentLight"))) Rig.Player1AccentLight = *It;
			if (!Rig.Player2AccentLight && It->ActorHasTag(TEXT("FLICK.Player2AccentLight"))) Rig.Player2AccentLight = *It;
			if (!Rig.ArenaFillLight && It->ActorHasTag(TEXT("FLICK.ArenaFillLight"))) Rig.ArenaFillLight = *It;
			for (int32 Index = 0; Index < Rig.RimSpillLights.Num(); ++Index)
				if (!Rig.RimSpillLights[Index] && It->ActorHasTag(RimSpillTag(Index))) Rig.RimSpillLights[Index] = *It;
		}
		for (TActorIterator<ARectLight> It(World); It; ++It)
		{
			for (int32 Index = 0; Index < Rig.StadiumWarmLights.Num(); ++Index)
				if (!Rig.StadiumWarmLights[Index] && It->ActorHasTag(FName(*FString::Printf(TEXT("FLICK.StadiumWarm%d"), Index))))
					Rig.StadiumWarmLights[Index] = *It;
			if (!Rig.TestPuckKeyLight && It->ActorHasTag(TEXT("FLICK.TestPuckKeyLight"))) Rig.TestPuckKeyLight = *It;
			if (!Rig.TestPuckRimLight && It->ActorHasTag(TEXT("FLICK.TestPuckRimLight"))) Rig.TestPuckRimLight = *It;
			if (!Rig.PuckFinishLight && It->ActorHasTag(TEXT("FLICK.PuckFinishLight"))) Rig.PuckFinishLight = *It;
			if (!Rig.PuckFillLight && It->ActorHasTag(TEXT("FLICK.PuckFillLight"))) Rig.PuckFillLight = *It;
			if (!Rig.DeckReflectionLight && It->ActorHasTag(TEXT("FLICK.DeckReflectionLight"))) Rig.DeckReflectionLight = *It;
		}
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
		const float Ambient = bPremiumArena || bBobArenaLighting ? AmbientIntensityScale(Value(EControl::Ambient)) : 1.f;
		const float Fill = Value(EControl::Fill), Key = Value(EControl::Key);
		const float Rim = Value(EControl::Rim), Accents = Value(EControl::Accents), Direct = Value(EControl::Direct);
		const float Highlights = Value(EControl::Highlights);
		const float ArenaScale = ArenaRadius / (bBobArenaLighting ? 620.f : FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
		// Fixture dimensions/distances scale with the board. Scale local light
		// power by area to preserve illumination instead of darkening large arenas.
		const float LocalPowerScale = bPremiumArena || bBobArenaLighting ? FMath::Square(ArenaScale) : 1.0f;
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
					: bBobArenaLighting ? FLinearColor(1.f, .95f, .86f) : FLinearColor(0.9f, 0.94f, 1.0f));
			Light->SetIntensity(
				(bTestArenaMode ? 1.45f : bClassicArenaLighting ? 0.78f : bBobArenaLighting ? Parameters.BobDirectionalIntensity : 1.15f)
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
			if (bTestArenaMode || bShowcaseLighting || bBobArenaLighting)
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
				(bTestArenaMode ? 1.05f : bClassicArenaLighting ? 0.34f : bBobArenaLighting ? Parameters.BobSkyIntensity : 0.28f)
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
				if (bBobArenaLighting)
				{
					// Use the two existing accents beside the actual service rails,
					// not as a whole-board colour flood. +X is screen-left/amber.
					const bool bWarm = LightActor == Player1AccentLight;
					LightActor->SetActorLocation(FVector((bWarm ? 730.f : -730.f) * ArenaScale, 0.f,
						ArenaSurfaceZ - 14.f * ArenaScale));
					LightActor->SetActorRotation(FRotationMatrix::MakeFromZ(FVector::YAxisVector).Rotator());
					Light->SetIntensityUnits(ELightUnits::Lumens);
					Light->SetIntensity((bWarm ? Parameters.BobWarmRailLightLumens : Parameters.BobCoolRailLightLumens)
						* Accents * LocalPowerScale);
					Light->SetLightColor(bWarm ? FLinearColor(1.f, .48f, .15f) : FLinearColor(.07f, .62f, 1.f));
					Light->SetSourceLength(760.f * ArenaScale);
					Light->SetSourceRadius(6.f * ArenaScale);
					Light->SetSoftSourceRadius(30.f * ArenaScale);
					Light->SetAttenuationRadius(500.f * ArenaScale);
					Light->SetSpecularScale(.35f * Highlights);
					Light->SetIndirectLightingIntensity(0.f);
					Light->SetVolumetricScatteringIntensity(0.f);
					Light->SetCastShadows(false);
					return;
				}
				// Reused actors must not retain BOB's lumen/capsule state in Knockout.
				Light->SetIntensityUnits(ELightUnits::Unitless);
				Light->SetSourceLength(0.f);
				Light->SetSoftSourceRadius(0.f);
				Light->SetVolumetricScatteringIntensity(1.f);
				LightActor->SetActorRotation(FRotator::ZeroRotator);
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
			ArenaFillLight->PointLightComponent->SetIntensityUnits(bBobArenaLighting ? ELightUnits::Lumens : ELightUnits::Unitless);
			if (bBobArenaLighting)
			{
				UPointLightComponent* Light = ArenaFillLight->PointLightComponent;
				ArenaFillLight->SetActorLocation(ScaleLocation(FVector(0.f, 0.f, 1100.f)));
				Light->SetLightColor(FLinearColor(1.f, .94f, .84f));
				Light->SetIntensity(Parameters.BobFillLightLumens * Fill * LocalPowerScale);
				Light->SetSourceRadius(400.f * ArenaScale);
				Light->SetAttenuationRadius(1800.f * ArenaScale);
			}
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
				: (bShowcaseLighting ? MenuSoftboxLightMultiplier : 1.0f);
			Light->SetIntensity(bShowcaseLighting || bTestArenaMode || bBobArenaLighting ? Intensity * Multiplier * PowerScale * LocalPowerScale : 0.0f);
			Light->SetAttenuationRadius((bPremiumArena ? 2500.0f : bBobArenaLighting ? 1850.0f : 1250.0f) * ArenaScale);
			Light->SetSourceWidth(Width * ArenaScale);
			Light->SetSourceHeight(Height * ArenaScale);
			Light->SetSpecularScale(Highlights);
			Light->SetIndirectLightingIntensity(0.05f);
			Light->SetCastShadows(false);
			Light->SetIntensityUnits(bBobArenaLighting ? ELightUnits::Lumens : ELightUnits::Unitless);
			if (bBobArenaLighting)
			{
				const bool bKey = LightActor == TestPuckKeyLight;
				// The key sits across the board from the default gameplay camera.
				// Its reflection is a broad satin streak, not an overhead white flood.
				const FVector Position = ScaleLocation(bKey ? FVector(-250.f, 1250.f, 1100.f) : FVector(500.f, 600.f, 1000.f));
				LightActor->SetActorLocation(Position);
				LightActor->SetActorRotation((FVector(0.f, 0.f, ArenaSurfaceZ) - Position).Rotation());
				Light->SetLightColor(bKey ? FLinearColor(1.f, .88f, .72f) : FLinearColor(.72f, .86f, 1.f));
				Light->SetIntensity((bKey ? Parameters.BobKeyLightLumens : Parameters.BobRimLightLumens) * PowerScale * LocalPowerScale);
				Light->SetSourceWidth((bKey ? 900.f : 650.f) * ArenaScale);
				Light->SetSourceHeight((bKey ? 700.f : 500.f) * ArenaScale);
				Light->SetAttenuationRadius((bKey ? 2400.f : 1900.f) * ArenaScale);
			}
		};
		// Separated cool/warm strip reflections reveal the deck's fine normal relief.
		// Reuse the same two shadow-free fixtures and the existing preference controls.
		ConfigurePuckSoftbox(TestPuckKeyLight,
			bShowcaseLighting ? (bPremiumArena ? Parameters.DeckKeyOffset : FVector(-380.0f, 420.0f, 820.0f))
				: bPremiumArena || bBobArenaLighting ? FVector(-360.0f, -280.0f, 1050.0f) : FVector(-620.0f, -420.0f, 560.0f),
			bPremiumArena ? Parameters.SwitchyardKeyColor : bShowcaseLighting ? FLinearColor(0.48f, 0.82f, 1.0f) : FLinearColor(0.82f, 0.91f, 1.0f),
			TestPuckKeyLightIntensity, bPremiumArena ? Parameters.SwitchyardKeySourceWidth : bBobArenaLighting ? (bShowcaseLighting ? 220.0f : 600.0f) : 460.0f,
			bPremiumArena ? Parameters.SwitchyardKeySourceHeight : bBobArenaLighting ? 580.0f : 170.0f, Key);
		ConfigurePuckSoftbox(TestPuckRimLight,
			bShowcaseLighting ? (bPremiumArena ? Parameters.DeckWarmOffset : FVector(0.0f, 600.0f, 660.0f))
				: bPremiumArena || bBobArenaLighting ? FVector(360.0f, 300.0f, 1050.0f) : FVector(600.0f, 300.0f, 450.0f),
			bPremiumArena ? Parameters.SwitchyardWarmColor
				: bShowcaseLighting ? FLinearColor(1.0f, 0.53f, 0.22f) : FLinearColor(1.0f, 0.86f, 0.72f),
			TestPuckRimLightIntensity, bPremiumArena ? Parameters.SwitchyardRimSourceWidth : bBobArenaLighting && !bShowcaseLighting ? 520.0f : 360.0f,
			bPremiumArena ? Parameters.SwitchyardRimSourceHeight : bBobArenaLighting && !bShowcaseLighting ? 420.0f : 130.0f, Rim);
		if (!IsValid(Rig.DeckReflectionLight)) Rig.DeckReflectionLight = FindRig(World).DeckReflectionLight;
		if (bPremiumArena)
		{
			ConfigurePuckSoftbox(Rig.DeckReflectionLight, Parameters.DeckSecondKeyOffset,
				Parameters.SwitchyardKeyColor, TestPuckKeyLightIntensity * .75f,
				Parameters.SwitchyardKeySourceWidth, Parameters.SwitchyardKeySourceHeight, Key);
			if (Rig.DeckReflectionLight) Rig.DeckReflectionLight->Tags.AddUnique(TEXT("FLICK.DeckReflectionLight"));
		}
		else if (IsValid(Rig.DeckReflectionLight)) Rig.DeckReflectionLight->RectLightComponent->SetIntensity(0.f);
		if (bPremiumArena && bShowcaseLighting)
			UpdateMenuSoftboxes(TestPuckKeyLight, TestPuckRimLight, FVector(170.0f, -1600.0f, 1140.0f), ArenaRadius, ArenaSurfaceZ, Rig.DeckReflectionLight);
		// Visible LED strips do not reliably illuminate nearby geometry through
		// Lumen alone (tiny emitters inside one large mesh/card). Match them with
		// a bounded, soft source ring below the playing plane, outside the skirt.
		// Both host and client paths recover the same tagged actors on reapply.
		for (TActorIterator<APointLight> It(World); It; ++It)
			for (int32 Index = 0; Index < Rig.RimSpillLights.Num(); ++Index)
				if (!IsValid(Rig.RimSpillLights[Index]) && It->ActorHasTag(RimSpillTag(Index))) Rig.RimSpillLights[Index] = *It;
		for (int32 Index = 0; Index < Rig.RimSpillLights.Num(); ++Index)
		{
			auto& Actor = Rig.RimSpillLights[Index];
			if (!IsValid(Actor) && bPremiumArena)
			{
				Actor = World->SpawnActor<APointLight>();
				if (Actor) Actor->Tags.AddUnique(RimSpillTag(Index));
			}
			if (!IsValid(Actor) || !Actor->PointLightComponent) continue;
			UPointLightComponent* Light = Actor->PointLightComponent;
			const float Angle = UE_TWO_PI * (Index + .5f) / Rig.RimSpillLights.Num();
			const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			const FVector Tangent(-Radial.Y, Radial.X, 0.f);
			Light->SetMobility(EComponentMobility::Movable);
			Actor->SetActorLocation(Radial * (ArenaRadius + 5.f * ArenaScale)
				+ FVector(0.f, 0.f, ArenaSurfaceZ - 18.f * ArenaScale));
			// UE's finite point-light source runs along local Z, not forward.
			Actor->SetActorRotation(FRotationMatrix::MakeFromZ(Tangent).Rotator());
			Light->SetIntensityUnits(ELightUnits::Lumens);
			Light->SetIntensity(bPremiumArena ? Parameters.RimSpillLightLumens * LocalPowerScale * Rim : 0.f);
			Light->SetLightColor(FLinearColor(1.f, .36f, .08f));
			Light->SetSourceRadius(15.f * ArenaScale);
			Light->SetSoftSourceRadius(32.f * ArenaScale);
			Light->SetSourceLength(140.f * ArenaScale);
			Light->SetAttenuationRadius(450.f * ArenaScale);
			// The strips are inset, not freestanding bright bars in the floor.
			// A broad source and restrained specular proxy produce their soft pool.
			Light->SetSpecularScale(Highlights * .12f);
			Light->SetCastShadows(false);
			Light->SetIndirectLightingIntensity(0.f);
			Light->SetVolumetricScatteringIntensity(0.f);
		}
		// The game mode retains the original rig members, whereas clients find
		// the whole rig by tags. Recover these shared fixtures on either path.
		for (TActorIterator<ARectLight> It(World); It; ++It)
			for (int32 Index = 0; Index < Rig.StadiumWarmLights.Num(); ++Index)
				if (!IsValid(Rig.StadiumWarmLights[Index]) && It->ActorHasTag(FName(*FString::Printf(TEXT("FLICK.StadiumWarm%d"), Index))))
					Rig.StadiumWarmLights[Index] = *It;
		for (int32 Index = 0; Index < Rig.StadiumWarmLights.Num(); ++Index)
		{
			auto& Actor = Rig.StadiumWarmLights[Index];
			if (!Actor && bPremiumArena)
			{
				Actor = World->SpawnActor<ARectLight>();
				if (Actor) Actor->Tags.AddUnique(FName(*FString::Printf(TEXT("FLICK.StadiumWarm%d"), Index)));
			}
			if (!IsValid(Actor) || !Actor->RectLightComponent) continue;
			URectLightComponent* Light = Actor->RectLightComponent;
			const float Angle = FMath::DegreesToRadians(44.f + (Index % 2) * 92.f + (Index / 2) * 180.f);
			const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
			// Above-deck fixtures can be reflected by the playing surface. The old
			// below-plane placement could light only the outside concourse.
			const FVector Position = Radial * (1220.f * ArenaScale) + FVector(0.f, 0.f, ArenaSurfaceZ + 450.f * ArenaScale);
			const FVector Target = Radial * (700.f * ArenaScale) + FVector(0.f, 0.f, ArenaSurfaceZ);
			Light->SetMobility(EComponentMobility::Movable);
			Actor->SetActorLocation(Position);
			Actor->SetActorRotation((Target - Position).Rotation());
			Light->SetIntensityUnits(ELightUnits::Lumens);
			Light->SetIntensity(bPremiumArena ? Parameters.StadiumWarmLightLumens * LocalPowerScale * Rim : 0.f);
			Light->SetLightColor(FLinearColor(1.f, .55f, .24f));
			Light->SetSourceWidth(Parameters.StadiumWarmSourceWidth * ArenaScale);
			Light->SetSourceHeight(Parameters.StadiumWarmSourceHeight * ArenaScale);
			Light->SetAttenuationRadius(1250.f * ArenaScale);
			// Preserve warm diffuse spill without a pale mirror-card at diagonal views.
			Light->SetSpecularScale(Highlights * Parameters.StadiumWarmSpecularScale);
			Light->SetIndirectLightingIntensity(.25f);
			Light->SetCastShadows(false);
		}
		// One shared studio pair, not lights per puck. No saved arena preference
		// changes their energy; channel 1 cannot illuminate the board or stadium.
		const FRig FoundStudio = FindRig(World);
		if (!IsValid(Rig.PuckFinishLight)) Rig.PuckFinishLight = FoundStudio.PuckFinishLight;
		if (!IsValid(Rig.PuckFillLight)) Rig.PuckFillLight = FoundStudio.PuckFillLight;
		const auto ConfigureStudio = [&](TObjectPtr<ARectLight>& Actor, bool bFill)
		{
			const bool bCreated = !IsValid(Actor);
			if (!IsValid(Actor)) Actor = World->SpawnActor<ARectLight>();
			if (!IsValid(Actor) || !Actor->RectLightComponent) return;
			URectLightComponent* Light = Actor->RectLightComponent;
			Light->SetMobility(EComponentMobility::Movable);
			const FVector Position = ScaleLocation(bFill ? Parameters.PuckFillOffset : Parameters.PuckKeyOffset);
			if (bCreated)
			{
				Actor->SetActorLocation(Position);
				Actor->SetActorRotation((FVector(0, 0, ArenaSurfaceZ) - Position).Rotation());
			}
			const float Coverage = Actor->ActorHasTag(PuckWideStudioTag) ? Parameters.PuckGameplayCoverageScale : 1.f;
			Light->SetLightingChannels(false, true, false);
			Light->SetLightColor(bFill ? FLinearColor(.82f, .9f, 1.f) : FLinearColor(1.f, .97f, .94f));
			Light->SetIntensityUnits(ELightUnits::Lumens);
			Light->SetIntensity((bFill ? Parameters.PuckFillLightLumens : Parameters.PuckFinishLightLumens) * LocalPowerScale * FMath::Square(Coverage));
			Light->SetDiffuseScale(1.f);
			// Broad silver bands without a white mirror plate over the crown.
			Light->SetSpecularScale(bFill ? .15f : Actor->ActorHasTag(PuckGameplayStudioTag) ? Parameters.PuckGameplayKeySpecularScale : .55f);
			Light->SetSourceWidth((bFill ? 900.f : 1100.f) * ArenaScale * Coverage);
			Light->SetSourceHeight((bFill ? 600.f : 650.f) * ArenaScale * Coverage);
			Light->SetAttenuationRadius(2200.f * ArenaScale * Coverage);
			Light->SetCastShadows(false);
			Light->SetIndirectLightingIntensity(0.f);
			Light->SetVolumetricScatteringIntensity(0.f);
			Actor->Tags.AddUnique(bFill ? TEXT("FLICK.PuckFillLight") : TEXT("FLICK.PuckFinishLight"));
		};
		ConfigureStudio(Rig.PuckFinishLight, false);
		ConfigureStudio(Rig.PuckFillLight, true);
		if (DirectionalLightActor) DirectionalLightActor->Tags.AddUnique(TEXT("FLICK.DirectionalLightActor"));
		if (SkyLightActor) SkyLightActor->Tags.AddUnique(TEXT("FLICK.SkyLightActor"));
		if (Player1AccentLight) Player1AccentLight->Tags.AddUnique(TEXT("FLICK.Player1AccentLight"));
		if (Player2AccentLight) Player2AccentLight->Tags.AddUnique(TEXT("FLICK.Player2AccentLight"));
		if (ArenaFillLight) ArenaFillLight->Tags.AddUnique(TEXT("FLICK.ArenaFillLight"));
		if (TestPuckKeyLight) TestPuckKeyLight->Tags.AddUnique(TEXT("FLICK.TestPuckKeyLight"));
		if (TestPuckRimLight) TestPuckRimLight->Tags.AddUnique(TEXT("FLICK.TestPuckRimLight"));
	}

}
