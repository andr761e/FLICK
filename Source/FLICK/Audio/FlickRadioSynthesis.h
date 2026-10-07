#pragma once
#include "CoreMinimal.h"

// Original instrumental arrangements. Pure PCM generation; safe on a worker thread.
namespace FlickRadioSynthesis
{
	constexpr int32 TrackCount = 3;
	constexpr int32 SampleRate = 24000;
	const TCHAR* GetTitle(int32 Track);
	const TCHAR* GetStyle(int32 Track);
	float GetDuration(int32 Track);
	TArray<int16> Render(int32 Track, float ReviewDuration = 0.0f);
}
