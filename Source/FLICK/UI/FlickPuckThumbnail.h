#pragma once

#include "Core/FlickPieceArchetypeRules.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

enum class EFlickPuckThumbnailView : uint8 { Original, Presentation, Locker, Hero, ProfileCutout };

// Offline renders of the actual optimized FBXs. Never creates runtime scene captures.
class SFlickPuckThumbnail final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickPuckThumbnail) : _Archetype(EFlickPieceArchetype::Standard), _Skin(0), _View(EFlickPuckThumbnailView::Original), _CutSize(0), _FadeRight(0), _Zoom(1), _RollDegrees(0) {}
		SLATE_ATTRIBUTE(EFlickPieceArchetype, Archetype)
		SLATE_ATTRIBUTE(int32, Skin)
		SLATE_ARGUMENT(EFlickPuckThumbnailView, View)
		SLATE_ARGUMENT(float, CutSize)
		SLATE_ARGUMENT(float, FadeRight)
		SLATE_ARGUMENT(float, Zoom)
		SLATE_ARGUMENT(float, RollDegrees)
		SLATE_ARGUMENT(FMargin, EdgeFade)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual void Tick(const FGeometry&, double, float) override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(256, 192); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&,
		int32, const FWidgetStyle&, bool) const override;
private:
	TAttribute<EFlickPieceArchetype> Archetype;
	TAttribute<int32> Skin;
	int32 LoadedKey = INDEX_NONE;
	bool bReady = false;
	EFlickPuckThumbnailView View = EFlickPuckThumbnailView::Original;
	float CutSize = 0;
	float FadeRight = 0;
	float Zoom = 1;
	float RollDegrees = 0;
	FMargin EdgeFade;
	TStrongObjectPtr<UTexture2D> Texture;
	mutable FSlateBrush Brush;
};
