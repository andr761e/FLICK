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
class UFlickRankingSubsystem;
class SButton;
class SBox;
class SOverlay;
class SWidget;
struct FFlickPlayerMatchStats;

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
	Stats,
	Leaderboards,
	MatchHistory
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
	TSharedRef<SWidget> BuildMainMenu();
	TSharedRef<SWidget> BuildProfile();
	TSharedRef<SWidget> BuildProfileStatsPanel();
	TSharedRef<SWidget> BuildStartupOverlay();
	TSharedRef<SWidget> BuildSocialPanel();
	TSharedRef<SWidget> BuildSocialFriendRow(int32 FriendIndex);
	TSharedRef<SWidget> BuildRecentPlayerRow(int32 RecentIndex);
	TSharedRef<SWidget> BuildPartyMemberRow(int32 PartySlot);
	TSharedRef<SWidget> BuildMainMenuPartyMember(int32 PartySlot);
	TSharedRef<SWidget> BuildItemShop();
	TSharedRef<SWidget> BuildShopItemCard(const FString& Name, const FString& Type, const FString& Mark, const FLinearColor& Accent);
	TSharedRef<SWidget> BuildModeSelect();
	TSharedRef<SWidget> BuildPrivateMatchSetup();
	TSharedRef<SWidget> BuildPrivateMatchSlot(EFlickTeam Team, int32 PlayerSlot);
	TSharedRef<SWidget> BuildOnlineBrowser();
	TSharedRef<SWidget> BuildOnlineSessionRow(int32 ResultIndex);
	TSharedRef<SWidget> BuildNetworkLobby();
	TSharedRef<SWidget> BuildLoadout();
	TSharedRef<SWidget> BuildLoadoutModeButton(EFlickMatchVariant Variant, const FString& Label);
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
	TSharedRef<SWidget> BuildMatchHud();
	TSharedRef<SWidget> BuildCinematicReplayOverlay();
	TSharedRef<SWidget> BuildScoreboardOverlay();
	TSharedRef<SWidget> BuildScoreboardTeamSection(EFlickTeam Team);
	TSharedRef<SWidget> BuildScoreboardPlayerRow(EFlickTeam Team, int32 PlayerSlot);
	TSharedRef<SWidget> BuildTeamPlate(EFlickTeam Team);
	TSharedRef<SWidget> BuildTrainingToolsPanel();
	TSharedRef<SWidget> BuildTutorialOverlay();
	TSharedRef<SWidget> BuildControlHintPanel(bool bRightSide);
	TSharedRef<SWidget> BuildPowerMeter();
	TSharedRef<SWidget> BuildCameraOrbitHint();
	TSharedRef<SWidget> BuildEventFeed();
	TSharedRef<SWidget> BuildPauseOverlay();
	TSharedRef<SWidget> BuildRoundOverOverlay();
	TSharedRef<SWidget> BuildModeCard(EFlickMatchVariant Variant);
	TSharedRef<SWidget> BuildPlayPlaylistCard(
		EFlickPlayPlaylist Playlist,
		const FString& Label,
		const FString& Summary,
		const FLinearColor& Accent,
		bool bAvailable,
		TSharedPtr<SButton>* OutButton = nullptr);
	TSharedRef<SWidget> BuildTrainingActivityCard(
		EFlickTrainingActivity Activity,
		const FString& Label,
		const FString& Summary,
		const FString& Detail,
		const FLinearColor& Accent,
		TSharedPtr<SButton>* OutButton = nullptr);
	TSharedRef<SWidget> BuildPlayFormatCard(
		int32 PlayersPerTeam,
		bool bBob,
		TSharedPtr<SButton>* OutButton = nullptr);
	TSharedRef<SWidget> MakeMenuButton(
		const FString& Label,
		const FOnClicked& OnClicked,
		bool bPrimary = false,
		bool bDanger = false,
		float Height = 50.0f,
		TSharedPtr<SButton>* OutButton = nullptr);
	TSharedRef<SWidget> MakeMainMenuButton(
		const FString& Label,
		const FOnClicked& OnClicked,
		bool bPrimary = false,
		bool bDanger = false,
		float Height = 74.0f,
		TSharedPtr<SButton>* OutButton = nullptr);
	TSharedRef<SWidget> MakeSectionLabel(const FString& Label) const;
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
	EVisibility GetMatchHudVisibility() const;
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
	FText GetScoreboardStatText(EFlickTeam Team, int32 PlayerSlot, int32 StatIndex) const;
	FText GetScoreboardTeamSummary(EFlickTeam Team) const;
	FText GetScoreboardMatchSummary() const;
	FText GetRoundResultText() const;
	FText GetRoundScoreText() const;
	FText GetPieceCountText(EFlickTeam Team) const;
	FText GetRoundsText(EFlickTeam Team) const;
	FText GetLoadoutName(EFlickTeam Team, int32 SlotIndex) const;
	FText GetLoadoutSummary(EFlickTeam Team, int32 SlotIndex) const;
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
	EFlickTeam GetClassSelectionTeam() const;
	int32 GetClassSelectionPlayerSlot() const;
	EFlickLineupPreset GetSelectedClassDraft() const;
	EFlickPieceArchetype GetSelectedClassPiece(int32 PieceSlot) const;
	float GetSelectedClassStatValue(int32 StatIndex) const;
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
	FSliderStyle SliderStyle;
	FProgressBarStyle ShotClockBarStyle;
	TSharedPtr<SButton> MainMenuDefaultButton;
	TSharedPtr<SButton> ItemShopDefaultButton;
	TSharedPtr<SButton> ProfileDefaultButton;
	TSharedPtr<SButton> ModeSelectDefaultButton;
	TSharedPtr<SButton> TrainingActivityDefaultButton;
	TSharedPtr<SButton> ModeFormatDefaultButton;
	TSharedPtr<SButton> PrivateMatchDefaultButton;
	TSharedPtr<SButton> OnlineBrowserDefaultButton;
	TSharedPtr<SButton> LobbyDefaultButton;
	TSharedPtr<SButton> LoadoutDefaultButton;
	TSharedPtr<SButton> ClassSelectDefaultButton;
	TSharedPtr<SButton> SettingsDefaultButton;
	TSharedPtr<SButton> PauseDefaultButton;
	TSharedPtr<SButton> RoundOverDefaultButton;
	TSharedPtr<SWidget> StartupOverlayWidget;
	TSharedPtr<SBox> MainMenuScreenWidget;
	TSharedPtr<SBox> ProfileScreenWidget;
	TSharedPtr<SOverlay> MainMenuInteractiveWidget;
	TSharedPtr<SBox> MainMenuSelectionWidget;
	EFlickFrontendScreen LastFocusedScreen = EFlickFrontendScreen::Playing;
	bool bLastRoundOverVisible = false;
	bool bHasAppliedInitialFocus = false;
	bool bSocialPanelOpen = false;
	bool bShowingRecentPlayers = false;
	bool bSocialPartyExpanded = true;
	bool bSocialFriendsExpanded = true;
	bool bSocialInGameExpanded = true;
	bool bSocialOnlineExpanded = true;
	bool bSocialOfflineExpanded = false;
	bool bSocialRecentExpanded = true;
	bool bStartupOverlayVisible = false;
	float StartupOverlayElapsed = 0.0f;
	float StartupOverlayHoldDuration = 0.0f;
	float StartupOverlayFadeDuration = 0.35f;
	EFlickPlayPlaylist SelectedPlayPlaylist = EFlickPlayPlaylist::None;
	EFlickTrainingActivity SelectedTrainingActivity = EFlickTrainingActivity::None;
	EFlickProfileTab SelectedProfileTab = EFlickProfileTab::Stats;
	int32 Player1SelectedLoadoutSlot = 0;
	int32 Player2SelectedLoadoutSlot = 0;
	TOptional<EFlickPieceArchetype> Player1HoveredLoadoutArchetype;
	TOptional<EFlickPieceArchetype> Player2HoveredLoadoutArchetype;
};
