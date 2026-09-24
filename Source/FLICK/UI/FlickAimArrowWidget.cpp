#include "UI/FlickAimArrowWidget.h"

#include "Rendering/DrawElements.h"
#include "UI/FlickHUD.h"

void SFlickAimArrowWidget::Construct(const FArguments& InArgs)
{
	OwnerHud = InArgs._OwnerHud;
	SetCanTick(false);
	ForceVolatile(true); // Aim positions change every frame while dragging.
}

FVector2D SFlickAimArrowWidget::ComputeDesiredSize(const float LayoutScaleMultiplier) const
{
	return FVector2D(1600.0f, 900.0f);
}

int32 SFlickAimArrowWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
	const AFlickHUD* Hud = OwnerHud.Get();
	if (!Hud)
	{
		return LayerId;
	}
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
	for (const FFlickAimArrowVisual& Arrow : Hud->GetAimArrows())
	{
		if (Arrow.CanvasSize.X <= 0.0f || Arrow.CanvasSize.Y <= 0.0f)
		{
			continue;
		}
		const FVector2D Scale(LocalSize.X / Arrow.CanvasSize.X, LocalSize.Y / Arrow.CanvasSize.Y);
		const float StrokeScale = FMath::Max(0.35f, FMath::Min(Scale.X, Scale.Y));
		const auto ToLocal = [Scale](const FVector2D& Point)
		{
			return FVector2D(Point.X * Scale.X, Point.Y * Scale.Y);
		};
		const FVector2D Vector = Arrow.End - Arrow.Start;
		const float Length = Vector.Size();
		if (Length < 18.0f)
		{
			continue;
		}
		const FVector2D Direction = Vector / Length;
		const FVector2D Side(-Direction.Y, Direction.X);
		const FVector2D RailStart = Arrow.Start + Direction * FMath::Min(25.0f, Length * 0.16f);
		const FVector2D HeadBase = Arrow.End - Direction * FMath::Min(FMath::Clamp(Length * 0.14f, 22.0f, 36.0f), Length * 0.4f);
		const float RailLength = FVector2D::Distance(RailStart, HeadBase);
		const FLinearColor Accent = Arrow.Accent * Tint;
		const FLinearColor BrightAccent = FMath::Lerp(Arrow.Accent, FLinearColor::White, 0.22f) * Tint;
		const auto Stroke = [&](const TArray<FVector2D>& Points, const FLinearColor& Color, const float Width, const int32 Layer)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + Layer, AllottedGeometry.ToPaintGeometry(),
				Points, ESlateDrawEffect::None, Color, true, Width * StrokeScale);
		};

		for (const float RailSide : {-1.0f, 1.0f})
		{
			const FVector2D Offset = Side * 6.0f * RailSide;
			Stroke({ToLocal(RailStart + Offset), ToLocal(HeadBase + Offset)}, Accent.CopyWithNewOpacity(0.38f), 1.3f, 0);
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float A = static_cast<float>(Index) / 6.0f;
			const float B = FMath::Min(A + 0.72f / 6.0f, 1.0f);
			const TArray<FVector2D> Segment = {ToLocal(RailStart + Direction * RailLength * A),
				ToLocal(RailStart + Direction * RailLength * B)};
			const float Opacity = FMath::Lerp(0.52f, 0.96f, B);
			Stroke(Segment, FLinearColor(0.0f, 0.006f, 0.012f, 0.9f) * Tint, 8.0f, 0);
			Stroke(Segment, Accent.CopyWithNewOpacity(Opacity), 3.5f, 1);
			Stroke(Segment, BrightAccent.CopyWithNewOpacity(Opacity * 0.72f), 1.0f, 2);
		}

		const FVector2D Left = HeadBase + Side * 17.0f;
		const FVector2D Right = HeadBase - Side * 17.0f;
		Stroke({ToLocal(Left), ToLocal(Arrow.End), ToLocal(Right)},
			FLinearColor(0.0f, 0.006f, 0.012f, 0.92f) * Tint, 7.0f, 0);
		Stroke({ToLocal(Left), ToLocal(Arrow.End), ToLocal(Right)}, Accent, 3.0f, 1);
		Stroke({ToLocal(HeadBase + Side * 10.5f), ToLocal(Arrow.End), ToLocal(HeadBase - Side * 10.5f)},
			BrightAccent, 1.4f, 2);

		TArray<FVector2D> Origin;
		Origin.Reserve(49);
		for (int32 Index = 0; Index <= 48; ++Index)
		{
			const float Angle = PI * 0.125f + 2.0f * PI * static_cast<float>(Index) / 48.0f;
			Origin.Add(ToLocal(Arrow.Start + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 18.0f));
		}
		Stroke(Origin, FLinearColor(0.0f, 0.006f, 0.012f, 0.88f) * Tint, 5.5f, 0);
		Stroke(Origin, Accent.CopyWithNewOpacity(0.9f), 2.0f, 1);
		Stroke({ToLocal(Arrow.Start - Side * 7.0f), ToLocal(Arrow.Start + Side * 7.0f)}, BrightAccent, 1.5f, 2);
	}
	return LayerId + 2;
}
