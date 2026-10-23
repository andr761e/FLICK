#include "UI/FlickMenuArtwork.h"

#include "Rendering/DrawElements.h"
#include "UI/FlickUITheme.h"
#include "UI/FlickArtworkGeometry.h"
#include "Styling/CoreStyle.h"

const TCHAR* FlickMenuArtwork::AssetName(const EFlickMenuArtwork Artwork)
{
	static const TCHAR* Names[] = {
		TEXT("PlayCasualV2"), TEXT("PlayCompetitiveV2"), TEXT("PlayTrainingV2"), TEXT("PlayPrivateV2"),
		TEXT("PracticeSwitchV2"), TEXT("PracticeBob"), TEXT("TrainingTutorialV2"), TEXT("TrainingFreePlayV2"),
		TEXT("TrainingBotV2"), TEXT("TrainingBobV2"), TEXT("FormatDoublesV2"), TEXT("FormatTriplesV2"), TEXT("PlayBackdropV2"),
		TEXT("SettingsBackdropV2"), TEXT("SettingsImpactV2"),
		TEXT("ProfileBackdropV2"), TEXT("ProfileRankStageV2"), TEXT("ProfileFinishTrioV4"),
		TEXT("PracticePrecisionV3"), TEXT("PracticeKnockoutsV3"), TEXT("PracticeSwitchV3"), TEXT("PracticeBobV3"),
		TEXT("TrainingPracticeWideV3"), TEXT("ProfileIdentityBannerV3"), TEXT("ProfileShowcaseFloorV4"), TEXT("ProfilePlacementV4")
	};
	static_assert(UE_ARRAY_COUNT(Names) == static_cast<uint8>(EFlickMenuArtwork::Count));
	return Names[FMath::Clamp(static_cast<int32>(Artwork), 0, UE_ARRAY_COUNT(Names) - 1)];
}

FVector2f FlickMenuArtwork::AuthoredSize(const EFlickMenuArtwork Artwork)
{
	if (Artwork == EFlickMenuArtwork::ProfileShowcaseFloor) return FVector2f(1448,1086);
	if (Artwork == EFlickMenuArtwork::TrainingPracticeWide || Artwork == EFlickMenuArtwork::ProfileIdentityBanner) return FVector2f(2172,724);
	if (Artwork == EFlickMenuArtwork::BobPockets
		|| Artwork == EFlickMenuArtwork::Backdrop || Artwork == EFlickMenuArtwork::SettingsBackdrop
		|| Artwork == EFlickMenuArtwork::SettingsImpact
		|| Artwork == EFlickMenuArtwork::ProfileBackdrop || Artwork == EFlickMenuArtwork::ProfileRankStage
		|| Artwork == EFlickMenuArtwork::ProfileCustomize || Artwork == EFlickMenuArtwork::ProfilePlacement
		|| Artwork == EFlickMenuArtwork::PracticePrecision || Artwork == EFlickMenuArtwork::PracticeKnockouts
		|| Artwork == EFlickMenuArtwork::PracticeSwitch || Artwork == EFlickMenuArtwork::PracticeBob) return FVector2f(1672, 941);
	return Artwork == EFlickMenuArtwork::Bob || Artwork == EFlickMenuArtwork::Triples
		? FVector2f(1862, 845) : FVector2f(1860, 845);
}

void SFlickMenuArtwork::Construct(const FArguments& Args)
{
	Artwork = Args._Artwork;
	CutSize = Args._CutSize;
	bShadeForText = Args._ShadeForText;
	TextShadeStart = FMath::Clamp(Args._TextShadeStart,0.f,.95f);
	EdgeFade = Args._EdgeFade;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	SetCanTick(true);
}

void SFlickMenuArtwork::Tick(const FGeometry&, double, float)
{
	if (!Texture.IsValid())
	{
		const FString Name = FlickMenuArtwork::AssetName(Artwork);
		Texture.Reset(LoadObject<UTexture2D>(nullptr,
			*FString::Printf(TEXT("/Game/UI/MenuArtwork/T_Menu_%s.T_Menu_%s"), *Name, *Name)));
		if (!Texture.IsValid()) { SetCanTick(false); return; }
	}
#if WITH_EDITOR
	// A fresh editor DDC compiles BC7 asynchronously. Don't cache a blank Slate
	// resource forever while that work is pending. Cooked games skip this branch.
	if (Texture.IsValid() && Texture->IsCompiling()) return;
#endif
	if (Texture.IsValid() && !Texture->GetResource()) return;
	Brush.SetResourceObject(Texture.Get());
	// Hidden pages don't load their illustrations. Visible images load once, with no recurring work.
	SetCanTick(false);
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SFlickMenuArtwork::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
	FSlateWindowElementList& Elements, const int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	if (!Texture.IsValid()) return Layer;
	const FVector2D Size = Geometry.GetLocalSize();
	if (Size.X <= 0 || Size.Y <= 0) return Layer;
	// Authored aspect ratio, independent of power-of-two cooked mip dimensions.
	const FVector2f AuthoredSize = FlickMenuArtwork::AuthoredSize(Artwork);
	const float SourceAspect = AuthoredSize.X / AuthoredSize.Y;
	const float Aspect = Size.X / Size.Y;
	FVector2f Min(0, 0), Max(1, 1);
	if (Aspect > SourceAspect)
	{
		Min.Y = (1 - SourceAspect / Aspect) * .5f;
		Max.Y = 1 - Min.Y;
	}
	else
	{
		Min.X = (1 - Aspect / SourceAspect) * .5f;
		Max.X = 1 - Min.X;
	}
	// Rectangular Slate clipping cannot follow cut corners. Draw the texture on
	// the actual eight-sided polygon, so no image can escape its card frame.
	const float Cut = FMath::Clamp(CutSize, 0.0f, FMath::Min(Size.X, Size.Y) * .24f);
	const auto Points = FlickUITheme::GetPanelOutline(Size, Cut);
	const auto& Transform = Geometry.GetAccumulatedRenderTransform();
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	const ESlateDrawEffect Effect = bParentEnabled && IsEnabled() ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	TArray<FSlateVertex> Vertices;
	TArray<SlateIndex> Indices;
	for (const auto& Polygon : FlickArtworkGeometry::FadePolygons(Points,Size,EdgeFade))
	{
		const SlateIndex Start = static_cast<SlateIndex>(Vertices.Num());
		for (const FVector2D& Point : Polygon)
		{
			const FVector2f Uv = Min + FVector2f(Point / Size) * (Max - Min);
			FLinearColor Color = Tint;
			Color.A *= FlickArtworkGeometry::Opacity(Point,Size,EdgeFade);
			Vertices.Add(FSlateVertex::Make(Transform,FVector2f(Point),Uv,Color.ToFColor(true)));
		}
		for (int32 Index = 1; Index + 1 < Polygon.Num(); ++Index)
			Indices.Append({Start,static_cast<SlateIndex>(Start+Index),static_cast<SlateIndex>(Start+Index+1)});
	}
	FSlateDrawElement::MakeCustomVerts(Elements, Layer, Brush.GetRenderingResource(), Vertices, Indices, nullptr, 0, 0, Effect);
	if (!bShadeForText) return Layer;

	// Soft lower scrim, not an opaque rectangular text band. Each horizontal
	// slice follows the same cut silhouette as the image, including the corners.
	// Preserve the original sample rows for existing menu cards; only callers
	// opting into a later score-strip shade change their illustration contrast.
	const bool bOriginalShade = FMath::IsNearlyEqual(TextShadeStart,.30f);
	const float FadeStartY = FMath::Clamp(static_cast<float>(Size.Y)*(bOriginalShade ? .38f : TextShadeStart),Cut,static_cast<float>(Size.Y)-Cut);
	const float FadeMidY = FMath::Clamp(static_cast<float>(Size.Y)*(bOriginalShade ? .68f : (TextShadeStart+1)*.5f),FadeStartY,static_cast<float>(Size.Y)-Cut);
	const float Rows[] = {0, Cut, FadeStartY, FadeMidY,
		static_cast<float>(Size.Y) - Cut, static_cast<float>(Size.Y)};
	Vertices.Reset(); Indices.Reset();
	for (int32 Row = 0; Row < UE_ARRAY_COUNT(Rows); ++Row)
	{
		const float Y = Rows[Row];
		const float Inset = FMath::Max(0.0f, FMath::Max(Cut - Y, Y - (Size.Y - Cut)));
		const float Fraction = Y / Size.Y;
		const float Alpha = FMath::GetMappedRangeValueClamped(FVector2f(TextShadeStart, 1.f), FVector2f(.01f, .96f), Fraction);
		const FColor Color = (FLinearColor(.001f, .006f, .011f, Alpha) * Tint).ToFColor(true);
		Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Inset, Y), FVector2f::ZeroVector, Color));
		Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Size.X - Inset, Y), FVector2f::ZeroVector, Color));
		if (Row > 0)
		{
			const SlateIndex I = static_cast<SlateIndex>((Row - 1) * 2);
			Indices.Append({I, static_cast<SlateIndex>(I + 1), static_cast<SlateIndex>(I + 2),
				static_cast<SlateIndex>(I + 1), static_cast<SlateIndex>(I + 3), static_cast<SlateIndex>(I + 2)});
		}
	}
	FSlateDrawElement::MakeCustomVerts(Elements, Layer + 1, FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0, Effect);
	return Layer + 1;
}
