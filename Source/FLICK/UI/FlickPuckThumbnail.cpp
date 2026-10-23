#include "UI/FlickPuckThumbnail.h"
#include "Rendering/DrawElements.h"
#include "UI/FlickUITheme.h"
#include "UI/FlickArtworkGeometry.h"

void SFlickPuckThumbnail::Construct(const FArguments& Args)
{
	Archetype = Args._Archetype;
	Skin = Args._Skin;
	View = Args._View;
	CutSize = Args._CutSize;
	FadeRight = Args._FadeRight;
	Zoom = FMath::Max(1.f, Args._Zoom);
	RollDegrees = Args._RollDegrees;
	EdgeFade = Args._EdgeFade;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	SetCanTick(true);
}

void SFlickPuckThumbnail::Tick(const FGeometry&, double, float)
{
	const int32 Type = FMath::Clamp(static_cast<int32>(Archetype.Get()), 0, 8);
	const int32 Finish = FMath::Clamp(Skin.Get(), 0, 5);
	const int32 Key = Type * 6 + Finish;
	if (LoadedKey != Key)
	{
		static const TCHAR* Names[] = {TEXT("Standard"), TEXT("Heavy"), TEXT("Striker"), TEXT("Grippy"),
			TEXT("Slider"), TEXT("Blocker"), TEXT("Compact"), TEXT("Bouncer"), TEXT("Toppler")};
		LoadedKey = Key;
		bReady = false;
		Texture.Reset(LoadObject<UTexture2D>(nullptr, *FString::Printf(
			TEXT("/Game/UI/PuckThumbnails/%sT_Puck_%s_%d.T_Puck_%s_%d"),
			View == EFlickPuckThumbnailView::ProfileCutout ? TEXT("ProfileCutout/") : View == EFlickPuckThumbnailView::Hero ? TEXT("Hero/") : View == EFlickPuckThumbnailView::Locker ? TEXT("Locker/") : View == EFlickPuckThumbnailView::Presentation ? TEXT("Presentation/") : TEXT(""),
			Names[Type], Finish, Names[Type], Finish)));
	}
	if (bReady) return;
#if WITH_EDITOR
	if (Texture.IsValid() && Texture->IsCompiling()) return;
#endif
	if (Texture.IsValid() && !Texture->GetResource()) return;
	Brush.SetResourceObject(Texture.Get());
	bReady = true;
	Invalidate(EInvalidateWidgetReason::Paint);
	if (!Archetype.IsBound() && !Skin.IsBound()) SetCanTick(false);
}

int32 SFlickPuckThumbnail::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, const int32 Layer, const FWidgetStyle& Style, bool) const
{
	if (!bReady || !Texture.IsValid()) return Layer;
	const FVector2D Area = Geometry.GetLocalSize();
	if (Area.X <= 0 || Area.Y <= 0) return Layer;
	if (View != EFlickPuckThumbnailView::Original)
	{
		const float Aspect = Area.X / Area.Y;
		const float SourceAspect = View == EFlickPuckThumbnailView::Locker ? 1.f : 4.f / 3.f;
		FVector2f Min(0, 0), Max(1, 1);
		if (Aspect > SourceAspect) { Min.Y = (1 - SourceAspect / Aspect) * .5f; Max.Y = 1 - Min.Y; }
		else { Min.X = (1 - Aspect / SourceAspect) * .5f; Max.X = 1 - Min.X; }
		const FVector2f HalfUv = (Max - Min) * (.5f / Zoom);
		Min = FVector2f(.5f, .5f) - HalfUv;
		Max = FVector2f(.5f, .5f) + HalfUv;
		const auto Points = FlickUITheme::GetPanelOutline(Area, CutSize);
		const float Angle=FMath::DegreesToRadians(RollDegrees);
		const float Cos=FMath::Cos(Angle), Sin=FMath::Sin(Angle);
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		const float FadeWidth = FMath::Clamp(FadeRight, 0.f, static_cast<float>(Area.X));
		const float FadeStart = Area.X - FadeWidth;
		const auto AppendPolygon = [&](const TArray<FVector2D>& Polygon) {
			const SlateIndex Start = static_cast<SlateIndex>(Vertices.Num());
			for (const FVector2D Point : Polygon)
			{
				FVector2f Uv = Min + FVector2f(Point / Area) * (Max - Min);
				// Optional product-camera roll for the lineup's diagonal portrait.
				// Aspect correction rotates without stretching the circular model.
				const FVector2f P=(Uv-FVector2f(.5f,.5f))*FVector2f(SourceAspect,1);
				Uv=FVector2f(.5f,.5f)+FVector2f(P.X*Cos-P.Y*Sin,P.X*Sin+P.Y*Cos)/FVector2f(SourceAspect,1);
				FLinearColor Tint = Style.GetColorAndOpacityTint();
				if (FadeWidth > 0) Tint.A *= FMath::Clamp((Area.X - Point.X) / FadeWidth, 0., 1.);
				Tint.A *= FlickArtworkGeometry::Opacity(Point,Area,EdgeFade);
				Vertices.Add(FSlateVertex::Make(Geometry.GetAccumulatedRenderTransform(), FVector2f(Point), Uv, Tint.ToFColor(true)));
			}
			for (int32 Index = 1; Index + 1 < Polygon.Num(); ++Index)
				Indices.Append({Start, static_cast<SlateIndex>(Start + Index), static_cast<SlateIndex>(Start + Index + 1)});
		};
		if (EdgeFade != FMargin(0))
		{
			// Profile product portraits blend into their shell, leaving the model opaque.
			FMargin Fade = EdgeFade;
			Fade.Right = FMath::Max(Fade.Right,FadeWidth);
			for (const auto& Polygon : FlickArtworkGeometry::FadePolygons(Points,Area,Fade)) AppendPolygon(Polygon);
		}
		else if (FadeWidth > 0)
		{
			// Split the polygon at the fade boundary, so only the final strip
			// blends into the native card; the puck itself stays fully opaque.
			for (const bool bLeft : {true, false})
			{
				TArray<FVector2D> Clipped;
				for (int32 Index = 0; Index < Points.Num(); ++Index)
				{
					const FVector2D A = Points[Index], B = Points[(Index + 1) % Points.Num()];
					const bool bAInside = bLeft ? A.X <= FadeStart : A.X >= FadeStart;
					const bool bBInside = bLeft ? B.X <= FadeStart : B.X >= FadeStart;
					if (bAInside) Clipped.Add(A);
					if (bAInside != bBInside) Clipped.Add(FMath::Lerp(A, B, (FadeStart - A.X) / (B.X - A.X)));
				}
				AppendPolygon(Clipped);
			}
		}
		else AppendPolygon(Points);
		FSlateDrawElement::MakeCustomVerts(Elements, Layer, Brush.GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);
		return Layer;
	}
	// Trim empty framing margins without distorting the model or changing relative archetype sizes.
	constexpr float Aspect = 4.0f / 3.0f;
	const FVector2D Size = FVector2D(Aspect, 1) * FMath::Min(Area.X / Aspect, Area.Y);
	Brush.SetUVRegion(FBox2f(FVector2f(.04f, .04f), FVector2f(.96f, .96f)));
	FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(Size,
		FSlateLayoutTransform((Area - Size) * .5f)), &Brush, ESlateDrawEffect::None, Style.GetColorAndOpacityTint());
	return Layer;
}
