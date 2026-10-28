//Lineups, classes, settings
#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickLineupWidgets.h"

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
	const auto NameEditor = SNew(SEditableTextBox).Tag(TEXT("LineupName")).Style(&LineupNameStyle)
		.Text_Lambda([this]() { return FText::FromString(GetClassDisplayName(GetDisplayedLoadoutPreset())); })
		.HintText(FText::FromString(TEXT("LINEUP NAME"))).SelectAllTextWhenFocused(true)
		.OnVerifyTextChanged_Lambda([](const FText& Text, FText& Error) {
			if (Text.ToString().Len() <= 16) return true;
			Error = FText::FromString(TEXT("LINEUP NAMES ARE LIMITED TO 16 CHARACTERS")); return false;
		})
		.OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type) {
			if (GameMode.IsValid()) GameMode->SetClassName(GameMode->GetLoadoutEditingPreset(), Text.ToString());
			else if (auto* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr)
				Instance->SetClassName(RemoteLoadoutPreset, Text.ToString());
		});
	TSharedRef<SButton> BackButton = SNew(SButton).Tag(TEXT("LineupBack")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() {
			if (GameMode.IsValid()) GameMode->CloseLoadout(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu;
			return FReply::Handled();
		});
	BackButton->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(BackButton)));
	return SNew(SOverlay)
		+ SOverlay::Slot()[FlickPlaylistWidgets::Background()]
		+ SOverlay::Slot().Padding(30, 18, 30, 28)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[FrontendBrandHeader(TEXT("HOME  //  LINEUPS"))]
			+ SVerticalBox::Slot().AutoHeight().Padding(12, 6, 0, 4)
			[SNew(STextBlock).Text(FText::FromString(TEXT("BUILD YOUR ADVANTAGE"))).Font(DisplayFont(36)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(12, 0, 0, 12)
			[
				SNew(SBox).WidthOverride(450).HeightOverride(30).HAlign(HAlign_Fill)
				[SNew(SFlickAngularBorder).BackgroundColor(Ink.CopyWithNewOpacity(.9f)).AccentColor(Cyan.CopyWithNewOpacity(.65f))
					.UseAccentForOutline(true).CutSize(7).Padding(FMargin(10, 2))
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1)[NameEditor]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0)
						[SNew(STextBlock).Text(FText::FromString(TEXT("\u270E"))).Font(UiFont(14)).ColorAndOpacity(Cyan)]]]
			]
			+ SVerticalBox::Slot().FillHeight(1)[BuildLoadoutWorkspace()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
			[
				SNew(SBox).HeightOverride(52)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(178)[BackButton]]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Center)
					[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d PUCKS  //  %s"),
						GameMode.IsValid() ? GameMode->GetLoadoutEditingPieceCount() : 4, *GetClassDisplayName(GetDisplayedLoadoutPreset()))); })
						.Font(DisplayFont(10)).ColorAndOpacity(Muted)]
					+ SHorizontalBox::Slot().AutoWidth()
					[SNew(SBox).WidthOverride(260)[MakeMenuButton(TEXT("SAVE LINEUP"), FOnClicked::CreateLambda([this]() {
						if (GameMode.IsValid()) { GameMode->PlayMenuSound(true); GameMode->CloseLoadout(); }
						else RemotePartyScreen = EFlickFrontendScreen::MainMenu;
						return FReply::Handled(); }), true, false, 52)]]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutWorkspace()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(FlickLineupStyle::WorkspaceHeight)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(.30f).Padding(0, 0, 8, 0)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]() { return bReadableLineup ? EVisibility::Visible : EVisibility::Collapsed; })[BuildLineupInspector()]]
					+ SOverlay::Slot()
					[
						SNew(SBox).Visibility_Lambda([this]() { return bReadableLineup ? EVisibility::Collapsed : EVisibility::Visible; })
						[SNew(SOverlay)
							+ SOverlay::Slot()[BuildLineupRadar()]
							+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(16, 10)
							[SNew(SButton).Tag(TEXT("LineupViewToggle")).ButtonStyle(&TransparentButtonStyle)
								.OnClicked_Lambda([this]() { bReadableLineup = true; return FReply::Handled(); })
								[SNew(STextBlock).Text(FText::FromString(TEXT("PUCK //"))).Font(DisplayFont(9)).ColorAndOpacity(Cyan)]]]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(.33f).Padding(8, 0)[BuildLoadoutFormation(EFlickTeam::Player1)]
				+ SHorizontalBox::Slot().FillWidth(.37f).Padding(8, 0, 0, 0)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1)[BuildLoadoutComparison(EFlickTeam::Player1)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 0)[SNew(SBox).HeightOverride(36)[BuildLoadoutPresetBar(EFlickTeam::Player1)]]]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1).Padding(0, 10, 0, 0)[BuildArchetypePicker(EFlickTeam::Player1)];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLineupInspector()
{
	return SNew(SFlickAngularBorder).BackgroundColor(Panel).AccentColor(Cyan.CopyWithNewOpacity(.55f))
		.UseAccentForOutline(true).CutSize(14).Padding(1.5f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
		[SNew(SFlickPuckThumbnail).View(EFlickPuckThumbnailView::Hero).CutSize(12.5f).RollDegrees(12)
				.EdgeFade(FMargin(0,85,0,62))
				.Archetype_Lambda([this]() { return GetPreviewLoadoutArchetype(EFlickTeam::Player1); })
				.Skin_Lambda([this]() { return SelectedPuckSkins[static_cast<int32>(GetPreviewLoadoutArchetype(EFlickTeam::Player1))]; })]
			+ SOverlay::Slot().VAlign(VAlign_Top).Padding(20, 12)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)
					[SNew(STextBlock).Text(FText::FromString(TEXT("//  PUCK PREVIEW"))).Font(DisplayFont(11)).ColorAndOpacity(Cyan)]
					+ SHorizontalBox::Slot().AutoWidth()
					[SNew(SButton).Tag(TEXT("LineupViewToggle")).ButtonStyle(&TransparentButtonStyle)
						.OnClicked_Lambda([this]() { bReadableLineup = false; return FReply::Handled(); })
						[SNew(STextBlock).Text(FText::FromString(TEXT("RADAR //"))).Font(DisplayFont(8)).ColorAndOpacity(Muted)]]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
				[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(GetPieceArchetypeName(GetPreviewLoadoutArchetype(EFlickTeam::Player1))); }).Font(DisplayFont(26)).ColorAndOpacity(Paper)]
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(EFlickTeam::Player1)).ClassLabel); })
					.Font(DisplayFont(11)).ColorAndOpacity(Cyan)]
			]
			+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(20, 12)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(UiFont(10)).ColorAndOpacity(Paper).AutoWrapText(true)
					.Text_Lambda([this]() {
						switch (GetPreviewLoadoutArchetype(EFlickTeam::Player1))
						{
						case EFlickPieceArchetype::Heavy: return FText::FromString(TEXT("Heavy contact; slower shots. More mass and friction, but a lower launch speed."));
						case EFlickPieceArchetype::Striker: return FText::FromString(TEXT("Fast, lively shots. Less mass means you are also easier to displace."));
						case EFlickPieceArchetype::Grippy: return FText::FromString(TEXT("Stops sooner. Higher friction helps limit drift, but reduces travel distance."));
						case EFlickPieceArchetype::Slider: return FText::FromString(TEXT("Coasts farther. Low friction preserves movement; watch your power near the rim."));
						case EFlickPieceArchetype::Blocker: return FText::FromString(TEXT("Covers more space. Its wider footprint is useful for blocking, but it is lighter and slower."));
						case EFlickPieceArchetype::Compact: return FText::FromString(TEXT("Fits tighter gaps. A smaller footprint also covers less of the board."));
						case EFlickPieceArchetype::Bouncer: return FText::FromString(TEXT("Livelier rebounds. Useful for banks, but harder to bring to a controlled stop."));
						case EFlickPieceArchetype::Toppler: return FText::FromString(TEXT("Tall and easier to tip. Its raised centre of mass and lower angular damping favour tumbling."));
						default: return FText::FromString(TEXT("The balanced baseline. Normal size, mass and launch speed; no specialised advantage or penalty."));
						}
					})]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
				[SNew(STextBlock).Font(DisplayFont(8)).ColorAndOpacity(Muted).AutoWrapText(true)
					.Text_Lambda([this]() { return FText::FromString(GetPreviewLoadoutArchetype(EFlickTeam::Player1) == GetDisplayedLoadoutPiece(GetSelectedLoadoutSlot(EFlickTeam::Player1))
						? TEXT("EQUIPPED  //  HOVER TO COMPARE. CLICK TO REPLACE.") : TEXT("PREVIEW ONLY  //  CLICK TO EQUIP. WHITE MARKER = EQUIPPED.")); })]
			]
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
	auto Formation = SNew(SConstraintCanvas);
	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		Formation->AddSlot()
			.Anchors(TAttribute<FAnchors>::CreateLambda([this, SlotIndex]() {
				const FVector2D Position = FlickLineupStyle::SlotPosition(SlotIndex, GameMode.IsValid() && GameMode->GetLoadoutEditingPieceCount() == 3);
				return FAnchors(Position.X, Position.Y);
			}))
			.Alignment(FVector2D(.5f, .5f)).Offset(FMargin(0, 0, FlickLineupStyle::SlotSize, FlickLineupStyle::SlotSize))
			[BuildFormationPuck(Team, SlotIndex)];
	}
	return FlickLineupWidgets::Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(FText::FromString(TEXT("//  STARTING FORMATION"))).Font(DisplayFont(11)).ColorAndOpacity(Cyan)]
		+ SVerticalBox::Slot().FillHeight(1).Padding(0, 5)
		[
			SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center)
			[SNew(SBox).WidthOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(.5f))]]
			+ SOverlay::Slot().VAlign(VAlign_Center).Padding(32, 0)
			[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(.5f))]]
			+ SOverlay::Slot()[Formation]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
		[SNew(STextBlock).Text_Lambda([this, Team]() {
			const int32 Slot = GetSelectedLoadoutSlot(Team);
			return FText::FromString(FString::Printf(TEXT("SLOT %02d  //  %s"), Slot + 1, *GetPieceArchetypeName(GetDisplayedLoadoutPiece(Slot))));
		}).Font(DisplayFont(12)).ColorAndOpacity(Cyan)]);
}

TSharedRef<SWidget> SFlickGameLayer::BuildFormationPuck(const EFlickTeam Team, const int32 SlotIndex)
{
	const auto Selected = [this, Team, SlotIndex]() { return GetSelectedLoadoutSlot(Team) == SlotIndex; };
	return SNew(SBox).Visibility_Lambda([this, SlotIndex]() { return GetLoadoutRowVisibility(SlotIndex); })
	[
		SNew(SButton).Tag(FName(*FString::Printf(TEXT("LineupSlot%d"), SlotIndex))).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
		.ToolTipText_Lambda([this, SlotIndex]() { return FText::FromString(FString::Printf(TEXT("SLOT %02d // %s"), SlotIndex + 1, *GetPieceArchetypeName(GetDisplayedLoadoutPiece(SlotIndex)))); })
		.OnClicked_Lambda([this, Team, SlotIndex]() { SelectLoadoutSlot(Team, SlotIndex); return FReply::Handled(); })
		[
			SNew(SFlickAngularBorder).CutSize(12).Padding(1.5f).DrawNeutralOutline(false)
			.BackgroundColor_Lambda([Selected]() { return Selected() ? Brand.CopyWithNewOpacity(.065f) : FLinearColor::Transparent; })
			.AccentColor_Lambda([Selected]() { return Selected() ? Brand : FLinearColor::Transparent; })
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(132).HeightOverride(132)
					[SNew(SFlickPuckThumbnail).View(EFlickPuckThumbnailView::Locker).Zoom(1.25f).EdgeFade(FMargin(18))
						.Archetype_Lambda([this, SlotIndex]() { return GetDisplayedLoadoutPiece(SlotIndex); })
						.Skin_Lambda([this, SlotIndex]() { return SelectedPuckSkins[static_cast<int32>(GetDisplayedLoadoutPiece(SlotIndex))]; })]]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(10, 8)
				[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%02d"), SlotIndex + 1))).Font(DisplayFont(11))
					.ColorAndOpacity_Lambda([Selected]() { return Selected() ? Brand : Cyan; })]
				+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).Active_Lambda(Selected)]
			]
		]
	];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutComparison(const EFlickTeam Team)
{
	return FlickLineupWidgets::Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(FString::Printf(TEXT("//  SLOT %02d"), GetSelectedLoadoutSlot(Team) + 1)); }).Font(DisplayFont(11)).ColorAndOpacity(Cyan)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(GetPieceArchetypeName(GetPreviewLoadoutArchetype(Team))); }).Font(DisplayFont(27)).ColorAndOpacity(Paper)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(SFlickAngularBorder).CutSize(5).Padding(FMargin(15, 3)).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.65f)).BackgroundColor(Ink)
				[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).ClassLabel); }).Font(DisplayFont(10)).ColorAndOpacity(Cyan)]]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Summary); }).Font(UiFont(11)).ColorAndOpacity(Paper)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
		[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(TEXT("+  ") + FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Strengths); })
			.Font(DisplayFont(10)).ColorAndOpacity(FLinearColor(.18f, .95f, .58f))]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(TEXT("-  ") + FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Weaknesses); })
			.Font(DisplayFont(10)).ColorAndOpacity(Orange)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 7)
		[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(.5f))]]
		+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("SPEED"), 0)]
		+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("WEIGHT"), 1)]
		+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("IMPACT"), 2)]
		+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("CONTROL"), 3)]
		+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("COAST"), 4)]
		+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("STABILITY"), 5)]);
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutStatRow(const EFlickTeam Team, const FString& Label, const int32 StatIndex)
{
	const auto Comparing = [this, Team]() { return Team == EFlickTeam::Player1 ? Player1HoveredLoadoutArchetype.IsSet() : Player2HoveredLoadoutArchetype.IsSet(); };
	return SNew(SBox).HeightOverride(FlickLineupStyle::StatRowHeight)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(100)[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(10)).ColorAndOpacity(Muted)]]
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
		[SNew(SBox).HeightOverride(9)
			[SNew(FlickLineupWidgets::SStatMeter).Comparing_Lambda(Comparing)
				.Equipped_Lambda([this, Team, StatIndex]() { return GetLoadoutStatValue(Team, StatIndex, false); })
				.Preview_Lambda([this, Team, StatIndex]() { return GetLoadoutStatValue(Team, StatIndex, true); })]]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7, 0, 0, 0)
		[
			SNew(SBox).WidthOverride(26).Visibility_Lambda([Comparing]() { return Comparing() ? EVisibility::Visible : EVisibility::Collapsed; })
			[SNew(STextBlock).Text_Lambda([this, Team, StatIndex]() {
				const int32 Delta = FMath::RoundToInt((GetLoadoutStatValue(Team, StatIndex, true) - GetLoadoutStatValue(Team, StatIndex, false)) * 100);
				return FText::FromString(Delta > 0 ? FString::Printf(TEXT("+%d"), Delta) : FString::FromInt(Delta));
			}).Font(DisplayFont(9)).ColorAndOpacity_Lambda([this, Team, StatIndex]() {
				const float Delta = GetLoadoutStatValue(Team, StatIndex, true) - GetLoadoutStatValue(Team, StatIndex, false);
				return Delta > .01f ? FLinearColor(.2f, .95f, .55f) : Delta < -.01f ? Orange : Muted;
			})]
		]
	];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutPresetBar(const EFlickTeam Team)
{
	auto Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(3, 0));
	int32 Column = 0;
	for (const EFlickLineupPreset Preset : {EFlickLineupPreset::Balanced, EFlickLineupPreset::Power, EFlickLineupPreset::Speed, EFlickLineupPreset::Control})
	{
		TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("LineupPreset%d"), Column)))
			.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
			.HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.OnClicked_Lambda([this, Team, Preset]() {
				if (Team == EFlickTeam::Player1) Player1HoveredLoadoutArchetype.Reset(); else Player2HoveredLoadoutArchetype.Reset();
				if (GameMode.IsValid()) GameMode->SelectLoadoutEditingPreset(Preset); else RemoteLoadoutPreset = Preset;
				return FReply::Handled();
			});
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SFlickAngularBorder).Tag(FName(*FString::Printf(TEXT("LineupPresetSurface%d"),Column)))
				.CutSize(4).Padding(4).UseAccentForOutline(true).RefinedSurface(true)
			.AccentColor_Lambda([this, Preset, WeakButton]() {
				const auto Pinned = WeakButton.Pin();
				return GetDisplayedLoadoutPreset() == Preset || (Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus())) ? Brand : Cyan.CopyWithNewOpacity(.65f);
			})
			.BackgroundColor_Lambda([this, Preset]() { return FlickLineupStyle::PresetSurface(GetDisplayedLoadoutPreset() == Preset); })
			[SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(STextBlock).Text_Lambda([this, Preset]() { return FText::FromString(GetClassDisplayName(Preset)); })
					.Font(DisplayFont(11)).Justification(ETextJustify::Center).ColorAndOpacity(Paper)]]]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).CutSize(4)
				.Active_Lambda([this,Preset]() { return GetDisplayedLoadoutPreset()==Preset; })]);
		Grid->AddSlot(Column++, 0)[Button];
	}
	return Grid;
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassPlayerRow(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	TSharedRef<SUniformGridPanel> ClassGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f));
	int32 Column = 0;
	for (const EFlickLineupPreset Preset : {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control})
	{
		TSharedRef<SButton> ClassButton = SNew(SButton).Tag(FName(*FString::Printf(TEXT("ClassPreset%d"), Column)))
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Cursor(EMouseCursor::Hand)
			.OnClicked_Lambda([this, Preset]()
			{
				if (PlayerController.IsValid())
				{
					PlayerController->RequestSelectClass(Preset);
				}
				return FReply::Handled();
			});

		const TWeakPtr<SButton> WeakClassButton = ClassButton;
		ClassButton->SetContent(SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SFlickAngularBorder).RefinedSurface(true)
			.Tag(FName(*FString::Printf(TEXT("ClassPresetSurface%d"), Column)))
			.BackgroundColor_Lambda([this, Preset]() { return FlickLineupStyle::PresetSurface(GetSelectedClassDraft() == Preset); })
			.AccentColor_Lambda([this, Preset, WeakClassButton]()
			{
				const TSharedPtr<SButton> Pinned = WeakClassButton.Pin();
				const bool bSelected = GetSelectedClassDraft() == Preset;
				return bSelected || (Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()))
					? Brand
					: Hairline;
			})
			.CutSize(8.0f)
			.BorderWidth(1.0f)
			.UseAccentForOutline(true)
			.Padding(FMargin(12, 8))
			[SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(STextBlock).Text_Lambda([this, Preset]() { return FText::FromString(GetClassDisplayName(Preset)); })
				.Font(DisplayFont(20)).OverflowPolicy(ETextOverflowPolicy::Ellipsis).ColorAndOpacity(Paper)]]]
		+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).CutSize(8)
			.Active_Lambda([this, Preset]() { return GetSelectedClassDraft() == Preset; })]);
		ClassGrid->AddSlot(Column++, 0)[ClassButton];
	}

	return SNew(SBox)
		.HeightOverride(78.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(Ink.CopyWithNewOpacity(.95f)).RefinedSurface(true)
			.AccentColor(Cyan.CopyWithNewOpacity(.7f)).UseAccentForOutline(true)
			.CutSize(14.0f)
			.BorderWidth(0.8f)
			.Padding(FMargin(12.0f, 9.0f))[ClassGrid]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassPuckCard(const int32 PieceSlot)
{
	return SNew(SBox)
		.HeightOverride(185.0f)
		[
			SNew(SFlickAngularBorder).RefinedSurface(true)
			.BackgroundColor(Ink.CopyWithNewOpacity(.96f))
			.AccentColor_Lambda([this, PieceSlot]()
			{
				return FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam())).CopyWithNewOpacity(0.72f);
			})
			.CutSize(8.0f)
			.BorderWidth(0.9f)
			.UseAccentForOutline(true)
			.Padding(2.0f)
			[
				SNew(SOverlay)
				// Match the locker's near-top-down product display, keeping each
				// class symbol and equipped finish readable beside the native labels.
				+ SOverlay::Slot().HAlign(HAlign_Left)
				[
					SNew(SBox).WidthOverride(220.0f)
					[
						SNew(SFlickPuckThumbnail).View(EFlickPuckThumbnailView::Locker)
							.Tag(FName(*FString::Printf(TEXT("ClassPuckPortrait%d"), PieceSlot)))
							.CutSize(6).Zoom(1.1f).EdgeFade(FMargin(18,12,36,12))
							.Archetype_Lambda([this, PieceSlot]() { return GetSelectedClassPiece(PieceSlot); })
							.Skin_Lambda([this, PieceSlot]()
							{
								return SelectedPuckSkins[static_cast<int32>(GetSelectedClassPiece(PieceSlot))];
							})
					]
				]
				+ SOverlay::Slot().VAlign(VAlign_Center).Padding(228.0f, 10.0f, 12.0f, 10.0f)
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
						.Font(DisplayFont(12))
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
						.Font(UiFont(9))
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
	constexpr float BarWidth = 278.0f;
	return SNew(SBox).HeightOverride(31.0f)
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
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(12.0f)
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
							.BorderBackgroundColor(Ice)
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

	return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.65f).Padding(0.0f, 0.0f, 14.0f, 0.0f)
			[
				SNew(SFlickAngularBorder).RefinedSurface(true)
				.BackgroundColor(Ink.CopyWithNewOpacity(.96f)).AccentColor(Cyan.CopyWithNewOpacity(.7f))
				.UseAccentForOutline(true).CutSize(16).Padding(FMargin(16, 13))
				[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("//  YOUR FOUR-PUCK LINEUP"))).Font(UiFont(11, true)).ColorAndOpacity(Ice)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					PuckGrid
				]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.35f)
			[
				SNew(SFlickAngularBorder).RefinedSurface(true)
				.BackgroundColor(Ink.CopyWithNewOpacity(.97f))
				.AccentColor(Cyan.CopyWithNewOpacity(.7f)).UseAccentForOutline(true)
				.CutSize(16.0f)
				.BorderWidth(0.8f)
				.Padding(FMargin(32.0f, 24.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock).Tag(TEXT("ClassProfileTitle"))
						.Text_Lambda([this]() { return FText::FromString(GetClassDisplayName(GetSelectedClassDraft())); })
						.Font(DisplayFont(30))
						.AutoWrapText(true)
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)
					[SNew(STextBlock).Text(FText::FromString(TEXT("//  LINEUP PROFILE"))).Font(UiFont(11, true)).ColorAndOpacity(Ice)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("SPEED"), 0)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("WEIGHT"), 1)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("IMPACT"), 2)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("CONTROL"), 3)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("COAST"), 4)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("STABILITY"), 5)]
					+ SVerticalBox::Slot().FillHeight(1)[SNew(SSpacer)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 18)
					[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)]]
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(TEXT("Your four pucks. Your opening advantage.")))
						.Font(UiFont(12)).ColorAndOpacity(Paper).AutoWrapText(true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 10)
					[SNew(STextBlock).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
						.Text_Lambda([this]() {
							if (GameMode.IsValid() && GameMode->IsChangingClassForNextRound())
								return FText::FromString(TEXT("Your new class applies when the next round starts."));
							const AFlickGameState* State = GetScoreboardGameState();
							if (State && (State->bNetworkClassSelectionActive || State->bPrivateMatchAssignmentActive))
								return FText::FromString(TEXT("Choose your saved class for this match."));
							return FText::FromString(GameMode.IsValid() && GameMode->IsPreparingTrainingBotMatch()
								? TEXT("The bot receives a random class.") : TEXT("Other local players receive random classes."));
						})]
				]
			]
		;
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassSelect()
{
	const EFlickTeam SelectionTeam = GetClassSelectionTeam();
	const int32 PlayerSlot = GetClassSelectionPlayerSlot();
	const TSharedRef<SButton> BackButton = SNew(SButton).Tag(TEXT("ClassBack"))
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
		.OnClicked_Lambda([this]() { if (GameMode.IsValid()) GameMode->CancelClassSelection(); return FReply::Handled(); });
	BackButton->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(BackButton)));
	const TSharedRef<SButton> ConfirmButton = SNew(SButton).Tag(TEXT("ClassConfirm"))
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).HAlign(HAlign_Fill).VAlign(VAlign_Fill).Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestConfirmClass(); return FReply::Handled(); });
	const TWeakPtr<SButton> WeakConfirm = ConfirmButton;
	ConfirmButton->SetContent(SNew(SFlickAngularBorder).CutSize(10).AccentColor(Brand).UseAccentForOutline(true).Padding(FMargin(20, 0))
		.BackgroundColor_Lambda([WeakConfirm]() { const auto Pinned = WeakConfirm.Pin();
			return Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()) ? Paper : Brand; })
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 16, 0)
			[SNew(SBox).WidthOverride(5).HeightOverride(22)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink)]]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("CONFIRM CLASS"))).Font(DisplayFont(17)).ColorAndOpacity(Ink)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(FlickPlaylistWidgets::SCardArrow).Divider(false).Color(Ink)]]);
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.004f, 0.01f, 0.88f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::DownOnly)
			[
				SNew(SBox).WidthOverride(1440.0f).HeightOverride(820.0f)
				[
					SNew(SBox)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
						[FrontendBrandHeader(TEXT("PLAY  //  SELECT CLASS"))]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("CHOOSE YOUR LINEUP.")))
									.Font(DisplayFont(40)).ColorAndOpacity(Paper)
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text_Lambda([this]()
									{
										return FText::FromString(TEXT("Pick a saved class. Every puck has a purpose."));
									})
									.Font(UiFont(13)).ColorAndOpacity(Muted)
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SFlickAngularBorder).BackgroundColor(Ink).AccentColor(Cyan).UseAccentForOutline(true)
								.RefinedSurface(true).CutSize(7).Padding(FMargin(22, 10))
								[SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const EFlickTeam Team = GetClassSelectionTeam();
									return FText::FromString(FString::Printf(
										TEXT("%s TEAM  //  PLAYER %d"),
										Team == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
										GetClassSelectionPlayerSlot() + 1));
								})
								.Font(UiFont(10, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 12.0f)
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
								.WidthOverride(190.0f).HeightOverride(56)[BackButton]
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
								SNew(SBox).WidthOverride(280.0f).HeightOverride(56)[ConfirmButton]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildArchetypePicker(const EFlickTeam Team)
{
	auto Rows = SNew(SVerticalBox);
	for (int32 Row = 0; Row < 2; ++Row)
	{
		auto Choices = SNew(SHorizontalBox);
		for (int32 Index = Row == 0 ? 0 : 5; Index < (Row == 0 ? 5 : FlickPieceArchetypeRules::ArchetypeCount); ++Index)
			Choices->AddSlot().FillWidth(1).Padding(4, 0)[BuildArchetypeChoice(Team, static_cast<EFlickPieceArchetype>(Index))];
		Rows->AddSlot().FillHeight(1).Padding(0, Row == 0 ? 0 : 8, 0, 0)[Choices];
	}
	return FlickLineupWidgets::Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 8)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)
			[SNew(STextBlock).Text(FText::FromString(TEXT("//  AVAILABLE PUCK TYPES"))).Font(DisplayFont(11)).ColorAndOpacity(Cyan)]
			+ SHorizontalBox::Slot().AutoWidth()
			[SNew(STextBlock).Text_Lambda([this, Team]() { return FText::FromString(FString::Printf(TEXT("SELECT A PUCK  //  SLOT %02d"), GetSelectedLoadoutSlot(Team) + 1)); })
				.Font(DisplayFont(9)).ColorAndOpacity(Muted)]
		]
		+ SVerticalBox::Slot().FillHeight(1)[Rows], FMargin(6, 8, 6, 9));
}

TSharedRef<SWidget> SFlickGameLayer::BuildArchetypeChoice(const EFlickTeam Team, const EFlickPieceArchetype Archetype)
{
	const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
	const FLinearColor Accent = FlickPieceArchetypeRules::GetVisualAccent(Archetype, GetTeamAccent(Team));
	const auto Equipped = [this, Team, Archetype]() { return GetDisplayedLoadoutPiece(GetSelectedLoadoutSlot(Team)) == Archetype; };
	TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("LineupChoice%d"), static_cast<int32>(Archetype))))
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand).ToolTipText(FText::FromString(Rules.Summary))
		.OnHovered_Lambda([this, Team, Archetype]() { SetHoveredLoadoutArchetype(Team, Archetype); })
		.OnUnhovered_Lambda([this, Team, Archetype]() { ClearHoveredLoadoutArchetype(Team, Archetype); })
		.OnClicked_Lambda([this, Team, Archetype]() {
			if (GameMode.IsValid()) { GameMode->SetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team), Archetype); GameMode->PlayMenuSound(false); }
			else SetDisplayedLoadoutPiece(GetSelectedLoadoutSlot(Team), Archetype);
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakButton = Button;
	const auto Active = [Equipped, WeakButton]() { const auto Pinned = WeakButton.Pin(); return Equipped() || (Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus())); };
	static const FSlateRoundedBoxBrush CheckBackground(Brand, 10.f);
	Button->SetContent(SNew(SFlickAngularBorder).CutSize(10).Padding(1.5f).UseAccentForOutline(true)
		.BackgroundColor_Lambda([Active]() { return Active() ? PanelRaised : Panel; })
		.AccentColor_Lambda([Active]() { return Active() ? Brand : Hairline; })
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SBox).WidthOverride(144)
					[SNew(SFlickPuckThumbnail).Archetype(Archetype).View(EFlickPuckThumbnailView::Locker).CutSize(8.5f).EdgeFade(FMargin(12,8,24,8))
						.Skin_Lambda([this, Archetype]() { return SelectedPuckSkins[static_cast<int32>(Archetype)]; })]]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(9, 8, 9, 8)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(GetPieceArchetypeName(Archetype))).Font(DisplayFont(13)).ColorAndOpacity(Accent)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
					[SNew(STextBlock).Text(FText::FromString(Rules.ClassLabel)).Font(DisplayFont(9)).ColorAndOpacity(Muted)]
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(8)
			[
				SNew(SBox).WidthOverride(18).HeightOverride(18).Visibility_Lambda([Equipped]() { return Equipped() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[SNew(SBorder).BorderImage(&CheckBackground).Padding(0).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(FText::FromString(TEXT("\u2713"))).Font(UiFont(10, true)).ColorAndOpacity(Ink).Justification(ETextJustify::Center)]]
			]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).Active_Lambda(Active)]
		]);
	return SNew(SBox).MinDesiredHeight(FlickLineupStyle::ChoiceHeight)[Button];
}
