#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "Input/Reply.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

class AFlickGameMode;
class AFlickGameState;
class AFlickHUD;
class AFlickPlayerController;
class AFlickPlayerState;
class UFlickRankingSubsystem;
class UFlickSessionSubsystem;
class SWidget;
class SVerticalBox;
struct FSlateBrush;
struct FFlickPlayerMatchStats;
struct FFlickProfileStats;

enum class EFlickPlayPlaylist : uint8
{
	None,
	Casual,
	Competitive,
	Training,
	PrivateMatch
};

enum class EFlickTrainingActivity : uint8
{
	None,
	Tutorial,
	FreePlay,
	BotMatch,
	BobBotMatch
};

enum class EFlickProfileTab : uint8
{
	Overview,
	Stats,
	Leaderboards,
	MatchHistory,
	Customization
};

enum class EFlickSettingsTab : uint8
{
	GameFeel,
	Camera,
	Interface,
	Sound,
	Display,
	StreamSafe,
	Controls,
	Lighting
};

class SFlickGameLayer final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickGameLayer) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AFlickHUD>, OwnerHud)
		SLATE_ARGUMENT(TWeakObjectPtr<AFlickGameMode>, GameMode)
		SLATE_ARGUMENT(TWeakObjectPtr<AFlickPlayerController>, PlayerController)
	SLATE_END_ARGS()

	SFlickGameLayer();
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FFlickPuckInspectionTest;
#endif
	TSharedRef<SWidget> BuildMainMenu();
	TSharedRef<SWidget> BuildMainMenuFooter();
	TSharedRef<SWidget> BuildChallengePreview();
	TSharedRef<SWidget> BuildChallengesPanel();
	TSharedRef<SWidget> BuildChallengeRow(int32 ChallengeIndex, bool bCompact);
	void RefreshChallengePreview();
	FFlickProfileStats GetChallengeStats() const;
	void RefreshDisplayOptions();
	void ApplyPendingDisplaySettings();
	EFlickLineupPreset GetDisplayedLoadoutPreset() const;
	EFlickPieceArchetype GetDisplayedLoadoutPiece(int32 SlotIndex) const;
	void SetDisplayedLoadoutPiece(int32 SlotIndex, EFlickPieceArchetype Archetype);
	TSharedRef<SWidget> BuildProfile();
	TSharedRef<SWidget> BuildProfileStatsPanel();
	TSharedRef<SWidget> BuildStartupOverlay();
	TSharedRef<SWidget> BuildSocialPanel();
	TSharedRef<SWidget> BuildSocialFriendRow(int32 FriendIndex);
	TSharedRef<SWidget> BuildRecentPlayerRow(int32 RecentIndex);
	void RebuildSocialPlayerList();
	TSharedRef<SWidget> BuildPartyMemberRow(int32 PartySlot);
	TSharedRef<SWidget> BuildMainMenuPartyMember(int32 PartySlot);
	TSharedRef<SWidget> BuildItemShop();
	TSharedRef<SWidget> BuildShopItemCard(const FString& Name, const FString& Type, const FString& Mark, const FLinearColor& Accent);
	TSharedRef<SWidget> BuildModeSelect();
	TSharedRef<SWidget> BuildPrivateMatchSetup();
	TSharedRef<SWidget> BuildPrivateMatchTeamPicker();
	TSharedRef<SWidget> BuildOnlineBrowser();
	TSharedRef<SWidget> BuildOnlineSessionRow(int32 ResultIndex);
	TSharedRef<SWidget> BuildMatchmakingStatusBar();
	TSharedRef<SWidget> BuildPartyInvitePrompt();
	TSharedRef<SWidget> BuildNetworkLobby();
	TSharedRef<SWidget> BuildLoadout();
	TSharedRef<SWidget> BuildLoadoutWorkspace();
	TSharedRef<SWidget> BuildLineupRadar();
	TSharedRef<SWidget> BuildLoadoutFormation(EFlickTeam Team);
	TSharedRef<SWidget> BuildFormationPuck(EFlickTeam Team, int32 SlotIndex);
	TSharedRef<SWidget> BuildLoadoutComparison(EFlickTeam Team);
	TSharedRef<SWidget> BuildLoadoutStatRow(EFlickTeam Team, const FString& Label, int32 StatIndex);
	TSharedRef<SWidget> BuildLoadoutPresetBar(EFlickTeam Team);
	TSharedRef<SWidget> BuildClassSelect();
	TSharedRef<SWidget> BuildClassPlayerRow(EFlickTeam Team, int32 PlayerSlot);
	TSharedRef<SWidget> BuildClassShowcase();
	TSharedRef<SWidget> BuildClassPuckCard(int32 PieceSlot);
	TSharedRef<SWidget> BuildClassProfileStatRow(const FString& Label, int32 StatIndex);
	TSharedRef<SWidget> BuildArchetypePicker(EFlickTeam Team);
	TSharedRef<SWidget> BuildArchetypeChoice(EFlickTeam Team, EFlickPieceArchetype Archetype);
	TSharedRef<SWidget> BuildSettings();
	TSharedRef<SWidget> BuildLightingSettings();
	TSharedRef<SWidget> BuildMatchHud();
	TSharedRef<SWidget> BuildCinematicReplayOverlay();
	TSharedRef<SWidget> BuildScoreboardOverlay(bool bEmbedded = false);
	TSharedRef<SWidget> BuildScoreboardTeamSection(EFlickTeam Team);
	TSharedRef<SWidget> BuildScoreboardPlayerRow(EFlickTeam Team, int32 PlayerSlot);
	TSharedRef<SWidget> BuildTeamPlate(EFlickTeam Team);
	TSharedRef<SWidget> BuildTrainingToolsPanel();
	TSharedRef<SWidget> BuildTutorialOverlay();
	TSharedRef<SWidget> BuildControlHintPanel(bool bRightSide);
	TSharedRef<SWidget> BuildPowerMeter();
	TSharedRef<SWidget> BuildCameraOrbitHint();
	TSharedRef<SWidget> BuildPrivateSpectatorCard();
	TSharedRef<SWidget> BuildTeamPingFeed();
	TSharedRef<SWidget> BuildEventFeed();
	TSharedRef<SWidget> BuildPauseOverlay();
	TSharedRef<SWidget> BuildRoundOverOverlay();
	TSharedRef<SWidget> BuildPostMatchOverlay();
	TSharedRef<SWidget> BuildModeCard(EFlickMatchVariant Variant);
	TSharedRef<SWidget> BuildPlayPlaylistCard(
		EFlickPlayPlaylist Playlist,
		const FString& Label,
		const FString& Summary);
	TSharedRef<SWidget> BuildTrainingActivityCard(
		EFlickTrainingActivity Activity,
		const FString& Label,
		const FString& Summary,
		const FString& Detail,
		const FLinearColor& Accent);
	TSharedRef<SWidget> BuildPlayFormatCard(
		int32 PlayersPerTeam,
		bool bBob);
	TSharedRef<SWidget> MakeMenuButton(
		const FString& Label,
		const FOnClicked& OnClicked,
		bool bPrimary = false,
		bool bDanger = false,
		float Height = 50.0f);
	TSharedRef<SWidget> MakeMainMenuButton(
		const FString& Label,
		const FOnClicked& OnClicked,
		bool bPrimary = false,
		bool bDanger = false,
		float Height = 74.0f);
	TSharedRef<SWidget> MakeToggleRow(
		const FString& Label,
		const TAttribute<ECheckBoxState>& State,
		const FOnCheckStateChanged& OnChanged) const;
	TSharedRef<SWidget> MakeSliderRow(
		const FString& Label,
		const TAttribute<float>& Value,
		const FOnFloatValueChanged& OnChanged) const;
	TSharedRef<SWidget> MakeCycleRow(
		const FString& Label,
		const TAttribute<FText>& Value,
		const FOnClicked& Previous,
		const FOnClicked& Next) const;

	EVisibility GetScreenVisibility(EFlickFrontendScreen Screen) const;
	EVisibility GetMatchmakingStatusVisibility() const;
	EVisibility GetMatchHudVisibility() const;
	bool ShouldShowGameplayControls() const;
	EVisibility GetScoreboardVisibility() const;
	EVisibility GetScoreboardPlayerVisibility(int32 PlayerSlot) const;
	EVisibility GetRoundOverVisibility() const;
	EVisibility GetPowerVisibility() const;
	EVisibility GetLoadoutRowVisibility(int32 SlotIndex) const;
	EVisibility GetEventVisibility(int32 IndexFromNewest) const;
	FText GetEventText(int32 IndexFromNewest) const;
	FSlateColor GetEventColor(int32 IndexFromNewest) const;
	EVisibility GetEventPointsVisibility(int32 IndexFromNewest) const;
	FText GetEventPointsText(int32 IndexFromNewest) const;
	FText GetMatchStatusText() const;
	FSlateColor GetMatchStatusColor() const;
	FText GetNextTurnText() const;
	FSlateColor GetNextTurnColor() const;
	EVisibility GetNextTurnVisibility() const;
	FText GetScoreboardPlayerName(EFlickTeam Team, int32 PlayerSlot) const;
	const FSlateBrush* GetScoreboardPlayerAvatarBrush(EFlickTeam Team, int32 PlayerSlot) const;
	const AFlickPlayerState* FindScoreboardPlayerState(EFlickTeam Team, int32 PlayerSlot) const;
	FText GetScoreboardStatText(EFlickTeam Team, int32 PlayerSlot, int32 StatIndex) const;
	FText GetScoreboardPingText(EFlickTeam Team, int32 PlayerSlot) const;
	FLinearColor GetScoreboardPingColor(EFlickTeam Team, int32 PlayerSlot) const;
	FText GetScoreboardTeamSummary(EFlickTeam Team) const;
	FText GetScoreboardMatchSummary() const;
	FText GetRoundResultText() const;
	FText GetRoundScoreText() const;
	FText GetPieceCountText(EFlickTeam Team) const;
	FText GetRoundsText(EFlickTeam Team) const;
	int32 GetSelectedLoadoutSlot(EFlickTeam Team) const;
	void SelectLoadoutSlot(EFlickTeam Team, int32 SlotIndex);
	EFlickPieceArchetype GetPreviewLoadoutArchetype(EFlickTeam Team) const;
	float GetLoadoutStatValue(EFlickTeam Team, int32 StatIndex, bool bPreview) const;
	TArray<float> GetLineupProfileStats(EFlickTeam Team) const;
	void SetHoveredLoadoutArchetype(EFlickTeam Team, EFlickPieceArchetype Archetype);
	void ClearHoveredLoadoutArchetype(EFlickTeam Team, EFlickPieceArchetype Archetype);
	FText GetPowerText() const;
	FSlateColor GetPowerColor() const;
	FLinearColor GetModeAccent(EFlickMatchVariant Variant) const;
	FLinearColor GetCurrentModeAccent() const;
	FLinearColor GetTeamAccent(EFlickTeam Team) const;
	const FFlickPrivateMatchSettings& GetDisplayedPrivateMatchSettings() const;
	UFlickSessionSubsystem* GetDisplayedSessionSubsystem() const;
	bool IsDisplayedPartyActive() const;
	AFlickPlayerState* GetDisplayedPartyMember(int32 PartySlot) const;
	bool HasDisplayedPartyMember(int32 PartySlot) const;
	FString GetDisplayedPartyMemberName(int32 PartySlot) const;
	FString GetDisplayedPartyMemberUserId(int32 PartySlot) const;
	bool IsDisplayedPartyMemberLeader(int32 PartySlot) const;
	bool IsLocalDisplayedPartyLeader() const;
	int32 GetDisplayedPartyMemberCount() const;
	EFlickTeam GetClassSelectionTeam() const;
	int32 GetClassSelectionPlayerSlot() const;
	EFlickLineupPreset GetSelectedClassDraft() const;
	EFlickPieceArchetype GetSelectedClassPiece(int32 PieceSlot) const;
	float GetSelectedClassStatValue(int32 StatIndex) const;
	FString GetClassDisplayName(EFlickLineupPreset Preset) const;
	const AFlickGameState* GetScoreboardGameState() const;
	UFlickRankingSubsystem* GetRankingSubsystem() const;

	TWeakObjectPtr<AFlickHUD> OwnerHud;
	TWeakObjectPtr<AFlickGameMode> GameMode;
	TWeakObjectPtr<AFlickPlayerController> PlayerController;
	FButtonStyle MenuButtonStyle;
	FButtonStyle PrimaryButtonStyle;
	FButtonStyle DangerButtonStyle;
	FButtonStyle CompactMenuButtonStyle;
	FButtonStyle CompactPrimaryButtonStyle;
	FButtonStyle CompactDangerButtonStyle;
	FButtonStyle TransparentButtonStyle;
	FCheckBoxStyle ToggleStyle;
	FComboBoxStyle DropdownStyle;
	FTableRowStyle DropdownRowStyle;
	FSliderStyle SliderStyle;
	FProgressBarStyle ShotClockBarStyle;
	TSharedPtr<SWidget> StartupOverlayWidget;
	TSharedPtr<SBox> MainMenuQuitCard;
	TSharedPtr<SBox> MainMenuProfileCard;
	TSharedPtr<SBox> MainMenuPuckAnchor;
	EFlickFrontendScreen LastFocusedScreen = EFlickFrontendScreen::Playing;
	bool bLastRoundOverVisible = false;
	bool bHasAppliedInitialFocus = false;
	bool bSocialPanelOpen = false;
	bool bShowingRecentPlayers = false;
	bool bShowingOnlineFriends = false;
	bool bSocialPartyExpanded = true;
	bool bSocialFriendsExpanded = true;
	bool bSocialInGameExpanded = true;
	bool bSocialOnlineExpanded = true;
	bool bSocialOfflineExpanded = true;
	bool bSocialRecentExpanded = true;
	TSharedPtr<SVerticalBox> SocialPlayerList;
	TSharedPtr<SBox> InvitePromptWidget;
	TSharedPtr<SBox> MainMenuPartyTray;
	TSharedPtr<SWidget> MainMenuDiagonalPanel;
	TSharedPtr<SVerticalBox> ChallengePreviewRows;
	bool bChallengesOpen = false;
	int32 SelectedChallengeCategory = 0;
	uint32 LastChallengePreviewSignature = 0;
	float InvitePromptElapsed = 0.0f;
	bool bInvitePromptWasVisible = false;
	float PartyTrayElapsed = 0.0f;
	bool bPartyTrayWasVisible = false;
	bool bInvitePromptPreview = false;
	bool bPartyTrayPreview = false;
	bool bReplayOverlayPreview = false;
	bool bFpsPreview = false;
	int32 CachedSocialFriendCount = INDEX_NONE;
	int32 CachedSocialRecentCount = INDEX_NONE;
	bool bStartupOverlayVisible = false;
	float StartupOverlayElapsed = 0.0f;
	float StartupOverlayHoldDuration = 0.0f;
	float StartupOverlayFadeDuration = 0.35f;
	FVector2D LayerLocalSize = FVector2D(1920.0f, 1080.0f);
	FVector2D ViewportLocalSize = FVector2D(1920.0f, 1080.0f);
	double FooterTickerElapsed = 0.0;
	bool bFooterTickerWasVisible = false;
	EFlickPlayPlaylist SelectedPlayPlaylist = EFlickPlayPlaylist::None;
	int32 SelectedPlayFormat = 0;
	EFlickTrainingActivity SelectedTrainingActivity = EFlickTrainingActivity::None;
	EFlickProfileTab SelectedProfileTab = EFlickProfileTab::Overview;
	EFlickProfileTab AnimatedProfileTab = EFlickProfileTab::Overview;
	float ProfileEntranceElapsed = 0.0f;
	bool bProfileWasVisible = false;
	int32 SelectedProfileStatsView = 0; // Total, Casual, Competitive.
	int32 SelectedBannerStyle = 0;
	int32 SelectedBannerTag = 0;
	int32 SelectedAvatarBorder = 0;
	int32 SelectedLockerCategory = 0;
	TArray<int32> SelectedPuckSkins;
	TArray<int32> SelectedPuckEffects;
	int32 GetSelectedCosmeticIndex(int32 Category) const;
	int32 LastLockerPreviewCategory = INDEX_NONE;
	EFlickSettingsTab SelectedSettingsTab = EFlickSettingsTab::GameFeel;
	bool bEditMenuLighting = true;
	bool bLightingPreview = false;
	TArray<TSharedPtr<int32>> LightingSceneOptions;
	float SmoothedFrameSeconds = 1.0f / 60.0f;
	TArray<TSharedPtr<FIntPoint>> ResolutionOptions;
	TArray<TSharedPtr<int32>> WindowModeOptions;
	TArray<TSharedPtr<int32>> CameraShakeOptions;
	TArray<TSharedPtr<int32>> PuckHoverSizeOptions;
	TArray<TSharedPtr<int32>> PuckHoverDetailOptions;
	TArray<TSharedPtr<int32>> PuckIndicatorOptions;
	TArray<TSharedPtr<int32>> FrameLimitOptions;
	FIntPoint PendingResolution = FIntPoint::ZeroValue;
	int32 PendingWindowMode = 1; // 0 fullscreen, 1 borderless, 2 windowed.
	int32 PendingFrameLimit = 0; // Zero is uncapped.
	bool bPendingVSync = false;
	bool bDisplayOptionsInitialized = false;
	FString ControlBindingMessage;
	// Remote party members do not own the authoritative GameMode. Their
	// non-gameplay frontend navigation therefore remains local to their Slate UI.
	EFlickFrontendScreen RemotePartyScreen = EFlickFrontendScreen::MainMenu;
	EFlickLineupPreset RemoteLoadoutPreset = EFlickLineupPreset::Balanced;
	int32 Player1SelectedLoadoutSlot = 0;
	int32 Player2SelectedLoadoutSlot = 0;
	TOptional<EFlickPieceArchetype> Player1HoveredLoadoutArchetype;
	TOptional<EFlickPieceArchetype> Player2HoveredLoadoutArchetype;
};
