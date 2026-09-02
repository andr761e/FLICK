#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

class SFlickLogoWidget final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickLogoWidget)
		: _Opacity(1.0f)
		, _CropToArtwork(false)
	{}
		SLATE_ATTRIBUTE(float, Opacity)
		SLATE_ARGUMENT(bool, CropToArtwork)
	SLATE_END_ARGS()

	static UTexture2D* LoadPreparedTexture();

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	TAttribute<float> Opacity;
	bool bCropToArtwork = false;
	TStrongObjectPtr<UTexture2D> LogoTexture;
	FSlateBrush LogoBrush;
};
