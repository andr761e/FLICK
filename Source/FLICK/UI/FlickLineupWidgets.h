#pragma once

#include "UI/FlickPlaylistWidgets.h"
#include "UI/FlickLineupStyle.h"

namespace FlickLineupWidgets
{
	// Native meters always use the existing normalized archetype display stats.
	class SStatMeter final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SStatMeter) : _Equipped(.5f), _Preview(.5f), _Comparing(false) {}
			SLATE_ATTRIBUTE(float, Equipped)
			SLATE_ATTRIBUTE(float, Preview)
			SLATE_ATTRIBUTE(bool, Comparing)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Equipped = Args._Equipped; Preview = Args._Preview; Comparing = Args._Comparing; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(200, 9); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			const FLinearColor Tint = Style.GetColorAndOpacityTint();
			const auto Box = [&](int32 Offset, float X, float Width, const FLinearColor Color) {
				FSlateDrawElement::MakeBox(Elements, Layer + Offset,
					Geometry.ToPaintGeometry(FVector2D(FMath::Max(0.f, Width), Size.Y), FSlateLayoutTransform(FVector2D(X, 0))),
					WhiteBrush(), ESlateDrawEffect::None, Color * Tint);
			};
			Box(0, 0, Size.X, FLinearColor(.001f, .005f, .009f));
			const float Baseline = Size.X * FMath::Clamp(Equipped.Get(), 0.f, 1.f);
			if (Comparing.Get()) Box(1, 0, Baseline, FlickUITheme::Muted.CopyWithNewOpacity(.48f));
			Box(2, 0, Size.X * FMath::Clamp(Preview.Get(), 0.f, 1.f), FlickUITheme::Ice);
			if (Comparing.Get()) Box(3, FMath::Max(0.f, Baseline - 1), 2, FlickUITheme::Paper);
			return Layer + 3;
		}
	private:
		TAttribute<float> Equipped, Preview;
		TAttribute<bool> Comparing;
	};

	inline TSharedRef<SWidget> Panel(const TSharedRef<SWidget>& Content, const FMargin Padding = FMargin(18, 14))
	{
		return SNew(SFlickAngularBorder).CutSize(14).Padding(Padding).UseAccentForOutline(true)
			.BackgroundColor(FLinearColor(.003f, .012f, .02f, .94f)).AccentColor(FlickUITheme::Cyan.CopyWithNewOpacity(.48f))[Content];
	}
}
