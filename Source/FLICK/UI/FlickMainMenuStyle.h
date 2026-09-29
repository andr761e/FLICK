#pragma once

#include "CoreMinimal.h"
#include "UI/FlickUITheme.h"

// Home-specific layout. The palette and panel framing are shared by every UI.
namespace FlickMainMenuStyle
{
	inline const FLinearColor Lime = FlickUITheme::Brand;
	inline const FLinearColor Ice = FlickUITheme::Ice;
	constexpr float LeftPadding = 52.0f;
	constexpr float NavigationTop = 256.0f;

	namespace Navigation
	{
		constexpr float PrimaryHeight = 92.0f;
		constexpr float SecondaryHeight = 62.0f;
		constexpr float Gap = 10.0f;
		constexpr float FirstWidth = 460.0f;
		constexpr float RightEdgeSlope = 0.075f;

		constexpr float GetRowTop(const int32 Index)
		{
			return Index <= 0 ? 0.0f : PrimaryHeight + Gap + (Index - 1) * (SecondaryHeight + Gap);
		}
		constexpr float GetRowHeight(const int32 Index) { return Index == 0 ? PrimaryHeight : SecondaryHeight; }
		constexpr float GetRowLeft(const int32 Index) { return 0.0f; }
		constexpr float GetRowWidth(const int32 Index)
		{
			return FirstWidth + (GetRowTop(Index) + GetRowHeight(Index) - PrimaryHeight) * RightEdgeSlope;
		}
		constexpr float StackWidth = GetRowWidth(5);
	}

	inline TArray<FVector2D> GetCardOutline(const FVector2D Size)
	{
		const float Cut = FMath::Min(8.0f, Size.Y * 0.2f);
		const float Sweep = Size.Y * Navigation::RightEdgeSlope;
		return {{Cut, 0.0f}, {Size.X - Sweep - Cut, 0.0f}, {Size.X - Sweep, Cut},
			{Size.X, Size.Y - Cut}, {Size.X - Cut, Size.Y}, {Cut, Size.Y},
			{0.0f, Size.Y - Cut}, {0.0f, Cut}};
	}

	inline float GetDiagonalBottomEdgeX(const FVector2D Size, const float ColumnScale)
	{
		return FMath::Min(Size.X, Size.Y * (16.0f / 9.0f)) * 0.410f * ColumnScale;
	}

	inline float GetDiagonalTopEdgeX(const FVector2D Size, const float ColumnScale)
	{
		const float DesignWidth = FMath::Min(Size.X, Size.Y * (16.0f / 9.0f));
		const float Bottom = GetDiagonalBottomEdgeX(Size, ColumnScale);
		// A steeper diagonal matches the art direction. On taller supported
		// frames retain enough glass behind PLAY, including its outer glow.
		const float PlayBottom = NavigationTop + Navigation::PrimaryHeight * ColumnScale;
		const float Alpha = FMath::Clamp(PlayBottom / FMath::Max(1.0, Size.Y), 0.0f, 0.9f);
		const float MinimumTop = (LeftPadding + Navigation::FirstWidth * ColumnScale + 22.0f - Bottom * Alpha) / (1.0f - Alpha);
		return FMath::Max(DesignWidth * 0.292f * ColumnScale, MinimumTop);
	}
}
