#pragma once
#include "UI/FlickPlaylistWidgets.h"

namespace FlickSettingsWidgets
{
	// Small native line glyphs; all other categories reuse the existing menu icon set.
	class SGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGlyph) : _Kind(0), _Color(FlickUITheme::Paper) {}
			SLATE_ARGUMENT(int32, Kind)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Kind = Args._Kind; Color = Args._Color; SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(30, 30); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& E,
			int32 L, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Scale = G.GetLocalSize() / FVector2D(30, 30);
			const FLinearColor Tint = Color.Get() * Style.GetColorAndOpacityTint();
			const auto Line = [&](TArray<FVector2D> P, float W = 1.7f) { for (auto& V : P) V *= Scale; FSlateDrawElement::MakeLines(E, L, G.ToPaintGeometry(), P, ESlateDrawEffect::None, Tint, true, W); };
			const auto Arc = [&](float R, float A, float B) { TArray<FVector2D> P; for (int I = 0; I <= 32; ++I) { const float T = FMath::Lerp(A, B, I / 32.f); P.Add(FVector2D(15,15) + FVector2D(FMath::Cos(T), FMath::Sin(T)) * R); } Line(P); };
			if (Kind == 0) // Crosshair.
			{ Arc(10, 0, 2*PI); Line({{15,1},{15,9}}); Line({{15,21},{15,29}}); Line({{1,15},{9,15}}); Line({{21,15},{29,15}}); Arc(1,0,2*PI); }
			else if (Kind == 1) // Broadcast.
			{ Arc(2,0,2*PI); for (float R : {7.f,12.f}) { Arc(R,-.85f,.85f); Arc(R,PI-.85f,PI+.85f); } }
			else if (Kind == 2) // Video.
			{ Line({{2,4},{28,4},{28,26},{2,26},{2,4}}); Line({{12,9},{21,15},{12,21},{12,9}}); }
			else if (Kind == 3) // Sun.
			{ Arc(6,0,2*PI); for (int I=0;I<8;++I) { const FVector2D D(FMath::Cos(I*PI/4),FMath::Sin(I*PI/4)); Line({FVector2D(15,15)+D*10,FVector2D(15,15)+D*14}); } }
			else // Information.
			{ Arc(12,0,2*PI); Arc(1,0,2*PI); Line({{15,13},{15,23}}); Line({{14,8},{16,8}},2); }
			return L;
		}
	private:
		int32 Kind = 0;
		TAttribute<FLinearColor> Color;
	};
}
