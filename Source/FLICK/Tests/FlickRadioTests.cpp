#if WITH_DEV_AUTOMATION_TESTS
#include "Audio/FlickRadioSynthesis.h"
#include "Audio/FlickMenuRadioComponent.h"
#include "Audio/FlickAudioDirector.h"
#include "Audio.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Sound/SoundWaveProcedural.h"
#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Components/AudioComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRadioSynthesisTest, "FLICK.Audio.RadioSynthesis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickRadioSynthesisTest::RunTest(const FString& Parameters)
{
	const bool Export = FParse::Param(FCommandLine::Get(), TEXT("FlickExportSoundReview"));
	TArray<int16> Previous;
	for (int32 Track = 0; Track < FlickRadioSynthesis::TrackCount; ++Track)
	{
		const float Duration = FlickRadioSynthesis::GetDuration(Track);
		TestTrue(TEXT("Tracks have a full arranged duration"), Duration > 40 && Duration < 70);
		const TArray<int16> Samples = FlickRadioSynthesis::Render(Track, Export ? 0 : 12.0f);
		TestEqual(TEXT("Stereo sample count"), Samples.Num(), FMath::RoundToInt((Export ? Duration : 12.0f) * FlickRadioSynthesis::SampleRate) * 2);
		if (Samples.IsEmpty()) return false;
		TestEqual(TEXT("Silent onset"), Samples[0], int16(0));
		TestEqual(TEXT("Silent end"), Samples.Last(), int16(0));
		int32 Peak = 0;
		double Energy = 0;
		int32 DifferentStereoFrames = 0;
		for (int32 Index = 0; Index < Samples.Num(); ++Index)
		{
			Peak = FMath::Max(Peak, FMath::Abs(int32(Samples[Index])));
			Energy += double(Samples[Index]) * Samples[Index];
			if (Index % 2 == 0 && Samples[Index] != Samples[Index + 1]) ++DifferentStereoFrames;
		}
		TestTrue(TEXT("Audible music with mixing headroom"), FMath::Sqrt(Energy / Samples.Num()) > 100 && Peak < 29000);
		TestTrue(TEXT("Stereo width"), DifferentStereoFrames > Samples.Num() / 4);
		TestTrue(TEXT("Different arrangements"), Samples != Previous);
		Previous = Samples;
		TestTrue(TEXT("Deterministic music independent of physics RNG"), FlickRadioSynthesis::Render(Track, 1.0f) == FlickRadioSynthesis::Render(Track, 1.0f));
		if (Export)
		{
			const FString Directory = FPaths::ProjectSavedDir() / TEXT("AudioReview");
			IFileManager::Get().MakeDirectory(*Directory, true);
			TArray<uint8> Wave;
			SerializeWaveFile(Wave, reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16), 2, FlickRadioSynthesis::SampleRate);
			TestTrue(TEXT("Music review WAV saved"), FFileHelper::SaveArrayToFile(Wave, *(Directory / (FString(FlickRadioSynthesis::GetTitle(Track)) + TEXT(".wav")))));
		}
	}
	TestTrue(TEXT("Invalid track rejected"), FlickRadioSynthesis::Render(-1).IsEmpty());
	TestTrue(TEXT("Out-of-range track rejected"), FlickRadioSynthesis::Render(3).IsEmpty());
	TestTrue(TEXT("Invalid preview duration rejected"), FlickRadioSynthesis::Render(0, -1).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRadioContextTest, "FLICK.Audio.RadioContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickRadioContextTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Radio test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickGameState* State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
	AFlickPlayerController* Controller = World->SpawnActor<AFlickPlayerController>();
	UFlickMenuRadioComponent* Radio = Controller->GetMenuRadio();
	TestNotNull(TEXT("Every controller has local radio"), Radio);
	State->MatchPhase = EFlickMatchPhase::WaitingToStart;
	TestTrue(TEXT("Client lobby is a music context"), Radio->IsMenuContext());
	State->MatchPhase = EFlickMatchPhase::Aiming;
	TestFalse(TEXT("Client gameplay silences radio"), Radio->IsMenuContext());
	State->MatchPhase = EFlickMatchPhase::RoundOver;
	State->bSeriesComplete = true;
	TestFalse(TEXT("Winning showcase does not overlap menu music"), Radio->IsMenuContext());
	const FString First = Radio->GetTrackTitle();
	Radio->Skip(-1);
	TestEqual(TEXT("Previous wraps"), Radio->GetTrackTitle(), FString(FlickRadioSynthesis::GetTitle(2)));
	Radio->Skip(1);
	TestEqual(TEXT("Next wraps"), Radio->GetTrackTitle(), First);
	const AFlickAudioDirector* Director = World->SpawnActor<AFlickAudioDirector>();
	TestTrue(TEXT("World sound director replicates to online clients"), Director->GetIsReplicated());
	TestTrue(TEXT("World sound director is always relevant"), Director->bAlwaysRelevant);
	const UFlickGameInstance* Instance = NewObject<UFlickGameInstance>();
	TestTrue(TEXT("New profiles enable radio"), Instance->IsMenuRadioEnabled());
	TestEqual(TEXT("Conservative music mix default"), Instance->GetMusicVolume(), 0.45f);
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRadioResumeTest, "FLICK.Audio.RadioResume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickRadioResumeTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Resume test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Controller = World->SpawnActor<AFlickPlayerController>();
	auto* Radio = Controller->GetMenuRadio();
	TArray<int16> Samples;
	Samples.Init(1000, FlickRadioSynthesis::SampleRate * 2 * 30);
	for (const float Position : {0.0f, 4.25f, 13.5f, 28.0f})
	{
		Radio->PlayedSeconds = Position;
		Radio->StartTrack(Samples);
		TestEqual(TEXT("Resume retains song position"), Radio->PlayedSeconds, Position);
		TestEqual(TEXT("Resumed voice lasts only remaining song"), Radio->Source->Duration, 30.0f - Position);
		const int32 RemainingBytes = FMath::RoundToInt((30.0f - Position) * FlickRadioSynthesis::SampleRate) * 2 * sizeof(int16);
		TestEqual(TEXT("Only unplayed stereo PCM is queued"), Radio->Source->GetAvailableAudioByteCount(), RemainingBytes);
		Radio->SuspendPlayback();
		TestNull(TEXT("Match suspension discards the old procedural source"), Radio->Source.Get());
		TestNull(TEXT("Match suspension releases the voice"), Radio->Music.Get());
		TestEqual(TEXT("Match suspension retains song position"), Radio->PlayedSeconds, Position);
		Radio->StartTrack(Samples);
		TestEqual(TEXT("Returning from match requeues audible PCM"), Radio->Source->GetAvailableAudioByteCount(), RemainingBytes);
		Radio->SuspendPlayback();
	}
	if (GEngine->GetMainAudioDevice().IsValid())
	{
		World->bAllowAudioPlayback = true;
		FAudioDeviceParams DeviceParams;
		DeviceParams.AssociatedWorld = World;
		DeviceParams.Scope = EAudioDeviceScope::Shared;
		World->SetAudioDevice(FAudioDeviceManager::Get()->RequestAudioDevice(DeviceParams));
		World->SetGameInstance(NewObject<UFlickGameInstance>());
		Controller->SetAsLocalPlayerController();
		auto* State = World->SpawnActor<AFlickGameState>();
		World->SetGameState(State);
		State->MatchPhase = EFlickMatchPhase::WaitingToStart;
		Radio->Cache.Add(0, Samples);
		Radio->PlayedSeconds = 4.25f;
		Radio->TickComponent(0.1f, LEVELTICK_All, nullptr);
		if (TestNotNull(TEXT("Menu creates a real audio voice"), Radio->Music.Get()))
		{
			TestTrue(TEXT("Initial menu voice is playing"), Radio->Music->IsPlaying());
			const TWeakObjectPtr<USoundWaveProcedural> OldSource = Radio->Source;
			State->MatchPhase = EFlickMatchPhase::Aiming;
			Radio->TickComponent(0.1f, LEVELTICK_All, nullptr);
			Radio->PauseAt = 0; // finish the fade without sleeping in the test
			Radio->TickComponent(0.1f, LEVELTICK_All, nullptr);
			TestNull(TEXT("Gameplay releases the menu voice"), Radio->Music.Get());
			const float SavedPosition = Radio->PlayedSeconds;
			State->MatchPhase = EFlickMatchPhase::WaitingToStart;
			Radio->TickComponent(0.1f, LEVELTICK_All, nullptr);
			if (TestNotNull(TEXT("Returning to menu recreates the audio voice"), Radio->Music.Get()))
			{
				TestTrue(TEXT("Returned menu voice is playing"), Radio->Music->IsPlaying());
				TestTrue(TEXT("Return does not revive the old drained source"), Radio->Source.Get() != OldSource.Get());
				TestTrue(TEXT("Return preserves position"), FMath::IsNearlyEqual(Radio->PlayedSeconds, SavedPosition, 0.05f));
				Radio->FadeStartedAt -= 1.0;
				Radio->TickComponent(0.1f, LEVELTICK_All, nullptr);
				TestTrue(TEXT("Returned voice fades above zero volume"), Radio->Music->VolumeMultiplier > 0.0f);
			}
			Radio->SuspendPlayback();
		}
	}
	else AddWarning(TEXT("No audio device: PCM resume tested, real voice transition skipped."));
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
