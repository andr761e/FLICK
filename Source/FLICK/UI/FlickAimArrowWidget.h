#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AFlickHUD;

// Screen-projected aim arrows are painted at viewport resolution with Slate's
// anti-aliased paths; shot direction and power still come from the controller.
class SFlickAimArrowWidget final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickAimArrowWidget) {} 
		SLATE_ARGUMENT(TWeakObjectPtr<AFlickHUD>, OwnerHud)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TWeakObjectPtr<AFlickHUD> OwnerHud;
};
