#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickDevelopmentRankSettings.h"

TSharedRef<SWidget> SFlickGameLayer::BuildRankedProfilePanel()
{
	static const FProgressBarStyle RankProgressStyle = FProgressBarStyle()
		.SetBackgroundImage(FSlateColorBrush(Ink)).SetFillImage(FSlateColorBrush(Paper)).SetMarqueeImage(FSlateNoResource());
	TSharedRef<SUniformGridPanel> Playlists = SNew(SUniformGridPanel).SlotPadding(FMargin(7));
	for (int32 Playlist = 0; Playlist < 4; ++Playlist)
	{
		const EFlickMatchVariant Variant = Playlist == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
		const int32 TeamSize = Playlist == 3 ? 1 : Playlist + 1;
		const auto Progress = [this, Variant, TeamSize]()
		{
			const auto* Ranking = GetRankingSubsystem();
			return Ranking ? Ranking->GetDisplayProgress(Variant, TeamSize) : FFlickRankProgress();
		};
		Playlists->AddSlot(Playlist, 0)
		[SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Cyan).UseAccentForOutline(true)
		 .CutSize(10).BorderWidth(1).Padding(FMargin(18, 14))
		 [SNew(SVerticalBox)
		  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		  [SNew(STextBlock).Text(FText::FromString(Playlist == 3 ? TEXT("BOB 1V1")
			: FString::Printf(TEXT("KNOCKOUT %dV%d"), TeamSize, TeamSize))).Font(UiFont(10, true)).ColorAndOpacity(Muted)]
		  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6)
		  [SNew(SBox).WidthOverride(82).HeightOverride(82)[SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SFlickRankBadge).Progress_Lambda(Progress).Size(82)]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("?"))).Font(DisplayFont(28)).ColorAndOpacity(Muted)
				.Visibility_Lambda([Progress]() { return Progress().MatchesPlayed < 5 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]]]
		  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		  [SNew(STextBlock).Text_Lambda([Progress]() { return FText::FromString(FlickRankRules::GetRankLabel(Progress())); })
			.Font(DisplayFont(21)).ColorAndOpacity(Paper)]
		  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
		  [SNew(STextBlock).Text_Lambda([Progress, Variant, TeamSize]()
		   {
			const auto Rank = Progress();
			return FText::FromString(Rank.MatchesPlayed < FlickRankRules::PlacementMatchCount
				? FString::Printf(TEXT("PLACEMENTS %d / 5  |  %d MMR"), Rank.MatchesPlayed, Rank.Rating)
				: FString::Printf(TEXT("%s%d MMR"), FlickDevelopmentRankSettings::GetRating(Variant, TeamSize).IsSet() ? TEXT("DEV / ") : TEXT(""), Rank.Rating));
		   }).Font(DisplayFont(12)).ColorAndOpacity(FlickHomeSurface::Gold)]
		  + SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)
		  [SNew(SBox).HeightOverride(4)[SNew(SProgressBar).Style(&RankProgressStyle).FillColorAndOpacity(Cyan)
			.Percent_Lambda([Progress]() -> TOptional<float> {
				const auto Rank=Progress();
				if (Rank.MatchesPlayed<5) return FMath::Clamp(Rank.MatchesPlayed/5.f,0.f,1.f);
				const int32 Badge=FlickRankRules::GetBadgeIndex(Rank);
				if (Badge>=FlickRankRules::BadgeCount-1) return 1.f;
				const int32 Start=FlickRankRules::GetBadgeProgress(Badge).Rating, End=FlickRankRules::GetBadgeProgress(Badge+1).Rating;
				return FMath::Clamp((Rank.Rating-Start)/float(FMath::Max(1,End-Start)),0.f,1.f);
			})]]
		 ]];
	}

	TSharedRef<SUniformGridPanel> Ladder = SNew(SUniformGridPanel).SlotPadding(FMargin(7));
	for (int32 Tier = 0; Tier < 8; ++Tier)
	{
		TSharedRef<SHorizontalBox> Symbols = SNew(SHorizontalBox);
		for (int32 Division = 0; Division < (Tier == 7 ? 1 : 3); ++Division)
		{
			const FFlickRankProgress Rank = FlickRankRules::GetBadgeProgress(Tier * 3 + Division);
			Symbols->AddSlot().FillWidth(1)
			[SNew(SVerticalBox)
			 + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			 [SNew(SFlickRankBadge).Progress(Rank).Size(44)]
			 + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 3)
			 [SNew(STextBlock).Text(FText::FromString(FlickRankRules::GetDivisionName(FlickRankRules::GetDivision(Rank))))
			  .Font(UiFont(9, true)).ColorAndOpacity(Paper)]
			 + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			 [SNew(STextBlock).Text(FText::FromString(FlickRankRules::GetRatingRangeLabel(Rank).Replace(TEXT(" MMR"), TEXT(""))))
			  .Font(UiFont(8)).ColorAndOpacity(Muted)]
			];
		}
		Ladder->AddSlot(Tier % 4, Tier / 4)
		[SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Hairline)
		 .CutSize(8).BorderWidth(1).Padding(FMargin(9, 8))
		 [SNew(SVerticalBox)
		  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 4)
		  [SNew(STextBlock).Text(FText::FromString(FlickRankRules::GetTierName(static_cast<EFlickRankTier>(Tier + 1))))
		   .Font(DisplayFont(15)).ColorAndOpacity(Paper)]
		  + SVerticalBox::Slot().AutoHeight()[Symbols]
		 ]];
	}
	// Fit the complete ladder in its reserved profile body. The bounded width
	// gives wrapped copy a stable desired height; smaller viewports scale down
	// as a unit rather than clipping the footer or reintroducing a scroll bar.
	return SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
	[SNew(SBox).WidthOverride(1468)
	[SNew(SVerticalBox).Tag(TEXT("RankedProfileContents"))
	 + SVerticalBox::Slot().AutoHeight().Padding(5, 0, 5, 7)
	 [SNew(STextBlock).Text(FText::FromString(TEXT("YOUR COMPETITIVE RANKS"))).Font(DisplayFont(20)).ColorAndOpacity(Paper)]
	 + SVerticalBox::Slot().AutoHeight()[Playlists]
	 + SVerticalBox::Slot().AutoHeight().Padding(5, 6)
	 [SNew(STextBlock).Text(FText::FromString(TEXT("Five placements per playlist. Win to climb; tougher opponents award more MMR. Personal stats never affect rating.")))
	  .AutoWrapText(true).Font(UiFont(10)).ColorAndOpacity(Muted)]
	 + SVerticalBox::Slot().AutoHeight().Padding(5, 7)
	 [SNew(STextBlock).Text(FText::FromString(TEXT("THE RANK LADDER  //  MMR RANGES"))).Font(DisplayFont(20)).ColorAndOpacity(BrandHover)]
	 + SVerticalBox::Slot().AutoHeight()[Ladder]
	 + SVerticalBox::Slot().AutoHeight().Padding(5, 6)
	 [SNew(STextBlock).Tag(TEXT("RankedProfileFooter")).Text(FText::FromString(TEXT("Ratings are separate for each playlist. Zenith is a single rank at 2400+ MMR. Global leaderboards require the live service.")))
	  .AutoWrapText(true).Font(UiFont(9)).ColorAndOpacity(Muted)]
	]];
}
