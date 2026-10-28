#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Audio/FlickMenuRadioComponent.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickPracticeCatalog.h"
#include "Game/FlickPracticeComponent.h"
#include "UI/FlickPracticeStyle.h"
#include "UI/FlickMainMenuStyle.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "Core/FlickLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameMode.h"
#include "GameMapsSettings.h"
#include "Online/FlickSessionSubsystem.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "UI/FlickGameLayer.h"
#include "UI/FlickHUD.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "TimerManager.h"

namespace
{
	int32 CountWidgetType(const TSharedRef<SWidget>& Widget, const FString& Type)
	{
		int32 Count = Widget->GetTypeAsString() == Type ? 1 : 0;
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
			Count += CountWidgetType(Children->GetChildAt(Index), Type);
		return Count;
	}

	bool ContainsLabel(const TSharedRef<SWidget>& Widget, const FString& Label)
	{
		if (Widget->GetTypeAsString() == TEXT("STextBlock")
			&& StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Label) return true;
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
			if (ContainsLabel(Children->GetChildAt(Index), Label)) return true;
		return false;
	}

	TSharedPtr<SButton> FindVisibleButton(const TSharedRef<SWidget>& Widget, const FString& Label)
	{
		if (!Widget->GetVisibility().IsVisible()) return nullptr;
		if (Widget->GetTypeAsString() == TEXT("SButton") && ContainsLabel(Widget, Label))
			return StaticCastSharedRef<SButton>(Widget);
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
			if (auto Button = FindVisibleButton(Children->GetChildAt(Index), Label)) return Button;
		return nullptr;
	}

	TSharedPtr<SWidget> FindVisibleTag(const TSharedRef<SWidget>& Widget, const FName Tag)
	{
		if (!Widget->GetVisibility().IsVisible()) return nullptr;
		if (Widget->GetTag() == Tag) return Widget;
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
			if (auto Found = FindVisibleTag(Children->GetChildAt(Index), Tag)) return Found;
		return nullptr;
	}

	bool PressButton(const TSharedPtr<SButton>& Button)
	{
		if (!Button.IsValid() || !Button->IsEnabled()) return false;
		// Exercise the real Slate button callback, without moving the OS cursor
		// or taking focus from the developer's desktop during offscreen tests.
		const FKeyEvent Enter(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0);
		const bool bPressed = Button->OnKeyDown(Button->GetCachedGeometry(), Enter).IsEventHandled();
		return Button->OnKeyUp(Button->GetCachedGeometry(), Enter).IsEventHandled() && bPressed;
	}

	bool ActivateButton(const TSharedRef<SWidget>& Root, const FString& Label)
	{
		// Several transitions occur in one frame: refresh bound visibility first.
		Root->Invalidate(EInvalidateWidgetReason::Prepass);
		Root->SlatePrepass(1.0f);
		const bool bPressed = PressButton(FindVisibleButton(Root, Label));
		Root->Invalidate(EInvalidateWidgetReason::Prepass);
		Root->SlatePrepass(1.0f);
		return bPressed;
	}

	bool ActivateTag(const TSharedRef<SWidget>& Root, const FName Tag)
	{
		Root->Invalidate(EInvalidateWidgetReason::Prepass);
		Root->SlatePrepass(1.0f);
		const auto Found = FindVisibleTag(Root, Tag);
		const bool bPressed = Found && Found->GetTypeAsString() == TEXT("SButton") && PressButton(StaticCastSharedPtr<SButton>(Found));
		Root->Invalidate(EInvalidateWidgetReason::Prepass);
		Root->SlatePrepass(1.0f);
		return bPressed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPackagingConfigurationTest, "FLICK.Frontend.PackagingConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPackagingConfigurationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Runtime default is the empty bootstrap"), UGameMapsSettings::GetGameDefaultMap(), FString(TEXT("/Engine/Maps/Entry")));
	FString Command;
	if (TestTrue(TEXT("Packaging command exists"), FFileHelper::LoadFileToString(Command,
		*(FPaths::ProjectDir() / TEXT("package-flick-development.cmd")))))
	{
		TestTrue(TEXT("Cooks the startup map"), Command.Contains(TEXT("-map=/Engine/Maps/Entry")));
		TestFalse(TEXT("Does not skip the editor module needed for cooking"), Command.Contains(TEXT("-nocompileeditor")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPackagedStartupTest, "FLICK.Frontend.PackagedStartup",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FFlickPackagedStartupTest::RunTest(const FString& Parameters)
{
	// Run against the real game world and normal intro, never a synthetic test
	// world, preview mode, or FlickSkipIntro. Also usable via UnrealEditor -game.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]()
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
			if (Context.WorldType == EWorldType::Game && Context.World()) { World = Context.World(); break; }
		if (!TestNotNull(TEXT("Running game world"), World)) return;
		auto* Mode = World->GetAuthGameMode<AFlickGameMode>();
		auto* Instance = Cast<UFlickGameInstance>(World->GetGameInstance());
		auto* Controller = Cast<AFlickPlayerController>(World->GetFirstPlayerController());
		auto* Hud = Controller ? Cast<AFlickHUD>(Controller->GetHUD()) : nullptr;
		if (!TestNotNull(TEXT("Frontend mode"), Mode) || !TestNotNull(TEXT("Game instance"), Instance)
			|| !TestNotNull(TEXT("Local controller"), Controller) || !TestNotNull(TEXT("Frontend HUD"), Hud)) return;
		if (!TestTrue(TEXT("Slate game layer exists"), Hud->GameLayer.IsValid())) return;
		const auto Layer = Hud->GameLayer.ToSharedRef();
		TestEqual(TEXT("Loads configured startup map"), World->GetOutermost()->GetName(), UGameMapsSettings::GetGameDefaultMap());
		TestEqual(TEXT("Empty bootstrap map, no template landscape"), UGameMapsSettings::GetGameDefaultMap(), FString(TEXT("/Engine/Maps/Entry")));
		TestFalse(TEXT("Intro finished without skipping it"), Instance->ShouldShowStartupPresentation());
		TestFalse(TEXT("No startup overlay blocking menu input"), Layer->bStartupOverlayVisible);
		TestFalse(TEXT("World keeps ticking"), World->IsPaused());
		TestTrue(TEXT("Main menu is active"), Mode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu);
		TestTrue(TEXT("Cursor enabled"), Controller->bShowMouseCursor);
		TestEqual(TEXT("Only the solo cosmetic showcase puck, not a match formation"), Mode->Pieces.Num(), 1);
		if (Mode->Pieces.Num() == 1)
		{
			const auto* Sessions = Instance->GetSubsystem<UFlickSessionSubsystem>();
			const auto Archetype = Sessions->GetShowcaseArchetype();
			TestTrue(TEXT("Shows equipped puck type"), Mode->Pieces[0]->GetArchetype() == Archetype);
			TestTrue(TEXT("High-detail visuals present"), Mode->Pieces[0]->HasTestArenaVisuals());
			TestEqual(TEXT("Shows equipped puck skin"), Mode->Pieces[0]->GetPuckSkin(),
				FlickCosmeticCatalog::LoadPuckSkins()[static_cast<int32>(Archetype)]);
			int32 ExpectedPuck = INDEX_NONE;
			if (FParse::Value(FCommandLine::Get(), TEXT("FlickExpectedShowcase="), ExpectedPuck))
				TestEqual(TEXT("Saved showcase preference loaded"), static_cast<int32>(Archetype), ExpectedPuck);
		}
		const auto StatusPanel = FindVisibleTag(Layer, TEXT("HomeStatus"));
		if (TestTrue(TEXT("Combined Home status panel exists"), StatusPanel.IsValid()))
		{
			TestTrue(TEXT("Shared rank panel contains Social"), ContainsLabel(StatusPanel.ToSharedRef(), TEXT("SOCIAL")));
			// Startup timing can land in any interval of the rotating rank panel.
			// Exercise every queue rather than assuming the title is still 1v1.
			const double OriginalHomeTime = Layer->FooterTickerElapsed;
			const auto* Ranking = Instance->GetSubsystem<UFlickRankingSubsystem>();
			TestNotNull(TEXT("Ranking subsystem available at startup"), Ranking);
			for (int32 Playlist = 0; Playlist < 4; ++Playlist)
			{
				Layer->FooterTickerElapsed = Playlist * FlickMainMenuStyle::RankDisplaySeconds + .25;
				StatusPanel->Invalidate(EInvalidateWidgetReason::Layout);
				StatusPanel->SlatePrepass(1.0f);
				const EFlickMatchVariant Variant = Playlist == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
				const int32 TeamSize = Playlist == 3 ? 1 : Playlist + 1;
				const FString Title = Playlist == 3 ? TEXT("BOB 1V1 CURRENT RANK")
					: FString::Printf(TEXT("%dV%d CURRENT RANK"), TeamSize, TeamSize);
				TestTrue(FString::Printf(TEXT("Shared panel displays %s"), *Title), ContainsLabel(StatusPanel.ToSharedRef(), Title));
				if (Ranking)
					TestTrue(TEXT("Rank progress follows the displayed competitive queue"),
						ContainsLabel(StatusPanel.ToSharedRef(), Ranking->GetDisplayProgressLabel(Variant, TeamSize)));
			}
			Layer->FooterTickerElapsed = OriginalHomeTime;
			StatusPanel->Invalidate(EInvalidateWidgetReason::Layout);
			StatusPanel->SlatePrepass(1.0f);
			TestEqual(TEXT("Combined panel retains its design width"), StatusPanel->GetDesiredSize().X, 650.f);
		}
		TestTrue(TEXT("Combined header Social accepts input"), ActivateButton(Layer, TEXT("SOCIAL")));
		TestTrue(TEXT("Social opens"), Layer->bSocialPanelOpen);
		TestTrue(TEXT("Social uses the shared framed drawer"), FindVisibleTag(Layer, TEXT("SocialDrawer")).IsValid());
		TestTrue(TEXT("Social Online tab accepts input"), ActivateTag(Layer, TEXT("SocialTab1")));
		TestTrue(TEXT("Social Online tab selects the online filter"), Layer->bShowingOnlineFriends && !Layer->bShowingRecentPlayers);
		TestTrue(TEXT("Social Recent tab accepts input"), ActivateTag(Layer, TEXT("SocialTab2")));
		TestTrue(TEXT("Social Recent tab selects recent players"), Layer->bShowingRecentPlayers && !Layer->bShowingOnlineFriends);
		TestTrue(TEXT("Social All tab accepts input"), ActivateTag(Layer, TEXT("SocialTab0")));
		TestFalse(TEXT("Social All restores the unfiltered list"), Layer->bShowingRecentPlayers || Layer->bShowingOnlineFriends);
		// Only refresh the null/offline service during automation; never send
		// real platform requests or invitations from this visual-input check.
		if (FParse::Param(FCommandLine::Get(), TEXT("nosteam")))
			TestTrue(TEXT("Social Refresh accepts input offline"), ActivateTag(Layer, TEXT("SocialAction_REFRESH")));
		TestTrue(TEXT("Social closes"), ActivateButton(Layer, TEXT("X")));
		TestFalse(TEXT("Social close restores the header"), Layer->bSocialPanelOpen);
		if (auto* Radio = Controller->GetMenuRadio())
		{
			const bool bWasEnabled = Radio->IsEnabled();
			const FString OriginalTrack = Radio->GetTrackTitle();
			TestTrue(TEXT("Circular radio toggle accepts input"), ActivateTag(Layer, TEXT("HomeRadioToggle")));
			TestEqual(TEXT("Radio toggle uses the real setting"), Radio->IsEnabled(), !bWasEnabled);
			TestTrue(TEXT("Radio toggle restores playback preference"), ActivateTag(Layer, TEXT("HomeRadioToggle")));
			TestEqual(TEXT("Playback preference restored"), Radio->IsEnabled(), bWasEnabled);
			TestTrue(TEXT("Radio next accepts input"), ActivateTag(Layer, TEXT("HomeRadioNext")));
			TestNotEqual(TEXT("Radio next really changes track"), Radio->GetTrackTitle(), OriginalTrack);
			TestTrue(TEXT("Radio previous accepts input"), ActivateTag(Layer, TEXT("HomeRadioPrevious")));
			TestEqual(TEXT("Previous restores the track"), Radio->GetTrackTitle(), OriginalTrack);
		}
		else AddError(TEXT("Menu radio component is missing"));
		TestTrue(TEXT("Settings button accepts input"), ActivateButton(Layer, TEXT("SETTINGS")));
		TestTrue(TEXT("Settings opens"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Settings);
		TestEqual(TEXT("Settings does not draw Home UI behind its own header"), Layer->GetScreenVisibility(EFlickFrontendScreen::MainMenu), EVisibility::Collapsed);
		const TPair<const TCHAR*, EFlickSettingsTab> Tabs[] = {
			{TEXT("CAMERA"), EFlickSettingsTab::Camera}, {TEXT("INTERFACE"), EFlickSettingsTab::Interface},
			{TEXT("STREAM SAFE"), EFlickSettingsTab::StreamSafe}, {TEXT("VIDEO"), EFlickSettingsTab::Display},
			{TEXT("LIGHTING"), EFlickSettingsTab::Lighting}, {TEXT("AUDIO"), EFlickSettingsTab::Sound},
			{TEXT("CONTROLS"), EFlickSettingsTab::Controls}, {TEXT("QUICK CHAT"), EFlickSettingsTab::QuickChat},
			{TEXT("GAMEPLAY"), EFlickSettingsTab::GameFeel}};
		for (const auto& Tab : Tabs)
		{
			TestTrue(FString::Printf(TEXT("Settings category accepts input: %s"), Tab.Key), ActivateButton(Layer, Tab.Key));
			TestTrue(TEXT("Correct settings page is selected"), Layer->SelectedSettingsTab == Tab.Value);
		}
		const bool OriginalImpact = Mode->AreImpactEffectsEnabled();
		const float OriginalSensitivity = Mode->GetShotMouseSensitivity();
		if (const auto Toggle = FindVisibleTag(Layer, TEXT("SettingsImpactEffects")))
		{
			const FKeyEvent Accept(EKeys::Enter,FModifierKeysState(),0,false,0,0);
			TestTrue(TEXT("Impact switch accepts press"), Toggle->OnKeyDown(Toggle->GetCachedGeometry(),Accept).IsEventHandled());
			TestTrue(TEXT("Impact switch accepts release"), Toggle->OnKeyUp(Toggle->GetCachedGeometry(),Accept).IsEventHandled());
			TestEqual(TEXT("Impact switch uses existing preference"), Mode->AreImpactEffectsEnabled(), !OriginalImpact);
			Toggle->OnKeyDown(Toggle->GetCachedGeometry(),Accept); Toggle->OnKeyUp(Toggle->GetCachedGeometry(),Accept);
			TestEqual(TEXT("Impact switch restores original preference"), Mode->AreImpactEffectsEnabled(), OriginalImpact);
		}
		else AddError(TEXT("Gameplay impact switch missing"));
		if (const auto Found = FindVisibleTag(Layer,TEXT("SettingsShotSensitivity")))
		{
			const auto Slider = StaticCastSharedPtr<SSlider>(Found);
			Mode->SetShotMouseSensitivity(.4f);
			Layer->SlatePrepass(1.f);
			const FKeyEvent Accept(EKeys::Enter,FModifierKeysState(),0,false,0,0);
			TestTrue(TEXT("Sensitivity accepts controller lock"), Slider->OnKeyDown(Slider->GetCachedGeometry(),Accept).IsEventHandled());
			Slider->OnNavigation(Slider->GetCachedGeometry(),FNavigationEvent(FModifierKeysState(),0,EUINavigation::Right,ENavigationGenesis::Controller));
			TestTrue(TEXT("Full-width slider changes the actual setting"), Mode->GetShotMouseSensitivity() > .4f);
			float SavedSensitivity = 0;
			GConfig->GetFloat(TEXT("FLICK.GameplaySettings"),TEXT("ShotMouseSensitivity"),SavedSensitivity,GGameUserSettingsIni);
			TestEqual(TEXT("Sensitivity is saved immediately using existing key"), SavedSensitivity,Mode->GetShotMouseSensitivity());
			Slider->OnKeyDown(Slider->GetCachedGeometry(),Accept);
			Mode->SetShotMouseSensitivity(OriginalSensitivity);
		}
		else AddError(TEXT("Gameplay sensitivity slider missing"));
		if (!UE_BUILD_SHIPPING) TestTrue(TEXT("Development rank category remains available"),ActivateTag(Layer,TEXT("SettingsTab9")));
		TestTrue(TEXT("Lighting page accepts input"),ActivateTag(Layer,TEXT("SettingsTab7")));
		TestTrue(TEXT("Live arena preview accepts input"),ActivateButton(Layer,TEXT("PREVIEW ARENA")));
		TestTrue(TEXT("Live preview mode is active"),Layer->bLightingPreview);
		TestFalse(TEXT("Illustrated background is hidden in real lighting preview"),FindVisibleTag(Layer,TEXT("SettingsBackdrop")).IsValid());
		TestTrue(TEXT("Return from real preview accepts input"),ActivateButton(Layer,TEXT("BACK TO SETTINGS")));
		TestFalse(TEXT("Normal settings frame is restored"),Layer->bLightingPreview);
		TestTrue(TEXT("New settings Back accepts input"),ActivateTag(Layer,TEXT("SettingsBack")));
		TestTrue(TEXT("Play button accepts input"), ActivateButton(Layer, TEXT("PLAY")));
		TestTrue(TEXT("Play menu opens"), Mode->GetFrontendScreen() == EFlickFrontendScreen::ModeSelect);
		TestTrue(TEXT("Illustrated Casual card accepts input"), ActivateButton(Layer, TEXT("CASUAL")));
		TestTrue(TEXT("Casual format page selected"), Layer->SelectedPlayPlaylist == EFlickPlayPlaylist::Casual);
		for (int32 TeamSize = 1; TeamSize <= 3; ++TeamSize)
		{
			TestTrue(TEXT("Illustrated team-format card accepts input"), ActivateButton(Layer, FString::Printf(TEXT("%dV%d KNOCKOUT"), TeamSize, TeamSize)));
			TestEqual(TEXT("Format selection preserves team size"), Mode->GetMatchmakingPlayersPerTeam(), TeamSize);
			TestEqual(TEXT("Selected format matches highlighted card"), Layer->SelectedPlayFormat, TeamSize);
		}
		TestTrue(TEXT("BOB format accepts input"), ActivateButton(Layer, TEXT("BOB")));
		TestTrue(TEXT("BOB selection preserves match variant"), Mode->GetSelectedMatchVariant() == EFlickMatchVariant::Bob);
		TestEqual(TEXT("BOB format highlighted"), Layer->SelectedPlayFormat, 4);
		TestTrue(TEXT("Casual Back returns to playlist root"), ActivateButton(Layer, TEXT("BACK")));
		TestTrue(TEXT("Playlist root restored"), Layer->SelectedPlayPlaylist == EFlickPlayPlaylist::None);
		TestTrue(TEXT("Illustrated Competitive card accepts input"), ActivateButton(Layer, TEXT("COMPETITIVE")));
		TestTrue(TEXT("Competitive format page selected"), Layer->SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive);
		TestTrue(TEXT("Ranked 2v2 format accepts input"), ActivateButton(Layer, TEXT("2V2 KNOCKOUT")));
		TestEqual(TEXT("Ranked format highlighted"), Layer->SelectedPlayFormat, 2);
		TestTrue(TEXT("Format selection does not queue or leave menu"), Mode->GetFrontendScreen() == EFlickFrontendScreen::ModeSelect);
		TestTrue(TEXT("Competitive Back returns to playlist root"), ActivateButton(Layer, TEXT("BACK")));
		TestTrue(TEXT("Illustrated Training card accepts input"), ActivateButton(Layer, TEXT("TRAINING")));
		TestTrue(TEXT("Training menu selected"), Layer->SelectedPlayPlaylist == EFlickPlayPlaylist::Training);
		TestTrue(TEXT("Free Play card accepts input"), ActivateButton(Layer, TEXT("FREE PLAY")));
		TestTrue(TEXT("Free Play format submenu selected"), Layer->SelectedTrainingActivity == EFlickTrainingActivity::FreePlay);
		TestTrue(TEXT("Free Play BOB card accepts input"), ActivateButton(Layer, TEXT("BOB PRACTICE")));
		TestEqual(TEXT("Free Play BOB highlighted"), Layer->SelectedPlayFormat, 4);
		TestTrue(TEXT("Free Play Back returns to Training"), ActivateButton(Layer, TEXT("BACK")));
		TestTrue(TEXT("Training activity grid restored"), Layer->SelectedTrainingActivity == EFlickTrainingActivity::None);
		Mode->SelectMatchVariant(EFlickMatchVariant::Classic);
		Mode->SetMatchmakingPlayersPerTeam(1);
		TestTrue(TEXT("Practice Packs accepts input"), ActivateButton(Layer, TEXT("PRACTICE PACKS")));
		TestTrue(TEXT("Combined focus/difficulty page"), Layer->SelectedTrainingActivity == EFlickTrainingActivity::PracticePacks);
		const int32 PracticeMasteryBefore=FlickPracticeCatalog::PerfectPacks();
		TArray<int32> PracticeScoresBefore;
		for (int32 Category=0;Category<FlickPracticeCatalog::CategoryCount;++Category)
		{
			TestTrue(TEXT("Every illustrated practice focus accepts input"),ActivateTag(Layer,FName(*FString::Printf(TEXT("PracticeFocus%d"),Category))));
			TestEqual(TEXT("Focus selection controls the details panel"),Layer->SelectedPracticeCategory,Category);
			if (auto Title=FindVisibleTag(Layer,TEXT("PracticeFocusTitle")))
				TestEqual(TEXT("Details title follows the actual focus"),StaticCastSharedPtr<STextBlock>(Title)->GetText().ToString(),FString(FlickPracticeCatalog::Name(Category)));
			else AddError(TEXT("Practice details title missing"));
			for (int32 Difficulty=0;Difficulty<FlickPracticeCatalog::DifficultyCount;++Difficulty)
			{
				const int32 Best=FlickPracticeCatalog::BestScore(Category,Difficulty);
				PracticeScoresBefore.Add(Best);
				TestTrue(TEXT("Every difficulty accepts input without launching"),ActivateTag(Layer,FName(*FString::Printf(TEXT("PracticeDifficulty%d"),Difficulty))));
				TestEqual(TEXT("Correct practice difficulty selected"),Layer->SelectedPracticeDifficulty,Difficulty);
				TestTrue(TEXT("Choosing a card or difficulty stays in the menu"),Mode->GetFrontendScreen()==EFlickFrontendScreen::ModeSelect);
				if (auto Score=FindVisibleTag(Layer,FName(*FString::Printf(TEXT("PracticeDifficultyScore%d"),Difficulty))))
					TestEqual(TEXT("Detail score reads the selected focus's real saved best"),StaticCastSharedPtr<STextBlock>(Score)->GetText().ToString(),
						FString::Printf(TEXT("%d / %d"),Best,FlickPracticeCatalog::ChallengesPerRun));
				else AddError(TEXT("Practice difficulty score missing"));
				if (auto Score=FindVisibleTag(Layer,FName(*FString::Printf(TEXT("PracticeScore%d_%d"),Category,Difficulty))))
					TestEqual(TEXT("Focus card names each difficulty alongside its score"),StaticCastSharedPtr<STextBlock>(Score)->GetText().ToString(),
						FString::Printf(TEXT("%s %d/%d"),FlickPracticeCatalog::DifficultyName(Difficulty),Best,FlickPracticeCatalog::ChallengesPerRun));
				else AddError(TEXT("Practice focus score missing"));
			}
		}
		for (int32 Category=0;Category<FlickPracticeCatalog::CategoryCount;++Category)
			for (int32 Difficulty=0;Difficulty<FlickPracticeCatalog::DifficultyCount;++Difficulty)
				TestEqual(TEXT("Browsing never edits saved practice bests"),FlickPracticeCatalog::BestScore(Category,Difficulty),PracticeScoresBefore[Category*3+Difficulty]);
		TestEqual(TEXT("Browsing never awards mastery"),FlickPracticeCatalog::PerfectPacks(),PracticeMasteryBefore);
		TestTrue(TEXT("Switch Control accepts input"), ActivateButton(Layer, TEXT("SWITCH CONTROL")));
		TestEqual(TEXT("Switch Control category selected"), Layer->SelectedPracticeCategory, 2);
		TestTrue(TEXT("Expert difficulty accepts input"), ActivateButton(Layer, TEXT("EXPERT")));
		TestEqual(TEXT("Difficulty selection doesn't start a run"), Layer->SelectedPracticeDifficulty, 2);
		TestTrue(TEXT("Stays in practice selection until Start"), Mode->GetFrontendScreen() == EFlickFrontendScreen::ModeSelect);
		TestTrue(TEXT("Large Start Practice button launches the selected pack"),ActivateTag(Layer,TEXT("PracticeStart")));
		TestTrue(TEXT("Practice uses the existing offline training flow"),Mode->IsPracticeMode());
		if (auto* Practice=Mode->GetPractice())
		{
			TestEqual(TEXT("Launch keeps selected focus"),Practice->GetCategory(),2);
			TestEqual(TEXT("Launch keeps selected difficulty"),Practice->GetDifficulty(),2);
		}
		else AddError(TEXT("Practice component missing"));
		TestTrue(TEXT("Playing overlay returns to the pack menu"),ActivateButton(Layer,TEXT("PRACTICE PACKS")));
		TestTrue(TEXT("Practice menu restored after launch"),Layer->SelectedTrainingActivity==EFlickTrainingActivity::PracticePacks
			&& Mode->GetFrontendScreen()==EFlickFrontendScreen::ModeSelect);
		TestTrue(TEXT("Back returns directly to Training"), ActivateButton(Layer, TEXT("BACK")));
		TestTrue(TEXT("No invisible category-only back step"), Layer->SelectedTrainingActivity == EFlickTrainingActivity::None);
		Mode->CloseModeSelect();
		TestTrue(TEXT("Lineups opens from the real Home button"), ActivateButton(Layer, TEXT("LINEUPS")));
		TestTrue(TEXT("Lineup editor is visible"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Loadout);
		const EFlickLineupPreset OriginalPreset = Mode->GetLoadoutEditingPreset();
		TArray<EFlickPieceArchetype> OriginalPieces;
		for (int32 Slot = 0; Slot < 4; ++Slot) OriginalPieces.Add(Mode->GetLoadoutPiece(EFlickTeam::Player1, Slot));
		TestTrue(TEXT("Name field is present below the headline"), FindVisibleTag(Layer, TEXT("LineupName")).IsValid());
		TestTrue(TEXT("Formation selects slot 3"), ActivateTag(Layer, TEXT("LineupSlot2")));
		TestEqual(TEXT("Selected slot drives the inspector"), Layer->GetSelectedLoadoutSlot(EFlickTeam::Player1), 2);
		Layer->SetHoveredLoadoutArchetype(EFlickTeam::Player1, EFlickPieceArchetype::Heavy);
		TestTrue(TEXT("Hover really previews a different type"), Layer->GetPreviewLoadoutArchetype(EFlickTeam::Player1) == EFlickPieceArchetype::Heavy);
		TestTrue(TEXT("Hover never edits the loadout"), Mode->GetLoadoutPiece(EFlickTeam::Player1, 2) == OriginalPieces[2]);
		Layer->ClearHoveredLoadoutArchetype(EFlickTeam::Player1, EFlickPieceArchetype::Heavy);
		TestTrue(TEXT("Leaving a card restores the equipped preview"), Layer->GetPreviewLoadoutArchetype(EFlickTeam::Player1) == OriginalPieces[2]);
		for (int32 Type = 0; Type < 9; ++Type)
		{
			TArray<EFlickPieceArchetype> Expected;
			for (int32 Slot = 0; Slot < 4; ++Slot) Expected.Add(Mode->GetLoadoutPiece(EFlickTeam::Player1, Slot));
			const auto Archetype = static_cast<EFlickPieceArchetype>(Type);
			const int32 ExistingSlot = Expected.Find(Archetype);
			if (ExistingSlot != INDEX_NONE && ExistingSlot != 2) Swap(Expected[ExistingSlot], Expected[2]);
			else Expected[2] = Archetype;
			TestTrue(TEXT("Every illustrated archetype accepts input"), ActivateTag(Layer, FName(*FString::Printf(TEXT("LineupChoice%d"), Type))));
			for (int32 Slot = 0; Slot < 4; ++Slot)
				TestTrue(TEXT("Card preserves the existing one-of-each rule, including swaps"), Mode->GetLoadoutPiece(EFlickTeam::Player1, Slot) == Expected[Slot]);
		}
		for (int32 Slot = 0; Slot < 4; ++Slot) Mode->SetLoadoutPiece(EFlickTeam::Player1, Slot, OriginalPieces[Slot]);
		for (int32 Preset = 0; Preset < 4; ++Preset)
		{
			TestTrue(TEXT("Every saved lineup preset accepts input"), ActivateTag(Layer, FName(*FString::Printf(TEXT("LineupPreset%d"), Preset))));
			TestEqual(TEXT("Correct saved preset is inspected"), static_cast<int32>(Mode->GetLoadoutEditingPreset()), Preset);
			TestTrue(TEXT("Selected preset keeps its visible opaque surface"), FindVisibleTag(Layer,FName(*FString::Printf(TEXT("LineupPresetSurface%d"),Preset))).IsValid());
			TestTrue(TEXT("Already selected preset can be clicked again without disabling or vanishing"), ActivateTag(Layer,FName(*FString::Printf(TEXT("LineupPreset%d"),Preset))));
			if (auto Name = FindVisibleTag(Layer, TEXT("LineupName")))
				TestEqual(TEXT("Name editor follows the selected preset"), StaticCastSharedPtr<SEditableTextBox>(Name)->GetText().ToString(),
					Layer->GetClassDisplayName(static_cast<EFlickLineupPreset>(Preset)));
		}
		Mode->SelectLoadoutEditingPreset(OriginalPreset);
		TestTrue(TEXT("Radar toggle accepts input"), ActivateTag(Layer, TEXT("LineupViewToggle")));
		TestFalse(TEXT("Radar comparison view shown"), Layer->bReadableLineup);
		TestTrue(TEXT("Puck preview toggle accepts input"), ActivateTag(Layer, TEXT("LineupViewToggle")));
		TestTrue(TEXT("Puck preview restored"), Layer->bReadableLineup);
		// Remote-party clients have no authoritative GameMode. Their inspector,
		// catalogue and radar must still read the same saved lineup data.
		const auto OriginalMode = Layer->GameMode;
		const auto OriginalRemotePreset = Layer->RemoteLoadoutPreset;
		Layer->RemoteLoadoutPreset = OriginalPreset;
		Layer->GameMode.Reset();
		TestTrue(TEXT("Remote inspector uses the selected saved puck, not a Standard fallback"),
			Layer->GetPreviewLoadoutArchetype(EFlickTeam::Player1) == Instance->GetClassLoadoutPiece(OriginalPreset, 2));
		TestEqual(TEXT("Remote equipped/preview stats agree at rest"), Layer->GetLoadoutStatValue(EFlickTeam::Player1, 0, true),
			Layer->GetLoadoutStatValue(EFlickTeam::Player1, 0, false));
		Layer->GameMode = OriginalMode;
		Layer->RemoteLoadoutPreset = OriginalRemotePreset;
		TestTrue(TEXT("Save Lineup accepts input"), ActivateButton(Layer, TEXT("SAVE LINEUP")));
		TestTrue(TEXT("Save returns to Home"), Mode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu);
		TestTrue(TEXT("Profile opens"), ActivateButton(Layer, TEXT("PROFILE")));
		TestTrue(TEXT("Profile overview shows the actual selected showcase puck"), FindVisibleTag(Layer,TEXT("ProfileShowcase")).IsValid());
		TestTrue(TEXT("Profile identity uses its arena stage"), FindVisibleTag(Layer,TEXT("ProfileShowcaseStage")).IsValid());
		if (const auto HistoryCard=FindVisibleTag(Layer,TEXT("ProfileChoice2")); HistoryCard.IsValid())
			TestFalse(TEXT("Decorative Stats artwork never leaks into Match History"),FindVisibleTag(HistoryCard.ToSharedRef(),TEXT("ProfileStatsIllustration")).IsValid());
		const int32 ProfileMatchesBefore = Instance->GetProfileStats().MatchesPlayed;
		const int32 ProfileHistoryBefore = Instance->GetRecentMatches().Num();
		for (int32 Destination = 0; Destination < 3; ++Destination)
		{
			TestTrue(TEXT("Each illustrated profile card accepts input"), ActivateTag(Layer,FName(*FString::Printf(TEXT("ProfileChoice%d"),Destination))));
			TestEqual(TEXT("Profile destination keeps its existing page mapping"), static_cast<int32>(Layer->SelectedProfileTab),Destination+1);
			if (Destination == 1)
			{
				const auto Ranks = FindVisibleTag(Layer, TEXT("RankedProfileContents"));
				if (TestTrue(TEXT("Competitive rank content is visible"), Ranks.IsValid()))
				{
					TestEqual(TEXT("Ranks need no scroll container"), CountWidgetType(Ranks.ToSharedRef(), TEXT("SScrollBox")), 0);
					TestEqual(TEXT("All four playlist badges and 22 ladder badges remain"), CountWidgetType(Ranks.ToSharedRef(), TEXT("SFlickRankBadge")), 26);
					TestTrue(TEXT("Complete rank explanation is retained"), FindVisibleTag(Ranks.ToSharedRef(), TEXT("RankedProfileFooter")).IsValid());
				}
			}
			TestTrue(TEXT("Detail page returns to overview"),ActivateButton(Layer,TEXT("PROFILE MENU")));
			TestTrue(TEXT("Overview is restored"), Layer->SelectedProfileTab == EFlickProfileTab::Overview);
		}
		TestEqual(TEXT("Browsing overview never records fictitious matches"),Instance->GetProfileStats().MatchesPlayed,ProfileMatchesBefore);
		TestEqual(TEXT("Browsing overview never fabricates history"),Instance->GetRecentMatches().Num(),ProfileHistoryBefore);
		TestTrue(TEXT("Profile Back returns Home"),ActivateTag(Layer,TEXT("ProfileBack")));
		TestTrue(TEXT("Profile Back closes the actual frontend screen"),Mode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu);
		TestTrue(TEXT("Profile reopens for customization"),ActivateButton(Layer,TEXT("PROFILE")));
		TestTrue(TEXT("Four-destination profile opens customization"), ActivateButton(Layer, TEXT("CUSTOMIZE")));
		TestTrue(TEXT("Live locker selected"), Layer->SelectedProfileTab == EFlickProfileTab::Customization);
		const auto OriginalSkins = Layer->SelectedPuckSkins;
		for (int32 Type = 0; Type < 9; ++Type)
		{
			const int32 Category = FlickCosmeticCatalog::PuckCategoryStart + Type;
			TestTrue(TEXT("Every puck category accepts input"), ActivateTag(Layer, FName(*FString::Printf(TEXT("LockerCategory%d"), Category))));
			if (TestEqual(TEXT("Exactly one live locker puck"), Mode->Pieces.Num(), 1))
				TestEqual(TEXT("Live preview follows actual category"), static_cast<int32>(Mode->Pieces[0]->GetArchetype()), Type);
			for (int32 Finish = 0; Finish < 6; ++Finish)
			{
				TestTrue(TEXT("Each of six new illustrated finish cards accepts input"), ActivateTag(Layer, FName(*FString::Printf(TEXT("LockerItem%d_%d"), Category, Finish))));
				TestEqual(TEXT("Finish selected in UI"), Layer->GetSelectedCosmeticIndex(Category), Finish);
				if (Mode->Pieces.Num() == 1) TestEqual(TEXT("Finish updates the live puck immediately"), Mode->Pieces[0]->GetPuckSkin(), Finish);
				int32 SavedFinish = INDEX_NONE;
				GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), *FlickCosmeticCatalog::GetConfigKey(Category), SavedFinish, GGameUserSettingsIni);
				TestEqual(TEXT("Automatic persistence keeps the stable skin index"), SavedFinish, Finish);
			}
			TestTrue(TEXT("Restore original equipped finish"), ActivateTag(Layer, FName(*FString::Printf(TEXT("LockerItem%d_%d"), Category, OriginalSkins[Type]))));
		}
		Layer->SelectedLockerCategory = FlickCosmeticCatalog::PuckCategoryStart;
		Mode->SetLockerPreview(Layer->SelectedLockerCategory);
		TestTrue(TEXT("Locker clockwise arrow accepts input"), ActivateTag(Layer, TEXT("LockerRotateNext")));
		TestEqual(TEXT("Locker rotation changes only the preview yaw"), Mode->LockerPreviewYaw, 20.0f);
		TestTrue(TEXT("Locker anticlockwise arrow accepts input"), ActivateTag(Layer, TEXT("LockerRotatePrevious")));
		TestEqual(TEXT("Locker rotation is reversible"), Mode->LockerPreviewYaw, 0.0f);
		Layer->SelectedLockerCategory = 0;
		Mode->SetLockerPreview(0);
		Mode->RotateLockerPreview(30);
		TestEqual(TEXT("Identity categories cannot rotate a puck"), Mode->LockerPreviewYaw, 0.0f);
		TestTrue(TEXT("Locker returns to profile"), ActivateTag(Layer, TEXT("LockerBack")));
		Mode->CloseProfile();
		Mode->RotateLockerPreview(30);
		TestEqual(TEXT("Home cannot be rotated through the locker control"), Mode->LockerPreviewYaw, 0.0f);
		TestTrue(TEXT("Shop opens"), ActivateButton(Layer, TEXT("ITEM SHOP")));
		const FFlickShopDemoState OriginalShopDemo = Layer->ShopDemo;
		Layer->ShopDemo.Reset();
		TestTrue(TEXT("Featured inspection opens"), ActivateTag(Layer, TEXT("ShopFeaturedInspect")));
		TestEqual(TEXT("Correct demo item inspected"), Layer->SelectedShopItem, 0);
		TestTrue(TEXT("Demo acquisition accepts input"), ActivateTag(Layer, TEXT("ShopAcquire")));
		TestEqual(TEXT("Existing featured price is deducted exactly once"), Layer->ShopDemo.GetCredits(), 1100);
		TestTrue(TEXT("Demo ownership updates"), Layer->ShopDemo.Owns(0));
		TestTrue(TEXT("Inspection Close accepts input"), ActivateTag(Layer, TEXT("ShopDetailClose")));
		for (int32 Filter = 0; Filter < 5; ++Filter)
		{
			TestTrue(TEXT("Each shop filter accepts input"), ActivateTag(Layer, FName(*FString::Printf(TEXT("ShopFilter%d"), Filter))));
			TestEqual(TEXT("Correct shop filter selected"), static_cast<int32>(Layer->SelectedShopFilter), Filter);
			for (int32 Item = 0; Item < FlickShopCatalog::GetItems().Num(); ++Item)
				TestEqual(TEXT("Visible native product cards match the actual catalogue filter"),
					FindVisibleTag(Layer, FName(*FString::Printf(TEXT("ShopItem%d"), Item))).IsValid(), Layer->ShopDemo.Matches(Item, static_cast<EFlickShopFilter>(Filter)));
		}
		TestTrue(TEXT("Reset remains accessible from collection"), ActivateTag(Layer, TEXT("ShopReset")));
		TestEqual(TEXT("Reset restores demo balance"), Layer->ShopDemo.GetCredits(), 2000);
		TestEqual(TEXT("Reset clears only demo collection"), Layer->ShopDemo.GetOwnedCount(), 0);
		TestTrue(TEXT("Empty collection can return to browsing"), ActivateTag(Layer, TEXT("ShopBrowse")));
		for (int32 Item = 0; Item < FlickShopCatalog::GetItems().Num(); ++Item)
		{
			TestTrue(TEXT("Every product card opens its inspection"), ActivateTag(Layer, FName(*FString::Printf(TEXT("ShopItem%d"), Item))));
			TestEqual(TEXT("Inspection uses actual card index"), Layer->SelectedShopItem, Item);
			const FKeyEvent Escape(EKeys::Escape, FModifierKeysState(), 0, false, 0, 0);
			TestTrue(TEXT("Escape closes only the inspection"), Layer->OnPreviewKeyDown(Layer->GetCachedGeometry(), Escape).IsEventHandled());
			TestEqual(TEXT("Inspection state is cleared"), Layer->SelectedShopItem, INDEX_NONE);
		}
		TestTrue(TEXT("Shop Back returns to Home"), ActivateTag(Layer, TEXT("ShopBack")));
		Layer->ShopDemo = OriginalShopDemo;
		Mode->UpdateMainMenuPresentation();
		// Exercise the redesigned session menus through their real Slate buttons.
		// This runs in the smoke test's isolated profile; it never fabricates a win.
		TestTrue(TEXT("Play reopens for class-selection smoke"), ActivateButton(Layer, TEXT("PLAY")));
		TestTrue(TEXT("Training opens for class-selection smoke"), ActivateButton(Layer, TEXT("TRAINING")));
		TestTrue(TEXT("Bot match opens class selection"), ActivateButton(Layer, TEXT("1V1 VS BOT")));
		TestTrue(TEXT("Actual class screen selected"), Mode->GetFrontendScreen() == EFlickFrontendScreen::ClassSelect);
		for (int32 Preset = 0; Preset < 4; ++Preset)
		{
			TestTrue(TEXT("Every class tab accepts input"), ActivateTag(Layer, FName(*FString::Printf(TEXT("ClassPreset%d"), Preset))));
			TestEqual(TEXT("Tab changes the real class draft"), static_cast<int32>(Layer->GetSelectedClassDraft()), Preset);
			TestTrue(TEXT("Selected class retains its opaque surface"), FindVisibleTag(Layer, FName(*FString::Printf(TEXT("ClassPresetSurface%d"), Preset))).IsValid());
			if (const auto Title = FindVisibleTag(Layer, TEXT("ClassProfileTitle")))
				TestEqual(TEXT("Class profile follows saved preset name"), StaticCastSharedPtr<STextBlock>(Title)->GetText().ToString(), Layer->GetClassDisplayName(static_cast<EFlickLineupPreset>(Preset)));
			else AddError(TEXT("Class profile title missing"));
			for (int32 Slot = 0; Slot < 4; ++Slot)
			{
				TestTrue(TEXT("All four product portraits exist"), FindVisibleTag(Layer, FName(*FString::Printf(TEXT("ClassPuckPortrait%d"), Slot))).IsValid());
				TestTrue(TEXT("Portrait archetype source is the real saved lineup"), Layer->GetSelectedClassPiece(Slot) == Mode->GetClassLoadoutPiece(static_cast<EFlickLineupPreset>(Preset), Slot));
			}
		}
		TestTrue(TEXT("Class Back accepts input"), ActivateTag(Layer, TEXT("ClassBack")));
		TestTrue(TEXT("Cancel restores the populated Training grid"), Mode->GetFrontendScreen() == EFlickFrontendScreen::ModeSelect
			&& Layer->SelectedPlayPlaylist == EFlickPlayPlaylist::Training && Layer->SelectedTrainingActivity == EFlickTrainingActivity::None);
		TestTrue(TEXT("Bot class screen reopens"), ActivateButton(Layer, TEXT("1V1 VS BOT")));
		TestTrue(TEXT("Confirm Class accepts input"), ActivateTag(Layer, TEXT("ClassConfirm")));
		TestTrue(TEXT("Confirmation starts actual bot training"), Mode->IsTrainingBotMatch() && Mode->GetFrontendScreen() == EFlickFrontendScreen::Playing);
		Mode->TogglePauseMenu();
		Layer->Invalidate(EInvalidateWidgetReason::Prepass);
		Layer->SlatePrepass(1.0f);
		TestTrue(TEXT("Centered pause panel exists"), FindVisibleTag(Layer, TEXT("CenteredPausePanel")).IsValid());
		TestTrue(TEXT("Training pause shows contextual exit"), ContainsLabel(Layer, TEXT("EXIT TRAINING")));
		TestTrue(TEXT("Pause Settings accepts input"), ActivateTag(Layer, TEXT("PauseSETTINGS")));
		TestTrue(TEXT("Pause opens actual settings"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Settings);
		TestTrue(TEXT("Settings returns to pause"), ActivateTag(Layer, TEXT("SettingsBack")));
		TestTrue(TEXT("Pause restored after settings"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Paused);
		TestTrue(TEXT("Pause Change Class accepts input"), ActivateTag(Layer, TEXT("PauseCHANGECLASS")));
		TestTrue(TEXT("In-match class selection remains deferred to next round"), Mode->IsChangingClassForNextRound());
		TestTrue(TEXT("Deferred class confirm accepts input"), ActivateTag(Layer, TEXT("ClassConfirm")));
		TestTrue(TEXT("Class change returns to pause with queued state"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Paused && Mode->HasPendingClassChanges());
		TestTrue(TEXT("Pause Resume accepts input"), ActivateTag(Layer, TEXT("PauseRESUME")));
		TestTrue(TEXT("Resume restores Playing"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Playing);
		Mode->TogglePauseMenu();
		TestTrue(TEXT("Training Exit accepts input"), ActivateTag(Layer, TEXT("PauseFORFEIT")));
		TestTrue(TEXT("Training Exit returns Home"), Mode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu);
		TestTrue(TEXT("Play reopens for BOB pause check"), ActivateButton(Layer, TEXT("PLAY")));
		TestTrue(TEXT("Training reopens for BOB pause check"), ActivateButton(Layer, TEXT("TRAINING")));
		TestTrue(TEXT("BOB bot training starts"), ActivateButton(Layer, TEXT("BOB VS BOT")));
		Mode->TogglePauseMenu();
		Layer->Invalidate(EInvalidateWidgetReason::Prepass);
		Layer->SlatePrepass(1.0f);
		if (const auto ModeLabel = FindVisibleTag(Layer, TEXT("PauseModeLabel")))
			TestEqual(TEXT("Pause never mislabels BOB as 1V1 Knockout"), StaticCastSharedPtr<STextBlock>(ModeLabel)->GetText().ToString(), FString(TEXT("BOB  //  VS BOT")));
		else AddError(TEXT("Dynamic pause mode label missing"));
		TestFalse(TEXT("BOB retains its no-class-change rule"), FindVisibleTag(Layer, TEXT("PauseCHANGECLASS")).IsValid());
		TestTrue(TEXT("BOB exit uses the same real action"), ActivateTag(Layer, TEXT("PauseFORFEIT")));
		TestTrue(TEXT("BOB exit returns Home"), Mode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu);
		Mode->SelectMatchVariant(EFlickMatchVariant::Classic);
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PackagedStartup.png"), true, false);
		UE_LOG(LogFlick, Log, TEXT("PACKAGED_STARTUP_SMOKE_TEST %s"), HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"));
		if (FParse::Param(FCommandLine::Get(), TEXT("FlickStartupSmokeQuit")))
		{
			FTimerHandle QuitTimer;
			World->GetTimerManager().SetTimer(QuitTimer, [Layer]()
			{
				UE_LOG(LogFlick, Log, TEXT("PACKAGED_STARTUP_QUIT_BUTTON %s"),
					ActivateButton(Layer, TEXT("QUIT")) ? TEXT("PASS") : TEXT("FAIL"));
			}, 2.0f, false);
		}
	}, 2.0f));
	return true;
}
#endif
