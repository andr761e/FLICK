#include "UI/FlickGameLayerPrivate.h"
#include "Player/FlickPostMatchPresentationComponent.h"
#include "Fonts/FontMeasure.h"
#include "UI/FlickRankResult.h"



TSharedRef<SWidget> SFlickGameLayer::BuildPostMatchOverlay()
{
 const auto Presentation = [this]() -> UFlickPostMatchPresentationComponent*
 {
  return PlayerController.IsValid() ? PlayerController->FindComponentByClass<UFlickPostMatchPresentationComponent>() : nullptr;
 };
 const auto ShowDetails = [Presentation]() { const auto* View = Presentation(); return View && View->ShowsDetails(); };
 const auto Text = [this](TAttribute<FText> Label, int32 Size, FLinearColor Color, bool bWrap = true) -> TSharedRef<SWidget>
 {
  return SNew(STextBlock).Text(Label).Font(UiFont(Size, true)).ColorAndOpacity(Color)
   .ShadowOffset(FVector2D(0, 1)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .8f))
   .AutoWrapText(bWrap).OverflowPolicy(ETextOverflowPolicy::Ellipsis);
 };
 const auto CenteredLine = [](TAttribute<FText> Label, int32 Size, FLinearColor Color) -> TSharedRef<SWidget>
 {
  return SNew(STextBlock).Text(Label).Font(UiFont(Size, true)).ColorAndOpacity(Color)
   .Justification(ETextJustify::Center).AutoWrapText(false).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
   .ShadowOffset(FVector2D(0, 1)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .85f));
 };
 const auto Action = [this](const FString& Label, FOnClicked Click, bool bPrimary) -> TSharedRef<SWidget>
 {
  return SNew(SBox).HeightOverride(52)
  [SNew(SFlickMainMenuFrame).Primary(bPrimary)
   .StartColor(bPrimary ? Brand : Ink).EndColor(bPrimary ? Brand : Ink)
   .BorderColor(bPrimary ? Brand : Cyan.CopyWithNewOpacity(.55f)).HoverBorderColor(Brand).Padding(2)
   [SNew(SButton).ButtonStyle(&TransparentButtonStyle)
   .OnClicked(Click).ContentPadding(FMargin(16, 8))
   [SNew(STextBlock).Text_Lambda([this, Label, bPrimary]()
   {
    const AFlickGameState* State = GetScoreboardGameState();
    if (State && State->bPrivateMatchActive && bPrimary)
     return FText::FromString(GameMode.IsValid() ? TEXT("START REMATCH") : TEXT("WAITING FOR HOST"));
    return FText::FromString(Label);
   }).Font(UiFont(14, true)).ColorAndOpacity(bPrimary ? Ink : Paper)]]];
 };
 TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox);
 Menu->AddSlot().AutoHeight()[Text(TAttribute<FText>::CreateLambda([this]()
 {
  const AFlickGameState* State = GetScoreboardGameState();
  return FText::FromString(State && State->bPrivateMatchActive ? TEXT("PRIVATE MATCH")
   : State && State->bRankedMatch ? TEXT("COMPETITIVE") : TEXT("CASUAL"));
 }), 11, Brand)];
 Menu->AddSlot().AutoHeight().Padding(0, 12, 0, 0)
 [SNew(SBox).IsEnabled_Lambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  if (!State || !PlayerController.IsValid()) return false;
  const auto* Instance = Cast<UFlickGameInstance>(PlayerController->GetGameInstance());
  if (Instance && Instance->bPostMatchQueuePending) return false;
  const auto* Party = Instance ? Instance->GetSubsystem<UFlickPartySubsystem>() : nullptr;
  if (Party && Party->IsActive() && !Party->IsLocalLeader()) return false;
  if (!State->bPrivateMatchActive) return true;
  const auto* Local = PlayerController->GetPlayerState<AFlickPlayerState>();
  return GameMode.IsValid() && !State->bRematchStarting && !State->bMatchEndedByForfeit
   && (!State->bPartyActive || (Local && Local->IsPartyLeader()))
   && (State->RematchDeadlineServerTime <= 0 || State->GetServerWorldTimeSeconds() < State->RematchDeadlineServerTime);
 })
 [Action(TEXT("PLAY AGAIN"), FOnClicked::CreateLambda([this]()
 { if (PlayerController.IsValid()) PlayerController->PlayAgain(); return FReply::Handled(); }), true)]];
 Menu->AddSlot().AutoHeight().Padding(0, 12, 0, 18)
 [Text(TAttribute<FText>::CreateLambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  if (!State) return FText::GetEmpty();
  if (State->bPrivateMatchActive) return FText::FromString(TEXT("Host starts the next match with the same room, players and settings."));
  if (State->bMatchmakingLobby || State->bRankedMatch || (PlayerController.IsValid() && PlayerController->GetNetMode() != NM_Standalone))
   return FText::FromString(TEXT("Find a new match in this playlist. Browse the main menu while we search. Only the party leader can queue a party."));
  return FText::FromString(TEXT("Play another match with the same mode and lineup."));
 }), 11, Muted)];
 Menu->AddSlot().AutoHeight().Padding(0, 0, 0, 18)
 [SNew(SBox).Visibility_Lambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  const auto* Ranking = GetRankingSubsystem();
  return State && State->bRankedMatch && Ranking && Ranking->HasRatingUpdateForMatch(State->MatchId)
   ? EVisibility::Visible : EVisibility::Collapsed;
 })
 [SNew(SFlickRankResult).Update_Lambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  const auto* Ranking = GetRankingSubsystem();
  return State && Ranking && Ranking->HasRatingUpdateForMatch(State->MatchId) ? Ranking->GetLastRatingUpdate() : FFlickRatingUpdate();
 }).Time_Lambda([this]() { const auto* State = GetScoreboardGameState(); return State ? static_cast<double>(State->GetServerWorldTimeSeconds()) : 0.; })]];
 Menu->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
 [SNew(SBox).Visibility_Lambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  const auto* Ranking = GetRankingSubsystem();
  return State && State->bRankedMatch && (!Ranking || !Ranking->HasRatingUpdateForMatch(State->MatchId)) ? EVisibility::Visible : EVisibility::Collapsed;
 })[Text(FText::FromString(TEXT("COMPETITIVE RESULT SYNCING...")), 11, Brand)]];
 Menu->AddSlot().AutoHeight()[Action(TEXT("EXIT TO MAIN MENU"), FOnClicked::CreateLambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  if (GameMode.IsValid() && (!GameMode->IsNetworkSession() || (State && State->bPrivateMatchActive))) GameMode->ReturnToMainMenu();
  else if (PlayerController.IsValid()) PlayerController->LeaveNetworkSession();
  return FReply::Handled();
 }), false)];
 TSharedRef<SConstraintCanvas> Winners = SNew(SConstraintCanvas);
 const TWeakPtr<SConstraintCanvas> WinnerCanvas = Winners;
 for (int32 Slot = 0; Slot < 3; ++Slot)
 {
  Winners->AddSlot().AutoSize(true).Alignment(FVector2D(0.5f, 0.0f))
  .Offset_Lambda([this, Presentation, WinnerCanvas, Slot]()
  {
   const auto Canvas = WinnerCanvas.Pin();
   const FVector2D CanvasSize = Canvas.IsValid() ? Canvas->GetCachedGeometry().GetLocalSize() : LayerLocalSize;
   const auto* State = GetScoreboardGameState();
   const int32 Count = State ? FMath::Clamp(State->PlayersPerTeam, 1, 3) : 1;
   // Follow the pucks horizontally, but keep a stable bottom row with enough
   // spacing for every card even while the camera pulls back.
   float CenterX = CanvasSize.X * .5f;
   float Spacing = 269.f;
   int32 Width = 0, Height = 0;
   if (PlayerController.IsValid() && Presentation())
   {
    PlayerController->GetViewportSize(Width, Height);
    FVector2D First, Last;
    if (Width > 0 && Height > 0
     && PlayerController->ProjectWorldLocationToScreen(Presentation()->GetWinnerPosition(0), First)
     && PlayerController->ProjectWorldLocationToScreen(Presentation()->GetWinnerPosition(Count - 1), Last))
    {
     const FVector2D FullSize(Width, Height);
     const FVector2D FrameSize = FlickPresentationFrame::GetContainedSize(FullSize);
     const float Scale = CanvasSize.X / FrameSize.X;
     CenterX = ((First.X + Last.X) * .5f - (FullSize.X - FrameSize.X) * .5f) * Scale;
     if (Count > 1) Spacing = FMath::Max(269.f, static_cast<float>(FMath::Abs(Last.X - First.X) * Scale / (Count - 1)));
    }
   }
   if (Count > 1) Spacing = FMath::Min(Spacing, FMath::Max(269.f, static_cast<float>((CanvasSize.X - 293.f) / (Count - 1))));
   const float HalfRow = (245.f + (Count - 1) * Spacing) * .5f;
   CenterX = FMath::Clamp(CenterX, HalfRow + 24.f, FMath::Max(HalfRow + 24.f, static_cast<float>(CanvasSize.X) - HalfRow - 24.f));
   const float X = CenterX + (Slot - (Count - 1) * .5f) * Spacing;
   return FMargin(X, FMath::Max(160.f, static_cast<float>(CanvasSize.Y) - 252.f), 0, 0);
  })
  [SNew(SBox).WidthOverride(245).HeightOverride(128).Visibility_Lambda([this, Slot, Presentation]()
  {
   const auto* State = GetScoreboardGameState();
   const auto* View = Presentation();
   return View && View->GetWinnerStatsOpacity() > 0.0f && State && State->WinnerTeam != EFlickTeam::None && Slot < State->PlayersPerTeam
    ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
  })
   [SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(10)
    .BackgroundColor(Ink.CopyWithNewOpacity(.97f)).AccentColor(Cyan.CopyWithNewOpacity(.65f)).UseAccentForOutline(true)
    .Padding(FMargin(12, 10))
   [SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight()
    [SNew(SBox).HeightOverride(28)
     [CenteredLine(TAttribute<FText>::CreateLambda([this, Slot]()
      {
       const auto* State = GetScoreboardGameState();
       FString Name = State ? GetScoreboardPlayerName(State->WinnerTeam, Slot).ToString() : FString();
       Name.ReplaceInline(TEXT("\r"), TEXT(" "));
       Name.ReplaceInline(TEXT("\n"), TEXT(" "));
       // Elide by rendered width, keeping a readable font size. Character
       // limits alone do not account for wide names or Unicode glyphs.
       const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
       const FSlateFontInfo Font = UiFont(18, true);
       if (Measure->Measure(Name, Font).X > 213.f)
       {
        while (Name.Len() > 1 && Measure->Measure(Name + TEXT("..."), Font).X > 213.f) Name.LeftChopInline(1);
        Name += TEXT("...");
       }
       return FText::FromString(Name);
      }), 18, Paper)]]
    + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
    [SNew(SBox).HeightOverride(19)
     [CenteredLine(TAttribute<FText>::CreateLambda([this, Slot]()
    {
     const auto* State = GetScoreboardGameState();
     const auto* Stats = State ? State->FindPlayerMatchStats(State->WinnerTeam, Slot) : nullptr;
     if (!Stats) return FText::GetEmpty();
     return FText::FromString(FString::Printf(TEXT("%d %s"), Stats->Knockouts,
      State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("PUCKS POCKETED") : TEXT("OPPONENT KNOCKOUTS")));
    }), 11, Paper)]]
    + SVerticalBox::Slot().AutoHeight()
    [SNew(SBox).HeightOverride(19)
     [CenteredLine(TAttribute<FText>::CreateLambda([this, Slot]()
    {
     const auto* State = GetScoreboardGameState();
     const auto* Stats = State ? State->FindPlayerMatchStats(State->WinnerTeam, Slot) : nullptr;
     if (!Stats) return FText::GetEmpty();
     return FText::FromString(FString::Printf(TEXT("%d %s%s"),
      Stats->SelfKnockouts, State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("STRIKER PENALTIES") : TEXT("SELF-KOS"),
      State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("") : *FString::Printf(TEXT("  /  %d SWITCHES"), Stats->SwitchActivations)));
    }), 11, Muted)]]
    + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
    [SNew(SBox).HeightOverride(19)
     [CenteredLine(TAttribute<FText>::CreateLambda([this, Slot]()
    {
     const auto* State = GetScoreboardGameState();
     return FText::FromString(State && !State->bMatchEndedByForfeit && !State->bDecisiveShotSelfKnockout
      && State->DecisiveShotTurn > 0 && State->DecisiveShotTeam == State->WinnerTeam
      && State->DecisiveShotPlayerSlot == Slot ? TEXT("DECISIVE SHOT") : TEXT(""));
    }), 11, Brand)]]
   ]]];
 }
 const auto DecisiveMoment = [this]()
 {
  const auto* State = GetScoreboardGameState();
  if (!State) return FText::GetEmpty();
  if (State->bMatchEndedByForfeit) return FText::FromString(TEXT("MATCH ENDED BY FORFEIT  /  NO DECISIVE SHOT"));
  if (State->DecisiveShotTurn <= 0 || State->DecisiveShotTeam == EFlickTeam::None) return FText::FromString(TEXT("DECISIVE MOMENT  /  SIMULTANEOUS KICKOFF"));
  FString PlayerName = GetScoreboardPlayerName(State->DecisiveShotTeam, State->DecisiveShotPlayerSlot).ToString();
  PlayerName.ReplaceInline(TEXT("\r"), TEXT(" "));
  PlayerName.ReplaceInline(TEXT("\n"), TEXT(" "));
  if (PlayerName.Len() > 28) PlayerName = PlayerName.Left(25) + TEXT("...");
  return FText::FromString(FString::Printf(TEXT("%s  /  %s  /  ROUND %d, SHOT %d"),
   State->bDecisiveShotSelfKnockout ? TEXT("DECISIVE SELF-KNOCKOUT") : TEXT("DECISIVE SHOT"),
   *PlayerName,
   State->RoundNumber, State->DecisiveShotTurn));
 };
 return SNew(SOverlay)
 + SOverlay::Slot()
 [SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0, 0, 0, .18f))
  .Visibility_Lambda([ShowDetails]() { return ShowDetails() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]
 + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(24, 24)
 [SNew(SBox).WidthOverride(700)
  [SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(14)
   .BackgroundColor(Ink.CopyWithNewOpacity(.97f)).AccentColor(Cyan.CopyWithNewOpacity(.65f)).UseAccentForOutline(true)
   .Padding(FMargin(20, 12))
 [SNew(SVerticalBox)
  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
  [Text(TAttribute<FText>::CreateLambda([this]()
   { const auto* State = GetScoreboardGameState(); return FText::FromString(!State || State->WinnerTeam == EFlickTeam::None ? TEXT("MATCH DRAW")
     : State->WinnerTeam == EFlickTeam::Player1 ? TEXT("BLUE WINS") : TEXT("ORANGE WINS")); }), 36, Paper)]
  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)
  [SNew(SBox).HeightOverride(22).WidthOverride(660)
   [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
    [CenteredLine(TAttribute<FText>::CreateLambda(DecisiveMoment), 11, Brand)]]]
 ]]]
 + SOverlay::Slot()
 [SNew(SBorder).BorderImage(FAppStyle::GetBrush("NoBrush")).Padding(0)
  .Visibility(EVisibility::HitTestInvisible)
  .ColorAndOpacity_Lambda([Presentation]()
  { const auto* View = Presentation(); return FLinearColor(1, 1, 1, View ? View->GetWinnerStatsOpacity() : 0.0f); })
  [Winners]]
 + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 30)
 [SNew(SBox).Visibility_Lambda([ShowDetails]() { return ShowDetails() ? EVisibility::Collapsed : EVisibility::Visible; })
  [Action(TEXT("CONTINUE TO SUMMARY"), FOnClicked::CreateLambda([Presentation]()
   { if (auto* View = Presentation()) View->ContinueToSummary(); return FReply::Handled(); }), false)]]
 + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
 .Padding(TAttribute<FMargin>::CreateLambda([this]()
 { const auto* State = GetScoreboardGameState(); return FMargin(32, State && State->bRankedMatch ? 100.f : 150.f, 32, 32); }))
 [SNew(SBox).WidthOverride(1680).HeightOverride_Lambda([this]()
  { const auto* State = GetScoreboardGameState(); return State && State->bRankedMatch ? 700.f : 760.f; }).Visibility_Lambda([ShowDetails]() { return ShowDetails() ? EVisibility::Visible : EVisibility::Collapsed; })
  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
   [SNew(SBox).WidthOverride(1230)
    [SNew(SHorizontalBox)
     + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 30, 0)
     [SNew(SBox).WidthOverride(300)
      [SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Ink.CopyWithNewOpacity(.94f)).AccentColor(Cyan).UseAccentForOutline(true).CutSize(18).Padding(20)[Menu]]]
     + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[BuildScoreboardOverlay(true)]
    ]
   ]
  ]
 ];
}
