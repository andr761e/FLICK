#pragma once

#include "CoreMinimal.h"

enum class EFlickGeneratedSoundKind : uint8
{
	UiNavigate, UiConfirm, Launch, Impact, RimImpact, RingOut, Turn, RoundWin, MatchWin, ReplayMusic
};

// Pure PCM rendering: independent of world state and gameplay random streams.
namespace FlickSoundSynthesis
{
	constexpr int32 SampleRate = 48000;
	TArray<int16> Render(EFlickGeneratedSoundKind Kind, float Duration, float Frequency,
		int32 Seed, float Timbre = 0.5f, float Intensity = 1.0f);
}
