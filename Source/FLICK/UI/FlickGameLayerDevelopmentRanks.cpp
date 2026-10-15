#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickDevelopmentRankSettings.h"
#include "Widgets/Input/SSpinBox.h"

#if !UE_BUILD_SHIPPING
TSharedRef<SWidget> SFlickGameLayer::BuildDevelopmentRankSettings()
{
	using namespace FlickDevelopmentRankSettings;
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Playlist = 0; Playlist < 4; ++Playlist)
	{
		const EFlickMatchVariant Variant = Playlist == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
		const int32 TeamSize = Playlist == 3 ? 1 : Playlist + 1;
		const auto Progress = [this, Variant, TeamSize]()
		{
			const auto* Ranking = GetRankingSubsystem();
			return Ranking ? Ranking->GetDisplayProgress(Variant, TeamSize) : FFlickRankProgress();
		};
		Rows->AddSlot().AutoHeight().Padding(0, 5)
		[SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(Hairline).CutSize(8).Padding(FMargin(18, 9))
		 [SNew(SHorizontalBox)
		  + SHorizontalBox::Slot().FillWidth(.27f).VAlign(VAlign_Center)
		  [SNew(SCheckBox).Style(&ToggleStyle).Padding(FMargin(10, 7))
		   .ToolTipText(FText::FromString(TEXT("Override this playlist's menu rank. Uncheck to show your real rank.")))
		   .IsChecked_Lambda([Variant, TeamSize]() { return GetRating(Variant, TeamSize).IsSet() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
		   .OnCheckStateChanged_Lambda([Progress, Variant, TeamSize](ECheckBoxState State)
		   { if (State == ECheckBoxState::Checked) SetRating(Variant, TeamSize, Progress().Rating); else ClearRating(Variant, TeamSize); })
		   [SNew(STextBlock).Text(FText::FromString(Playlist == 3 ? TEXT("BOB 1V1")
			: FString::Printf(TEXT("KNOCKOUT %dV%d"), TeamSize, TeamSize))).Font(UiFont(12, true))
			.ColorAndOpacity_Lambda([Variant, TeamSize]() { return GetRating(Variant, TeamSize).IsSet() ? Ink : Paper; })]]
		  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 0)
		  [SNew(SBox).WidthOverride(150)
		   [SNew(SSpinBox<int32>).Font(UiFont(14, true)).MinValue(0).MaxValue(FlickRankRules::MaximumRating)
			.MinSliderValue(0).MaxSliderValue(3000).Delta(1)
			.ToolTipText(FText::FromString(TEXT("Type an exact MMR and press Enter. Editing enables the display override.")))
			.Value_Lambda([Progress]() { return Progress().Rating; })
			.OnValueCommitted_Lambda([Variant, TeamSize](int32 Rating, ETextCommit::Type) { SetRating(Variant, TeamSize, Rating); })]]
		  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6, 0, 16, 0)
		  [SNew(SFlickRankBadge).Progress_Lambda(Progress).Size(60)]
		  + SHorizontalBox::Slot().FillWidth(.4f).VAlign(VAlign_Center)
		  [SNew(SVerticalBox)
		   + SVerticalBox::Slot().AutoHeight()
		   [SNew(STextBlock).Text_Lambda([Progress]() { return FText::FromString(FlickRankRules::GetRankLabel(Progress())); }).Font(UiFont(13, true)).ColorAndOpacity(Brand)]
		   + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		   [SNew(STextBlock).Text_Lambda([Progress]() { return FText::FromString(FlickRankRules::GetRatingRangeLabel(Progress())); }).Font(UiFont(10)).ColorAndOpacity(Muted)]]
		  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		  [SNew(SBox).WidthOverride(150)
		   [MakeMenuButton(TEXT("USE REAL RANK"), FOnClicked::CreateLambda([Variant, TeamSize]() { ClearRating(Variant, TeamSize); return FReply::Handled(); }), false, false, 38)]]
		 ]];
	}
	return SNew(SVerticalBox)
	 + SVerticalBox::Slot().AutoHeight()
	 [SNew(STextBlock).Text(FText::FromString(TEXT("DEVELOPMENT  /  COMPETITIVE RANKS"))).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
	 + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 12)
	 [SNew(STextBlock).Text(FText::FromString(TEXT("Menu display only. Type an MMR and press Enter to preview its badge on playlists, the main menu and your profile. Overrides bypass placements for display, are saved locally, and are unavailable in Shipping builds. Matchmaking MMR and real match results are never changed.")))
	  .AutoWrapText(true).Font(UiFont(11)).ColorAndOpacity(Muted)]
	 + SVerticalBox::Slot().AutoHeight()[Rows]
	 + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 12)
	 [SNew(SBox).WidthOverride(260)
	  [MakeMenuButton(TEXT("CLEAR ALL RANK OVERRIDES"), FOnClicked::CreateLambda([]() { ClearAll(); return FReply::Handled(); }), false, false, 44)]];
}
#endif
