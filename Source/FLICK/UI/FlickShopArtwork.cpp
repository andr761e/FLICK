#include "UI/FlickShopArtwork.h"

#include "Core/FlickShopCatalog.h"
#include "Rendering/DrawElements.h"

void SFlickShopArtwork::Construct(const FArguments& Args)
{
	ItemIndex = Args._ItemIndex;
	bCover = Args._Cover;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	SetCanTick(true);
}

void SFlickShopArtwork::Tick(const FGeometry&, double, float)
{
	const int32 Index = ItemIndex.Get();
	if (Index == LoadedIndex) return;
	LoadedIndex = Index;
	const auto* Item = FlickShopCatalog::Find(Index);
	Texture.Reset(Item ? LoadObject<UTexture2D>(nullptr, *Item->GetTexturePath()) : nullptr);
	Brush.SetResourceObject(Texture.Get());
	Invalidate(EInvalidateWidgetReason::Paint);
	if (!ItemIndex.IsBound()) SetCanTick(false);
}

int32 SFlickShopArtwork::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, const int32 Layer, const FWidgetStyle& Style, bool) const
{
	if (!Texture.IsValid()) return Layer;
	const FVector2D Area = Geometry.GetLocalSize();
	if (Area.X <= 0 || Area.Y <= 0) return Layer;
	// All six source renders are 3:2. Keep UVs independent of cooked mip dimensions.
	FVector2D DrawSize = Area;
	FVector2D Offset = FVector2D::ZeroVector;
	FVector2f UVMin(0, 0), UVMax(1, 1);
	if (bCover)
	{
		const float Aspect = Area.X / Area.Y;
		if (Aspect > 1.5f) { const float H = 1.5f / Aspect; UVMin.Y = (1 - H) * .5f; UVMax.Y = 1 - UVMin.Y; }
		else { const float W = Aspect / 1.5f; UVMin.X = (1 - W) * .5f; UVMax.X = 1 - UVMin.X; }
	}
	else
	{
		DrawSize = FVector2D(1.5f, 1) * FMath::Min(Area.X / 1.5f, Area.Y);
		Offset = (Area - DrawSize) * .5f;
	}
	Brush.SetUVRegion(FBox2f(UVMin, UVMax));
	FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(Offset)),
		&Brush, ESlateDrawEffect::None, Style.GetColorAndOpacityTint());
	return Layer;
}
