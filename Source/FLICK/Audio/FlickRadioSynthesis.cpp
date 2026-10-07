#include "Audio/FlickRadioSynthesis.h"

namespace
{
	constexpr float RadioBpm[] = {108.0f, 92.0f, 124.0f};
	constexpr float RadioRoot[] = {110.0f, 130.8128f, 123.4708f};
	float RadioTone(float Frequency, float Time) { return FMath::Sin(2.0f * PI * Frequency * Time); }
	float RadioNote(float Age, float Frequency, float Decay)
	{
		if (Age < 0 || Age > 2.0f) return 0;
		return (1.0f - FMath::Exp(-Age * 95.0f)) * FMath::Exp(-Age * Decay)
			* (RadioTone(Frequency, Age) + 0.18f * RadioTone(Frequency * 2, Age));
	}
}

const TCHAR* FlickRadioSynthesis::GetTitle(int32 Track)
{
	const TCHAR* Titles[] = {TEXT("AFTER HOURS"), TEXT("SOFT RESET"), TEXT("BREAK POINT")};
	return Titles[FMath::Clamp(Track, 0, TrackCount - 1)];
}

const TCHAR* FlickRadioSynthesis::GetStyle(int32 Track)
{
	const TCHAR* Styles[] = {TEXT("Synthwave"), TEXT("Downtempo"), TEXT("Electronic")};
	return Styles[FMath::Clamp(Track, 0, TrackCount - 1)];
}

float FlickRadioSynthesis::GetDuration(int32 Track)
{
	return Track >= 0 && Track < TrackCount ? 24.0f * 4.0f * 60.0f / RadioBpm[Track] : 0.0f;
}

TArray<int16> FlickRadioSynthesis::Render(int32 Track, float ReviewDuration)
{
	TArray<int16> Samples;
	if (Track < 0 || Track >= TrackCount || !FMath::IsFinite(ReviewDuration) || ReviewDuration < 0) return Samples;
	const float Duration = ReviewDuration > 0 ? FMath::Min(ReviewDuration, GetDuration(Track)) : GetDuration(Track);
	const int32 Frames = FMath::RoundToInt(Duration * SampleRate);
	Samples.SetNumUninitialized(Frames * 2);
	const float Beat = 60.0f / RadioBpm[Track];
	const float Root = RadioRoot[Track];
	constexpr int32 Chords[] = {0, 8, 3, 10}; // minor, VI, III, VII
	constexpr int32 Melody[] = {12, 19, 15, 22, 19, 15, 10, 15, 12, 15, 19, 24, 22, 19, 15, 10};
	uint32 NoiseState = 8191 + Track * 65537;
	float LowNoise = 0;
	for (int32 Frame = 0; Frame < Frames; ++Frame)
	{
		const float Time = static_cast<float>(Frame) / SampleRate;
		const int32 Bar = FMath::FloorToInt(Time / (Beat * 4));
		const float BarAge = FMath::Fmod(Time, Beat * 4);
		const int32 ChordIndex = (Bar / 2) % 4;
		const float ChordRoot = Root * FMath::Pow(2.0f, Chords[ChordIndex] / 12.0f);
		const float Third = ChordIndex == 0 ? 1.189207f : 1.259921f;
		const bool Full = Bar >= 4 && Bar < 20;
		const bool Breakdown = Bar >= 12 && Bar < 16;
		NoiseState = NoiseState * 1664525u + 1013904223u;
		const float Noise = static_cast<float>(NoiseState >> 8) / 8388607.5f - 1.0f;
		LowNoise += (Noise - LowNoise) * 0.06f;
		const float BeatAge = FMath::Fmod(Time, Beat);
		const float Duck = Full && !Breakdown ? 0.65f + 0.35f * FMath::Min(BeatAge / 0.18f, 1.0f) : 1.0f;
		const float PadEnvelope = FMath::Min(BarAge / 0.32f, 1.0f) * FMath::Min((Beat * 4 - BarAge) / 0.32f, 1.0f);
		float Left = 0.055f * PadEnvelope * Duck * (RadioTone(ChordRoot * 2, Time)
			+ RadioTone(ChordRoot * Third * 2, Time) + RadioTone(ChordRoot * 3.002f, Time));
		float Right = 0.055f * PadEnvelope * Duck * (RadioTone(ChordRoot * 2.003f, Time)
			+ RadioTone(ChordRoot * Third * 1.998f, Time) + RadioTone(ChordRoot * 3, Time));
		const float Step = Beat * 0.5f;
		const int32 StepIndex = FMath::FloorToInt(Time / Step);
		const float StepAge = FMath::Fmod(Time, Step);
		// Overlapping lead notes and a quiet opposite-side delay, not a single looping drone.
		for (int32 Tail = 0; Tail < 3; ++Tail)
		{
			const int32 Index = StepIndex - Tail;
			if (Index < 0) continue;
			const float NoteHz = Root * FMath::Pow(2.0f, Melody[(Index + Track * 3) % 16] / 12.0f);
			const float Lead = RadioNote(StepAge + Tail * Step, NoteHz, Track == 1 ? 3.2f : 5.5f)
				* (Full ? 0.13f : 0.08f);
			Left += Lead * (Index % 2 ? 0.65f : 1.0f);
			Right += Lead * (Index % 2 ? 1.0f : 0.65f);
			Right += 0.025f * RadioNote(StepAge + Tail * Step - Beat * 0.75f, NoteHz, 4.5f);
		}
		float Rhythm = 0;
		if (Full && !Breakdown)
		{
			const int32 BeatIndex = FMath::FloorToInt(Time / Beat);
			const float KickAge = Track == 1 ? FMath::Fmod(Time, Beat * 2) : BeatAge;
			Rhythm += 0.30f * RadioTone(48, KickAge) * FMath::Exp(-KickAge * 19)
				* FMath::Min(KickAge / 0.003f, 1.0f);
			if (BeatIndex % 2 == 1)
				Rhythm += 0.12f * (Noise - LowNoise) * FMath::Exp(-BeatAge * 24)
					+ 0.07f * RadioTone(180, BeatAge) * FMath::Exp(-BeatAge * 28);
			Rhythm += 0.035f * (Noise - LowNoise) * FMath::Exp(-StepAge * 90);
			Rhythm += 0.13f * RadioNote(StepAge, ChordRoot * 0.5f, 7);
		}
		const float Fade = FMath::Clamp(Time / 1.0f, 0.0f, 1.0f)
			* FMath::Clamp((Frames - 1 - Frame) / (SampleRate * 1.5f), 0.0f, 1.0f);
		for (int32 Channel = 0; Channel < 2; ++Channel)
		{
			float Value = ((Channel ? Right : Left) + Rhythm) * Fade;
			Value /= 1.0f + FMath::Abs(Value) * 0.4f;
			Samples[Frame * 2 + Channel] = static_cast<int16>(FMath::Clamp(Value * 28000, -29000.0f, 29000.0f));
		}
	}
	return Samples;
}
