#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickArenaLighting.h"
#include "Core/FlickLightingSettings.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickKnockoutLightingControlsTest, "FLICK.Visuals.KnockoutLightingControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickKnockoutLightingControlsTest::RunTest(const FString& Parameters)
{
	using namespace FlickLightingSettings;
	const FString Ini = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("KnockoutControls-") + FGuid::NewGuid().ToString() + TEXT(".ini"));
	const FString PreviousIni = GGameUserSettingsIni;
	GGameUserSettingsIni = Ini;
	GConfig->Add(Ini, FConfigFile());
	ON_SCOPE_EXIT { GGameUserSettingsIni = PreviousIni; GConfig->UnloadFile(Ini); IFileManager::Get().Delete(*Ini); };
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Lighting control world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FlickArenaLighting::FRig Rig;
	const FlickArenaLighting::FParameters Defaults;
	FlickArenaLighting::Configure(World, Rig, Defaults);
	if (!TestNotNull(TEXT("Puck studio key"), Rig.PuckFinishLight.Get())
		|| !TestNotNull(TEXT("Puck studio fill"), Rig.PuckFillLight.Get())
		|| !TestNotNull(TEXT("Second deck reflection"), Rig.DeckReflectionLight.Get())
		|| !TestNotNull(TEXT("Seating source"), Rig.StadiumWarmLights[0].Get())
		|| !TestNotNull(TEXT("Rim spill"), Rig.RimSpillLights[0].Get())) return false;
	const auto Read = [&Rig]()
	{
		return TArray<float>{Rig.SkyLightActor->GetLightComponent()->Intensity,
			Rig.ArenaFillLight->PointLightComponent->Intensity,
			Rig.TestPuckKeyLight->RectLightComponent->Intensity,
			Rig.TestPuckRimLight->RectLightComponent->Intensity,
			Rig.Player1AccentLight->PointLightComponent->Intensity,
			Rig.Player2AccentLight->PointLightComponent->Intensity,
			Rig.DirectionalLightActor->GetLightComponent()->Intensity,
			Rig.TestPuckKeyLight->RectLightComponent->SpecularScale,
			Rig.StadiumWarmLights[0]->RectLightComponent->Intensity,
			Rig.RimSpillLights[0]->PointLightComponent->Intensity,
			Rig.PuckFinishLight->RectLightComponent->Intensity,
			Rig.StadiumWarmLights[0]->RectLightComponent->SpecularScale,
			Rig.PuckFillLight->RectLightComponent->Intensity,
			Rig.DeckReflectionLight->RectLightComponent->Intensity};
	};
	const auto Initial = Read();
	const EControl Owners[] = {EControl::Ambient, EControl::Fill, EControl::Key, EControl::Rim,
		EControl::Accents, EControl::Accents, EControl::Direct, EControl::Highlights,
		EControl::Rim, EControl::Rim, EControl::Count, EControl::Highlights, EControl::Count, EControl::Key};
	const auto Count = [World]() { int32 Total = 0; for (TActorIterator<AActor> It(World); It; ++It) ++Total; return Total; };
	const int32 InitialActors = Count();
	for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
	{
		const auto Control = static_cast<EControl>(Index);
		for (const float Multiplier : {0.f, 2.f})
		{
			SetValue(EScene::Gameplay, Control, GetControl(Control).GameplayDefault * Multiplier);
			FlickArenaLighting::Configure(World, Rig, Defaults);
			FlickArenaLighting::Configure(World, Rig, Defaults);
			const auto Actual = Read();
			for (int32 Sample = 0; Sample < Actual.Num(); ++Sample)
			{
				const float Response = Control == EControl::Ambient
					? AmbientIntensityScale(GetControl(Control).GameplayDefault * Multiplier) / AmbientIntensityScale(GetControl(Control).GameplayDefault)
					: Multiplier;
				const float Expected = Initial[Sample] * (Owners[Sample] == Control ? Response : 1.f);
				TestTrue(*FString::Printf(TEXT("%s x%.0f changes only its own source group (sample %d)"), GetControl(Control).Key, Multiplier, Sample),
					FMath::IsNearlyEqual(Actual[Sample], Expected, .01f));
			}
			TestEqual(TEXT("Repeated slider changes never add lights"), Count(), InitialActors);
		}
		SetValue(EScene::Gameplay, Control, GetControl(Control).GameplayDefault);
	}
	TestEqual(TEXT("Neutral fill remains reflection-free"), Rig.ArenaFillLight->PointLightComponent->SpecularScale, 0.f);
	TestFalse(TEXT("Neutral fill still adds no dynamic shadows"), Rig.ArenaFillLight->PointLightComponent->CastShadows);
	for (const auto* Actor : {Rig.PuckFinishLight.Get(), Rig.PuckFillLight.Get()})
	{
		const auto* Light = Actor->RectLightComponent.Get();
		TestTrue(TEXT("Studio lights affect only puck channel 1"),
			!Light->LightingChannels.bChannel0 && Light->LightingChannels.bChannel1 && !Light->LightingChannels.bChannel2);
		TestFalse(TEXT("Studio lights add no shadow passes"), Light->CastShadows);
		TestEqual(TEXT("Studio lights cannot bounce into the arena"), Light->IndirectLightingIntensity, 0.f);
		TestEqual(TEXT("Studio lights cannot illuminate arena fog"), Light->VolumetricScatteringIntensity, 0.f);
	}
	const auto KeyPosition = Rig.PuckFinishLight->GetActorLocation();
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(0, -1600, 1200), 1.f, 250.f);
	const auto InitialKey = Rig.PuckFinishLight->GetActorLocation();
	const auto InitialFill = Rig.PuckFillLight->GetActorLocation();
	TestTrue(TEXT("Default camera uses the authored studio key"), InitialKey.Equals(KeyPosition, .01f));
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(1600, 0, 1200), 1.f, 250.f);
	const FRotator QuarterTurn(0, 90, 0);
	TestTrue(TEXT("Studio key follows camera azimuth without changing its height or distance"),
		Rig.PuckFinishLight->GetActorLocation().Equals(QuarterTurn.RotateVector(InitialKey), .01f));
	TestTrue(TEXT("Studio fill follows the same camera-relative orientation"),
		Rig.PuckFillLight->GetActorLocation().Equals(QuarterTurn.RotateVector(InitialFill), .01f));
	const FVector Focus(0, 0, 250.f);
	const FQuat ProductFrame = FQuat::Identity;
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(0, -1600, 1200), 1.f, 250.f, &ProductFrame);
	const float Coverage = Defaults.PuckGameplayCoverageScale;
	TestTrue(TEXT("Wide studio preserves light direction while increasing coverage"),
		(Rig.PuckFinishLight->GetActorLocation() - Focus).Equals((KeyPosition - Focus) * Coverage, .01f));
	TestEqual(TEXT("Wide studio preserves centre illumination by scaling power with area"),
		Rig.PuckFinishLight->RectLightComponent->Intensity, Defaults.PuckFinishLightLumens * Coverage * Coverage);
	const FQuat ElevatedFrame(FVector::ForwardVector, FMath::DegreesToRadians(20.f));
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(0, -1600, 1200), 1.f, 250.f, &ElevatedFrame);
	TestTrue(TEXT("Product lighting responds to camera elevation too"),
		(Rig.PuckFinishLight->GetActorLocation() - Focus).Equals(ElevatedFrame.RotateVector((KeyPosition - Focus) * Coverage), .01f));
	const FVector ElevatedPosition = Rig.PuckFinishLight->GetActorLocation();
	FlickArenaLighting::Configure(World, Rig, Defaults);
	TestTrue(TEXT("Reapplying arena settings never snaps puck studio direction"), Rig.PuckFinishLight->GetActorLocation().Equals(ElevatedPosition, .01f));
	TestEqual(TEXT("Reapplying arena settings never flashes puck studio power"),
		Rig.PuckFinishLight->RectLightComponent->Intensity, Defaults.PuckFinishLightLumens * Coverage * Coverage);
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(0, -1600, 1200), 1.f, 250.f);
	TestTrue(TEXT("Returning to locker restores its original light position"), Rig.PuckFinishLight->GetActorLocation().Equals(KeyPosition, .01f));
	TestEqual(TEXT("Returning to locker restores its original light power"), Rig.PuckFinishLight->RectLightComponent->Intensity, Defaults.PuckFinishLightLumens);
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(0, -1600, 1200), 1.f, 250.f, &ProductFrame, true);
	TestEqual(TEXT("Aligned gameplay reflection uses the softer key"), Rig.PuckFinishLight->RectLightComponent->SpecularScale, Defaults.PuckGameplayKeySpecularScale);
	FlickArenaLighting::Configure(World, Rig, Defaults);
	TestEqual(TEXT("Reapplying settings preserves gameplay reflection strength"), Rig.PuckFinishLight->RectLightComponent->SpecularScale, Defaults.PuckGameplayKeySpecularScale);
	FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight, Rig.PuckFillLight, FVector(0, -1600, 1200), 1.f, 250.f);
	TestEqual(TEXT("Locker restores its approved specular strength"), Rig.PuckFinishLight->RectLightComponent->SpecularScale, .55f);
	const FRotator ReferenceView(-30.f, 90.f, 0.f);
	const FVector ReferenceReflection(Defaults.PuckKeyOffset.X, Defaults.PuckKeyOffset.Y, Defaults.PuckKeyOffset.Z - 250.f);
	for (float Elevation : {5.f, 15.f, 30.f, 45.f, 65.f, 85.f})
	{
		const FRotator View(-Elevation, 90.f, 0.f);
		const auto Frame = FlickArenaLighting::MakePuckStudioFrame(View, ReferenceView, -90.f, 0.f, true);
		const FVector ToEye = -View.Vector();
		const FVector ToLight = Frame.RotateVector(ReferenceReflection).GetSafeNormal();
		const FVector HalfVector = (ToEye + ToLight).GetSafeNormal();
		TestTrue(*FString::Printf(TEXT("Silver reflection stays on the top normal at %.0f degrees"), Elevation),
			FVector::DotProduct(HalfVector, FVector::UpVector) > .999f);
		TestTrue(TEXT("Fill stays above the puck plane"), Frame.RotateVector(Defaults.PuckFillOffset - Focus).Z > 0.f);
	}
	const FRotator MenuView(-20.f, 120.f, 0.f);
	const FQuat LegacyMenuFrame = MenuView.Quaternion() * ReferenceView.Quaternion().Inverse();
	TestTrue(TEXT("Approved Home lighting retains its direct view frame"),
		FlickArenaLighting::MakePuckStudioFrame(MenuView, ReferenceView, -90.f, 0.f, false).Equals(LegacyMenuFrame, .0001f));
	return true;
}
#endif
