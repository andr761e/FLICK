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
		float TestPuckKeyLightIntensity = 260.0f;
		float TestPuckRimLightIntensity = 180.0f;
		float MenuAccentLightIntensity = 3200.0f;
		float MenuSoftboxLightMultiplier = 80.0f;
		float OneVsOneMenuSoftboxMultiplier = 250.0f;
		float OneVsOneGameplaySoftboxMultiplier = 250.0f;
		// Broad source areas avoid concentrated mirror patches on the metallic deck.
		float SwitchyardKeySourceWidth = 1000.0f;
		float SwitchyardKeySourceHeight = 550.0f;
		float SwitchyardRimSourceWidth = 650.0f;
		float SwitchyardRimSourceHeight = 500.0f;
		float OneVsOneSkyLightMultiplier = 1.0f;
		float OneVsOneFillLightIntensity = 150000.0f;
		// Four local, shadow-free sources aligned with the authored seating banks.
		float StadiumWarmLightLumens = 180.0f;
		// Short-range capsule sources represent the segmented amber skirt LEDs.
		// No shadow maps, emissive GI noise or full-stadium light overlap.
		float RimSpillLightLumens = 32.0f;
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
	void UpdateMenuSoftboxes(ARectLight* Key, ARectLight* Rim, const FVector& CameraLocation, float ArenaRadius, float ArenaSurfaceZ);
}
