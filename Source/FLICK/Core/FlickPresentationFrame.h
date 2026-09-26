#pragma once

#include "CoreMinimal.h"

// Keep ordinary displays responsive. Beyond these limits the game is shown in
// a centered presentation frame, with black bars filling the unused area.
namespace FlickPresentationFrame
{
	constexpr float MinimumAspect = 4.0f / 3.0f;
	// Includes common 3440x1440 ultrawide displays while still capping 32:9.
	constexpr float MaximumAspect = 5.0f / 2.0f;

	inline FVector2D GetContainedSize(const FVector2D ViewportSize)
	{
		if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f) return ViewportSize;
		const float Aspect = ViewportSize.X / ViewportSize.Y;
		if (Aspect < MinimumAspect) return FVector2D(ViewportSize.X, ViewportSize.X / MinimumAspect);
		if (Aspect > MaximumAspect) return FVector2D(ViewportSize.Y * MaximumAspect, ViewportSize.Y);
		return ViewportSize;
	}
}
