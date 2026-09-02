#include "Audio/FlickAudioDirector.h"

#include "Game/FlickGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	constexpr int32 AudioSampleRate = 24000;
	constexpr float TwoPi = 2.0f * PI;

	float NextNoise(uint32& State)
	{
		State = State * 1664525u + 1013904223u;
		return static_cast<float>((State >> 8) & 0xffffu) / 32767.5f - 1.0f;
	}
}

AFlickAudioDirector::AFlickAudioDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFlickAudioDirector::PlayUi(const bool bConfirm)
{
	PlayGenerated(
		bConfirm ? EFlickGeneratedSoundKind::UiConfirm : EFlickGeneratedSoundKind::UiNavigate,
		bConfirm ? 0.13f : 0.07f,
		bConfirm ? 520.0f : 680.0f,
		bConfirm ? 0.38f : 0.24f,
		1.0f);
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
		FMath::Lerp(0.16f, 0.3f, SafePower),
		Frequency,
		FMath::Lerp(0.28f, 0.72f, SafePower),
		Pitch,
		&Location);
}

void AFlickAudioDirector::PlayImpact(
	const float Strength,
	const float CombinedMassKg,
	const FVector& Location)
{
	const float SafeStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	const float MassPitch = FMath::Clamp(10.0f / FMath::Max(CombinedMassKg, 2.0f), 0.72f, 1.22f);
	PlayGenerated(
		EFlickGeneratedSoundKind::Impact,
		FMath::Lerp(0.1f, 0.34f, SafeStrength),
		FMath::Lerp(230.0f, 95.0f, SafeStrength),
		FMath::Lerp(0.22f, 0.92f, SafeStrength),
		MassPitch,
		&Location);
}

void AFlickAudioDirector::PlayRingOut(const FVector& Location)
{
	PlayGenerated(EFlickGeneratedSoundKind::RingOut, 0.72f, 190.0f, 0.86f, 1.0f, &Location);
}

void AFlickAudioDirector::PlayTurn(const EFlickTeam Team)
{
	PlayGenerated(
		EFlickGeneratedSoundKind::Turn,
		0.3f,
		Team == EFlickTeam::Player2 ? 390.0f : 460.0f,
		0.42f,
		1.0f);
}

void AFlickAudioDirector::PlayRoundResult(
	const EFlickTeam Winner,
	const bool bDraw,
	const bool bSeriesComplete)
{
	const float Root = bDraw ? 285.0f : (Winner == EFlickTeam::Player2 ? 330.0f : 392.0f);
	PlayGenerated(
		bSeriesComplete ? EFlickGeneratedSoundKind::MatchWin : EFlickGeneratedSoundKind::RoundWin,
		bSeriesComplete ? 1.25f : 0.72f,
		Root,
		bSeriesComplete ? 0.82f : 0.62f,
		1.0f);
}

void AFlickAudioDirector::UpdateScrape(
	const float NormalizedSpeed,
	const FVector& Location,
	const float DeltaSeconds)
{
	ScrapeCooldown = FMath::Max(0.0f, ScrapeCooldown - DeltaSeconds);
	const float SafeSpeed = FMath::Clamp(NormalizedSpeed, 0.0f, 1.0f);
	if (ScrapeCooldown > 0.0f || SafeSpeed < 0.055f)
	{
		return;
	}

	ScrapeCooldown = FMath::Lerp(0.16f, 0.085f, SafeSpeed);
	PlayGenerated(
		EFlickGeneratedSoundKind::Scrape,
		0.11f,
		FMath::Lerp(620.0f, 1180.0f, SafeSpeed),
		FMath::Lerp(0.035f, 0.18f, SafeSpeed),
		FMath::Lerp(0.86f, 1.15f, SafeSpeed),
		&Location);
}

USoundWaveProcedural* AFlickAudioDirector::CreateSound(
	const EFlickGeneratedSoundKind Kind,
	const float Duration,
	const float BaseFrequency,
	const int32 Seed)
{
	USoundWaveProcedural* Sound = NewObject<USoundWaveProcedural>(this);
	if (!Sound)
	{
		return nullptr;
	}

	const int32 SampleCount = FMath::Max(1, FMath::RoundToInt(Duration * AudioSampleRate));
	TArray<int16> Samples;
	Samples.SetNumUninitialized(SampleCount);
	uint32 NoiseState = static_cast<uint32>(Seed);
	float SmoothedNoise = 0.0f;

	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Time = static_cast<float>(Index) / AudioSampleRate;
		const float Alpha = static_cast<float>(Index) / FMath::Max(1, SampleCount - 1);
		const float Decay = FMath::Square(1.0f - Alpha);
		const float Noise = NextNoise(NoiseState);
		SmoothedNoise = FMath::Lerp(SmoothedNoise, Noise, 0.16f);
		float Value = 0.0f;

		switch (Kind)
		{
		case EFlickGeneratedSoundKind::UiNavigate:
			Value = FMath::Sin(TwoPi * BaseFrequency * Time) * Decay;
			break;
		case EFlickGeneratedSoundKind::UiConfirm:
			Value = (FMath::Sin(TwoPi * BaseFrequency * Time)
				+ 0.35f * FMath::Sin(TwoPi * BaseFrequency * 1.5f * Time)) * Decay;
			break;
		case EFlickGeneratedSoundKind::Launch:
			Value = (0.72f * FMath::Sin(TwoPi * BaseFrequency * (1.0f - Alpha * 0.36f) * Time)
				+ 0.28f * SmoothedNoise) * Decay;
			break;
		case EFlickGeneratedSoundKind::Impact:
			Value = (0.58f * Noise + 0.62f * FMath::Sin(TwoPi * BaseFrequency * Time))
				* FMath::Pow(1.0f - Alpha, 3.0f);
			break;
		case EFlickGeneratedSoundKind::Scrape:
			Value = (Noise - SmoothedNoise * 0.72f) * FMath::Sin(PI * Alpha);
			break;
		case EFlickGeneratedSoundKind::RingOut:
			Value = (0.66f * FMath::Sin(TwoPi * BaseFrequency * (1.0f - Alpha * 0.68f) * Time)
				+ 0.22f * SmoothedNoise) * (1.0f - Alpha);
			break;
		case EFlickGeneratedSoundKind::Turn:
		{
			const float Note = Alpha < 0.48f ? BaseFrequency : BaseFrequency * 1.25f;
			Value = FMath::Sin(TwoPi * Note * Time) * FMath::Sin(PI * Alpha);
			break;
		}
		case EFlickGeneratedSoundKind::RoundWin:
		case EFlickGeneratedSoundKind::MatchWin:
		{
			const int32 NoteIndex = FMath::Min(2, FMath::FloorToInt(Alpha * 3.0f));
			const float Ratios[3] = {1.0f, 1.25f, 1.5f};
			const float Note = BaseFrequency * Ratios[NoteIndex];
			Value = (FMath::Sin(TwoPi * Note * Time)
				+ 0.28f * FMath::Sin(TwoPi * Note * 2.0f * Time))
				* FMath::Sin(PI * FMath::Frac(Alpha * 3.0f))
				* (1.0f - Alpha * 0.25f);
			break;
		}
		default:
			break;
		}

		Samples[Index] = static_cast<int16>(FMath::Clamp(Value * 24500.0f, -32767.0f, 32767.0f));
	}

	Sound->SetSampleRate(AudioSampleRate);
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
	const FVector* Location)
{
	const bool bInterfaceSound = Kind == EFlickGeneratedSoundKind::UiNavigate
		|| Kind == EFlickGeneratedSoundKind::UiConfirm;
	const float MixedVolume = Volume * (bInterfaceSound ? GetInterfaceVolume() : GetEffectsVolume());
	if (MixedVolume <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	USoundWaveProcedural* Sound = CreateSound(Kind, Duration, BaseFrequency, SoundSeed++);
	if (!Sound)
	{
		return;
	}

	if (Location)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, *Location, MixedVolume, Pitch);
	}
	else
	{
		UGameplayStatics::PlaySound2D(this, Sound, MixedVolume, Pitch);
	}
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
