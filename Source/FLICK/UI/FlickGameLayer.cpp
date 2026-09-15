#include "UI/FlickGameLayer.h"
#include "UI/FlickGameLayerPrivate.h"

SFlickGameLayer::SFlickGameLayer()
{
	MenuButtonStyle = FButtonStyle()
		.SetNormal(FSlateColorBrush(Panel))
		.SetHovered(FSlateColorBrush(PanelRaised))
		.SetPressed(FSlateColorBrush(FLinearColor(0.0f, 0.46f, 0.68f, 1.0f)))
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
		.SetUncheckedImage(FSlateColorBrush(Ink))
		.SetUncheckedHoveredImage(FSlateColorBrush(FLinearColor(0.035f, 0.16f, 0.22f, 1.0f)))
		.SetUncheckedPressedImage(FSlateColorBrush(FLinearColor(0.02f, 0.3f, 0.4f, 1.0f)))
		.SetCheckedImage(FSlateColorBrush(Brand))
		.SetCheckedHoveredImage(FSlateColorBrush(Paper))
		.SetCheckedPressedImage(FSlateColorBrush(FLinearColor(0.0f, 0.42f, 0.62f, 1.0f)))
		.SetPadding(FMargin(12.0f, 5.0f));
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
		.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.018f, 0.035f, 0.05f, 1.0f)))
		.SetFillImage(FSlateColorBrush(FLinearColor::White))
		.SetMarqueeImage(FSlateNoResource());
}

void SFlickGameLayer::Construct(const FArguments& InArgs)
{
	OwnerHud = InArgs._OwnerHud;
	GameMode = InArgs._GameMode;
	PlayerController = InArgs._PlayerController;
	if (const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
		? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
		: nullptr)
	{
		bStartupOverlayVisible = FlickGameInstance->ShouldShowStartupPresentation()
			&& GameMode.IsValid()
			&& GameMode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu;
	}

#if !UE_BUILD_SHIPPING
	bSocialPanelOpen = FParse::Param(FCommandLine::Get(), TEXT("FlickSocialPreview"));
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickPlayFormatPreview")))
	{
		SelectedPlayPlaylist = EFlickPlayPlaylist::Casual;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickLoadoutComparePreview")))
	{
		Player1HoveredLoadoutArchetype = EFlickPieceArchetype::Heavy;
	}
#endif

	ChildSlot
	[
		SNew(SScaleBox)
		.Stretch(EStretch::ScaleToFit)
		[
		SNew(SBox)
		.WidthOverride(FlickUITheme::ReferenceWidth)
		.HeightOverride(FlickUITheme::ReferenceHeight)
		[
		SNew(SOverlay)

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
				return GameMode.IsValid() && GameMode->IsCinematicReplayActive()
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
			SAssignNew(StartupOverlayWidget, SBox)
			.Visibility_Lambda([this]() { return bStartupOverlayVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				BuildStartupOverlay()
			]
		]
		]
		]
	];

}

void SFlickGameLayer::Tick(
	const FGeometry& AllottedGeometry,
	const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
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

	const EFlickFrontendScreen CurrentScreen = GameMode->GetFrontendScreen();
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
