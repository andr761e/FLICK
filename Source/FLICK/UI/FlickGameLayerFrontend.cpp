// main menu, profile, social, shop
#include "UI/FlickGameLayerPrivate.h"

TSharedRef<SWidget> SFlickGameLayer::BuildStartupOverlay()
{
	// The movie-player splash owns the startup logo. Once the frontend is ready,
	// only fade its dark backdrop so a second copy cannot linger over the menu.
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 1.0f));
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenu()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.08f))
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([this]()
			{
				const float Opacity = GameMode.IsValid()
					? GameMode->GetMenuPreviewTransitionOpacity()
					: 0.0f;
				return FLinearColor(0.0f, 0.006f, 0.012f, Opacity);
			})
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility(EVisibility::HitTestInvisible)
			.Opacity(0.9f)
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickDiagonalPanel)
			.Visibility(EVisibility::HitTestInvisible)
			.PanelColor(Ink)
			.EdgeColor(Hairline)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(52.0f, 62.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 18.0f, 0.0f)
				[
					SNew(SBox)
					.WidthOverride(36.0f)
					.HeightOverride(3.0f)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush())
						.BorderBackgroundColor(Brand)
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("PHYSICS. PRECISION. RIVALRY.")))
					.Font(UiFont(11, true))
					.ColorAndOpacity(Brand)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(-5.0f, 7.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(450.0f)
				.HeightOverride(150.0f)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					.StretchDirection(EStretchDirection::DownOnly)
					[
						SNew(SFlickLogoWidget)
						.TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark"))
						.DesiredSize(FVector2D(2048.0f, 683.0f))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("Small move. Big consequences."))).Font(WordmarkTaglineFont(15)).ColorAndOpacity(Muted)
				.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->GetFrontendScreen() == EFlickFrontendScreen::Profile ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
		]

		+ SOverlay::Slot()
		[
			SNew(SOverlay)
			.Visibility_Lambda([this]()
			{
				// A joined party client has no local authority GameMode, but should
				// still retain the normal frontend rather than seeing only the arena.
				if (!GameMode.IsValid()) return EVisibility::Visible;
				if (GameMode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu) return EVisibility::Visible;
				return EVisibility::Collapsed;
			})
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(52.0f, 282.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(450.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(0))
					[
						MakeMainMenuButton(TEXT("PLAY"), FOnClicked::CreateLambda([this]()
						{
							if (!GameMode.IsValid()) return FReply::Handled();
							SelectedPlayPlaylist = EFlickPlayPlaylist::None;
							SelectedTrainingActivity = EFlickTrainingActivity::None;
							if (GameMode.IsValid()) GameMode->OpenModeSelect();
							return FReply::Handled();
						}), true, false, MainMenuStackMetrics::GetRowHeight(0))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(1), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox)
					.WidthOverride(MainMenuStackMetrics::GetRowWidth(1))
					[
						MakeMainMenuButton(TEXT("LINEUPS"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->OpenLoadout(); else RemotePartyScreen = EFlickFrontendScreen::Loadout;
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(1))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(2), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(2))
					[
						MakeMainMenuButton(TEXT("PROFILE"), FOnClicked::CreateLambda([this]()
						{
							SelectedProfileTab = EFlickProfileTab::Stats;
							if (GameMode.IsValid()) GameMode->OpenProfile(); else RemotePartyScreen = EFlickFrontendScreen::Profile;
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(2))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(3), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(3))
					[
						MakeMainMenuButton(TEXT("ITEM SHOP"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->OpenItemShop(); else RemotePartyScreen = EFlickFrontendScreen::ItemShop;
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(3))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(4), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(4))
					[
						MakeMainMenuButton(TEXT("SETTINGS"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->OpenSettings(); else RemotePartyScreen = EFlickFrontendScreen::Settings;
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(4))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(5), 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(5))
					[
						MakeMainMenuButton(TEXT("QUIT"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->QuitGame();
							return FReply::Handled();
						}), false, true, MainMenuStackMetrics::GetRowHeight(5))
					]
				]
				]
			]
				+ SOverlay::Slot()
			.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 52.0f, 124.0f)
		[
			SNew(SBox)
			.WidthOverride(500.0f)
			.HeightOverride(160.0f)
			[
				SNew(SFlickMainMenuPanel)
				.BackgroundColor(FLinearColor::FromSRGBColor(FColor(14, 23, 25, 242)))
				.AccentColor(Brand)
				.CutSize(16.0f)
				.BorderWidth(1.0f)
				.RichShowcaseBorder(true)
				.Padding(FMargin(30.0f, 17.0f, 30.0f, 15.0f))
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("FEATURED MODE")))
								.Font(UiFont(14, true))
								.ColorAndOpacity(Brand)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								const int32 TeamSize = GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1;
								return FText::FromString(Variant == EFlickMatchVariant::Bob
									? TEXT("BOB")
									: FString::Printf(TEXT("%dV%d KNOCKOUT"), TeamSize, TeamSize));
							})
						.Font(DisplayFont(34, true))
						.ColorAndOpacity(Paper)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								if (Variant == EFlickMatchVariant::Bob) return FText::FromString(GetMatchVariantSummary(Variant));
								const int32 TeamSize = GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1;
								return FText::FromString(TeamSize == 1
									? TEXT("Every angle matters. Win the one-on-one duel.")
									: TeamSize == 2
										? TEXT("Coordinate your pair and control the arena.")
										: TEXT("Six players. Shared turns. Total knockout chaos."));
							})
						.Font(UiFont(13))
						.ColorAndOpacity(FLinearColor(0.67f, 0.72f, 0.78f, 0.96f))
						.AutoWrapText(false)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Bottom).HAlign(HAlign_Left).Padding(0.0f, 15.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(132.0f).HeightOverride(6.0f)
						[
							SNew(SFlickShowcaseProgress)
							.Percent_Lambda([this]() { return GameMode.IsValid() ? GameMode->GetMenuPreviewAlpha() : 0.0f; })
							.AccentColor(Brand)
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(52.0f, 0.0f, 0.0f, 48.0f)
		[
			SNew(SBox).WidthOverride(380.0f).HeightOverride(82.0f)
			[
				SNew(SFlickMainMenuPanel)
				.BackgroundColor_Lambda([this]()
				{
					return SelectedBannerStyle == 2
						? FLinearColor(0.018f, 0.075f, 0.105f, 0.97f)
						: SelectedBannerStyle == 1
							? FLinearColor(0.045f, 0.065f, 0.04f, 0.97f)
							: FLinearColor::FromSRGBColor(FColor(14, 23, 25, 247));
				})
				.AccentColor_Lambda([this]() { return SelectedAvatarBorder == 2 ? Brand : SelectedAvatarBorder == 1 ? Cyan : Hairline; })
				.CutSize(10.0f).BorderWidth(1.15f).Padding(FMargin(12.0f, 9.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 0.0f, 13.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(62.0f)
						[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor_Lambda([this]() { return SelectedAvatarBorder == 2 ? Brand : SelectedAvatarBorder == 1 ? Cyan : Hairline; })
						.CutSize(8.0f).BorderWidth(1.4f).Padding(FMargin(3.0f))
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SImage)
								.Image_Lambda([this]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions ? Sessions->GetLocalAvatarBrush() : nullptr; })
								.Visibility_Lambda([this]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetLocalAvatarBrush() ? EVisibility::Visible : EVisibility::Collapsed; })
							]
							+ SOverlay::Slot().Padding(9.0f, 6.0f)
							[
								SNew(SFlickStatusGlobe).Color(Cyan.CopyWithNewOpacity(0.94f))
								.Visibility_Lambda([this]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetLocalAvatarBrush() ? EVisibility::Collapsed : EVisibility::Visible; })
							]
						]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]() { const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER")); }).Font(UiFont(15, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)[SNew(STextBlock).Text_Lambda([this]() { static const TCHAR* Tags[] = {TEXT("READY TO FLICK"), TEXT("TABLE TACTICIAN"), TEXT("RIVAL INCOMING")}; return FText::FromString(Tags[SelectedBannerTag % 3]); }).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(450.0f, 0.0f, 0.0f, 44.0f)
		[
			SNew(SBox).HeightOverride(72.0f)
			.Visibility_Lambda([this]() { return IsDisplayedPartyActive() ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor::FromSRGBColor(FColor(9, 17, 19, 246)))
				.AccentColor(Cyan.CopyWithNewOpacity(0.58f))
				.CutSize(10.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(9.0f, 6.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBox).WidthOverride(20.0f).HeightOverride(2.0f)
							[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(7.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("PARTY  //  SELECT A MEMBER FOR CONTROLS"))).Font(UiFont(7, true)).ColorAndOpacity(Muted)
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[BuildMainMenuPartyMember(0)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f)[BuildMainMenuPartyMember(1)]
						+ SHorizontalBox::Slot().AutoWidth()[BuildMainMenuPartyMember(2)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f)[BuildMainMenuPartyMember(3)]
						+ SHorizontalBox::Slot().AutoWidth()[BuildMainMenuPartyMember(4)]
						+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(5)]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 28.0f, 270.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(390.0f).HeightOverride(72.0f)
			.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SFlickMainMenuPanel)
				.BackgroundColor(FLinearColor::FromSRGBColor(FColor(14, 23, 25, 242)))
				.AccentColor(Brand)
				.CutSize(12.0f)
				.BorderWidth(1.3f)
				.Padding(FMargin(20.0f, 9.0f, 12.0f, 9.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
						[
							SNew(SFlickMainMenuIcon)
							.Icon(EFlickMainMenuIcon::Rank)
							.Color(Brand)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									if (!GameMode.IsValid()) return FText::FromString(TEXT("CURRENT RANK"));
									const EFlickMatchVariant Variant = GameMode->GetActiveMatchVariant();
									return FText::FromString(Variant == EFlickMatchVariant::Bob
										? TEXT("BOB RANK")
										: FString::Printf(TEXT("%dV%d CURRENT RANK"), GameMode->GetPlayersPerTeam(), GameMode->GetPlayersPerTeam()));
								})
								.Font(UiFont(11, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
									const EFlickMatchVariant Variant = GameMode.IsValid()
										? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
									const int32 TeamSize = GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1;
									return FText::FromString(Ranking
										? Ranking->GetProgressLabel(Variant, TeamSize)
										: TEXT("UNAVAILABLE"));
								})
								.Font(UiFont(14, true)).ColorAndOpacity(Brand)
							]
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 28.0f, 34.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(220.0f).HeightOverride(72.0f)
			.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SFlickMainMenuPanel)
				.BackgroundColor(FLinearColor::FromSRGBColor(FColor(14, 23, 25, 242)))
				.AccentColor(Brand)
				.CutSize(12.0f)
				.BorderWidth(1.3f)
				.Padding(FMargin(1.5f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.ContentPadding(FMargin(14.0f, 8.0f))
					.OnClicked_Lambda([this]()
				{
					bSocialPanelOpen = !bSocialPanelOpen;
					if (bSocialPanelOpen)
					{
						if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem())
						{
							Sessions->RefreshFriends();
						}
					}
					return FReply::Handled();
				})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 17.0f, 0.0f)
						[
							SNew(SFlickMainMenuIcon)
							.Icon(EFlickMainMenuIcon::Social)
							.Color(FLinearColor(0.7f, 0.77f, 0.82f, 1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SOCIAL")))
							.Font(UiFont(17, true))
							.ColorAndOpacity(FLinearColor::White)
						]
					]
				]
			]
			]
			+ SOverlay::Slot()
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.ContentPadding(FMargin(0.0f))
				.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				.OnClicked_Lambda([this]()
				{
					bSocialPanelOpen = false;
					return FReply::Handled();
				})
				[
					SNew(SSpacer)
				]
			]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(468.0f)
			.Visibility_Lambda([this]()
			{
				return bSocialPanelOpen ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				BuildSocialPanel()
			]
		]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenuFooter()
{
	return SNew(SBox)
		.HeightOverride(32.0f)
		.Clipping(EWidgetClipping::ClipToBounds)
		.Visibility_Lambda([this]()
		{
			return GetScreenVisibility(EFlickFrontendScreen::MainMenu) == EVisibility::Collapsed
				? EVisibility::Collapsed
				: EVisibility::HitTestInvisible;
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::FromSRGBColor(FColor(9, 17, 19, 218))).Padding(0.0f)
			]
			+ SOverlay::Slot().VAlign(VAlign_Top)
			[
				SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(0.42f))]
			]
			+ SOverlay::Slot().VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				.RenderTransform_Lambda([this, TickerStartTime = FPlatformTime::Seconds()]()
				{
					const float ViewportWidth = FMath::Max(LayerLocalSize.X, 1.0f);
					const double TravelDistance = static_cast<double>(ViewportWidth) + 1900.0;
					const double ElapsedTime = FMath::Max(0.0, FPlatformTime::Seconds() - TickerStartTime);
					const float Offset = static_cast<float>(FMath::Fmod(ElapsedTime * 55.0, TravelDistance));
					return FSlateRenderTransform(FVector2D(ViewportWidth - 120.0f - Offset, 0.0f));
				})
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK NETWORK  "))).Font(UiFont(8, true)).ColorAndOpacity(Muted)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("  BUILD YOUR LINEUP. MASTER THE ANGLE. OWN THE TABLE.     "))).Font(UiFont(8, true)).ColorAndOpacity(Muted)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("     COMPETITIVE SEASON PREVIEW COMING SOON     "))).Font(UiFont(8, true)).ColorAndOpacity(Muted)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("     FLICK NETWORK  //  BUILD YOUR LINEUP. MASTER THE ANGLE. OWN THE TABLE."))).Font(UiFont(8, true)).ColorAndOpacity(Muted)]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfileStatsPanel()
{
	auto MakeStatCell = [this](const FString& Label, const FString& Detail, const int32 StatIndex, const FLinearColor& Accent) -> TSharedRef<SWidget>
	{
		return SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Hairline)
			.CutSize(9.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(18.0f, 11.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(UiFont(10, true))
					.ColorAndOpacity(Brand)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, StatIndex]()
					{
						const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
							? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
							: nullptr;
						if (!FlickGameInstance)
						{
							return FText::AsNumber(0);
						}
						const FFlickProfileStats& Stats = FlickGameInstance->GetProfileStats();
						switch (StatIndex)
						{
						case 0: return FText::AsNumber(Stats.MatchesPlayed);
						case 1: return FText::AsNumber(Stats.Wins);
						case 2: return FText::AsNumber(Stats.Points);
						case 3: return FText::AsNumber(Stats.Knockouts);
						case 4: return FText::AsNumber(Stats.DoubleKnockouts);
						case 5: return FText::AsNumber(Stats.Shots);
						case 6: return FText::AsNumber(Stats.GetTotalAccolades());
						case 7: return FText::AsNumber(Stats.GetAccoladeCount(EFlickAccolade::TeamWipeout));
						case 8: return FText::AsNumber(Stats.GetAccoladeCount(EFlickAccolade::FlawlessRound));
						default: return FText::AsNumber(0);
						}
					})
					.Font(DisplayFont(38))
					.ColorAndOpacity(Paper)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Detail))
					.Font(UiFont(10))
					.ColorAndOpacity(Muted)
				]
			];
	};

	return SNew(SFlickAngularBorder)
		.BackgroundColor(Panel)
		.AccentColor(Hairline)
		.UseAccentForOutline(false)
		.CutSize(14.0f)
		.BorderWidth(1.1f)
		.Padding(FMargin(22.0f, 17.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CAREER OVERVIEW")))
					.Font(DisplayFont(24))
					.ColorAndOpacity(Paper)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
							? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
						const FFlickProfileStats Stats = FlickGameInstance ? FlickGameInstance->GetProfileStats() : FFlickProfileStats();
						const int32 WinRate = Stats.MatchesPlayed > 0
							? FMath::RoundToInt(100.0f * Stats.Wins / static_cast<float>(Stats.MatchesPlayed)) : 0;
						return FText::FromString(FString::Printf(TEXT("ALL MODES  //  %d%% WIN RATE"), WinRate));
					})
					.Font(UiFont(9, true))
					.ColorAndOpacity(Brand)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f))
				+ SUniformGridPanel::Slot(0, 0)[MakeStatCell(TEXT("MATCHES PLAYED"), TEXT("COMPLETED SERIES"), 0, Cyan)]
				+ SUniformGridPanel::Slot(1, 0)[MakeStatCell(TEXT("MATCH WINS"), TEXT("ALL PLAYLISTS"), 1, FLinearColor(0.26f, 0.9f, 0.58f, 1.0f))]
				+ SUniformGridPanel::Slot(2, 0)[MakeStatCell(TEXT("CAREER POINTS"), TEXT("SHOTS, IMPACTS AND KOS"), 2, Orange)]
				+ SUniformGridPanel::Slot(0, 1)[MakeStatCell(TEXT("KNOCKOUTS"), TEXT("OPPONENT PUCKS REMOVED"), 3, Cyan)]
				+ SUniformGridPanel::Slot(1, 1)[MakeStatCell(TEXT("DOUBLE KOS"), TEXT("TWO PUCKS IN ONE SHOT"), 4, Orange)]
				+ SUniformGridPanel::Slot(2, 1)[MakeStatCell(TEXT("SHOTS TAKEN"), TEXT("ALL COMPLETED MATCHES"), 5, FLinearColor(0.68f, 0.77f, 0.86f, 1.0f))]
				+ SUniformGridPanel::Slot(0, 2)[MakeStatCell(TEXT("STYLE ACCOLADES"), TEXT("SPECIAL SHOTS AND PLAYS"), 6, FLinearColor(0.72f, 0.48f, 0.96f, 1.0f))]
				+ SUniformGridPanel::Slot(1, 2)[MakeStatCell(TEXT("TEAM WIPEOUTS"), TEXT("MULTI-KO ROUND FINISHES"), 7, Orange)]
				+ SUniformGridPanel::Slot(2, 2)[MakeStatCell(TEXT("FLAWLESS ROUNDS"), TEXT("NO FRIENDLY PUCKS LOST"), 8, Cyan)]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfile()
{
	auto MakeRankCard = [this](const int32 PlayersPerTeam, const FString& Label) -> TSharedRef<SWidget>
	{
		return SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Brand)
			.UseAccentForOutline(false).CutSize(12.0f).BorderWidth(1.0f).Padding(FMargin(20.0f, 22.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(11, true)).ColorAndOpacity(Muted)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, PlayersPerTeam]()
					{
						const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
						return FText::FromString(Ranking
							? Ranking->GetProgressLabel(EFlickMatchVariant::Classic, PlayersPerTeam)
							: TEXT("UNAVAILABLE"));
					})
					.Font(DisplayFont(19)).ColorAndOpacity(Paper)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("RANKED KNOCKOUT  //  CURRENT SEASON"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
			];
	};

	TSharedRef<SVerticalBox> HistoryRows = SNew(SVerticalBox);
	for (int32 MatchIndex = 0; MatchIndex < 8; ++MatchIndex)
	{
		HistoryRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
		[
			SNew(SBox).HeightOverride(60.0f)
			.Visibility_Lambda([this, MatchIndex]()
			{
				const UFlickGameInstance* Instance = PlayerController.IsValid()
					? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
				return Instance && Instance->GetRecentMatches().IsValidIndex(MatchIndex) ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(PanelRaised)
				.AccentColor_Lambda([this, MatchIndex]()
				{
					const UFlickGameInstance* Instance = PlayerController.IsValid()
						? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
					if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return Hairline;
					const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
					return Record.bDraw ? FLinearColor(0.9f, 0.75f, 0.2f, 0.75f)
						: Record.bWon ? FLinearColor(0.25f, 0.9f, 0.55f, 0.78f) : Orange.CopyWithNewOpacity(0.72f);
				})
				.CutSize(7.0f).BorderWidth(0.8f).Padding(FMargin(18.0f, 9.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(82.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this, MatchIndex]()
							{
								const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
								if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return FText::GetEmpty();
								const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
								return FText::FromString(Record.bDraw ? TEXT("DRAW") : Record.bWon ? TEXT("WIN") : TEXT("LOSS"));
							})
							.Font(UiFont(12, true)).ColorAndOpacity(Paper)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, MatchIndex]()
						{
							const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
							if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return FText::GetEmpty();
							const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
							const FString Mode = Record.Variant == EFlickMatchVariant::Bob
								? TEXT("BOB") : FString::Printf(TEXT("%dV%d KNOCKOUT"), Record.PlayersPerTeam, Record.PlayersPerTeam);
							return FText::FromString(FString::Printf(TEXT("%s  //  %s"), Record.bRanked ? TEXT("COMPETITIVE") : TEXT("CASUAL"), *Mode));
						})
						.Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, MatchIndex]()
						{
							const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
							if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return FText::GetEmpty();
							const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
							return FText::FromString(FString::Printf(TEXT("%d PTS   //   %d KOS   //   %d SHOTS"), Record.Points, Record.Knockouts, Record.Shots));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
				]
			]
		];
	}

	const auto MakeTab = [this](EFlickProfileTab Tab, const FString& Label) -> TSharedRef<SWidget>
	{
		TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0.0f)
			.OnClicked_Lambda([this, Tab]() { SelectedProfileTab = Tab; return FReply::Handled(); });
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(SNew(SBorder).BorderImage(WhiteBrush()).Padding(FMargin(18.0f, 9.0f))
			.BorderBackgroundColor_Lambda([this, Tab, WeakButton]()
			{
				const auto Pinned = WeakButton.Pin();
				return SelectedProfileTab == Tab ? Brand : Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()) ? PanelRaised : Panel;
			})
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(11, true))
			.ColorAndOpacity_Lambda([this, Tab]() { return SelectedProfileTab == Tab ? Ink : Paper; })]);
		return Button;
	};
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.004f, 0.012f, 0.02f, 0.82f))]
		+ SOverlay::Slot()[SNew(SFlickInterfaceBackdrop).Visibility(EVisibility::HitTestInvisible).Opacity(0.22f)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(42.0f)
		[
			SNew(SBox)
			.WidthOverride(1380.0f)
			.HeightOverride(760.0f)
			[
			SNew(SFlickAngularBorder)
			.BackgroundColor(Panel.CopyWithNewOpacity(0.97f))
			.AccentColor(Brand)
			.UseAccentForOutline(false)
			.CutSize(18.0f)
			.BorderWidth(1.2f)
			.Padding(FMargin(28.0f, 22.0f))
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK  /  YOUR RECORD"))).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 14.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE"))).Font(DisplayFont(36)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)[MakeTab(EFlickProfileTab::Stats, TEXT("STATS"))]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)[MakeTab(EFlickProfileTab::Leaderboards, TEXT("LEADERBOARDS"))]
				+ SHorizontalBox::Slot().AutoWidth()[MakeTab(EFlickProfileTab::MatchHistory, TEXT("MATCH HISTORY"))]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)[MakeTab(EFlickProfileTab::Customization, TEXT("CUSTOMIZE"))]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(500.0f)
				[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Customization ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(Brand).UseAccentForOutline(false).CutSize(14.0f).BorderWidth(1.1f).Padding(FMargin(32.0f, 28.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("PLAYER IDENTITY"))).Font(DisplayFont(28)).ColorAndOpacity(Paper)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 22.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("Choose how your player plate appears in menus and parties."))).Font(UiFont(11)).ColorAndOpacity(Muted)]
							+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("BANNER"), TAttribute<FText>::CreateLambda([this]() { static const TCHAR* Names[] = {TEXT("CARBON"), TEXT("ARENA"), TEXT("GLACIER")}; return FText::FromString(Names[SelectedBannerStyle % 3]); }), FOnClicked::CreateLambda([this]() { SelectedBannerStyle = (SelectedBannerStyle + 2) % 3; GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("BannerStyle"), SelectedBannerStyle, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { SelectedBannerStyle = (SelectedBannerStyle + 1) % 3; GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("BannerStyle"), SelectedBannerStyle, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); return FReply::Handled(); }))]
							+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("BANNER TAG"), TAttribute<FText>::CreateLambda([this]() { static const TCHAR* Names[] = {TEXT("READY TO FLICK"), TEXT("TABLE TACTICIAN"), TEXT("RIVAL INCOMING")}; return FText::FromString(Names[SelectedBannerTag % 3]); }), FOnClicked::CreateLambda([this]() { SelectedBannerTag = (SelectedBannerTag + 2) % 3; GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("BannerTag"), SelectedBannerTag, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { SelectedBannerTag = (SelectedBannerTag + 1) % 3; GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("BannerTag"), SelectedBannerTag, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); return FReply::Handled(); }))]
							+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("AVATAR BORDER"), TAttribute<FText>::CreateLambda([this]() { static const TCHAR* Names[] = {TEXT("STANDARD"), TEXT("CYAN CIRCUIT"), TEXT("LIME CHAMPION")}; return FText::FromString(Names[SelectedAvatarBorder % 3]); }), FOnClicked::CreateLambda([this]() { SelectedAvatarBorder = (SelectedAvatarBorder + 2) % 3; GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("AvatarBorder"), SelectedAvatarBorder, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { SelectedAvatarBorder = (SelectedAvatarBorder + 1) % 3; GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("AvatarBorder"), SelectedAvatarBorder, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); return FReply::Handled(); }))]
						]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Stats ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						BuildProfileStatsPanel()
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Leaderboards ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false)
						.CutSize(14.0f)
						.BorderWidth(1.1f)
						.Padding(FMargin(28.0f, 24.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR RANKED PLAYLISTS"))).Font(UiFont(16, true)).ColorAndOpacity(Paper)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 18.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("PLACEMENT AND MMR PROGRESS ACROSS EACH TEAM SIZE"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SUniformGridPanel).SlotPadding(FMargin(7.0f))
								+ SUniformGridPanel::Slot(0, 0)[MakeRankCard(1, TEXT("1V1 DUEL"))]
								+ SUniformGridPanel::Slot(1, 0)[MakeRankCard(2, TEXT("2V2 DOUBLES"))]
								+ SUniformGridPanel::Slot(2, 0)[MakeRankCard(3, TEXT("3V3 CHAOS"))]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(7.0f, 18.0f, 7.0f, 0.0f)
							[
								SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(Hairline).CutSize(7.0f).BorderWidth(0.8f).Padding(FMargin(20.0f, 14.0f))
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("GLOBAL PLAYER TABLES WILL APPEAR HERE WHEN THE LIVE LEADERBOARD SERVICE IS CONNECTED."))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
								]
							]
						]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::MatchHistory ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false)
						.CutSize(14.0f)
						.BorderWidth(1.1f)
						.Padding(FMargin(28.0f, 24.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("RECENT MATCHES"))).Font(UiFont(16, true)).ColorAndOpacity(Paper)]
								+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("LAST 8  //  STORED LOCALLY"))).Font(UiFont(9, true)).ColorAndOpacity(Brand)]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
							[
								SNew(SBox)
								.Visibility_Lambda([this]()
								{
									const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
									return !Instance || Instance->GetRecentMatches().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
								})
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("NO COMPLETED MATCHES YET  //  FINISH A SERIES TO START YOUR HISTORY"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)
								]
							]
							+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[SNew(SScrollBox) + SScrollBox::Slot()[HistoryRows]]
						]
					]
				]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(160.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CloseProfile(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu; return FReply::Handled(); }))]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE DATA SAVES AUTOMATICALLY"))).Font(UiFont(10)).ColorAndOpacity(Muted)]
			]
			]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenuPartyMember(const int32 PartySlot)
{
	return SNew(SBox)
		.WidthOverride(128.0f)
		.HeightOverride(44.0f)
		.Visibility_Lambda([this, PartySlot]()
		{
			return IsDisplayedPartyActive() && GetDisplayedPartyMember(PartySlot)
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor::Transparent)
			.Padding(0.0f)
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& Event)
			{
				if (Event.GetEffectingButton() != EKeys::RightMouseButton) return FReply::Unhandled();
				bSocialPanelOpen = true;
				bShowingRecentPlayers = false;
				bShowingOnlineFriends = false;
				bSocialPartyExpanded = true;
				return FReply::Handled();
			})
			[
			SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.OnClicked_Lambda([this]()
			{
				bSocialPanelOpen = true;
				bShowingRecentPlayers = false;
				bShowingOnlineFriends = false;
				bSocialPartyExpanded = true;
				return FReply::Handled();
			})
			[
			SNew(SFlickMainMenuPanel)
			.BackgroundColor(FLinearColor::FromSRGBColor(FColor(14, 23, 25, 248)))
			.AccentColor_Lambda([this, PartySlot]()
			{
				const AFlickPlayerState* Member = IsDisplayedPartyActive()
					? GetDisplayedPartyMember(PartySlot) : nullptr;
				return Member && Member->IsPartyLeader()
					? Brand
					: Cyan.CopyWithNewOpacity(0.5f);
			})
			.CutSize(7.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(8.0f, 5.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 8.0f, 2.0f)
				[
					SNew(SBox).WidthOverride(3.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 9.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("P%d"), PartySlot + 1)))
					.Font(UiFont(9, true)).ColorAndOpacity(Brand)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PartySlot]()
						{
							if (IsDisplayedPartyActive())
							{
								const AFlickPlayerState* Member = GetDisplayedPartyMember(PartySlot);
								return FText::FromString(Member ? Member->GetPlayerName() : FString());
							}
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER"));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PartySlot]()
						{
							const AFlickPlayerState* Member = IsDisplayedPartyActive()
								? GetDisplayedPartyMember(PartySlot) : nullptr;
							return FText::FromString(Member && Member->IsPartyLeader() ? TEXT("PARTY LEADER") : TEXT("IN PARTY"));
						})
						.Font(UiFont(7, true)).ColorAndOpacity(Cyan.CopyWithNewOpacity(0.82f))
					]
				]
			]
			]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildSocialPanel()
{
	auto MakeSocialTab = [this](const FString& Label, const int32 Tab) -> TSharedRef<SWidget>
	{
		return SNew(SBox).HeightOverride(38.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor_Lambda([this, Tab]()
				{
					const bool bSelected = Tab == 2 ? bShowingRecentPlayers
						: !bShowingRecentPlayers && bShowingOnlineFriends == (Tab == 1);
					return bSelected
						? FLinearColor::FromSRGBColor(FColor(22, 39, 39, 244))
						: FLinearColor::FromSRGBColor(FColor(12, 22, 25, 226));
				})
				.AccentColor_Lambda([this, Tab]()
				{
					const bool bSelected = Tab == 2 ? bShowingRecentPlayers
						: !bShowingRecentPlayers && bShowingOnlineFriends == (Tab == 1);
					return bSelected ? Brand : Hairline;
				})
				.UseAccentForOutline(false)
				.CutSize(6.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(1.0f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this, Tab]()
					{
						bShowingRecentPlayers = Tab == 2;
						bShowingOnlineFriends = Tab == 1;
						bSocialInGameExpanded = true;
						bSocialOnlineExpanded = true;
						bSocialOfflineExpanded = Tab == 0;
						RebuildSocialPlayerList();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this, Label, Tab]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							int32 Count = 0;
							if (Sessions)
							{
								if (Tab == 2) Count = Sessions->GetRecentPlayers().Num();
								else if (Tab == 0) Count = Sessions->GetFriends().Num();
								else for (const FFlickSocialPlayerEntry& Friend : Sessions->GetFriends()) Count += Friend.bOnline ? 1 : 0;
							}
							return FText::FromString(FString::Printf(TEXT("%s  %d"), *Label, Count));
						})
						.Font(UiFont(11, true))
						.ColorAndOpacity_Lambda([this, Tab]()
						{
							const bool bSelected = Tab == 2 ? bShowingRecentPlayers
								: !bShowingRecentPlayers && bShowingOnlineFriends == (Tab == 1);
							return FSlateColor(bSelected ? Brand : Paper);
						})
					]
				]
			];
	};
	auto MakeQueueButton = [this](const int32 PlayersPerTeam) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(44.0f)
			.IsEnabled_Lambda([this, PlayersPerTeam]()
			{
				return GameMode.IsValid() && GameMode->CanQueuePartyForMatchmaking(PlayersPerTeam);
			})
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(PanelRaised)
				.AccentColor(Hairline)
				.UseAccentForOutline(false)
				.CutSize(7.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(1.0f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this, PlayersPerTeam]()
					{
						if (GameMode.IsValid()) GameMode->QueuePartyForMatchmaking(PlayersPerTeam);
						return FReply::Handled();
					})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(9, true)).ColorAndOpacity(Paper)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("QUEUE %dV%d"), PlayersPerTeam, PlayersPerTeam)))
							.Font(UiFont(11, true))
							.ColorAndOpacity(Paper)
						]
					]
				]
			];
	};

	return SNew(SFlickMainMenuPanel)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(14, 23, 25, 246)))
		.AccentColor(Brand)
		.CutSize(14.0f)
		.BorderWidth(1.0f)
		.Padding(FMargin(20.0f, 20.0f, 20.0f, 17.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(43.0f).HeightOverride(43.0f)
						[SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Brand)]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 3.0f, 15.0f, 3.0f)
					[
						SNew(SBox).WidthOverride(1.0f)
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.72f))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(TEXT("SOCIAL"));
						})
						.Font(DisplayFont(24)).ColorAndOpacity(Paper)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
								const FString Service = Sessions ? Sessions->GetOnlineServiceName().ToUpper() : TEXT("LOCAL");
							const FString PlayerName = Sessions ? Sessions->GetLocalDisplayName().ToUpper() : TEXT("LOCAL PLAYER");
							return FText::FromString(FString::Printf(TEXT("%s  //  %s ONLINE"), *PlayerName, *Service));
							})
							.Font(UiFont(8, true)).ColorAndOpacity(Brand)
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(44.0f).HeightOverride(44.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false)
						.CutSize(7.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(SButton)
							.ButtonStyle(&TransparentButtonStyle)
							.ContentPadding(FMargin(0.0f))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this]()
							{
								bSocialPanelOpen = false;
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("X"))).Font(UiFont(16, true)).ColorAndOpacity(Paper)
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
				[
					MakeSocialTab(TEXT("ALL"), 0)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f)
				[
					MakeSocialTab(TEXT("ONLINE"), 1)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeSocialTab(TEXT("RECENT"), 2)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)
			[
				SNew(SBox).HeightOverride(31.0f)
				.Visibility_Lambda([this]() { return bShowingRecentPlayers ? EVisibility::Collapsed : EVisibility::Visible; })
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]()
					{
						bSocialPartyExpanded = !bSocialPartyExpanded;
						return FReply::Handled();
					})
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false).CutSize(3.0f).BorderWidth(0.75f).Padding(FMargin(9.0f, 4.0f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(bSocialPartyExpanded ? TEXT("v") : TEXT(">")); })
								.Font(UiFont(8, true)).ColorAndOpacity(Brand)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const int32 Members = IsDisplayedPartyActive() ? GetDisplayedPartyMemberCount() : 1;
									return FText::FromString(FString::Printf(TEXT("PARTY  (%d)"), Members));
								})
								.Font(UiFont(8, true)).ColorAndOpacity(Paper)
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("MAIN MENU"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(0)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(1)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(2)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(3)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(4)]
			+ SVerticalBox::Slot().AutoHeight()[BuildPartyMemberRow(5)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 13.0f, 0.0f, 5.0f)
			[
				SNew(SBox).HeightOverride(29.0f)
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]()
					{
						if (bShowingRecentPlayers)
						{
							bSocialRecentExpanded = !bSocialRecentExpanded;
						}
						else
						{
							bSocialFriendsExpanded = !bSocialFriendsExpanded;
						}
						return FReply::Handled();
					})
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false).CutSize(3.0f).BorderWidth(0.7f).Padding(FMargin(10.0f, 4.0f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
								if (bShowingRecentPlayers)
								{
									const int32 Count = Sessions ? Sessions->GetRecentPlayers().Num() : 0;
									return FText::FromString(FString::Printf(
										TEXT("%s  RECENT PLAYERS  (%d)"),
										bSocialRecentExpanded ? TEXT("v") : TEXT(">"), Count));
								}
								int32 OnlineCount = 0;
								if (Sessions)
								{
									for (const FFlickSocialPlayerEntry& Friend : Sessions->GetFriends())
									{
										OnlineCount += Friend.bOnline ? 1 : 0;
									}
								}
								return FText::FromString(FString::Printf(
									TEXT("%s  PLATFORM FRIENDS  //  %d ONLINE"),
									bSocialFriendsExpanded ? TEXT("v") : TEXT(">"), OnlineCount));
							})
							.Font(UiFont(8, true)).ColorAndOpacity(Paper)
						]
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SScrollBox)
				.ScrollBarAlwaysVisible(false)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 5.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(58.0f)
						.Visibility_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							bool bEmpty = !Sessions;
							if (Sessions)
							{
								bEmpty = bShowingRecentPlayers ? Sessions->GetRecentPlayers().IsEmpty() : Sessions->GetFriends().IsEmpty();
								if (bShowingOnlineFriends && !bShowingRecentPlayers)
								{
									bEmpty = !Sessions->GetFriends().ContainsByPredicate([](const FFlickSocialPlayerEntry& Friend) { return Friend.bOnline; });
								}
							}
							const bool bExpanded = bShowingRecentPlayers ? bSocialRecentExpanded : bSocialFriendsExpanded;
							return bEmpty && bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline.CopyWithNewOpacity(0.42f))
							.CutSize(5.0f).BorderWidth(0.8f).Padding(FMargin(14.0f, 10.0f))
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
								return FText::FromString(bShowingRecentPlayers
									? TEXT("NO RECENT PLAYERS  //  PLAY ONLINE TO BUILD YOUR LIST")
									: bShowingOnlineFriends
										? TEXT("NO STEAM FRIENDS ARE CURRENTLY ONLINE")
										: TEXT("NO PLATFORM FRIENDS FOUND  //  TRY REFRESH"));
								})
								.Font(UiFont(8, true)).ColorAndOpacity(Muted).AutoWrapText(true)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(SocialPlayerList, SVerticalBox)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SBox).HeightOverride(1.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.82f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SButton)
					.Visibility(EVisibility::Collapsed)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
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
								? TEXT("QUEUE TYPE  /  RANKED") : TEXT("QUEUE TYPE  /  CASUAL"));
						})
						.Font(UiFont(10, true))
						.ColorAndOpacity_Lambda([this]()
						{
							return FSlateColor(GameMode.IsValid() && GameMode->IsRankedQueueSelected() ? Orange : Cyan);
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					.Visibility(EVisibility::Collapsed)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						MakeQueueButton(2)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						MakeQueueButton(3)
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						return FText::FromString(Sessions ? Sessions->GetStatusMessage() : TEXT("STEAM SOCIAL UNAVAILABLE"));
					})
					.Font(UiFont(8, true)).ColorAndOpacity(Muted).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(44.0f)
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline)
							.CutSize(6.0f)
							.BorderWidth(1.0f)
							.Padding(FMargin(1.0f))
							[
								SNew(SButton)
								.ButtonStyle(&TransparentButtonStyle)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.OnClicked_Lambda([this]()
								{
									if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->RefreshFriends();
									return FReply::Handled();
								})
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("\u21BB"))).Font(UiFont(17, true)).ColorAndOpacity(Paper)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("REFRESH"))).Font(UiFont(11, true)).ColorAndOpacity(Paper)
									]
								]
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(150.0f)
						.Visibility_Lambda([this]() { return IsDisplayedPartyActive() && !GameMode.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("LEAVE PARTY"), FOnClicked::CreateLambda([this]()
							{
								if (PlayerController.IsValid()) PlayerController->LeaveNetworkSession();
								return FReply::Handled();
							}), false, true, 44.0f)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(130.0f)
						.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsPartySession() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("DISBAND"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->DisbandParty();
								return FReply::Handled();
							}), false, true, 44.0f)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPartyMemberRow(const int32 PartySlot)
{
	return SNew(SBox).HeightOverride(48.0f)
		.Visibility_Lambda([this, PartySlot]()
		{
			if (bShowingRecentPlayers || !bSocialPartyExpanded)
			{
				return EVisibility::Collapsed;
			}
			if (IsDisplayedPartyActive())
			{
				return GetDisplayedPartyMember(PartySlot) ? EVisibility::Visible : EVisibility::Collapsed;
			}
			return EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Hairline)
			.CutSize(5.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
					[
						SNew(SFlickRoundPip)
						.Color_Lambda([this, PartySlot]()
						{
							const bool bOccupied = IsDisplayedPartyActive()
								? GetDisplayedPartyMember(PartySlot) != nullptr : PartySlot == 0;
							return bOccupied ? Brand : Muted;
						})
						.Filled_Lambda([this, PartySlot]()
						{
							return IsDisplayedPartyActive()
								? GetDisplayedPartyMember(PartySlot) != nullptr : PartySlot == 0;
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(44.0f).HeightOverride(36.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.CutSize(6.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%02d"), PartySlot + 1)))
							.Font(UiFont(10, true))
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(Brand)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text_Lambda([this, PartySlot]()
							{
								if (IsDisplayedPartyActive())
								{
									const AFlickPlayerState* Member = GetDisplayedPartyMember(PartySlot);
									return FText::FromString(Member ? Member->GetPlayerName() : TEXT("OPEN PARTY SLOT"));
								}
								if (PartySlot == 0)
								{
									const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
									return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER"));
								}
								return FText::FromString(TEXT("OPEN PARTY SLOT"));
							}).Font(UiFont(11, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("*")))
							.Font(UiFont(10, true))
							.ColorAndOpacity(Orange)
							.Visibility_Lambda([this, PartySlot]()
							{
								const AFlickPlayerState* Member = IsDisplayedPartyActive() ? GetDisplayedPartyMember(PartySlot) : nullptr;
								return (Member && Member->IsPartyLeader()) || (!IsDisplayedPartyActive() && PartySlot == 0)
									? EVisibility::Visible : EVisibility::Collapsed;
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, PartySlot]()
						{
							const AFlickPlayerState* Member = IsDisplayedPartyActive() ? GetDisplayedPartyMember(PartySlot) : nullptr;
							return FText::FromString((Member && Member->IsPartyLeader()) || (!IsDisplayedPartyActive() && PartySlot == 0)
								? TEXT("PARTY LEADER") : Member ? TEXT("IN PARTY") : TEXT("INVITE A FRIEND"));
						}).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&CompactMenuButtonStyle)
					.Visibility_Lambda([this, PartySlot]()
					{
						const AFlickPlayerState* LocalState = PlayerController.IsValid()
							? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr;
						const AFlickPlayerState* Target = GetDisplayedPartyMember(PartySlot);
						return LocalState && LocalState->IsPartyLeader() && Target && Target != LocalState
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.OnClicked_Lambda([this, PartySlot]()
					{
						if (PlayerController.IsValid()) PlayerController->RequestPromotePartyMember(PartySlot);
						return FReply::Handled();
					})
					[SNew(STextBlock).Text(FText::FromString(TEXT("PROMOTE"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton).ButtonStyle(&DangerButtonStyle)
					.Visibility_Lambda([this, PartySlot]()
					{
						const AFlickPlayerState* LocalState = PlayerController.IsValid()
							? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr;
						const AFlickPlayerState* Target = GetDisplayedPartyMember(PartySlot);
						return LocalState && LocalState->IsPartyLeader() && Target && Target != LocalState
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.OnClicked_Lambda([this, PartySlot]()
					{
						if (PlayerController.IsValid()) PlayerController->RequestRemovePartyMember(PartySlot);
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("REMOVE"))).Font(UiFont(8, true)).ColorAndOpacity(Orange)
					]
				]
			]
		];
}

void SFlickGameLayer::RebuildSocialPlayerList()
{
	if (!SocialPlayerList.IsValid())
	{
		return;
	}
	SocialPlayerList->ClearChildren();
	const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
	CachedSocialFriendCount = Sessions ? Sessions->GetFriends().Num() : 0;
	CachedSocialRecentCount = Sessions ? Sessions->GetRecentPlayers().Num() : 0;
	for (int32 FriendIndex = 0; !bShowingRecentPlayers && FriendIndex < CachedSocialFriendCount; ++FriendIndex)
	{
		if (bShowingOnlineFriends && !Sessions->GetFriends()[FriendIndex].bOnline) continue;
		SocialPlayerList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)
		[
			BuildSocialFriendRow(FriendIndex)
		];
	}
	for (int32 RecentIndex = 0; bShowingRecentPlayers && RecentIndex < CachedSocialRecentCount; ++RecentIndex)
	{
		SocialPlayerList->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)
		[
			BuildRecentPlayerRow(RecentIndex)
		];
	}
}

TSharedRef<SWidget> SFlickGameLayer::BuildSocialFriendRow(const int32 FriendIndex)
{
	return SNew(SVerticalBox)
		.Visibility_Lambda([this, FriendIndex]()
		{
			const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
			if (bShowingRecentPlayers || !bSocialFriendsExpanded || !Sessions
				|| !Sessions->GetFriends().IsValidIndex(FriendIndex))
			{
				return EVisibility::Collapsed;
			}
			return !bShowingOnlineFriends || Sessions->GetFriends()[FriendIndex].bOnline
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(23.0f)
			.Visibility_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return EVisibility::Collapsed;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				const int32 Group = Friend.bPlayingFlick ? 0 : Friend.bOnline ? 1 : 2;
				if (FriendIndex == 0) return EVisibility::Visible;
				const FFlickSocialPlayerEntry& Previous = Sessions->GetFriends()[FriendIndex - 1];
				const int32 PreviousGroup = Previous.bPlayingFlick ? 0 : Previous.bOnline ? 1 : 2;
				return Group != PreviousGroup ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this, FriendIndex]()
				{
					const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
					if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return FReply::Handled();
					const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
					if (Friend.bPlayingFlick) bSocialInGameExpanded = !bSocialInGameExpanded;
					else if (Friend.bOnline) bSocialOnlineExpanded = !bSocialOnlineExpanded;
					else bSocialOfflineExpanded = !bSocialOfflineExpanded;
					return FReply::Handled();
				})
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(PanelRaised)
					.Padding(FMargin(9.0f, 3.0f))
					[
						SNew(STextBlock)
						.Text_Lambda([this, FriendIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return FText::GetEmpty();
							const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
							const bool bPlaying = Friend.bPlayingFlick;
							const bool bOnline = Friend.bOnline;
							const bool bExpanded = bPlaying ? bSocialInGameExpanded : bOnline ? bSocialOnlineExpanded : bSocialOfflineExpanded;
							int32 Count = 0;
							for (const FFlickSocialPlayerEntry& Candidate : Sessions->GetFriends())
							{
								Count += bPlaying
									? (Candidate.bPlayingFlick ? 1 : 0)
									: bOnline
										? (Candidate.bOnline && !Candidate.bPlayingFlick ? 1 : 0)
										: (!Candidate.bOnline ? 1 : 0);
							}
							const TCHAR* Label = bPlaying ? TEXT("IN-GAME") : bOnline ? TEXT("ONLINE") : TEXT("OFFLINE");
							return FText::FromString(FString::Printf(
								TEXT("%s  %s  (%d)"), bExpanded ? TEXT("v") : TEXT(">"), Label, Count));
						})
						.Font(UiFont(8, true)).ColorAndOpacity(Paper)
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(56.0f)
			.Visibility_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return EVisibility::Collapsed;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				const bool bExpanded = Friend.bPlayingFlick
					? bSocialInGameExpanded
					: Friend.bOnline ? bSocialOnlineExpanded : bSocialOfflineExpanded;
				return bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bPlayingFlick
					? PanelRaised : Panel;
			})
			.AccentColor_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return Hairline;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				return Friend.bPlayingFlick
					? Brand
					: Friend.bOnline ? Muted : Hairline.CopyWithNewOpacity(0.46f);
			})
			.UseAccentForOutline(false)
			.CutSize(6.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 6.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(38.0f).HeightOverride(38.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline)
							.CutSize(5.0f).BorderWidth(0.8f).Padding(FMargin(3.0f))
							[
								SNew(SOverlay)
								+ SOverlay::Slot()
								[
									SNew(SImage)
									.Image_Lambda([this, FriendIndex]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) ? Sessions->GetAvatarBrush(Sessions->GetFriends()[FriendIndex].UserId) : nullptr; })
									.Visibility_Lambda([this, FriendIndex]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetAvatarBrush(Sessions->GetFriends()[FriendIndex].UserId) ? EVisibility::Visible : EVisibility::Collapsed; })
								]
								+ SOverlay::Slot().Padding(4.0f)
								[
									SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Muted)
									.Visibility_Lambda([this, FriendIndex]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetAvatarBrush(Sessions->GetFriends()[FriendIndex].UserId) ? EVisibility::Collapsed : EVisibility::Visible; })
								]
							]
						]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, -2.0f, -2.0f)
						[
							SNew(SBox).WidthOverride(11.0f).HeightOverride(11.0f)
							[
								SNew(SFlickRoundPip)
								.Color_Lambda([this, FriendIndex]()
								{
									const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
									return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline
										? Brand : Muted;
								})
								.Filled_Lambda([this, FriendIndex]()
								{
									const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
									return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline;
								})
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, FriendIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) ? Sessions->GetFriends()[FriendIndex].DisplayName : FString());
						}).Font(UiFont(11, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, FriendIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) ? Sessions->GetFriends()[FriendIndex].Status : FString());
						}).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(84.0f).HeightOverride(34.0f)
					.Visibility_Lambda([this, FriendIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.IsEnabled_Lambda([this, FriendIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						const bool bPartyHasRoom = !GameMode.IsValid() || !GameMode->IsPartySession() || GameMode->GetPartyMemberCount() < FlickMaximumPartyMembers;
						return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline && bPartyHasRoom;
					})
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.CutSize(5.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(SButton)
							.ButtonStyle(&TransparentButtonStyle)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this, FriendIndex]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->InviteFriendToParty(FriendIndex);
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("INVITE"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)
							]
						]
					]
				]
			]
		]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildRecentPlayerRow(const int32 RecentIndex)
{
	return SNew(SBox).HeightOverride(56.0f)
		.Visibility_Lambda([this, RecentIndex]()
		{
			const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
			return bShowingRecentPlayers && bSocialRecentExpanded && Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex)
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Hairline.CopyWithNewOpacity(0.62f))
			.UseAccentForOutline(false)
			.CutSize(6.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 6.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(38.0f).HeightOverride(38.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline.CopyWithNewOpacity(0.7f))
						.CutSize(5.0f).BorderWidth(0.8f).Padding(FMargin(3.0f))
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SImage)
								.Image_Lambda([this, RecentIndex]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex) ? Sessions->GetAvatarBrush(Sessions->GetRecentPlayers()[RecentIndex].UserId) : nullptr; })
								.Visibility_Lambda([this, RecentIndex]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex) && Sessions->GetAvatarBrush(Sessions->GetRecentPlayers()[RecentIndex].UserId) ? EVisibility::Visible : EVisibility::Collapsed; })
							]
							+ SOverlay::Slot().Padding(4.0f)
							[
								SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Muted)
								.Visibility_Lambda([this, RecentIndex]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex) && Sessions->GetAvatarBrush(Sessions->GetRecentPlayers()[RecentIndex].UserId) ? EVisibility::Collapsed : EVisibility::Visible; })
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, RecentIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
							return FText::FromString(Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex) ? Sessions->GetRecentPlayers()[RecentIndex].DisplayName : FString());
						}).Font(UiFont(11, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("RECENTLY MET IN FLICK"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(84.0f).HeightOverride(34.0f)
					.IsEnabled_Lambda([this]() { return !GameMode.IsValid() || !GameMode->IsPartySession() || GameMode->GetPartyMemberCount() < FlickMaximumPartyMembers; })
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.CutSize(5.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(SButton)
							.ButtonStyle(&TransparentButtonStyle)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this, RecentIndex]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->InviteRecentPlayerToParty(RecentIndex);
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("INVITE"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildItemShop()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.002f, 0.006f, 0.011f, 0.9f))
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(116.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.008f, 0.016f, 0.94f))
				.AccentColor(Orange.CopyWithNewOpacity(0.76f))
				.CutSize(10.0f)
				.BorderWidth(1.1f)
				.Padding(FMargin(44.0f, 22.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(6.0f).HeightOverride(55.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Orange)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("ITEM SHOP"))).Font(UiFont(36, true)).ColorAndOpacity(FLinearColor::White)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("COSMETIC SHOWCASE"))).Font(UiFont(10, true)).ColorAndOpacity(Orange)]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK CREDITS"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString(TEXT("0"))).Font(UiFont(24, true)).ColorAndOpacity(FLinearColor::White)]
					]
				]
			]
		]
		+ SOverlay::Slot().Padding(44.0f, 142.0f, 44.0f, 104.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("FEATURED"))).Font(UiFont(18, true)).ColorAndOpacity(FLinearColor::White)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("STORE PREVIEW  /  COMING SOON"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 13.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(8.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)[BuildShopItemCard(TEXT("NEON CIRCUIT"), TEXT("PUCK FINISH"), TEXT("N"), Cyan)]
				+ SUniformGridPanel::Slot(1, 0)[BuildShopItemCard(TEXT("ARENA SIGNAL"), TEXT("RIM COLOR"), TEXT("A"), Orange)]
				+ SUniformGridPanel::Slot(2, 0)[BuildShopItemCard(TEXT("FOUNDERS MARK"), TEXT("PLAYER BANNER"), TEXT("F"), FLinearColor(0.72f, 0.38f, 1.0f, 1.0f))]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(82.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.93f))
				.AccentColor(Orange.CopyWithNewOpacity(0.58f))
				.CutSize(8.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(34.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(160.0f)
						[
							MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CloseItemShop(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu;
								return FReply::Handled();
							}), false, false, 50.0f)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("PURCHASES AND INVENTORY WILL ARRIVE IN A LATER MILESTONE"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildShopItemCard(
	const FString& Name,
	const FString& Type,
	const FString& Mark,
	const FLinearColor& Accent)
{
	TSharedRef<SWidget> Preview = SNew(SBox);
	if (Mark == TEXT("N"))
	{
		Preview = SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(205.0f).HeightOverride(205.0f)
				[
					SNew(SFlickPuckDisc).TeamColor(FLinearColor(0.025f, 0.08f, 0.12f, 1.0f)).AccentColor(Accent).Selected(true).RadiusScale(1.16f)
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("F"))).Font(UiFont(28, true)).ColorAndOpacity(FLinearColor::White)
			];
	}
	else if (Mark == TEXT("A"))
	{
		Preview = SNew(SOverlay)
			+ SOverlay::Slot().Padding(34.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.01f, 0.018f, 0.025f, 1.0f))
				.AccentColor(Accent)
				.CutSize(24.0f)
				.BorderWidth(4.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.025f, 0.04f, 0.052f, 1.0f))
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("ARENA RIM"))).Font(UiFont(14, true)).ColorAndOpacity(Accent)
			];
	}
	else
	{
		Preview = SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(260.0f).HeightOverride(105.0f)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(FMath::Lerp(Panel, Accent, 0.14f))
					.AccentColor(Accent)
					.CutSize(17.0f)
					.BorderWidth(2.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("F"))).Font(UiFont(54, true)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor::White)
					]
				]
			];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.008f, 0.016f, 0.027f, 0.98f))
		.AccentColor(Accent)
		.CutSize(16.0f)
		.BorderWidth(2.0f)
		.Padding(FMargin(18.0f))
		[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(FMath::Lerp(FLinearColor(0.008f, 0.016f, 0.027f, 1.0f), Accent, 0.16f))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[Preview]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(10.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("PREVIEW"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 15.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Type)).Font(UiFont(9, true)).ColorAndOpacity(Accent)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Name)).Font(UiFont(19, true)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(PanelRaised)
					.Padding(FMargin(10.0f, 7.0f))
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("NOT YET AVAILABLE"))).Font(UiFont(8, true)).Justification(ETextJustify::Center).ColorAndOpacity(Muted)
					]
				]
		];
}
