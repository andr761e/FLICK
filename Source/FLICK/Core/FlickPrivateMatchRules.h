#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

namespace FlickPrivateMatchRules
{
	FLICK_API void Normalize(FFlickPrivateMatchSettings& Settings);
	FLICK_API void CycleMode(FFlickPrivateMatchSettings& Settings, int32 Direction);
	FLICK_API int32 GetRequiredPlayingSlots(const FFlickPrivateMatchSettings& Settings);
}
