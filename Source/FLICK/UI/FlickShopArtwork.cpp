#include "UI/FlickShopArtwork.h"

#include "Core/FlickShopCatalog.h"
#include "Rendering/DrawElements.h"
#include "UI/FlickShopStyle.h"
#include "UI/FlickUITheme.h"
#include "Styling/CoreStyle.h"

void SFlickShopArtwork::Construct(const FArguments& Args)
{
	ItemIndex = Args._ItemIndex;
	bCover = Args._Cover;
	bHero = Args._Hero;
	CutSize = Args._CutSize;
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
	// A fresh editor DDC may still be compiling BC7. Cooked games skip this check.
#if WITH_EDITOR
	if (Texture.IsValid() && Texture->IsCompiling()) { LoadedIndex = INDEX_NONE; return; }
#endif
	if (Texture.IsValid() && !Texture->GetResource()) { LoadedIndex = INDEX_NONE; return; }
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
	const auto& Transform = Geometry.GetAccumulatedRenderTransform();
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	if (bHero)
	{
		const FVector2D Image = FlickShopStyle::HeroImageSize(Area);
		const FVector2D Origin = FlickShopStyle::HeroImageOffset(Area);
		const float Cut = FMath::Clamp(CutSize, 0.f, static_cast<float>(Area.Y) * .2f);
		TArray<float> Columns = {0, Cut, static_cast<float>(Origin.X), static_cast<float>(Origin.X + 70),
			static_cast<float>(Origin.X + Image.X - 70), static_cast<float>(Origin.X + Image.X), static_cast<float>(Area.X - Cut), static_cast<float>(Area.X)};
		for (float& X : Columns) X = FMath::Clamp(X, 0.f, static_cast<float>(Area.X));
		Columns.Sort();
		const float Rows[] = {0, Cut, static_cast<float>(Area.Y) - Cut, static_cast<float>(Area.Y)};
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		for (int32 R = 0; R < UE_ARRAY_COUNT(Rows); ++R)
		{
			const float Y = Rows[R];
			const float Inset = FMath::Max(0.f, FMath::Max(Cut - Y, Y - (Area.Y - Cut)));
			for (int32 C = 0; C < Columns.Num(); ++C)
			{
				const float X = FMath::Clamp(Columns[C], Inset, static_cast<float>(Area.X) - Inset);
				const FVector2D UV = (FVector2D(X, Y) - Origin) / Image;
				const float Alpha = FMath::Clamp(FMath::Min((X - Origin.X) / 70, (Origin.X + Image.X - X) / 70), 0., 1.);
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(X, Y), FVector2f(FMath::Clamp(UV.X, 0., 1.), FMath::Clamp(UV.Y, 0., 1.)),
					(Tint * FLinearColor(1, 1, 1, Alpha)).ToFColor(true)));
				if (R > 0 && C > 0)
				{
					const SlateIndex I = static_cast<SlateIndex>((R - 1) * Columns.Num() + C - 1), J = I + Columns.Num();
					Indices.Append({I, static_cast<SlateIndex>(I + 1), J, static_cast<SlateIndex>(I + 1), static_cast<SlateIndex>(J + 1), J});
				}
			}
		}
		FSlateDrawElement::MakeCustomVerts(Elements, Layer, Brush.GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);
		// Native left scrim unifies existing backdrop/product art without a hard split panel.
		Vertices.Reset(); Indices.Reset();
		const float ShadeColumns[] = {0, 430, 640, static_cast<float>(Area.X)};
		const float ShadeAlpha[] = {.94f, .84f, 0, 0};
		for (int32 R = 0; R < UE_ARRAY_COUNT(Rows); ++R)
		{
			const float Y = Rows[R], Inset = FMath::Max(0.f, FMath::Max(Cut - Y, Y - (Area.Y - Cut)));
			for (int32 C = 0; C < UE_ARRAY_COUNT(ShadeColumns); ++C)
			{
				const float X = FMath::Clamp(ShadeColumns[C], Inset, static_cast<float>(Area.X) - Inset);
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(X, Y), FVector2f::ZeroVector, (FLinearColor(.001f, .005f, .009f, ShadeAlpha[C]) * Tint).ToFColor(true)));
				if (R > 0 && C > 0) { const SlateIndex I = (R - 1) * 4 + C - 1, J = I + 4; Indices.Append({I, static_cast<SlateIndex>(I + 1), J, static_cast<SlateIndex>(I + 1), static_cast<SlateIndex>(J + 1), J}); }
			}
		}
		FSlateDrawElement::MakeCustomVerts(Elements, Layer + 1, FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);
		return Layer + 1;
	}
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
	// Image vertices follow the frame's cut corners, not just rectangular Slate clipping.
	const auto Points = FlickUITheme::GetPanelOutline(DrawSize, CutSize);
	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	for (const FVector2D& Point : Points)
		Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Point + Offset), UVMin + FVector2f(Point / DrawSize) * (UVMax - UVMin), Tint.ToFColor(true)));
	for (int32 I = 1; I + 1 < Points.Num(); ++I) Indices.Append({0, static_cast<SlateIndex>(I), static_cast<SlateIndex>(I + 1)});
	FSlateDrawElement::MakeCustomVerts(Elements, Layer, Brush.GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);
	return Layer;
}
