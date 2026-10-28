// main menu, profile, social, shop
#include "UI/FlickGameLayerPrivate.h"
#include "Misc/Paths.h"
#include "UI/FlickCosmeticWidgets.h"
#include "UI/FlickLockerStyle.h"
#include "UI/FlickProfileStyle.h"
#include "UI/FlickProfileWidgets.h"
#include "UI/FlickPlaylistWidgets.h"

static TAutoConsoleVariable<int32> CVarFlickMainMenuBackdrop(
	TEXT("flick.MainMenuBackdrop"), 0,
	TEXT("Show the full-screen main-menu tint and decorative backdrop (0 = off, 1 = on)."),
	ECVF_Default);

namespace
{
	TSharedRef<SWidget> SocialAction(const FString& Label, FOnClicked Click,
		const FButtonStyle* ButtonStyle, FLinearColor Accent = Cyan, bool bPrimary = false)
	{
		TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(ButtonStyle)
			.ContentPadding(0).OnClicked(Click).Tag(FName(*FString(TEXT("SocialAction_") + Label)));
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(7).UseAccentForOutline(true)
			.BackgroundColor_Lambda([WeakButton, bPrimary]()
			{ const auto B = WeakButton.Pin(); return bPrimary ? (B && (B->IsHovered() || B->HasKeyboardFocus()) ? FLinearColor(.85f, 1, .14f) : Brand) : Ink; })
			.AccentColor_Lambda([WeakButton, Accent, bPrimary]()
			{ const auto B = WeakButton.Pin(); return bPrimary || (B && (B->IsHovered() || B->HasKeyboardFocus())) ? Brand : Accent.CopyWithNewOpacity(.6f); })
			.Padding(FMargin(10, 6))
			[SNew(SHorizontalBox)
			 + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			 [SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(9, true)).ColorAndOpacity(bPrimary ? Ink : Paper)]
			 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
			 [SNew(SBox).WidthOverride(14).HeightOverride(14)
			  [SNew(FlickPlaylistWidgets::SCardArrow).Divider(false).Color(bPrimary ? Ink : Accent)]]]);
		return Button;
	}

	const FSlateBrush* GetCosmeticImageBrush(const int32 Category, const int32 Index)
	{
		static const TCHAR* BannerFiles[] = {
			TEXT("Blue ice.png"), TEXT("cold.png"), TEXT("Crystal.png"), TEXT("Fire red.png"),
			TEXT("Red crystal.png"), TEXT("Robotic.png"), TEXT("rock.png"), TEXT("tundra.png"),
			TEXT("CryoCircuit.png"), TEXT("SolarForge.png"), TEXT("PhaseRift.png")};
		static const TCHAR* BorderFiles[] = {
			TEXT("gold.png"), TEXT("ice.png"), TEXT("purple crystal.png"), TEXT("robotic.png")};
		const int32 AssetIndex = Index - 3; // The original three procedural styles retain their save indices.
		const bool bBanner = Category == 0;
		if (Category != 0 && Category != 2) return nullptr;
		if (AssetIndex < 0 || AssetIndex >= (bBanner ? UE_ARRAY_COUNT(BannerFiles) : UE_ARRAY_COUNT(BorderFiles))) return nullptr;
		static TMap<FString, TSharedPtr<FSlateDynamicImageBrush>> Brushes;
		const FString Path = FPaths::ProjectContentDir() / TEXT("UI") /
			(bBanner ? TEXT("Banners") : TEXT("Avatar borders")) /
			(bBanner ? BannerFiles[AssetIndex] : BorderFiles[AssetIndex]);
		if (!Brushes.Contains(Path))
		{
			Brushes.Add(Path, MakeShared<FSlateDynamicImageBrush>(FName(*Path),
				bBanner ? FVector2D(2172.0f, 724.0f) : FVector2D(1254.0f, 1254.0f)));
		}
		return Brushes[Path].Get();
	}

	FLinearColor GetBannerTagColor(const int32 Index)
	{
		const int32 Collection = FlickCosmeticCatalog::GetCollection(1, Index);
		if (Collection != INDEX_NONE) return FMath::Lerp(FlickCosmeticCatalog::GetCollectionColor(Collection), FLinearColor::White, 0.3f);
		const FLinearColor Colors[] = {
			Brand, Cyan, Orange, FLinearColor(0.82f, 0.58f, 1.0f),
			FLinearColor(1.0f, 0.78f, 0.25f), Cyan, Paper, Orange,
			FLinearColor(0.5f, 0.95f, 0.72f), FLinearColor(0.82f, 0.58f, 1.0f), Brand, Paper};
		return Colors[FMath::Clamp(Index, 0, UE_ARRAY_COUNT(Colors) - 1)];
	}

	FSlateFontInfo GetBannerTagFont(const int32 Index, const int32 Size)
	{
		return Index % 3 == 0 ? DisplayFont(Size, true)
			: Index % 3 == 1 ? DisplayFont(Size) : UiFont(Size, true);
	}

	FLinearColor GetBannerBackground(const int32 Index)
	{
		return Index == 2 ? FLinearColor(0.018f, 0.075f, 0.105f, 0.97f)
			: Index == 1 ? FLinearColor(0.045f, 0.065f, 0.04f, 0.97f)
			: FLinearColor::FromSRGBColor(FColor(14, 23, 25, 247));
	}
	FLinearColor GetAvatarFrameColor(const int32 Index)
	{
		const int32 Collection = FlickCosmeticCatalog::GetCollection(2, Index);
		return Collection != INDEX_NONE ? FlickCosmeticCatalog::GetCollectionColor(Collection)
			: Index == 2 ? Brand : Index == 1 ? Cyan : Hairline;
	}
}

TSharedRef<SWidget> SFlickGameLayer::BuildStartupOverlay()
{
	// The movie-player splash owns the startup logo. Once the frontend is ready,
	// only fade its dark backdrop so a second copy cannot linger over the menu.
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 1.0f));
}

TSharedRef<SWidget> SFlickGameLayer::BuildDisplayPuckSelector()
{
	const auto Cycle = [this](int32 Direction) { if (auto* Sessions = GetDisplayedSessionSubsystem()) Sessions->CycleShowcaseArchetype(Direction); return FReply::Handled(); };
	return SNew(SBox).WidthOverride(280).HeightOverride(42)
		.RenderTransform_Lambda([this]() { return FSlateRenderTransform(FVector2D(0, IsDisplayedPartyActive() || bPartyTrayPreview ? -70.f : 0.f)); })
	[
		SNew(SFlickAngularBorder).BackgroundColor(Ink).AccentColor(Cyan.CopyWithNewOpacity(.7f)).UseAccentForOutline(true).BorderWidth(.8f).CutSize(10).Padding(FMargin(12, 4))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("DISPLAY PUCK"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9, 0, 0, 0)[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(10)).ColorAndOpacity(Cyan)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)[SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(8, 0)).ToolTipText(FText::FromString(TEXT("Previous display puck")))
				.OnClicked_Lambda([Cycle]() { return Cycle(-1); })[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(UiFont(16, true)).ColorAndOpacity(Brand)]]
			+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center).VAlign(VAlign_Center)
			[SNew(STextBlock).Text_Lambda([this]() { const auto* Sessions = GetDisplayedSessionSubsystem(); return FText::FromString(GetPieceArchetypeName(Sessions ? Sessions->GetShowcaseArchetype() : EFlickPieceArchetype::Standard)); }).Font(UiFont(10, true)).ColorAndOpacity(Paper)]
			+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)[SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(8, 0)).ToolTipText(FText::FromString(TEXT("Next display puck")))
				.OnClicked_Lambda([Cycle]() { return Cycle(1); })[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(16, true)).ColorAndOpacity(Brand)]]
		]
	];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenu()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility_Lambda([]() { return CVarFlickMainMenuBackdrop.GetValueOnGameThread() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.08f))
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility_Lambda([]() { return CVarFlickMainMenuBackdrop.GetValueOnGameThread() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.Opacity(0.9f)
		]
		+ SOverlay::Slot()
		[
			SAssignNew(MainMenuDiagonalPanel, SFlickDiagonalPanel)
			.Visibility_Lambda([this]() { return bLightingPreview ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			.PanelColor(FLinearColor::FromSRGBColor(FColor(4, 10, 14, 252)))
			.EdgeColor(FlickMainMenuStyle::Ice.CopyWithNewOpacity(0.68f))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FlickMainMenuStyle::LeftPadding, 50.0f, 0.0f, 0.0f)
		[
			SNew(SDPIScaler).DPIScale_Lambda([]() { return GetMainMenuColumnScale(); })
			.Visibility_Lambda([this]() { return bLightingPreview ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
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
					.Font(WordmarkTaglineFont(9))
					.ColorAndOpacity(FlickMainMenuStyle::Lime)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(-5.0f, 7.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(400.0f)
				.HeightOverride(133.0f)
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
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("Small move. Big consequences."))).Font(WordmarkTaglineFont(15)).ColorAndOpacity(Muted)
				.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->GetFrontendScreen() == EFlickFrontendScreen::Profile ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
			]
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
		.Padding(FlickMainMenuStyle::LeftPadding, FlickMainMenuStyle::NavigationTop, 0.0f, 0.0f)
		[
			SNew(SDPIScaler).DPIScale_Lambda([]() { return GetMainMenuColumnScale(); })
			[
			SNew(SBox)
			.WidthOverride(MainMenuStackMetrics::StackWidth)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(0))
					[
						MakeMainMenuButton(TEXT("PLAY"), FOnClicked::CreateLambda([this]()
						{
							if (IsDisplayedPartyActive() && !IsLocalDisplayedPartyLeader()) return FReply::Handled();
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
							SelectedProfileTab = EFlickProfileTab::Overview;
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
					SAssignNew(MainMenuQuitCard, SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(5))
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
			]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, FlickMainMenuStyle::SelectorBottom)
		[SNew(SBox).Visibility_Lambda([this]() { return bLightingPreview ? EVisibility::Collapsed : EVisibility::Visible; })[BuildDisplayPuckSelector()]]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(52.0f, 0.0f, 0.0f, 48.0f)
		[
			SNew(SDPIScaler).DPIScale_Lambda([]() { return GetMainMenuColumnScale(); })
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
			SAssignNew(MainMenuProfileCard, SBox).WidthOverride(410.0f).HeightOverride(82.0f)
			[
				SNew(SFlickMainMenuPanel)
				.BackgroundColor_Lambda([this]() { return GetBannerBackground(SelectedBannerStyle); })
				.Premium(true)
				.ImageBrush_Lambda([this]() { return GetCosmeticImageBrush(0, SelectedBannerStyle); })
				.AccentColor_Lambda([this]() { return SelectedAvatarBorder == 1 ? Cyan : FlickMainMenuStyle::Lime; })
				.CutSize(10.0f).BorderWidth(1.15f).Padding(FMargin(12.0f, 9.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 0.0f, 13.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(62.0f)
						[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor_Lambda([this]() { return GetAvatarFrameColor(SelectedAvatarBorder); })
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
						+ SOverlay::Slot()
						[
							SNew(SImage).Image_Lambda([this]() { return GetCosmeticImageBrush(2, SelectedAvatarBorder); })
						]
						+ SOverlay::Slot()[SNew(SFlickCosmeticSwatch).Category(2).Item_Lambda([this]() { return SelectedAvatarBorder; }).Visibility(EVisibility::HitTestInvisible)]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]() { const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER")); }).Font(UiFont(15, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FText::FromString(FlickCosmeticCatalog::GetItems(1)[SelectedBannerTag]); })
							.Font_Lambda([this]() { return GetBannerTagFont(SelectedBannerTag, 10); })
							.ColorAndOpacity_Lambda([this]() { return GetBannerTagColor(SelectedBannerTag); })
							.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
					]
				]
			]
			]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 0.0f, 44.0f)
		[
			SAssignNew(MainMenuPartyTray, SBox).HeightOverride(78.0f)
			.Visibility_Lambda([this]() { return IsDisplayedPartyActive() || bPartyTrayPreview ? EVisibility::Visible : EVisibility::Collapsed; })
			.RenderTransform_Lambda([this]()
			{
				const float Progress = FMath::Clamp(PartyTrayElapsed / 0.38f, 0.0f, 1.0f);
				const float Eased = 1.0f - FMath::Pow(1.0f - Progress, 3.0f);
				const FVector2D LocalFrameSize = MainMenuDiagonalPanel.IsValid()
					? MainMenuDiagonalPanel->GetCachedGeometry().GetLocalSize() : LayerLocalSize;
				return FSlateRenderTransform(FVector2D(
					GetMainMenuDiagonalBottomEdgeX(LocalFrameSize) + 12.0f,
					(1.0f - Eased) * 86.0f));
			})
			[
				SNew(SFlickMainMenuPanel)
				.Premium(true)
				.BackgroundColor(FLinearColor::FromSRGBColor(FColor(11, 22, 25, 248)))
				.AccentColor(Brand)
				.CutSize(10.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(11.0f, 7.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SBox).WidthOverride(22.0f).HeightOverride(2.0f)
							[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Brand)]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(7.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("PARTY  //  %d MEMBERS"), bPartyTrayPreview ? 2 : GetDisplayedPartyMemberCount())); }).Font(UiFont(9, true)).ColorAndOpacity(Brand)
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
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, FlickMainMenuStyle::RightInset, 0.0f)
		[
			SNew(SBox).WidthOverride(FlickMainMenuStyle::RightColumnWidth)
			[BuildChallengePreview()]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)
		.Padding(0, FlickMainMenuStyle::StatusTop, FlickMainMenuStyle::RightInset, 0)
		[BuildHomeStatusPanel()]
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
		// The main-menu footer is 32 units tall; leave a small visual gap above it.
		.Padding(0.0f, 18.0f, 20.0f, 48.0f)
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
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return bChallengesOpen ? EVisibility::Visible : EVisibility::Collapsed; })
			[BuildChallengesPanel()]
		]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenuFooter()
{
	return SNew(SBox)
		.HeightOverride(24.0f).RenderOpacity(.6f)
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
				.RenderTransform_Lambda([this]()
				{
					const float ViewportWidth = FMath::Max(LayerLocalSize.X, 1.0f);
					const double TravelDistance = static_cast<double>(ViewportWidth) + 1900.0;
					const float Offset = static_cast<float>(FMath::Fmod(FooterTickerElapsed * 55.0, TravelDistance));
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
			.Padding(FMargin(18.0f, 8.0f))
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
						const FFlickProfileStats& Stats = FlickGameInstance->GetProfileStatsForView(
							static_cast<EFlickProfileStatsView>(SelectedProfileStatsView));
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
					.Font(DisplayFont(30))
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
						const FFlickProfileStats Stats = FlickGameInstance
							? FlickGameInstance->GetProfileStatsForView(static_cast<EFlickProfileStatsView>(SelectedProfileStatsView))
							: FFlickProfileStats();
						const int32 WinRate = Stats.MatchesPlayed > 0
							? FMath::RoundToInt(100.0f * Stats.Wins / static_cast<float>(Stats.MatchesPlayed)) : 0;
						static const TCHAR* Names[] = {TEXT("TOTAL"), TEXT("CASUAL"), TEXT("COMPETITIVE")};
						return FText::FromString(FString::Printf(TEXT("%s  //  %d%% WIN RATE"),
							Names[FMath::Clamp(SelectedProfileStatsView, 0, 2)], WinRate));
					})
					.Font(UiFont(9, true))
					.ColorAndOpacity(Brand)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[MakeMenuButton(TEXT("TOTAL"), FOnClicked::CreateLambda([this]() { SelectedProfileStatsView = 0; return FReply::Handled(); }))]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[MakeMenuButton(TEXT("CASUAL"), FOnClicked::CreateLambda([this]() { SelectedProfileStatsView = 1; return FReply::Handled(); }))]
				+ SHorizontalBox::Slot().AutoWidth()
				[MakeMenuButton(TEXT("COMPETITIVE"), FOnClicked::CreateLambda([this]() { SelectedProfileStatsView = 2; return FReply::Handled(); }))]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 11.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f))
				+ SUniformGridPanel::Slot(0, 0)[MakeStatCell(TEXT("MATCHES PLAYED"), TEXT("COMPLETED SERIES"), 0, Cyan)]
				+ SUniformGridPanel::Slot(1, 0)[MakeStatCell(TEXT("MATCH WINS"), TEXT("ELIGIBLE PLAYLISTS"), 1, FLinearColor(0.26f, 0.9f, 0.58f, 1.0f))]
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

TSharedRef<SWidget> SFlickGameLayer::BuildProfileIdentityCard(const bool bExpanded)
{
	const auto Showcase = [this]() { const auto* Sessions = GetDisplayedSessionSubsystem();
		return Sessions ? Sessions->GetShowcaseArchetype() : EFlickPieceArchetype::Standard; };
	if (bExpanded)
	{
		const auto Name = [this]() { const auto* Sessions=GetDisplayedSessionSubsystem();
			return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER")); };
		return SNew(SFlickAngularBorder).Tag(TEXT("ProfileIdentityCard")).RefinedSurface(true)
			.BackgroundColor(FlickProfileStyle::Surface).AccentColor(Cyan.CopyWithNewOpacity(.75f))
			.UseAccentForOutline(true).CutSize(16).Padding(1.5f)
			[SNew(SOverlay)
				+ SOverlay::Slot().VAlign(VAlign_Bottom)
				[SNew(SBox).HeightOverride(304)
					[SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Tag(TEXT("ProfileShowcaseStage")).Artwork(EFlickMenuArtwork::ProfileShowcaseFloor).CutSize(14.5f).EdgeFade(FMargin(0,54,0,0))]
						+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0,144,0,0)
						[SNew(SBox).WidthOverride(270).HeightOverride(54)[SNew(FlickProfileWidgets::SContactShadow)]]
						+ SOverlay::Slot()[SNew(SFlickPuckThumbnail).Tag(TEXT("ProfileShowcase")).View(EFlickPuckThumbnailView::ProfileCutout).CutSize(14.5f)
							.Archetype_Lambda(Showcase).Skin_Lambda([this,Showcase]() { return SelectedPuckSkins[static_cast<int32>(Showcase())]; })]]]
				+ SOverlay::Slot().Padding(22)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(SBox).HeightOverride(132)
						[SNew(SFlickMainMenuPanel).Premium(true).CutSize(11).Padding(0)
							.BackgroundColor_Lambda([this]() { return GetBannerBackground(SelectedBannerStyle); })
							.ImageBrush_Lambda([this]() { return GetCosmeticImageBrush(0,SelectedBannerStyle); }).AccentColor(Cyan.CopyWithNewOpacity(.6f))
							[SNew(SOverlay)
								+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::ProfileIdentityBanner).CutSize(11)
									.Visibility_Lambda([this]() { return SelectedBannerStyle==0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(7)
								[SNew(SBox).WidthOverride(116).HeightOverride(116)
									[SNew(SOverlay)
										+ SOverlay::Slot()[SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Ink).CutSize(11)
											.AccentColor_Lambda([this]() { return GetAvatarFrameColor(SelectedAvatarBorder); }).Padding(5)
											[SNew(SFlickPuckThumbnail).View(EFlickPuckThumbnailView::Locker).CutSize(7).Zoom(1.25f)
												.Archetype_Lambda(Showcase).Skin_Lambda([this,Showcase]() { return SelectedPuckSkins[static_cast<int32>(Showcase())]; })]]
										+ SOverlay::Slot().Padding(7)[SNew(SImage).Image_Lambda([this]() { auto* S=GetDisplayedSessionSubsystem(); return S ? S->GetLocalAvatarBrush() : nullptr; })]
										+ SOverlay::Slot()[SNew(SImage).Image_Lambda([this]() { return GetCosmeticImageBrush(2,SelectedAvatarBorder); })]
										+ SOverlay::Slot()[SNew(SFlickCosmeticSwatch).Category(2).Item_Lambda([this]() { return SelectedAvatarBorder; }).Visibility(EVisibility::HitTestInvisible)]]]]]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,14,0,6)
					[SNew(SBox).HeightOverride(40).VAlign(VAlign_Center)
						[SNew(STextBlock).Text_Lambda(Name).ToolTipText_Lambda(Name).Font(DisplayFont(29)).ColorAndOpacity(Paper)
							.OverflowPolicy(ETextOverflowPolicy::Ellipsis)]]
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
						[SNew(SBox).WidthOverride(9).HeightOverride(9)[SNew(SFlickRoundPip).Color(Brand).Filled(true)]]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(FlickCosmeticCatalog::GetItems(1)[SelectedBannerTag]); })
							.Font_Lambda([this]() { return GetBannerTagFont(SelectedBannerTag,12); }).ColorAndOpacity_Lambda([this]() { return GetBannerTagColor(SelectedBannerTag); })]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,19,0,0)
					[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.24f))]]]];
	}
	const TSharedRef<SWidget> Identity = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(112)
			[
				SNew(SFlickMainMenuPanel).BackgroundColor_Lambda([this]() { return GetBannerBackground(SelectedBannerStyle); })
				.ImageBrush_Lambda([this]() { return GetCosmeticImageBrush(0, SelectedBannerStyle); }).AccentColor(Cyan).CutSize(10).Padding(14)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(80).HeightOverride(80)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()[SNew(SFlickAngularBorder).BackgroundColor(Ink).AccentColor_Lambda([this]() { return GetAvatarFrameColor(SelectedAvatarBorder); }).CutSize(12).Padding(10)
								[SNew(SFlickStatusGlobe).Color(Cyan)]]
							+ SOverlay::Slot().Padding(9)[SNew(SImage).Image_Lambda([this]() { auto* Sessions = GetDisplayedSessionSubsystem(); return Sessions ? Sessions->GetLocalAvatarBrush() : nullptr; })]
							+ SOverlay::Slot()[SNew(SImage).Image_Lambda([this]() { return GetCosmeticImageBrush(2, SelectedAvatarBorder); })]
							+ SOverlay::Slot()[SNew(SFlickCosmeticSwatch).Category(2).Item_Lambda([this]() { return SelectedAvatarBorder; }).Visibility(EVisibility::HitTestInvisible)]
						]
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 6)
		[SNew(STextBlock).Text_Lambda([this]() { const auto* Sessions = GetDisplayedSessionSubsystem(); return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER")); })
			.Font(DisplayFont(29)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(FlickCosmeticCatalog::GetItems(1)[SelectedBannerTag]); })
			.Font_Lambda([this]() { return GetBannerTagFont(SelectedBannerTag, 12); }).ColorAndOpacity_Lambda([this]() { return GetBannerTagColor(SelectedBannerTag); }) ];
	return SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Panel.CopyWithNewOpacity(.9f))
		.AccentColor(Cyan).CutSize(16).Padding(22)[Identity];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfile()
{
	TSharedRef<SVerticalBox> LockerCategories = SNew(SVerticalBox);
	TSharedRef<SOverlay> LockerContents = SNew(SOverlay);
	for (int32 Category = 0; Category < FlickCosmeticCatalog::CategoryCount; ++Category)
	{
		const FString CategoryName = FlickCosmeticCatalog::GetCategoryName(Category);
		if (Category == 0 || Category == FlickCosmeticCatalog::PuckCategoryStart || Category == FlickCosmeticCatalog::TrailCategory)
		{
			if (Category != 0)
				LockerCategories->AddSlot().AutoHeight().Padding(12, 8, 12, 10)
				[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(.45f))]];
			LockerCategories->AddSlot().AutoHeight().Padding(12, 0, 0, 7)
			[SNew(STextBlock).Text(FText::FromString(Category == 0 ? TEXT("PLAYER IDENTITY  //")
				: Category == FlickCosmeticCatalog::TrailCategory ? TEXT("EFFECTS  //") : TEXT("PUCK APPEARANCES  //")))
				.Font(DisplayFont(10)).ColorAndOpacity(Cyan)];
		}
		LockerCategories->AddSlot().AutoHeight().Padding(0, 0, 0, 1)[BuildLockerCategory(Category)];

		TSharedRef<SGridPanel> Items = SNew(SGridPanel);
		const TArray<FString>& Names = FlickCosmeticCatalog::GetItems(Category);
		for (int32 Index = 0; Index < Names.Num(); ++Index)
		{
			if (FlickCosmeticCatalog::IsPuckCategory(Category))
			{
				Items->AddSlot(Index % 3, Index / 3).Padding(0, 0, Index % 3 == 2 ? 0.f : FlickLockerStyle::CardGap, FlickLockerStyle::CardGap)
					[BuildLockerPuckCard(Category, Index)];
				continue;
			}
			const FLinearColor PreviewAccent = Category == 1 ? GetBannerTagColor(Index)
				: Category == 2 ? GetAvatarFrameColor(Index)
				: FlickCosmeticCatalog::GetCollection(Category, Index) != INDEX_NONE
					? FlickCosmeticCatalog::GetCollectionColor(FlickCosmeticCatalog::GetCollection(Category, Index))
				: Category == 0 ? (Index == 1 ? Orange : Index == 2 ? Cyan : Brand) : Brand;
			const bool bWide = Category == 0 || Category == 1;
			const int32 Columns = bWide ? 2 : 3;
			const float CardWidth = bWide ? 220.0f : 140.0f;
			const float CardHeight = bWide ? 132.0f : Category == 2 ? 150.0f : 112.0f;
			TSharedRef<SWidget> Preview = SNew(SBox);
			if (Category == 0)
			{
				Preview = SNew(SBox).HeightOverride(72.0f)
				[
					SNew(SFlickMainMenuPanel).BackgroundColor(GetBannerBackground(Index))
					.ImageBrush(GetCosmeticImageBrush(0, Index))
					.AccentColor(PreviewAccent).CutSize(10.0f).BorderWidth(1.5f).Padding(FMargin(15.0f, 10.0f))
					[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
					[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK  //  KNOCKOUT"))).Font(DisplayFont(17)).ColorAndOpacity(Paper)]]
				];
			}
			else if (Category == 1)
			{
				Preview = SNew(SBox).HeightOverride(72.0f)
				[
					SNew(SFlickAngularBorder).BackgroundColor(Panel).AccentColor(PreviewAccent)
					.CutSize(8.0f).BorderWidth(1.0f).Padding(FMargin(12.0f, 11.0f))
					[SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
					[SNew(STextBlock).Text(FText::FromString(Names[Index]))
					.Font(GetBannerTagFont(Index, 15)).ColorAndOpacity(PreviewAccent)
					.OverflowPolicy(ETextOverflowPolicy::Clip)]]
				];
			}
			else if (Category == 2)
			{
				Preview = SNew(SBox).WidthOverride(94.0f).HeightOverride(94.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(PreviewAccent)
						.CutSize(13.0f).BorderWidth(3.0f).Padding(FMargin(9.0f))
						[SNew(SFlickStatusGlobe).Color(Cyan)]
					]
					+ SOverlay::Slot()[SNew(SImage).Image(GetCosmeticImageBrush(2, Index))]
					+ SOverlay::Slot()[SNew(SFlickCosmeticSwatch).Category(2).Item(Index).Visibility(EVisibility::HitTestInvisible)]
				];
			}
			else
			{
				Preview = SNew(SBox).HeightOverride(65.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SFlickCosmeticSwatch).Category(Category).Item(Index)
				];
			}
			TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(&TransparentButtonStyle)
				.ToolTipText(FText::FromString(FlickCosmeticCatalog::GetDescription(Category, Index)))
				.ContentPadding(0.0f).Cursor(EMouseCursor::Hand)
				.Tag(FName(*FString::Printf(TEXT("LockerItem%d_%d"), Category, Index)))
				.OnClicked_Lambda([this, Category, Index]() { EquipLockerItem(Category, Index); return FReply::Handled(); });
			const TWeakPtr<SButton> WeakButton = Button;
			Button->SetContent(
				SNew(SFlickAngularBorder)
				.BackgroundColor_Lambda([this, Category, Index, WeakButton]()
				{
					const TSharedPtr<SButton> Pinned = WeakButton.Pin();
					const int32 Selected = GetSelectedCosmeticIndex(Category);
					return Selected == Index ? PanelRaised
						: Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()) ? PanelRaised : Panel;
				})
				.AccentColor_Lambda([this, Category, Index]()
				{
					const int32 Selected = GetSelectedCosmeticIndex(Category);
					return Selected == Index ? Brand : Hairline;
				})
				.CutSize(8.0f).BorderWidth(1.0f).UseAccentForOutline(true).Padding(FMargin(10.0f, 8.0f))
				[
					SNew(SBox).WidthOverride(CardWidth).HeightOverride(CardHeight)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(bWide ? HAlign_Fill : HAlign_Center).VAlign(VAlign_Center)
						[Preview]
						+ SVerticalBox::Slot().AutoHeight()
						[SNew(STextBlock).Text(FText::FromString(Names[Index])).Font(UiFont(11, true))
							.ColorAndOpacity(Paper).AutoWrapText(true)]
						+ SVerticalBox::Slot().AutoHeight()
						[SNew(STextBlock).Text_Lambda([this, Category, Index]()
						{
							const int32 Selected = GetSelectedCosmeticIndex(Category);
							return FText::FromString(Selected == Index ? TEXT("EQUIPPED") : TEXT("OWNED"));
						}).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
					]
				]);
			Items->AddSlot(Index % Columns, Index / Columns)
				.Padding(0.0f, 0.0f, 8.0f, 8.0f)[Button];
		}
		TSharedRef<SScrollBox> ItemScroller = SNew(SScrollBox) + SScrollBox::Slot()[Items];
		const int32 Columns = Category <= 1 ? 2 : 3;
		const float RowStep = Category <= 1 ? 156.0f : Category == 2 ? 174.0f
			: FlickCosmeticCatalog::IsPuckCategory(Category) ? FlickLockerStyle::CardHeight + FlickLockerStyle::CardGap : 136.0f;
		ItemScroller->SetScrollOffset(FlickCosmeticCatalog::IsPuckCategory(Category) ? 0.f : (GetSelectedCosmeticIndex(Category) / Columns) * RowStep);
		LockerContents->AddSlot()
		[
			SNew(SBox).Visibility_Lambda([this, Category]() { return SelectedLockerCategory == Category ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
				[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.35f))]]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(CategoryName)).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 18)
				[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("OWNED  %d / %d  //  SELECT AN ITEM TO EQUIP"), Names.Num(), Names.Num()))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					ItemScroller
				]
			]
		];
	}

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

	return SNew(SOverlay)
		// Illustrated overview and live locker each own their presentation.
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.004f, 0.012f, 0.02f, 0.32f))
		.Visibility_Lambda([this]()
		{
			return SelectedProfileTab == EFlickProfileTab::Customization || SelectedProfileTab == EFlickProfileTab::Overview ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
		})]
		+ SOverlay::Slot()[SNew(SFlickInterfaceBackdrop).Opacity(0.08f).Visibility_Lambda([this]()
		{
			return SelectedProfileTab == EFlickProfileTab::Customization || SelectedProfileTab == EFlickProfileTab::Overview ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
		})]
		+ SOverlay::Slot()
		[SNew(SBox).Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Customization ? EVisibility::Visible : EVisibility::Collapsed; })
			[BuildLockerPage(LockerCategories, LockerContents)]]
		+ SOverlay::Slot()
		[SNew(SBox).Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Overview ? EVisibility::Visible : EVisibility::Collapsed; })
			[BuildProfileOverview()]]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(42.0f)
		[
			SNew(SBox).HeightOverride_Lambda([this]()
			{
				if (SelectedProfileTab == EFlickProfileTab::Leaderboards) return 780.0f;
				if (SelectedProfileTab == EFlickProfileTab::Customization) return 720.0f;
				if (SelectedProfileTab == EFlickProfileTab::MatchHistory)
				{
					const UFlickGameInstance* Instance = PlayerController.IsValid()
						? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
					return !Instance || Instance->GetRecentMatches().IsEmpty() ? 480.0f : 760.0f;
				}
				return 760.0f;
			})
			.WidthOverride_Lambda([this]()
			{
				return SelectedProfileTab == EFlickProfileTab::Customization ? 1380.0f
					: SelectedProfileTab == EFlickProfileTab::MatchHistory ? 1100.0f
					: SelectedProfileTab == EFlickProfileTab::Leaderboards ? 1240.0f : 1180.0f;
			})
			.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Overview || SelectedProfileTab == EFlickProfileTab::Customization ? EVisibility::Collapsed : EVisibility::Visible; })
			.RenderTransform_Lambda([this]()
			{
				const float Alpha = FMath::Clamp(ProfileEntranceElapsed / 0.38f, 0.0f, 1.0f);
				return FSlateRenderTransform(FVector2D(0.0f, (1.0f - Alpha) * 34.0f));
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
			SNew(SBox)
			.WidthOverride_Lambda([this]()
			{
				if (SelectedProfileTab == EFlickProfileTab::Stats) return 1180.0f;
				if (SelectedProfileTab == EFlickProfileTab::Leaderboards) return 1240.0f;
				if (SelectedProfileTab == EFlickProfileTab::MatchHistory) return 1100.0f;
				return 900.0f;
			})
			.HeightOverride_Lambda([this]()
			{
				if (SelectedProfileTab == EFlickProfileTab::Leaderboards) return 780.0f;
				if (SelectedProfileTab == EFlickProfileTab::Customization) return 720.0f;
				if (SelectedProfileTab == EFlickProfileTab::MatchHistory)
				{
					const UFlickGameInstance* Instance = PlayerController.IsValid()
						? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
					return !Instance || Instance->GetRecentMatches().IsEmpty() ? 480.0f : 760.0f;
				}
				return 760.0f;
			})
			[
			SNew(SFlickAngularBorder)
			.BackgroundColor(Panel.CopyWithNewOpacity(0.97f))
			.AccentColor_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::MatchHistory ? Orange
				: SelectedProfileTab == EFlickProfileTab::Leaderboards ? Brand : Cyan; })
			.UseAccentForOutline(true)
			.CutSize(18.0f)
			.BorderWidth(1.2f)
			.Padding(FMargin(28.0f, 22.0f))
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(
				SelectedProfileTab == EFlickProfileTab::Customization ? TEXT("FLICK  /  YOUR LOCKER") : TEXT("FLICK  /  YOUR RECORD")); })
				.Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 14.0f)
			[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(
				SelectedProfileTab == EFlickProfileTab::Stats ? TEXT("STATS")
				: SelectedProfileTab == EFlickProfileTab::Leaderboards ? TEXT("COMPETITIVE RANKS")
				: SelectedProfileTab == EFlickProfileTab::MatchHistory ? TEXT("MATCH HISTORY") : TEXT("CUSTOMIZE")); })
				.Font(DisplayFont(36)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(
				SelectedProfileTab == EFlickProfileTab::Stats ? TEXT("Read the match behind the numbers.")
				: SelectedProfileTab == EFlickProfileTab::Leaderboards ? TEXT("Your climb across the competitive playlists.")
				: SelectedProfileTab == EFlickProfileTab::MatchHistory ? TEXT("Every series leaves a mark.")
				: TEXT("Make every puck and every moment yours.")); })
				.Font(UiFont(12)).ColorAndOpacity(Muted)]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride_Lambda([this]()
				{
					if (SelectedProfileTab == EFlickProfileTab::Leaderboards) return 530.0f;
					if (SelectedProfileTab == EFlickProfileTab::MatchHistory)
					{
						const UFlickGameInstance* Instance = PlayerController.IsValid()
							? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
						return !Instance || Instance->GetRecentMatches().IsEmpty() ? 250.0f : 530.0f;
					}
					return SelectedProfileTab == EFlickProfileTab::Customization ? 490.0f : 530.0f;
				})
				[
				SNew(SOverlay)
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
						BuildRankedProfilePanel()
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
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(190.0f)[MakeMenuButton(TEXT("PROFILE MENU"), FOnClicked::CreateLambda([this]() { SelectedProfileTab = EFlickProfileTab::Overview; return FReply::Handled(); }))]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE DATA SAVES AUTOMATICALLY"))).Font(UiFont(10)).ColorAndOpacity(Muted)]
			]
			]
			]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).HAlign(HAlign_Center)
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Customization ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 18.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]() { return FText::FromString(SelectedLockerCategory < FlickCosmeticCatalog::PuckCategoryStart
								? TEXT("LIVE PROFILE PREVIEW") : TEXT("LIVE ARENA PREVIEW")); })
							.RenderTransform_Lambda([this]() { return FSlateRenderTransform(FVector2D(0.0f,
								SelectedLockerCategory < FlickCosmeticCatalog::PuckCategoryStart ? 0.0f : -365.0f)); })
							.Font(UiFont(10, true)).ColorAndOpacity(Cyan)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox).WidthOverride(330.0f).HeightOverride(86.0f)
							.Visibility_Lambda([this]() { return SelectedLockerCategory < FlickCosmeticCatalog::PuckCategoryStart
								? EVisibility::Visible : EVisibility::Collapsed; })
							[
								SNew(SFlickMainMenuPanel)
								.BackgroundColor_Lambda([this]() { return GetBannerBackground(SelectedBannerStyle); })
								.ImageBrush_Lambda([this]() { return GetCosmeticImageBrush(0, SelectedBannerStyle); })
								.AccentColor(Brand).Premium(true).CutSize(10.0f).BorderWidth(1.2f).Padding(FMargin(10.0f, 9.0f))
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
									[
										SNew(SBox).WidthOverride(58.0f)
										[
											SNew(SOverlay)
											+ SOverlay::Slot()
											[
												SNew(SFlickAngularBorder).BackgroundColor(PanelRaised)
												.AccentColor_Lambda([this]() { return GetAvatarFrameColor(SelectedAvatarBorder); })
												.CutSize(7.0f).BorderWidth(1.2f).Padding(FMargin(3.0f))
												[
													SNew(SOverlay)
													+ SOverlay::Slot()[SNew(SFlickStatusGlobe).Color(Cyan)
														.Visibility_Lambda([this]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions && Sessions->GetLocalAvatarBrush() ? EVisibility::Collapsed : EVisibility::Visible; })]
													+ SOverlay::Slot()[SNew(SImage).Image_Lambda([this]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return Sessions ? Sessions->GetLocalAvatarBrush() : nullptr; })]
												]
											]
											+ SOverlay::Slot()[SNew(SImage).Image_Lambda([this]() { return GetCosmeticImageBrush(2, SelectedAvatarBorder); })]
											+ SOverlay::Slot()[SNew(SFlickCosmeticSwatch).Category(2).Item_Lambda([this]() { return SelectedAvatarBorder; }).Visibility(EVisibility::HitTestInvisible)]
										]
									]
									+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
											.Text_Lambda([this]() { UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem(); return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER")); })
											.Font(UiFont(15, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
										+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
											.Text_Lambda([this]() { return FText::FromString(FlickCosmeticCatalog::GetItems(1)[SelectedBannerTag]); })
											.Font_Lambda([this]() { return GetBannerTagFont(SelectedBannerTag, 10); })
											.ColorAndOpacity_Lambda([this]() { return GetBannerTagColor(SelectedBannerTag); })]
									]
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenuPartyMember(const int32 PartySlot)
{
	return SNew(SBox)
		.WidthOverride(132.0f)
		.HeightOverride(46.0f)
		.Visibility_Lambda([this, PartySlot]()
		{
			return (IsDisplayedPartyActive() && HasDisplayedPartyMember(PartySlot)) || (bPartyTrayPreview && PartySlot < 2)
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
			.BackgroundColor(FLinearColor::FromSRGBColor(FColor(17, 31, 34, 248)))
			.AccentColor_Lambda([this, PartySlot]()
			{
				return (bPartyTrayPreview ? PartySlot == 0 : IsDisplayedPartyMemberLeader(PartySlot))
					? Brand
					: Cyan.CopyWithNewOpacity(0.5f);
			})
			.CutSize(8.0f)
			.BorderWidth(1.1f)
			.Padding(FMargin(8.0f, 5.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 8.0f, 2.0f)
				[
					SNew(SBox).WidthOverride(3.0f)
					[
					SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor_Lambda([this, PartySlot]() { return (bPartyTrayPreview ? PartySlot == 0 : IsDisplayedPartyMemberLeader(PartySlot)) ? Brand : Cyan; })
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
							if (bPartyTrayPreview) return FText::FromString(PartySlot == 0 ? TEXT("YOU") : TEXT("RIVAL PLAYER"));
							if (IsDisplayedPartyActive())
							{
								return FText::FromString(GetDisplayedPartyMemberName(PartySlot));
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
							return FText::FromString((bPartyTrayPreview ? PartySlot == 0 : IsDisplayedPartyMemberLeader(PartySlot)) ? TEXT("PARTY LEADER") : TEXT("IN PARTY"));
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
		return SNew(SBox).HeightOverride(42.0f)
			[
				SNew(SFlickAngularBorder).RefinedSurface(true)
				.BackgroundColor_Lambda([this, Tab]()
				{
					const bool bSelected = Tab == 2 ? bShowingRecentPlayers
						: !bShowingRecentPlayers && bShowingOnlineFriends == (Tab == 1);
					return bSelected
						? FMath::Lerp(Ink, Brand, .065f)
						: Ink;
				})
				.AccentColor_Lambda([this, Tab]()
				{
					const bool bSelected = Tab == 2 ? bShowingRecentPlayers
						: !bShowingRecentPlayers && bShowingOnlineFriends == (Tab == 1);
					return bSelected ? Brand : Cyan.CopyWithNewOpacity(.4f);
				})
				.UseAccentForOutline(true)
				.CutSize(6.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(1.0f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.Tag(FName(*FString::Printf(TEXT("SocialTab%d"), Tab)))
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
				SNew(SFlickAngularBorder).RefinedSurface(true)
				.BackgroundColor(Ink)
				.AccentColor(Hairline)
				.UseAccentForOutline(true)
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

	return SNew(SFlickAngularBorder).RefinedSurface(true).Tag(TEXT("SocialDrawer"))
		.BackgroundColor(Ink.CopyWithNewOpacity(.98f))
		.AccentColor(Cyan).UseAccentForOutline(true)
		.CutSize(18.0f)
		.BorderWidth(1.0f)
		.Padding(FMargin(24.0f, 24.0f, 24.0f, 20.0f))
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
						[SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Cyan)]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 3.0f, 15.0f, 3.0f)
					[
						SNew(SBox).WidthOverride(1.0f)
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.72f))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)
						[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK NETWORK  //"))).Font(UiFont(8, true)).ColorAndOpacity(Cyan)]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SOCIAL")))
						.Font(DisplayFont(30)).ColorAndOpacity(Paper)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
								const FString Service = Sessions ? Sessions->GetOnlineServiceName().ToUpper() : TEXT("LOCAL");
							const FString PlayerName = Sessions ? Sessions->GetLocalDisplayName().ToUpper() : TEXT("LOCAL PLAYER");
							return FText::FromString(FString::Printf(TEXT("%s  //  %s"), *PlayerName, *Service));
							})
							.Font(UiFont(8, true)).ColorAndOpacity(Cyan).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(44.0f).HeightOverride(44.0f)
					[
						SNew(SFlickAngularBorder).RefinedSurface(true)
						.BackgroundColor(Ink)
						.AccentColor(Hairline)
						.UseAccentForOutline(true)
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
						SNew(SFlickAngularBorder).RefinedSurface(true)
						.BackgroundColor(Ink)
						.AccentColor(Hairline)
						.UseAccentForOutline(true).CutSize(3.0f).BorderWidth(0.75f).Padding(FMargin(9.0f, 4.0f))
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
						SNew(SFlickAngularBorder).RefinedSurface(true)
						.BackgroundColor(Ink)
						.AccentColor(Hairline)
						.UseAccentForOutline(true).CutSize(3.0f).BorderWidth(0.7f).Padding(FMargin(10.0f, 4.0f))
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
						SNew(SBox).HeightOverride(154.0f)
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
							SNew(SFlickAngularBorder).RefinedSurface(true)
							.BackgroundColor(Ink)
							.AccentColor(Hairline.CopyWithNewOpacity(0.42f))
							.CutSize(10.0f).BorderWidth(0.8f).Padding(FMargin(20.0f, 14.0f))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 8)
								[SNew(SBox).WidthOverride(30).HeightOverride(30)[SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Cyan.CopyWithNewOpacity(.65f))]]
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
								[SNew(STextBlock).Text(FText::FromString(TEXT("FIND YOUR NEXT RIVALRY"))).Font(DisplayFont(17)).ColorAndOpacity(Paper)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
								[SNew(STextBlock)
								.Text_Lambda([this]()
								{
								return FText::FromString(bShowingRecentPlayers
									? TEXT("NO RECENT PLAYERS  //  PLAY ONLINE TO BUILD YOUR LIST")
									: bShowingOnlineFriends
										? TEXT("NO STEAM FRIENDS ARE CURRENTLY ONLINE")
										: TEXT("NO PLATFORM FRIENDS FOUND  //  TRY REFRESH"));
								})
								.Font(UiFont(9)).ColorAndOpacity(Muted).AutoWrapText(true).Justification(ETextJustify::Center)]
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
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(140.0f).HeightOverride(44.0f)
						[
							SocialAction(TEXT("REFRESH"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->RefreshFriends();
								return FReply::Handled();
							}), &TransparentButtonStyle)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return IsDisplayedPartyActive() && !IsLocalDisplayedPartyLeader() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							SocialAction(TEXT("LEAVE PARTY"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->LeaveParty();
								return FReply::Handled();
							}), &TransparentButtonStyle, Orange)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]() { return IsDisplayedPartyActive() && IsLocalDisplayedPartyLeader() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							SocialAction(TEXT("DISBAND"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->DisbandParty();
								return FReply::Handled();
							}), &TransparentButtonStyle, Orange)
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
				return HasDisplayedPartyMember(PartySlot) ? EVisibility::Visible : EVisibility::Collapsed;
			}
			return EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder).RefinedSurface(true)
			.BackgroundColor(Ink)
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
								? HasDisplayedPartyMember(PartySlot) : PartySlot == 0;
							return bOccupied ? Brand : Muted;
						})
						.Filled_Lambda([this, PartySlot]()
						{
							return IsDisplayedPartyActive()
								? HasDisplayedPartyMember(PartySlot) : PartySlot == 0;
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(44.0f).HeightOverride(36.0f)
					[
						SNew(SFlickAngularBorder).RefinedSurface(true)
						.BackgroundColor(Ink)
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
									return FText::FromString(GetDisplayedPartyMemberName(PartySlot));
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
								return IsDisplayedPartyMemberLeader(PartySlot) || (!IsDisplayedPartyActive() && PartySlot == 0)
									? EVisibility::Visible : EVisibility::Collapsed;
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, PartySlot]()
						{
							return FText::FromString(IsDisplayedPartyMemberLeader(PartySlot) || (!IsDisplayedPartyActive() && PartySlot == 0)
								? TEXT("PARTY LEADER") : HasDisplayedPartyMember(PartySlot) ? TEXT("IN PARTY") : TEXT("INVITE A FRIEND"));
						}).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&CompactMenuButtonStyle)
					.Visibility_Lambda([this, PartySlot]()
					{
						return IsLocalDisplayedPartyLeader() && HasDisplayedPartyMember(PartySlot)
							&& !IsDisplayedPartyMemberLeader(PartySlot)
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.OnClicked_Lambda([this, PartySlot]()
					{
						if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem())
						{
							Sessions->PromotePartyMember(GetDisplayedPartyMemberUserId(PartySlot));
						}
						return FReply::Handled();
					})
					[SNew(STextBlock).Text(FText::FromString(TEXT("PROMOTE"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton).ButtonStyle(&DangerButtonStyle)
					.Visibility_Lambda([this, PartySlot]()
					{
						return IsLocalDisplayedPartyLeader() && HasDisplayedPartyMember(PartySlot)
							&& !GetDisplayedPartyMemberUserId(PartySlot).IsEmpty()
							&& !IsDisplayedPartyMemberLeader(PartySlot)
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.OnClicked_Lambda([this, PartySlot]()
					{
						if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem())
						{
							Sessions->RemovePartyMember(GetDisplayedPartyMemberUserId(PartySlot));
						}
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
			SNew(SFlickAngularBorder).RefinedSurface(true)
			.BackgroundColor_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bPlayingFlick
					? FMath::Lerp(Ink, Cyan, .018f) : Ink;
			})
			.AccentColor_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem();
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return Hairline;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				return Friend.bPlayingFlick
					? Brand
					: Friend.bOnline ? Cyan.CopyWithNewOpacity(.45f) : Hairline.CopyWithNewOpacity(0.46f);
			})
			.UseAccentForOutline(true)
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
							SNew(SFlickAngularBorder).RefinedSurface(true)
							.BackgroundColor(Ink)
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
						SocialAction(TEXT("INVITE"), FOnClicked::CreateLambda([this, FriendIndex]()
						{
							if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->InviteFriendToParty(FriendIndex);
							return FReply::Handled();
						}), &TransparentButtonStyle, Brand, true)
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
			SNew(SFlickAngularBorder).RefinedSurface(true)
			.BackgroundColor(Ink)
			.AccentColor(Hairline.CopyWithNewOpacity(0.62f))
			.UseAccentForOutline(true)
			.CutSize(6.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 6.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(38.0f).HeightOverride(38.0f)
					[
						SNew(SFlickAngularBorder).RefinedSurface(true)
						.BackgroundColor(Ink)
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
						SocialAction(TEXT("INVITE"), FOnClicked::CreateLambda([this, RecentIndex]()
						{
							if (UFlickSessionSubsystem* Sessions = GetDisplayedSessionSubsystem()) Sessions->InviteRecentPlayerToParty(RecentIndex);
							return FReply::Handled();
						}), &TransparentButtonStyle, Brand, true)
					]
				]
			]
		];
}
