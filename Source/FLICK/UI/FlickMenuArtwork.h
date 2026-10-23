#pragma once

#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SLeafWidget.h"

// Shared, text-free card illustrations. These are not substitutes for the live arena.
enum class EFlickMenuArtwork : uint8
{
	Casual, Competitive, Training, PrivateMatch, SwitchControl, BobPockets,
	Tutorial, FreePlay, BotMatch, Bob, Doubles, Triples, Backdrop, SettingsBackdrop, SettingsImpact,
	ProfileBackdrop, ProfileRankStage, ProfileCustomize,
	PracticePrecision, PracticeKnockouts, PracticeSwitch, PracticeBob,
	TrainingPracticeWide, ProfileIdentityBanner, ProfileShowcaseFloor, ProfilePlacement, Count
};

namespace FlickMenuArtwork
{
	const TCHAR* AssetName(EFlickMenuArtwork Artwork);
	FVector2f AuthoredSize(EFlickMenuArtwork Artwork);
}

class SFlickMenuArtwork final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickMenuArtwork) : _Artwork(EFlickMenuArtwork::Casual), _CutSize(0), _ShadeForText(false), _TextShadeStart(.30f) {}
		SLATE_ARGUMENT(EFlickMenuArtwork, Artwork)
		SLATE_ARGUMENT(float, CutSize)
		SLATE_ARGUMENT(bool, ShadeForText)
		SLATE_ARGUMENT(float, TextShadeStart)
		SLATE_ARGUMENT(FMargin, EdgeFade)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual void Tick(const FGeometry&, double, float) override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(480, 270); }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&,
		FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
private:
	EFlickMenuArtwork Artwork = EFlickMenuArtwork::Casual;
	float CutSize = 0;
	bool bShadeForText = false;
	float TextShadeStart = .30f;
	FMargin EdgeFade;
	TStrongObjectPtr<UTexture2D> Texture;
	mutable FSlateBrush Brush;
};
