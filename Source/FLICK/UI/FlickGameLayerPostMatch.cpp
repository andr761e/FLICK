#include "UI/FlickGameLayerPrivate.h"
#include "Player/FlickPostMatchPresentationComponent.h"



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
   .StartColor(bPrimary ? Brand : Panel).EndColor(bPrimary ? FLinearColor(.82f, 1.0f, .025f) : PanelRaised)
   .BorderColor(bPrimary ? Brand : Hairline).HoverBorderColor(Cyan).Padding(2)
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
   FVector2D Screen(CanvasSize.X * .5f, CanvasSize.Y * .65f);
   int32 Width = 0, Height = 0;
   if (PlayerController.IsValid() && Presentation())
   {
    PlayerController->GetViewportSize(Width, Height);
    if (Width > 0 && Height > 0 && PlayerController->ProjectWorldLocationToScreen(
     Presentation()->GetWinnerPosition(Slot) + FVector(0, -100, -20), Screen))
    {
     // Use the puck's center for horizontal alignment; the point in front of
     // it supplies only the label height (perspective otherwise splays labels).
     FVector2D PuckCenter;
     if (PlayerController->ProjectWorldLocationToScreen(Presentation()->GetWinnerPosition(Slot), PuckCenter))
      Screen.X = PuckCenter.X;
     const FVector2D FullSize(Width, Height);
     // Canvas units are after FLICK's DPI scaler. LayerLocalSize is before it;
     // mixing the two caused labels to drift away from their pucks.
     const FVector2D FrameSize = FlickPresentationFrame::GetContainedSize(FullSize);
     Screen = (Screen - (FullSize - FrameSize) * .5f)
      * FVector2D(CanvasSize.X / FrameSize.X, CanvasSize.Y / FrameSize.Y);
    }
   }
   return FMargin(Screen.X, Screen.Y, 0, 0);
  })
  [SNew(SBox).WidthOverride(245).HeightOverride(108).Visibility_Lambda([this, Slot, ShowDetails]()
  {
   const auto* State = GetScoreboardGameState();
   return !ShowDetails() && State && State->WinnerTeam != EFlickTeam::None && Slot < State->PlayersPerTeam
    ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
  })
   [SNew(SVerticalBox)
    + SVerticalBox::Slot().AutoHeight()
    [SNew(SBox).HeightOverride(28)
     [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
     [CenteredLine(TAttribute<FText>::CreateLambda([this, Slot]()
      {
       const auto* State = GetScoreboardGameState();
       FString Name = State ? GetScoreboardPlayerName(State->WinnerTeam, Slot).ToString() : FString();
       Name.ReplaceInline(TEXT("\r"), TEXT(" "));
       Name.ReplaceInline(TEXT("\n"), TEXT(" "));
       // Keep unusually long display names readable rather than shrinking
       // them to tiny text. The full name remains on the summary scoreboard.
       if (Name.Len() > 28) Name = Name.Left(25) + TEXT("...");
       return FText::FromString(Name);
      }), 18, Paper)]]]
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
   ]];
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
  return FText::FromString(FString::Printf(TEXT("%s  /  %s  /  ROUND %d, SHOT %d"),
   State->bDecisiveShotSelfKnockout ? TEXT("DECISIVE SELF-KNOCKOUT") : TEXT("DECISIVE SHOT"),
   *PlayerName,
   State->RoundNumber, State->DecisiveShotTurn));
 };
 return SNew(SOverlay)
 + SOverlay::Slot()
 [SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0, 0, 0, .18f))
  .Visibility_Lambda([ShowDetails]() { return ShowDetails() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]
 + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(24, 32)
 [SNew(SVerticalBox)
  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
  [Text(TAttribute<FText>::CreateLambda([this]()
   { const auto* State = GetScoreboardGameState(); return FText::FromString(!State || State->WinnerTeam == EFlickTeam::None ? TEXT("MATCH DRAW")
     : State->WinnerTeam == EFlickTeam::Player1 ? TEXT("BLUE WINS") : TEXT("ORANGE WINS")); }), 36, Paper)]
  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)
  [SNew(SBox).HeightOverride(20).WidthOverride_Lambda([WinnerCanvas]()
   {
    const auto Canvas = WinnerCanvas.Pin();
    return Canvas.IsValid() ? FMath::Clamp(static_cast<float>(Canvas->GetCachedGeometry().GetLocalSize().X) - 80.0f, 1.0f, 1100.0f) : 1100.0f;
   })
   [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
    [CenteredLine(TAttribute<FText>::CreateLambda(DecisiveMoment), 11, Brand)]]]
 ]
 + SOverlay::Slot()[Winners]
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
      [SNew(SFlickAngularBorder).BackgroundColor(Panel.CopyWithNewOpacity(.94f)).AccentColor(Brand).Padding(20)[Menu]]]
     + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[BuildScoreboardOverlay(true)]
    ]
   ]
  ]
 ];
}
