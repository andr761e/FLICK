#pragma once

#include "CoreMinimal.h"

// Keep ordinary displays responsive. Beyond these limits the game is shown in
// a centered presentation frame, with black bars filling the unused area.
namespace FlickPresentationFrame
{
	constexpr float MinimumAspect = 4.0f / 3.0f;
	// Includes common 3440x1440 ultrawide displays while still capping 32:9.
	constexpr float MaximumAspect = 5.0f / 2.0f;
	// Input must remain in Slate local units. Mixing physical pixels into this
	// value applies Unreal/Windows DPI a second time at resolutions below 1080p.
	inline float GetDesignScale(FVector2D LocalSize)
	{
		return FMath::Max(.25f, static_cast<float>(FMath::Min(LocalSize.X / 1600.0, LocalSize.Y / 900.0)));
	}

	inline FVector2D GetContainedSize(const FVector2D ViewportSize)
	{
		if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f) return ViewportSize;
		const float Aspect = ViewportSize.X / ViewportSize.Y;
		if (Aspect < MinimumAspect) return FVector2D(ViewportSize.X, ViewportSize.X / MinimumAspect);
		if (Aspect > MaximumAspect) return FVector2D(ViewportSize.Y * MaximumAspect, ViewportSize.Y);
		return ViewportSize;
	}
}
