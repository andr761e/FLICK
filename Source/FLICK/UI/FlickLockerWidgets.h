#pragma once

#include "UI/FlickPlaylistWidgets.h"
#include "UI/FlickLockerStyle.h"

namespace FlickLockerWidgets
{
	// Local glass shading keeps moving arena LEDs/reflections out of the text.
	// It ends before the product column and never darkens the live puck itself.
	class SReadabilityShade final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SReadabilityShade) {} SLATE_END_ARGS()
		void Construct(const FArguments&) { SetVisibility(EVisibility::HitTestInvisible); SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			if (Size.X <= 0 || Size.Y <= 0) return Layer;
			const auto Shade = [&](float Alpha) { return FLinearColor(.001f, .004f, .007f, Alpha * Style.GetColorAndOpacityTint().A); };
			FSlateDrawElement::MakeGradient(Elements, Layer, Geometry.ToPaintGeometry(),
				{FSlateGradientStop(FVector2f(0, 0), Shade(.65f)), FSlateGradientStop(FVector2f(340, 0), Shade(.65f)),
				 FSlateGradientStop(FVector2f(770, 0), Shade(.45f)), FSlateGradientStop(FVector2f(960, 0), Shade(0))}, Orient_Horizontal);
			FSlateDrawElement::MakeGradient(Elements, Layer + 1,
				Geometry.ToPaintGeometry(FVector2D(Size.X, 190), FSlateLayoutTransform()),
				{FSlateGradientStop(FVector2f(0, 0), Shade(.5f)), FSlateGradientStop(FVector2f(0, 190), Shade(0))}, Orient_Vertical);
			return Layer + 1;
		}
	};

	class SCategoryGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCategoryGlyph) : _Category(0), _Color(FlickUITheme::Muted) {}
			SLATE_ARGUMENT(int32, Category)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Category = Args._Category; Color = Args._Color; SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(22, 22); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FLinearColor Tint = Color.Get() * Style.GetColorAndOpacityTint();
			const FVector2D Scale = Geometry.GetLocalSize() / FVector2D(22, 22);
			const auto Line = [&](TArray<FVector2D> Points, float Width = 1.4f) {
				for (auto& Point : Points) Point *= Scale;
				FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Tint, true, Width);
			};
			const auto Circle = [&](FVector2D Center, float Radius) {
				TArray<FVector2D> Points;
				for (int32 Index = 0; Index <= 32; ++Index)
				{ const float Angle = Index * 2.f * PI / 32; Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius); }
				Line(Points);
			};
			if (FlickCosmeticCatalog::IsPuckCategory(Category) || Category == FlickCosmeticCatalog::KnockoutCategory)
			{ Circle({11, 11}, 8); Circle({11, 11}, 4.5f); }
			else if (Category == 0)
			{ Line({{3, 4}, {19, 4}, {19, 18}, {3, 18}, {3, 4}}); Line({{4, 14}, {8, 10}, {12, 14}, {16, 9}, {18, 12}}); }
			else if (Category == 1)
			{ Line({{3, 4}, {19, 4}, {19, 15}, {10, 15}, {5, 19}, {5, 15}, {3, 15}, {3, 4}}); }
			else if (Category == 2)
			{ Circle({11, 7}, 4); Line({{3, 20}, {4, 16}, {7, 13}, {15, 13}, {18, 16}, {19, 20}}); }
			else if (Category == FlickCosmeticCatalog::TrailCategory)
			{ Line({{12, 2}, {4, 12}, {10, 12}, {8, 21}, {18, 9}, {12, 9}, {12, 2}}, 1.8f); }
			else if (Category == FlickCosmeticCatalog::SpawnCategory)
			{
				Circle({11, 11}, 4);
				for (int32 Ray = 0; Ray < 8; ++Ray)
				{ const FVector2D Direction(FMath::Cos(Ray * PI / 4), FMath::Sin(Ray * PI / 4)); Line({FVector2D(11, 11) + Direction * 6, FVector2D(11, 11) + Direction * 10}); }
			}
			else // A mouse body and centre wheel for the rotation hint.
			{ Line({{7, 2}, {15, 2}, {18, 5}, {18, 17}, {15, 20}, {7, 20}, {4, 17}, {4, 5}, {7, 2}}); Line({{11, 2}, {11, 10}}); }
			return Layer;
		}
	private:
		int32 Category = 0;
		TAttribute<FLinearColor> Color;
	};
}
