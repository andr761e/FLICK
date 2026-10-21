#if WITH_DEV_AUTOMATION_TESTS
#include "Audio/FlickAudioDirector.h"
#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickAudioLifecycleTest, "FLICK.Audio.VoiceLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickAudioLifecycleTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(true)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Audio lifecycle world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	if (!GEngine->GetMainAudioDevice().IsValid())
	{
		AddWarning(TEXT("No audio device: voice lifecycle integration requires audio enabled."));
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return true;
	}
	FAudioDeviceParams DeviceParams;
	DeviceParams.AssociatedWorld = World;
	DeviceParams.Scope = EAudioDeviceScope::Shared;
	World->SetAudioDevice(FAudioDeviceManager::Get()->RequestAudioDevice(DeviceParams));
	AFlickAudioDirector* Director = World->SpawnActor<AFlickAudioDirector>();
	TestTrue(TEXT("Audio cleanup ticks while paused"), Director->PrimaryActorTick.bTickEvenWhenPaused);
	TestEqual(TEXT("Pause cannot stall an interval cooldown"), Director->PrimaryActorTick.TickInterval, 0.0f);
	FFlickAudioCue Cue;
	Cue.Kind = static_cast<uint8>(EFlickGeneratedSoundKind::Impact);
	Cue.Duration = .26f;
	Cue.Frequency = 140;
	Cue.Volume = .1f;
	Cue.Pitch = .8f;
	Cue.bSpatial = true;
	Director->PlayLocalCue(Cue);
	if (TestEqual(TEXT("Spatial cue has one owned voice"), Director->ActiveVoices.Num(), 1))
	{
		UAudioComponent* Voice = Director->ActiveVoices[0].Component;
		TestTrue(TEXT("Voice starts playing"), Voice->IsPlaying());
		const double StartedAt = Director->LastVoiceUpdate;
		const double OriginalStopAt = Director->ActiveVoices[0].StopAt;
		TestTrue(TEXT("Expiry includes pitch-adjusted duration and mixer tail"),
			OriginalStopAt >= StartedAt + Cue.Duration / Cue.Pitch + .14);
		Director->UpdateVoices(StartedAt + 2.0, true);
		if (TestEqual(TEXT("Paused physical sound is not expired"), Director->ActiveVoices.Num(), 1))
			TestTrue(TEXT("Pause extends physical voice lifetime"), FMath::IsNearlyEqual(Director->ActiveVoices[0].StopAt, OriginalStopAt + 2.0, .001));
		Director->UpdateVoices(OriginalStopAt + 2.01, false);
		TestTrue(TEXT("Expired procedural voice releases component"), Voice->IsBeingDestroyed());
		TestEqual(TEXT("No silent physical voices retained"), Director->ActiveVoices.Num(), 0);
	}
	Director->PlayLocalNotification(true);
	if (TestEqual(TEXT("Interface notification has an owned voice"), Director->ActiveVoices.Num(), 1))
	{
		Director->UpdateVoices(Director->ActiveVoices[0].StopAt + .01, true);
		TestEqual(TEXT("UI sounds finish even when gameplay is paused"), Director->ActiveVoices.Num(), 0);
	}
	Director->PlayReplayMusicLocal(3, EFlickTeam::Player1);
	if (TestEqual(TEXT("Replay bed also has a finite voice"), Director->ActiveVoices.Num(), 1))
	{
		Director->UpdateVoices(Director->ActiveVoices[0].StopAt + .01, false);
		TestEqual(TEXT("Replay cannot leave a silent streaming voice"), Director->ActiveVoices.Num(), 0);
	}
	Director->PlayLocalNotification(false);
	UAudioComponent* LastVoice = Director->ActiveVoices.IsEmpty() ? nullptr : Director->ActiveVoices[0].Component.Get();
	Director->EndPlay(EEndPlayReason::LevelTransition);
	TestEqual(TEXT("Travel clears all tracked audio"), Director->ActiveVoices.Num(), 0);
	if (LastVoice) TestTrue(TEXT("Travel stops the old voice"), LastVoice->IsBeingDestroyed());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
