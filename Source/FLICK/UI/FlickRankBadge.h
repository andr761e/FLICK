#pragma once

#include "Core/FlickRankRules.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

// All earned symbols share a single cooked texture. No per-frame asset loading.
class SFlickRankBadge final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickRankBadge) : _Size(64.0f) {}
		SLATE_ATTRIBUTE(FFlickRankProgress, Progress)
		SLATE_ARGUMENT(float, Size)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Size); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
		FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;

private:
	TAttribute<FFlickRankProgress> Progress;
	float Size = 64.0f;
	TStrongObjectPtr<UTexture2D> Atlas;
	mutable FSlateBrush Brush;
};
