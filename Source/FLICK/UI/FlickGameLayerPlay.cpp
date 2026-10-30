//playlists, formats, private matches, online
#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickMatchmakingRules.h"
#include "UI/FlickPlaylistWidgets.h"

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
	BackButton->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(BackButton)));
	return SNew(SOverlay)
		+ SOverlay::Slot()[FlickPlaylistWidgets::Background()]
		+ SOverlay::Slot().VAlign(VAlign_Top).Padding(72, 30, 48, 0)
		[
			SNew(SBox).Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
				&& SelectedTrainingActivity == EFlickTrainingActivity::PracticePacks ? EVisibility::Collapsed : EVisibility::Visible; })
			[FlickPlaylistWidgets::Header(
				TAttribute<FText>::CreateLambda([this]() {
					const TCHAR* Playlist = SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive ? TEXT("COMPETITIVE")
						: SelectedPlayPlaylist == EFlickPlayPlaylist::Casual ? TEXT("CASUAL")
						: SelectedPlayPlaylist == EFlickPlayPlaylist::Training ? TEXT("TRAINING") : TEXT("");
					return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::None ? TEXT("HOME  //  PLAY")
						: FString::Printf(TEXT("HOME  //  PLAY  //  %s"), Playlist));
				}),
				TAttribute<FText>::CreateLambda([this]() {
					if (SelectedPlayPlaylist == EFlickPlayPlaylist::None) return FText::FromString(TEXT("FIND YOUR NEXT RIVALRY."));
					if (SelectedTrainingActivity == EFlickTrainingActivity::PracticePacks) return FText::FromString(TEXT("PRACTICE PACKS"));
					if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training && SelectedTrainingActivity == EFlickTrainingActivity::None)
						return FText::FromString(TEXT("MAKE EVERY SHOT COUNT."));
					return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::Training ? TEXT("CHOOSE YOUR PRACTICE ARENA.") : TEXT("SELECT YOUR MATCH FORMAT."));
				}),
				TAttribute<FText>::CreateLambda([this]() {
					if (SelectedPlayPlaylist == EFlickPlayPlaylist::None) return FText::FromString(TEXT("01  //  MODE"));
					if (SelectedTrainingActivity == EFlickTrainingActivity::PracticePacks) return FText::FromString(TEXT("03  //  FOCUS + DIFFICULTY"));
					if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training)
						return FText::FromString(SelectedTrainingActivity == EFlickTrainingActivity::None ? TEXT("02  //  OFFLINE PRACTICE") : TEXT("03  //  ARENA"));
					return FText::FromString(TEXT("02  //  FORMAT"));
				}))]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(40, 140, 40, 118)
		[
			SNew(SBox)
			.WidthOverride(FlickPlaylistWidgets::ContentWidth)
			.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::None ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f)
				+ SGridPanel::Slot(0, 0).Padding(FlickPlaylistWidgets::Gap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Casual, TEXT("CASUAL"), TEXT("Relaxed online matchmaking."))]
				+ SGridPanel::Slot(1, 0).Padding(FlickPlaylistWidgets::Gap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Competitive, TEXT("COMPETITIVE"), TEXT("Ranked matches with MMR."))]
				+ SGridPanel::Slot(0, 1).Padding(FlickPlaylistWidgets::Gap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Training, TEXT("TRAINING"), TEXT("Learn, practice, or challenge a bot."))]
				+ SGridPanel::Slot(1, 1).Padding(FlickPlaylistWidgets::Gap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::PrivateMatch, TEXT("PRIVATE MATCH"), TEXT("Your rules. Your rivals."))]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(40, 140, 40, 118)
		[
			SNew(SBox)
			.WidthOverride(FlickPlaylistWidgets::ContentWidth)
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
					.BackgroundColor(Panel.CopyWithNewOpacity(.82f))
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
				+ SVerticalBox::Slot().AutoHeight().Padding(UiMetrics::CardGap)
				[
					BuildTrainingActivityCard(EFlickTrainingActivity::PracticePacks, TEXT("PRACTICE PACKS"),
						TEXT("PRECISION, KNOCKOUTS, SWITCH CONTROL AND BOB POCKETS"),
						TEXT("3 DIFFICULTIES  |  SCORED CHALLENGES  |  SAVED PERSONAL BESTS"), Brand)
				]
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
				&& SelectedTrainingActivity == EFlickTrainingActivity::PracticePacks ? EVisibility::Visible : EVisibility::Collapsed; })
			[BuildPracticeMenu()]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(40, 140, 40, 118)
		[
			SNew(SBox)
			.WidthOverride(FlickPlaylistWidgets::ContentWidth)
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
		+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(38, 0, 38, 24)
		[
			SNew(SBox).HeightOverride(64).Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
				&& SelectedTrainingActivity == EFlickTrainingActivity::PracticePacks ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SFlickAngularBorder).CutSize(12).UseAccentForOutline(true).BorderWidth(.8f)
				.AccentColor(Cyan.CopyWithNewOpacity(.6f)).BackgroundColor(Ink.CopyWithNewOpacity(.82f))
				.Padding(FMargin(18, 6))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(186)[BackButton]]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right).Padding(24, 0)
					[SNew(STextBlock).Font(DisplayFont(11)).ColorAndOpacity(Muted)
						.Text_Lambda([this]() {
							return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::Training
								? TEXT("OFFLINE  //  PRACTICE AT YOUR OWN PACE")
								: TEXT("ONLINE MATCHMAKING  //  DEVELOPMENT PREVIEW"));
						})]
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

	TSharedRef<SButton> BackButton = SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() { if (GameMode.IsValid()) GameMode->ClosePrivateMatchSetup(); return FReply::Handled(); });
	BackButton->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(BackButton)));

	return SNew(SOverlay)
		+ SOverlay::Slot()[FlickPlaylistWidgets::Background()]
		+ SOverlay::Slot().VAlign(VAlign_Top).Padding(72, 30, 48, 0)
		[FlickPlaylistWidgets::Header(FText::FromString(TEXT("HOME  //  PLAY  //  PRIVATE MATCH")),
			FText::FromString(TEXT("YOUR RULES. YOUR RIVALS.")), FText::FromString(TEXT("02  //  MATCH SETUP")))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(40, 140, 40, 118)
		[
			SNew(SBox).WidthOverride(FlickPlaylistWidgets::ContentWidth)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.65f).Padding(0, 0, 18, 0)
				[
					SNew(SFlickAngularBorder).BackgroundColor(Panel.CopyWithNewOpacity(.91f)).AccentColor(Cyan.CopyWithNewOpacity(.6f))
						.UseAccentForOutline(true).CutSize(12).Padding(FMargin(26, 20))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 0, 8)
						[SNew(STextBlock).Text(FText::FromString(TEXT("MATCH RULES  //  HOST SETTINGS"))).Font(DisplayFont(16)).ColorAndOpacity(Paper)]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("MODE"), EFlickPrivateMatchSetting::Mode, TAttribute<FText>::CreateLambda([this]() { const FFlickPrivateMatchSettings& Settings = GetDisplayedPrivateMatchSettings(); return FText::FromString(Settings.Variant == EFlickMatchVariant::Bob ? TEXT("BOB") : FString::Printf(TEXT("%dV%d"), Settings.PlayersPerTeam, Settings.PlayersPerTeam)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("ROUNDS TO WIN"), EFlickPrivateMatchSetting::RoundsToWin, TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber(GetDisplayedPrivateMatchSettings().RoundsToWin); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("ARENA SIZE"), EFlickPrivateMatchSetting::ArenaScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().ArenaScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("FRICTION"), EFlickPrivateMatchSetting::FrictionScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().FrictionScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("LAUNCH POWER"), EFlickPrivateMatchSetting::LaunchSpeedScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().LaunchSpeedScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("BOUNCE"), EFlickPrivateMatchSetting::RestitutionScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), GetDisplayedPrivateMatchSettings().RestitutionScale * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("KICKOFF"), EFlickPrivateMatchSetting::SimultaneousKickoff, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GetDisplayedPrivateMatchSettings().bSimultaneousKickoff ? TEXT("SIMULTANEOUS") : TEXT("ALTERNATING")); }))]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1)
				[
					FlickPlaylistWidgets::Card(EFlickMenuArtwork::PrivateMatch, false,
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[SNew(STextBlock).Text(FText::FromString(TEXT("FRIENDS  //  BOTS  //  SPECTATORS"))).Font(DisplayFont(10)).ColorAndOpacity(Cyan)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("PARTY  %d / %d"),
							FMath::Max(GetDisplayedPartyMemberCount(), 1), FlickMaximumPartyMembers)); }).Font(DisplayFont(27)).ColorAndOpacity(Paper)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
						[SNew(STextBlock).Text(FText::FromString(TEXT("Set your rules here, then choose Blue, Orange or Spectate inside the arena. Empty seats are filled by bots.")))
							.Font(UiFont(12)).ColorAndOpacity(Paper.CopyWithNewOpacity(.86f)).AutoWrapText(true)]
						+ SVerticalBox::Slot().AutoHeight()
						[SNew(STextBlock).Text(FText::FromString(TEXT("Private matches do not affect competitive MMR."))).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true)])
				]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(38, 0, 38, 24)
		[FlickPlaylistWidgets::Footer(
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[SNew(SBox).WidthOverride(186)[BackButton]]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(24, 0)
			[SNew(STextBlock).Text(FText::FromString(TEXT("CUSTOM MATCH  //  NO RANKED STAKES"))).Font(DisplayFont(11)).ColorAndOpacity(Muted)]
			+ SHorizontalBox::Slot().AutoWidth()
			[SNew(SBox).WidthOverride(250).IsEnabled_Lambda([this]() { return GameMode.IsValid() && GameMode->CanStartPrivateMatch(); })
				[MakeMenuButton(TEXT("START MATCH"), FOnClicked::CreateLambda([this]() {
					if (PlayerController.IsValid()) PlayerController->RequestStartNetworkMatch(); return FReply::Handled(); }), true, false, 52)]]
		)];
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
	const FString PlaylistTag = Playlist == EFlickPlayPlaylist::Casual ? TEXT("ONLINE")
		: Playlist == EFlickPlayPlaylist::Competitive ? TEXT("RANKED ONLINE")
		: Playlist == EFlickPlayPlaylist::Training ? TEXT("OFFLINE PRACTICE")
		: Playlist == EFlickPlayPlaylist::PrivateMatch ? TEXT("YOUR RULES") : TEXT("EXPERIMENTAL");
	const EFlickMenuArtwork Artwork = Playlist == EFlickPlayPlaylist::Casual ? EFlickMenuArtwork::Casual
		: Playlist == EFlickPlayPlaylist::Competitive ? EFlickMenuArtwork::Competitive
		: Playlist == EFlickPlayPlaylist::Training ? EFlickMenuArtwork::Training : EFlickMenuArtwork::PrivateMatch;
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
	CardButton->SetContent(FlickPlaylistWidgets::Card(Artwork, TAttribute<bool>::CreateLambda(IsActive),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(PlaylistCode + TEXT("  //  ") + PlaylistTag)).Font(DisplayFont(11))
				.ColorAndOpacity_Lambda([IsActive, Playlist]() { return IsActive() ? Brand : Playlist == EFlickPlayPlaylist::PrivateMatch ? FLinearColor(.16f, .9f, .52f) : Cyan; })]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 2)
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(32)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Summary)).Font(UiFont(12)).ColorAndOpacity(Paper.CopyWithNewOpacity(.84f)).AutoWrapText(true)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(16, 0, 0, 0)
		[SNew(FlickPlaylistWidgets::SCardArrow).Active_Lambda(IsActive)]));
	return SNew(SBox).HeightOverride(FlickPlaylistWidgets::CardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTrainingActivityCard(
	const EFlickTrainingActivity Activity,
	const FString& Label,
	const FString& Summary,
	const FString& Detail,
	const FLinearColor& Accent)
{
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this, Activity]()
		{
			// Only these activities have a submenu here. Immediate-start activities
			// must leave the training grid selected when their next screen is cancelled.
			SelectedTrainingActivity = (Activity == EFlickTrainingActivity::FreePlay
				|| Activity == EFlickTrainingActivity::PracticePacks) ? Activity : EFlickTrainingActivity::None;
			if (Activity == EFlickTrainingActivity::PracticePacks)
			{
				SelectedPracticeCategory = INDEX_NONE;
				return FReply::Handled();
			}
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
	const EFlickMenuArtwork Artwork = Activity == EFlickTrainingActivity::Tutorial ? EFlickMenuArtwork::Tutorial
		: Activity == EFlickTrainingActivity::FreePlay ? EFlickMenuArtwork::FreePlay
		: Activity == EFlickTrainingActivity::BotMatch ? EFlickMenuArtwork::BotMatch
		: Activity == EFlickTrainingActivity::BobBotMatch ? EFlickMenuArtwork::Bob : EFlickMenuArtwork::TrainingPracticeWide;
	CardButton->SetContent(FlickPlaylistWidgets::Card(Artwork, TAttribute<bool>::CreateLambda(IsActive),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Activity == EFlickTrainingActivity::PracticePacks ? TEXT("05  //  SCORED CHALLENGES") : TEXT("PRACTICE  //  OFFLINE")))
				.Font(DisplayFont(10)).ColorAndOpacity(Accent)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(26)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Summary)).Font(UiFont(10)).ColorAndOpacity(Paper.CopyWithNewOpacity(.87f)).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
			[SNew(STextBlock).Text(FText::FromString(Detail)).Font(DisplayFont(9)).ColorAndOpacity(Muted).AutoWrapText(true)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(14, 0, 0, 0)
		[SNew(FlickPlaylistWidgets::SCardArrow).Active_Lambda(IsActive)]));
	return SNew(SBox).HeightOverride(Activity == EFlickTrainingActivity::PracticePacks ? 128 : 184)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPlayFormatCard(
	const int32 PlayersPerTeam,
	const bool bBob)
{
	const bool bAvailable = bBob || (PlayersPerTeam >= 1 && PlayersPerTeam <= 3);
	const FString Title = bBob ? TEXT("BOB") : FString::Printf(TEXT("%dV%d KNOCKOUT"), PlayersPerTeam, PlayersPerTeam);
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
	const EFlickMenuArtwork Artwork = bBob ? EFlickMenuArtwork::Bob : PlayersPerTeam == 1 ? EFlickMenuArtwork::BotMatch
		: PlayersPerTeam == 2 ? EFlickMenuArtwork::Doubles : EFlickMenuArtwork::Triples;
	CardButton->SetContent(FlickPlaylistWidgets::Card(Artwork,
		TAttribute<bool>::CreateLambda([IsActive, IsSelected]() { return IsActive() || IsSelected(); }),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(bBob ? TEXT("POCKET GAME  //  1V1") : FString::Printf(TEXT("%d PLAYERS  //  %d PUCKS PER TEAM"), PlayersPerTeam * 2, PlayersPerTeam * 4)))
				.Font(DisplayFont(10)).ColorAndOpacity(Cyan)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 2)
			[SNew(STextBlock).Text_Lambda([this, Title, PlayersPerTeam, bBob]() {
				return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::Training
					? bBob ? TEXT("BOB PRACTICE") : FString::Printf(TEXT("%dV%d ARENA"), PlayersPerTeam, PlayersPerTeam) : Title);
			}).Font(DisplayFont(30)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text_Lambda([this, bBob]() {
				return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::Training
					? TEXT("Set up the board. Find your shot. Repeat.")
					: bBob ? TEXT("Pocket your color. Clear the board.") : TEXT("Team knockouts. First to three rounds wins."));
			}).Font(UiFont(11)).ColorAndOpacity(Paper.CopyWithNewOpacity(.85f)).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
			[
				SNew(SHorizontalBox).Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive ? EVisibility::Visible : EVisibility::Collapsed; })
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
				[SNew(SFlickRankBadge).Size(26).Progress_Lambda([this, PlayersPerTeam, bBob]() {
					const auto* Ranking = GetRankingSubsystem();
					return Ranking ? Ranking->GetDisplayProgress(bBob ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic, PlayersPerTeam) : FFlickRankProgress();
				})]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[SNew(STextBlock).Text_Lambda([this, PlayersPerTeam, bBob]() {
					const auto* Ranking = GetRankingSubsystem();
					return FText::FromString(Ranking ? Ranking->GetDisplayProgressLabel(bBob ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic, PlayersPerTeam) : TEXT("RANK UNAVAILABLE"));
				}).Font(DisplayFont(10)).ColorAndOpacity(Paper)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
			[SNew(STextBlock).Text_Lambda([this, IsSelected, FitsDisplayedParty]() {
				return FText::FromString(!FitsDisplayedParty() ? TEXT("PARTY TOO LARGE FOR THIS FORMAT")
					: IsSelected() ? TEXT("SELECTED  //  READY TO PLAY")
					: SelectedPlayPlaylist == EFlickPlayPlaylist::Training ? TEXT("OFFLINE  //  FREE PLAY") : TEXT("SELECT FORMAT"));
			}).Font(DisplayFont(10)).ColorAndOpacity_Lambda([IsSelected]() { return IsSelected() ? Brand : Muted; })]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(16, 0, 0, 0)
		[SNew(FlickPlaylistWidgets::SCardArrow).Active_Lambda([IsSelected, IsActive]() { return IsSelected() || IsActive(); })]));
	return SNew(SBox).HeightOverride(FlickPlaylistWidgets::CardHeight)[CardButton];
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
									? Ranking->GetDisplayProgressLabel(Variant, TeamSize)
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
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
					[SNew(SBox).WidthOverride(180).Visibility_Lambda([this]() {
					 return GameMode.IsValid() && GameMode->IsMatchmakingSession()
					  && (GameMode->GetLobbyPlayerCount(EFlickTeam::Player1) < GameMode->GetPlayersPerTeam() || GameMode->GetLobbyPlayerCount(EFlickTeam::Player2) < GameMode->GetPlayersPerTeam())
					  ? EVisibility::Visible : EVisibility::Collapsed;
					})[MakeMenuButton(TEXT("MAIN MENU"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ReturnToMainMenu(); return FReply::Handled(); }), false, false)]]
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
