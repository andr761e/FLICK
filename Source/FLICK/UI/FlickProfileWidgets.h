#pragma once

#include "UI/FlickGameLayerPrivate.h"

namespace FlickProfileWidgets
{
	// Empty-state editorial artwork, not a chart of fictional match results.
	class SStatsIllustration final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SStatsIllustration) {} SLATE_END_ARGS()
		void Construct(const FArguments&) { SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return {208,114}; }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,
			int32 L,const FWidgetStyle& Style,bool) const override
		{
			const FVector2D Size=G.GetLocalSize();
			const FLinearColor Tint=Style.GetColorAndOpacityTint();
			const float Step=(Size.X-18)/7,Bottom=Size.Y-9;
			for (int I=0;I<7;++I)
			{
				const float H=(.17f+I*.13f)*(Size.Y-18),X=10+I*Step,W=Step*.56f;
				FSlateDrawElement::MakeGradient(E,L,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Bottom-H))),
					{FSlateGradientStop(FVector2f(0,0),FlickUITheme::Cyan.CopyWithNewOpacity(.27f)*Tint),
					 FSlateGradientStop(FVector2f(0,H),FlickUITheme::Cyan.CopyWithNewOpacity(.025f)*Tint)},Orient_Vertical);
				const TArray<FVector2D> Cap={{X,Bottom-H},{X+W,Bottom-H}};
				FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),Cap,ESlateDrawEffect::None,FlickUITheme::Ice.CopyWithNewOpacity(.26f)*Tint,true,.7f);
			}
			return L+1;
		}
	};

	// Profile-only, normalized line icons. Never changes gameplay/HUD glyphs.
	class SGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGlyph) : _Kind(0), _Color(FlickUITheme::Paper) {}
			SLATE_ARGUMENT(int32, Kind)
			SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_END_ARGS()
		void Construct(const FArguments& A) { Kind=A._Kind; Color=A._Color; SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return {44,44}; }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,
			int32 L,const FWidgetStyle& Style,bool) const override
		{
			const FVector2D Scale=G.GetLocalSize()/FVector2D(44,44);
			const auto Line=[&](TArray<FVector2D> P,float W=1.7f) {
				for (auto& V:P) V*=Scale;
				FSlateDrawElement::MakeLines(E,L,G.ToPaintGeometry(),P,ESlateDrawEffect::None,Color*Style.GetColorAndOpacityTint(),true,W);
			};
			if (Kind==0)
			{
				Line({{3,39},{41,39}});
				for (int I=0;I<4;++I) { const float X=6+I*9.f,Y=30-I*7.f; Line({{X,36},{X,Y},{X+5,Y},{X+5,36}}); }
			}
			else if (Kind==1)
			{
				Line({{11,5},{33,5},{32,17},{30,23},{26,27},{22,28},{18,27},{14,23},{12,17},{11,5}});
				Line({{11,9},{5,9},{6,16},{9,20},{14,21}}); Line({{33,9},{39,9},{38,16},{35,20},{30,21}});
				Line({{22,28},{22,36},{14,39},{30,39}});
			}
			else if (Kind==2)
			{
				TArray<FVector2D> Arc;
				for (int I=0;I<=40;++I) { const float A=-2.55f+I*5.45f/40; Arc.Add(FVector2D(23,23)+FVector2D(FMath::Cos(A),FMath::Sin(A))*16); }
				Line(Arc); Line({{5,9},{6,19},{16,15}}); Line({{23,12},{23,23},{32,29}});
			}
			else
			{
				// Tapered brush and separate bristle silhouette, not a pencil.
				Line({{15,29},{29,8},{35,3},{40,4},{40,9},{35,15},{23,33},{15,29}});
				Line({{16,27},{24,32}}); Line({{15,31},{19,34},{18,39},{13,42},{5,41},{9,37},{10,33},{15,31}});
			}
			return L;
		}
	private:
		int32 Kind=0;
		FLinearColor Color;
	};

	// Soft contact only; the portrait itself remains the actual puck and finish.
	class SContactShadow final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SContactShadow) {} SLATE_END_ARGS()
		void Construct(const FArguments&) { SetCanTick(false); SetVisibility(EVisibility::HitTestInvisible); }
		virtual FVector2D ComputeDesiredSize(float) const override { return {270,54}; }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,
			int32 L,const FWidgetStyle& Style,bool) const override
		{
			const FVector2D Center=G.GetLocalSize()*.5;
			TArray<FSlateVertex> Vertices; TArray<SlateIndex> Indices;
			constexpr int Rings=8,Steps=40;
			for (int R=0;R<=Rings;++R) for (int S=0;S<=Steps;++S)
			{
				const float Radius=R/float(Rings),Angle=(S%Steps)*UE_TWO_PI/Steps;
				const FVector2D P=Center+Center*FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius;
				const FLinearColor Tint=FLinearColor(0,0,0,FMath::Square(1-Radius)*.5f)*Style.GetColorAndOpacityTint();
				Vertices.Add(FSlateVertex::Make(G.GetAccumulatedRenderTransform(),FVector2f(P),FVector2f::ZeroVector,Tint.ToFColor(true)));
				if (R>0 && S>0) { const SlateIndex V=(R-1)*(Steps+1)+S-1;
					Indices.Append({V,static_cast<SlateIndex>(V+1),static_cast<SlateIndex>(V+Steps+1),
						static_cast<SlateIndex>(V+1),static_cast<SlateIndex>(V+Steps+2),static_cast<SlateIndex>(V+Steps+1)}); }
			}
			FSlateDrawElement::MakeCustomVerts(E,L,WhiteBrush()->GetRenderingResource(),Vertices,Indices,nullptr,0,0);
			return L;
		}
	};
}
