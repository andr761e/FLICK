#include "Audio/FlickMenuRadioComponent.h"
#include "Audio/FlickRadioSynthesis.h"
#include "Core/FlickLog.h"
#include "Async/Async.h"
#include "Components/AudioComponent.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

UFlickMenuRadioComponent::UFlickMenuRadioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

bool UFlickMenuRadioComponent::IsMenuContext() const
{
	const AFlickGameMode* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
	// A stale frontend match state can remain behind the main menu: the screen wins.
	if (Mode)
	{
		const EFlickFrontendScreen Screen = Mode->GetFrontendScreen();
		if (Screen == EFlickFrontendScreen::Playing || Screen == EFlickFrontendScreen::Paused) return false;
		if (Screen == EFlickFrontendScreen::Settings)
			return Mode->GetSettingsReturnScreen() != EFlickFrontendScreen::Paused;
		if (Screen == EFlickFrontendScreen::ClassSelect)
			return !State || (!State->bNetworkClassSelectionActive && !State->bPrivateMatchAssignmentActive);
		return true;
	}
	return State && !State->IsGameplayActive();
}

bool UFlickMenuRadioComponent::IsEnabled() const
{
	const auto* Settings = Cast<UFlickGameInstance>(GetOwner()->GetGameInstance());
	return Settings && Settings->IsMenuRadioEnabled();
}

void UFlickMenuRadioComponent::TogglePlayback()
{
	if (auto* Settings = Cast<UFlickGameInstance>(GetOwner()->GetGameInstance()))
		Settings->SetMenuRadioEnabled(!Settings->IsMenuRadioEnabled());
}

void UFlickMenuRadioComponent::Skip(int32 Direction)
{
	if (IsValid(Music))
	{
		Music->bAutoDestroy = true;
		Music->FadeOut(0.2f, 0.0f);
	}
	Music = nullptr;
	Source = nullptr;
	bAudible = false;
	LastVolume = -1;
	PlayedSeconds = 0;
	AppliedVolume = 0;
	Track = (Track + (Direction < 0 ? FlickRadioSynthesis::TrackCount - 1 : 1)) % FlickRadioSynthesis::TrackCount;
}

FString UFlickMenuRadioComponent::GetTrackTitle() const { return FlickRadioSynthesis::GetTitle(Track); }
FString UFlickMenuRadioComponent::GetStatus() const
{
	const auto* Settings = Cast<UFlickGameInstance>(GetOwner()->GetGameInstance());
	if (Settings && Settings->IsReplayMusicMutedForStreaming()) return TEXT("MUSIC MUTED / STREAM SAFE");
	if (!IsEnabled()) return TEXT("PAUSED");
	if (!Music) return TEXT("TUNING IN...");
	return FString::Printf(TEXT("FLICK ORIGINALS / %s"), FlickRadioSynthesis::GetStyle(Track));
}
float UFlickMenuRadioComponent::GetProgress() const
{
	return FMath::Clamp(PlayedSeconds / FlickRadioSynthesis::GetDuration(Track), 0.0f, 1.0f);
}

void UFlickMenuRadioComponent::RequestTrack()
{
	if (PlayedSeconds >= FlickRadioSynthesis::GetDuration(Track)) Skip(1);
	if (const TArray<int16>* Samples = Cache.Find(Track)) { StartTrack(*Samples); return; }
	if (PendingSamples.IsValid()) return;
	PendingTrack = Track;
	// No UObject captured: travel/destruction cannot invalidate the rendering job.
	PendingSamples = Async(EAsyncExecution::ThreadPool, [Index = Track]() { return FlickRadioSynthesis::Render(Index); });
}

void UFlickMenuRadioComponent::StartTrack(const TArray<int16>& Samples)
{
	if (Samples.IsEmpty()) return;
	const int32 StartFrame = FMath::Clamp(FMath::FloorToInt(PlayedSeconds * FlickRadioSynthesis::SampleRate),
		0, Samples.Num() / 2 - 1);
	const int32 StartSample = StartFrame * 2;
	PlayedSeconds = static_cast<float>(StartFrame) / FlickRadioSynthesis::SampleRate;
	Source = NewObject<USoundWaveProcedural>(this);
	Source->SetSampleRate(FlickRadioSynthesis::SampleRate);
	Source->NumChannels = 2;
	Source->Duration = static_cast<float>(Samples.Num() - StartSample) / (2 * FlickRadioSynthesis::SampleRate);
	Source->SoundGroup = SOUNDGROUP_Music;
	Source->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
	// Procedural waves cannot seek themselves. Requeue only the unplayed PCM,
	// rather than reviving an old paused voice whose buffer may have drained.
	Source->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData() + StartSample),
		(Samples.Num() - StartSample) * sizeof(int16));
	Music = UGameplayStatics::SpawnSound2D(this, Source, 0.0f, 1.0f, 0.0f, nullptr, false, false);
	if (Music) UE_LOG(LogFlick, Log, TEXT("MENU_RADIO_STARTED: %s, %.2fs, %d stereo samples"),
		FlickRadioSynthesis::GetTitle(Track), Source->Duration, Samples.Num());
	LastVolume = -1;
	bAudible = false;
	AppliedVolume = 0;
}

void UFlickMenuRadioComponent::SuspendPlayback()
{
	if (IsValid(Music)) { Music->Stop(); Music->DestroyComponent(); }
	Music = nullptr;
	Source = nullptr;
	bAudible = false;
	AppliedVolume = 0;
	LastVolume = -1;
	// Keep Track, PlayedSeconds and cached PCM. Gameplay suspension is not the
	// user's persistent Pause preference and must never change that setting.
}

void UFlickMenuRadioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || GetNetMode() == NM_DedicatedServer
		|| !GetWorld()->AllowAudioPlayback()) return;
	const auto* Settings = Cast<UFlickGameInstance>(Controller->GetGameInstance());
	if (!Settings) return;
	// A controller can survive seamless travel while its old world audio cannot.
	if (Music && (!IsValid(Music) || Music->GetWorld() != GetWorld() || !Music->IsPlaying()))
	{
		SuspendPlayback();
	}
	const double Now = FPlatformTime::Seconds();
	const float RealDelta = PreviousTime > 0 ? FMath::Clamp(static_cast<float>(Now - PreviousTime), 0.0f, 0.5f) : 0;
	PreviousTime = Now;
	const float Volume = Settings->GetMasterVolume() * Settings->GetMusicVolume();
	const bool WantAudio = IsMenuContext() && IsEnabled() && !Settings->IsReplayMusicMutedForStreaming() && Volume > KINDA_SMALL_NUMBER;
	if (PendingSamples.IsValid() && PendingSamples.IsReady())
	{
		Cache.Add(PendingTrack, PendingSamples.Get());
		PendingSamples = TFuture<TArray<int16>>();
	}
	if (!Music && WantAudio) RequestTrack();
	if (!Music) return;
	if (WantAudio != bAudible || (WantAudio && !FMath::IsNearlyEqual(Volume, LastVolume)))
	{
		FadeStartedAt = Now;
		FadeStartVolume = AppliedVolume;
		if (!WantAudio) PauseAt = Now + 0.35;
		bAudible = WantAudio;
		LastVolume = Volume;
	}
	// AdjustVolume(..., 0) schedules an Unreal voice stop, so it cannot be used
	// for a resumable radio pause. Fade the multiplier, then release the voice.
	AppliedVolume = FMath::Lerp(FadeStartVolume, WantAudio ? Volume : 0.0f,
		FMath::Clamp(static_cast<float>((Now - FadeStartedAt) / (WantAudio ? 0.6 : 0.35)), 0.0f, 1.0f));
	Music->SetVolumeMultiplier(AppliedVolume);
	PlayedSeconds += RealDelta;
	if (!WantAudio && Now >= PauseAt) { SuspendPlayback(); return; }
	if (WantAudio)
	{
		if (PlayedSeconds >= FlickRadioSynthesis::GetDuration(Track)) Skip(1);
	}
}

void UFlickMenuRadioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Music) { Music->Stop(); Music->DestroyComponent(); }
	Music = nullptr;
	Source = nullptr;
	Cache.Empty();
	Super::EndPlay(Reason);
}
