#pragma once

#include "CoreMinimal.h"

// Per-machine options beyond UGameUserSettings' scalability groups.
namespace FlickVisualSettings
{
	FLICK_API int32 GetRenderScale();
	FLICK_API void SetRenderScale(int32 Percent);
	FLICK_API bool IsHardwareLumenEnabled();
	FLICK_API void SetHardwareLumenEnabled(bool bEnabled);
	FLICK_API void ApplySaved();
}
