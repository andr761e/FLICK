#include "UI/FlickRankBadge.h"

#include "Rendering/DrawElements.h"

void SFlickRankBadge::Construct(const FArguments& Args)
{
	Progress = Args._Progress;
	Size = Args._Size;
	Atlas.Reset(LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Ranks/RankBadges.RankBadges")));
	Brush.SetResourceObject(Atlas.Get());
	Brush.DrawAs = ESlateBrushDrawType::Image;
	SetCanTick(false);
	ForceVolatile(Progress.IsBound()); // Static ladder symbols need no per-frame invalidation.
	SetToolTipText(TAttribute<FText>::CreateLambda([this]()
	{
		return FText::FromString(FlickRankRules::GetRankLabel(Progress.Get()) + TEXT("\n")
			+ FlickRankRules::GetRatingRangeLabel(Progress.Get()));
	}));
}

int32 SFlickRankBadge::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, const int32 Layer, const FWidgetStyle& Style, bool) const
{
	const int32 Index = FlickRankRules::GetBadgeIndex(Progress.Get());
	const FVector2D Area = Geometry.GetLocalSize();
	if (Index == INDEX_NONE || !Atlas.IsValid())
	{
		// Neutral placement symbol: never imply that an unranked player earned Carbon.
		TArray<FVector2D> Ring;
		for (int32 Point = 0; Point <= 32; ++Point)
		{
			const float Angle = Point * UE_TWO_PI / 32.0f;
			Ring.Add(Area * .5f + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Area.GetMin() * .3f);
		}
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Ring,
			ESlateDrawEffect::None, FLinearColor(.35f, .47f, .53f) * Style.GetColorAndOpacityTint(), true, 2.0f);
		return Layer;
	}

	// Pixel-space crops of the approved generated atlas, not an assumed uniform grid.
	// Transparent gutters keep adjacent symbols out of the sampled region.
	static const FBox2f Regions[] =
	{
		{{80, 50}, {250, 210}}, {{358, 45}, {531, 210}}, {{633, 45}, {810, 210}},
		{{72, 240}, {257, 401}}, {{350, 240}, {538, 403}}, {{620, 240}, {823, 401}},
		{{60, 428}, {270, 599}}, {{339, 428}, {550, 600}}, {{615, 428}, {827, 601}},
		{{51, 616}, {279, 811}}, {{326, 616}, {563, 809}}, {{600, 616}, {842, 809}},
		{{65, 822}, {269, 1005}}, {{339, 822}, {556, 1003}}, {{609, 822}, {842, 1005}},
		{{17, 1034}, {301, 1190}}, {{302, 1036}, {588, 1191}}, {{588, 1037}, {876, 1193}},
		{{56, 1216}, {269, 1408}}, {{322, 1213}, {563, 1406}}, {{596, 1215}, {845, 1407}},
		{{313, 1437}, {575, 1714}}
	};
	static_assert(UE_ARRAY_COUNT(Regions) == FlickRankRules::BadgeCount);
	const FBox2f& Region = Regions[Index];
	// UVs are measured against source pixels, independent of platform downscaling
	// or whether an asynchronously compiling texture has a GPU resource yet.
	const FVector2f TextureSize(887, 1774);
	Brush.SetUVRegion(FBox2f(Region.Min / TextureSize, Region.Max / TextureSize));
	const FVector2D SourceSize(Region.GetSize());
	const float Scale = FMath::Min(Area.X / SourceSize.X, Area.Y / SourceSize.Y);
	const FVector2D DrawSize = SourceSize * Scale;
	FSlateDrawElement::MakeBox(Elements, Layer,
		Geometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform((Area - DrawSize) * .5f)),
		&Brush, ESlateDrawEffect::None, Style.GetColorAndOpacityTint());
	return Layer;
}
