#pragma once

#include "CoreMinimal.h"

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
		float OneVsOneGameplaySoftboxMultiplier = 50.0f;
		float OneVsOneSkyLightMultiplier = 2.25f;
		float OneVsOneFillLightIntensity = 150000.0f;
		// BOB retains its warmer board lighting, with adjustable broad softboxes.
		float BobSoftboxLightMultiplier = 12.0f;
		float FrontendArenaDirectionalLightMultiplier = 1.22f;
		float FrontendArenaSkyLightMultiplier = 1.28f;
		float FrontendArenaFillLightMultiplier = 1.18f;
	};
	void Configure(UWorld* World, FRig& Rig, const FParameters& Parameters);
	FRig FindRig(UWorld* World);
	void UpdateMenuSoftboxes(ARectLight* Key, ARectLight* Rim, const FVector& CameraLocation, float ArenaRadius, float ArenaSurfaceZ);
}
