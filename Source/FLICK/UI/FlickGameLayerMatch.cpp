// HUD, scoreboard, tutorial, pause/results and state
#include "UI/FlickGameLayerPrivate.h"

namespace FlickScoreboardLayout
{
	constexpr float Score = 94.0f;
	constexpr float Knockouts = 112.0f;
	constexpr float Shots = 82.0f;
	constexpr float FinalStat = 116.0f;
	constexpr float Ping = 92.0f;
}

TSharedRef<SWidget> SFlickGameLayer::BuildMatchHud()
{
	return SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(18.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GameMode.IsValid() && (GameMode->IsFreePlayTraining() || GameMode->IsTutorialMode()) ? EVisibility::Collapsed : EVisibility::Visible; })
			[BuildTeamPlate(EFlickTeam::Player1)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 14.0f, 18.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GameMode.IsValid() && (GameMode->IsFreePlayTraining() || GameMode->IsTutorialMode()) ? EVisibility::Collapsed : EVisibility::Visible; })
			[BuildTeamPlate(EFlickTeam::Player2)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.0f, 16.0f, 0.0f, 0.0f)[BuildTrainingToolsPanel()]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.0f, 132.0f, 0.0f, 0.0f)[BuildTutorialOverlay()]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(356.0f)
			.HeightOverride(62.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.015f, 0.84f))
				.AccentColor(Brand.CopyWithNewOpacity(0.6f))
				.CutSize(7.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(10.0f, 3.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 2.0f)
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFit)
							.StretchDirection(EStretchDirection::DownOnly)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return GetMatchStatusText(); })
								.Font(DisplayFont(16))
								.ColorAndOpacity_Lambda([this]() { return GetMatchStatusColor(); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Visibility_Lambda([this]() { return GetNextTurnVisibility(); })
							.Text_Lambda([this]() { return GetNextTurnText(); })
							.Font(UiFont(8, true))
							.ColorAndOpacity_Lambda([this]() { return GetNextTurnColor(); })
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const AFlickGameState* State = GetScoreboardGameState();
								if (!State) return FText::GetEmpty();
								if (GameMode.IsValid() && GameMode->IsTrainingMode())
								{
									if (GameMode->IsTutorialMode())
									{
										return FText::FromString(GameMode->IsTutorialComplete()
											? TEXT("TUTORIAL COMPLETE")
											: FString::Printf(TEXT("LESSON %d / %d"), GameMode->GetTutorialStageNumber(), GameMode->GetTutorialStageCount()));
									}
									if (GameMode->IsTrainingEditMode())
									{
										return FText::FromString(TEXT("BOARD EDITOR  /  T SAVE"));
									}
									if (!GameMode->IsTrainingBotMatch())
									{
										return FText::FromString(FString::Printf(TEXT("FREE PLAY  /  SHOT %d"), State->TurnNumber));
									}
								}
								const FString Context = State->ActiveMatchVariant == EFlickMatchVariant::Bob
									? FString::Printf(TEXT("BOB  /  SHOT %d"), State->TurnNumber)
									: FString::Printf(TEXT("ROUND %d  /  SHOT %d"), State->RoundNumber, State->TurnNumber);
								return FText::FromString(State->bShotClockActive
									? FString::Printf(TEXT("%s  /  %02d SEC"), *Context, FMath::CeilToInt(State->GetShotClockTimeRemaining()))
									: Context);
							})
							.Font(UiFont(8, true))
							.ColorAndOpacity(Muted)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24.0f, 0.0f, 0.0f, 36.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				BuildPowerMeter()
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildControlHintPanel(false)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 24.0f, 36.0f)[BuildControlHintPanel(true)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 36.0f)[BuildCameraOrbitHint()]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 86.0f, 18.0f, 0.0f)[BuildEventFeed()];
}

TSharedRef<SWidget> SFlickGameLayer::BuildCinematicReplayOverlay()
{
	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(78.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 0.97f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(78.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 0.97f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30.0f, 23.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(42.0f).HeightOverride(3.0f)
				[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Orange)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("CINEMATIC REPLAY"))).Font(UiFont(12, true)).ColorAndOpacity(Paper)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 24.0f, 30.0f, 0.0f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("ROUND-WINNING SHOT"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)
		]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(30.0f, 0.0f, 30.0f, 31.0f)
		[
			SNew(SBox).HeightOverride(3.0f)
			[
				SNew(SProgressBar)
				.Style(&ShotClockBarStyle)
				.Percent_Lambda([this]()
				{
					if (PlayerController.IsValid())
					{
						return TOptional<float>(PlayerController->GetCinematicReplayPresentationProgress());
					}
					return TOptional<float>(GameMode.IsValid() ? GameMode->GetCinematicReplayProgress() : 0.0f);
				})
				.FillColorAndOpacity(Orange)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 30.0f, 46.0f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("SLOW MOTION  //  PLAYBACK"))).Font(UiFont(9, true)).ColorAndOpacity(Orange)
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildScoreboardOverlay()
{
	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.0f, 0.003f, 0.008f, 0.42f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24.0f)
		[
			SNew(SBox)
			.WidthOverride_Lambda([this]() { return FMath::Min(1060.0f, FMath::Max(1.0f, LayerLocalSize.X - 48.0f)); })
			[
				SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
				[
				SNew(SBox).WidthOverride(1060.0f)
				[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.003f, 0.012f, 0.019f, 0.86f))
				.AccentColor(Hairline.CopyWithNewOpacity(0.72f))
				.CutSize(11.0f)
				.BorderWidth(0.9f)
				.Padding(FMargin(19.0f, 15.0f, 19.0f, 13.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 11.0f, 0.0f)
						[
							SNew(SBox).WidthOverride(3.0f).HeightOverride(31.0f)
							[
								SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("SCOREBOARD")))
								.Font(UiFont(21, true))
								.ColorAndOpacity(FLinearColor::White)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, -1.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return GetScoreboardMatchSummary(); })
								.Font(UiFont(9, true))
								.ColorAndOpacity(Brand)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("HOLD TAB")))
							.Font(UiFont(9, true))
							.ColorAndOpacity(Muted)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 13.0f, 0.0f, 0.0f)[BuildScoreboardTeamSection(EFlickTeam::Player1)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)[BuildScoreboardTeamSection(EFlickTeam::Player2)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 11.0f, 2.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("FLICK  //  LIVE MATCH DATA")))
						.Font(UiFont(8, true))
						.ColorAndOpacity(Muted.CopyWithNewOpacity(0.72f))
					]
				]
				]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildScoreboardTeamSection(const EFlickTeam Team)
{
	const FLinearColor Accent = GetTeamAccent(Team);
	const auto MakeHeading = [](const FString& Label, const float Width) -> TSharedRef<SWidget>
	{
		return SNew(SBox).WidthOverride(Width).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[
			SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(8, true))
			.Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor(0.7f, 0.79f, 0.84f, 1.0f))
		];
	};
	TSharedRef<SVerticalBox> PlayerRows = SNew(SVerticalBox);
	for (int32 PlayerSlot = 0; PlayerSlot < 3; ++PlayerSlot)
	{
		PlayerRows->AddSlot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this, PlayerSlot]() { return GetScoreboardPlayerVisibility(PlayerSlot); })
			[
				BuildScoreboardPlayerRow(Team, PlayerSlot)
			]
		];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(67.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Accent.CopyWithNewOpacity(0.16f))
				.AccentColor(Accent.CopyWithNewOpacity(0.48f))
				.CutSize(5.0f)
				.BorderWidth(0.8f)
				.Padding(FMargin(13.0f, 0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(52.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this, Team]()
							{
								const AFlickGameState* State = GetScoreboardGameState();
								if (!State) return FText::AsNumber(0);
								if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
								{
									const int32 Remaining = Team == EFlickTeam::Player1 ? State->Player1ActivePieces : State->Player2ActivePieces;
									return FText::AsNumber(FMath::Max(0, State->StartingPiecesPerTeam - Remaining));
								}
								return FText::AsNumber(Team == EFlickTeam::Player1 ? State->Player1RoundsWon : State->Player2RoundsWon);
							})
							.Font(DisplayFont(35)).ColorAndOpacity(Accent)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(Team == EFlickTeam::Player1 ? TEXT("BLUE") : TEXT("ORANGE")))
							.Font(UiFont(17, true)).ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this, Team]() { return GetScoreboardTeamSummary(Team); })
							.Font(UiFont(8, true)).ColorAndOpacity(Accent.CopyWithNewOpacity(0.9f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()[MakeHeading(TEXT("SCORE"), FlickScoreboardLayout::Score)]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(FlickScoreboardLayout::Knockouts).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const AFlickGameState* State = GetScoreboardGameState();
								return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("POCKETS") : TEXT("KNOCKOUTS"));
							})
							.Font(UiFont(8, true)).Justification(ETextJustify::Center)
							.ColorAndOpacity(FLinearColor(0.7f, 0.79f, 0.84f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()[MakeHeading(TEXT("SHOTS"), FlickScoreboardLayout::Shots)]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(FlickScoreboardLayout::FinalStat).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 10.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const AFlickGameState* State = GetScoreboardGameState();
								return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("IMPACTS") : TEXT("SURVIVORS"));
							})
							.Font(UiFont(8, true)).Justification(ETextJustify::Center)
							.ColorAndOpacity(FLinearColor(0.7f, 0.79f, 0.84f, 1.0f))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()[MakeHeading(TEXT("PING"), FlickScoreboardLayout::Ping)]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[PlayerRows];
}

TSharedRef<SWidget> SFlickGameLayer::BuildScoreboardPlayerRow(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	const FLinearColor Accent = GetTeamAccent(Team);
	const auto MakeStatCell = [this, Team, PlayerSlot](const int32 StatIndex, const float Width) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(Width)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team, PlayerSlot, StatIndex]()
				{
					return GetScoreboardStatText(Team, PlayerSlot, StatIndex);
				})
				.Font(UiFont(14, true))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(StatIndex == 0
					? FLinearColor::White
					: FLinearColor(0.78f, 0.85f, 0.9f, 1.0f))
			];
	};

	TSharedRef<SHorizontalBox> PingBars = SNew(SHorizontalBox);
	for (int32 BarIndex = 0; BarIndex < 4; ++BarIndex)
	{
		PingBars->AddSlot().AutoWidth().VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 2.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(3.0f).HeightOverride(4.0f + BarIndex * 3.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor_Lambda([this, Team, PlayerSlot]()
				{
					return GetScoreboardPingColor(Team, PlayerSlot);
				})
			]
		];
	}

	return SNew(SBox).HeightOverride(44.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, Team, PlayerSlot, Accent]()
			{
				const AFlickGameState* State = GetScoreboardGameState();
				const bool bActive = State
					&& State->CurrentTeam == Team
					&& State->CurrentTeamPlayerSlot == PlayerSlot
					&& State->MatchPhase != EFlickMatchPhase::RoundOver;
				const AFlickPlayerState* RowPlayer = FindScoreboardPlayerState(Team, PlayerSlot);
				const bool bLocalPlayer = RowPlayer && PlayerController.IsValid()
					&& RowPlayer == PlayerController->PlayerState;
				return bActive ? Accent.CopyWithNewOpacity(0.24f)
					: bLocalPlayer ? Accent.CopyWithNewOpacity(0.17f)
					: Accent.CopyWithNewOpacity(0.095f);
			})
			.AccentColor_Lambda([this, Team, PlayerSlot, Accent]()
			{
				const AFlickPlayerState* RowPlayer = FindScoreboardPlayerState(Team, PlayerSlot);
				return RowPlayer && PlayerController.IsValid()
					&& RowPlayer == PlayerController->PlayerState
					? Brand.CopyWithNewOpacity(0.8f)
					: Accent.CopyWithNewOpacity(0.28f);
			})
			.CutSize(3.0f)
			.BorderWidth(0.7f)
			.Padding(FMargin(13.0f, 0.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(3.0f).HeightOverride(25.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Accent)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 9.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(32.0f).HeightOverride(32.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Accent.CopyWithNewOpacity(0.65f))
						.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(1.0f))
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SImage)
								.Image_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPlayerAvatarBrush(Team, PlayerSlot); })
								.Visibility_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPlayerAvatarBrush(Team, PlayerSlot) ? EVisibility::Visible : EVisibility::Collapsed; })
							]
							+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("?")))
								.Font(UiFont(12, true))
								.ColorAndOpacity(Accent)
								.Visibility_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPlayerAvatarBrush(Team, PlayerSlot) ? EVisibility::Collapsed : EVisibility::Visible; })
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPlayerName(Team, PlayerSlot); })
					.Font(UiFont(14, true))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(0, FlickScoreboardLayout::Score)]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(1, FlickScoreboardLayout::Knockouts)]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(2, FlickScoreboardLayout::Shots)]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(3, FlickScoreboardLayout::FinalStat)]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(FlickScoreboardLayout::Ping).VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[PingBars]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPingText(Team, PlayerSlot); })
							.Font(UiFont(12, true)).Justification(ETextJustify::Right)
							.ColorAndOpacity_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPingColor(Team, PlayerSlot); })
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTeamPlate(const EFlickTeam Team)
{
	const auto IsTeamToPlay = [this, Team]()
	{
		const AFlickGameState* State = GetScoreboardGameState();
		return State && State->CurrentTeam == Team
			&& (State->MatchPhase == EFlickMatchPhase::Aiming
				|| State->MatchPhase == EFlickMatchPhase::KickoffPlanning);
	};
	TSharedRef<SHorizontalBox> SeriesPips = SNew(SHorizontalBox);
	for (int32 PipIndex = 0; PipIndex < 3; ++PipIndex)
	{
		SeriesPips->AddSlot().AutoWidth().Padding(0.0f, 0.0f, PipIndex == 2 ? 0.0f : 6.0f, 0.0f)
				[
					SNew(SFlickRoundPip)
					.Color_Lambda([this, Team]() { return GetTeamAccent(Team); })
					.Filled_Lambda([this, Team, PipIndex]()
					{
						const AFlickGameState* State = GetScoreboardGameState();
						const int32 Rounds = State ? (Team == EFlickTeam::Player1 ? State->Player1RoundsWon : State->Player2RoundsWon) : 0;
						return PipIndex < Rounds;
					})
				];
	}

	return SNew(SBox)
		.WidthOverride(218.0f)
		.HeightOverride(70.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.015f, 0.84f))
			.AccentColor_Lambda([this, Team, IsTeamToPlay]() { return GetTeamAccent(Team).CopyWithNewOpacity(IsTeamToPlay() ? 0.85f : 0.38f); })
			.CutSize(7.0f)
			.BorderWidth(1.0f)
			// Reserve a clear baseline above the shot-clock strip for the rounds/pocketed label.
			.Padding(FMargin(10.0f, 3.0f, 9.0f, 14.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
							{
								return FText::FromString(Team == EFlickTeam::Player1 ? TEXT("YOU") : TEXT("BOT"));
							}
							if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
							{
								return FText::FromString(Team == EFlickTeam::Player1 ? TEXT("YOUR PUCKS") : TEXT("TARGETS"));
							}
							return FText::FromString(State && State->PlayersPerTeam > 1
								? FString::Printf(TEXT("TEAM %d"), GetTeamNumber(Team))
								: FString::Printf(TEXT("PLAYER %d"), GetTeamNumber(Team)));
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity(GetTeamAccent(Team))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([IsTeamToPlay]() { return FText::FromString(IsTeamToPlay() ? TEXT("TO PLAY") : TEXT("")); })
						.Font(UiFont(7, true))
						.ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							return (GameMode.IsValid() && GameMode->IsFreePlayTraining())
								|| (State && State->ActiveMatchVariant == EFlickMatchVariant::Bob)
								? EVisibility::Collapsed : EVisibility::Visible;
						})
						[SeriesPips]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]() { return GetRoundsText(Team); })
						.Font(UiFont(8, true))
						.ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(5.0f, -3.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]() { return GetPieceCountText(Team); })
						.Font(DisplayFont(25))
						.ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, -3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("LEFT TO POCKET") : TEXT("PUCKS IN PLAY"));
						})
						.Font(UiFont(7, true))
						.ColorAndOpacity(Muted)
					]
				]
			]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
			[
				SNew(SBox).WidthOverride(2.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush())
					.BorderBackgroundColor_Lambda([this, Team, IsTeamToPlay]()
					{
						return GetTeamAccent(Team).CopyWithNewOpacity(IsTeamToPlay() ? 1.0f : 0.35f);
					})
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(2.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.HeightOverride(2.0f)
				.Visibility_Lambda([this]()
				{
					const AFlickGameState* State = PlayerController.IsValid()
						? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
						: nullptr;
					return State && State->bShotClockActive
						? EVisibility::HitTestInvisible
						: EVisibility::Collapsed;
				})
				[
					SNew(SProgressBar)
					.Style(&ShotClockBarStyle)
					.BarFillType(EProgressBarFillType::LeftToRight)
					.BorderPadding(FVector2D::ZeroVector)
					.Percent_Lambda([this, Team]()
					{
						const AFlickGameState* State = PlayerController.IsValid()
							? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
							: nullptr;
						return TOptional<float>(State ? State->GetShotClockFraction(Team) : 1.0f);
					})
					.FillColorAndOpacity_Lambda([this, Team]()
					{
						const FLinearColor Accent = GetTeamAccent(Team);
						const AFlickGameState* State = PlayerController.IsValid()
							? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
							: nullptr;
						const bool bClockActive = State
							&& State->CurrentTeam == Team
							&& (State->MatchPhase == EFlickMatchPhase::Aiming
								|| State->MatchPhase == EFlickMatchPhase::KickoffPlanning);
						if (!bClockActive)
						{
							return FSlateColor(Accent.CopyWithNewOpacity(0.2f));
						}
						const float Fraction = State ? State->GetShotClockFraction(Team) : 1.0f;
						if (bClockActive && Fraction <= 1.0f / 3.0f)
						{
							return FSlateColor(Fraction <= 0.12f
								? FLinearColor(1.0f, 0.08f, 0.015f, 1.0f)
								: FLinearColor(1.0f, 0.34f, 0.12f, 1.0f));
						}
						return FSlateColor(Accent);
					})
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTrainingToolsPanel()
{
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsFreePlayTraining()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.WidthOverride(320.0f)
		.HeightOverride(128.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.01f, 0.018f, 0.95f))
			.AccentColor(Cyan.CopyWithNewOpacity(0.82f))
			.CutSize(14.0f)
			.BorderWidth(1.1f)
			.UseAccentForOutline(true)
			.Padding(FMargin(20.0f, 14.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("TRAINING TOOLS")))
					.Font(UiFont(14, true))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!GameMode.IsValid() || !GameMode->IsTrainingEditMode())
						{
							return FText::FromString(TEXT("SHOT MODE  /  T EDIT BOARD"));
						}
						return FText::FromString(FString::Printf(
							TEXT("%s  /  %s"),
							GameMode->GetTrainingPlacementTeam() == EFlickTeam::Player1 ? TEXT("YOUR PUCK") : TEXT("TARGET PUCK"),
							GameMode->IsBobMode()
								? TEXT("STANDARD ONLY")
								: *GetPieceArchetypeName(GameMode->GetTrainingPlacementArchetype())));
					})
					.Font(UiFont(11, true))
					.ColorAndOpacity_Lambda([this]()
					{
						if (!GameMode.IsValid() || !GameMode->IsTrainingEditMode())
						{
							return FSlateColor(FLinearColor(0.2f, 0.78f, 0.5f, 1.0f));
						}
						return FSlateColor(GetTeamAccent(GameMode->GetTrainingPlacementTeam()));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (GameMode.IsValid() && GameMode->IsTrainingEditMode())
						{
							return FText::FromString(GameMode->IsBobMode()
								? TEXT("1 OWN  2 TARGET  |  LMB PLACE/DRAG  |  DEL REMOVE NON-STRIKERS")
								: TEXT("WHEEL TYPE  |  LMB PLACE/DRAG PUCK OR TOGGLE DIVIDER  |  DEL REMOVE"));
						}
						return FText::FromString(TEXT("Aim and shoot normally   |   R restores your saved setup"));
					})
					.Font(UiFont(8, true))
					.AutoWrapText(true)
					.WrapTextAt(272.0f)
					.ColorAndOpacity(FLinearColor(0.68f, 0.76f, 0.82f, 1.0f))
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTutorialOverlay()
{
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsTutorialMode()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.WidthOverride(430.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.003f, 0.014f, 0.024f, 0.94f))
			.AccentColor(Cyan.CopyWithNewOpacity(0.82f))
			.CutSize(12.0f)
			.BorderWidth(1.0f)
			.UseAccentForOutline(true)
			.Padding(FMargin(22.0f, 13.0f, 22.0f, 15.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (!GameMode.IsValid()) return FText::GetEmpty();
							return FText::FromString(GameMode->IsTutorialComplete()
								? TEXT("GUIDED TRAINING  /  COMPLETE")
								: FString::Printf(TEXT("GUIDED TRAINING  /  LESSON %d OF %d"),
									GameMode->GetTutorialStageNumber(), GameMode->GetTutorialStageCount()));
						})
						.Font(UiFont(10, true))
						.ColorAndOpacity(Cyan)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("R  RETRY")))
						.Font(UiFont(9, true))
						.ColorAndOpacity(Muted)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetTutorialTitle() : TEXT("TUTORIAL")); })
					.Font(DisplayFont(21))
					.ColorAndOpacity(Paper)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetTutorialObjective() : FString()); })
					.Font(UiFont(11, true))
					.ColorAndOpacity(FLinearColor::White)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetTutorialHint() : FString()); })
					.Font(UiFont(9))
					.ColorAndOpacity(Muted)
					.AutoWrapText(true)
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildControlHintPanel(const bool bRightSide)
{
	const auto MakeHintRow = [bRightSide](const bool bTraining, const bool bEditing) -> TSharedRef<SHorizontalBox>
	{
		TSharedRef<SHorizontalBox> HintRow = SNew(SHorizontalBox);
		const FLinearColor Accent = bRightSide ? Orange : Cyan;
		const auto AddHint = [&HintRow, Accent](const FString& Key, const FString& Label, const bool bLast)
		{
			HintRow->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
				.AccentColor(Accent.CopyWithNewOpacity(0.72f))
				.CutSize(4.0f)
				.BorderWidth(0.8f)
				.Padding(FMargin(7.0f, 3.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Key))
					.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
				]
			];
			HintRow->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, bLast ? 0.0f : 14.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
			];
		};

		if (bRightSide)
		{
			if (bTraining && bEditing)
			{
				AddHint(TEXT("T"), TEXT("SAVE / DONE"), false);
				AddHint(TEXT("C"), TEXT("CLEAR"), false);
				AddHint(TEXT("R"), TEXT("RESET"), true);
			}
			else if (bTraining)
			{
				AddHint(TEXT("T"), TEXT("EDIT BOARD"), false);
				AddHint(TEXT("X"), TEXT("FREE CAMERA"), false);
				AddHint(TEXT("V"), TEXT("TOP VIEW"), false);
				AddHint(TEXT("R"), TEXT("RESET"), true);
			}
			else
			{
				AddHint(TEXT("TAB"), TEXT("SCOREBOARD"), false);
				AddHint(TEXT("R"), TEXT("RESTART"), false);
				AddHint(TEXT("ESC"), TEXT("PAUSE"), true);
			}
		}
		else if (bTraining && bEditing)
		{
			AddHint(TEXT("LMB"), TEXT("PUCK / DIVIDER"), false);
			AddHint(TEXT("DEL"), TEXT("REMOVE"), false);
			AddHint(TEXT("1 / 2"), TEXT("OWN / TARGET"), true);
		}
		else if (bTraining)
		{
			AddHint(TEXT("LMB"), TEXT("AIM"), false);
			AddHint(TEXT("DRAG"), TEXT("POWER"), false);
			AddHint(TEXT("RELEASE"), TEXT("SHOOT"), true);
		}
		else
		{
			AddHint(TEXT("LMB"), TEXT("AIM"), false);
			AddHint(TEXT("DRAG"), TEXT("POWER"), false);
			AddHint(TEXT("RELEASE"), TEXT("SHOOT"), true);
		}
		return HintRow;
	};

	TSharedRef<SWidget> StandardHints = SNew(SBox)
		.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsFreePlayTraining() ? EVisibility::Collapsed : EVisibility::Visible; })
		[MakeHintRow(false, false)];
	TSharedRef<SWidget> TrainingShotHints = SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsFreePlayTraining() && !GameMode->IsTrainingEditMode()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[MakeHintRow(true, false)];
	TSharedRef<SWidget> TrainingEditHints = SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsTrainingEditMode()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[MakeHintRow(true, true)];

	return SNew(SBox)
		// Matching outer widths keep the centered camera panel optically and
		// mathematically equidistant from both bottom-corner panels.
		.WidthOverride(470.0f)
		.HeightOverride(66.0f)
		.Visibility_Lambda([this]()
		{
			return !GameMode.IsValid() || GameMode->IsControlOverviewEnabled()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.017f, 0.94f))
			.AccentColor((bRightSide ? Orange : Cyan).CopyWithNewOpacity(0.7f))
			.CutSize(13.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(15.0f, 7.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this, bRightSide]()
					{
						if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
						{
							if (GameMode->IsTrainingEditMode())
							{
								return FText::FromString(bRightSide ? TEXT("BOARD TOOLS") : TEXT("BOARD EDITOR"));
							}
							return FText::FromString(bRightSide ? TEXT("TRAINING") : TEXT("SHOT CONTROL"));
						}
						return FText::FromString(bRightSide ? TEXT("MATCH") : TEXT("SHOT CONTROL"));
					})
					.Font(UiFont(7, true))
					.ColorAndOpacity((bRightSide ? Orange : Cyan).CopyWithNewOpacity(0.9f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[StandardHints]
					+ SOverlay::Slot()[TrainingShotHints]
					+ SOverlay::Slot()[TrainingEditHints]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPowerMeter()
{
	TSharedRef<SHorizontalBox> PowerSegments = SNew(SHorizontalBox);
	for (int32 SegmentIndex = 0; SegmentIndex < 10; ++SegmentIndex)
	{
		PowerSegments->AddSlot().FillWidth(1.0f).Padding(
			0.0f,
			0.0f,
			SegmentIndex == 9 ? 0.0f : 3.0f,
			0.0f)
		[
			SNew(SBox).HeightOverride(14.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor_Lambda([this, SegmentIndex]()
				{
					const float Power = PlayerController.IsValid()
						? PlayerController->GetAimResult().NormalizedPower
						: 0.0f;
					return Power >= static_cast<float>(SegmentIndex + 1) / 10.0f
						? GetPowerColor().GetSpecifiedColor()
						: FLinearColor(0.065f, 0.068f, 0.06f, 1.0f);
				})
			]
		];
	}

	return SNew(SBox)
		.WidthOverride(320.0f)
		.Visibility_Lambda([this]() { return GetPowerVisibility(); })
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(Panel)
			.AccentColor(Brand)
			.CutSize(0.0f)
			.Padding(FMargin(18.0f, 14.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("SHOT POWER"))).Font(UiFont(10, true)).ColorAndOpacity(Paper)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock).Text_Lambda([this]() { return GetPowerText(); }).Font(DisplayFont(28)).ColorAndOpacity_Lambda([this]() { return GetPowerColor(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
				[
					PowerSegments
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("RELEASE TO FLICK")))
					.Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("RMB / ESC  CANCEL")))
						.Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildCameraOrbitHint()
{
	return SNew(SBox)
		.WidthOverride(610.0f)
		.HeightOverride(66.0f)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid()
				&& GameMode->IsControlOverviewEnabled()
				&& GameMode->CanChangeCameraView()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.017f, 0.94f))
			.AccentColor(Cyan.CopyWithNewOpacity(0.7f))
			.CutSize(11.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(15.0f, 7.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
					return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive()
						? TEXT("FREE CAMERA  /  X OR ESC TO EXIT")
						: GameMode.IsValid() && GameMode->IsTrainingEditMode()
							? TEXT("EDIT VIEW  /  VERTICAL ANGLE LOCKED")
							: TEXT("CAMERA"));
					})
					.Font(UiFont(7, true))
					.ColorAndOpacity(Cyan.CopyWithNewOpacity(0.9f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
						.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
						.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(8.0f, 3.0f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive() ? TEXT("WASD") : TEXT("Q / E")); })
							.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 14.0f, 0.0f)
					[
					SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive() ? TEXT("MOVE") : TEXT("ORBIT")); }).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return GameMode.IsValid() && GameMode->IsTrainingEditMode() && GameMode->IsBobMode()
								? EVisibility::Collapsed
								: EVisibility::Visible;
						})
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
							.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
							.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(8.0f, 3.0f))
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive() ? TEXT("MOUSE") : TEXT("WHEEL")); })
								.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (GameMode.IsValid() && GameMode->IsTrainingEditMode())
							{
								if (GameMode->IsBobMode())
								{
									return FText::FromString(TEXT("STANDARD PUCKS ONLY"));
								}
								return FText::FromString(FString::Printf(
									TEXT("PUCK TYPE  /  %s"),
									*GetPieceArchetypeName(GameMode->GetTrainingPlacementArchetype())));
							}
							return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive()
								? TEXT("LOOK")
								: TEXT("VERTICAL ANGLE"));
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
						.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
						.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(8.0f, 3.0f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive() ? TEXT("SPACE / CTRL") : TEXT("F")); })
							.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
					[
					SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(PlayerController.IsValid() && PlayerController->IsFreeCameraActive() ? TEXT("HEIGHT") : TEXT("RESET")); }).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildEventFeed()
{
	TSharedRef<SVerticalBox> Feed = SNew(SVerticalBox);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Feed->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			SNew(SBox)
			.WidthOverride(320.0f)
			.Visibility_Lambda([this, Index]() { return GetEventVisibility(Index); })
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor(Panel.CopyWithNewOpacity(0.95f))
				.Padding(FMargin(0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(3.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush())
							.BorderBackgroundColor_Lambda([this, Index]() { return GetEventColor(Index).GetSpecifiedColor(); })
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(13.0f, 12.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Index]() { return GetEventText(Index); })
						.Font(UiFont(10, true))
						.AutoWrapText(true)
						.ColorAndOpacity(Paper)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f)
					[
						SNew(STextBlock)
						.Visibility_Lambda([this, Index]() { return GetEventPointsVisibility(Index); })
						.Text_Lambda([this, Index]() { return GetEventPointsText(Index); })
						.Font(UiFont(14, true))
						.ColorAndOpacity(Brand)
					]
				]
			]
		];
	}
	return Feed;
}
