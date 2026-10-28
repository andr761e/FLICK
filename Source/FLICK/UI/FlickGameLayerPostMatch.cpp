#include "UI/FlickGameLayerPrivate.h"
#include "Player/FlickPostMatchPresentationComponent.h"
#include "Fonts/FontMeasure.h"



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
    if (State && State->bPrivateMatchActive && Label.StartsWith(TEXT("CHANGE"))) return FText::FromString(TEXT("ROLES & LINEUP"));
    if (bPrimary && State && PlayerController.IsValid() && State->RematchVotes.Contains(PlayerController->PlayerState)) return FText::FromString(TEXT("REMATCH READY"));
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
  const AFlickGameState* State = GetScoreboardGameState();
  const AFlickPlayerState* Local = PlayerController.IsValid() ? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr;
  return State && !State->bRematchStarting && !State->bMatchEndedByForfeit
   && (State->RematchDeadlineServerTime <= 0.0f || State->GetServerWorldTimeSeconds() < State->RematchDeadlineServerTime) && (!State->bPrivateMatchActive
   || (GameMode.IsValid() && (!State->bPartyActive || (Local && Local->IsPartyLeader()))));
 })
 [Action(TEXT("PLAY AGAIN"), FOnClicked::CreateLambda([this]()
 { if (PlayerController.IsValid()) PlayerController->RequestRematch(false); return FReply::Handled(); }), true)]];
 Menu->AddSlot().AutoHeight().Padding(0, 8, 0, 0)
 [SNew(SBox).Visibility_Lambda([this]()
 {
  const AFlickGameState* State = GetScoreboardGameState();
  return State && (State->ActiveMatchVariant == EFlickMatchVariant::Classic || State->bPrivateMatchActive) ? EVisibility::Visible : EVisibility::Collapsed;
 }).IsEnabled_Lambda([this]()
 {
  const AFlickGameState* State = GetScoreboardGameState();
  const AFlickPlayerState* Local = PlayerController.IsValid() ? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr;
  return State && !State->bRematchStarting && !State->bMatchEndedByForfeit
   && (State->RematchDeadlineServerTime <= 0.0f || State->GetServerWorldTimeSeconds() < State->RematchDeadlineServerTime)
   && (!State->bPrivateMatchActive || (GameMode.IsValid() && (!State->bPartyActive || (Local && Local->IsPartyLeader()))));
 })
 [Action(TEXT("CHANGE LINEUP"), FOnClicked::CreateLambda([this]()
 { if (PlayerController.IsValid()) PlayerController->RequestRematch(true); return FReply::Handled(); }), false)]];
 Menu->AddSlot().AutoHeight().Padding(0, 12, 0, 16)
 [Text(TAttribute<FText>::CreateLambda([this]()
 {
  const AFlickGameState* State = GetScoreboardGameState();
  if (!State) return FText::GetEmpty();
  if (State->RematchDeadlineServerTime > 0.0f && State->GetServerWorldTimeSeconds() >= State->RematchDeadlineServerTime) return FText::FromString(TEXT("Rematch window ended. Exit to find a match."));
  if (!State->RematchStatus.IsEmpty()) return FText::FromString(State->RematchStatus);
  if (State->bMatchEndedByForfeit) return FText::FromString(TEXT("Match ended by forfeit. Leave to find another match."));
  if (State->bPrivateMatchActive) return FText::FromString(TEXT("Host starts the next match. Same room, players and settings. Change lineup to revisit roles and classes."));
  if (State->bMatchmakingLobby || (PlayerController.IsValid() && PlayerController->GetNetMode() != NM_Standalone && !State->bPrivateMatchActive))
   return FText::FromString(FString::Printf(TEXT("REMATCH READY  %d / %d\nAll players stay on the same teams.%s"), State->RematchVotes.Num(), State->PlayersPerTeam * 2,
    State->bRematchChangeLineup ? TEXT(" Class selection opens before the next match.") : TEXT("")));
  return FText::FromString(TEXT("Keep the same teams and lineup, or choose a new class first."));
 }), 11, Muted)];
 Menu->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
 [Text(TAttribute<FText>::CreateLambda([this]()
 {
  const AFlickGameState* State = GetScoreboardGameState();
  return State && State->RematchDeadlineServerTime > 0.0f
   ? FText::FromString(FString::Printf(TEXT("REMATCH WINDOW  %ds"), FMath::Max(0, FMath::CeilToInt(State->RematchDeadlineServerTime - State->GetServerWorldTimeSeconds())))) : FText::GetEmpty();
 }), 11, Brand)];
 Menu->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
 [SNew(SBox).HAlign(HAlign_Left).Visibility_Lambda([this]()
 {
  const auto* State = GetScoreboardGameState();
  const auto* Ranking = GetRankingSubsystem();
  return State && State->bRankedMatch && Ranking && Ranking->HasRatingUpdateForMatch(State->MatchId)
   ? EVisibility::Visible : EVisibility::Collapsed;
 })
 [SNew(SHorizontalBox)
  + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
  [SNew(SFlickRankBadge).Size(56).Progress_Lambda([this]()
  {
   const auto* Ranking = GetRankingSubsystem();
   if (!Ranking) return FFlickRankProgress();
   const auto& Update = Ranking->GetLastRatingUpdate();
   return Ranking->GetProgress(Update.Variant, Update.PlayersPerTeam);
  })]
  + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
  [Text(TAttribute<FText>::CreateLambda([this]()
  {
   const auto* Ranking = GetRankingSubsystem();
   if (!Ranking) return FText::GetEmpty();
   const auto& Update = Ranking->GetLastRatingUpdate();
   const int32 OldIndex = Update.OldTier == EFlickRankTier::Unranked ? -1
    : Update.OldTier == EFlickRankTier::Zenith ? 21 : (static_cast<int32>(Update.OldTier) - 1) * 3 + Update.OldDivision - 1;
   const int32 NewIndex = FlickRankRules::GetBadgeIndex(Ranking->GetProgress(Update.Variant, Update.PlayersPerTeam));
   const TCHAR* Change = OldIndex < 0 && NewIndex >= 0 ? TEXT("PLACEMENTS COMPLETE")
    : NewIndex > OldIndex ? TEXT("RANK UP") : NewIndex < OldIndex ? TEXT("RANK ADJUSTED") : TEXT("COMPETITIVE RESULT");
   return FText::FromString(FString::Printf(TEXT("%s\n%d  >  %d MMR"), Change, Update.OldRating, Update.NewRating));
  }), 11, Paper)]
 ]];
 Menu->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
 [Text(TAttribute<FText>::CreateLambda([this]()
 {
  const AFlickGameState* State = GetScoreboardGameState();
  const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
  if (!State || !State->bRankedMatch) return FText::GetEmpty();
  if (!Ranking || !Ranking->HasRatingUpdateForMatch(State->MatchId)) return FText::FromString(TEXT("COMPETITIVE RESULT SYNCING..."));
  const FFlickRatingUpdate& Update = Ranking->GetLastRatingUpdate();
  return FText::FromString(FString::Printf(TEXT("RATING  %s%d  /  %s"), Update.RatingDelta >= 0 ? TEXT("+") : TEXT(""), Update.RatingDelta,
   *FlickRankRules::GetProgressLabel(Ranking->GetProgress(Update.Variant, Update.PlayersPerTeam))));
 }), 11, Brand)];
 Menu->AddSlot().AutoHeight()[Action(TEXT("EXIT TO MAIN MENU"), FOnClicked::CreateLambda([this]()
 {
  if (GameMode.IsValid()) GameMode->ReturnToMainMenu();
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
 + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(32, 105, 32, 32)
 [SNew(SBox).Visibility_Lambda([ShowDetails]() { return ShowDetails() ? EVisibility::Visible : EVisibility::Collapsed; })
  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
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
