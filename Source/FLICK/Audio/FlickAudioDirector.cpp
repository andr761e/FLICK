#include "Audio/FlickAudioDirector.h"

#include "Components/AudioComponent.h"
#include "Game/FlickGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"

AFlickAudioDirector::AFlickAudioDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFlickAudioDirector::PlayUi(const bool bConfirm)
{
	if (!CanPlay(bConfirm ? EFlickGeneratedSoundKind::UiConfirm : EFlickGeneratedSoundKind::UiNavigate, 0.025f))
	{
		return;
	}
	PlayGenerated(
		bConfirm ? EFlickGeneratedSoundKind::UiConfirm : EFlickGeneratedSoundKind::UiNavigate,
		bConfirm ? 0.18f : 0.055f,
		bConfirm ? 587.33f : 740.0f,
		bConfirm ? 0.46f : 0.27f,
		1.0f,
		nullptr,
		bConfirm ? 0.62f : 0.78f,
		bConfirm ? 0.9f : 0.68f);
}

float AFlickAudioDirector::GetArchetypeTimbre(const EFlickPieceArchetype Archetype) const
{
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy: return 0.14f;
	case EFlickPieceArchetype::Blocker: return 0.24f;
	case EFlickPieceArchetype::Toppler: return 0.32f;
	case EFlickPieceArchetype::Grippy: return 0.42f;
	case EFlickPieceArchetype::Standard: return 0.5f;
	case EFlickPieceArchetype::Slider: return 0.64f;
	case EFlickPieceArchetype::Bouncer: return 0.74f;
	case EFlickPieceArchetype::Striker: return 0.84f;
	case EFlickPieceArchetype::Compact: return 0.92f;
	default: return 0.5f;
	}
}

void AFlickAudioDirector::PlayLaunch(
	const EFlickPieceArchetype Archetype,
	const float Power,
	const FVector& Location)
{
	const float SafePower = FMath::Clamp(Power, 0.0f, 1.0f);
	float Frequency = 170.0f;
	float Pitch = 1.0f;
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy:
		Frequency = 105.0f;
		Pitch = 0.88f;
		break;
	case EFlickPieceArchetype::Striker:
		Frequency = 255.0f;
		Pitch = 1.12f;
		break;
	case EFlickPieceArchetype::Grippy:
		Frequency = 145.0f;
		Pitch = 0.95f;
		break;
	case EFlickPieceArchetype::Slider:
		Frequency = 205.0f;
		Pitch = 1.04f;
		break;
	case EFlickPieceArchetype::Blocker:
		Frequency = 125.0f;
		Pitch = 0.91f;
		break;
	case EFlickPieceArchetype::Compact:
		Frequency = 285.0f;
		Pitch = 1.16f;
		break;
	case EFlickPieceArchetype::Bouncer:
		Frequency = 235.0f;
		Pitch = 1.09f;
		break;
	case EFlickPieceArchetype::Toppler:
		Frequency = 132.0f;
		Pitch = 0.92f;
		break;
	case EFlickPieceArchetype::Standard:
	default:
		break;
	}
	PlayGenerated(
		EFlickGeneratedSoundKind::Launch,
		FMath::Lerp(0.16f, 0.30f, SafePower),
		Frequency,
		FMath::Lerp(0.46f, 0.95f, SafePower),
		Pitch,
		&Location,
		GetArchetypeTimbre(Archetype),
		SafePower);
}

void AFlickAudioDirector::PlayImpact(
	const float Strength,
	const float CombinedMassKg,
	const EFlickPieceArchetype FirstArchetype,
	const EFlickPieceArchetype SecondArchetype,
	const FVector& Location)
{
	if (!CanPlay(EFlickGeneratedSoundKind::Impact, 0.045f))
	{
		return;
	}
	const float SafeStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	const float MassPitch = FMath::Clamp(10.0f / FMath::Max(CombinedMassKg, 2.0f), 0.72f, 1.22f);
	const float Timbre = (GetArchetypeTimbre(FirstArchetype) + GetArchetypeTimbre(SecondArchetype)) * 0.5f;
	PlayGenerated(
		EFlickGeneratedSoundKind::Impact,
		FMath::Lerp(0.09f, 0.26f, SafeStrength),
		FMath::Lerp(230.0f, 95.0f, SafeStrength),
		FMath::Lerp(0.22f, 0.92f, SafeStrength),
		MassPitch,
		&Location,
		Timbre,
		SafeStrength);
}

void AFlickAudioDirector::PlayRimImpact(
	const float Strength,
	const float MassKg,
	const EFlickPieceArchetype Archetype,
	const FVector& Location)
{
	if (!CanPlay(EFlickGeneratedSoundKind::RimImpact, 0.075f))
	{
		return;
	}
	const float SafeStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	const float MassPitch = FMath::Clamp(5.0f / FMath::Max(MassKg, 1.0f), 0.78f, 1.18f);
	PlayGenerated(
		EFlickGeneratedSoundKind::RimImpact,
		FMath::Lerp(0.16f, 0.42f, SafeStrength),
		FMath::Lerp(420.0f, 245.0f, SafeStrength),
		FMath::Lerp(0.32f, 0.88f, SafeStrength),
		MassPitch,
		&Location,
		GetArchetypeTimbre(Archetype),
		SafeStrength);
}

void AFlickAudioDirector::PlayRingOut(const FVector& Location)
{
	PlayGenerated(EFlickGeneratedSoundKind::RingOut, 0.56f, 220.0f, 0.76f, 1.0f, &Location, 0.5f, 1.0f);
}

void AFlickAudioDirector::PlayTurn(const EFlickTeam Team)
{
	PlayGenerated(
		EFlickGeneratedSoundKind::Turn,
		0.3f,
		Team == EFlickTeam::Player2 ? 440.0f : 587.33f,
		0.40f,
		1.0f);
}

void AFlickAudioDirector::PlayRoundResult(
	const EFlickTeam Winner,
	const bool bDraw,
	const bool bSeriesComplete)
{
	const float Root = bDraw ? 293.66f : (Winner == EFlickTeam::Player2 ? 440.0f : 587.33f);
	PlayGenerated(
		bSeriesComplete ? EFlickGeneratedSoundKind::MatchWin : EFlickGeneratedSoundKind::RoundWin,
		bSeriesComplete ? 1.4f : 0.78f,
		Root,
		bSeriesComplete ? 0.80f : 0.64f,
		1.0f);
}

void AFlickAudioDirector::PlayReplayMusic(const float Duration, const EFlickTeam WinningTeam)
{
	StopReplayMusic();
	const float MixedVolume = 0.72f * GetEffectsVolume();
	if (MixedVolume <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Root = WinningTeam == EFlickTeam::Player2 ? 92.50f : 110.0f;
	USoundWaveProcedural* Sound = CreateSound(
		EFlickGeneratedSoundKind::ReplayMusic,
		FMath::Clamp(Duration, 1.25f, 8.0f),
		Root,
		SoundSeed++,
		0.62f,
		1.0f);
	if (!Sound)
	{
		return;
	}
	Sound->SoundGroup = SOUNDGROUP_Music;
	ReplayMusicComponent = UGameplayStatics::SpawnSound2D(this, Sound, MixedVolume, 1.0f);
}

void AFlickAudioDirector::StopReplayMusic()
{
	if (IsValid(ReplayMusicComponent))
	{
		ReplayMusicComponent->FadeOut(0.18f, 0.0f);
		ReplayMusicComponent = nullptr;
	}
}

USoundWaveProcedural* AFlickAudioDirector::CreateSound(
	const EFlickGeneratedSoundKind Kind,
	const float Duration,
	const float BaseFrequency,
	const int32 Seed,
	const float Timbre,
	const float Intensity)
{
	USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(this);
	if (!Sound)
	{
		return nullptr;
	}

	const TArray<int16> Samples = FlickSoundSynthesis::Render(Kind, Duration, BaseFrequency, Seed, Timbre, Intensity);
	if (Samples.IsEmpty()) return nullptr;

	Sound->SetSampleRate(FlickSoundSynthesis::SampleRate);
	Sound->NumChannels = 1;
	Sound->Duration = Duration;
	Sound->SoundGroup = SOUNDGROUP_Effects;
	Sound->bLooping = false;
	Sound->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
	GeneratedSounds.Add(Sound);
	if (GeneratedSounds.Num() > 128)
	{
		GeneratedSounds.RemoveAt(0, 32, EAllowShrinking::No);
	}
	return Sound;
}

void AFlickAudioDirector::PlayGenerated(
	const EFlickGeneratedSoundKind Kind,
	const float Duration,
	const float BaseFrequency,
	const float Volume,
	const float Pitch,
	const FVector* Location,
	const float Timbre,
	const float Intensity)
{
	const bool bInterfaceSound = Kind == EFlickGeneratedSoundKind::UiNavigate
		|| Kind == EFlickGeneratedSoundKind::UiConfirm;
	const float MixedVolume = Volume * (bInterfaceSound ? GetInterfaceVolume() : GetEffectsVolume());
	if (MixedVolume <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	USoundWaveProcedural* Sound = CreateSound(Kind, Duration, BaseFrequency, SoundSeed++, Timbre, Intensity);
	if (!Sound)
	{
		return;
	}

	if (Location)
	{
		if (!SpatialAttenuation)
		{
			SpatialAttenuation = NewObject<USoundAttenuation>(this, TEXT("RuntimeArenaAttenuation"));
			SpatialAttenuation->Attenuation.bAttenuate = true;
			SpatialAttenuation->Attenuation.bSpatialize = true;
			SpatialAttenuation->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
			SpatialAttenuation->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
			// The gameplay camera is roughly 2,200 units from board center and can
			// be farther from pucks on the opposite edge. Keep the entire arena in
			// the full-volume region; spatialization still supplies direction.
			SpatialAttenuation->Attenuation.AttenuationShapeExtents = FVector(3600.0f, 0.0f, 0.0f);
			SpatialAttenuation->Attenuation.FalloffDistance = 2400.0f;
		}
		UGameplayStatics::PlaySoundAtLocation(
			this,
			Sound,
			*Location,
			MixedVolume,
			Pitch,
			0.0f,
			SpatialAttenuation);
	}
	else
	{
		UGameplayStatics::PlaySound2D(this, Sound, MixedVolume, Pitch);
	}
}

bool AFlickAudioDirector::CanPlay(const EFlickGeneratedSoundKind Kind, const float MinimumInterval)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// UI audio must continue to debounce correctly while the game is paused.
	const float Now = World->GetRealTimeSeconds();
	float* LastTime = nullptr;
	switch (Kind)
	{
	case EFlickGeneratedSoundKind::Impact:
		LastTime = &LastImpactTime;
		break;
	case EFlickGeneratedSoundKind::RimImpact:
		LastTime = &LastRimImpactTime;
		break;
	case EFlickGeneratedSoundKind::UiNavigate:
	case EFlickGeneratedSoundKind::UiConfirm:
		LastTime = &LastUiTime;
		break;
	default:
		return true;
	}
	if (Now - *LastTime < MinimumInterval)
	{
		return false;
	}
	*LastTime = Now;
	return true;
}

float AFlickAudioDirector::GetEffectsVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetGameInstance<UFlickGameInstance>();
	return FlickGameInstance
		? FlickGameInstance->GetMasterVolume() * FlickGameInstance->GetEffectsVolume()
		: 1.0f;
}

float AFlickAudioDirector::GetInterfaceVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetGameInstance<UFlickGameInstance>();
	return FlickGameInstance
		? FlickGameInstance->GetMasterVolume() * FlickGameInstance->GetInterfaceVolume()
		: 1.0f;
}
