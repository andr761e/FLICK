#pragma once

#include "UI/FlickPlaylistWidgets.h"

namespace FlickShopWidgets
{
	class SStatusIcon final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SStatusIcon) : _Coins(false) {} SLATE_ARGUMENT(bool, Coins) SLATE_END_ARGS()
		void Construct(const FArguments& Args) { bCoins = Args._Coins; SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(36, 36); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Scale = Geometry.GetLocalSize() / FVector2D(36, 36);
			const FLinearColor Color = (bCoins ? FlickUITheme::Brand : FlickUITheme::Paper) * Style.GetColorAndOpacityTint();
			const auto Line = [&](TArray<FVector2D> Points) {
				for (auto& Point : Points) Point *= Scale;
				FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, 1.5f);
			};
			const auto Ellipse = [&](FVector2D Centre, float Rx, float Ry) {
				TArray<FVector2D> Points;
				for (int32 I = 0; I <= 32; ++I) { const float A = I * 2 * PI / 32; Points.Add(Centre + FVector2D(FMath::Cos(A) * Rx, FMath::Sin(A) * Ry)); }
				Line(Points);
			};
			if (bCoins)
			{
				for (int32 I = 0; I < 3; ++I) { const float Y = 29 - I * 5; Ellipse({13, Y}, 10, 3.5f); }
				Line({{3, 19}, {3, 29}}); Line({{23, 19}, {23, 29}});
				for (int32 I = 0; I < 3; ++I) { const float Y = 16 - I * 5; Ellipse({23, Y}, 10, 3.5f); }
				Line({{13, 6}, {13, 16}}); Line({{33, 6}, {33, 16}});
			}
			else { Line({{2, 18}, {9, 11}, {18, 8}, {27, 11}, {34, 18}, {27, 25}, {18, 28}, {9, 25}, {2, 18}}); Ellipse({18, 18}, 5, 5); }
			return Layer;
		}
	private:
		bool bCoins = false;
	};
}
