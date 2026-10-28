#include "UI/FlickGameLayerPrivate.h"

TSharedRef<SWidget> SFlickGameLayer::BuildHomeStatusPanel()
{
	// Use the home-only clock so the cycle restarts at 1v1 on returning home.
	// All three bindings use the same queue: badge, title and placement/MMR.
	const auto RankQueue = [this]()
	{
		const int32 Playlist = FlickMainMenuStyle::GetRankPlaylist(FooterTickerElapsed);
		return TPair<EFlickMatchVariant, int32>(Playlist == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic,
			Playlist == 3 ? 1 : Playlist + 1);
	};
	const auto ChangePulse = [this]()
	{
		if (FooterTickerElapsed < FlickMainMenuStyle::RankDisplaySeconds) return 0.f;
		const float Phase = FMath::Fmod(FooterTickerElapsed, FlickMainMenuStyle::RankDisplaySeconds);
		return 1.f - FMath::SmoothStep(0.f, .85f, Phase);
	};
	auto RankSteps = SNew(SHorizontalBox);
	for (int32 Playlist = 0; Playlist < 4; ++Playlist)
	{
		RankSteps->AddSlot().AutoWidth().Padding(0, 0, 5, 0)
		[SNew(SBox).WidthOverride(22).HeightOverride(3)
			.ToolTipText(FText::FromString(Playlist == 3 ? TEXT("BOB 1V1")
				: FString::Printf(TEXT("KNOCKOUT %dV%d"), Playlist + 1, Playlist + 1)))
			[SNew(SBorder).BorderImage(WhiteBrush()).Padding(0)
				.BorderBackgroundColor_Lambda([this, Playlist, ChangePulse]()
				{
					return FlickMainMenuStyle::GetRankPlaylist(FooterTickerElapsed) == Playlist
						? FMath::Lerp(Cyan, Paper, ChangePulse()) : Hairline.CopyWithNewOpacity(.6f);
				})]];
	}
	// One shell, two independently laid-out sections. Social remains a real
	// button rather than making the non-interactive MMR area clickable.
	TSharedRef<SButton> Social = SNew(SButton).ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(FMargin(18, 0, 4, 0)).HAlign(HAlign_Fill).VAlign(VAlign_Center)
		.ToolTipText(FText::FromString(TEXT("Open friends, invitations and your party")))
		.OnClicked_Lambda([this]() {
			bSocialPanelOpen = !bSocialPanelOpen;
			if (bSocialPanelOpen)
				if (auto* Sessions = GetDisplayedSessionSubsystem()) Sessions->RefreshFriends();
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakSocial = Social;
	Social->SetContent(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 13, 0)
		[SNew(SBox).WidthOverride(36).HeightOverride(36)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
			[SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Paper)]]]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[SNew(STextBlock).Text(FText::FromString(TEXT("SOCIAL"))).Font(DisplayFont(17)).ColorAndOpacity(Paper)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9, 0)
		[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(13)).ColorAndOpacity(Cyan)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[SNew(STextBlock).Text(FText::FromString(TEXT("+"))).Font(UiFont(27)).ColorAndOpacity_Lambda([WeakSocial]() {
			const auto Button = WeakSocial.Pin();
			return Button && (Button->IsHovered() || Button->HasKeyboardFocus()) ? Paper : Brand;
		})]);

	TSharedRef<SBox> StatusBox = SNew(SBox).WidthOverride(FlickMainMenuStyle::StatusWidth).HeightOverride(FlickMainMenuStyle::StatusHeight)
		.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Collapsed : EVisibility::Visible; })
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SFlickAngularBorder).BackgroundColor(Ink.CopyWithNewOpacity(.97f))
			.AccentColor(Cyan.CopyWithNewOpacity(.65f)).UseAccentForOutline(true).CutSize(14).BorderWidth(.8f).Padding(FMargin(18, 10))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 17, 0)
				[SNew(SFlickRankBadge).Size(48).Progress_Lambda([this, RankQueue]() {
					const auto* Ranking = GetRankingSubsystem();
					const auto Queue = RankQueue();
					return Ranking ? Ranking->GetDisplayProgress(Queue.Key, Queue.Value) : FFlickRankProgress();
				}).RenderTransformPivot(FVector2D(.5f, .5f))
					.RenderTransform_Lambda([ChangePulse]() { return FSlateRenderTransform(FScale2D(1.f + .08f * ChangePulse())); })]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0, 0, 16, 0)
				[
					SNew(SVerticalBox)
					.RenderTransform_Lambda([ChangePulse]() { return FSlateRenderTransform(FVector2D(8.f * ChangePulse(), 0)); })
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text_Lambda([RankQueue]() {
						const auto Queue = RankQueue();
						return FText::FromString(Queue.Key == EFlickMatchVariant::Bob ? TEXT("BOB 1V1 CURRENT RANK")
							: FString::Printf(TEXT("%dV%d CURRENT RANK"), Queue.Value, Queue.Value));
					}).Font(DisplayFont(12)).ColorAndOpacity_Lambda([ChangePulse]() { return FMath::Lerp(Paper, Cyan, ChangePulse()); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
					[SNew(STextBlock).Text_Lambda([this, RankQueue]() {
						const auto* Ranking = GetRankingSubsystem();
						const auto Queue = RankQueue();
						return FText::FromString(Ranking ? Ranking->GetDisplayProgressLabel(Queue.Key, Queue.Value) : TEXT("UNAVAILABLE"));
					}).Font(DisplayFont(16)).ColorAndOpacity(Brand).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)[RankSteps]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 14, 0)
				[SNew(SBox).WidthOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)]]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SBox).WidthOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.3f))]]
				+ SHorizontalBox::Slot().AutoWidth()[Social]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(14, 0, 0, 0)
		[SNew(SBox).WidthOverride(22).HeightOverride(2)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)]]
	];
	StatusBox->SetTag(TEXT("HomeStatus"));
	return StatusBox;
}
