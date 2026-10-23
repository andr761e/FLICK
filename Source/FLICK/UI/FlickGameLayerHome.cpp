#include "UI/FlickGameLayerPrivate.h"

TSharedRef<SWidget> SFlickGameLayer::BuildHomeStatusPanel()
{
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
				[SNew(SFlickRankBadge).Size(48).Progress_Lambda([this]() {
					const auto* Ranking = GetRankingSubsystem();
					return Ranking ? Ranking->GetDisplayProgress(GameMode.IsValid() ? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic,
						GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1) : FFlickRankProgress();
				})]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(0, 0, 16, 0)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text_Lambda([this]() {
						if (!GameMode.IsValid()) return FText::FromString(TEXT("CURRENT RANK"));
						return FText::FromString(GameMode->GetActiveMatchVariant() == EFlickMatchVariant::Bob ? TEXT("BOB RANK")
							: FString::Printf(TEXT("%dV%d CURRENT RANK"), GameMode->GetPlayersPerTeam(), GameMode->GetPlayersPerTeam()));
					}).Font(DisplayFont(12)).ColorAndOpacity(Paper)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
					[SNew(STextBlock).Text_Lambda([this]() {
						const auto* Ranking = GetRankingSubsystem();
						return FText::FromString(Ranking ? Ranking->GetDisplayProgressLabel(GameMode.IsValid() ? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic,
							GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1) : TEXT("UNAVAILABLE"));
					}).Font(DisplayFont(16)).ColorAndOpacity(Brand).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
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
