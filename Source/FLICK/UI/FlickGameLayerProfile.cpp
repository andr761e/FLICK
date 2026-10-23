#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickPlaylistWidgets.h"
#include "UI/FlickProfileStyle.h"
#include "UI/FlickProfileWidgets.h"

namespace
{
	// Native, data-driven card decoration. Empty profiles show an empty chart /
	// muted timeline, never fictional results baked into an illustration.
	class SProfileActivity final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SProfileActivity) : _History(false) {}
			SLATE_ARGUMENT(bool, History)
			SLATE_ATTRIBUTE(TArray<FFlickProfileMatchRecord>, Records)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args)
		{
			bHistory = Args._History; Records = Args._Records;
			SetCanTick(false); ForceVolatile(Records.IsBound());
		}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(218, 118); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& E,
			int32 L, const FWidgetStyle& Style, bool) const override
		{
			const FVector2D Size = G.GetLocalSize();
			const auto Matches = Records.Get();
			const FLinearColor Tint = Style.GetColorAndOpacityTint();
			const auto Line = [&](TArray<FVector2D> Points, FLinearColor Color, float Width = 1.f) {
				FSlateDrawElement::MakeLines(E, L, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color * Tint, true, Width);
			};
			const auto Box = [&](FVector2D P, FVector2D S, FLinearColor Color) {
				FSlateDrawElement::MakeBox(E, L, G.ToPaintGeometry(S, FSlateLayoutTransform(P)), WhiteBrush(), ESlateDrawEffect::None, Color * Tint);
			};
			const auto Text = [&](FVector2D P, const FString& Value, FLinearColor Color, int32 FontSize=8) {
				FSlateDrawElement::MakeText(E,L,G.ToPaintGeometry(FVector2D(Size.X,18),FSlateLayoutTransform(P)),
					FText::FromString(Value),DisplayFont(FontSize),ESlateDrawEffect::None,Color*Tint);
			};
			if (bHistory)
			{
				for (int32 I=0; I<4; ++I)
				{
					const float Y=I*(Size.Y-6)/4;
					const bool bPresent=Matches.IsValidIndex(I);
					const FLinearColor Color=!bPresent ? Muted.CopyWithNewOpacity(.2f)
						: Matches[I].bWon ? Cyan : Matches[I].bDraw ? Muted : Orange;
					TArray<FVector2D> Pip;
					for (int P=0;P<=24;++P) { const float A=(P%24)*UE_TWO_PI/24; Pip.Add(FVector2D(8,Y+12)+FVector2D(FMath::Cos(A),FMath::Sin(A))*4); }
					Line(Pip,Color.CopyWithNewOpacity(bPresent ? .75f : .5f),1.4f);
					if (bPresent)
					{
						const auto& Match=Matches[I];
						Text({16,Y+3},FString::Printf(TEXT("%s  //  %dV%d"),Match.bWon ? TEXT("WIN") : Match.bDraw ? TEXT("DRAW") : TEXT("LOSS"),
							Match.PlayersPerTeam,Match.PlayersPerTeam),Color.CopyWithNewOpacity(.8f));
						Text({Size.X-56,Y+3},FString::Printf(TEXT("%d PTS"),Match.Points),Muted.CopyWithNewOpacity(.75f));
					}
					else
					{
						Box({26,Y+8},{Size.X-34,5},Muted.CopyWithNewOpacity(.16f));
						Box({26,Y+17},{Size.X*.5,3},Muted.CopyWithNewOpacity(.07f));
					}
					if (!Matches.IsEmpty()) Line({{26,Y+25},{Size.X-4,Y+25}},Cyan.CopyWithNewOpacity(.08f),.7f);
				}
			}
			else
			{
				TArray<int32> Points;
				for (int32 I=0; I<FMath::Min(Matches.Num(),8); ++I) Points.Add(Matches[I].Points);
				const auto Bars=FlickProfileStyle::NormalizePoints(Points);
				Text({4,0},TEXT("RECENT MATCH POINTS"),Muted.CopyWithNewOpacity(.46f));
				const float Bottom=Size.Y-10, Top=24;
				for (int32 I=0; I<4; ++I) {
					const float Y=FMath::Lerp(Top,Bottom,I/3.f);
					Line({{4,Y},{Size.X-4,Y}},Cyan.CopyWithNewOpacity(I==3 ? .26f : .08f),.7f);
				}
				const float Step=(Size.X-16)/8;
				for (int32 I=0; I<Bars.Num(); ++I)
				{
					const float X=8+I*Step, H=FMath::Max(1.f,Bars[I]*(Bottom-Top-4)), W=Step*.58f;
					FSlateDrawElement::MakeGradient(E,L,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Bottom-H))),
						{FSlateGradientStop(FVector2f(0,0),Cyan.CopyWithNewOpacity(.58f)*Tint),
						FSlateGradientStop(FVector2f(0,H),Cyan.CopyWithNewOpacity(.055f)*Tint)},Orient_Vertical);
					Line({{X,Bottom-H},{X+W,Bottom-H}},Ice.CopyWithNewOpacity(.65f),1.f);
				}
				if (Bars.IsEmpty()) Text({34,56},TEXT("NO MATCH DATA"),Muted.CopyWithNewOpacity(.26f),9);
			}
			return L;
		}
	private:
		bool bHistory = false;
		TAttribute<TArray<FFlickProfileMatchRecord>> Records;
	};

	TSharedRef<SWidget> ProfileRule()
	{
		return SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.14f))];
	}
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfileOverview()
{
	const auto Instance = [this]() { return PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; };
	const auto Records = TAttribute<TArray<FFlickProfileMatchRecord>>::CreateLambda([Instance]() {
		return Instance() ? Instance()->GetRecentMatches() : TArray<FFlickProfileMatchRecord>();
	});
	const auto Progress = [this]() { const auto* Ranking = GetRankingSubsystem();
		return Ranking ? Ranking->GetDisplayProgress(EFlickMatchVariant::Classic, 1) : FFlickRankProgress(); };
	const auto MakeCard = [this, Instance, Records, Progress](int32 Order, EFlickProfileTab Page,
		const TCHAR* Title, const TCHAR* Description, EFlickMainMenuIcon Icon) -> TSharedRef<SWidget>
	{
		TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("ProfileChoice%d"),Order)))
			.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.RenderTransform_Lambda([this,Order]() {
				const float Alpha = FMath::Clamp((ProfileEntranceElapsed-Order*.075f)/.42f,0.f,1.f);
				const float Offset = FMath::Pow(1-Alpha,3.f);
				return FSlateRenderTransform(FVector2D((Order%2 ? 1.f : -1.f)*Offset*62,Offset*24));
			})
			.OnClicked_Lambda([this,Page]() { SelectedProfileTab=Page; if (GameMode.IsValid()) GameMode->PlayMenuSound(true); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak = Button;
		const auto Active = [Weak]() { const auto B = Weak.Pin(); return B && (B->IsHovered() || B->HasKeyboardFocus()); };
		TSharedRef<SOverlay> Art = SNew(SOverlay).Visibility(EVisibility::HitTestInvisible);
		if (Order == 1 || Order == 3)
		{
			Art->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,1,1)
			[SNew(SBox).WidthOverride(380).HeightOverride(214)
				.Visibility_Lambda([Order,Progress]() { return Order==1 && Progress().MatchesPlayed<FlickRankRules::PlacementMatchCount ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
				[SNew(SFlickMenuArtwork).Artwork(Order == 1 ? EFlickMenuArtwork::ProfileRankStage : EFlickMenuArtwork::ProfileCustomize)
					.CutSize(12).EdgeFade(FMargin(Order==1 ? 112.f : 150.f,Order==1 ? 76.f : 96.f,16,16))]];
			if (Order == 1)
			{
				Art->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,1,1)
				[SNew(SBox).WidthOverride(380).HeightOverride(214)
					.Visibility_Lambda([Progress]() { return Progress().MatchesPlayed<FlickRankRules::PlacementMatchCount ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
					[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::ProfilePlacement).CutSize(12).EdgeFade(FMargin(110,60,8,8))]];
				Art->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,53,26)
				[SNew(SBox).WidthOverride(92).HeightOverride(92)
					.Visibility_Lambda([Progress]() { return Progress().MatchesPlayed>=FlickRankRules::PlacementMatchCount ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
					[SNew(SFlickRankBadge).Progress_Lambda(Progress).Size(92)]];
			}
		}
		else
		{
			Art->AddSlot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,24,22)
			[SNew(SBox).WidthOverride(208).HeightOverride(114)
				[SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SProfileActivity).History(Order==2).Records(Records)
						.Visibility_Lambda([Order,Records]() { return Order==0 && Records.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
					// Editorial empty-state artwork; only the other branch plots recorded points.
					+ SOverlay::Slot()[SNew(FlickProfileWidgets::SStatsIllustration).Tag(TEXT("ProfileStatsIllustration"))
						.Visibility_Lambda([Order,Records]() { return Order==0 && Records.Get().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]]];
		}
		const auto Summary = [Order,Instance,Progress]() -> FText {
			const auto* I = Instance();
			if (Order == 0) return FText::FromString(FString::Printf(TEXT("%d MATCHES"), I ? I->GetProfileStats().MatchesPlayed : 0));
			if (Order == 2) return FText::FromString(FString::Printf(TEXT("%d RECENT SERIES"), I ? I->GetRecentMatches().Num() : 0));
			if (Order == 3) return FText::FromString(TEXT("9 PUCK TYPES"));
			const auto Rank = Progress();
			return FText::FromString(Rank.MatchesPlayed < FlickRankRules::PlacementMatchCount
				? FString::Printf(TEXT("PLACEMENT  //  %d / %d"),Rank.MatchesPlayed,FlickRankRules::PlacementMatchCount)
				: FlickRankRules::GetRankLabel(Rank));
		};
		const auto Detail = [this,Order,Instance,Progress]() -> FText {
			const auto* I = Instance();
			if (Order == 3) return FText::FromString(TEXT("6 finishes each.\nMake every puck yours."));
			if (Order == 1) {
				const auto* Ranking = GetRankingSubsystem();
				const auto Rank = Progress();
				return FText::FromString(FString::Printf(TEXT("1V1 KNOCKOUT  //  %d MMR\n%s"),Rank.Rating,
					Ranking && Ranking->GetDisplayProgressLabel(EFlickMatchVariant::Classic,1).StartsWith(TEXT("DEV")) ? TEXT("Development preview")
					: Rank.MatchesPlayed < FlickRankRules::PlacementMatchCount ? TEXT("Complete your placements.") : TEXT("Keep building your rank.")));
			}
			if (!I || (Order == 0 ? I->GetProfileStats().MatchesPlayed == 0 : I->GetRecentMatches().IsEmpty()))
				return FText::FromString(TEXT("Your next match starts\nyour story."));
			if (Order == 0) return FText::FromString(FString::Printf(TEXT("%d WINS  //  %d POINTS\nRecent match points shown."), I->GetProfileStats().Wins, I->GetProfileStats().Points));
			return FText::FromString(TEXT("Your latest results,\nready to revisit."));
		};
		Button->SetContent(SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(15).UseAccentForOutline(true).BorderWidth(.8f)
				.AccentColor_Lambda([Active]() { return Active() ? Brand : Hairline.CopyWithNewOpacity(.7f); })
				.BackgroundColor_Lambda([Active]() { return Active() ? FlickProfileStyle::SurfaceHover : FlickProfileStyle::Surface; }).Padding(0)[Art]]
			+ SOverlay::Slot().Padding(FlickProfileStyle::CardPadding,24)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0,4,24,0)
					[SNew(SBox).WidthOverride(50).HeightOverride(50)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
						[SNew(FlickProfileWidgets::SGlyph).Kind(Order).Color(Order==0 ? Ice : Paper)]]]
					+ SHorizontalBox::Slot().FillWidth(1)
					[SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Title)).Font(DisplayFont(23)).ColorAndOpacity(Paper)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,6,20,0)
						[SNew(STextBlock).Text(FText::FromString(Description)).Font(UiFont(12)).ColorAndOpacity(Muted).AutoWrapText(true)]]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
					[SNew(FlickPlaylistWidgets::SCardArrow).Divider(false).Active_Lambda(Active)]]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,22,0,0)[ProfileRule()]
				+ SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Bottom).Padding(6,14,0,2)
				[SNew(SBox).WidthOverride(230).HAlign(HAlign_Left)
					[SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda(Summary).Font(DisplayFont(19))
							.ColorAndOpacity(Order==1 ? Brand : Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)
						[SNew(STextBlock).Text_Lambda(Detail).Font(UiFont(11)).ColorAndOpacity(Muted).WrapTextAt(212)]]]]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).Active_Lambda(Active)]);
		return Button;
	};
	const auto Cards = SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1).Padding(0,0,0,FlickProfileStyle::CardGap/2)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).Padding(0,0,FlickProfileStyle::CardGap/2,0)
			[MakeCard(0,EFlickProfileTab::Stats,TEXT("STATS"),TEXT("Your performance across\nevery playlist."),EFlickMainMenuIcon::Stats)]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(FlickProfileStyle::CardGap/2,0,0,0)
			[MakeCard(1,EFlickProfileTab::Leaderboards,TEXT("COMPETITIVE RANKS"),TEXT("Your placement, MMR\nand the rank ladder."),EFlickMainMenuIcon::Leaderboard)]]
		+ SVerticalBox::Slot().FillHeight(1).Padding(0,FlickProfileStyle::CardGap/2,0,0)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).Padding(0,0,FlickProfileStyle::CardGap/2,0)
			[MakeCard(2,EFlickProfileTab::MatchHistory,TEXT("MATCH HISTORY"),TEXT("Revisit your most recent series."),EFlickMainMenuIcon::History)]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(FlickProfileStyle::CardGap/2,0,0,0)
			[MakeCard(3,EFlickProfileTab::Customization,TEXT("CUSTOMIZE"),TEXT("Identity, puck skins and live effects."),EFlickMainMenuIcon::Brush)]];

	TSharedRef<SButton> Back = SNew(SButton).Tag(TEXT("ProfileBack")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() { if (GameMode.IsValid()) GameMode->CloseProfile(); else RemotePartyScreen=EFlickFrontendScreen::MainMenu; return FReply::Handled(); });
	Back->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(Back)));
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::ProfileBackdrop).Visibility(EVisibility::HitTestInvisible)]
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0,0,0,.14f)).Visibility(EVisibility::HitTestInvisible)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(FlickProfileStyle::ContentWidth).HeightOverride(FlickProfileStyle::ContentHeight)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(SBox).HeightOverride(62)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(SBox).WidthOverride(192).HeightOverride(62)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
							[SNew(SFlickLogoWidget).VectorPath(TEXT("UI/FlickKnockoutWordmark.svg"))
								.TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark")).DesiredSize(FVector2D(2048,683))]]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(26,0)
						[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(DisplayFont(16)).ColorAndOpacity(Cyan)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(TEXT("HOME"))).Font(DisplayFont(14)).ColorAndOpacity(Muted)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(13,0)
						[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(DisplayFont(14)).ColorAndOpacity(Cyan)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE"))).Font(DisplayFont(14)).ColorAndOpacity(Brand)]]]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,27,0,24)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR RECORD. YOUR PROGRESS."))).Font(DisplayFont(38)).ColorAndOpacity(Paper)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)
					[SNew(STextBlock).Text(FText::FromString(TEXT("Every shot leaves a mark. Where do you want to look?"))).Font(UiFont(13)).ColorAndOpacity(Muted)]]
				+ SVerticalBox::Slot().FillHeight(1)
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(FlickProfileStyle::IdentityWidth)[BuildProfileIdentityCard(true)]]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(FlickProfileStyle::IdentityGap,0,0,0)[Cards]]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,26,0,0)
				[SNew(SBox).WidthOverride(180).HeightOverride(52).HAlign(HAlign_Left)[Back]]]];
}
