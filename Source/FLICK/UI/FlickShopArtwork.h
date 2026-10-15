#pragma once

#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

// Loads only when a visible shop image ticks, not when the hidden shop is built.
class SFlickShopArtwork final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickShopArtwork) : _ItemIndex(0), _Cover(false) {}
		SLATE_ATTRIBUTE(int32, ItemIndex)
		SLATE_ARGUMENT(bool, Cover)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual void Tick(const FGeometry&, double, float) override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(300, 200); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&,
		int32, const FWidgetStyle&, bool) const override;
private:
	TAttribute<int32> ItemIndex;
	bool bCover = false;
	int32 LoadedIndex = INDEX_NONE;
	TStrongObjectPtr<UTexture2D> Texture;
	mutable FSlateBrush Brush;
};
