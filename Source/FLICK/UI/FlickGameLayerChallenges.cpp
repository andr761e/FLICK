#include "UI/FlickGameLayerPrivate.h"

FFlickProfileStats SFlickGameLayer::GetChallengeStats() const
{
	const UFlickGameInstance* Instance = PlayerController.IsValid()
		? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
		: GameMode.IsValid() ? Cast<UFlickGameInstance>(GameMode->GetGameInstance()) : nullptr;
	return Instance ? Instance->GetProfileStats() : FFlickProfileStats();
}

void SFlickGameLayer::RefreshChallengePreview()
{
	if (!ChallengePreviewRows.IsValid()) return;
	ChallengePreviewRows->ClearChildren();
	const FFlickProfileStats Stats = GetChallengeStats();
	const TArray<int32> Featured = FlickChallengeCatalog::GetFeaturedIndices(Stats);
	for (const int32 Index : Featured)
	{
		ChallengePreviewRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[BuildChallengeRow(Index, true)];
	}
	if (Featured.IsEmpty())
	{
		ChallengePreviewRows->AddSlot().AutoHeight().Padding(0.0f, 16.0f)
		[SNew(STextBlock).Text(FText::FromString(TEXT("ALL CAREER CHALLENGES COMPLETE"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)];
	}
}

TSharedRef<SWidget> SFlickGameLayer::BuildChallengeRow(const int32 ChallengeIndex, const bool bCompact)
{
	using namespace FlickChallengeCatalog;
	const FDefinition& Challenge = Challenges[ChallengeIndex];
	const float BarWidth = bCompact ? 266.0f : 598.0f;
	static const FProgressBarStyle MenuProgressStyle = FProgressBarStyle()
		.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor::FromSRGBColor(FColor(62, 83, 94)), 2.5f))
		.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White, 2.5f))
		.SetMarqueeImage(FSlateNoResource());
	return SNew(SBox).HeightOverride(bCompact ? 84.0f : 104.0f)
	[
		SNew(SFlickMainMenuPanel)
		.Premium(bCompact)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(6, 17, 24, 245)))
		.AccentColor_Lambda([this, ChallengeIndex]()
		{
			const auto& Entry = FlickChallengeCatalog::Challenges[ChallengeIndex];
			return FlickChallengeCatalog::GetProgress(Entry, GetChallengeStats()) >= Entry.Goal ? Brand : Cyan;
		})
		.CutSize(9.0f).BorderWidth(1.0f).Padding(FMargin(bCompact ? 12.0f : 17.0f, bCompact ? 10.0f : 10.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, bCompact ? 12.0f : 0.0f, 0.0f)
			[
				SNew(SFlickMainMenuIcon).Icon(Challenge.Metric == EMetric::Shots ? EFlickMainMenuIcon::Target : Challenge.Metric == EMetric::Points ? EFlickMainMenuIcon::Stats : EFlickMainMenuIcon::Whistle)
				.Color(Challenge.Metric == EMetric::Points ? FlickMainMenuStyle::Ice : Paper).Glow(bCompact && Challenge.Metric == EMetric::Points)
				.Visibility(bCompact ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[SNew(STextBlock).Text(FText::FromString(Challenge.Title)).Font(UiFont(bCompact ? 11 : 15, true)).ColorAndOpacity(Paper)]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(STextBlock).Text_Lambda([this, ChallengeIndex]()
				{
					const auto& Entry = FlickChallengeCatalog::Challenges[ChallengeIndex];
					const int32 Progress = FMath::Min(FlickChallengeCatalog::GetProgress(Entry, GetChallengeStats()), Entry.Goal);
					return FText::FromString(FString::Printf(TEXT("%d / %d"), Progress, Entry.Goal));
				}).Font(UiFont(bCompact ? 9 : 12, true)).ColorAndOpacity(Brand)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bCompact ? 0.0f : 5.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(Challenge.Detail)).Font(UiFont(bCompact ? 9 : 11)).ColorAndOpacity(FLinearColor(0.57f, 0.68f, 0.72f)).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bCompact ? 5.0f : 10.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(bCompact ? 7.0f : 6.0f)
				[
					SNew(SProgressBar).Style(&MenuProgressStyle)
					.BarFillStyle(EProgressBarFillStyle::Scale)
					.FillColorAndOpacity(FlickMainMenuStyle::Lime)
					.Percent_Lambda([this, ChallengeIndex]() -> TOptional<float>
					{
						const auto& Entry = FlickChallengeCatalog::Challenges[ChallengeIndex];
						return FMath::Clamp(static_cast<float>(FlickChallengeCatalog::GetProgress(Entry, GetChallengeStats())) / Entry.Goal, 0.0f, 1.0f);
					})
				]
			]
			]
		]
	];
}

TSharedRef<SWidget> SFlickGameLayer::BuildChallengePreview()
{
	TSharedRef<SButton> ViewAllButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(15.0f, 6.0f))
		.OnClicked_Lambda([this]()
		{
			bSocialPanelOpen = false;
			bChallengesOpen = true;
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 13.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("\u2261"))).Font(UiFont(23)).ColorAndOpacity(Brand)]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("VIEW ALL CHALLENGES"))).Font(UiFont(14, true)).ColorAndOpacity(Paper)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("\u2192"))).Font(UiFont(17)).ColorAndOpacity(Paper)]
		];
	const TWeakPtr<SButton> WeakViewAll = ViewAllButton;
	return SNew(SFlickMainMenuPanel)
		.Premium(true)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(3, 11, 18, 248)))
		.AccentColor(Cyan)
		.CutSize(14.0f).BorderWidth(1.3f).Padding(FMargin(18.0f, 19.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(TEXT("CHALLENGES  //  NEAR COMPLETION"))).Font(WordmarkTaglineFont(9)).ColorAndOpacity(FlickMainMenuStyle::Ice)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 18.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR NEXT MILESTONES"))).Font(UiFont(19, true)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SAssignNew(ChallengePreviewRows, SVerticalBox)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(52.0f)
				[
					SNew(SFlickMainMenuPanel).Premium(true)
					.BackgroundColor(FLinearColor::FromSRGBColor(FColor(4, 13, 19, 248)))
					.AccentColor_Lambda([WeakViewAll]()
					{
						const TSharedPtr<SButton> Button = WeakViewAll.Pin();
						return Button && (Button->IsHovered() || Button->HasKeyboardFocus()) ? Cyan : Brand;
					})
					.CutSize(10.0f).BorderWidth(1.0f).Padding(FMargin(1.0f))[ViewAllButton]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildChallengesPanel()
{
	using namespace FlickChallengeCatalog;
	const TCHAR* CategoryNames[] = {TEXT("FIRST FLICKS"), TEXT("PRECISION"), TEXT("RIVALRY"), TEXT("EVENTS")};
	TSharedRef<SVerticalBox> Categories = SNew(SVerticalBox);
	for (int32 CategoryIndex = 0; CategoryIndex < UE_ARRAY_COUNT(CategoryNames); ++CategoryIndex)
	{
		Categories->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SBox).HeightOverride(55.0f)
			[
				SNew(SFlickMainMenuPanel)
				.BackgroundColor_Lambda([this, CategoryIndex]() { return SelectedChallengeCategory == CategoryIndex ? PanelRaised : Panel; })
				.AccentColor_Lambda([this, CategoryIndex]() { return SelectedChallengeCategory == CategoryIndex ? Brand : Hairline; })
				.CutSize(8.0f).BorderWidth(1.0f).Padding(FMargin(2.0f))
				[
					SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(11.0f, 6.0f))
					.OnClicked_Lambda([this, CategoryIndex]() { SelectedChallengeCategory = CategoryIndex; return FReply::Handled(); })
					[SNew(STextBlock).Text(FText::FromString(CategoryNames[CategoryIndex])).Font(UiFont(12, true)).ColorAndOpacity(Paper)]
				]
			]
		];
	}
	TSharedRef<SVerticalBox> ChallengeRows = SNew(SVerticalBox);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Challenges); ++Index)
	{
		const ECategory Category = Challenges[Index].Category;
		ChallengeRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this, Category]() { return SelectedChallengeCategory == static_cast<int32>(Category) ? EVisibility::Visible : EVisibility::Collapsed; })
			[BuildChallengeRow(Index, false)]
		];
	}
	ChallengeRows->AddSlot().AutoHeight().Padding(0.0f, 20.0f)
	[
		SNew(SBox).Visibility_Lambda([this]() { return SelectedChallengeCategory == static_cast<int32>(ECategory::Events) ? EVisibility::Visible : EVisibility::Collapsed; })
		[SNew(STextBlock).Text(FText::FromString(TEXT("NO ACTIVE EVENT  //  NEW EVENT CHALLENGES WILL APPEAR HERE"))).Font(UiFont(12, true)).ColorAndOpacity(Muted)]
	];
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.003f, 0.011f, 0.017f, 0.93f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1080.0f).HeightOverride(680.0f)
			[
				SNew(SFlickMainMenuPanel).BackgroundColor(Panel).AccentColor(Brand).CutSize(16.0f).BorderWidth(1.2f).Padding(FMargin(27.0f, 22.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK NETWORK  //  MILESTONES"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 17.0f)
					[SNew(STextBlock).Text(FText::FromString(TEXT("CHALLENGES"))).Font(DisplayFont(34)).ColorAndOpacity(Paper)]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 21.0f, 0.0f)
						[SNew(SBox).WidthOverride(230.0f)[Categories]]
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(SFlickMainMenuPanel).BackgroundColor(PanelRaised).AccentColor(Hairline).CutSize(10.0f).BorderWidth(1.0f).Padding(FMargin(20.0f, 17.0f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[SNew(STextBlock).Text_Lambda([this]()
								{
									static const TCHAR* Names[] = {TEXT("FIRST FLICKS"), TEXT("PRECISION"), TEXT("RIVALRY"), TEXT("EVENTS")};
									return FText::FromString(Names[FMath::Clamp(SelectedChallengeCategory, 0, 3)]);
								}).Font(DisplayFont(24)).ColorAndOpacity(Paper)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 17.0f)
								[SNew(STextBlock).Text_Lambda([this]()
								{
									return FText::FromString(SelectedChallengeCategory == static_cast<int32>(ECategory::Events)
										? TEXT("LIMITED EVENTS  //  NONE ACTIVE")
										: TEXT("PERMANENT CAREER PROGRESS  //  CASUAL + COMPETITIVE"));
								}).Font(UiFont(9, true)).ColorAndOpacity(Cyan)]
								+ SVerticalBox::Slot().FillHeight(1.0f)
								[SNew(SScrollBox) + SScrollBox::Slot()[ChallengeRows]]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 17.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[SNew(SBox).WidthOverride(160.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { bChallengesOpen = false; return FReply::Handled(); }))]]
						+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(TEXT("PROGRESS SAVES WITH YOUR LOCAL PROFILE"))).Font(UiFont(9)).ColorAndOpacity(Muted)]
					]
				]
			]
		];
}
