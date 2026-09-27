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
		ChallengePreviewRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
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
	const float BarWidth = bCompact ? 272.0f : 598.0f;
	return SNew(SBox).HeightOverride(bCompact ? 65.0f : 104.0f)
	[
		SNew(SFlickMainMenuPanel)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(15, 30, 34, 246)))
		.AccentColor_Lambda([this, ChallengeIndex]()
		{
			const auto& Entry = FlickChallengeCatalog::Challenges[ChallengeIndex];
			return FlickChallengeCatalog::GetProgress(Entry, GetChallengeStats()) >= Entry.Goal ? Brand : Cyan;
		})
		.CutSize(7.0f).BorderWidth(1.0f).Padding(FMargin(bCompact ? 10.0f : 17.0f, bCompact ? 5.0f : 10.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[SNew(STextBlock).Text(FText::FromString(Challenge.Title)).Font(UiFont(bCompact ? 10 : 15, true)).ColorAndOpacity(Paper)]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(STextBlock).Text_Lambda([this, ChallengeIndex]()
				{
					const auto& Entry = FlickChallengeCatalog::Challenges[ChallengeIndex];
					const int32 Progress = FMath::Min(FlickChallengeCatalog::GetProgress(Entry, GetChallengeStats()), Entry.Goal);
					return FText::FromString(FString::Printf(TEXT("%d / %d"), Progress, Entry.Goal));
				}).Font(UiFont(bCompact ? 9 : 12, true)).ColorAndOpacity(Brand)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bCompact ? 0.0f : 5.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(Challenge.Detail)).Font(UiFont(bCompact ? 8 : 11)).ColorAndOpacity(Muted)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, bCompact ? 5.0f : 10.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(bCompact ? 4.0f : 6.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.55f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox).WidthOverride_Lambda([this, ChallengeIndex, BarWidth]()
						{
							const auto& Entry = FlickChallengeCatalog::Challenges[ChallengeIndex];
							return BarWidth * FMath::Clamp(static_cast<float>(FlickChallengeCatalog::GetProgress(Entry, GetChallengeStats())) / Entry.Goal, 0.0f, 1.0f);
						})
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)]
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SFlickGameLayer::BuildChallengePreview()
{
	return SNew(SFlickMainMenuPanel)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(9, 20, 24, 242)))
		.AccentColor(Cyan)
		.CutSize(12.0f).BorderWidth(1.1f).Padding(FMargin(13.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(TEXT("CHALLENGES  //  NEAR COMPLETION"))).Font(UiFont(10, true)).ColorAndOpacity(Cyan)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR NEXT MILESTONES"))).Font(DisplayFont(20)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SAssignNew(ChallengePreviewRows, SVerticalBox)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(SBox).HeightOverride(38.0f)
				[MakeMenuButton(TEXT("VIEW ALL CHALLENGES"), FOnClicked::CreateLambda([this]()
				{
					bSocialPanelOpen = false;
					bChallengesOpen = true;
					return FReply::Handled();
				}), false, false, 38.0f)]
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
										: TEXT("PERMANENT CAREER PROGRESS  //  ALL MODES"));
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
