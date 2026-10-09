#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickPracticeCatalog.h"
#include "Game/FlickPracticeComponent.h"

TSharedRef<SWidget> SFlickGameLayer::BuildPracticeMenu()
{
	auto Categories = SNew(SUniformGridPanel).SlotPadding(8);
	for (int32 Category = 0; Category < FlickPracticeCatalog::CategoryCount; ++Category)
	{
		Categories->AddSlot(Category % 2, Category / 2)
		[
			SNew(SButton).ButtonStyle(&MenuButtonStyle).ContentPadding(24)
			.OnClicked_Lambda([this, Category]() { SelectedPracticeCategory = Category; return FReply::Handled(); })
			[
				SNew(SBox).HeightOverride(184)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(FlickPracticeCatalog::Name(Category))).Font(DisplayFont(28)).ColorAndOpacity(Paper)]
					+ SVerticalBox::Slot().FillHeight(1).Padding(0, 12)
					[SNew(STextBlock).Text(FText::FromString(FlickPracticeCatalog::Description(Category))).Font(UiFont(12)).ColorAndOpacity(Muted).AutoWrapText(true)]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(UiFont(11, true)).ColorAndOpacity(Brand)
						.Text_Lambda([Category]() { return FText::FromString(FString::Printf(TEXT("FOUNDATION %d/5   |   SKILLED %d/5   |   EXPERT %d/5"),
							FlickPracticeCatalog::BestScore(Category, 0), FlickPracticeCatalog::BestScore(Category, 1), FlickPracticeCatalog::BestScore(Category, 2))); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
					[SNew(STextBlock).Text(FText::FromString(TEXT("CHOOSE DIFFICULTY  >"))).Font(UiFont(10, true)).ColorAndOpacity(Cyan)]
				]
			]
		];
	}
	auto Difficulties = SNew(SHorizontalBox);
	for (int32 Difficulty = 0; Difficulty < FlickPracticeCatalog::DifficultyCount; ++Difficulty)
	{
		Difficulties->AddSlot().FillWidth(1).Padding(8)
		[
			SNew(SButton).ButtonStyle(&MenuButtonStyle).ContentPadding(24)
			.OnClicked_Lambda([this, Difficulty]() { if (GameMode.IsValid()) GameMode->StartPractice(SelectedPracticeCategory, Difficulty); return FReply::Handled(); })
			[
				SNew(SBox).HeightOverride(270)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(FlickPracticeCatalog::DifficultyName(Difficulty))).Font(DisplayFont(26)).ColorAndOpacity(Paper)]
					+ SVerticalBox::Slot().FillHeight(1).Padding(0, 20)
					[
						SNew(STextBlock).Font(UiFont(13)).ColorAndOpacity(Muted).AutoWrapText(true)
						.Text(FText::FromString(Difficulty == 0 ? TEXT("Generous targets and straightforward approaches. Build consistent fundamentals.")
							: Difficulty == 1 ? TEXT("Longer shots and angled approaches. Keep friendly pucks safe.")
							: TEXT("Tighter precision and tougher cuts. Switch Control requires exactly one switch.")))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(DisplayFont(23)).ColorAndOpacity(Brand)
						.Text_Lambda([this, Difficulty]() { const int32 Best = FlickPracticeCatalog::BestScore(SelectedPracticeCategory, Difficulty);
							return FText::FromString(Best == 5 ? TEXT("MASTERED  5 / 5") : FString::Printf(TEXT("BEST  %d / 5"), Best)); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
					[SNew(STextBlock).Text(FText::FromString(TEXT("START FIVE-SHOT RUN  >"))).Font(UiFont(11, true)).ColorAndOpacity(Cyan)]
				]
			]
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 18)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[SNew(STextBlock).Font(DisplayFont(26)).ColorAndOpacity(Paper).Text_Lambda([this]() { return FText::FromString(
				SelectedPracticeCategory >= 0 ? FlickPracticeCatalog::Name(SelectedPracticeCategory) : TEXT("CHOOSE YOUR FOCUS")); })]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity(Brand).Text_Lambda([]() { return FText::FromString(FString::Printf(TEXT("MASTERY  %d / 12 PACKS"), FlickPracticeCatalog::PerfectPacks())); })]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(SBox).Visibility_Lambda([this]() { return SelectedPracticeCategory < 0 ? EVisibility::Visible : EVisibility::Collapsed; })[Categories]]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(SBox).Visibility_Lambda([this]() { return SelectedPracticeCategory >= 0 ? EVisibility::Visible : EVisibility::Collapsed; })[Difficulties]]
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 18)
		[SNew(STextBlock).Font(UiFont(12)).ColorAndOpacity(Muted).AutoWrapText(true).Text(FText::FromString(TEXT("One attempt per challenge. Misses advance too. Your best completed run is saved for each difficulty; 5/5 earns mastery. Restart begins a fresh run.")))];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPracticeOverlay()
{
	const auto Practice = [this]() -> UFlickPracticeComponent* { return GameMode.IsValid() ? GameMode->GetPractice() : nullptr; };
	auto Results = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < FlickPracticeCatalog::ChallengesPerRun; ++Index)
	{
		Results->AddSlot().FillWidth(1).Padding(2)
		[
			SNew(SBorder).BorderImage(WhiteBrush()).Padding(6)
			.BorderBackgroundColor_Lambda([Practice, Index]() { const int32 Result = Practice() ? Practice()->GetResult(Index) : -1;
				return Result < 0 ? PanelRaised : Result ? FLinearColor(.08f, .22f, .02f, 1) : FLinearColor(.28f, .05f, .02f, 1); })
			[
				SNew(STextBlock).Font(UiFont(9, true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)
				.Text_Lambda([Practice, Index]() { const int32 Result = Practice() ? Practice()->GetResult(Index) : -1;
					return FText::FromString(Result < 0 ? FString::FromInt(Index + 1) : Result ? TEXT("PASS") : TEXT("MISS")); })
			]
		];
	}
	return SNew(SBox).WidthOverride(392)
		.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsPracticeMode() && GameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing
			? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		[
			SNew(SFlickAngularBorder).BackgroundColor(FLinearColor(.003f, .014f, .024f, .94f)).AccentColor(Brand).CutSize(12).Padding(20)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(UiFont(10, true)).ColorAndOpacity(Brand).Text_Lambda([Practice]() { return FText::FromString(Practice()
					? FString::Printf(TEXT("PRACTICE  /  %s"), FlickPracticeCatalog::DifficultyName(Practice()->GetDifficulty())) : FString()); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
				[SNew(STextBlock).Font(DisplayFont(25)).ColorAndOpacity(Paper).Text_Lambda([Practice]() { return FText::FromString(Practice() ? FlickPracticeCatalog::Name(Practice()->GetCategory()) : TEXT("PRACTICE")); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 10)
				[SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity(Cyan).Text_Lambda([Practice]() { return FText::FromString(Practice()
					? FString::Printf(TEXT("%s  /  SCORE %d OF 5"), Practice()->IsComplete() ? TEXT("RUN COMPLETE") : *FString::Printf(TEXT("CHALLENGE %d"), Practice()->GetChallengeNumber()), Practice()->GetScore()) : FString()); })]
				+ SVerticalBox::Slot().AutoHeight()[Results]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 14)
				[SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity(Paper).AutoWrapText(true).Text_Lambda([Practice]() { return FText::FromString(Practice() ? Practice()->GetObjective() : FString()); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
				[SNew(STextBlock).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true).Text_Lambda([Practice]() { return FText::FromString(Practice() ? Practice()->GetFeedback() : FString()); })]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
					[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).OnClicked_Lambda([this]() { if (GameMode.IsValid()) GameMode->RestartMatch(); return FReply::Handled(); })
						[SNew(STextBlock).Font(UiFont(10, true)).ColorAndOpacity(Paper).Text(FText::FromString(TEXT("RESTART RUN")))]]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(4, 0, 0, 0)
					[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).OnClicked_Lambda([this, Practice]() {
						SelectedPracticeCategory = Practice() ? Practice()->GetCategory() : INDEX_NONE;
						SelectedPlayPlaylist = EFlickPlayPlaylist::Training; SelectedTrainingActivity = EFlickTrainingActivity::PracticePacks;
						if (GameMode.IsValid()) { GameMode->ReturnToMainMenu(); GameMode->OpenModeSelect(); }
						return FReply::Handled(); })[SNew(STextBlock).Font(UiFont(10, true)).ColorAndOpacity(Paper).Text(FText::FromString(TEXT("PRACTICE PACKS")))]]
				]
			]
		];
}
