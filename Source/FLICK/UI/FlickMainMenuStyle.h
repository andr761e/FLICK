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
	// One right-hand grid: header, challenges and radio share the same inset.
	constexpr float RightInset = 34.0f;
	constexpr float RightColumnWidth = 390.0f;
	constexpr float StatusWidth = 650.0f;
	constexpr float StatusHeight = 82.0f;
	constexpr float StatusTop = 32.0f;
	constexpr float RadioHeight = 150.0f;
	constexpr float RadioBottom = 48.0f;
	constexpr float SelectorBottom = 86.0f;
	inline float GetVerticalInset(FVector2D CanvasSize)
	{
		return FMath::Max(0.f, static_cast<float>((CanvasSize.Y - 900.0) * .5));
	}
	constexpr double RankDisplaySeconds = 5.0;
	inline int32 GetRankPlaylist(const double VisibleSeconds)
	{
		return FMath::FloorToInt(FMath::Fmod(FMath::Max(0.0, VisibleSeconds), RankDisplaySeconds * 4) / RankDisplaySeconds);
	}

	namespace Navigation
	{
		constexpr float PrimaryHeight = 92.0f;
		constexpr float SecondaryHeight = 62.0f;
		constexpr float Gap = 10.0f;
		constexpr float FirstWidth = 410.0f;
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

	inline FVector2D GetDiagonalEdges(const FVector2D Size, const float ColumnScale)
	{
		const float DesignHeight = FMath::Min(Size.Y, 900.0);
		const float DesignWidth = FMath::Min(Size.X, DesignHeight * (16.0f / 9.0f));
		const float Bottom = DesignWidth * .355f * ColumnScale;
		const float PlayBottom = NavigationTop + Navigation::PrimaryHeight * ColumnScale;
		const float Alpha = FMath::Clamp(PlayBottom / FMath::Max(1.f, DesignHeight), 0.0f, 0.9f);
		const float MinimumTop = (LeftPadding + Navigation::FirstWidth * ColumnScale + 22.0f - Bottom * Alpha) / (1.0f - Alpha);
		const float Top = FMath::Max(DesignWidth * .250f * ColumnScale, MinimumTop);
		// Extend the same diagonal above/below the centred composition in 16:10
		// and 4:3 windows, instead of changing its relationship to the buttons.
		const float Extension = (Bottom - Top) * GetVerticalInset(Size) / FMath::Max(1.f, DesignHeight);
		return FVector2D(Top - Extension, Bottom + Extension);
	}
	inline float GetDiagonalBottomEdgeX(FVector2D Size, float Scale) { return GetDiagonalEdges(Size, Scale).Y; }
	inline float GetDiagonalTopEdgeX(FVector2D Size, float Scale) { return GetDiagonalEdges(Size, Scale).X; }
}
