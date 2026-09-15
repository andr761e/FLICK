// pause/results, shared controls and dynamic match text
#include "UI/FlickGameLayerPrivate.h"

TSharedRef<SWidget> SFlickGameLayer::BuildPauseOverlay()
{
	auto MakePauseButton = [this](
		const FString& Label,
		const bool bDanger,
		const FOnClicked& OnClicked) -> TSharedRef<SWidget>
	{
		const bool bPrimary = Label == TEXT("RESUME");
		TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Cursor(EMouseCursor::Hand)
			.OnClicked(OnClicked);

		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([WeakButton, bDanger, bPrimary]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				const bool bActive = PinnedButton.IsValid()
					&& (PinnedButton->IsHovered() || PinnedButton->HasKeyboardFocus());
				if (bDanger)
				{
					return bActive
						? FLinearColor(0.15f, 0.041f, 0.022f, 1.0f)
						: PanelRaised;
				}
				if (bPrimary) return bActive ? Paper : Brand;
				return bActive ? FLinearColor(0.085f, 0.09f, 0.078f, 1.0f) : PanelRaised;
			})
			.AccentColor_Lambda([WeakButton, bDanger, bPrimary]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				const bool bActive = PinnedButton.IsValid()
					&& (PinnedButton->IsHovered() || PinnedButton->HasKeyboardFocus());
				if (bDanger)
				{
					return Orange.CopyWithNewOpacity(bActive ? 1.0f : 0.38f);
				}
				return bPrimary || bActive ? Brand : Hairline;
			})
			.CutSize(5.0f)
			.BorderWidth(1.0f)
			.UseAccentForOutline(true)
			.Padding(FMargin(2.0f))
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.0f, 0.0f, 20.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(17, true)).ColorAndOpacity(bPrimary ? Ink : bDanger ? Orange : Muted)
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(22.0f, 0.0f, 42.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Label]()
						{
							if (Label == TEXT("CHANGE CLASS")
								&& GameMode.IsValid()
								&& GameMode->HasPendingClassChanges())
							{
								return FText::FromString(TEXT("CHANGE CLASS  (QUEUED)"));
							}
							return FText::FromString(Label == TEXT("FORFEIT")
								&& GameMode.IsValid()
								&& GameMode->IsTrainingMode()
									? TEXT("EXIT TRAINING")
									: Label);
						})
						.Font(UiFont(15, true))
						.ColorAndOpacity(bPrimary ? Ink : bDanger ? Orange : Paper)
					]
				]
			]
		);

		return SNew(SBox).HeightOverride(bPrimary ? 64.0f : 54.0f)[Button];
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.003f, 0.004f, 0.003f, 0.8f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(500.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel.CopyWithNewOpacity(0.985f))
				.AccentColor(Brand)
				.CutSize(10.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(32.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("FLICK / IN SESSION"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 24.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("MATCH MENU"))).Font(DisplayFont(34)).ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						MakePauseButton(TEXT("RESUME"), false, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->TogglePauseMenu(); return FReply::Handled(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return GameMode.IsValid() && GameMode->CanOpenClassChange()
								? EVisibility::Visible
								: EVisibility::Collapsed;
						})
						[
							MakePauseButton(TEXT("CHANGE CLASS"), false, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->OpenClassChange(); return FReply::Handled(); }))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						MakePauseButton(TEXT("SETTINGS"), false, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->OpenSettings(); return FReply::Handled(); }))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakePauseButton(TEXT("FORFEIT"), true, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ReturnToMainMenu(); return FReply::Handled(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ESC   RETURN TO GAME")))
						.Font(UiFont(9, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildRoundOverOverlay()
{
	FSlateFontInfo RoundHeaderFont = UiFont(11, true);
	RoundHeaderFont.LetterSpacing = 120;
	FSlateFontInfo RoundScoreFont = UiFont(18, true);
	RoundScoreFont.LetterSpacing = 40;
	FSlateFontInfo RoundButtonFont = UiFont(17, true);
	RoundButtonFont.LetterSpacing = 30;

	TSharedRef<SButton> NextActionButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(FMargin(0.0f))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this]()
		{
			const AFlickGameState* State = PlayerController.IsValid()
				? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
				: nullptr;
			if (State && State->bSeriesComplete && State->bMatchmakingLobby)
			{
				if (GameMode.IsValid()) GameMode->ReturnToMainMenu();
				else if (PlayerController.IsValid()) PlayerController->LeaveNetworkSession();
			}
			else if (PlayerController.IsValid())
			{
				if (State && State->bSeriesComplete) PlayerController->RequestRestartMatch();
				else PlayerController->RequestNextRound();
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakNextActionButton = NextActionButton;
	NextActionButton->SetContent(
		SNew(SFlickAngularBorder)
		.BackgroundColor_Lambda([WeakNextActionButton]()
		{
			const TSharedPtr<SButton> PinnedButton = WeakNextActionButton.Pin();
			return PinnedButton.IsValid()
				&& (PinnedButton->IsHovered() || PinnedButton->HasKeyboardFocus())
				? Paper : Brand;
		})
		.AccentColor(Brand)
		.CutSize(5.0f)
		.BorderWidth(1.0f)
		.UseAccentForOutline(true)
		.Padding(FMargin(22.0f, 0.0f))
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const AFlickGameState* State = PlayerController.IsValid()
						? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
						: nullptr;
					if (State && State->bSeriesComplete)
					{
						return FText::FromString(State->bMatchmakingLobby ? TEXT("RETURN TO PARTY") : TEXT("REMATCH"));
					}
					return FText::FromString(FString::Printf(
						TEXT("NEXT ROUND  /  %02d"),
						State ? FMath::CeilToInt(State->GetRoundAdvanceTimeRemaining()) : 0));
				})
				.Font(RoundButtonFont)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(Ink)
			]
		]
	);

	TSharedRef<SVerticalBox> ResultLayout = SNew(SVerticalBox);
	ResultLayout->AddSlot().AutoHeight()
	[
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				const AFlickGameState* State = GetScoreboardGameState();
				if (State && State->bSeriesComplete) return FText::FromString(TEXT("MATCH COMPLETE"));
				return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
					? TEXT("BOB COMPLETE") : TEXT("ROUND COMPLETE"));
			})
			.Font(RoundHeaderFont)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(Brand)
		]
	];
	ResultLayout->AddSlot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 22.0f)
	[
		SNew(SBox).HeightOverride(76.0f)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return GetRoundResultText(); })
				.Font(DisplayFont(44))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity_Lambda([this]() { return GetMatchStatusColor(); })
			]
		]
	];
	ResultLayout->AddSlot().AutoHeight()
	[
		SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(18.0f, 16.0f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return GetRoundScoreText(); })
			.Font(RoundScoreFont)
			.AutoWrapText(true)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(Paper)
		]
	];
	ResultLayout->AddSlot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
	[
		SNew(STextBlock)
		.Visibility_Lambda([this]()
		{
			const AFlickGameState* State = PlayerController.IsValid()
				? PlayerController->GetWorld()->GetGameState<AFlickGameState>() : nullptr;
			return State && State->bSeriesComplete && State->bRankedMatch
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		.Text_Lambda([this]()
		{
			const AFlickGameState* State = PlayerController.IsValid()
				? PlayerController->GetWorld()->GetGameState<AFlickGameState>() : nullptr;
			const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
			if (!State || !Ranking || !Ranking->HasRatingUpdateForMatch(State->MatchId))
			{
				return FText::FromString(TEXT("RANKED RESULT SYNCING..."));
			}
			const FFlickRatingUpdate& Update = Ranking->GetLastRatingUpdate();
			const FFlickRankProgress Progress = Ranking->GetProgress(Update.Variant, Update.PlayersPerTeam);
			return FText::FromString(FString::Printf(
				TEXT("RATING  %s%d  /  %s%s"),
				Update.RatingDelta >= 0 ? TEXT("+") : TEXT(""),
				Update.RatingDelta,
				*FlickRankRules::GetProgressLabel(Progress),
				Update.bForfeit ? TEXT("  /  FORFEIT") : TEXT("")));
		})
		.Font(UiFont(11, true))
		.AutoWrapText(true)
		.Justification(ETextJustify::Center)
		.ColorAndOpacity_Lambda([this]()
		{
			const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
			return FSlateColor(Ranking && Ranking->GetLastRatingUpdate().RatingDelta < 0 ? Orange : Brand);
		})
	];
	ResultLayout->AddSlot().AutoHeight().Padding(0.0f, 22.0f, 0.0f, 0.0f)
	[
		SNew(SBox).HeightOverride(62.0f)[NextActionButton]
	];

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(640.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel.CopyWithNewOpacity(0.985f))
				.AccentColor(Brand)
				.CutSize(10.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(40.0f, 32.0f))
				[
					ResultLayout
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeMainMenuButton(
	const FString& Label,
	const FOnClicked& OnClicked,
	const bool bPrimary,
	const bool bDanger,
	const float Height)
{
	EFlickMainMenuIcon Icon = EFlickMainMenuIcon::Play;
	if (Label == TEXT("STATS")) Icon = EFlickMainMenuIcon::Stats;
	else if (Label.Contains(TEXT("LEADERBOARD"))) Icon = EFlickMainMenuIcon::Leaderboard;
	else if (Label.Contains(TEXT("HISTORY"))) Icon = EFlickMainMenuIcon::History;
	else if (Label == TEXT("BACK")) Icon = EFlickMainMenuIcon::Back;
	else if (Label.Contains(TEXT("LINEUP"))) Icon = EFlickMainMenuIcon::Lineups;
	else if (Label.Contains(TEXT("PROFILE"))) Icon = EFlickMainMenuIcon::Profile;
	else if (Label.Contains(TEXT("SHOP"))) Icon = EFlickMainMenuIcon::Shop;
	else if (Label.Contains(TEXT("SETTINGS"))) Icon = EFlickMainMenuIcon::Settings;
	else if (Label.Contains(TEXT("QUIT"))) Icon = EFlickMainMenuIcon::Quit;

	const FString Detail = Label == TEXT("PLAY") ? TEXT("Find your next rivalry")
		: Label == TEXT("LINEUPS") ? TEXT("Build your advantage")
		: Label == TEXT("PROFILE") ? TEXT("Your record. Your progress.")
		: Label == TEXT("ITEM SHOP") ? TEXT("Explore the collection")
		: Label == TEXT("SETTINGS") ? TEXT("Make yourself comfortable") : FString();
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(22.0f, 0.0f, 24.0f, 0.0f))
		.HAlign(HAlign_Fill).VAlign(VAlign_Center).OnClicked(OnClicked);
	const TWeakPtr<SButton> WeakButton = Button;
	Button->SetContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
		[SNew(SFlickMainMenuIcon).Icon(Icon).Color(bPrimary ? Ink : bDanger ? Muted : Brand)]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(bPrimary ? 30 : 21)).ColorAndOpacity(bPrimary ? Ink : Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Detail)).Font(UiFont(10)).ColorAndOpacity(bPrimary ? Ink : Muted)
				.Visibility(Detail.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[SNew(STextBlock).Text(FText::FromString(TEXT("\u2197"))).Font(UiFont(20)).ColorAndOpacity(bPrimary ? Ink : Brand)]);
	return SNew(SBox).HeightOverride(Height)
	[
		SNew(SFlickMainMenuFrame)
		.StartColor(bPrimary ? Brand : Panel).EndColor(bPrimary ? Brand : Panel)
		.HoverStartColor(bPrimary ? FMath::Lerp(Brand, Paper, 0.16f) : PanelRaised).HoverEndColor(bPrimary ? FMath::Lerp(Brand, Paper, 0.16f) : PanelRaised)
		.BorderColor(bPrimary ? Brand : Hairline).HoverBorderColor(bDanger ? Orange : Brand)
		.Primary(bPrimary)
		.Highlighted_Lambda([WeakButton]()
		{
			const TSharedPtr<SButton> Pinned = WeakButton.Pin();
			return Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus());
		})
		.BorderWidth(1.0f).Padding(FMargin(1.0f))[Button]
	];
}

TSharedRef<SWidget> SFlickGameLayer::MakeMenuButton(
	const FString& Label,
	const FOnClicked& OnClicked,
	const bool bPrimary,
	const bool bDanger,
	const float Height)
{
	FString Icon = TEXT(">");
	if (Label.Contains(TEXT("BACK"))) Icon = TEXT("<");
	else if (Label.Contains(TEXT("LINEUP"))) Icon = TEXT("III");
	else if (Label.Contains(TEXT("SHOP"))) Icon = TEXT("$");
	else if (Label.Contains(TEXT("SETTINGS"))) Icon = TEXT("*");
	else if (Label.Contains(TEXT("QUIT")) || Label.Contains(TEXT("MAIN MENU"))) Icon = TEXT("X");
	else if (Label.Contains(TEXT("RESTART")) || Label.Contains(TEXT("REMATCH"))) Icon = TEXT("R");
	else if (Label.Contains(TEXT("REFRESH"))) Icon = TEXT("↻");
	else if (Label.Contains(TEXT("APPLY")) || Label.Contains(TEXT("SAVE"))) Icon = TEXT("+");
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(FMargin(12.0f, 6.0f))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.OnClicked(OnClicked)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 10.0f, 2.0f)
			[
				SNew(SBox).WidthOverride(bPrimary ? 5.0f : 3.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(bPrimary ? Ink : bDanger ? Orange : Brand)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(24.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Icon))
					.Font(UiFont(11, true))
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(bDanger ? Orange : bPrimary ? Ink : Brand)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(UiFont(15, true))
				.ColorAndOpacity(bPrimary ? Ink : Paper)
			]
		];
	const TWeakPtr<SButton> WeakButton = Button;
	return SNew(SBox)
		.HeightOverride(Height)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([WeakButton, bPrimary, bDanger]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				const bool bActive = PinnedButton.IsValid()
					&& (PinnedButton->HasKeyboardFocus() || PinnedButton->IsHovered());
				if (bDanger)
				{
					return bActive
						? FLinearColor(0.15f, 0.027f, 0.012f, 0.94f)
						: FLinearColor(0.026f, 0.009f, 0.008f, 0.72f);
				}
				if (bPrimary)
				{
					return bActive
						? FMath::Lerp(Brand, Paper, 0.16f) : Brand;
				}
				return bActive
					? PanelRaised : Panel;
			})
			.AccentColor_Lambda([WeakButton, bPrimary, bDanger]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				if (PinnedButton.IsValid() && (PinnedButton->HasKeyboardFocus() || PinnedButton->IsHovered()))
				{
					return Brand;
				}
				return bPrimary ? Brand : bDanger ? Orange : Hairline;
			})
			.CutSize(8.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(1.0f))
			[
				Button
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeToggleRow(
	const FString& Label,
	const TAttribute<ECheckBoxState>& State,
	const FOnCheckStateChanged& OnChanged) const
{
	return SNew(SBox).HeightOverride(58.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(74.0f).HeightOverride(32.0f)
				[
					SNew(SCheckBox)
					.Style(&ToggleStyle)
					.IsChecked(State)
					.OnCheckStateChanged(OnChanged)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([State]() { return FText::FromString(State.Get() == ECheckBoxState::Checked ? TEXT("ON") : TEXT("OFF")); })
						.Font(UiFont(10, true)).ColorAndOpacity_Lambda([State]() { return State.Get() == ECheckBoxState::Checked ? Ink : Paper; })
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeSliderRow(
	const FString& Label,
	const TAttribute<float>& Value,
	const FOnFloatValueChanged& OnChanged) const
{
	return SNew(SBox).HeightOverride(58.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.36f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
			+ SHorizontalBox::Slot().FillWidth(0.54f).VAlign(VAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().VAlign(VAlign_Center)
				[
					SNew(SBox).HeightOverride(6.0f)
					[
						SNew(SProgressBar)
						.Style(&ShotClockBarStyle)
						.Percent_Lambda([Value]() { return TOptional<float>(Value.Get()); })
						.FillColorAndOpacity(Brand)
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SSlider).Style(&SliderStyle).Value(Value).OnValueChanged(OnChanged)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.1f).VAlign(VAlign_Center).HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text_Lambda([Value]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value.Get() * 100.0f))); })
				.Font(UiFont(11, true)).ColorAndOpacity(Brand)
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeCycleRow(
	const FString& Label,
	const TAttribute<FText>& Value,
	const FOnClicked& Previous,
	const FOnClicked& Next) const
{
	return SNew(SBox).HeightOverride(58.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(36.0f).HeightOverride(34.0f)[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(0.0f).OnClicked(Previous)[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(UiFont(13, true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)]]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(154.0f)[SNew(STextBlock).Text(Value).Font(UiFont(12, true)).Justification(ETextJustify::Center).ColorAndOpacity(Brand)]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(36.0f).HeightOverride(34.0f)[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(0.0f).OnClicked(Next)[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(13, true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)]]]
		];
}

EVisibility SFlickGameLayer::GetScreenVisibility(const EFlickFrontendScreen Screen) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (State && State->bNetworkClassSelectionActive)
	{
		return Screen == EFlickFrontendScreen::ClassSelect
			? EVisibility::Visible : EVisibility::Collapsed;
	}
	if (!GameMode.IsValid())
	{
		if (!State)
		{
			return EVisibility::Collapsed;
		}
		const bool bVisible = Screen == EFlickFrontendScreen::PrivateMatch
			? State->bPrivateMatchLobbyActive
			: Screen == EFlickFrontendScreen::NetworkLobby
				? false
				: Screen == EFlickFrontendScreen::MainMenu
					? State->bPartyActive && !State->bPrivateMatchLobbyActive && !State->bNetworkLobbyActive
					: false;
		return bVisible ? EVisibility::Visible : EVisibility::Collapsed;
	}
	const EFlickFrontendScreen CurrentScreen = GameMode->GetFrontendScreen();
	if (CurrentScreen == Screen)
	{
		return EVisibility::Visible;
	}
	if (CurrentScreen == EFlickFrontendScreen::Settings
		&& GameMode->GetSettingsReturnScreen() == Screen
		&& Screen != EFlickFrontendScreen::Paused)
	{
		return EVisibility::HitTestInvisible;
	}
	return EVisibility::Collapsed;
}

UFlickSessionSubsystem* SFlickGameLayer::GetDisplayedSessionSubsystem() const
{
	if (GameMode.IsValid())
	{
		return GameMode->GetFlickSessionSubsystem();
	}
	return PlayerController.IsValid() && PlayerController->GetGameInstance()
		? PlayerController->GetGameInstance()->GetSubsystem<UFlickSessionSubsystem>()
		: nullptr;
}

bool SFlickGameLayer::IsDisplayedPartyActive() const
{
	if (GameMode.IsValid())
	{
		return GameMode->IsPartySession();
	}
	const AFlickGameState* State = GetScoreboardGameState();
	return State && State->bPartyActive;
}

AFlickPlayerState* SFlickGameLayer::GetDisplayedPartyMember(const int32 PartySlot) const
{
	if (GameMode.IsValid())
	{
		return GameMode->GetPartyMember(PartySlot);
	}
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State)
	{
		return nullptr;
	}
	for (APlayerState* PlayerState : State->PlayerArray)
	{
		AFlickPlayerState* Member = Cast<AFlickPlayerState>(PlayerState);
		if (Member && Member->GetPartySlot() == PartySlot)
		{
			return Member;
		}
	}
	return nullptr;
}

int32 SFlickGameLayer::GetDisplayedPartyMemberCount() const
{
	if (GameMode.IsValid())
	{
		return GameMode->GetPartyMemberCount();
	}
	int32 Count = 0;
	for (int32 PartySlot = 0; PartySlot < FlickMaximumPartyMembers; ++PartySlot)
	{
		Count += GetDisplayedPartyMember(PartySlot) ? 1 : 0;
	}
	return Count;
}

EVisibility SFlickGameLayer::GetMatchHudVisibility() const
{
	if (GameMode.IsValid())
	{
		if (GameMode->IsCinematicReplayActive())
		{
			return EVisibility::Collapsed;
		}
		const EFlickFrontendScreen Screen = GameMode->GetFrontendScreen();
		return Screen == EFlickFrontendScreen::Playing || Screen == EFlickFrontendScreen::Paused
			? EVisibility::SelfHitTestInvisible
			: EVisibility::Collapsed;
	}
	const AFlickGameState* State = GetScoreboardGameState();
	return State && State->IsGameplayActive()
		&& !State->bNetworkLobbyActive
		&& !State->bPrivateMatchLobbyActive
		&& !State->bPartyActive
		? EVisibility::SelfHitTestInvisible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetScoreboardVisibility() const
{
	if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
	{
		return EVisibility::Collapsed;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickScoreboardPreview")))
	{
		return EVisibility::HitTestInvisible;
	}
	if (!PlayerController.IsValid() || !PlayerController->IsScoreboardVisible())
	{
		return EVisibility::Collapsed;
	}
	if (GameMode.IsValid() && GameMode->GetFrontendScreen() != EFlickFrontendScreen::Playing)
	{
		return EVisibility::Collapsed;
	}
	const AFlickGameState* State = GetScoreboardGameState();
	return State && State->IsGameplayActive()
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetScoreboardPlayerVisibility(const int32 PlayerSlot) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const int32 PlayersPerTeam = State ? State->PlayersPerTeam : 1;
	return PlayerSlot >= 0 && PlayerSlot < PlayersPerTeam
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetRoundOverVisibility() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return EVisibility::Visible;
	const AFlickGameState* State = GetScoreboardGameState();
	const bool bPlayingScreen = !GameMode.IsValid()
		|| GameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing;
	return bPlayingScreen && State && State->MatchPhase == EFlickMatchPhase::RoundOver
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetPowerVisibility() const
{
	return PlayerController.IsValid() && PlayerController->IsAimingShot() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetLoadoutRowVisibility(const int32 SlotIndex) const
{
	return GameMode.IsValid() && SlotIndex < GameMode->GetLoadoutEditingPieceCount() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetEventVisibility(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid() || !OwnerHud->GetWorld()) return EVisibility::Collapsed;
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex) && Events[EventIndex].ExpiresAt > OwnerHud->GetWorld()->GetTimeSeconds()
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

FText SFlickGameLayer::GetEventText(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return FText::GetEmpty();
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex) ? FText::FromString(Events[EventIndex].Message) : FText::GetEmpty();
}

FSlateColor SFlickGameLayer::GetEventColor(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return FSlateColor(FLinearColor::White);
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return FSlateColor(Events.IsValidIndex(EventIndex) ? Events[EventIndex].Color : FLinearColor::White);
}

EVisibility SFlickGameLayer::GetEventPointsVisibility(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return EVisibility::Collapsed;
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex) && !Events[EventIndex].PointsText.IsEmpty()
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

FText SFlickGameLayer::GetEventPointsText(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return FText::GetEmpty();
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex)
		? FText::FromString(Events[EventIndex].PointsText)
		: FText::GetEmpty();
}

FText SFlickGameLayer::GetScoreboardPlayerName(
	const EFlickTeam Team,
	const int32 PlayerSlot) const
{
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		if (Team == EFlickTeam::Player2)
		{
			return FText::FromString(TEXT("TRAINING BOT"));
		}
		if (Team == EFlickTeam::Player1)
		{
			return FText::FromString(TEXT("YOU"));
		}
	}
	if (const AFlickGameState* State = GetScoreboardGameState())
	{
		for (const APlayerState* BasePlayerState : State->PlayerArray)
		{
			const AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(BasePlayerState);
			if (FlickPlayerState
				&& ((FlickPlayerState->GetTeam() == Team
						&& FlickPlayerState->GetTeamPlayerSlot() == PlayerSlot)
					|| FlickPlayerState->ControlsPrivateSlot(Team, PlayerSlot))
				&& !FlickPlayerState->GetPlayerName().IsEmpty())
			{
				return FText::FromString(FlickPlayerState->GetPlayerName());
			}
		}
	}

	return FText::FromString(Team == EFlickTeam::Player1
		? FString::Printf(TEXT("BLUE PLAYER %d"), PlayerSlot + 1)
		: FString::Printf(TEXT("ORANGE PLAYER %d"), PlayerSlot + 1));
}

FText SFlickGameLayer::GetScoreboardStatText(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const int32 StatIndex) const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickScoreboardPreview")))
	{
		static const int32 PreviewStats[2][3][4] =
		{
			{{245, 1, 6, 17}, {135, 1, 4, 5}, {65, 0, 3, 7}},
			{{220, 1, 5, 14}, {100, 0, 4, 12}, {55, 0, 3, 5}}
		};
		const int32 TeamIndex = Team == EFlickTeam::Player2 ? 1 : 0;
		return FText::AsNumber(PreviewStats[TeamIndex][FMath::Clamp(PlayerSlot, 0, 2)][FMath::Clamp(StatIndex, 0, 3)]);
	}

	const AFlickGameState* State = GetScoreboardGameState();
	const FFlickPlayerMatchStats* Stats = State
		? State->FindPlayerMatchStats(Team, PlayerSlot)
		: nullptr;
	if (!Stats)
	{
		return FText::AsNumber(0);
	}
	const int32 Value = StatIndex == 0
		? Stats->Score
		: StatIndex == 1
			? Stats->Knockouts
			: StatIndex == 2
				? Stats->Shots
				: State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
					? Stats->Impacts
					: Stats->SurvivingPucks;
	return FText::AsNumber(Value);
}

FText SFlickGameLayer::GetScoreboardTeamSummary(const EFlickTeam Team) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State)
	{
		return FText::GetEmpty();
	}
	if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
	{
		const int32 ActivePieces = Team == EFlickTeam::Player1
			? State->Player1ActivePieces
			: State->Player2ActivePieces;
		return FText::FromString(FString::Printf(TEXT("PUCKS ACTIVE  %d"), ActivePieces));
	}
	if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		const int32 ActivePieces = Team == EFlickTeam::Player1
			? State->Player1ActivePieces
			: State->Player2ActivePieces;
		return FText::FromString(FString::Printf(TEXT("PUCKS LEFT  %d"), ActivePieces));
	}
	const int32 RoundWins = Team == EFlickTeam::Player1
		? State->Player1RoundsWon
		: State->Player2RoundsWon;
	return FText::FromString(FString::Printf(
		TEXT("ROUND WINS  %d   |   TEAM SCORE  %d"),
		RoundWins,
		State->GetTeamScore(Team)));
}

FText SFlickGameLayer::GetScoreboardMatchSummary() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State)
	{
		return FText::FromString(TEXT("LIVE MATCH"));
	}
	const FString Mode = State->ActiveMatchVariant == EFlickMatchVariant::Bob
		? TEXT("BOB")
		: FString::Printf(TEXT("%dV%d FLICK"), State->PlayersPerTeam, State->PlayersPerTeam);
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		return FText::FromString(FString::Printf(TEXT("%s   /   TRAINING BOT   /   ROUND %d"), *Mode, State->RoundNumber));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingMode())
	{
		return FText::FromString(FString::Printf(TEXT("%s   /   OFFLINE FREE PLAY"), *Mode));
	}
	return FText::FromString(FString::Printf(TEXT("%s   /   ROUND %d"), *Mode, State->RoundNumber));
}

const AFlickGameState* SFlickGameLayer::GetScoreboardGameState() const
{
	if (GameMode.IsValid())
	{
		return GameMode->GetFlickGameState();
	}
	return PlayerController.IsValid() && PlayerController->GetWorld()
		? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
		: nullptr;
}

FText SFlickGameLayer::GetMatchStatusText() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FText::GetEmpty();
	if (GameMode.IsValid() && GameMode->IsCinematicReplayActive())
	{
		return FText::FromString(TEXT("ROUND-WINNING SHOT"));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		if (State->MatchPhase == EFlickMatchPhase::RoundOver)
		{
			return GetRoundResultText();
		}
		if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
		{
			return FText::FromString(GameMode->IsResolvingKickoff()
				? TEXT("KICKOFF IN MOTION")
				: TEXT("PUCKS IN MOTION"));
		}
		if (State->PlayersPerTeam > 1)
		{
			const TCHAR* TeamName = State->CurrentTeam == EFlickTeam::Player2
				? TEXT("ORANGE") : TEXT("BLUE");
			const int32 PlayerNumber = State->CurrentTeamPlayerSlot + 1;
			if (State->CurrentTeam == EFlickTeam::Player2)
			{
				return FText::FromString(State->MatchPhase == EFlickMatchPhase::KickoffPlanning
					? FString::Printf(TEXT("%s P%d  /  BOT SETTING KICKOFF"), TeamName, PlayerNumber)
					: FString::Printf(TEXT("%s P%d  /  BOT THINKING"), TeamName, PlayerNumber));
			}
			return FText::FromString(State->MatchPhase == EFlickMatchPhase::KickoffPlanning
				? FString::Printf(TEXT("%s P%d  /  SET YOUR KICKOFF"), TeamName, PlayerNumber)
				: FString::Printf(TEXT("%s P%d  /  YOUR TURN"), TeamName, PlayerNumber));
		}
		if (State->CurrentTeam == EFlickTeam::Player2)
		{
			return FText::FromString(TEXT("BOT THINKING"));
		}
		return FText::FromString(State->MatchPhase == EFlickMatchPhase::KickoffPlanning
			? TEXT("SET YOUR KICKOFF")
			: TEXT("YOUR TURN"));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingMode())
	{
		if (GameMode->IsTrainingEditMode())
		{
			return FText::FromString(GameMode->GetTrainingPlacementTeam() == EFlickTeam::Player1
				? TEXT("PLACE YOUR PUCKS")
				: TEXT("PLACE TARGET PUCKS"));
		}
		return FText::FromString(State->MatchPhase == EFlickMatchPhase::ResolvingPhysics
			? TEXT("PUCKS IN MOTION")
			: TEXT("SELECT ANY BLUE PUCK"));
	}
	if (State->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		return FText::FromString(State->PlayersPerTeam > 1
			? FString::Printf(
				TEXT("%s P%d SET KICKOFF  /  %d OF %d LOCKED"),
				State->CurrentTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
				State->CurrentTeamPlayerSlot + 1,
				State->KickoffShotsLocked,
				State->KickoffShotsRequired)
			: FString::Printf(TEXT("PLAYER %d SET KICKOFF"), GetTeamNumber(State->CurrentTeam)));
	}
	if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
	{
		if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
		{
			return FText::FromString(TEXT("TABLE IN MOTION"));
		}
		return FText::FromString(GameMode.IsValid() && GameMode->IsResolvingKickoff()
			? TEXT("KICKOFF IN MOTION")
			: TEXT("PUCKS IN MOTION"));
	}
	if (State->MatchPhase == EFlickMatchPhase::RoundOver) return GetRoundResultText();
	if (State->PlayersPerTeam > 1)
	{
		return FText::FromString(State->ActiveMatchVariant == EFlickMatchVariant::Bob
			? FString::Printf(
				TEXT("%s P%d STRIKER"),
				State->CurrentTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
				State->CurrentTeamPlayerSlot + 1)
			: FString::Printf(
				TEXT("%s P%d  /  TURN"),
				State->CurrentTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
				State->CurrentTeamPlayerSlot + 1));
	}
	return FText::FromString(State->ActiveMatchVariant == EFlickMatchVariant::Bob
		? FString::Printf(TEXT("PLAYER %d STRIKER"), GetTeamNumber(State->CurrentTeam))
		: FString::Printf(TEXT("PLAYER %d TURN"), GetTeamNumber(State->CurrentTeam)));
}

FSlateColor SFlickGameLayer::GetMatchStatusColor() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return FSlateColor(Orange);
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FSlateColor(FLinearColor::White);
	if (State->bShotClockActive
		&& State->GetShotClockFraction(State->CurrentTeam) <= 1.0f / 3.0f)
	{
		return FSlateColor(FLinearColor(1.0f, 0.34f, 0.12f, 1.0f));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
		{
			return FSlateColor(FLinearColor(1.0f, 0.72f, 0.12f, 1.0f));
		}
		return FSlateColor(GetTeamAccent(
			State->MatchPhase == EFlickMatchPhase::RoundOver ? State->WinnerTeam : State->CurrentTeam));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingMode())
	{
		if (GameMode->IsTrainingEditMode())
		{
			return FSlateColor(GetTeamAccent(GameMode->GetTrainingPlacementTeam()));
		}
		return FSlateColor(State->MatchPhase == EFlickMatchPhase::ResolvingPhysics
			? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f)
			: FLinearColor(0.2f, 0.78f, 0.5f, 1.0f));
	}
	if (State->MatchPhase == EFlickMatchPhase::KickoffPlanning) return FSlateColor(GetTeamAccent(State->CurrentTeam));
	if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics) return FSlateColor(FLinearColor(1.0f, 0.72f, 0.12f, 1.0f));
	return FSlateColor(GetTeamAccent(State->MatchPhase == EFlickMatchPhase::RoundOver ? State->WinnerTeam : State->CurrentTeam));
}

FText SFlickGameLayer::GetNextTurnText() const
{
	EFlickTeam NextTeam = EFlickTeam::None;
	int32 NextPlayerSlot = INDEX_NONE;
	if (!GameMode.IsValid() || !GameMode->GetNextScheduledTurn(NextTeam, NextPlayerSlot))
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(
		TEXT("NEXT TURN  P%d  /  %s"),
		NextPlayerSlot + 1,
		NextTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE")));
}

FSlateColor SFlickGameLayer::GetNextTurnColor() const
{
	EFlickTeam NextTeam = EFlickTeam::None;
	int32 NextPlayerSlot = INDEX_NONE;
	return FSlateColor(GameMode.IsValid()
		&& GameMode->GetNextScheduledTurn(NextTeam, NextPlayerSlot)
		? GetTeamAccent(NextTeam)
		: FLinearColor::Transparent);
}

EVisibility SFlickGameLayer::GetNextTurnVisibility() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	EFlickTeam NextTeam = EFlickTeam::None;
	int32 NextPlayerSlot = INDEX_NONE;
	return State
		&& State->PlayersPerTeam > 1
		&& (!GameMode.IsValid() || !GameMode->IsCinematicReplayActive())
		&& GameMode.IsValid()
		&& GameMode->GetNextScheduledTurn(NextTeam, NextPlayerSlot)
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FText SFlickGameLayer::GetRoundResultText() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return FText::FromString(TEXT("PLAYER 2 TAKES IT"));
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FText::GetEmpty();
	if (State->bDraw) return FText::FromString(TEXT("ROUND DRAW"));
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		const bool bPlayerWon = State->WinnerTeam == EFlickTeam::Player1;
		return FText::FromString(State->bSeriesComplete
			? bPlayerWon ? TEXT("YOU WIN") : TEXT("BOT WINS")
			: bPlayerWon ? TEXT("YOU TAKE IT") : TEXT("BOT TAKES IT"));
	}
	if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		return FText::FromString(FString::Printf(TEXT("PLAYER %d CLEARS BOB"), GetTeamNumber(State->WinnerTeam)));
	}
	const TCHAR* WinnerLabel = State->PlayersPerTeam > 1 ? TEXT("TEAM") : TEXT("PLAYER");
	return FText::FromString(State->bSeriesComplete
		? FString::Printf(TEXT("%s %d WINS"), WinnerLabel, GetTeamNumber(State->WinnerTeam))
		: FString::Printf(TEXT("%s %d TAKES IT"), WinnerLabel, GetTeamNumber(State->WinnerTeam)));
}

FText SFlickGameLayer::GetRoundScoreText() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return FText::FromString(TEXT("SERIES   0  -  1"));
	const AFlickGameState* State = GetScoreboardGameState();
	if (State && State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		return FText::FromString(FString::Printf(
			TEXT("POCKETED   %d  -  %d"),
			State->StartingPiecesPerTeam - State->Player1ActivePieces,
			State->StartingPiecesPerTeam - State->Player2ActivePieces));
	}
	if (!State)
	{
		return FText::GetEmpty();
	}
	if (State->bSeriesComplete && State->Player1RoundsWon == State->Player2RoundsWon)
	{
		return FText::FromString(FString::Printf(
			TEXT("SERIES   %d  -  %d    |    POINTS   %d  -  %d"),
			State->Player1RoundsWon,
			State->Player2RoundsWon,
			State->GetTeamScore(EFlickTeam::Player1),
			State->GetTeamScore(EFlickTeam::Player2)));
	}
	return FText::FromString(FString::Printf(TEXT("SERIES   %d  -  %d"), State->Player1RoundsWon, State->Player2RoundsWon));
}

FText SFlickGameLayer::GetPieceCountText(const EFlickTeam Team) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const int32 Count = !State ? 0 : Team == EFlickTeam::Player1 ? State->Player1ActivePieces : State->Player2ActivePieces;
	return FText::AsNumber(Count);
}

FText SFlickGameLayer::GetRoundsText(const EFlickTeam Team) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FText::GetEmpty();
	if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
	{
		return FText::FromString(Team == EFlickTeam::Player1
			? TEXT("CONTROL ANY BLUE PUCK")
			: TEXT("CLEAR TO AUTO-RESET"));
	}
	if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		const int32 Remaining = Team == EFlickTeam::Player1 ? State->Player1ActivePieces : State->Player2ActivePieces;
		return FText::FromString(FString::Printf(
			TEXT("POCKETED  %d / %d"),
			State->StartingPiecesPerTeam - Remaining,
			State->StartingPiecesPerTeam));
	}
	const int32 Rounds = Team == EFlickTeam::Player1 ? State->Player1RoundsWon : State->Player2RoundsWon;
	return FText::FromString(FString::Printf(TEXT("ROUNDS  %d / %d"), Rounds, State->RoundsToWin));
}

int32 SFlickGameLayer::GetSelectedLoadoutSlot(const EFlickTeam Team) const
{
	const int32 SelectedSlot = Team == EFlickTeam::Player2
		? Player2SelectedLoadoutSlot
		: Player1SelectedLoadoutSlot;
	const int32 ActiveSlots = GameMode.IsValid()
		? GameMode->GetLoadoutEditingPieceCount()
		: 4;
	return FMath::Clamp(SelectedSlot, 0, FMath::Max(ActiveSlots - 1, 0));
}

void SFlickGameLayer::SelectLoadoutSlot(const EFlickTeam Team, const int32 SlotIndex)
{
	const int32 ActiveSlots = GameMode.IsValid()
		? GameMode->GetLoadoutEditingPieceCount()
		: 4;
	const int32 SafeSlot = FMath::Clamp(SlotIndex, 0, FMath::Max(ActiveSlots - 1, 0));
	if (Team == EFlickTeam::Player2)
	{
		Player2SelectedLoadoutSlot = SafeSlot;
		Player2HoveredLoadoutArchetype.Reset();
	}
	else
	{
		Player1SelectedLoadoutSlot = SafeSlot;
		Player1HoveredLoadoutArchetype.Reset();
	}
	if (GameMode.IsValid())
	{
		GameMode->PlayMenuSound(false);
	}
}

EFlickPieceArchetype SFlickGameLayer::GetPreviewLoadoutArchetype(const EFlickTeam Team) const
{
	const TOptional<EFlickPieceArchetype>& Hovered = Team == EFlickTeam::Player2
		? Player2HoveredLoadoutArchetype
		: Player1HoveredLoadoutArchetype;
	if (Hovered.IsSet())
	{
		return Hovered.GetValue();
	}
	return GameMode.IsValid()
		? GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team))
		: EFlickPieceArchetype::Standard;
}

float SFlickGameLayer::GetLoadoutStatValue(
	const EFlickTeam Team,
	const int32 StatIndex,
	const bool bPreview) const
{
	const EFlickPieceArchetype Archetype = bPreview
		? GetPreviewLoadoutArchetype(Team)
		: GameMode.IsValid()
			? GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team))
			: EFlickPieceArchetype::Standard;
	return GetDisplayStatValue(FlickPieceArchetypeRules::GetDisplayStats(Archetype), StatIndex);
}

TArray<float> SFlickGameLayer::GetLineupProfileStats(const EFlickTeam Team) const
{
	TArray<float> Profile;
	Profile.Init(0.0f, 6);
	const int32 PieceCount = GameMode.IsValid()
		? FMath::Clamp(GameMode->GetLoadoutEditingPieceCount(), 1, 4)
		: 4;
	const int32 SelectedSlot = GetSelectedLoadoutSlot(Team);
	for (int32 SlotIndex = 0; SlotIndex < PieceCount; ++SlotIndex)
	{
		const EFlickPieceArchetype Archetype = SlotIndex == SelectedSlot
			? GetPreviewLoadoutArchetype(Team)
			: GameMode.IsValid()
				? GameMode->GetLoadoutPiece(Team, SlotIndex)
				: EFlickPieceArchetype::Standard;
		const FFlickPieceDisplayStats Stats = FlickPieceArchetypeRules::GetDisplayStats(Archetype);
		for (int32 StatIndex = 0; StatIndex < Profile.Num(); ++StatIndex)
		{
			Profile[StatIndex] += GetDisplayStatValue(Stats, StatIndex) / static_cast<float>(PieceCount);
		}
	}
	return Profile;
}

void SFlickGameLayer::SetHoveredLoadoutArchetype(
	const EFlickTeam Team,
	const EFlickPieceArchetype Archetype)
{
	if (Team == EFlickTeam::Player2)
	{
		Player2HoveredLoadoutArchetype = Archetype;
	}
	else
	{
		Player1HoveredLoadoutArchetype = Archetype;
	}
}

void SFlickGameLayer::ClearHoveredLoadoutArchetype(
	const EFlickTeam Team,
	const EFlickPieceArchetype Archetype)
{
	TOptional<EFlickPieceArchetype>& Hovered = Team == EFlickTeam::Player2
		? Player2HoveredLoadoutArchetype
		: Player1HoveredLoadoutArchetype;
	if (Hovered.IsSet() && Hovered.GetValue() == Archetype)
	{
		Hovered.Reset();
	}
}

FText SFlickGameLayer::GetPowerText() const
{
	const float Power = PlayerController.IsValid() ? PlayerController->GetAimResult().NormalizedPower : 0.0f;
	return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Power * 100.0f)));
}

FSlateColor SFlickGameLayer::GetPowerColor() const
{
	const float Power = PlayerController.IsValid() ? PlayerController->GetAimResult().NormalizedPower : 0.0f;
	if (Power < 0.65f)
	{
		return FSlateColor(FLinearColor::LerpUsingHSV(FLinearColor(0.0f, 0.78f, 0.95f, 1.0f), FLinearColor(1.0f, 0.8f, 0.12f, 1.0f), Power / 0.65f));
	}
	return FSlateColor(FLinearColor::LerpUsingHSV(FLinearColor(1.0f, 0.8f, 0.12f, 1.0f), Orange, (Power - 0.65f) / 0.35f));
}

UFlickRankingSubsystem* SFlickGameLayer::GetRankingSubsystem() const
{
	const UGameInstance* GameInstance = PlayerController.IsValid()
		? PlayerController->GetGameInstance()
		: GameMode.IsValid() ? GameMode->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFlickRankingSubsystem>() : nullptr;
}

FLinearColor SFlickGameLayer::GetModeAccent(const EFlickMatchVariant Variant) const
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob: return FLinearColor(0.18f, 0.82f, 0.48f, 1.0f);
	case EFlickMatchVariant::Classic:
	default: return Cyan;
	}
}

FLinearColor SFlickGameLayer::GetCurrentModeAccent() const
{
	return GetModeAccent(GameMode.IsValid() ? GameMode->GetSelectedMatchVariant() : EFlickMatchVariant::Classic);
}

FLinearColor SFlickGameLayer::GetTeamAccent(const EFlickTeam Team) const
{
	return Team == EFlickTeam::Player2 ? Orange : Cyan;
}

FString SFlickGameLayer::GetClassDisplayName(const EFlickLineupPreset Preset) const
{
	return GameMode.IsValid() ? GameMode->GetClassName(Preset) : GetLineupPresetName(Preset);
}

EFlickTeam SFlickGameLayer::GetClassSelectionTeam() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const AFlickPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (State && State->bNetworkClassSelectionActive && LocalPlayerState)
	{
		return LocalPlayerState->GetTeam();
	}
	return EFlickTeam::Player1;
}

int32 SFlickGameLayer::GetClassSelectionPlayerSlot() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const AFlickPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (State && State->bNetworkClassSelectionActive && LocalPlayerState)
	{
		return FMath::Max(0, LocalPlayerState->GetTeamPlayerSlot());
	}
	return 0;
}

EFlickLineupPreset SFlickGameLayer::GetSelectedClassDraft() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const AFlickPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (State && State->bNetworkClassSelectionActive && LocalPlayerState)
	{
		return LocalPlayerState->GetNetworkSelectedClass();
	}
	return GameMode.IsValid()
		? GameMode->GetPlayerClass(GetClassSelectionTeam(), GetClassSelectionPlayerSlot(), true)
		: EFlickLineupPreset::Balanced;
}

EFlickPieceArchetype SFlickGameLayer::GetSelectedClassPiece(const int32 PieceSlot) const
{
	return GameMode.IsValid()
		? GameMode->GetClassLoadoutPiece(GetSelectedClassDraft(), PieceSlot)
		: FlickPieceArchetypeRules::GetPreset(GetSelectedClassDraft()).IsValidIndex(PieceSlot)
			? FlickPieceArchetypeRules::GetPreset(GetSelectedClassDraft())[PieceSlot]
			: EFlickPieceArchetype::Standard;
}

float SFlickGameLayer::GetSelectedClassStatValue(const int32 StatIndex) const
{
	float Total = 0.0f;
	constexpr int32 ClassPieceCount = 4;
	for (int32 PieceSlot = 0; PieceSlot < ClassPieceCount; ++PieceSlot)
	{
		Total += GetDisplayStatValue(
			FlickPieceArchetypeRules::GetDisplayStats(GetSelectedClassPiece(PieceSlot)),
			StatIndex);
	}
	return FMath::Clamp(Total / static_cast<float>(ClassPieceCount), 0.0f, 1.0f);
}
