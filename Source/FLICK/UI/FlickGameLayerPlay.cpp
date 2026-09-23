//playlists, formats, private matches, online
#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickMatchmakingRules.h"

const FFlickPrivateMatchSettings& SFlickGameLayer::GetDisplayedPrivateMatchSettings() const
{
	if (GameMode.IsValid())
	{
		return GameMode->GetPrivateMatchSettings();
	}
	if (const AFlickGameState* State = GetScoreboardGameState())
	{
		return State->PrivateMatchSettings;
	}
	static const FFlickPrivateMatchSettings Defaults;
	return Defaults;
}

TSharedRef<SWidget> SFlickGameLayer::BuildModeSelect()
{
	TSharedRef<SButton> BackButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this]()
		{
			if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training
				&& SelectedTrainingActivity != EFlickTrainingActivity::None)
			{
				SelectedTrainingActivity = EFlickTrainingActivity::None;
			}
			else if (SelectedPlayPlaylist != EFlickPlayPlaylist::None)
			{
				SelectedPlayPlaylist = EFlickPlayPlaylist::None;
				SelectedTrainingActivity = EFlickTrainingActivity::None;
			}
			else if (GameMode.IsValid())
			{
				GameMode->CloseModeSelect();
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakBackButton = BackButton;
	BackButton->SetContent(
		SNew(SFlickPlaylistCardPanel)
		.Selected_Lambda([WeakBackButton]()
		{
			const TSharedPtr<SButton> Button = WeakBackButton.Pin();
			return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14.0f, 0.0f, 14.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(3.0f).HeightOverride(24.0f)
				[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(UiFont(16, true)).ColorAndOpacity(Brand)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18.0f, 0.0f, 20.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("BACK"))).Font(UiFont(14, true)).ColorAndOpacity(Paper)
			]
		]);
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.003f, 0.008f, 0.68f))
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility(EVisibility::HitTestInvisible)
			.Opacity(0.78f)
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility(EVisibility::HitTestInvisible)
			.Opacity(0.2f)
		]
		+ SOverlay::Slot().VAlign(VAlign_Top).Padding(8.0f, 24.0f, 8.0f, 0.0f)
		[
			SNew(SBox).HeightOverride(UiMetrics::ModeHeaderHeight)
			[
				SNew(SFlickPlayHeaderPanel)
				.Padding(FMargin(34.0f, 13.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(240.0f).HeightOverride(80.0f)
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFit)
							.StretchDirection(EStretchDirection::DownOnly)
							[
								SNew(SFlickLogoWidget)
								.VectorPath(TEXT("UI/FlickKnockoutWordmark.svg"))
								.TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark"))
								.DesiredSize(FVector2D(2048.0f, 683.0f))
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.0f, 6.0f, 20.0f, 6.0f)
					[
						SNew(SBox).WidthOverride(1.0f).HeightOverride(62.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.48f, 0.54f, 0.56f, 0.82f))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::None) return FText::FromString(TEXT("FIND YOUR NEXT RIVALRY."));
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training && SelectedTrainingActivity == EFlickTrainingActivity::None) return FText::FromString(TEXT("TRAINING"));
								return FText::FromString(TEXT("SELECT MATCH FORMAT"));
							})
							.Font(DisplayFont(30))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::None) return FText::FromString(TEXT("CHOOSE HOW YOU WANT TO PLAY"));
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training && SelectedTrainingActivity == EFlickTrainingActivity::None) return FText::FromString(TEXT("CHOOSE A TRAINING ACTIVITY"));
								return FText::FromString(TEXT("SET THE ARENA RULESET AND TEAM SIZE"));
							})
							.Font(UiFont(10, true))
							.ColorAndOpacity(Brand)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("PLAY CONFIGURATION"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::None
									? TEXT("01  //  MODE")
									: TEXT("02  //  FORMAT"));
							})
							.Font(UiFont(13, true)).ColorAndOpacity(Brand)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::None ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f)
				+ SGridPanel::Slot(0, 0).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Casual, TEXT("CASUAL"), TEXT("LOCAL PLAY OR RELAXED ONLINE MATCHMAKING"))]
				+ SGridPanel::Slot(1, 0).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Competitive, TEXT("COMPETITIVE"), TEXT("RANKED ONLINE MATCHES WITH MMR"))]
				+ SGridPanel::Slot(0, 1).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Training, TEXT("TRAINING"), TEXT("LEARN THE BASICS, PRACTICE FREELY, OR PLAY AGAINST THE BOT"))]
				+ SGridPanel::Slot(1, 1).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::PrivateMatch, TEXT("PRIVATE MATCH"), TEXT("CUSTOM RULES, FLEXIBLE TEAMS, AND SPECTATORS"))]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]()
			{
				return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
					&& SelectedTrainingActivity == EFlickTrainingActivity::None
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(UiMetrics::CardGap)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(PanelRaised)
					.AccentColor(Orange.CopyWithNewOpacity(0.66f))
					.CutSize(8.0f)
					.BorderWidth(0.9f)
					.Padding(FMargin(18.0f, 2.0f))
					[
						MakeCycleRow(
							TEXT("BOT DIFFICULTY"),
							TAttribute<FText>::CreateLambda([this]()
							{
								return FText::FromString(GameMode.IsValid() ? GameMode->GetBotDifficultyLabel() : TEXT("NORMAL"));
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleBotDifficulty(-1);
								return FReply::Handled();
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleBotDifficulty(1);
								return FReply::Handled();
							}))
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(UiMetrics::CardGap))
					+ SUniformGridPanel::Slot(0, 0)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::Tutorial,
						TEXT("TUTORIAL"),
						TEXT("LEARN AIMING, POWER, KNOCKOUTS, AND SWITCHES"),
						TEXT("4 GUIDED LESSONS  |  FIXED SETUPS  |  INSTANT RETRY"),
						Cyan)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::FreePlay,
						TEXT("FREE PLAY"),
						TEXT("BUILD A CUSTOM BOARD AND REPEAT ANY SHOT"),
						TEXT("BOARD EDITOR  |  INSTANT RESET  |  ANY ARENA"),
						FLinearColor(0.2f, 0.78f, 0.5f, 1.0f))
				]
				+ SUniformGridPanel::Slot(0, 1)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::BotMatch,
						TEXT("1V1 VS BOT"),
						TEXT("PLAY A COMPLETE 1V1 KNOCKOUT SERIES"),
						TEXT("4 PUCKS EACH  |  FIRST TO 3 ROUNDS  |  OFFLINE"),
						Orange)
				]
				+ SUniformGridPanel::Slot(1, 1)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::BobBotMatch,
						TEXT("BOB VS BOT"),
						TEXT("POCKET YOUR COLOR BEFORE THE BOT CLEARS THEIRS"),
						TEXT("STANDARD PUCKS  |  BOB RULES  |  OFFLINE"),
						FLinearColor(0.18f, 0.82f, 0.48f, 1.0f))
				]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]()
			{
				return (SelectedPlayPlaylist == EFlickPlayPlaylist::Casual
					|| SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive
					|| (SelectedPlayPlaylist == EFlickPlayPlaylist::Training
						&& SelectedTrainingActivity == EFlickTrainingActivity::FreePlay))
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(UiMetrics::CardGap))
				+ SUniformGridPanel::Slot(0, 0)[BuildPlayFormatCard(1, false)]
				+ SUniformGridPanel::Slot(1, 0)[BuildPlayFormatCard(2, false)]
				+ SUniformGridPanel::Slot(0, 1)[BuildPlayFormatCard(3, false)]
				+ SUniformGridPanel::Slot(1, 1)[BuildPlayFormatCard(1, true)]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(20.0f, 0.0f, 20.0f, 18.0f)
		[
			SNew(SBox).HeightOverride(UiMetrics::ModeFooterHeight)
			[
				SNew(SFlickPlayFooterPanel)
				.Padding(FMargin(22.0f, 12.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(164.0f)[BackButton]]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(218.0f)
						.Visibility_Lambda([this]()
						{
							return SelectedPlayPlaylist != EFlickPlayPlaylist::None
								&& SelectedPlayPlaylist != EFlickPlayPlaylist::Training
								&& GameMode.IsValid()
								&& GameMode->DoesSelectedModeSupportLoadouts()
								? EVisibility::Visible
								: EVisibility::Collapsed;
						})
						[
							MakeMenuButton(TEXT("EDIT LINEUPS"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->OpenLoadout(); return FReply::Handled(); }), false, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(224.0f)
						.IsEnabled_Lambda([this]() { return SelectedPlayFormat != 0 && (!GameMode.IsValid() || !GameMode->IsPartySession()); })
						.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Casual ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("LOCAL MATCH"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->StartSelectedMatch(); return FReply::Handled(); }), false, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(242.0f)
						.IsEnabled_Lambda([this]()
						{
							return SelectedPlayFormat != 0 && GameMode.IsValid()
								&& (!GameMode->IsPartySession()
									|| GameMode->CanQueuePartyForMatchmaking(GameMode->GetMatchmakingPlayersPerTeam()));
						})
						.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Casual ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("FIND CASUAL"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid())
								{
									GameMode->SetRankedQueueSelected(false);
									GameMode->StartSelectedMatchmaking();
								}
								return FReply::Handled();
							}), true, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(254.0f)
						.IsEnabled_Lambda([this]() { return SelectedPlayFormat != 0; })
						.Visibility_Lambda([this]()
						{
							return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
								&& SelectedTrainingActivity == EFlickTrainingActivity::FreePlay
								? EVisibility::Visible
								: EVisibility::Collapsed;
						})
						[
							MakeMenuButton(TEXT("START TRAINING"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid())
								{
									GameMode->StartTrainingMode();
								}
								return FReply::Handled();
							}), true, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(276.0f)
						.IsEnabled_Lambda([this]()
						{
							return SelectedPlayFormat != 0 && GameMode.IsValid()
								&& (!GameMode->IsPartySession()
									|| GameMode->CanQueuePartyForMatchmaking(GameMode->GetMatchmakingPlayersPerTeam()));
						})
						.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("FIND COMPETITIVE"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid())
								{
									GameMode->SetRankedQueueSelected(true);
									GameMode->StartSelectedMatchmaking();
								}
								return FReply::Handled();
							}), true, false, UiMetrics::ActionHeight)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPrivateMatchSetup()
{
	auto CycleSetting = [this](const FString& Label, const EFlickPrivateMatchSetting Setting, const TAttribute<FText>& Value)
	{
		return MakeCycleRow(
			Label,
			Value,
			FOnClicked::CreateLambda([this, Setting]()
			{
				if (GameMode.IsValid()) GameMode->CyclePrivateMatchSetting(Setting, -1);
				return FReply::Handled();
			}),
			FOnClicked::CreateLambda([this, Setting]()
			{
				if (GameMode.IsValid()) GameMode->CyclePrivateMatchSetting(Setting, 1);
				return FReply::Handled();
			}));
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.003f, 0.008f, 0.82f))]
		+ SOverlay::Slot().Padding(42.0f, 30.0f, 42.0f, 106.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("PRIVATE MATCH"))).Font(DisplayFont(38)).ColorAndOpacity(FLinearColor::White)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("SET THE RULES  /  LAUNCH THE ARENA  /  CHOOSE TEAMS IN GAME"))).Font(UiFont(10, true)).ColorAndOpacity(Cyan)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(FString::Printf(
							TEXT("PARTY  %d / %d"),
							FMath::Max(GetDisplayedPartyMemberCount(), 1),
							FlickMaximumPartyMembers));
					})
					.Font(UiFont(11, true)).ColorAndOpacity(Muted)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 22.0f, 0.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(780.0f)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.017f, 0.96f)).AccentColor(Hairline).CutSize(12.0f).BorderWidth(1.0f).Padding(FMargin(22.0f, 16.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 8.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("MATCH RULES  //  HOST SETTINGS"))).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("MODE"), EFlickPrivateMatchSetting::Mode, TAttribute<FText>::CreateLambda([this]() { const FFlickPrivateMatchSettings& Settings = GetDisplayedPrivateMatchSettings(); return FText::FromString(Settings.Variant == EFlickMatchVariant::Bob ? TEXT("BOB") : FString::Printf(TEXT("%dV%d"), Settings.PlayersPerTeam, Settings.PlayersPerTeam)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("ROUNDS TO WIN"), EFlickPrivateMatchSetting::RoundsToWin, TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber(GetDisplayedPrivateMatchSettings().RoundsToWin); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("ARENA SIZE"), EFlickPrivateMatchSetting::ArenaScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().ArenaScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("FRICTION"), EFlickPrivateMatchSetting::FrictionScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().FrictionScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("LAUNCH POWER"), EFlickPrivateMatchSetting::LaunchSpeedScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().LaunchSpeedScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("BOUNCE"), EFlickPrivateMatchSetting::RestitutionScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().RestitutionScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("KICKOFF"), EFlickPrivateMatchSetting::SimultaneousKickoff, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GetDisplayedPrivateMatchSettings().bSimultaneousKickoff ? TEXT("SIMULTANEOUS") : TEXT("ALTERNATING")); }))]
						+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 14.0f, 4.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Starting brings the party into the arena. Choose Blue, Orange, or Spectate there; empty seats are bots.")))
							.Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(88.0f)
			[
				SNew(SFlickAngularBorder).BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.96f)).AccentColor(Hairline).CutSize(8.0f).BorderWidth(1.0f).Padding(FMargin(34.0f, 16.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(180.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ClosePrivateMatchSetup(); return FReply::Handled(); }), false, false, 54.0f)]]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(250.0f).IsEnabled_Lambda([this]() { return GameMode.IsValid() && GameMode->CanStartPrivateMatch(); })
						[
							MakeMenuButton(TEXT("START MATCH"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestStartNetworkMatch(); return FReply::Handled(); }), true, false, 54.0f)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPlayPlaylistCard(
	const EFlickPlayPlaylist Playlist,
	const FString& Label,
	const FString& Summary)
{
	const FString PlaylistCode = Playlist == EFlickPlayPlaylist::Casual ? TEXT("01")
		: Playlist == EFlickPlayPlaylist::Competitive ? TEXT("02")
		: Playlist == EFlickPlayPlaylist::Training ? TEXT("03")
		: Playlist == EFlickPlayPlaylist::PrivateMatch ? TEXT("04") : TEXT("05");
	const FString PlaylistTag = Playlist == EFlickPlayPlaylist::Casual ? TEXT("LOCAL + ONLINE")
		: Playlist == EFlickPlayPlaylist::Competitive ? TEXT("RANKED ONLINE")
		: Playlist == EFlickPlayPlaylist::Training ? TEXT("OFFLINE PRACTICE")
		: Playlist == EFlickPlayPlaylist::PrivateMatch ? TEXT("YOUR RULES") : TEXT("EXPERIMENTAL");
	const FString PlaylistSymbolPath = Playlist == EFlickPlayPlaylist::Casual
		? TEXT("/Game/UI/CasualPlaylistSymbol.CasualPlaylistSymbol")
		: Playlist == EFlickPlayPlaylist::Competitive
			? TEXT("/Game/UI/CompetitivePlaylistSymbol.CompetitivePlaylistSymbol")
			: Playlist == EFlickPlayPlaylist::Training
				? TEXT("/Game/UI/TrainingPlaylistSymbol.TrainingPlaylistSymbol")
				: TEXT("/Game/UI/PrivateMatchPlaylistSymbol.PrivateMatchPlaylistSymbol");
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.IsEnabled_Lambda([this]() { return !IsDisplayedPartyActive() || IsLocalDisplayedPartyLeader(); })
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this, Playlist]()
		{
			if (Playlist == EFlickPlayPlaylist::PrivateMatch)
			{
				if (IsDisplayedPartyActive())
				{
					if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem())
					{
						Sessions->BeginPrivateMatchForParty();
					}
				}
				else if (GameMode.IsValid())
				{
					GameMode->OpenPrivateMatchSetup();
				}
				return FReply::Handled();
			}
			SelectedPlayPlaylist = Playlist;
			SelectedPlayFormat = 0;
			SelectedTrainingActivity = EFlickTrainingActivity::None;
			if (GameMode.IsValid())
			{
				GameMode->SetRankedQueueSelected(Playlist == EFlickPlayPlaylist::Competitive);
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakCardButton = CardButton;
	const auto IsActive = [WeakCardButton]()
	{
		const TSharedPtr<SButton> Button = WeakCardButton.Pin();
		return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
	};
	CardButton->SetContent(
		SNew(SFlickPlaylistCardPanel)
		.Selected_Lambda(IsActive)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(FMargin(24.0f, 17.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(PlaylistCode + TEXT("   /   ") + PlaylistTag))
						.Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(34)).ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Summary)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(14.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 18.0f, 2.0f)
					[
						SNew(SBox).WidthOverride(1.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush())
							.BorderBackgroundColor(FLinearColor(0.42f, 0.48f, 0.50f, 0.52f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox).WidthOverride(92.0f).HeightOverride(92.0f)
							[
								SNew(SScaleBox)
								.Stretch(EStretch::ScaleToFit)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SNew(SFlickLogoWidget)
									.TexturePath(PlaylistSymbolPath)
									.Opacity(1.0f)
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 5.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("PLAY  >")))
							.Font(UiFont(12, true)).ColorAndOpacity(Paper)
						]
					]
				]
			]
		]);
	return SNew(SBox).HeightOverride(UiMetrics::PlaylistCardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTrainingActivityCard(
	const EFlickTrainingActivity Activity,
	const FString& Label,
	const FString& Summary,
	const FString& Detail,
	const FLinearColor& Accent)
{
	const FString ActivitySymbolPath = Activity == EFlickTrainingActivity::Tutorial
		? TEXT("/Game/UI/TutorialPlaylist.TutorialPlaylist")
		: Activity == EFlickTrainingActivity::FreePlay
			? TEXT("/Game/UI/FreePlayPlaylist.FreePlayPlaylist")
			: Activity == EFlickTrainingActivity::BotMatch
				? TEXT("/Game/UI/1v1vsBotPlaylist.1v1vsBotPlaylist")
				: TEXT("/Game/UI/BOBvsBotPlaylist.BOBvsBotPlaylist");
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this, Activity]()
		{
			SelectedTrainingActivity = Activity;
			if (GameMode.IsValid())
			{
				GameMode->SetMatchmakingPlayersPerTeam(1);
				if (Activity == EFlickTrainingActivity::Tutorial)
				{
					GameMode->SelectMatchVariant(EFlickMatchVariant::Classic);
					GameMode->StartTutorialMode();
					return FReply::Handled();
				}
				const bool bBotMatch = Activity == EFlickTrainingActivity::BotMatch
					|| Activity == EFlickTrainingActivity::BobBotMatch;
				GameMode->SelectMatchVariant(
					Activity == EFlickTrainingActivity::BobBotMatch
						? EFlickMatchVariant::Bob
						: EFlickMatchVariant::Classic);
				if (bBotMatch)
				{
					GameMode->StartTrainingBotMatch();
					return FReply::Handled();
				}
			}

			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakCardButton = CardButton;
	const auto IsActive = [WeakCardButton]()
	{
		const TSharedPtr<SButton> Button = WeakCardButton.Pin();
		return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
	};
	CardButton->SetContent(
		SNew(SFlickPlaylistCardPanel)
		.Selected_Lambda(IsActive)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(FMargin(24.0f, 14.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(TEXT("PRACTICE  /  OFFLINE"))).Font(UiFont(10, true)).ColorAndOpacity(Accent)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(30)).ColorAndOpacity(Paper).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Summary)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 14.0f, 7.0f)
					[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.45f))]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Detail)).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(14.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 18.0f, 2.0f)
					[
						SNew(SBox).WidthOverride(1.0f)
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.62f))]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox).WidthOverride(92.0f).HeightOverride(92.0f)
							[
								SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)
								[SNew(SFlickLogoWidget).TexturePath(ActivitySymbolPath)]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 7.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Activity == EFlickTrainingActivity::FreePlay ? TEXT("CHOOSE  >") : TEXT("PLAY  >")))
							.Font(UiFont(11, true)).ColorAndOpacity(Paper)
						]
					]
				]
			]
		]);
	return SNew(SBox).HeightOverride(UiMetrics::TrainingCardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPlayFormatCard(
	const int32 PlayersPerTeam,
	const bool bBob)
{
	const bool bAvailable = bBob || (PlayersPerTeam >= 1 && PlayersPerTeam <= 3);
	const FString Title = bBob ? TEXT("BOB") : FString::Printf(TEXT("%dV%d KNOCKOUT"), PlayersPerTeam, PlayersPerTeam);
	const FString FormatSymbolPath = bBob
		? TEXT("/Game/UI/BobPlaylistSymbol.BobPlaylistSymbol")
		: PlayersPerTeam == 1
			? TEXT("/Game/UI/1v1PlaylistSymbol.1v1PlaylistSymbol")
			: PlayersPerTeam == 2
				? TEXT("/Game/UI/2v2PlaylistSymbol.2v2PlaylistSymbol")
				: TEXT("/Game/UI/3v3PlaylistSymbol.3v3PlaylistSymbol");
	const auto IsSelected = [this, PlayersPerTeam, bBob]()
	{
		return SelectedPlayFormat == (bBob ? 4 : PlayersPerTeam);
	};
	const auto FitsDisplayedParty = [this, PlayersPerTeam]()
	{
		const bool bPublicPlaylist = SelectedPlayPlaylist == EFlickPlayPlaylist::Casual
			|| SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive;
		return !bPublicPlaylist
			|| !IsDisplayedPartyActive()
			|| FlickMatchmakingRules::IsPartySizeValid(GetDisplayedPartyMemberCount(), PlayersPerTeam);
	};
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.IsEnabled_Lambda([this, bAvailable, FitsDisplayedParty]()
		{
			const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
			return bAvailable && FitsDisplayedParty() && (!Sessions || !Sessions->IsMatchmakingActive());
		})
		.Cursor(bAvailable ? EMouseCursor::Hand : EMouseCursor::Default)
		.OnClicked_Lambda([this, PlayersPerTeam, bBob, FitsDisplayedParty]()
		{
			if (!FitsDisplayedParty())
			{
				return FReply::Handled();
			}
			SelectedPlayFormat = bBob ? 4 : PlayersPerTeam;
			if (GameMode.IsValid())
			{
				GameMode->SetMatchmakingPlayersPerTeam(PlayersPerTeam);
				GameMode->SelectMatchVariant(bBob ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic);
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakCardButton = CardButton;
	const auto IsActive = [WeakCardButton, bAvailable, FitsDisplayedParty]()
	{
		const TSharedPtr<SButton> Button = WeakCardButton.Pin();
		return bAvailable && FitsDisplayedParty() && Button.IsValid()
			&& (Button->IsHovered() || Button->HasKeyboardFocus());
	};
	CardButton->SetContent(
		SNew(SFlickPlaylistCardPanel)
		.Selected_Lambda([IsActive, IsSelected]() { return IsSelected() || IsActive(); })
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(FMargin(24.0f, 13.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Fill)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(bBob ? TEXT("POCKET GAME  /  1V1") : TEXT("4 PUCKS PER PLAYER"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, PlayersPerTeam, bBob, Title]()
						{
							if (SelectedPlayPlaylist != EFlickPlayPlaylist::Training) return FText::FromString(Title);
							if (SelectedTrainingActivity == EFlickTrainingActivity::BotMatch) return FText::FromString(TEXT("1V1 VS BOT"));
							return FText::FromString(bBob ? TEXT("BOB PRACTICE") : FString::Printf(TEXT("%dV%d ARENA"), PlayersPerTeam, PlayersPerTeam));
						})
						.Font(DisplayFont(32)).ColorAndOpacity(bAvailable ? Paper : Muted)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 14.0f, 0.0f)
					[
					SNew(STextBlock)
					.Text_Lambda([this, bBob]()
					{
						return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::Training
							? SelectedTrainingActivity == EFlickTrainingActivity::BotMatch
								? TEXT("You play blue. The bot plays orange.")
								: TEXT("Set up the board. Find your shot. Repeat.")
							: bBob ? TEXT("Pocket your color before your opponent clears theirs.") : TEXT("Clear the opposing team. First to three rounds wins."));
					})
					.Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
				]
					+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, bBob, bAvailable, FitsDisplayedParty]()
						{
							return FText::FromString(!bAvailable ? TEXT("UNAVAILABLE")
								: !FitsDisplayedParty() ? TEXT("PARTY TOO LARGE FOR THIS PLAYLIST")
								: SelectedPlayPlaylist == EFlickPlayPlaylist::Training ? TEXT("OFFLINE PRACTICE")
								: bBob ? TEXT("ONE BOARD  /  STANDARD PUCKS") : TEXT("FIRST TO 3  /  TEAM KNOCKOUT"));
						})
						.Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(14.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 18.0f, 2.0f)
					[
						SNew(SBox).WidthOverride(1.0f)
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.62f))]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox).WidthOverride(92.0f).HeightOverride(92.0f)
							[
								SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)
								[SNew(SFlickLogoWidget).TexturePath(FormatSymbolPath).Opacity(bAvailable ? 1.0f : 0.3f)]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 7.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([IsSelected]() { return FText::FromString(IsSelected() ? TEXT("SELECTED  >") : TEXT("SELECT  >")); })
							.Font(UiFont(10, true)).ColorAndOpacity_Lambda([IsSelected]() { return IsSelected() ? Brand : Paper; })
						]
					]
				]
			]
		]);
	return SNew(SBox).HeightOverride(UiMetrics::FormatCardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildOnlineBrowser()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.001f, 0.004f, 0.009f, 0.92f))
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(116.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.008f, 0.016f, 0.95f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.76f))
				.CutSize(10.0f)
				.BorderWidth(1.1f)
				.Padding(FMargin(44.0f, 22.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(6.0f).HeightOverride(55.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("ONLINE LOBBIES"))).Font(UiFont(36, true)).ColorAndOpacity(FLinearColor::White)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("STEAM SESSION DISCOVERY"))).Font(UiFont(10, true)).ColorAndOpacity(Cyan)]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString(TEXT("SIGNED IN AS"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
								return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("OFFLINE"));
							})
							.Font(UiFont(17, true)).ColorAndOpacity(Cyan)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetSelectedMatchVariant() : EFlickMatchVariant::Classic;
								const int32 TeamSize = GameMode.IsValid() ? GameMode->GetMatchmakingPlayersPerTeam() : 1;
								return FText::FromString(Ranking
									? Ranking->GetProgressLabel(Variant, TeamSize)
									: TEXT("RANK UNAVAILABLE"));
							})
							.Font(UiFont(9, true)).ColorAndOpacity(Muted)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().Padding(44.0f, 142.0f, 44.0f, 105.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.006f, 0.018f, 0.03f, 0.97f))
				.AccentColor(Hairline)
				.CutSize(12.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(18.0f, 12.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(Sessions ? Sessions->GetStatusMessage() : TEXT("SESSION SERVICE UNAVAILABLE"));
						})
						.Font(UiFont(11, true))
						.ColorAndOpacity_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FSlateColor(Sessions && Sessions->GetState() == EFlickSessionState::Error ? Orange : Cyan);
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(FString::Printf(TEXT("SERVICE  /  %s"), Sessions ? *Sessions->GetOnlineServiceName() : TEXT("OFFLINE")));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(Muted)
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(0)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(1)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(2)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(3)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(4)]
				+ SVerticalBox::Slot().FillHeight(1.0f)[BuildOnlineSessionRow(5)]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(82.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.95f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.58f))
				.CutSize(8.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(34.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(170.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CloseOnlineBrowser(); return FReply::Handled(); }))]]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)[SNew(SBox).WidthOverride(230.0f)[MakeMenuButton(TEXT("HOST LOCAL"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->HostLocalNetworkMatch(); return FReply::Handled(); }))]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)[SNew(SBox).WidthOverride(230.0f)[MakeMenuButton(TEXT("JOIN LOCAL"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->JoinLocalNetworkMatch(); return FReply::Handled(); }))]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(190.0f)
						[
							MakeMenuButton(TEXT("REFRESH"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->FindSessions();
								return FReply::Handled();
							}), false, false, 50.0f)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(170.0f)
						[
							SNew(SButton)
							.ButtonStyle(&MenuButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.IsEnabled_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
								return !Sessions || !Sessions->IsMatchmakingActive();
							})
							.OnClicked_Lambda([this]()
							{
								if (GameMode.IsValid()) GameMode->ToggleRankedQueue();
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return FText::FromString(GameMode.IsValid() && GameMode->IsRankedQueueSelected()
										? TEXT("RANKED") : TEXT("CASUAL"));
								})
								.Font(UiFont(13, true))
								.ColorAndOpacity_Lambda([this]()
								{
									return FSlateColor(GameMode.IsValid() && GameMode->IsRankedQueueSelected() ? Orange : Cyan);
								})
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(160.0f)
						[
							SNew(SButton)
							.ButtonStyle(&MenuButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleMatchmakingPlayersPerTeam(1);
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const int32 TeamSize = GameMode.IsValid() ? GameMode->GetMatchmakingPlayersPerTeam() : 1;
									return FText::FromString(FString::Printf(TEXT("%dV%d"), TeamSize, TeamSize));
								})
								.Font(UiFont(14, true)).ColorAndOpacity(Cyan)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(260.0f)
						[
							SNew(SButton)
							.ButtonStyle(&PrimaryButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([this]()
							{
								if (!GameMode.IsValid()) return FReply::Handled();
								const UFlickSessionSubsystem* Sessions = GameMode->GetFlickSessionSubsystem();
								if (Sessions && Sessions->IsMatchmakingActive()) GameMode->CancelUnrankedMatchmaking();
								else GameMode->StartSelectedMatchmaking();
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
									if (Sessions && Sessions->IsMatchmakingActive()) return FText::FromString(TEXT("CANCEL QUEUE"));
									return FText::FromString(GameMode.IsValid() && GameMode->IsRankedQueueSelected()
										? TEXT("FIND RANKED") : TEXT("FIND CASUAL"));
								})
								.Font(UiFont(15, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildOnlineSessionRow(const int32 ResultIndex)
{
	return SNew(SBox)
		.Visibility_Lambda([this, ResultIndex]()
		{
			const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
			return Sessions && Sessions->GetBrowserEntries().IsValidIndex(ResultIndex) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.008f, 0.018f, 0.03f, 0.98f))
			.AccentColor(Hairline)
			.CutSize(12.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(19.0f, 7.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.34f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, ResultIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						return FText::FromString(Sessions && Sessions->GetBrowserEntries().IsValidIndex(ResultIndex) ? Sessions->GetBrowserEntries()[ResultIndex].OwnerName : FString());
					}).Font(UiFont(14, true)).ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, ResultIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						if (!Sessions || !Sessions->GetBrowserEntries().IsValidIndex(ResultIndex)) return FText::GetEmpty();
						const FFlickSessionBrowserEntry& Entry = Sessions->GetBrowserEntries()[ResultIndex];
						return FText::FromString(Entry.bMatchmaking
							? FString::Printf(
								TEXT("%s  /  %dV%d %s%s"),
								*GetMatchVariantName(Entry.Variant),
								Entry.PlayersPerTeam,
								Entry.PlayersPerTeam,
								Entry.bRanked ? TEXT("RANKED") : TEXT("CASUAL"),
								Entry.bRanked ? *FString::Printf(TEXT("  /  %d MMR"), Entry.QueueRating) : TEXT(""))
							: GetMatchVariantName(Entry.Variant));
					}).Font(UiFont(11, true)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().FillWidth(0.18f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, ResultIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						if (!Sessions || !Sessions->GetBrowserEntries().IsValidIndex(ResultIndex)) return FText::GetEmpty();
						const FFlickSessionBrowserEntry& Entry = Sessions->GetBrowserEntries()[ResultIndex];
						return FText::FromString(FString::Printf(TEXT("%d / %d   %d MS"), Entry.CurrentPlayers, Entry.MaximumPlayers, Entry.PingMilliseconds));
					}).Font(UiFont(10, true)).ColorAndOpacity(Muted)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(150.0f)
					[
						MakeMenuButton(TEXT("JOIN"), FOnClicked::CreateLambda([this, ResultIndex]()
						{
							if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->JoinSession(ResultIndex);
							return FReply::Handled();
						}), true, false, 42.0f)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildNetworkLobby()
{
	auto BuildPlayerSlot = [this](const EFlickTeam Team, const int32 PlayerSlot, const FLinearColor& Accent) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
		.Visibility_Lambda([this, PlayerSlot]()
		{
			return GameMode.IsValid() && PlayerSlot < GameMode->GetPlayersPerTeam()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
		SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.006f, 0.015f, 0.026f, 0.96f))
			.AccentColor(Accent)
			.CutSize(15.0f)
			.BorderWidth(1.5f)
			.Padding(FMargin(20.0f, 14.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(
							TEXT("TEAM %d  /  PLAYER %d%s"),
							GetTeamNumber(Team),
							PlayerSlot + 1,
							Team == EFlickTeam::Player1 && PlayerSlot == 0 ? TEXT("  /  HOST") : TEXT(""))))
						.Font(UiFont(9, true)).ColorAndOpacity(Accent)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team, PlayerSlot]()
						{
							const AFlickPlayerState* Player = GameMode.IsValid() ? GameMode->GetLobbyPlayer(Team, PlayerSlot) : nullptr;
							return FText::FromString(Player ? Player->GetPlayerName() : TEXT("WAITING FOR PLAYER"));
						})
						.Font(UiFont(18, true)).ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team, PlayerSlot]()
					{
						const AFlickPlayerState* Player = GameMode.IsValid() ? GameMode->GetLobbyPlayer(Team, PlayerSlot) : nullptr;
						if (!Player) return FText::FromString(TEXT("OPEN SLOT"));
						if (GameMode.IsValid() && GameMode->IsRankedMatch() && !Player->IsRankedIdentityVerified())
						{
							return FText::FromString(TEXT("VERIFYING IDENTITY"));
						}
						return FText::FromString(Player->IsLobbyReady() ? TEXT("READY") : TEXT("NOT READY"));
					})
					.Font(UiFont(11, true))
					.ColorAndOpacity_Lambda([this, Team, PlayerSlot, Accent]()
					{
						const AFlickPlayerState* Player = GameMode.IsValid() ? GameMode->GetLobbyPlayer(Team, PlayerSlot) : nullptr;
						return FSlateColor(Player && Player->IsLobbyReady() ? Accent : Muted);
					})
				]
			]
		];
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.004f, 0.01f, 0.78f))
		]
		+ SOverlay::Slot().Padding(34.0f, 24.0f, 34.0f, 96.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(6.0f).HeightOverride(58.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(GameMode.IsValid() && GameMode->IsMatchmakingSession()
								? GameMode->IsRankedMatch() ? TEXT("RANKED MATCHMAKING") : TEXT("CASUAL MATCHMAKING")
								: Sessions && Sessions->HasActiveSession() ? TEXT("STEAM LOBBY") : TEXT("LOCAL LOBBY"));
						})
						.Font(UiFont(34, true)).ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(GameMode.IsValid() && GameMode->IsMatchmakingSession()
								? GameMode->IsRankedMatch()
									? TEXT("MMR MATCHED  /  CONFIRM READY  /  RATING ACTIVE")
									: TEXT("FIND PLAYERS  /  CONFIRM READY  /  PLAY")
								: TEXT("CHOOSE A COMPETITION  /  READY UP  /  HOST STARTS"));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(Cyan)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!GameMode.IsValid()) return FText::FromString(TEXT("WAITING FOR PLAYERS"));
						const AFlickGameState* State = GameMode->GetFlickGameState();
						if (State && State->bMatchmakingTimedOut) return FText::FromString(TEXT("QUEUE TIMED OUT"));
						if (GameMode->IsRankedMatch()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player1) == GameMode->GetPlayersPerTeam()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player2) == GameMode->GetPlayersPerTeam()
							&& !GameMode->AreRankedPlayersAuthenticated())
						{
							return FText::FromString(TEXT("VERIFYING STEAM IDENTITIES"));
						}
						if (GameMode->CanStartNetworkMatch()) return FText::FromString(TEXT("ALL PLAYERS READY"));
						if (GameMode->IsMatchmakingSession()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player1) == GameMode->GetPlayersPerTeam()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player2) == GameMode->GetPlayersPerTeam())
						{
							return FText::FromString(TEXT("MATCH FOUND - CONFIRM READY"));
						}
						return FText::FromString(TEXT("SEARCHING FOR PLAYERS"));
					})
					.Font(UiFont(12, true))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(GameMode.IsValid() && GameMode->CanStartNetworkMatch() ? Cyan : Muted); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player1, 0, Cyan)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player1, 1, Cyan)]
					+ SVerticalBox::Slot().AutoHeight()[BuildPlayerSlot(EFlickTeam::Player1, 2, Cyan)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player2, 0, Orange)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player2, 1, Orange)]
					+ SVerticalBox::Slot().AutoHeight()[BuildPlayerSlot(EFlickTeam::Player2, 2, Orange)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() ? GetMatchVariantName(GameMode->GetSelectedMatchVariant()) : TEXT("KNOCKOUT")); })
				.Font(UiFont(17, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetCurrentModeAccent()); })
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
			SNew(SUniformGridPanel)
				.SlotPadding(FMargin(8.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)[BuildModeCard(EFlickMatchVariant::Classic)]
				+ SUniformGridPanel::Slot(1, 0)[BuildModeCard(EFlickMatchVariant::Bob)]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(82.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.95f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.58f))
				.CutSize(8.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(34.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(180.0f)[MakeMenuButton(GameMode.IsValid() && GameMode->IsMatchmakingSession() ? TEXT("CANCEL QUEUE") : TEXT("CANCEL LOBBY"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CancelNetworkLobby(); return FReply::Handled(); }), false, true)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(230.0f)
						.Visibility_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return Sessions && Sessions->HasActiveSession()
								&& (!GameMode.IsValid() || !GameMode->IsMatchmakingSession())
								? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							MakeMenuButton(TEXT("INVITE FRIENDS"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->OpenInviteOverlay();
								return FReply::Handled();
							}))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(210.0f)
						[
							SNew(SButton)
							.ButtonStyle(&MenuButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([this]() { if (PlayerController.IsValid()) PlayerController->ToggleLobbyReady(); return FReply::Handled(); })
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const AFlickPlayerState* Player = PlayerController.IsValid() ? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr;
									return FText::FromString(Player && Player->IsLobbyReady() ? TEXT("SET NOT READY") : TEXT("READY UP"));
								})
								.Font(UiFont(14, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(240.0f)
						.IsEnabled_Lambda([this]() { return GameMode.IsValid() && GameMode->CanStartNetworkMatch(); })
						[
							MakeMenuButton(TEXT("START"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestStartNetworkMatch(); return FReply::Handled(); }), true)
						]
					]
				]
			]
		];
}
