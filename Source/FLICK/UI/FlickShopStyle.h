#pragma once

#include "CoreMinimal.h"

// Shared 900-unit frontend design frame. Presentation only; no prices or shop state.
namespace FlickShopStyle
{
	constexpr float Margin = 48;
	constexpr float HeaderHeight = 146;
	constexpr float HeroHeight = 294;
	constexpr float CardHeight = 230;
	constexpr float CardGap = 12;
	inline int32 Columns(float Width) { return Width < 1260 ? 3 : 6; }
	inline float CardWidth(float Width) { return (Width - (Columns(Width) - 1) * CardGap - 2) / Columns(Width); }

	// Fit the complete existing product within a wide banner without stretching it.
	inline FVector2D HeroImageSize(FVector2D Area) { return FVector2D(3, 2) * Area.Y; }
	inline FVector2D HeroImageOffset(FVector2D Area)
	{
		const FVector2D Image = HeroImageSize(Area);
		return FVector2D(Area.X * .67 - Image.X * .66, Area.Y * .5 - Image.Y * .51);
	}
}
