#include "Audio/FlickAudioDirector.h"

#include "Game/FlickGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWaveProcedural.h"

namespace
{
	constexpr int32 AudioSampleRate = 48000;
	constexpr float TwoPi = 2.0f * PI;

	float NextNoise(uint32& State)
	{
		State = State * 1664525u + 1013904223u;
		return static_cast<float>((State >> 8) & 0xffffu) / 32767.5f - 1.0f;
	}

	float SoftClip(const float Value)
	{
		const float Driven = Value * 1.4f;
		return Driven / (1.0f + FMath::Abs(Driven)) * 0.95f;
	}

	float FastTransient(const float Alpha, const float Sharpness)
	{
		return FMath::Exp(-Alpha * Sharpness);
	}
}

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
		bConfirm ? 0.13f : 0.07f,
		bConfirm ? 520.0f : 680.0f,
		bConfirm ? 0.5f : 0.34f,
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
		FMath::Lerp(0.2f, 0.38f, SafePower),
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
		FMath::Lerp(0.1f, 0.34f, SafeStrength),
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
		FMath::Lerp(0.18f, 0.52f, SafeStrength),
		FMath::Lerp(420.0f, 245.0f, SafeStrength),
		FMath::Lerp(0.32f, 0.88f, SafeStrength),
		MassPitch,
		&Location,
		GetArchetypeTimbre(Archetype),
		SafeStrength);
}

void AFlickAudioDirector::PlayRingOut(const FVector& Location)
{
	PlayGenerated(EFlickGeneratedSoundKind::RingOut, 0.72f, 190.0f, 0.86f, 1.0f, &Location, 0.5f, 1.0f);
}

void AFlickAudioDirector::PlayTurn(const EFlickTeam Team)
{
	PlayGenerated(
		EFlickGeneratedSoundKind::Turn,
		0.3f,
		Team == EFlickTeam::Player2 ? 390.0f : 460.0f,
		0.58f,
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
		bSeriesComplete ? 0.95f : 0.78f,
		1.0f);
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

	const int32 SampleCount = FMath::Max(1, FMath::RoundToInt(Duration * AudioSampleRate));
	TArray<int16> Samples;
	Samples.SetNumUninitialized(SampleCount);
	uint32 NoiseState = static_cast<uint32>(Seed);
	float SmoothedNoise = 0.0f;
	const float SafeTimbre = FMath::Clamp(Timbre, 0.0f, 1.0f);
	const float SafeIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	const float Detune = 1.0f + static_cast<float>((Seed % 17) - 8) * 0.0018f;

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
			Value = (0.76f * FMath::Sin(TwoPi * BaseFrequency * Detune * Time)
				+ 0.2f * FMath::Sin(TwoPi * BaseFrequency * 2.02f * Time)
				+ 0.08f * Noise * FastTransient(Alpha, 30.0f)) * FMath::Pow(1.0f - Alpha, 3.2f);
			break;
		case EFlickGeneratedSoundKind::UiConfirm:
		{
			const float NoteRatio = Alpha < 0.48f ? 1.0f : 1.5f;
			const float NoteAlpha = FMath::Frac(Alpha * 2.0f);
			Value = (0.7f * FMath::Sin(TwoPi * BaseFrequency * NoteRatio * Detune * Time)
				+ 0.22f * FMath::Sin(TwoPi * BaseFrequency * NoteRatio * 2.0f * Time))
				* FMath::Sin(PI * NoteAlpha) * (1.0f - Alpha * 0.35f);
			break;
		}
		case EFlickGeneratedSoundKind::Launch:
		{
			const float Snap = (Noise - SmoothedNoise * 0.7f) * FastTransient(Alpha, 58.0f);
			const float BodyFrequency = BaseFrequency * Detune * (1.0f - Alpha * 0.42f);
			const float Body = FMath::Sin(TwoPi * BodyFrequency * Time)
				+ 0.32f * FMath::Sin(TwoPi * BodyFrequency * 1.58f * Time);
			const float WhooshEnvelope = FMath::Sin(PI * FMath::Clamp(Alpha * 1.3f, 0.0f, 1.0f));
			const float Whoosh = (Noise - SmoothedNoise) * WhooshEnvelope;
			Value = Snap * FMath::Lerp(0.32f, 0.52f, SafeTimbre)
				+ Body * Decay * FMath::Lerp(0.72f, 0.42f, SafeTimbre)
				+ Whoosh * FMath::Lerp(0.15f, 0.36f, SafeTimbre);
			break;
		}
		case EFlickGeneratedSoundKind::Impact:
			Value = (0.58f * Noise + 0.62f * FMath::Sin(TwoPi * BaseFrequency * Time))
				* FMath::Pow(1.0f - Alpha, 3.0f);
			break;
		case EFlickGeneratedSoundKind::RimImpact:
		{
			const float Strike = (Noise - SmoothedNoise * 0.62f) * FastTransient(Alpha, 86.0f);
			const float RingEnvelope = FMath::Pow(1.0f - Alpha, FMath::Lerp(3.4f, 1.8f, SafeIntensity));
			const float Ring = 0.62f * FMath::Sin(TwoPi * BaseFrequency * Detune * Time)
				+ 0.34f * FMath::Sin(TwoPi * BaseFrequency * 2.71f * Time)
				+ FMath::Lerp(0.08f, 0.25f, SafeTimbre) * FMath::Sin(TwoPi * BaseFrequency * 4.13f * Time);
			Value = Strike * 0.5f + Ring * RingEnvelope;
			break;
		}
		case EFlickGeneratedSoundKind::RingOut:
			Value = (0.66f * FMath::Sin(TwoPi * BaseFrequency * (1.0f - Alpha * 0.68f) * Time)
				+ 0.22f * SmoothedNoise) * (1.0f - Alpha);
			break;
		case EFlickGeneratedSoundKind::Turn:
		{
			const float Note = Alpha < 0.48f ? BaseFrequency : BaseFrequency * 1.25f;
			Value = (FMath::Sin(TwoPi * Note * Time)
				+ 0.18f * FMath::Sin(TwoPi * Note * 2.0f * Time)) * FMath::Sin(PI * Alpha);
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

		if (Kind == EFlickGeneratedSoundKind::Impact || Kind == EFlickGeneratedSoundKind::RingOut)
		{
			Samples[Index] = static_cast<int16>(FMath::Clamp(Value * 24500.0f, -32767.0f, 32767.0f));
		}
		else
		{
			Value *= FMath::Lerp(0.72f, 1.08f, SafeIntensity);
			Samples[Index] = static_cast<int16>(FMath::Clamp(SoftClip(Value) * 27000.0f, -32767.0f, 32767.0f));
		}
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
