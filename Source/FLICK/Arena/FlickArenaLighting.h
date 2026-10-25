#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"

class UWorld;
class ADirectionalLight;
class ASkyLight;
class APointLight;
class ARectLight;

// One reusable, non-replicated rig per world. The same code configures host and client visuals.
namespace FlickArenaLighting
{
	struct FRig
	{
		TObjectPtr<ADirectionalLight> DirectionalLightActor;
		TObjectPtr<ASkyLight> SkyLightActor;
		TObjectPtr<APointLight> Player1AccentLight, Player2AccentLight, ArenaFillLight;
		TObjectPtr<ARectLight> TestPuckKeyLight, TestPuckRimLight;
		TObjectPtr<ARectLight> PuckFinishLight;
		TObjectPtr<ARectLight> PuckFillLight, DeckReflectionLight;
		TStaticArray<TObjectPtr<ARectLight>, 4> StadiumWarmLights{};
		TStaticArray<TObjectPtr<APointLight>, 12> RimSpillLights{};
	};
	struct FParameters
	{
		bool bTestArenaMode = true;
		bool bClassicArenaLighting = true;
		bool bBobArenaLighting = false;
		bool bFrontendShowcase = false;
		bool bPremiumMenu = false;
		// All Switchyard formats share the premium finish and local preferences.
		bool bPremiumArena = true;
		float ArenaRadius = 650.0f;
		float ArenaSurfaceZ = 250.0f;
		float TestPuckKeyLightIntensity = 190.0f;
		float TestPuckRimLightIntensity = 150.0f;
		// Fixed product-lighting rig on channel 1, independent of arena sliders.
		float PuckFinishLightLumens = 1500.0f;
		float PuckFillLightLumens = 750.0f;
		FVector PuckKeyOffset = FVector(-450.f, 900.f, 1050.f);
		FVector PuckFillOffset = FVector(600.f, -350.f, 1150.f);
		FVector DeckKeyOffset = FVector(-520.f, 1100.f, 1150.f);
		FVector DeckSecondKeyOffset = FVector(520.f, 1100.f, 1150.f);
		FVector DeckWarmOffset = FVector(900.f, 1250.f, 950.f);
		float MenuAccentLightIntensity = 1800.0f;
		float MenuSoftboxLightMultiplier = 80.0f;
		float OneVsOneMenuSoftboxMultiplier = 250.0f;
		float OneVsOneGameplaySoftboxMultiplier = 250.0f;
		// Long strip sources reflect as stadium fixtures, not a white ceiling sheet.
		float SwitchyardKeySourceWidth = 600.0f;
		float SwitchyardKeySourceHeight = 360.0f;
		float SwitchyardRimSourceWidth = 600.0f;
		float SwitchyardRimSourceHeight = 150.0f;
		FLinearColor SwitchyardKeyColor = FLinearColor(.10f, .62f, 1.f);
		FLinearColor SwitchyardWarmColor = FLinearColor(1.f, .48f, .17f);
		// Readability comes from broad neutral coverage, not hotter reflections.
		float OneVsOneSkyLightMultiplier = .75f;
		float OneVsOneFillLightIntensity = 195000.0f;
		// Four local, shadow-free sources aligned with the authored seating banks.
		float StadiumWarmLightLumens = 200.0f;
		float StadiumWarmSourceWidth = 180.0f;
		float StadiumWarmSourceHeight = 75.0f;
		float StadiumWarmSpecularScale = .7f;
		// Short-range capsule sources represent the segmented amber skirt LEDs.
		// No shadow maps, emissive GI noise or full-stadium light overlap.
		float RimSpillLightLumens = 28.0f;
		// BOB's ivory/walnut presentation reuses the existing rig, with explicit
		// local-light units and source-board scaling (620 cm half extent).
		float BobKeyLightLumens = 650.f;
		float BobRimLightLumens = 250.f;
		float BobFillLightLumens = 650.f;
		float BobWarmRailLightLumens = 120.f;
		float BobCoolRailLightLumens = 80.f;
		float BobSkyIntensity = .28f;
		float BobDirectionalIntensity = .75f;
		float FrontendArenaDirectionalLightMultiplier = 1.22f;
		float FrontendArenaSkyLightMultiplier = 1.28f;
		float FrontendArenaFillLightMultiplier = 1.18f;
	};
	void Configure(UWorld* World, FRig& Rig, const FParameters& Parameters);
	FRig FindRig(UWorld* World);
	void UpdateMenuSoftboxes(ARectLight* Key, ARectLight* Rim, const FVector& CameraLocation, float ArenaRadius, float ArenaSurfaceZ, ARectLight* SecondKey = nullptr);
	void UpdatePuckStudioLights(ARectLight* Key, ARectLight* Fill, const FVector& CameraLocation, float ArenaScale, float ArenaSurfaceZ);
}
