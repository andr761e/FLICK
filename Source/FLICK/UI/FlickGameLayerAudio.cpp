#include "UI/FlickGameLayerPrivate.h"
#include "Audio/FlickMenuRadioComponent.h"

namespace
{
	// Decorative playback activity, not an FFT. Paused/muted tracks remain still.
	class SFlickRadioActivity final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickRadioActivity) : _Active(false), _Progress(0) {}
			SLATE_ATTRIBUTE(bool, Active)
			SLATE_ATTRIBUTE(float, Progress)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Active = Args._Active; Progress = Args._Progress; SetCanTick(true); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(142, 40); }
		virtual void Tick(const FGeometry&, double, float) override { if (Active.Get()) Invalidate(EInvalidateWidgetReason::Paint); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Size = Geometry.GetLocalSize();
			const bool bActive = Active.Get();
			const float Phase = bActive ? Progress.Get() * 700.f : 0.f;
			for (int32 Bar = 0; Bar < 25; ++Bar)
			{
				const float Shape = .18f + .65f * FMath::Abs(FMath::Sin(Bar * 1.73f)) * FMath::Sin((Bar + 1) * PI / 26.f);
				const float Height = Size.Y * Shape * (bActive ? .65f + .35f * FMath::Sin(Phase + Bar * .8f) : .65f);
				const float X = (Bar + .5f) * Size.X / 25.f;
				FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(),
					TArray<FVector2D>{{X, Size.Y - 2}, {X, Size.Y - 2 - Height}}, ESlateDrawEffect::None,
					Cyan.CopyWithNewOpacity(bActive ? .65f : .25f) * Style.GetColorAndOpacityTint(), true, 1.f);
			}
			return Layer;
		}
	private:
		TAttribute<bool> Active;
		TAttribute<float> Progress;
	};

	class SFlickRadioTransport final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickRadioTransport) : _Action(0), _Playing(false), _Focused(false) {}
			SLATE_ARGUMENT(int32, Action)
			SLATE_ATTRIBUTE(bool, Playing)
			SLATE_ATTRIBUTE(bool, Focused)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args) { Action = Args._Action; Playing = Args._Playing; Focused = Args._Focused; SetCanTick(false); }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(50, 50); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
			FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Center = Geometry.GetLocalSize() * .5f;
			const FLinearColor Tint = Style.GetColorAndOpacityTint();
			const auto Line = [&](TArray<FVector2D> Points, const FLinearColor Color, float Width) {
				for (auto& Point : Points) Point += Center;
				FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color * Tint, true, Width);
			};
			const auto Triangle = [&](float Direction) {
				const auto& Transform = Geometry.GetAccumulatedRenderTransform();
				TArray<FSlateVertex> Vertices;
				for (const FVector2D Point : {FVector2D(-5 * Direction, -7), FVector2D(6 * Direction, 0), FVector2D(-5 * Direction, 7)})
					Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Center + Point), FVector2f::ZeroVector, (Paper * Tint).ToFColor(true)));
				FSlateDrawElement::MakeCustomVerts(Elements, Layer + 1, WhiteBrush()->GetRenderingResource(), Vertices, {0, 1, 2}, nullptr, 0, 0);
			};
			if (Action == 0)
			{
				TArray<FVector2D> Ring;
				for (int32 Step = 0; Step <= 64; ++Step) { const float Angle = 2 * PI * Step / 64; Ring.Add(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 23); }
				Line(Ring, Focused.Get() ? Brand : Cyan, 1.f);
				if (Playing.Get()) { Line({{-5, -9}, {-5, 9}}, Paper, 4); Line({{5, -9}, {5, 9}}, Paper, 4); }
				else Triangle(1);
			}
			else
			{
				Triangle(Action > 0 ? 1.f : -1.f);
				const float X = Action > 0 ? 9.f : -9.f;
				Line({{X, -7}, {X, 7}}, Focused.Get() ? Brand : Paper, 2);
			}
			return Layer + 1;
		}
	private:
		int32 Action = 0;
		TAttribute<bool> Playing, Focused;
	};
}

TSharedRef<SWidget> SFlickGameLayer::BuildMenuRadio()
{
	const auto Radio = [this]() -> UFlickMenuRadioComponent* { return PlayerController.IsValid() ? PlayerController->GetMenuRadio() : nullptr; };
	const auto MakeTransport = [this, Radio](const TCHAR* Tooltip, int32 Action) -> TSharedRef<SWidget>
	{
		TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
			.ToolTipText(FText::FromString(Tooltip)).OnClicked_Lambda([Radio, Action]() {
				if (auto* Player = Radio()) { if (Action == 0) Player->TogglePlayback(); else Player->Skip(Action); }
				return FReply::Handled();
			});
		Button->SetTag(Action == 0 ? FName(TEXT("HomeRadioToggle")) : Action < 0 ? FName(TEXT("HomeRadioPrevious")) : FName(TEXT("HomeRadioNext")));
		const TWeakPtr<SButton> Weak = Button;
		Button->SetContent(SNew(SBox).WidthOverride(50).HeightOverride(50)
			[SNew(SFlickRadioTransport).Action(Action).Playing_Lambda([Radio]() { return Radio() && Radio()->IsEnabled(); })
				.Focused_Lambda([Weak]() { const auto Pinned = Weak.Pin(); return Pinned && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()); })]);
		return Button;
	};
	static const FProgressBarStyle RadioProgress = FProgressBarStyle()
		.SetBackgroundImage(FSlateColorBrush(Cyan.CopyWithNewOpacity(.35f)))
		.SetFillImage(FSlateColorBrush(Cyan)).SetMarqueeImage(FSlateNoResource());
	return SNew(SBox).WidthOverride(FlickMainMenuStyle::RightColumnWidth).HeightOverride(FlickMainMenuStyle::RadioHeight)
	[SNew(SFlickAngularBorder).BackgroundColor(Ink.CopyWithNewOpacity(.97f)).AccentColor(Cyan.CopyWithNewOpacity(.7f))
		.UseAccentForOutline(true).BorderWidth(.8f).CutSize(12).Padding(FMargin(22, 10))
		[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK RADIO //"))).Font(UiFont(9)).ColorAndOpacity(Cyan)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Bottom)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([Radio]() { return FText::FromString(Radio() ? Radio()->GetTrackTitle() : TEXT("TUNING IN...")); }).Font(DisplayFont(16)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)[SNew(STextBlock).Text_Lambda([Radio]() { return FText::FromString(Radio() ? Radio()->GetStatus() : TEXT("")); }).Font(UiFont(9)).ColorAndOpacity(Muted).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(10, 0, 0, 0)
			[SNew(SBox).WidthOverride(142).HeightOverride(40)[SNew(SFlickRadioActivity)
				.ToolTipText(FText::FromString(TEXT("Playback activity"))).Active_Lambda([Radio]() { return Radio() && Radio()->HasAudiblePlayback(); })
				.Progress_Lambda([Radio]() { return Radio() ? Radio()->GetProgress() : 0; })]]
		]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(1)[SNew(SProgressBar).Style(&RadioProgress).BarFillStyle(EProgressBarFillStyle::Scale)
			.Percent_Lambda([Radio]() { return Radio() ? Radio()->GetProgress() : 0; }).FillColorAndOpacity(Cyan)]]
		+ SVerticalBox::Slot().FillHeight(1).Padding(0, 4, 0, 0)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[MakeTransport(TEXT("Previous track"), -1)]
			+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[MakeTransport(TEXT("Pause or resume menu music"), 0)]
			+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)[MakeTransport(TEXT("Next track"), 1)]]]];
}
