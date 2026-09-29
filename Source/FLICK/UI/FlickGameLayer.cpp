#include "UI/FlickGameLayer.h"
#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickAimArrowWidget.h"
#include "Online/FlickMatchmakingCoordinatorSubsystem.h"

SFlickGameLayer::SFlickGameLayer()
{
	MenuButtonStyle = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Panel, 3.0f, Hairline, 1.0f))
		.SetHovered(FSlateRoundedBoxBrush(PanelRaised, 3.0f, Brand, 1.0f))
		.SetPressed(FSlateRoundedBoxBrush(Ink, 3.0f, Cyan, 1.0f))
		.SetNormalPadding(FMargin(21.0f, 10.0f))
		.SetPressedPadding(FMargin(24.0f, 11.0f, 18.0f, 9.0f));
	PrimaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.085f, 0.14f, 0.02f, 1.0f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.15f, 0.24f, 0.025f, 1.0f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.0f, 0.78f, 1.0f, 1.0f)))
		.SetNormalPadding(FMargin(21.0f, 10.0f))
		.SetPressedPadding(FMargin(24.0f, 11.0f, 18.0f, 9.0f));
	DangerButtonStyle = FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.1f, 0.018f, 0.014f, 0.96f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.58f, 0.055f, 0.018f, 1.0f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.92f, 0.1f, 0.02f, 1.0f)))
		.SetNormalPadding(FMargin(21.0f, 10.0f))
		.SetPressedPadding(FMargin(24.0f, 11.0f, 18.0f, 9.0f));
	CompactMenuButtonStyle = MenuButtonStyle;
	CompactMenuButtonStyle
		.SetNormalPadding(FMargin(8.0f, 4.0f))
		.SetPressedPadding(FMargin(10.0f, 5.0f, 6.0f, 3.0f));
	CompactPrimaryButtonStyle = PrimaryButtonStyle;
	CompactPrimaryButtonStyle
		.SetNormalPadding(FMargin(8.0f, 4.0f))
		.SetPressedPadding(FMargin(10.0f, 5.0f, 6.0f, 3.0f));
	CompactDangerButtonStyle = DangerButtonStyle;
	CompactDangerButtonStyle
		.SetNormalPadding(FMargin(8.0f, 4.0f))
		.SetPressedPadding(FMargin(10.0f, 5.0f, 6.0f, 3.0f));
	TransparentButtonStyle = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f));
	ToggleStyle = FCheckBoxStyle()
		.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
		.SetUncheckedImage(FSlateRoundedBoxBrush(Ink, 3.0f, Hairline, 1.0f))
		.SetUncheckedHoveredImage(FSlateRoundedBoxBrush(PanelRaised, 3.0f, Brand, 1.0f))
		.SetUncheckedPressedImage(FSlateRoundedBoxBrush(PanelRaised, 3.0f, Cyan, 1.0f))
		.SetCheckedImage(FSlateRoundedBoxBrush(Brand, 3.0f))
		.SetCheckedHoveredImage(FSlateRoundedBoxBrush(FMath::Lerp(Brand, Paper, 0.15f), 3.0f))
		.SetCheckedPressedImage(FSlateRoundedBoxBrush(Ice, 3.0f))
		.SetPadding(FMargin(0.0f));
	// Own these styles for the lifetime of the widget; Slate keeps pointers.
	DropdownStyle = FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox");
	DropdownStyle.ComboButtonStyle.ButtonStyle = CompactMenuButtonStyle;
	DropdownStyle.ComboButtonStyle.SetMenuBorderBrush(FSlateRoundedBoxBrush(Ink, 3.0f, Hairline, 1.0f));
	DropdownStyle.ComboButtonStyle.SetMenuBorderPadding(FMargin(4.0f));
	DropdownRowStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");
	DropdownRowStyle.SetEvenRowBackgroundBrush(FSlateColorBrush(Ink))
		.SetOddRowBackgroundBrush(FSlateColorBrush(Ink))
		.SetEvenRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(PanelRaised, 2.0f, Cyan, 1.0f))
		.SetOddRowBackgroundHoveredBrush(FSlateRoundedBoxBrush(PanelRaised, 2.0f, Cyan, 1.0f))
		.SetActiveBrush(FSlateRoundedBoxBrush(PanelRaised, 2.0f, Brand, 1.0f))
		.SetActiveHoveredBrush(FSlateRoundedBoxBrush(PanelRaised, 2.0f, Brand, 1.0f))
		.SetInactiveBrush(FSlateRoundedBoxBrush(PanelRaised, 2.0f, Hairline, 1.0f))
		.SetInactiveHoveredBrush(FSlateRoundedBoxBrush(PanelRaised, 2.0f, Cyan, 1.0f));
	SliderStyle = FSliderStyle()
		.SetNormalBarImage(FSlateColorBrush(FLinearColor(0.05f, 0.1f, 0.135f, 1.0f)))
		.SetHoveredBarImage(FSlateColorBrush(FLinearColor(0.07f, 0.18f, 0.24f, 1.0f)))
		.SetNormalThumbImage(FSlateColorBrush(Paper))
		.SetHoveredThumbImage(FSlateColorBrush(Brand))
		.SetBarThickness(5.0f);
	SliderStyle.NormalThumbImage.ImageSize = FVector2D(12.0f, 22.0f);
	SliderStyle.HoveredThumbImage.ImageSize = FVector2D(12.0f, 22.0f);
	SliderStyle.SetNormalBarImage(FSlateNoResource()).SetHoveredBarImage(FSlateNoResource());
	ShotClockBarStyle = FProgressBarStyle()
		.SetBackgroundImage(FSlateRoundedBoxBrush(Track, 2.5f))
		.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White, 2.5f))
		.SetMarqueeImage(FSlateNoResource());
}

void SFlickGameLayer::Construct(const FArguments& InArgs)
{
	OwnerHud = InArgs._OwnerHud;
	GameMode = InArgs._GameMode;
	PlayerController = InArgs._PlayerController;
	GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("BannerStyle"), SelectedBannerStyle, GGameUserSettingsIni);
	GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("BannerTag"), SelectedBannerTag, GGameUserSettingsIni);
	GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), TEXT("AvatarBorder"), SelectedAvatarBorder, GGameUserSettingsIni);
	SelectedBannerStyle = FMath::Clamp(SelectedBannerStyle, 0, FlickCosmeticCatalog::GetItems(0).Num() - 1);
	SelectedBannerTag = FMath::Clamp(SelectedBannerTag, 0, FlickCosmeticCatalog::GetItems(1).Num() - 1);
	SelectedAvatarBorder = FMath::Clamp(SelectedAvatarBorder, 0, FlickCosmeticCatalog::GetItems(2).Num() - 1);
	SelectedPuckSkins.SetNumZeroed(FlickPieceArchetypeRules::ArchetypeCount);
	for (int32 Category = FlickCosmeticCatalog::PuckCategoryStart; Category < FlickCosmeticCatalog::CategoryCount; ++Category)
	{
		int32& Skin = SelectedPuckSkins[Category - FlickCosmeticCatalog::PuckCategoryStart];
		GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), *FlickCosmeticCatalog::GetConfigKey(Category), Skin, GGameUserSettingsIni);
		Skin = FMath::Clamp(Skin, 0, FlickCosmeticCatalog::GetItems(Category).Num() - 1);
	}
	if (const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
		? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
		: nullptr)
	{
		bStartupOverlayVisible = FlickGameInstance->ShouldShowStartupPresentation()
			&& GameMode.IsValid()
			&& GameMode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu;
	}

#if !UE_BUILD_SHIPPING
	bInvitePromptPreview = FParse::Param(FCommandLine::Get(), TEXT("FlickPartyInvitePreview"));
	bPartyTrayPreview = FParse::Param(FCommandLine::Get(), TEXT("FlickPartyTrayPreview"));
	bReplayOverlayPreview = FParse::Param(FCommandLine::Get(), TEXT("FlickReplayPreview"));
	bFpsPreview = FParse::Param(FCommandLine::Get(), TEXT("FlickFpsPreview"));
	bChallengesOpen = FParse::Param(FCommandLine::Get(), TEXT("FlickChallengesPreview"));
	bSocialPanelOpen = FParse::Param(FCommandLine::Get(), TEXT("FlickSocialPreview"));
	int32 SettingsPreviewTab = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickSettingsTab="), SettingsPreviewTab))
	{
		SelectedSettingsTab = static_cast<EFlickSettingsTab>(FMath::Clamp(SettingsPreviewTab, 0, static_cast<int32>(EFlickSettingsTab::Lighting)));
	}
	bLightingPreview = SelectedSettingsTab == EFlickSettingsTab::Lighting
		&& FParse::Param(FCommandLine::Get(), TEXT("FlickLightingPreview"));
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickProfileCustomizePreview")))
	{
		SelectedProfileTab = EFlickProfileTab::Customization;
		int32 PreviewCategory = 0;
		if (FParse::Value(FCommandLine::Get(), TEXT("FlickLockerCategory="), PreviewCategory))
		{
			SelectedLockerCategory = FMath::Clamp(PreviewCategory, 0, FlickCosmeticCatalog::CategoryCount - 1);
		}
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickPlayFormatPreview")))
	{
		SelectedPlayPlaylist = EFlickPlayPlaylist::Casual;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickLoadoutComparePreview")))
	{
		Player1HoveredLoadoutArchetype = EFlickPieceArchetype::Heavy;
	}
#endif
	bEditMenuLighting = !GameMode.IsValid() || GameMode->GetSettingsReturnScreen() == EFlickFrontendScreen::MainMenu;
	if (GameMode.IsValid() && GameMode->GetFrontendScreen() == EFlickFrontendScreen::Settings)
		LastFocusedScreen = EFlickFrontendScreen::Settings;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride_Lambda([this]() { return (ViewportLocalSize.Y - FlickPresentationFrame::GetContainedSize(ViewportLocalSize).Y) * 0.5f; })
			[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride_Lambda([this]() { return (ViewportLocalSize.Y - FlickPresentationFrame::GetContainedSize(ViewportLocalSize).Y) * 0.5f; })
			[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride_Lambda([this]() { return (ViewportLocalSize.X - FlickPresentationFrame::GetContainedSize(ViewportLocalSize).X) * 0.5f; })
			[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right)
		[
			SNew(SBox).WidthOverride_Lambda([this]() { return (ViewportLocalSize.X - FlickPresentationFrame::GetContainedSize(ViewportLocalSize).X) * 0.5f; })
			[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Black)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
		SNew(SBox)
		.WidthOverride_Lambda([this]() { return FlickPresentationFrame::GetContainedSize(ViewportLocalSize).X; })
		.HeightOverride_Lambda([this]() { return FlickPresentationFrame::GetContainedSize(ViewportLocalSize).Y; })
		[
		// Keep a stable 900-unit vertical design grid inside the presentation
		// frame. Ordinary displays expand naturally; extreme ratios use bars.
		SNew(SDPIScaler)
		.DPIScale_Lambda([this]()
		{
			// At low resolutions Unreal's UI DPI already shrinks Slate. Use the
			// available local units there so FLICK does not shrink a second time.
			// Keep the physical-size rule at ordinary and ultrawide resolutions.
			FVector2D ViewportSize = LayerLocalSize;
			if (GEngine && GEngine->GameViewport)
			{
				FVector2D PhysicalSize;
				GEngine->GameViewport->GetViewportSize(PhysicalSize);
				if (PhysicalSize.Y >= FlickUITheme::ReferenceHeight)
				{
					ViewportSize.X = FMath::Min(ViewportSize.X, PhysicalSize.X);
					ViewportSize.Y = FMath::Min(ViewportSize.Y, PhysicalSize.Y);
				}
			}
			return FMath::Max(0.25f, FMath::Min(
				ViewportSize.Y / FlickUITheme::ReferenceHeight,
				ViewportSize.X / FlickUITheme::ReferenceWidth));
		})
		[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SFlickAimArrowWidget).OwnerHud(OwnerHud).Visibility(EVisibility::HitTestInvisible)
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::MainMenu); })
			[
				BuildMainMenu()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::ModeSelect); })
			[
				BuildModeSelect()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::NetworkLobby); })
			[
				BuildNetworkLobby()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::PrivateMatch); })
			[
				BuildPrivateMatchSetup()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::OnlineBrowser); })
			[
				BuildOnlineBrowser()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::ItemShop); })
			[
				BuildItemShop()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Profile); })
			[
				BuildProfile()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Loadout); })
			[
				BuildLoadout()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::ClassSelect); })
			[
				BuildClassSelect()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Settings); })
			[
				BuildSettings()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetMatchHudVisibility(); })
			[
				BuildMatchHud()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				return ((GameMode.IsValid() && GameMode->IsCinematicReplayActive())
					|| (PlayerController.IsValid() && PlayerController->IsCinematicReplayPresentationActive()) || bReplayOverlayPreview)
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			[
				BuildCinematicReplayOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScoreboardVisibility(); })
			[
				BuildScoreboardOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetRoundOverVisibility(); })
			[
				BuildRoundOverOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Paused); })
			[
				BuildPauseOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				return PlayerController.IsValid() && PlayerController->ShouldShowPrivateTeamMenu()
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				BuildPrivateMatchTeamPicker()
			]
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 18.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(720.0f)
			.HeightOverride(58.0f)
			.Visibility_Lambda([this]() { return GetMatchmakingStatusVisibility(); })
			[
				BuildMatchmakingStatusBar()
			]
		]

		+ SOverlay::Slot()
		[
			SAssignNew(StartupOverlayWidget, SBox)
			.Visibility_Lambda([this]() { return bStartupOverlayVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				BuildStartupOverlay()
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 26.0f, 42.0f)
		[
			SAssignNew(InvitePromptWidget, SBox).WidthOverride(376.0f)
			.Visibility_Lambda([this]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				return (Sessions && Sessions->HasPendingPartyInvite()) || bInvitePromptPreview ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.RenderTransform_Lambda([this]()
			{
				const float Alpha = FMath::Clamp(InvitePromptElapsed / 0.42f, 0.0f, 1.0f);
				const float Eased = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
				return FSlateRenderTransform(FVector2D(0.0f, (1.0f - Eased) * 154.0f));
			})
			[BuildPartyInvitePrompt()]
		]
		+ SOverlay::Slot()
		.VAlign(VAlign_Bottom)
		[
			BuildMainMenuFooter()
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(12.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink).Padding(FMargin(6.0f, 2.0f))
			.Visibility_Lambda([this]() { return FlickVisualSettings::IsFpsVisible() || bFpsPreview ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d FPS"), FMath::RoundToInt(1.0f / FMath::Max(SmoothedFrameSeconds, 0.0001f)))); })
				.Font(UiFont(9, true)).ColorAndOpacity(Muted)
			]
		]
		]
		]
		]
	];

}

TSharedRef<SWidget> SFlickGameLayer::BuildPartyInvitePrompt()
{
	return SNew(SFlickMainMenuPanel)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(11, 22, 25, 250)))
		.AccentColor(Brand)
		.CutSize(12.0f)
		.BorderWidth(1.3f)
		.Padding(FMargin(16.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Brand)]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(TEXT("PARTY INVITE  //  STEAM"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text_Lambda([this]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				return FText::FromString(Sessions && Sessions->HasPendingPartyInvite()
					? Sessions->GetPendingPartyInviteName() : TEXT("RIVAL PLAYER"));
			}).Font(UiFont(17, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 11.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("INVITED YOU TO JOIN THEIR PARTY"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[SNew(SBox).HeightOverride(38.0f)[MakeMenuButton(TEXT("ACCEPT"), FOnClicked::CreateLambda([this]()
				{
					if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->AcceptPendingPartyInvite();
					return FReply::Handled();
				}), true, false, 38.0f)]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[SNew(SBox).HeightOverride(38.0f)[MakeMenuButton(TEXT("DECLINE"), FOnClicked::CreateLambda([this]()
				{
					if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->DeclinePendingPartyInvite();
					return FReply::Handled();
				}), false, false, 38.0f)]]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMatchmakingStatusBar()
{
	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor::FromSRGBColor(FColor(9, 17, 19, 244)))
		.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
		.UseAccentForOutline(false)
		.CutSize(9.0f)
		.BorderWidth(1.1f)
		.Padding(FMargin(16.0f, 7.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill)
			[
				SNew(SBox).WidthOverride(4.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(13.0f, 0.0f, 18.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!GameMode.IsValid()) return FText::GetEmpty();
						const FString QueueType = GameMode->IsRankedQueueSelected() ? TEXT("COMPETITIVE") : TEXT("CASUAL");
						const EFlickMatchVariant Variant = GameMode->GetSelectedMatchVariant();
						const FString Playlist = Variant == EFlickMatchVariant::Bob
							? TEXT("BOB")
							: FString::Printf(TEXT("%dV%d KNOCKOUT"), GameMode->GetMatchmakingPlayersPerTeam(), GameMode->GetMatchmakingPlayersPerTeam());
						return FText::FromString(FString::Printf(TEXT("%s  /  %s"), *QueueType, *Playlist));
					})
					.Font(UiFont(12, true))
					.ColorAndOpacity(Paper)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
						return FText::FromString(Sessions ? Sessions->GetStatusMessage() : TEXT("MATCHMAKING UNAVAILABLE"));
					})
					.Font(UiFont(9, true))
					.ColorAndOpacity(Brand)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(SSpacer)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(112.0f).HeightOverride(36.0f)
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this]()
					{
						if (GameMode.IsValid()) GameMode->CancelUnrankedMatchmaking();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("CANCEL  X")))
						.Font(UiFont(10, true))
						.ColorAndOpacity(Muted)
					]
				]
			]
		];
}

EVisibility SFlickGameLayer::GetMatchmakingStatusVisibility() const
{
	if (!GameMode.IsValid())
	{
		return EVisibility::Collapsed;
	}
	const EFlickFrontendScreen Screen = GameMode->GetFrontendScreen();
	if (Screen == EFlickFrontendScreen::Playing
		|| Screen == EFlickFrontendScreen::Paused
		|| Screen == EFlickFrontendScreen::ClassSelect
		|| Screen == EFlickFrontendScreen::NetworkLobby)
	{
		return EVisibility::Collapsed;
	}
	const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
	if (Sessions && Sessions->IsMatchmakingActive())
	{
		return EVisibility::Visible;
	}
	const UGameInstance* GameInstance = PlayerController.IsValid() ? PlayerController->GetGameInstance() : nullptr;
	const UFlickMatchmakingCoordinatorSubsystem* Coordinator = GameInstance
		? GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>()
		: nullptr;
	return Coordinator && Coordinator->GetQueueState() == EFlickCoordinatorQueueState::Allocated
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

void SFlickGameLayer::Tick(
	const FGeometry& AllottedGeometry,
	const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (InDeltaTime > 0.0f)
	{
		SmoothedFrameSeconds = FMath::Lerp(SmoothedFrameSeconds, InDeltaTime, 1.0f - FMath::Exp(-InDeltaTime * 4.0f));
	}
	ViewportLocalSize = AllottedGeometry.GetLocalSize();
	LayerLocalSize = FlickPresentationFrame::GetContainedSize(ViewportLocalSize);
	const UFlickSessionSubsystem* InviteSessions = GetDisplayedSessionSubsystem();
	const bool bInviteVisible = (InviteSessions && InviteSessions->HasPendingPartyInvite()) || bInvitePromptPreview;
	InvitePromptElapsed = bInviteVisible
		? (bInvitePromptWasVisible ? InvitePromptElapsed + FMath::Max(0.0f, InDeltaTime) : 0.0f)
		: 0.0f;
	bInvitePromptWasVisible = bInviteVisible;
	if (InvitePromptWidget.IsValid())
	{
		const float Alpha = FMath::Clamp(InvitePromptElapsed / 0.26f, 0.0f, 1.0f);
		InvitePromptWidget->SetRenderOpacity(0.35f + 0.65f * Alpha);
	}
	const bool bFooterVisible = GetScreenVisibility(EFlickFrontendScreen::MainMenu) != EVisibility::Collapsed;
	if (bFooterVisible)
	{
		// Restart at the right edge when returning from another menu instead of
		// advancing the ticker while it is hidden.
		FooterTickerElapsed = bFooterTickerWasVisible
			? FooterTickerElapsed + FMath::Max(0.0f, InDeltaTime)
			: 0.0;
	}
	bFooterTickerWasVisible = bFooterVisible;
	const bool bPartyTrayVisible = bFooterVisible && (IsDisplayedPartyActive() || bPartyTrayPreview);
	PartyTrayElapsed = bPartyTrayVisible
		? (bPartyTrayWasVisible ? PartyTrayElapsed + FMath::Max(0.0f, InDeltaTime) : 0.0f)
		: 0.0f;
	bPartyTrayWasVisible = bPartyTrayVisible;
	if (bFooterVisible && ChallengePreviewRows.IsValid())
	{
		const FFlickProfileStats Stats = GetChallengeStats();
		uint32 Signature = GetTypeHash(Stats.MatchesPlayed);
		Signature = HashCombine(Signature, GetTypeHash(Stats.Wins));
		Signature = HashCombine(Signature, GetTypeHash(Stats.Points));
		Signature = HashCombine(Signature, GetTypeHash(Stats.Knockouts));
		Signature = HashCombine(Signature, GetTypeHash(Stats.DoubleKnockouts));
		Signature = HashCombine(Signature, GetTypeHash(Stats.Shots));
		if (Signature != LastChallengePreviewSignature)
		{
			LastChallengePreviewSignature = Signature;
			RefreshChallengePreview();
		}
	}
	if (MainMenuPartyTray.IsValid())
	{
		const float Alpha = FMath::Clamp(PartyTrayElapsed / 0.24f, 0.0f, 1.0f);
		MainMenuPartyTray->SetRenderOpacity(0.35f + 0.65f * Alpha);
	}
	if (bStartupOverlayVisible)
	{
		StartupOverlayElapsed += InDeltaTime;
		if (StartupOverlayWidget.IsValid())
		{
			const float FadeAlpha = StartupOverlayElapsed <= StartupOverlayHoldDuration
				? 1.0f
				: 1.0f - FMath::Clamp(
					(StartupOverlayElapsed - StartupOverlayHoldDuration)
					/ FMath::Max(StartupOverlayFadeDuration, KINDA_SMALL_NUMBER),
					0.0f,
					1.0f);
			StartupOverlayWidget->SetRenderOpacity(FadeAlpha);
		}
		if (StartupOverlayElapsed >= StartupOverlayHoldDuration + StartupOverlayFadeDuration)
		{
			bStartupOverlayVisible = false;
			bHasAppliedInitialFocus = false;
			if (UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
				? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
				: nullptr)
			{
				FlickGameInstance->CompleteStartupPresentation();
			}
		}
	}
	if (!GameMode.IsValid())
	{
		return;
	}
	if (bSocialPanelOpen)
	{
		const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
		const int32 FriendCount = Sessions ? Sessions->GetFriends().Num() : 0;
		const int32 RecentCount = Sessions ? Sessions->GetRecentPlayers().Num() : 0;
		if (FriendCount != CachedSocialFriendCount || RecentCount != CachedSocialRecentCount)
		{
			RebuildSocialPlayerList();
		}
	}

	const EFlickFrontendScreen CurrentScreen = GameMode->GetFrontendScreen();
	if (CurrentScreen == EFlickFrontendScreen::Settings && LastFocusedScreen != EFlickFrontendScreen::Settings)
	{
		// Reopening from a match must not retain a menu-only preview from the previous visit.
		bEditMenuLighting = GameMode->GetSettingsReturnScreen() == EFlickFrontendScreen::MainMenu;
		bLightingPreview = false;
	}
	if (CurrentScreen == EFlickFrontendScreen::Loadout
		&& LastFocusedScreen != EFlickFrontendScreen::Loadout)
	{
		Player1SelectedLoadoutSlot = 0;
		Player2SelectedLoadoutSlot = 0;
		Player1HoveredLoadoutArchetype.Reset();
		Player2HoveredLoadoutArchetype.Reset();
	}

	LastFocusedScreen = CurrentScreen;
}
