//Lineups, classes, settings
#include "UI/FlickGameLayerPrivate.h"

EFlickLineupPreset SFlickGameLayer::GetDisplayedLoadoutPreset() const
{
	return GameMode.IsValid() ? GameMode->GetLoadoutEditingPreset() : RemoteLoadoutPreset;
}

EFlickPieceArchetype SFlickGameLayer::GetDisplayedLoadoutPiece(const int32 SlotIndex) const
{
	if (GameMode.IsValid()) return GameMode->GetLoadoutPiece(EFlickTeam::Player1, SlotIndex);
	const UFlickGameInstance* Instance = PlayerController.IsValid()
		? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
	return Instance ? Instance->GetClassLoadoutPiece(RemoteLoadoutPreset, SlotIndex) : EFlickPieceArchetype::Standard;
}

void SFlickGameLayer::SetDisplayedLoadoutPiece(const int32 SlotIndex, const EFlickPieceArchetype Archetype)
{
	if (GameMode.IsValid()) { GameMode->SetLoadoutPiece(EFlickTeam::Player1, SlotIndex, Archetype); return; }
	if (UFlickGameInstance* Instance = PlayerController.IsValid()
		? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr)
	{
		Instance->SetClassLoadoutPiece(RemoteLoadoutPreset, SlotIndex, Archetype);
	}
}

TSharedRef<SWidget> SFlickGameLayer::BuildModeCard(const EFlickMatchVariant Variant)
{
	const FString CountLabel = GetMatchVariantFormatLabel(Variant);
	const FString IndexLabel = Variant == EFlickMatchVariant::Classic ? TEXT("01") : TEXT("02");
	const FString FormatBadge = Variant == EFlickMatchVariant::Bob ? TEXT("BOB") : CountLabel;
	return SNew(SBox)
		.HeightOverride(294.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor::Transparent)
			.AccentColor_Lambda([this, Variant]()
			{
				const bool bSelected = GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant;
				return bSelected ? GetModeAccent(Variant) : FLinearColor(0.075f, 0.11f, 0.14f, 0.9f);
			})
			.CutSize(18.0f)
			.BorderWidth(2.0f)
			.Padding(FMargin(2.0f))
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.IsEnabled_Lambda([this]() { return !GameMode.IsValid() || !GameMode->IsMatchmakingSession(); })
				.OnClicked_Lambda([this, Variant]()
				{
					if (GameMode.IsValid()) GameMode->SelectMatchVariant(Variant);
					return FReply::Handled();
				})
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor_Lambda([this, Variant]()
					{
						const bool bSelected = GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant;
					return bSelected ? FLinearColor(0.006f, 0.021f, 0.034f, 0.92f) : FLinearColor(0.003f, 0.009f, 0.016f, 0.86f);
				})
					.AccentColor(FLinearColor::Transparent)
					.CutSize(15.0f)
					.DrawNeutralOutline(false)
					.Padding(FMargin(28.0f, 20.0f, 24.0f, 20.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush())
								.BorderBackgroundColor(FLinearColor(0.025f, 0.075f, 0.105f, 0.82f))
								.Padding(FMargin(7.0f, 4.0f))
								[
									SNew(STextBlock).Text(FText::FromString(IndexLabel)).Font(UiFont(10, true)).ColorAndOpacity(FLinearColor(0.72f, 0.82f, 0.88f, 1.0f))
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SFlickAngularBorder)
								.BackgroundColor(FLinearColor(0.004f, 0.012f, 0.021f, 0.88f))
								.AccentColor_Lambda([this, Variant]() { return FSlateColor(GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant ? GetModeAccent(Variant) : Hairline).GetSpecifiedColor(); })
								.CutSize(5.0f)
								.BorderWidth(0.8f)
								.Padding(FMargin(15.0f, 6.0f))
								[
									SNew(STextBlock).Text(FText::FromString(FormatBadge)).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(GetMatchVariantName(Variant)))
								.Font(UiFont(25, true))
								.ColorAndOpacity_Lambda([this, Variant]() { return FSlateColor(GetModeAccent(Variant)); })
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								.Visibility_Lambda([this, Variant]() { return GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
								[
									SNew(SFlickRoundPip).Color_Lambda([this, Variant]() { return GetModeAccent(Variant); }).Filled(true)
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("SELECTED"))).Font(UiFont(9, true)).ColorAndOpacity_Lambda([this, Variant]() { return FSlateColor(GetModeAccent(Variant)); })
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 23.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[
								SNew(SBox).WidthOverride(28.0f).HeightOverride(2.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor_Lambda([this, Variant]() { return GetModeAccent(Variant); })]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.12f, 0.20f, 0.25f, 0.42f))]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(GetMatchVariantSummary(Variant)))
							.Font(UiFont(11))
							.ColorAndOpacity(Muted)
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush())
							.BorderBackgroundColor(FLinearColor(0.012f, 0.03f, 0.044f, 0.88f))
							.Padding(FMargin(12.0f, 18.0f))
							[
								SNew(STextBlock).Text(FText::FromString(GetMatchVariantSeriesLabel(Variant))).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadout()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.001f, 0.004f, 0.008f, 0.92f))
		]
		+ SOverlay::Slot().Padding(10.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::DownOnly)
			[
				SNew(SBox).WidthOverride(1720.0f).HeightOverride(900.0f)
				[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(7.0f).HeightOverride(62.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(18.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("LINEUP"))).Font(UiFont(42, true)).ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("BUILD YOUR CLASS  /  TUNE EACH SLOT"))).Font(UiFont(12, true)).ColorAndOpacity(Cyan)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
						[
							SNew(SBox).WidthOverride(460.0f)
							[
								SNew(SEditableTextBox)
								.Text_Lambda([this]()
								{
					return FText::FromString(GetClassDisplayName(GetDisplayedLoadoutPreset()));
								})
								.HintText(FText::FromString(TEXT("LINEUP NAME")))
								.SelectAllTextWhenFocused(true)
								.OnVerifyTextChanged_Lambda([](const FText& Text, FText& Error)
								{
									if (Text.ToString().Len() <= 16) return true;
									Error = FText::FromString(TEXT("LINEUP NAMES ARE LIMITED TO 16 CHARACTERS"));
									return false;
								})
								.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type)
								{
					if (GameMode.IsValid())
					{
						GameMode->SetClassName(GameMode->GetLoadoutEditingPreset(), Text.ToString());
					}
					else if (UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr)
					{
						Instance->SetClassName(RemoteLoadoutPreset, Text.ToString());
					}
								})
							]
						]
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 16.0f, 0.0f, 14.0f)
				[
					BuildLoadoutWorkspace()
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(170.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->CloseLoadout(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu;
							return FReply::Handled();
						}))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const int32 Count = GameMode.IsValid() ? GameMode->GetLoadoutEditingPieceCount() : 4;
							return FText::FromString(FString::Printf(TEXT("%d PUCK LINEUP  /  %s"), Count,
								*GetClassDisplayName(GetDisplayedLoadoutPreset())));
						})
						.Font(UiFont(10, true)).ColorAndOpacity(Muted)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(285.0f)[MakeMenuButton(TEXT("SAVE LINEUP"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid())
							{
								GameMode->PlayMenuSound(true);
								GameMode->CloseLoadout();
							}
							else RemotePartyScreen = EFlickFrontendScreen::MainMenu;
							return FReply::Handled();
						}), true, false, 52.0f)]
					]
				]
			]
			]
		]
		;
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutWorkspace()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(350.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					BuildLineupRadar()
				]
				+ SHorizontalBox::Slot().FillWidth(0.33f).Padding(10.0f, 0.0f)
				[
					BuildLoadoutFormation(EFlickTeam::Player1)
				]
				+ SHorizontalBox::Slot().FillWidth(0.42f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						BuildLoadoutComparison(EFlickTeam::Player1)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						BuildLoadoutPresetBar(EFlickTeam::Player1)
					]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			BuildArchetypePicker(EFlickTeam::Player1)
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLineupRadar()
{
	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.004f, 0.013f, 0.023f, 0.96f))
		.AccentColor(Hairline)
		.CutSize(10.0f)
		.Padding(FMargin(16.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("LINEUP PROFILE"))).Font(UiFont(12, true)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(GetClassDisplayName(GetDisplayedLoadoutPreset())); })
					.Font(UiFont(10, true)).ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(24.0f, 10.0f)
				[
					SNew(SFlickRadarChart)
					.ValueProvider([this]() { return GetLineupProfileStats(EFlickTeam::Player1); })
					.AccentColor(Cyan)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("SPEED"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 55.0f, 2.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("WEIGHT"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 2.0f, 44.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("IMPACT"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("CONTROL"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(2.0f, 0.0f, 0.0f, 44.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("COAST"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(2.0f, 55.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("STABILITY"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutFormation(const EFlickTeam Team)
{
	constexpr float FormationPuckSize = 112.0f;
	TSharedRef<SConstraintCanvas> Formation = SNew(SConstraintCanvas);
	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		Formation->AddSlot()
			.Anchors(TAttribute<FAnchors>::CreateLambda([this, SlotIndex]()
			{
				const bool bThreePuckFormation = GameMode.IsValid()
					&& GameMode->GetLoadoutEditingPieceCount() == 3;
				if (bThreePuckFormation)
				{
					const FVector2D Positions[] = {
						FVector2D(0.5f, 0.3f),
						FVector2D(0.29f, 0.7f),
						FVector2D(0.71f, 0.7f),
						FVector2D(0.5f, 0.9f)};
					return FAnchors(Positions[SlotIndex].X, Positions[SlotIndex].Y);
				}
				const FVector2D Positions[] = {
					FVector2D(0.29f, 0.31f),
					FVector2D(0.71f, 0.31f),
					FVector2D(0.31f, 0.7f),
					FVector2D(0.69f, 0.7f)};
				return FAnchors(Positions[SlotIndex].X, Positions[SlotIndex].Y);
			}))
			.Alignment(FVector2D(0.5f, 0.5f))
			.Offset(FMargin(0.0f, 0.0f, FormationPuckSize, FormationPuckSize))
			.AutoSize(false)
		[
			BuildFormationPuck(Team, SlotIndex)
		];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.004f, 0.013f, 0.023f, 0.96f))
		.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
		.CutSize(18.0f)
		.BorderWidth(1.4f)
		.Padding(FMargin(8.0f))
		[
			SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(1.0f).HeightOverride(150.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(150.0f).HeightOverride(1.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)
					]
				]
				+ SOverlay::Slot().Padding(10.0f)
				[
					Formation
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f, 8.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("STARTING FORMATION"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 7.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]()
					{
						const int32 Slot = GetSelectedLoadoutSlot(Team);
						return FText::FromString(FString::Printf(TEXT("SLOT %02d  /  %s"),
							Slot + 1,
							*GetPieceArchetypeName(GetDisplayedLoadoutPiece(Slot))));
					})
					.Font(UiFont(10, true))
					.ColorAndOpacity(GetTeamAccent(Team))
				]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildFormationPuck(const EFlickTeam Team, const int32 SlotIndex)
{
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.ToolTipText_Lambda([this, Team, SlotIndex]()
		{
			return FText::FromString(FString::Printf(
				TEXT("SLOT %02d  /  %s"),
				SlotIndex + 1,
				*GetPieceArchetypeName(GetDisplayedLoadoutPiece(SlotIndex))));
		})
		.OnClicked_Lambda([this, Team, SlotIndex]()
		{
			SelectLoadoutSlot(Team, SlotIndex);
			return FReply::Handled();
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SFlickPuckDisc)
				.TeamColor(GetTeamAccent(Team))
				.Archetype_Lambda([this, SlotIndex]()
				{
					return GetDisplayedLoadoutPiece(SlotIndex);
				})
				.AccentColor_Lambda([this, Team, SlotIndex]()
				{
					return FlickPieceArchetypeRules::GetVisualAccent(
						GetDisplayedLoadoutPiece(SlotIndex),
						GetTeamAccent(Team));
				})
				.Selected_Lambda([this, Team, SlotIndex]() { return GetSelectedLoadoutSlot(Team) == SlotIndex; })
				.RadiusScale_Lambda([this, SlotIndex]()
				{
					return FlickPieceArchetypeRules::Get(GetDisplayedLoadoutPiece(SlotIndex)).RadiusMultiplier;
				})
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(3.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), SlotIndex + 1)))
				.Font(UiFont(7, true))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
	return SNew(SBox)
		.Visibility_Lambda([this, SlotIndex]() { return GetLoadoutRowVisibility(SlotIndex); })
		[
			Button
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutComparison(const EFlickTeam Team)
{
	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.006f, 0.016f, 0.026f, 0.97f))
		.AccentColor_Lambda([this, Team]() { return FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team)).CopyWithNewOpacity(0.72f); })
		.CutSize(10.0f)
		.Padding(FMargin(18.0f, 14.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(FString::Printf(TEXT("SLOT %02d"), GetSelectedLoadoutSlot(Team) + 1)); })
					.Font(UiFont(8, true))
					.ColorAndOpacity(Muted)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(GetPieceArchetypeName(GetPreviewLoadoutArchetype(Team))); })
					.Font(UiFont(25, true))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).ClassLabel); })
					.Font(UiFont(10, true))
					.ColorAndOpacity_Lambda([this, Team]() { return FSlateColor(FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team))); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Summary); })
				.Font(UiFont(9, true))
				.ColorAndOpacity(Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team]() { return FText::FromString(TEXT("+  ") + FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Strengths); })
				.Font(UiFont(8, true))
				.ColorAndOpacity(FLinearColor(0.28f, 0.9f, 0.55f, 1.0f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team]() { return FText::FromString(TEXT("-  ") + FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Weaknesses); })
				.Font(UiFont(8, true))
				.ColorAndOpacity(FLinearColor(1.0f, 0.42f, 0.18f, 1.0f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)[BuildLoadoutStatRow(Team, TEXT("SPEED"), 0)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("WEIGHT"), 1)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("IMPACT"), 2)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("CONTROL"), 3)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("COAST"), 4)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("STABILITY"), 5)]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutStatRow(
	const EFlickTeam Team,
	const FString& Label,
	const int32 StatIndex)
{
	constexpr float BarWidth = 270.0f;
	return SNew(SBox).HeightOverride(23.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(72.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(7.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.004f, 0.008f, 0.014f, 1.0f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this, Team, StatIndex]()
						{
							return FOptionalSize(BarWidth * GetLoadoutStatValue(Team, StatIndex, false));
						}))
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.35f, 0.43f, 0.49f, 0.76f))
						]
					]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this, Team, StatIndex]()
						{
							return FOptionalSize(BarWidth * GetLoadoutStatValue(Team, StatIndex, true));
						}))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush())
							.BorderBackgroundColor_Lambda([this, Team]()
							{
								return FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team)).CopyWithNewOpacity(0.88f);
							})
						]
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team, StatIndex]()
				{
					const bool bHovering = Team == EFlickTeam::Player1
						? Player1HoveredLoadoutArchetype.IsSet()
						: Player2HoveredLoadoutArchetype.IsSet();
					if (!bHovering) return FText::FromString(TEXT("--"));
					const int32 Delta = FMath::RoundToInt((GetLoadoutStatValue(Team, StatIndex, true)
						- GetLoadoutStatValue(Team, StatIndex, false)) * 100.0f);
					return FText::FromString(Delta > 0
						? FString::Printf(TEXT("+%d"), Delta)
						: FString::Printf(TEXT("%d"), Delta));
				})
				.Font(UiFont(8, true))
				.ColorAndOpacity_Lambda([this, Team, StatIndex]()
				{
					const float Delta = GetLoadoutStatValue(Team, StatIndex, true)
						- GetLoadoutStatValue(Team, StatIndex, false);
					return FSlateColor(Delta > 0.01f
						? FLinearColor(0.22f, 0.92f, 0.56f, 1.0f)
						: Delta < -0.01f ? FLinearColor(1.0f, 0.38f, 0.16f, 1.0f) : Muted);
				})
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutPresetBar(const EFlickTeam Team)
{
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(2.0f));
	int32 Column = 0;
	for (const EFlickLineupPreset Preset : {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control})
	{
		TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.OnClicked_Lambda([this, Team, Preset]()
			{
				if (GameMode.IsValid())
				{
					if (Team == EFlickTeam::Player1) Player1HoveredLoadoutArchetype.Reset();
					else Player2HoveredLoadoutArchetype.Reset();
						GameMode->SelectLoadoutEditingPreset(Preset);
					}
					else RemoteLoadoutPreset = Preset;
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor(FLinearColor(0.01f, 0.022f, 0.034f, 0.98f))
				.Padding(FMargin(4.0f, 7.0f))
				[
					SNew(STextBlock)
					.Text_Lambda([this, Preset]() { return FText::FromString(GetClassDisplayName(Preset)); })
					.Font(UiFont(10, true))
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FLinearColor::White)
				]
			];
		const TWeakPtr<SButton> WeakButton = Button;
		Grid->AddSlot(Column++, 0)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([this, Team, Preset, WeakButton]()
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				if (Pinned.IsValid() && Pinned->HasKeyboardFocus()) return FLinearColor::White;
				return GetDisplayedLoadoutPreset() == Preset
					? GetTeamAccent(Team)
					: Hairline;
			})
			.Padding(1.0f)
			[
				Button
			]
		];
	}
	return Grid;
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassPlayerRow(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	const FLinearColor TeamAccent = GetTeamAccent(Team);
	TSharedRef<SUniformGridPanel> ClassGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f));
	int32 Column = 0;
	for (const EFlickLineupPreset Preset : {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control})
	{
		TSharedRef<SButton> ClassButton = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Cursor(EMouseCursor::Hand)
			.OnClicked_Lambda([this, Team, PlayerSlot, Preset]()
			{
				if (PlayerController.IsValid())
				{
					PlayerController->RequestSelectClass(Preset);
				}
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor_Lambda([this, Team, PlayerSlot, Preset, TeamAccent]()
				{
					return GetSelectedClassDraft() == Preset
						? GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.2f)
						: FLinearColor(0.006f, 0.017f, 0.027f, 0.96f);
				})
				.Padding(FMargin(8.0f, 4.0f))
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Preset]() { return FText::FromString(GetClassDisplayName(Preset)); })
						.Font(UiFont(18, true))
						.Justification(ETextJustify::Center)
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						.ColorAndOpacity(Muted)
					]
				]
			];

		const TWeakPtr<SButton> WeakClassButton = ClassButton;
		ClassGrid->AddSlot(Column++, 0)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.004f, 0.012f, 0.02f, 0.98f))
			.AccentColor_Lambda([this, Team, PlayerSlot, Preset, WeakClassButton, TeamAccent]()
			{
				const TSharedPtr<SButton> Pinned = WeakClassButton.Pin();
				const bool bSelected = GetSelectedClassDraft() == Preset;
				return bSelected || (Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()))
					? GetTeamAccent(GetClassSelectionTeam())
					: Hairline;
			})
			.CutSize(5.0f)
			.BorderWidth(1.0f)
			.UseAccentForOutline(true)
			.Padding(1.0f)
			[
				ClassButton
			]
		];
	}

	return SNew(SBox)
		.HeightOverride(100.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.006f, 0.017f, 0.028f, 0.96f))
			.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.52f); })
			.CutSize(7.0f)
			.BorderWidth(0.8f)
			.Padding(FMargin(12.0f, 9.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 0.0f, 0.0f, 5.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("PLAYER %d"), GetClassSelectionPlayerSlot() + 1)); })
					.Font(UiFont(10, true))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					ClassGrid
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassPuckCard(const int32 PieceSlot)
{
	return SNew(SBox)
		.HeightOverride(166.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.004f, 0.014f, 0.024f, 0.98f))
			.AccentColor_Lambda([this, PieceSlot]()
			{
				return FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam())).CopyWithNewOpacity(0.72f);
			})
			.CutSize(8.0f)
			.BorderWidth(0.9f)
			.UseAccentForOutline(true)
			.Padding(FMargin(12.0f, 10.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(120.0f).HeightOverride(120.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SFlickPuckDisc)
							.TeamColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()); })
							.Archetype_Lambda([this, PieceSlot]() { return GetSelectedClassPiece(PieceSlot); })
							.AccentColor_Lambda([this, PieceSlot]()
							{
								return FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam()));
							})
							.Selected(true)
							.RadiusScale_Lambda([this, PieceSlot]()
							{
								return FlickPieceArchetypeRules::Get(GetSelectedClassPiece(PieceSlot)).RadiusMultiplier;
							})
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(11.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PieceSlot]() { return FText::FromString(GetPieceArchetypeName(GetSelectedClassPiece(PieceSlot))); })
						.Font(DisplayFont(20))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, PieceSlot]()
						{
							return FText::FromString(FlickPieceArchetypeRules::Get(GetSelectedClassPiece(PieceSlot)).ClassLabel);
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity_Lambda([this, PieceSlot]()
						{
							return FSlateColor(FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam())));
						})
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PieceSlot]()
						{
							return FText::FromString(FlickPieceArchetypeRules::Get(GetSelectedClassPiece(PieceSlot)).Summary);
						})
						.Font(UiFont(8, true))
						.ColorAndOpacity(Muted)
						.AutoWrapText(true)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassProfileStatRow(
	const FString& Label,
	const int32 StatIndex)
{
	constexpr float BarWidth = 290.0f;
	return SNew(SBox).HeightOverride(28.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(86.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(9, true)).ColorAndOpacity(Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(8.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.002f, 0.007f, 0.012f, 1.0f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this, StatIndex]()
						{
							return FOptionalSize(BarWidth * GetSelectedClassStatValue(StatIndex));
						}))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush())
							.BorderBackgroundColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()); })
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassShowcase()
{
	TSharedRef<SUniformGridPanel> PuckGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	for (int32 PieceSlot = 0; PieceSlot < 4; ++PieceSlot)
	{
		PuckGrid->AddSlot(PieceSlot % 2, PieceSlot / 2)[BuildClassPuckCard(PieceSlot)];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.002f, 0.011f, 0.021f, 0.98f))
		.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.76f); })
		.CutSize(12.0f)
		.BorderWidth(1.0f)
		.UseAccentForOutline(true)
		.Padding(FMargin(18.0f, 15.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.62f).Padding(0.0f, 0.0f, 15.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("YOUR FOUR-PUCK LINEUP"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					PuckGrid
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.38f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.005f, 0.017f, 0.029f, 0.98f))
				.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.48f); })
				.CutSize(9.0f)
				.BorderWidth(0.8f)
				.Padding(FMargin(20.0f, 17.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(GetClassDisplayName(GetSelectedClassDraft())); })
						.Font(DisplayFont(26))
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("SPEED"), 0)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("WEIGHT"), 1)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("IMPACT"), 2)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("CONTROL"), 3)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("COAST"), 4)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("STABILITY"), 5)]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassSelect()
{
	const EFlickTeam SelectionTeam = GetClassSelectionTeam();
	const int32 PlayerSlot = GetClassSelectionPlayerSlot();
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.004f, 0.01f, 0.88f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::DownOnly)
			[
				SNew(SBox).WidthOverride(1320.0f).HeightOverride(830.0f)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(FLinearColor(0.001f, 0.01f, 0.02f, 0.985f))
					.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.82f); })
					.CutSize(18.0f).BorderWidth(1.2f).UseAccentForOutline(true).Padding(FMargin(34.0f, 28.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() && GameMode->IsChangingClassForNextRound() ? TEXT("CHANGE CLASS") : TEXT("SELECT CLASS")); })
									.Font(DisplayFont(34)).ColorAndOpacity(FLinearColor::White)
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text_Lambda([this]()
									{
										if (GameMode.IsValid() && GameMode->IsChangingClassForNextRound())
										{
											return FText::FromString(TEXT("YOUR NEW CLASS APPLIES WHEN THE NEXT ROUND STARTS"));
										}
										const AFlickGameState* State = GetScoreboardGameState();
										if (State && (State->bNetworkClassSelectionActive || State->bPrivateMatchAssignmentActive))
										{
											return FText::FromString(TEXT("CHOOSE YOUR CLASS FOR THIS MATCH"));
										}
										return FText::FromString(GameMode.IsValid() && GameMode->IsPreparingTrainingBotMatch()
											? TEXT("CHOOSE YOUR CLASS; THE BOT RECEIVES A RANDOM CLASS")
											: TEXT("CHOOSE BLUE PLAYER 1'S CLASS; OTHER LOCAL PLAYERS RECEIVE RANDOM CLASSES"));
									})
									.Font(UiFont(11, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const EFlickTeam Team = GetClassSelectionTeam();
									return FText::FromString(FString::Printf(
										TEXT("%s TEAM  /  PLAYER %d"),
										Team == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
										GetClassSelectionPlayerSlot() + 1));
								})
								.Font(UiFont(10, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 12.0f)
						[
							BuildClassPlayerRow(SelectionTeam, PlayerSlot)
						]
						+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 18.0f)
						[
							BuildClassShowcase()
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SBox)
								.Visibility_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return State && (State->bNetworkClassSelectionActive || State->bPrivateMatchAssignmentActive)
										? EVisibility::Collapsed : EVisibility::Visible;
								})
								.WidthOverride(190.0f)
								[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CancelClassSelection(); return FReply::Handled(); }), false, false, 56.0f)]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
							[
								SNew(STextBlock)
								.Visibility_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return (State && (State->bNetworkClassSelectionActive || State->bPrivateMatchAssignmentActive))
										|| (GameMode.IsValid() && !GameMode->IsChangingClassForNextRound())
										? EVisibility::HitTestInvisible : EVisibility::Collapsed;
								})
								.Text_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									const float Remaining = State && State->bNetworkClassSelectionActive
										? State->GetNetworkClassSelectionTimeRemaining()
										: State && State->bPrivateMatchAssignmentCountdownActive
											? State->GetPrivateMatchAssignmentTimeRemaining()
										: GameMode.IsValid() ? GameMode->GetInitialClassSelectionTimeRemaining() : 0.0f;
									return FText::FromString(FString::Printf(
										TEXT("AUTO CONFIRM IN %02d"),
										FMath::CeilToInt(Remaining)));
								})
								.Font(UiFont(10, true))
								.ColorAndOpacity(Cyan)
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SBox).WidthOverride(280.0f)
								[
									MakeMenuButton(TEXT("CONFIRM CLASS"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestConfirmClass(); return FReply::Handled(); }), true, false, 56.0f)
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildArchetypePicker(const EFlickTeam Team)
{
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.0f));
	for (int32 Index = 0; Index < FlickPieceArchetypeRules::ArchetypeCount; ++Index)
	{
		Grid->AddSlot(Index % 5, Index / 5)
		[
			BuildArchetypeChoice(Team, static_cast<EFlickPieceArchetype>(Index))
		];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.003f, 0.01f, 0.018f, 0.96f))
		.AccentColor(Hairline)
		.CutSize(12.0f)
		.Padding(FMargin(12.0f, 9.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6.0f, 0.0f, 6.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("AVAILABLE PUCK TYPES"))).Font(UiFont(13, true)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(FString::Printf(TEXT("PREVIEWING SLOT %d"), GetSelectedLoadoutSlot(Team) + 1)); })
					.Font(UiFont(9, true)).ColorAndOpacity(Muted)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				Grid
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildArchetypeChoice(
	const EFlickTeam Team,
	const EFlickPieceArchetype Archetype)
{
	const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
	const FFlickPieceDisplayStats Stats = FlickPieceArchetypeRules::GetDisplayStats(Archetype);
	const FLinearColor Accent = FlickPieceArchetypeRules::GetVisualAccent(Archetype, GetTeamAccent(Team));
	const FString ClassLabel = Rules.ClassLabel;
	const TArray<float> Values = {Stats.Speed, Stats.Weight, Stats.Impact, Stats.Control, Stats.Coast, Stats.Stability};
	const TArray<FString> Labels = {TEXT("SPD"), TEXT("WGT"), TEXT("IMP"), TEXT("CTL"), TEXT("CST"), TEXT("STB")};
	TSharedRef<SVerticalBox> StatRows = SNew(SVerticalBox);
	for (int32 StatIndex = 0; StatIndex < Values.Num(); ++StatIndex)
	{
		StatRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(27.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Labels[StatIndex])).Font(UiFont(6, true)).ColorAndOpacity(Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(154.0f).HeightOverride(6.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.003f, 0.007f, 0.012f, 1.0f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox).WidthOverride(154.0f * Values[StatIndex])
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Accent)
						]
					]
				]
			]
		];
	}
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.OnHovered_Lambda([this, Team, Archetype]()
		{
			SetHoveredLoadoutArchetype(Team, Archetype);
		})
		.OnUnhovered_Lambda([this, Team, Archetype]()
		{
			ClearHoveredLoadoutArchetype(Team, Archetype);
		})
		.OnClicked_Lambda([this, Team, Archetype]()
		{
			if (GameMode.IsValid())
			{
				GameMode->SetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team), Archetype);
				GameMode->PlayMenuSound(false);
			}
			else SetDisplayedLoadoutPiece(GetSelectedLoadoutSlot(Team), Archetype);
			return FReply::Handled();
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, Team, Archetype, Accent]()
			{
				const bool bSelected = GetDisplayedLoadoutPiece(GetSelectedLoadoutSlot(Team)) == Archetype;
				return bSelected ? FMath::Lerp(PanelRaised, Accent, 0.018f) : Panel;
			})
			.AccentColor_Lambda([this, Team, Archetype, Accent]()
			{
				const bool bSelected = GetDisplayedLoadoutPiece(GetSelectedLoadoutSlot(Team)) == Archetype;
				return bSelected ? Accent : Hairline;
			})
			.CutSize(8.0f)
			.Padding(FMargin(12.0f, 8.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(34.0f).HeightOverride(34.0f)
						[
							SNew(SFlickPuckDisc).TeamColor(GetTeamAccent(Team)).AccentColor(Accent).Archetype(Archetype).RadiusScale(0.72f)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(7.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(GetPieceArchetypeName(Archetype))).Font(UiFont(11, true)).ColorAndOpacity(Accent)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(ClassLabel)).Font(UiFont(7, true)).ColorAndOpacity(Muted)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, 4.0f, 1.0f, 5.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Rules.Summary)).Font(UiFont(7)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					StatRows
				]
			]
		];
	const TWeakPtr<SButton> WeakButton = Button;
	return SNew(SBox)
		.HeightOverride(154.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([this, Team, Archetype, Accent, WeakButton]()
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				if (Pinned.IsValid() && (Pinned->HasKeyboardFocus() || Pinned->IsHovered())) return FLinearColor::White;
				const bool bSelected = GameMode.IsValid()
					&& GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team)) == Archetype;
				return bSelected ? Accent : Hairline;
			})
			.Padding(1.0f)
			[
				Button
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildSettings()
{
	const auto Checked = [](const bool bValue) { return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };
	TSharedRef<SVerticalBox> ControlRows = SNew(SVerticalBox);
	FString LastGroup;
	for (const FlickControlBindings::FControl& Control : FlickControlBindings::GetControls())
	{
		if (LastGroup != Control.Group)
		{
			LastGroup = Control.Group;
			ControlRows->AddSlot().AutoHeight().Padding(0.0f, 13.0f, 0.0f, 5.0f)
			[SNew(STextBlock).Text(FText::FromString(LastGroup)).Font(UiFont(11, true)).ColorAndOpacity(Brand)];
		}
		const FString Id(Control.Id);
		ControlRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(Control.Label)).Font(UiFont(12)).ColorAndOpacity(Paper)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(210.0f)
				[
					SNew(SInputKeySelector)
					.SelectedKey_Lambda([Id]() { return FInputChord(FlickControlBindings::GetKey(*Id)); })
					.Font(UiFont(12, true)).AllowModifierKeys(false).AllowGamepadKeys(false)
					.EscapeCancelsSelection(false)
					.OnKeySelected_Lambda([this, Id](const FInputChord& Chord)
					{
						if (FlickControlBindings::SetKey(*Id, Chord.Key))
						{
							if (PlayerController.IsValid()) PlayerController->RefreshControlBindings();
							ControlBindingMessage = TEXT("CONTROL SAVED");
						}
						else ControlBindingMessage = TEXT("KEY ALREADY IN USE OR UNSUPPORTED");
					})
				]
			]
		];
	}
	TSharedRef<SVerticalBox> GraphicsRows = SNew(SVerticalBox);
	const auto QualityLabel = [](int32 Level) -> FText
	{
		static const TCHAR* Labels[] = {TEXT("LOW"), TEXT("MEDIUM"), TEXT("HIGH"), TEXT("EPIC")};
		return FText::FromString(Level >= 0 && Level < 4 ? Labels[Level] : TEXT("CUSTOM"));
	};
	const auto AddQualityRow = [&GraphicsRows, QualityLabel, this](const TCHAR* Label, auto Getter, auto Setter)
	{
		GraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(Label,
			TAttribute<FText>::CreateLambda([Getter, QualityLabel]()
			{
				const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
				return Settings ? QualityLabel((Settings->*Getter)()) : FText::FromString(TEXT("EPIC"));
			}),
			FOnClicked::CreateLambda([Getter, Setter]()
			{
				if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
				{
					(Settings->*Setter)(FMath::Clamp((Settings->*Getter)() - 1, 0, 3));
					Settings->ApplyNonResolutionSettings(); Settings->SaveSettings();
					FlickVisualSettings::ApplySaved();
				}
				return FReply::Handled();
			}),
			FOnClicked::CreateLambda([Getter, Setter]()
			{
				if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
				{
					(Settings->*Setter)(FMath::Clamp((Settings->*Getter)() + 1, 0, 3));
					Settings->ApplyNonResolutionSettings(); Settings->SaveSettings();
					FlickVisualSettings::ApplySaved();
				}
				return FReply::Handled();
			}))];
	};
	GraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(TEXT("OVERALL QUALITY"),
		TAttribute<FText>::CreateLambda([QualityLabel]()
		{
			const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
			return Settings ? QualityLabel(Settings->GetOverallScalabilityLevel()) : FText::FromString(TEXT("EPIC"));
		}),
		FOnClicked::CreateLambda([]()
		{
			if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
			{
				const int32 Current = Settings->GetOverallScalabilityLevel();
				Settings->SetOverallScalabilityLevel(FMath::Clamp(Current < 0 ? 2 : Current - 1, 0, 3));
				Settings->ApplyNonResolutionSettings(); Settings->SaveSettings(); FlickVisualSettings::ApplySaved();
			}
			return FReply::Handled();
		}),
		FOnClicked::CreateLambda([]()
		{
			if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
			{
				const int32 Current = Settings->GetOverallScalabilityLevel();
				Settings->SetOverallScalabilityLevel(FMath::Clamp(Current < 0 ? 2 : Current + 1, 0, 3));
				Settings->ApplyNonResolutionSettings(); Settings->SaveSettings(); FlickVisualSettings::ApplySaved();
			}
			return FReply::Handled();
		}))];
	GraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(TEXT("RENDER SCALE"),
		TAttribute<FText>::CreateLambda([]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FlickVisualSettings::GetRenderScale())); }),
		FOnClicked::CreateLambda([]()
		{
			static const int32 Steps[] = {50, 67, 75, 85, 100, 110};
			const int32 Current = FlickVisualSettings::GetRenderScale();
			int32 Next = Steps[0];
			for (const int32 Step : Steps) { if (Step < Current) Next = Step; }
			FlickVisualSettings::SetRenderScale(Next); return FReply::Handled();
		}),
		FOnClicked::CreateLambda([]()
		{
			static const int32 Steps[] = {50, 67, 75, 85, 100, 110};
			const int32 Current = FlickVisualSettings::GetRenderScale();
			int32 Next = Steps[UE_ARRAY_COUNT(Steps) - 1];
			for (const int32 Step : Steps) { if (Step > Current) { Next = Step; break; } }
			FlickVisualSettings::SetRenderScale(Next); return FReply::Handled();
		}))];
	GraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(TEXT("HARDWARE LUMEN"),
		TAttribute<FText>::CreateLambda([]() { return FText::FromString(FlickVisualSettings::IsHardwareLumenEnabled() ? TEXT("ON") : TEXT("OFF")); }),
		FOnClicked::CreateLambda([]() { FlickVisualSettings::SetHardwareLumenEnabled(false); return FReply::Handled(); }),
		FOnClicked::CreateLambda([]() { FlickVisualSettings::SetHardwareLumenEnabled(true); return FReply::Handled(); }))];
	AddQualityRow(TEXT("GLOBAL ILLUMINATION"), &UGameUserSettings::GetGlobalIlluminationQuality, &UGameUserSettings::SetGlobalIlluminationQuality);
	AddQualityRow(TEXT("REFLECTIONS"), &UGameUserSettings::GetReflectionQuality, &UGameUserSettings::SetReflectionQuality);
	AddQualityRow(TEXT("SHADOWS"), &UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality);
	AddQualityRow(TEXT("ANTI-ALIASING"), &UGameUserSettings::GetAntiAliasingQuality, &UGameUserSettings::SetAntiAliasingQuality);
	AddQualityRow(TEXT("VIEW DISTANCE"), &UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality);
	AddQualityRow(TEXT("POST-PROCESSING"), &UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality);
	AddQualityRow(TEXT("VISUAL EFFECTS"), &UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality);
	AddQualityRow(TEXT("TEXTURES"), &UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality);
	AddQualityRow(TEXT("SHADING"), &UGameUserSettings::GetShadingQuality, &UGameUserSettings::SetShadingQuality);
	const auto SectionHeading = [](const FString& Number, const FString& Title, const FString& Description) -> TSharedRef<SWidget>
	{
		(void)Number;
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Title)).Font(DisplayFont(25)).ColorAndOpacity(Paper)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
			[SNew(STextBlock).Text(FText::FromString(Description)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)];
	};
	const auto MakeSettingsTab = [this](const EFlickSettingsTab Tab, const FString& Label) -> TSharedRef<SWidget>
	{
		TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.OnClicked_Lambda([this, Tab]()
			{
				SelectedSettingsTab = Tab;
				if (GameMode.IsValid()) GameMode->PlayMenuSound(true);
				return FReply::Handled();
			});
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.Padding(FMargin(32.0f, 12.0f))
			.BorderBackgroundColor_Lambda([this, Tab, WeakButton]()
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				return SelectedSettingsTab == Tab
					? Brand
					: Pinned.IsValid() && Pinned->IsHovered() ? PanelRaised : Panel;
			})
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(UiFont(11, true))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity_Lambda([this, Tab]() { return SelectedSettingsTab == Tab ? Ink : Paper; })
			]);
		return Button;
	};
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink.CopyWithNewOpacity(0.88f))]
		+ SOverlay::Slot()[SNew(SFlickInterfaceBackdrop).Visibility(EVisibility::HitTestInvisible).Opacity(0.35f)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1320.0f).HeightOverride(820.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Panel).Padding(FMargin(36.0f, 24.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 20.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK  /  YOUR SETUP"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("MAKE IT YOURS."))).Font(DisplayFont(46)).ColorAndOpacity(Paper)]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(16.0f, 0.0f, 0.0f, 8.0f)
						[SNew(STextBlock).Text(FText::FromString(TEXT("SETTINGS"))).Font(UiFont(12, true)).ColorAndOpacity(Muted)]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 18.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeSettingsTab(EFlickSettingsTab::GameFeel, TEXT("GAMEPLAY"))]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeSettingsTab(EFlickSettingsTab::Camera, TEXT("CAMERA"))]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeSettingsTab(EFlickSettingsTab::Interface, TEXT("INTERFACE"))]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeSettingsTab(EFlickSettingsTab::Display, TEXT("VIDEO"))]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeSettingsTab(EFlickSettingsTab::Sound, TEXT("AUDIO"))]
						+ SHorizontalBox::Slot().AutoWidth()[MakeSettingsTab(EFlickSettingsTab::Controls, TEXT("CONTROLS"))]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SBorder).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Controls ? EVisibility::Visible : EVisibility::Collapsed; })
								.BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT(""), TEXT("CONTROLS"), TEXT("Select a binding, then press a new key or mouse button. Duplicate keys are rejected."))]
									+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
									[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ControlBindingMessage); }).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
									+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 16.0f)
									[SNew(SBox).WidthOverride(270.0f)[MakeMenuButton(TEXT("RESET TO DEFAULTS"), FOnClicked::CreateLambda([this]()
									{
										FlickControlBindings::ResetToDefaults();
										if (PlayerController.IsValid()) PlayerController->RefreshControlBindings();
										ControlBindingMessage = TEXT("ALL CONTROLS RESET TO DEFAULT");
										return FReply::Handled();
									}), false, false, 44.0f)]]
									+ SVerticalBox::Slot().AutoHeight()[ControlRows]
								]
							]
							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								.Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel || SelectedSettingsTab == EFlickSettingsTab::Camera || SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SVerticalBox::Slot().FillHeight(1.0f)
								[
									SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel ? EVisibility::Visible : EVisibility::Collapsed; })[SectionHeading(TEXT("01"), TEXT("GAMEPLAY"), TEXT("Tune aiming and the physical feedback of every shot."))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[SectionHeading(TEXT("02"), TEXT("CAMERA"), TEXT("Adjust camera motion, orbit response, and free-camera control."))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[SectionHeading(TEXT("03"), TEXT("INTERFACE"), TEXT("Choose the guides and information shown during play."))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeToggleRow(TEXT("AIM AND CONTACT GUIDE"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return Checked(GameMode.IsValid() ? GameMode->IsAimGuideEnabled() : I && I->IsAimGuideEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetAimGuideEnabled(!GameMode->IsAimGuideEnabled()); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetAimGuideEnabled(!I->IsAimGuideEnabled()); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel ? EVisibility::Visible : EVisibility::Collapsed; })[MakeToggleRow(TEXT("WORLD IMPACT EFFECTS"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return Checked(GameMode.IsValid() ? GameMode->AreImpactEffectsEnabled() : I && I->AreImpactEffectsEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetImpactEffectsEnabled(!GameMode->AreImpactEffectsEnabled()); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetImpactEffectsEnabled(!I->AreImpactEffectsEnabled()); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeToggleRow(TEXT("CONTROL OVERVIEW"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return Checked(GameMode.IsValid() ? GameMode->IsControlOverviewEnabled() : I && I->IsControlOverviewEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetControlOverviewEnabled(!GameMode->IsControlOverviewEnabled()); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetControlOverviewEnabled(!I->IsControlOverviewEnabled()); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("CAMERA SHAKE"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetCameraShakeIntensity() : 0.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetCameraShakeIntensity(Value); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("SHOT MOUSE SENSITIVITY"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return GameMode.IsValid() ? GameMode->GetShotMouseSensitivity() : I ? I->GetShotMouseSensitivity() : 0.5f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetShotMouseSensitivity(Value); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetShotMouseSensitivity(Value); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("GAMEPLAY CAMERA SENSITIVITY"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetGameplayCameraSensitivity() : 0.35f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetGameplayCameraSensitivity(Value); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("FREE CAMERA LOOK"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetFreeCameraLookSensitivity() : 0.33f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetFreeCameraLookSensitivity(Value); }))]]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("FREE CAMERA MOVEMENT"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetFreeCameraMoveSensitivity() : 0.38f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetFreeCameraMoveSensitivity(Value); }))]]
									]
								]

							]
							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().FillHeight(1.0f)
								[
									SNew(SBorder).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Sound ? EVisibility::Visible : EVisibility::Collapsed; }).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT("02"), TEXT("SOUND"), TEXT("Set the balance of the arena and the interface."))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("MASTER"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return GameMode.IsValid() ? GameMode->GetMasterVolume() : I ? I->GetMasterVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetMasterVolume(Value); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetMasterVolume(Value); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("PHYSICS EFFECTS"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetEffectsVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetEffectsVolume(Value); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("INTERFACE"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetInterfaceVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetInterfaceVolume(Value); }))]
									]
								]
								+ SVerticalBox::Slot().FillHeight(1.0f)
								[
									SNew(SBorder).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Display ? EVisibility::Visible : EVisibility::Collapsed; }).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT("03"), TEXT("DISPLAY"), TEXT("Display changes apply below. Graphics quality saves immediately."))]
										+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("V-SYNC"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { return Checked(GameMode.IsValid() && GameMode->IsVSyncEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->ToggleVSync(); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("WINDOW MODE"), TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetWindowModeLabel() : TEXT("WINDOWED")); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleWindowMode(-1); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleWindowMode(1); return FReply::Handled(); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("RESOLUTION"), TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetResolutionLabel() : TEXT("1920 x 1080")); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleResolution(-1); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleResolution(1); return FReply::Handled(); }))]
										+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("RENDERING QUALITY  //  CHANGES SAVE IMMEDIATELY"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)]
										+ SVerticalBox::Slot().AutoHeight()[GraphicsRows]
									]
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 18.0f)
					[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.55f))]]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(150.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CloseSettings(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu; return FReply::Handled(); }), false, false, 52.0f)]]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(24.0f, 0.0f)
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedSettingsTab == EFlickSettingsTab::Controls
								? TEXT("Controls save immediately.") : (SelectedSettingsTab == EFlickSettingsTab::Display ? TEXT("Graphics quality saves immediately. Apply display changes above.") : TEXT("Gameplay and audio update immediately."))); }).Font(UiFont(11)).ColorAndOpacity(Muted)]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(280.0f)
							[MakeMenuButton(TEXT("APPLY SETTINGS"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ApplyDisplaySettings(); return FReply::Handled(); }), true, false, 52.0f)]
						]
					]
				]
			]
		];
}
