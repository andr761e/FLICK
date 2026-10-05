#pragma once

#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickTrailStyle.h"
#include "Core/FlickSpawnStyle.h"
#include "Core/FlickKnockoutStyle.h"
#include "Core/FlickVisualSettings.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

// Small, bounded native previews: no render targets, extra scenes or asset dependencies.
class SFlickCosmeticSwatch final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickCosmeticSwatch) : _Category(2), _Item(0) {}
		SLATE_ATTRIBUTE(int32, Category)
		SLATE_ATTRIBUTE(int32, Item)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args)
	{
		Category = Args._Category; Item = Args._Item;
		SetCanTick(false); ForceVolatile(true);
	}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(110.0f, 88.0f); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Cull,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& WidgetStyle, bool bEnabled) const override
	{
		const int32 Selected = Item.Get();
		const int32 Group = FlickCosmeticCatalog::GetCollection(Category.Get(), Selected);
		if (Category.Get() == 2 && Group == INDEX_NONE) return Layer;
		const FLinearColor Accent = (Category.Get() == FlickCosmeticCatalog::TrailCategory ? FlickTrailStyle::Get(Selected).Primary
			: Category.Get() == FlickCosmeticCatalog::SpawnCategory ? FlickSpawnStyle::Get(Selected).Primary
			: Category.Get() == FlickCosmeticCatalog::KnockoutCategory ? FlickKnockoutStyle::Get(Selected).Primary
			: Group != INDEX_NONE ? FlickCosmeticCatalog::GetCollectionColor(Group)
			: Selected == 2 ? FLinearColor(1.0f, 0.3f, 0.04f) : Selected == 1 ? FLinearColor(0.02f, 0.7f, 1.0f)
			: FLinearColor(0.38f, 0.47f, 0.52f)) * WidgetStyle.GetColorAndOpacityTint();
		const FVector2D Size = Geometry.GetLocalSize();
		const FVector2D Center = Size * 0.5f;
		const double Time = FPlatformTime::Seconds();
		const auto Line = [&](const TArray<FVector2D>& Points, const FLinearColor& Color, float Width = 1.4f)
		{
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
				ESlateDrawEffect::None, Color.CopyWithNewOpacity(Color.A * 0.07f), true, Width + 6.0f);
			FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Points,
				ESlateDrawEffect::None, Color, true, Width);
		};
		const auto Arc = [&](FVector2D Origin, float Radius, float Start, float End, FLinearColor Color, float Width = 1.4f, float Flatten = 1.0f)
		{
			TArray<FVector2D> Points;
			for (int32 Index = 0; Index <= 48; ++Index)
			{
				const float Angle = FMath::Lerp(Start, End, Index / 48.0f);
				Points.Add(Origin + FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius * Flatten));
			}
			Line(Points, Color, Width);
		};
		if (Category.Get() == 2)
		{
			// The central avatar remains untouched; ornaments live in the outer eight pixels.
			const float X = Size.X, Y = Size.Y;
			Line({{10, 3}, {X - 10, 3}, {X - 3, 10}, {X - 3, Y - 10}, {X - 10, Y - 3},
				{10, Y - 3}, {3, Y - 10}, {3, 10}, {10, 3}}, Accent.CopyWithNewOpacity(0.72f), 2.0f);
			Line({{12, 6}, {X - 13, 6}}, FLinearColor(0.82f, 0.89f, 0.95f, 0.58f), 0.8f);
			if (Group == 0)
			{
				for (int32 Side = 0; Side < 2; ++Side)
				{
					const float Edge = Side == 0 ? 6.0f : X - 6.0f;
					Line({{Edge, 14}, {Edge, Y * 0.34f}, {Edge + (Side ? -4 : 4), Y * 0.4f}}, Accent, 2.0f);
					Line({{Edge, Y - 14}, {Edge, Y * 0.68f}}, Accent, 2.0f);
					Line({{Edge - 2, Y * 0.53f}, {Edge, Y * 0.5f}, {Edge + 2, Y * 0.53f}}, FLinearColor::White, 1.5f);
				}
			}
			else if (Group == 1)
			{
				for (int32 Index = 0; Index < 3; ++Index)
				{
					const float H = Y * (0.4f + Index * 0.15f);
					Line({{3, H + 4}, {7, H}, {10, H + 3}}, Accent, 1.8f);
					Line({{X - 3, H + 4}, {X - 7, H}, {X - 10, H + 3}}, Accent, 1.8f);
				}
				Line({{X * 0.4f, Y - 4}, {X * 0.44f, Y - 8}, {X * 0.5f, Y - 5}, {X * 0.56f, Y - 8}, {X * 0.6f, Y - 4}}, Accent, 2.0f);
			}
			else
			{
				Line({{3, 19}, {3, 10}, {10, 3}, {21, 3}}, FLinearColor(0.04f, 0.9f, 1.0f), 2.4f);
				Line({{X - 3, Y - 21}, {X - 3, Y - 10}, {X - 10, Y - 3}, {X - 21, Y - 3}}, Accent, 2.4f);
				Line({{X - 21, 7}, {X - 11, 7}, {X - 7, 11}, {X - 7, 21}}, Accent.CopyWithNewOpacity(0.5f), 1.0f);
			}
			return Layer + 2;
		}
		const float Radius = FMath::Min(Size.X * 0.27f, Size.Y * 0.32f);
		if (Category.Get() == FlickCosmeticCatalog::TrailCategory)
		{
			if (Selected == 0)
			{
				Arc(Center, 13.0f, 0.0f, 2.0f * PI, Accent, 1.4f);
				Line({Center - FVector2D(9, 9), Center + FVector2D(9, 9)}, Accent, 1.4f);
				return Layer + 2;
			}
			const auto& Trail = FlickTrailStyle::Get(Selected);
			for (int32 Lane = 0; Lane < Trail.Lanes; ++Lane)
			{
				TArray<FVector2D> Points;
				for (int32 Index = 0; Index < 36; ++Index)
				{
					const float T = Index / 35.0f;
					float Y = (Lane - (Trail.Lanes - 1) * .5f) * (Selected == 7 ? 3 : 2.2f);
					if (Selected == 2 || Selected == 4 || Selected == 5 || Selected == 9 || Selected == 12)
						Y += FMath::Sin(T * 10 - Time * 2 + Lane * 2) * 4 * (1-T);
					if (Selected == 6) Y += FMath::Sin(Index * 39 + Lane * 7 + FMath::FloorToFloat(Time * 10)) * 4 * (1-T);
					Points.Add(FVector2D(Size.X * (0.1f + 0.66f * T), Center.Y + Y));
				}
				const FLinearColor Color = Selected == 7 ? FlickTrailStyle::Rainbow(Lane / 7.0f)
					: FMath::Lerp(Accent, Trail.Secondary, Lane / float(Trail.Lanes - 1));
				Line(Points, Color.CopyWithNewOpacity(Selected == 9 || Selected == 12 ? .5f : .8f),
					Lane == 0 ? (Selected == 9 || Selected == 12 ? 7.f : 3.f) : 1.f);
			}
			Arc(FVector2D(Size.X * 0.78f, Center.Y), 9.0f, 0.0f, 2.0f * PI, Accent, 2.0f, 0.66f);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FVector2D P(Size.X * (.14f + Index*.09f), Center.Y + (Index % 2 ? -1 : 1) * (9 + Index % 3 * 3));
				const FLinearColor Color = Selected == 7 ? FlickTrailStyle::Rainbow(Index / 6.0f) : Trail.Secondary;
				if (Selected == 11)
					Line({P + FVector2D(0,3), P + FVector2D(-3,0), P + FVector2D(-3,-2), P + FVector2D(-1,-3),
						P, P + FVector2D(1,-3), P + FVector2D(3,-2), P + FVector2D(3,0), P + FVector2D(0,3)}, Color, 1.f);
				else if (Selected == 10)
					Line({P, P + FVector2D(3,0), P + FVector2D(3,3), P + FVector2D(0,3), P}, Color, 2.f);
				else if (Selected == 3)
					Line({P + FVector2D(0,-3), P + FVector2D(2,0), P + FVector2D(0,3), P + FVector2D(-2,0), P + FVector2D(0,-3)}, Color, 1.f);
				else
				{
					Line({P - FVector2D(2,0), P + FVector2D(2,0)}, Color, 1.f);
					if (Selected == 8 || Selected == 9) Line({P - FVector2D(0,3), P + FVector2D(0,3)}, Color, 1.f);
				}
			}
		}
		else if (Category.Get() == FlickCosmeticCatalog::SpawnCategory)
		{
			using EPattern = FlickSpawnStyle::EPattern;
			const auto& Spawn = FlickSpawnStyle::Get(Selected);
			const FVector2D Base(Center.X, Size.Y*.76f);
			if (Selected == 0)
			{
				Arc(Base, Radius*.5f, 0, 2*PI, Accent, 2.f, .4f);
				Line({Base-FVector2D(0,30),Base-FVector2D(0,12)},Accent,1.5f);
				return Layer+2;
			}
			Arc(Base, Radius, 0, 2*PI, Accent, 1.5f, .3f);
			Arc(Base, Radius*.6f, 0, 2*PI, Spawn.Secondary, 2.f, .3f);
			if (Spawn.Pattern == EPattern::Galaxy)
			{
				const FVector2D Origin = Base-FVector2D(0,28);
				for (int32 Arm = 0; Arm < 3; ++Arm)
				{
					TArray<FVector2D> Points;
					for (int32 I = 0; I < 36; ++I)
					{
						const float T = I/35.f, A = T*7 + Arm*2*PI/3 + Time*.5f;
						Points.Add(Origin+FVector2D(FMath::Cos(A),FMath::Sin(A)*.65f)*(3+T*Radius));
					}
					Line(Points,FMath::Lerp(Accent,Spawn.Secondary,Arm/2.f),1.8f);
				}
				Line({Origin-FVector2D(0,5),Origin+FVector2D(0,5)},Spawn.Secondary,1.2f);
				Line({Origin-FVector2D(5,0),Origin+FVector2D(5,0)},Accent,1.2f);
				return Layer+2;
			}
			for (int32 I = 0; I < 5; ++I)
			{
				const float Phase = I/5.f;
				const FLinearColor C = Spawn.Pattern == EPattern::Prism ? FlickSpawnStyle::Spectrum(Phase)
					: FMath::Lerp(Accent,Spawn.Secondary,Phase);
				if (Spawn.Pattern == EPattern::Ice)
				{
					const FVector2D P = Base+FVector2D((I-2)*9,0);
					Line({P-FVector2D(4,0), P-FVector2D(0,22+I%3*10),P+FVector2D(4,0)},C,1.8f);
				}
				else if (Spawn.Pattern == EPattern::Lightning)
				{
					const FVector2D P = Base+FVector2D((I-2)*9,0);
					Line({P,P+FVector2D(-5,-12),P+FVector2D(3,-22),P+FVector2D(-2,-34),
						Base-FVector2D(0,49)},C,1.4f);
				}
				else if (Spawn.Pattern == EPattern::Fire || Spawn.Pattern == EPattern::Spirit || Spawn.Pattern == EPattern::Sparks)
				{
					TArray<FVector2D> Points;
					for (int32 J = 0; J < 24; ++J)
					{
						const float T = J/23.f;
						Points.Add(Base+FVector2D(FMath::Sin(T*9+I*1.3f-Time*2)*Radius*(1-T)*.65f,-T*48));
					}
					Line(Points,C.CopyWithNewOpacity(.7f),I%2 ? 1.f : 2.5f);
				}
				else if (Spawn.Pattern == EPattern::Portal || Spawn.Pattern == EPattern::Pulse)
				{
					Arc(Base-FVector2D(0,8+I*9),Radius*(1-I*.12f),Time+I,Time+I+PI*1.8f,C,1.2f,.3f);
				}
				else if (Spawn.Pattern == EPattern::Hearts || Spawn.Pattern == EPattern::Pixel)
				{
					const FVector2D P = Base+FVector2D((I-2)*10,-12-I%3*12);
					if (Spawn.Pattern == EPattern::Hearts)
						Line({P+FVector2D(0,4),P+FVector2D(-4,0),P+FVector2D(-4,-3),P+FVector2D(-2,-4),
							P,P+FVector2D(2,-4),P+FVector2D(4,-3),P+FVector2D(4,0),P+FVector2D(0,4)},C,1.2f);
					else Line({P,P+FVector2D(4,0),P+FVector2D(4,-4),P+FVector2D(0,-4),P},C,2.f);
				}
				else
				{
					const FVector2D P = Base+FVector2D((I-2)*9,0);
					Line({P,P-FVector2D(0,45)},C.CopyWithNewOpacity(.65f),I%2 ? 1.f : 2.f);
				}
			}
		}
		else
		{
			using EPattern = FlickKnockoutStyle::EPattern;
			const auto& Look = FlickKnockoutStyle::Get(Selected);
			const EPattern Pattern = Look.Pattern;
			Arc(Center+FVector2D(0,13),Radius,0,2*PI,Accent.CopyWithNewOpacity(.5f),1.5f,.3f);
			if (Pattern == EPattern::Galaxy || Pattern == EPattern::Vortex || Pattern == EPattern::Arcane)
			{
				for (int32 Arm = 0; Arm < 3; ++Arm)
				{
					TArray<FVector2D> Points;
					for (int32 I = 0; I < 32; ++I)
					{
						const float T = I/31.f, A = T*7+Arm*2*PI/3-Time;
						Points.Add(Center+FVector2D(FMath::Cos(A),FMath::Sin(A)*.65f)*(3+T*Radius));
					}
					Line(Points,FMath::Lerp(Accent,Look.Secondary,Arm/2.f),2.f);
				}
			}
			else for (int32 I = 0; I < 10; ++I)
			{
				const float A = I*2*PI/10;
				const FVector2D D(FMath::Cos(A),FMath::Sin(A)*.8f), Side(-D.Y,D.X);
				const FVector2D P = Center+D*Radius;
				const FLinearColor C = Pattern == EPattern::Prism || Pattern == EPattern::Confetti
					|| Pattern == EPattern::Hologram ? FlickKnockoutStyle::Spectrum(I/10.f)
					: FMath::Lerp(Accent,Look.Secondary,I/10.f);
				if (Pattern == EPattern::Lightning)
					Line({Center+D*3,Center+D*Radius*.4f+Side*4,Center+D*Radius*.7f-Side*3,P},C,1.8f);
				else if (Pattern == EPattern::Ice || Pattern == EPattern::Hologram)
					Line({P-D*6,P+Side*3,P+D*6,P-Side*3,P-D*6},C,1.5f);
				else if (Pattern == EPattern::Hearts)
					Line({P+FVector2D(0,4),P+FVector2D(-4,0),P+FVector2D(-4,-3),P+FVector2D(-2,-4),
						P,P+FVector2D(2,-4),P+FVector2D(4,-3),P+FVector2D(4,0),P+FVector2D(0,4)},C,1.3f);
				else if (Pattern == EPattern::Pixel || Pattern == EPattern::Confetti)
					Line({P,P+FVector2D(4,0),P+FVector2D(4,4),P+FVector2D(0,4),P},C,1.8f);
				else if (Pattern == EPattern::Toxic || Pattern == EPattern::Water || Pattern == EPattern::Smoke)
					Arc(P,3+I%3,0,2*PI,C.CopyWithNewOpacity(.7f),1.4f);
				else if (Pattern == EPattern::Spirit || Pattern == EPattern::Fire)
				{
					TArray<FVector2D> Points;
					for (int32 J = 0; J < 20; ++J)
					{
						const float T = J/19.f;
						Points.Add(Center+D*(3+Radius*T)+Side*FMath::Sin(T*8-Time*2)*3);
					}
					Line(Points,C,2.f);
				}
				else Line({Center+D*Radius*.2f,P},C,I%2 ? 1.f : 2.4f);
			}
		}
		return Layer + 2;
	}
private:
	TAttribute<int32> Category, Item;
};
