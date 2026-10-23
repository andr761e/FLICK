#pragma once

#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickPlaylistStyle.h"

// Presentation-only helpers shared by every page in the Play flow.
namespace FlickPlaylistWidgets
{
	constexpr float ContentWidth = 1248;
	constexpr float CardHeight = 272;
	constexpr float Gap = 9;

	class SCardArrow final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCardArrow) : _Active(false), _Divider(true), _Color(FLinearColor::Transparent) {}
			SLATE_ATTRIBUTE(bool, Active)
			SLATE_ARGUMENT(bool, Divider)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Active = Args._Active; Color = Args._Color; bDivider = Args._Divider; SetCanTick(false); ForceVolatile(Active.IsBound() || Color.IsBound()); }
		virtual FVector2D ComputeDesiredSize(float) const override { return bDivider ? FVector2D(58,56) : FVector2D(26,26); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FLinearColor Tint = Style.GetColorAndOpacityTint();
			const FLinearColor ArrowTint = (Color.Get().A > 0 ? Color.Get() : Active.Get() ? FlickUITheme::Brand : FlickUITheme::Paper) * Tint;
			const FVector2D Size = Geometry.GetLocalSize();
			const float Side = FMath::Min(bDivider ? Size.X*.48 : Size.X,Size.Y*.72);
			const FVector2D Origin(bDivider ? Size.X*.42 : (Size.X-Side)*.5,(Size.Y-Side)*.5);
			if (bDivider) FSlateDrawElement::MakeLines(Elements,Layer,Geometry.ToPaintGeometry(),
				TArray<FVector2D>{{1,Size.Y*.05},{1,Size.Y*.95}},ESlateDrawEffect::None,FlickUITheme::Muted.CopyWithNewOpacity(.45f)*Tint,true,.8f);
			const auto Points = FlickPlaylistStyle::ArrowOutline(FVector2D(Side,Side));
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			Vertices.Add(FSlateVertex::Make(Geometry.GetAccumulatedRenderTransform(),FVector2f(Origin+FVector2D(Side*.69,Side*.5)),FVector2f::ZeroVector,ArrowTint.ToFColor(true)));
			for (const auto& P : Points) Vertices.Add(FSlateVertex::Make(Geometry.GetAccumulatedRenderTransform(),FVector2f(Origin+P),FVector2f::ZeroVector,ArrowTint.ToFColor(true)));
			for (int32 I=0; I<Points.Num(); ++I) Indices.Append({0,static_cast<SlateIndex>(I+1),static_cast<SlateIndex>((I+1)%Points.Num()+1)});
			FSlateDrawElement::MakeCustomVerts(Elements,Layer,WhiteBrush()->GetRenderingResource(),Vertices,Indices,nullptr,0,0);
			auto Edge=Points; const auto First=Edge[0]; Edge.Add(First);
			for (auto& P : Edge) P+=Origin;
			FSlateDrawElement::MakeLines(Elements,Layer,Geometry.ToPaintGeometry(),Edge,ESlateDrawEffect::None,ArrowTint,true,.55f);
			return Layer;
		}
	private:
		TAttribute<bool> Active;
		TAttribute<FLinearColor> Color;
		bool bDivider = true;
	};

	class SSelectionEdge final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SSelectionEdge) : _Active(false), _CutSize(10.5f) {}
			SLATE_ATTRIBUTE(bool, Active)
			SLATE_ARGUMENT(float, CutSize)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Active = Args._Active; CutSize = Args._CutSize; SetCanTick(false); ForceVolatile(Active.IsBound()); SetVisibility(EVisibility::HitTestInvisible); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override
		{
			if (!Active.Get() || !bParentEnabled) return Layer;
			const FVector2D Size = Geometry.GetLocalSize();
			if (Size.X <= 0 || Size.Y <= 0) return Layer;
			const auto Outline = FlickPlaylistStyle::GetClosedOutline(Size, CutSize);
			const FLinearColor Color = FlickUITheme::Brand * Style.GetColorAndOpacityTint();
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Outline,
				ESlateDrawEffect::None, Color.CopyWithNewOpacity(Color.A * .16f), true, 6);
			FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Outline,
				ESlateDrawEffect::None, Color, true, 2.4f);
			return Layer + 1;
		}
	private:
		TAttribute<bool> Active;
		float CutSize = 10.5f;
	};

	inline TSharedRef<SWidget> BackContent(const TWeakPtr<SButton>& WeakButton)
	{
		using namespace FlickUITheme;
		return SNew(SFlickAngularBorder).CutSize(11).Padding(FMargin(20, 0)).UseAccentForOutline(true)
			.AccentColor_Lambda([WeakButton]() {
				const auto Button = WeakButton.Pin();
				return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus()) ? Brand : Cyan.CopyWithNewOpacity(.55f);
			}).BackgroundColor(Ink.CopyWithNewOpacity(.65f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 24, 0)
				[SNew(STextBlock).Text(FText::FromString(TEXT("\u2190"))).Font(UiFont(24, true)).ColorAndOpacity(Brand)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(TEXT("BACK"))).Font(DisplayFont(16)).ColorAndOpacity(Paper)]
			];
	}

	inline TSharedRef<SWidget> Background()
	{
		return SNew(SOverlay).Visibility(EVisibility::HitTestInvisible)
			+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::Backdrop)]
			+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush())
				.BorderBackgroundColor(FLinearColor(.001f, .005f, .009f, .24f))];
	}

	inline TSharedRef<SWidget> Header(TAttribute<FText> Breadcrumb, TAttribute<FText> Title, TAttribute<FText> Stage)
	{
		using namespace FlickUITheme;
		return SNew(SBox).HeightOverride(104)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(SBox).WidthOverride(280).HeightOverride(94)
				[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
					[SNew(SFlickLogoWidget).VectorPath(TEXT("UI/FlickKnockoutWordmark.svg"))
						.TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark"))
						.DesiredSize(FVector2D(2048, 683))]]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28, 0)
			[SNew(SBox).WidthOverride(1).HeightOverride(92)
				[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Muted.CopyWithNewOpacity(.75f))]]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Breadcrumb).Font(DisplayFont(12)).ColorAndOpacity(Cyan)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 3)
				[SNew(STextBlock).Text(Title).Font(DisplayFont(34)).ColorAndOpacity(Paper)]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(Stage).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			]
		];
	}

	inline TSharedRef<SWidget> Footer(const TSharedRef<SWidget>& Content)
	{
		return SNew(SBox).HeightOverride(64)
			[SNew(SFlickAngularBorder).CutSize(12).BorderWidth(.8f).UseAccentForOutline(true)
				.AccentColor(FlickUITheme::Cyan.CopyWithNewOpacity(.6f))
				.BackgroundColor(FLinearColor(.002f, .01f, .017f, .82f)).Padding(FMargin(18, 6))[Content]];
	}

	inline TSharedRef<SWidget> Card(const EFlickMenuArtwork Artwork, const TAttribute<bool>& Active,
		const TSharedRef<SWidget>& Content, const float ArtworkWidth = 0)
	{
		return SNew(SFlickAngularBorder).CutSize(12).Padding(1.5f).UseAccentForOutline(true)
			.AccentColor(TAttribute<FLinearColor>::CreateLambda([Active]() {
				return Active.Get() ? FlickUITheme::Brand : FlickUITheme::Cyan.CopyWithNewOpacity(.52f);
			}))
			.BorderWidth(1)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(ArtworkWidth > 0 ? HAlign_Right : HAlign_Fill)
				[SNew(SBox).WidthOverride(ArtworkWidth > 0 ? FOptionalSize(ArtworkWidth) : FOptionalSize())
					[SNew(SFlickMenuArtwork).Artwork(Artwork).CutSize(10.5f).ShadeForText(true)]]
				+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(22, 16)[Content]
				+ SOverlay::Slot()[SNew(SSelectionEdge).Active(Active)]
			];
	}
}
