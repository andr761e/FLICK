#include "Audio/FlickSoundSynthesis.h"

namespace
{
	constexpr float Tau = 2.0f * PI;

	float Tone(float Frequency, float Time)
	{
		return FMath::Sin(Tau * Frequency * Time);
	}

	// Each note starts at zero with its own phase and overlaps the preceding tail.
	float Pluck(float Time, float Start, float Frequency, float Decay)
	{
		const float Age = Time - Start;
		if (Age < 0.0f) return 0.0f;
		const float Envelope = (1.0f - FMath::Exp(-Age * 650.0f)) * FMath::Exp(-Age * Decay);
		return Envelope * (Tone(Frequency, Age) + 0.16f * Tone(Frequency * 2.0f, Age)
			* FMath::Exp(-Age * 22.0f));
	}
}

TArray<int16> FlickSoundSynthesis::Render(const EFlickGeneratedSoundKind Kind,
	const float Duration, const float Frequency, const int32 Seed, const float Timbre, const float Intensity)
{
	TArray<int16> Samples;
	if (!FMath::IsFinite(Duration) || !FMath::IsFinite(Frequency)
		|| !FMath::IsFinite(Timbre) || !FMath::IsFinite(Intensity) || Duration <= 0.0f || Frequency <= 0.0f)
	{
		return Samples;
	}
	const float Length = FMath::Min(Duration,
		Kind == EFlickGeneratedSoundKind::ReplayMusic ? 8.0f : 4.0f);
	const int32 Count = FMath::Max(2, FMath::RoundToInt(Length * SampleRate));
	Samples.SetNumUninitialized(Count);
	const float Color = FMath::Clamp(Timbre, 0.0f, 1.0f);
	const float Force = FMath::Clamp(Intensity, 0.0f, 1.0f);
	const bool bPhysical = Kind == EFlickGeneratedSoundKind::Launch || Kind == EFlickGeneratedSoundKind::Impact
		|| Kind == EFlickGeneratedSoundKind::RimImpact || Kind == EFlickGeneratedSoundKind::RingOut;
	uint32 NoiseState = static_cast<uint32>(Seed);
	const float Variation = bPhysical ? 1.0f + (NoiseState % 17u) * 0.001f - 0.008f : 1.0f;
	const float Root = FMath::Clamp(Frequency, 40.0f, 2000.0f) * Variation;
	float LowNoise = 0.0f;
	float AirNoise = 0.0f;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Time = static_cast<float>(Index) / SampleRate;
		const float Alpha = static_cast<float>(Index) / (Count - 1);
		NoiseState = NoiseState * 1664525u + 1013904223u;
		const float Noise = static_cast<float>((NoiseState >> 8) & 0xffffu) / 32767.5f - 1.0f;
		LowNoise += 0.065f * (Noise - LowNoise);
		AirNoise += 0.32f * (Noise - AirNoise);
		const float Air = AirNoise - LowNoise;
		float Value = 0.0f;
		switch (Kind)
		{
		case EFlickGeneratedSoundKind::UiNavigate:
			Value = 0.55f * Pluck(Time, 0.0f, Root, 85.0f)
				+ 0.12f * Air * FMath::Exp(-Time * 180.0f);
			break;
		case EFlickGeneratedSoundKind::UiConfirm:
			Value = 0.43f * Pluck(Time, 0.0f, Root, 32.0f)
				+ 0.38f * Pluck(Time, 0.045f, Root * 1.5f, 30.0f);
			break;
		case EFlickGeneratedSoundKind::Launch:
		{
			// Integrated falling frequency avoids the phase discontinuity of stepped sweeps.
			const float Phase = Tau * Root * (Time - 0.23f * Time * Time / Length);
			Value = 0.68f * FMath::Sin(Phase) * FMath::Exp(-Time * 22.0f)
				+ 0.24f * Pluck(Time, 0.0f, Root * (2.0f + Color), 48.0f)
				+ Air * (0.32f + Color * 0.28f) * FMath::Exp(-Time * 90.0f)
				+ Air * Force * 0.5f * FMath::Sin(PI * Alpha) * FMath::Exp(-Time * 14.0f);
			break;
		}
		case EFlickGeneratedSoundKind::Impact:
			Value = 0.68f * Pluck(Time, 0.0f, Root * (0.85f + Color * 0.3f), 30.0f)
				+ 0.30f * Pluck(Time, 0.0f, Root * (2.3f + Color), 60.0f)
				+ Air * (0.4f + Force * 0.45f) * FMath::Exp(-Time * 125.0f);
			break;
		case EFlickGeneratedSoundKind::RimImpact:
			Value = 0.48f * Pluck(Time, 0.0f, Root, 17.0f)
				+ 0.28f * Pluck(Time, 0.0f, Root * 2.71f, 28.0f)
				+ (0.07f + Color * 0.12f) * Pluck(Time, 0.0f, Root * 4.13f, 45.0f)
				+ 0.5f * Air * FMath::Exp(-Time * 135.0f);
			break;
		case EFlickGeneratedSoundKind::RingOut:
		{
			const float Phase = Tau * Root * (Time - 0.35f * Time * Time / Length);
			Value = 0.5f * FMath::Sin(Phase) * FMath::Exp(-Time * 7.0f)
				+ 0.24f * Pluck(Time, 0.04f, Root * 2.0f, 18.0f)
				+ 0.6f * Air * FMath::Sin(PI * Alpha) * FMath::Exp(-Time * 6.0f);
			break;
		}
		case EFlickGeneratedSoundKind::Turn:
			Value = 0.38f * Pluck(Time, 0.0f, Root, 25.0f)
				+ 0.32f * Pluck(Time, 0.075f, Root * 1.5f, 22.0f);
			break;
		case EFlickGeneratedSoundKind::RoundWin:
			Value = 0.42f * Pluck(Time, 0.0f, Root, 13.0f)
				+ 0.36f * Pluck(Time, 0.105f, Root * 1.25f, 12.0f)
				+ 0.42f * Pluck(Time, 0.21f, Root * 1.5f, 9.0f);
			break;
		case EFlickGeneratedSoundKind::MatchWin:
			Value = 0.38f * Pluck(Time, 0.0f, Root, 11.0f)
				+ 0.32f * Pluck(Time, 0.12f, Root * 1.25f, 10.0f)
				+ 0.36f * Pluck(Time, 0.24f, Root * 1.5f, 9.0f)
				+ 0.42f * Pluck(Time, 0.40f, Root * 2.0f, 6.0f)
				+ 0.22f * Pluck(Time, 0.40f, Root * 0.5f, 5.0f);
			break;
		case EFlickGeneratedSoundKind::ReplayMusic:
		{
			// An intentionally oversized sports-broadcast sting: a low cinematic
			// drone, pulsing minor chord, accelerating arpeggio and noise riser.
			const float BeatLength = FMath::Lerp(0.48f, 0.27f, Alpha);
			const float BeatStart = Time - FMath::Fmod(Time, BeatLength);
			const int32 BeatIndex = FMath::FloorToInt(Time / BeatLength);
			constexpr float Arpeggio[] = {1.0f, 1.5f, 1.2f, 2.0f, 1.5f, 2.4f};
			const float ArpRatio = Arpeggio[BeatIndex % UE_ARRAY_COUNT(Arpeggio)];
			const float Rise = FMath::SmoothStep(0.0f, 1.0f, Alpha);
			const float OpeningImpact = FMath::Exp(-Time * 3.8f);
			const float FinalLift = FMath::SmoothStep(0.72f, 1.0f, Alpha);
			const float PulseAge = Time - BeatStart;
			Value = 0.22f * Tone(Root * 0.5f, Time) * (0.55f + 0.45f * Rise)
				+ 0.15f * Tone(Root * 0.6f, Time) * (0.4f + 0.6f * Rise)
				+ 0.12f * Tone(Root * 0.75f, Time)
				+ 0.28f * Pluck(Time, BeatStart, Root * ArpRatio, 5.2f)
				+ 0.15f * Pluck(Time, BeatStart, Root * ArpRatio * 2.0f, 8.5f)
				+ 0.34f * Tone(Root * 0.25f, Time) * OpeningImpact
				+ 0.18f * Air * Rise
				+ 0.13f * Tone(Root * 3.0f, Time) * FinalLift;
			Value *= FMath::Clamp(Time / 0.32f, 0.0f, 1.0f);
			Value += 0.08f * Air * FMath::Exp(-PulseAge * 18.0f);
			break;
		}
		}
		// Short edge fades prevent clicks; soft limiting retains headroom for overlapping hits.
		const float Fade = FMath::Clamp(Time / 0.0015f, 0.0f, 1.0f)
			* FMath::Clamp((Count - 1 - Index) / (SampleRate * 0.018f), 0.0f, 1.0f);
		Value *= Fade * FMath::Lerp(0.65f, 1.0f, Force);
		Value /= 1.0f + 0.45f * FMath::Abs(Value);
		Samples[Index] = static_cast<int16>(FMath::Clamp(Value * 24500.0f, -28000.0f, 28000.0f));
	}
	return Samples;
}
