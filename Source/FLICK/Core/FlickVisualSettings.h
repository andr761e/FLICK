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
	FLICK_API bool IsFpsVisible();
	FLICK_API void SetFpsVisible(bool bVisible);
	// Local interface preferences; never affect selection or replicated gameplay.
	FLICK_API int32 GetPuckHoverSize();
	FLICK_API void SetPuckHoverSize(int32 Size);
	FLICK_API int32 GetPuckHoverDetail(); // 0 off, 1 name, 2 type, 3 name + type.
	FLICK_API void SetPuckHoverDetail(int32 Detail);
	FLICK_API bool IsColorBlindAssistEnabled();
	FLICK_API void SetColorBlindAssistEnabled(bool bEnabled);
	// 0 colour rings, 1 solid ally/dashed enemy, 2 patterns plus relationship labels.
	FLICK_API int32 GetPuckIndicatorStyle();
	FLICK_API void SetPuckIndicatorStyle(int32 Style);
}
