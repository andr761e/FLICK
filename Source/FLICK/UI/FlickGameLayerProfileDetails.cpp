#include "UI/FlickCareerWidgets.h"
#include "Core/FlickPracticeCatalog.h"
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
    for (const auto Tab : {EFlickProfileTab::Stats, EFlickProfileTab::MatchHistory, EFlickProfileTab::Leaderboards, EFlickProfileTab::SavedReplays})
    {
        const FString Name = Tab == EFlickProfileTab::Stats ? TEXT("STATS") : Tab == EFlickProfileTab::MatchHistory ? TEXT("MATCH HISTORY") : Tab==EFlickProfileTab::SavedReplays ? TEXT("SAVED REPLAYS") : TEXT("COMPETITIVE RANKS");
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
 struct FRow { FString Name,Value,Help; };
 struct FCache { int32 Category=-1,View=-1,Count=-1; FString Sync; TArray<FRow> Rows; };
 auto Cache=MakeShared<FCache>();
 const auto Career=[this]() { return PlayerController.IsValid()?PlayerController->GetGameInstance()->GetSubsystem<UFlickProgressionSubsystem>():nullptr; };
 const auto Rows=[this,Career,Cache]() -> const TArray<FRow>&
 {
  auto* C=Career(); const int32 Count=C?C->GetEvents().Num():0;
  if(Cache->Category==CareerCategory && Cache->View==SelectedProfileStatsView && Cache->Count==Count && Cache->Sync==(C?C->GetSyncStatus():FString())) return Cache->Rows;
  Cache->Category=CareerCategory;Cache->View=SelectedProfileStatsView;Cache->Count=Count;Cache->Sync=C?C->GetSyncStatus():FString();Cache->Rows.Reset();
  int64 Matches=0,Wins=0,Draws=0,Seconds=0,Shots=0,Points=0,KOs=0,Doubles=0,Self=0,Switches=0,Pockets=0,Fouls=0,Multi=0;
  int64 Acc[FlickAccoladeCount]={}; int32 Practice=0; int64 PracticeXP=0,ChallengeXP=0;
  if(C) for(const auto& E:C->GetEvents())
  {
   if(E.Kind==1) { Practice+=E.Id.StartsWith(TEXT("Practice-")); PracticeXP+=E.XP(); }
   if(E.Kind==3) ChallengeXP+=E.XP();
   if(E.Kind!=0 || (SelectedProfileStatsView>0 && E.Playlist!=SelectedProfileStatsView-1)) continue;
   if((CareerCategory==1 && E.Format==3) || (CareerCategory==2 && E.Format!=3)) continue;
   ++Matches; Wins+=E.bWon;Draws+=E.bDraw;Seconds+=E.Seconds;Shots+=E.Shots;Points+=E.Points;KOs+=E.Knockouts;
   Doubles+=E.DoubleKnockouts;Self+=E.SelfKnockouts;Switches+=E.Switches;Pockets+=E.Pockets;Fouls+=E.Fouls;Multi+=E.MultiPockets;
   for(int32 I=0;I<FMath::Min(E.Accolades.Num(),FlickAccoladeCount);++I) Acc[I]+=E.Accolades[I];
  }
  const FString Since=TEXT("Recorded since the career update. Older matches did not store this breakdown.");
  if(C && (CareerCategory==0 || CareerCategory==3) && SelectedProfileStatsView!=3)
  {
   const auto L=C->GetLegacy(SelectedProfileStatsView); Matches+=L.MatchesPlayed;Wins+=L.Wins;Points+=L.Points;Shots+=L.Shots;KOs+=L.Knockouts;Doubles+=L.DoubleKnockouts;
   for(int32 I=0;I<FMath::Min(L.AccoladeCounts.Num(),FlickAccoladeCount);++I) Acc[I]+=L.AccoladeCounts[I];
  }
  const auto Row=[Cache](const TCHAR* N,int64 V,const FString& H) { Cache->Rows.Add({N,FText::AsNumber(V).ToString(),H}); };
  const auto StringRow=[Cache](const TCHAR* N,const FString& V,const FString& H) { Cache->Rows.Add({N,V,H}); };
  if(CareerCategory==0)
  {
   Row(TEXT("Matches played"),Matches,TEXT("Completed matches in the selected playlists. Includes preserved historical career totals."));
   Row(TEXT("Wins"),Wins,TEXT("Completed match wins, independent of your competitive MMR."));
   StringRow(TEXT("Win rate"),FString::Printf(TEXT("%.1f%%"),Matches?100.*Wins/Matches:0.),TEXT("Wins divided by completed matches. Draws are not wins."));
   StringRow(TEXT("Active match time"),FString::Printf(TEXT("%lldh %lldm"),Seconds/3600,Seconds/60%60),Since+TEXT(" Excludes menus, replays and spectating; completed matches only."));
   Row(TEXT("Career points"),Points,TEXT("Lifetime points; this counter continues after Steam achievement goals are reached."));
   Row(TEXT("Shots taken"),Shots,TEXT("Lifetime shots in the selected completed matches."));
   StringRow(TEXT("Shots per match"),FString::Printf(TEXT("%.2f"),Matches?double(Shots)/Matches:0.),TEXT("Lifetime shots divided by completed matches in this view."));
  }
  else if(CareerCategory==1)
  {
   Row(TEXT("Knockout matches"),Matches,Since);Row(TEXT("Knockout wins"),Wins,Since);
   Row(TEXT("Opponent knockouts"),KOs,Since);Row(TEXT("Double knockouts"),Doubles,Since);
   Row(TEXT("Self knockouts"),Self,Since);Row(TEXT("Switch activations"),Switches,Since);
   StringRow(TEXT("Knockouts per match"),FString::Printf(TEXT("%.2f"),Matches?double(KOs)/Matches:0.),Since);
  }
  else if(CareerCategory==2)
  {
   Row(TEXT("BOB matches"),Matches,Since);Row(TEXT("BOB wins"),Wins,Since);Row(TEXT("Pucks pocketed"),Pockets,Since+TEXT(" Scoring pucks only; striker pockets are fouls."));
   Row(TEXT("Striker fouls"),Fouls,Since);Row(TEXT("Multi-pocket shots"),Multi,Since+TEXT(" At least two of your scoring pucks pocketed in one shot."));
   Row(TEXT("BOB shots"),Shots,Since);
   StringRow(TEXT("Pockets per match"),FString::Printf(TEXT("%.2f"),Matches?double(Pockets)/Matches:0.),Since+TEXT(" Not labelled accuracy: opponents can also pocket your pieces."));
  }
  else if(CareerCategory==3)
   for(int32 I=0;I<FlickAccoladeCount;++I)
   {
    static const TCHAR* Help[]={TEXT("Knock out an opponent while activating a switch."),TEXT("Knock out an opponent after banking your shot off a divider."),TEXT("Earn a knockout where a newly raised divider affects play."),TEXT("Knock out an opponent through a chain of puck collisions."),TEXT("Remove at least two opponents in one shot to clear their remaining team."),TEXT("Trade one of your pieces for the final opponent while another ally survives."),TEXT("Earn a long-range knockout."),TEXT("Hit at least three pucks directly while earning a knockout."),TEXT("Earn a knockout, keep your shot puck near the edge and lose none of your own pieces."),TEXT("Earn a knockout with a last-moment shot release."),TEXT("Win a round with all of your team's starting pieces still in play.")};
    Row(GetFlickAccoladeName(static_cast<EFlickAccolade>(I)),Acc[I],FString(Help[I])+TEXT(" Lifetime awards in the selected playlists; no achievement counter cap."));
   }
  else if(CareerCategory==4)
  {
   Row(TEXT("Practice difficulties completed"),Practice,TEXT("Distinct completed practice runs. Each pack/difficulty pays its first-completion XP once."));
   int32 TrueMastered=0; for(int32 Cat=0;Cat<4;++Cat) for(int32 D=0;D<3;++D) TrueMastered+=C && C->GetPracticeBest(Cat,D)==5;
   Row(TEXT("Mastered difficulties"),TrueMastered,TEXT("A perfect 5/5 earns mastery. Best scores are preserved in the account career record."));
   Row(TEXT("Practice XP"),PracticeXP,TEXT("150 Foundation / 300 Skilled / 500 Expert. Replays do not award repeat XP."));
  }
  else
  {
   const auto L=C?C->GetLevel():FlickProgression::FLevel();
   Row(TEXT("Account level"),L.Number,TEXT("Permanent experience level. Never resets with a season and is separate from rank. No gameplay bonuses."));
   Row(TEXT("Lifetime XP"),C?C->GetXP():0,TEXT("Match completion, active time, modest performance bonuses and one-time learning/challenge rewards."));
   Row(TEXT("XP to next level"),L.Required-L.Progress,TEXT("1,000 XP at level 1; +100 per level until the requirement reaches 3,000. Levels continue thereafter."));
   Row(TEXT("Challenge XP"),ChallengeXP,TEXT("One-time rewards for completed eligible career challenges."));
   StringRow(TEXT("Account save"),C?C->GetSyncStatus():TEXT("UNAVAILABLE"),TEXT("Account-scoped local saves plus Steam Remote Storage when signed in and Steam Cloud is enabled. Offline awards merge by unique event ID."));
  }
  return Cache->Rows;
 };
 auto Filters=SNew(SHorizontalBox).Visibility_Lambda([this]() { return CareerCategory<4?EVisibility::Visible:EVisibility::Hidden; });
 for(int32 V=0;V<4;++V) Filters->AddSlot().AutoWidth().Padding(0,0,8,0)
 [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).OnClicked_Lambda([this,V]() { SelectedProfileStatsView=V;CareerSelectedRow=0;return FReply::Handled(); })
  [SNew(SFlickAngularBorder).HomeSurface(true).CutSize(7).Padding(FMargin(18,8)).UseAccentForOutline(true)
   .AccentColor_Lambda([this,V]() { return SelectedProfileStatsView==V?Brand:Cyan; }).BackgroundColor(Panel)
   [SNew(STextBlock).Text(FText::FromString(V==0?TEXT("ALL PLAYLISTS"):V==1?TEXT("CASUAL"):V==2?TEXT("COMPETITIVE"):TEXT("BOTS"))).Font(DisplayFont(11)).ColorAndOpacity(Paper)]]];
 auto Categories=SNew(SVerticalBox);
 const TCHAR* Names[]={TEXT("OVERVIEW"),TEXT("KNOCKOUT"),TEXT("BOB"),TEXT("ACCOLADES"),TEXT("PRACTICE"),TEXT("PROGRESSION")};
 for(int32 Cat=0;Cat<6;++Cat) Categories->AddSlot().AutoHeight().Padding(0,0,0,7)
 [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).OnClicked_Lambda([this,Cat]() { CareerCategory=Cat;CareerSelectedRow=0;return FReply::Handled(); })
  [SNew(SFlickAngularBorder).HomeSurface(true).CutSize(8).Padding(FMargin(14,17)).UseAccentForOutline(true)
   .AccentColor_Lambda([this,Cat]() { return CareerCategory==Cat?Brand:Cyan.CopyWithNewOpacity(.45f); }).BackgroundColor(Panel)
   [SNew(STextBlock).Text(FText::FromString(Names[Cat])).Font(DisplayFont(13)).ColorAndOpacity(Paper)]]];
 const auto CareerSurface=[](TSharedRef<SWidget> Content) -> TSharedRef<SWidget>
 { return SNew(SFlickAngularBorder).HomeSurface(false).BackgroundColor(FLinearColor(.005f,.027f,.045f,.96f)).AccentColor(Cyan.CopyWithNewOpacity(.35f)).CutSize(12).Padding(22)[Content]; };
 auto Stats=SNew(SVerticalBox);
 for(int32 I=0;I<FlickAccoladeCount;++I) Stats->AddSlot().AutoHeight().Padding(0,0,0,4)
 [SNew(SBox).HeightOverride(35).Visibility_Lambda([Rows,I]() { return Rows().IsValidIndex(I)?EVisibility::Visible:EVisibility::Collapsed; })
  [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).OnHovered_Lambda([this,I]() { CareerSelectedRow=I; })
   .OnClicked_Lambda([this,I]() { CareerSelectedRow=I;return FReply::Handled(); })
   [SNew(SFlickAngularBorder).HomeSurface(false).CutSize(6).Padding(FMargin(14,6)).UseAccentForOutline(true).BorderWidth(.7f)
    .AccentColor_Lambda([this,I]() { return CareerSelectedRow==I?Brand:Cyan.CopyWithNewOpacity(.12f); }).BackgroundColor_Lambda([this,I]() { return CareerSelectedRow==I?FLinearColor(.018f,.11f,.16f):FLinearColor(.008f,.045f,.065f,.8f); })
    [SNew(SHorizontalBox)
     + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([Rows,I]() { return Rows().IsValidIndex(I)?FText::FromString(Rows()[I].Name):FText::GetEmpty(); }).Font(UiFont(11,true)).ColorAndOpacity(Paper)]
     + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text_Lambda([Rows,I]() { return Rows().IsValidIndex(I)?FText::FromString(Rows()[I].Value):FText::GetEmpty(); }).Font(DisplayFont(16)).ColorAndOpacity(Cyan)]]]]];
 const auto Selected=[this,Rows]() { const auto& R=Rows();return R.IsValidIndex(CareerSelectedRow)?R[CareerSelectedRow]:FRow(); };
 return SNew(SVerticalBox)
  + SVerticalBox::Slot().AutoHeight()[Filters]
  + SVerticalBox::Slot().FillHeight(1).Padding(0,16,0,0)[SNew(SHorizontalBox)
   + SHorizontalBox::Slot().AutoWidth().Padding(0,0,16,0)[SNew(SBox).WidthOverride(205)[Categories]]
   + SHorizontalBox::Slot().FillWidth(1.55f).Padding(0,0,16,0)[CareerSurface(SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,14)[SNew(STextBlock).Text(FText::FromString(TEXT("CAREER STATISTICS"))).Font(DisplayFont(23)).ColorAndOpacity(Paper)]
    + SVerticalBox::Slot().FillHeight(1)[Stats])]
   + SHorizontalBox::Slot().FillWidth(1)[CareerSurface(SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("STAT BREAKDOWN"))).Font(UiFont(10,true)).ColorAndOpacity(Muted)]
    + SVerticalBox::Slot().AutoHeight().Padding(0,20,0,8)[SNew(STextBlock).Text_Lambda([Selected]() { return FText::FromString(Selected().Name); }).Font(DisplayFont(24)).ColorAndOpacity(Paper).AutoWrapText(true)]
    + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,20)[SNew(STextBlock).Text_Lambda([Selected]() { return FText::FromString(Selected().Value); }).Font(DisplayFont(46)).ColorAndOpacity(Cyan).AutoWrapText(true)]
    + SVerticalBox::Slot().AutoHeight()[DetailRule()]
    + SVerticalBox::Slot().AutoHeight().Padding(0,20)[SNew(STextBlock).Text_Lambda([Selected]() { return FText::FromString(Selected().Help); }).Font(UiFont(12)).ColorAndOpacity(Muted).AutoWrapText(true)]
    + SVerticalBox::Slot().FillHeight(1)[SNew(SSpacer)]
    + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(CareerCategory>=4?TEXT("ACCOUNT-WIDE PROGRESS"):SelectedProfileStatsView==0?TEXT("ALL PLAYLISTS"):SelectedProfileStatsView==1?TEXT("CASUAL MATCHES"):SelectedProfileStatsView==2?TEXT("COMPETITIVE MATCHES"):TEXT("BOT MATCHES")); }).Font(UiFont(10,true)).ColorAndOpacity(Cyan)]
    + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[DetailRule()]
    + SVerticalBox::Slot().AutoHeight()[FlickCareerUI::Progress(Career())]
   )]];
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
