#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickPlaylistWidgets.h"
#include "UI/FlickProfileWidgets.h"

namespace
{
    class SDetailBar final : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(SDetailBar) : _Color(Cyan) {} SLATE_ATTRIBUTE(FLinearColor, Color) SLATE_END_ARGS()
        void Construct(const FArguments& Args) { Color=Args._Color; SetCanTick(false); }
        virtual FVector2D ComputeDesiredSize(float) const override { return {40,160}; }
        virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,int32 L,const FWidgetStyle& Style,bool) const override
        {
            const FVector2D Size=G.GetLocalSize();
            if (Size.Y<=0 || Size.X<=0) return L;
            const FLinearColor Tint=Color.Get()*Style.GetColorAndOpacityTint();
            FSlateDrawElement::MakeGradient(E,L,G.ToPaintGeometry(),
                {FSlateGradientStop(FVector2f(0,0),Tint.CopyWithNewOpacity(.9f)),FSlateGradientStop(FVector2f(0,Size.Y),Tint.CopyWithNewOpacity(.16f))},Orient_Vertical);
            FSlateDrawElement::MakeLines(E,L+1,G.ToPaintGeometry(),TArray<FVector2D>{{0,0},{Size.X,0}},ESlateDrawEffect::None,
                FMath::Lerp(Tint,Paper,.4f),true,1.5f);
            return L+1;
        }
    private:
        TAttribute<FLinearColor> Color;
    };

    TSharedRef<SWidget> DetailRule()
    {
        return SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.24f))];
    }

    TSharedRef<SWidget> DetailSurface(TSharedRef<SWidget> Content, FLinearColor Accent = Cyan)
    {
        return SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Accent.CopyWithNewOpacity(.8f))
            .UseAccentForOutline(true).CutSize(14).Padding(22)[Content];
    }

    TArray<FFlickProfileMatchRecord> FilteredRecords(const UFlickGameInstance* Instance, int32 View)
    {
        TArray<FFlickProfileMatchRecord> Result;
        if (Instance) for (const auto& Record : Instance->GetRecentMatches())
            if (View == 0 || Record.bRanked == (View == 2)) Result.Add(Record);
        return Result;
    }

    FLinearColor ResultColor(const FFlickProfileMatchRecord& Record)
    {
        return Record.bDraw ? FlickHomeSurface::Gold : Record.bWon ? Cyan : Brand;
    }
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfileDetailPages()
{
    TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
    for (const auto Tab : {EFlickProfileTab::Stats, EFlickProfileTab::MatchHistory, EFlickProfileTab::Leaderboards})
    {
        const FString Name = Tab == EFlickProfileTab::Stats ? TEXT("STATS") : Tab == EFlickProfileTab::MatchHistory ? TEXT("MATCH HISTORY") : TEXT("COMPETITIVE RANKS");
        Tabs->AddSlot().AutoWidth().Padding(6,0)
        [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
            .OnClicked_Lambda([this,Tab]() { SelectedProfileTab=Tab; if (GameMode.IsValid()) GameMode->PlayMenuSound(true); return FReply::Handled(); })
            [SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(8).Padding(FMargin(18,12))
                .BackgroundColor_Lambda([this,Tab]() { return SelectedProfileTab==Tab ? PanelSelected : Ink; })
                .AccentColor_Lambda([this,Tab]() { return SelectedProfileTab==Tab ? Brand : Cyan.CopyWithNewOpacity(.45f); })
                [SNew(STextBlock).Text(FText::FromString(Name)).Font(DisplayFont(13)).ColorAndOpacity_Lambda([this,Tab]() { return SelectedProfileTab==Tab ? Paper : Muted; })]]];
    }
    return SNew(SOverlay)
        + SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::ProfileBackdrop).Visibility(EVisibility::HitTestInvisible)]
        + SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink.CopyWithNewOpacity(.28f)).Visibility(EVisibility::HitTestInvisible)]
        + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
        [SNew(SBox).WidthOverride(1468).HeightOverride(818)
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [SNew(SBox).HeightOverride(58)[SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(188)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
                        [SNew(SFlickLogoWidget).VectorPath(TEXT("UI/FlickKnockoutWordmark.svg")).TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark")).DesiredSize(FVector2D(2048,683))]]]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28,0)
                        [SNew(STextBlock).Text(FText::FromString(TEXT("HOME  //  PROFILE"))).Font(DisplayFont(13)).ColorAndOpacity(Cyan)]
                    + SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)[Tabs]]]
                + SVerticalBox::Slot().AutoHeight().Padding(0,20,0,16)
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedProfileTab==EFlickProfileTab::Stats ? TEXT("YOUR GAME. IN NUMBERS.") : SelectedProfileTab==EFlickProfileTab::MatchHistory ? TEXT("EVERY SERIES LEAVES A MARK.") : TEXT("YOUR NEXT LEVEL.")); }).Font(DisplayFont(36)).ColorAndOpacity(Paper)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedProfileTab==EFlickProfileTab::Stats ? TEXT("Your performance, your progress. Every point earned.") : SelectedProfileTab==EFlickProfileTab::MatchHistory ? TEXT("Your latest completed matches, all in one place.") : TEXT("Four playlists. Four climbs. One competitive spirit.")); }).Font(UiFont(12)).ColorAndOpacity(Muted)]]]
                + SVerticalBox::Slot().FillHeight(1)[SNew(SOverlay)
                    + SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]() { return SelectedProfileTab==EFlickProfileTab::Stats ? EVisibility::Visible : EVisibility::Collapsed; })[BuildProfileStatsPanel()]]
                    + SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]() { return SelectedProfileTab==EFlickProfileTab::MatchHistory ? EVisibility::Visible : EVisibility::Collapsed; })[BuildProfileHistoryPanel()]]
                    + SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]() { return SelectedProfileTab==EFlickProfileTab::Leaderboards ? EVisibility::Visible : EVisibility::Collapsed; })[BuildRankedProfilePanel()]]]
                + SVerticalBox::Slot().AutoHeight().Padding(0,18,0,0)
                [SNew(SBox).HeightOverride(50)[SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(180)[SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
                        .OnClicked_Lambda([this]() { SelectedProfileTab=EFlickProfileTab::Overview; return FReply::Handled(); })
                        [SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Cyan).UseAccentForOutline(true).CutSize(10).Padding(FMargin(18,10))
                            [SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE MENU"))).Font(DisplayFont(15)).ColorAndOpacity(Paper)]]]]
                    + SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(FText::FromString(TEXT("YOUR RECORD  //  SAVED AUTOMATICALLY"))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]]]]];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfileStatsPanel()
{
    const auto Instance=[this]() { return PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; };
    const auto Stats=[this,Instance]() { return Instance() ? Instance()->GetProfileStatsForView(static_cast<EFlickProfileStatsView>(SelectedProfileStatsView)) : FFlickProfileStats(); };
    const auto Records=[this,Instance]() { return FilteredRecords(Instance(),SelectedProfileStatsView); };
    TSharedRef<SHorizontalBox> Filters=SNew(SHorizontalBox);
    for (int32 View=0; View<3; ++View)
        Filters->AddSlot().AutoWidth().Padding(0,0,8,0)
        [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).OnClicked_Lambda([this,View]() { SelectedProfileStatsView=View; return FReply::Handled(); })
            [SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(7).Padding(FMargin(22,9))
                .BackgroundColor_Lambda([this,View]() { return SelectedProfileStatsView==View ? PanelSelected : Ink; })
                .AccentColor_Lambda([this,View]() { return SelectedProfileStatsView==View ? Brand : Hairline; })
                [SNew(STextBlock).Text(FText::FromString(View==0 ? TEXT("TOTAL") : View==1 ? TEXT("CASUAL") : TEXT("COMPETITIVE"))).Font(DisplayFont(12)).ColorAndOpacity(Paper)]]];
    TSharedRef<SHorizontalBox> Headlines=SNew(SHorizontalBox);
    const TCHAR* Names[]={TEXT("MATCHES PLAYED"),TEXT("MATCH WINS"),TEXT("WIN RATE"),TEXT("KNOCKOUTS")};
    for (int32 I=0; I<4; ++I)
    {
        const bool bCoral=I==0 || I==2;
        Headlines->AddSlot().FillWidth(1).Padding(I ? 8 : 0,0,I==3 ? 0 : 8,0)
        [SNew(SFlickMainMenuFrame).HomePresentation(true).Primary(bCoral)
            .StartColor(bCoral ? Brand : Panel).EndColor(bCoral ? Brand : Panel)
            .BorderColor(bCoral ? BrandHover : Cyan).Padding(FMargin(24,16))
            [SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Names[I])).Font(DisplayFont(13)).ColorAndOpacity(bCoral ? Ink : Muted)]
                + SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([Stats,I]() { const auto S=Stats(); return I==2 ? FText::FromString(FString::Printf(TEXT("%d%%"), S.MatchesPlayed>0 ? FMath::RoundToInt(100.f*S.Wins/S.MatchesPlayed) : 0)) : FText::AsNumber(I==0 ? S.MatchesPlayed : I==1 ? S.Wins : S.Knockouts); }).Font(DisplayFont(48)).ColorAndOpacity(bCoral ? Ink : Paper)]
                + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(I==2 ? TEXT("COMPLETED SERIES") : I==3 ? TEXT("OPPONENT PUCKS REMOVED") : TEXT("CAREER TOTAL"))).Font(UiFont(9)).ColorAndOpacity(bCoral ? Ink : Muted)]]];
    }
    TSharedRef<SHorizontalBox> Bars=SNew(SHorizontalBox);
    for (int32 Bar=0; Bar<8; ++Bar)
        Bars->AddSlot().FillWidth(1).Padding(10,0)
        [SNew(SVerticalBox).Visibility_Lambda([Records,Bar]() { return Records().IsValidIndex(Bar) ? EVisibility::Visible : EVisibility::Hidden; })
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text_Lambda([Records,Bar]() { const auto R=Records(); return R.IsValidIndex(Bar) ? FText::AsNumber(R[R.Num()-1-Bar].Points) : FText::GetEmpty(); }).Font(DisplayFont(14)).ColorAndOpacity(Paper)]
            + SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Bottom).Padding(0,8,0,6)
                [SNew(SBox).HeightOverride_Lambda([Records,Bar]() { const auto R=Records(); if (!R.IsValidIndex(Bar)) return 0.f; int32 Peak=1; for (const auto& Match:R) Peak=FMath::Max(Peak,Match.Points); return 160.f*R[R.Num()-1-Bar].Points/Peak; })
                    [SNew(SDetailBar).Color_Lambda([Records,Bar]() { const auto R=Records(); return R.IsValidIndex(Bar) && Bar==R.Num()-1 ? Brand : Cyan; })]]
            + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::AsNumber(Bar+1)).Font(DisplayFont(10)).ColorAndOpacity(Muted)]];
    const auto Chart=DetailSurface(SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("RECENT FORM  //  MATCH POINTS"))).Font(DisplayFont(22)).ColorAndOpacity(Paper)]
        + SVerticalBox::Slot().AutoHeight().Padding(0,5,0,15)[SNew(STextBlock).Text(FText::FromString(TEXT("Last eight stored results. Oldest to newest."))).Font(UiFont(11)).ColorAndOpacity(Muted)]
        + SVerticalBox::Slot().FillHeight(1)[SNew(SOverlay)
            + SOverlay::Slot()[Bars]
            + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                [SNew(SVerticalBox).Visibility_Lambda([Records]() { return Records().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(FlickProfileWidgets::SGlyph).Kind(0).Color(Cyan)]
                    + SVerticalBox::Slot().AutoHeight().Padding(0,16,0,4)[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR NEXT MATCH STARTS THE STORY"))).Font(DisplayFont(18)).ColorAndOpacity(Paper)]
                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("Complete a series to begin tracking your form."))).Font(UiFont(11)).ColorAndOpacity(Muted)]]]
        + SVerticalBox::Slot().AutoHeight().Padding(0,14,0,0)[DetailRule()]);
    TSharedRef<SUniformGridPanel> Supporting=SNew(SUniformGridPanel).SlotPadding(FMargin(7));
    const TCHAR* Labels[]={TEXT("CAREER POINTS"),TEXT("SHOTS TAKEN"),TEXT("DOUBLE KOS"),TEXT("STYLE ACCOLADES"),TEXT("TEAM WIPEOUTS"),TEXT("FLAWLESS ROUNDS")};
    for (int32 I=0; I<6; ++I)
        Supporting->AddSlot(I%2,I/2)[DetailSurface(SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Labels[I])).Font(DisplayFont(11)).ColorAndOpacity(Muted)]
            + SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([Stats,I]() { const auto S=Stats(); return FText::AsNumber(I==0 ? S.Points : I==1 ? S.Shots : I==2 ? S.DoubleKnockouts : I==3 ? S.GetTotalAccolades() : S.GetAccoladeCount(I==4 ? EFlickAccolade::TeamWipeout : EFlickAccolade::FlawlessRound)); }).Font(DisplayFont(28)).ColorAndOpacity(I==0 ? BrandHover : Paper)])];
    return SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[Filters]
        + SVerticalBox::Slot().AutoHeight().Padding(0,16,0,16)[SNew(SBox).HeightOverride(138)[Headlines]]
        + SVerticalBox::Slot().FillHeight(1)[SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.2f).Padding(0,0,9,0)[Chart]
            + SHorizontalBox::Slot().FillWidth(1).Padding(0,-7,-7,-7)[Supporting]];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfileHistoryPanel()
{
    const auto Records=[this]() { const auto* I=PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetRecentMatches() : TArray<FFlickProfileMatchRecord>(); };
    TSharedRef<SVerticalBox> Rows=SNew(SVerticalBox);
    for (int32 Index=0; Index<8; ++Index)
    {
        const auto Record=[Records,Index]() { const auto R=Records(); return R.IsValidIndex(Index) ? R[Index] : FFlickProfileMatchRecord(); };
        Rows->AddSlot().AutoHeight().Padding(0,0,0,6)
        [SNew(SBox).HeightOverride(50).Visibility_Lambda([Records,Index]() { return Records().IsValidIndex(Index) ? EVisibility::Visible : EVisibility::Collapsed; })
            [SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor_Lambda([Record]() { return ResultColor(Record()); }).CutSize(9).Padding(FMargin(14,8))
                [SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(78)
                        [SNew(STextBlock).Text_Lambda([Record]() { const auto R=Record(); return FText::FromString(R.bDraw ? TEXT("DRAW") : R.bWon ? TEXT("WIN") : TEXT("LOSS")); }).Font(DisplayFont(20)).ColorAndOpacity_Lambda([Record]() { return ResultColor(Record()); })]]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([Record]() { const auto R=Record(); return FText::FromString(FString::Printf(TEXT("%s  //  %s"),R.bRanked ? TEXT("COMPETITIVE") : TEXT("CASUAL"),R.Variant==EFlickMatchVariant::Bob ? TEXT("BOB 1V1") : *FString::Printf(TEXT("KNOCKOUT %dV%d"),R.PlayersPerTeam,R.PlayersPerTeam))); }).Font(DisplayFont(12)).ColorAndOpacity(Paper)]
                        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([Record]() { const auto R=Record(); return FText::FromString(R.CompletedUnixTime>0 ? FDateTime::FromUnixTimestamp(R.CompletedUnixTime).ToString(TEXT("%d/%m/%Y  %H:%M UTC")) : TEXT("TIME UNAVAILABLE")); }).Font(UiFont(9)).ColorAndOpacity(Muted)]]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(84).HAlign(HAlign_Center)[SNew(STextBlock).Text_Lambda([Record]() { return FText::AsNumber(Record().Points); }).Font(DisplayFont(20)).ColorAndOpacity(Paper)]]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(84).HAlign(HAlign_Center)[SNew(STextBlock).Text_Lambda([Record]() { return FText::AsNumber(Record().Knockouts); }).Font(DisplayFont(20)).ColorAndOpacity(Cyan)]]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(84).HAlign(HAlign_Center)[SNew(STextBlock).Text_Lambda([Record]() { return FText::AsNumber(Record().Shots); }).Font(DisplayFont(20)).ColorAndOpacity(Paper)]]]]];
    }
    const auto Summary=DetailSurface(SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)[SNew(SBox).WidthOverride(44).HeightOverride(44)[SNew(FlickProfileWidgets::SGlyph).Kind(2).Color(Cyan)]]
        + SVerticalBox::Slot().AutoHeight().Padding(0,16,0,6)[SNew(STextBlock).Text(FText::FromString(TEXT("THE LAST EIGHT"))).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("A snapshot of your most recent completed series."))).Font(UiFont(12)).ColorAndOpacity(Muted).AutoWrapText(true)]
        + SVerticalBox::Slot().AutoHeight().Padding(0,24)[DetailRule()]
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([Records]() { return FText::AsNumber(Records().Num()); }).Font(DisplayFont(58)).ColorAndOpacity(Paper)]
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("RECENT SERIES"))).Font(DisplayFont(13)).ColorAndOpacity(Muted)]
        + SVerticalBox::Slot().AutoHeight().Padding(0,22,0,6)[SNew(STextBlock).Text_Lambda([Records]() { int32 Wins=0,Draws=0; const auto R=Records(); for (const auto& M:R) { Wins+=M.bWon && !M.bDraw; Draws+=M.bDraw; } return FText::FromString(FString::Printf(TEXT("%d WINS  /  %d LOSSES  /  %d DRAWS"),Wins,R.Num()-Wins-Draws,Draws)); }).Font(DisplayFont(14)).ColorAndOpacity(FlickHomeSurface::Gold)]
        + SVerticalBox::Slot().FillHeight(1).VAlign(VAlign_Bottom)[SNew(STextBlock).Text(FText::FromString(TEXT("Casual and competitive results.\nPrivate matches and practice are excluded."))).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true)],Brand);
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(0,0,18,0)[SNew(SBox).WidthOverride(320)[Summary]]
        + SHorizontalBox::Slot().FillWidth(1)[DetailSurface(SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("MATCH LOG"))).Font(DisplayFont(23)).ColorAndOpacity(Paper)]
            + SVerticalBox::Slot().AutoHeight().Padding(14,12,14,12)[SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text(FText::FromString(TEXT("RESULT  //  PLAYLIST"))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(84).HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("POINTS"))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]]
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(84).HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("KOS"))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]]
                + SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(84).HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("SHOTS"))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]]]
            + SVerticalBox::Slot().FillHeight(1)[SNew(SOverlay)
                + SOverlay::Slot()[Rows]
                + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [SNew(SVerticalBox).Visibility_Lambda([Records]() { return Records().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(FlickProfileWidgets::SGlyph).Kind(2).Color(Cyan)]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,20,0,6)[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR FIRST RESULT BELONGS HERE"))).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("Finish a casual or competitive series to start your match log."))).Font(UiFont(12)).ColorAndOpacity(Muted)]]])];
}
