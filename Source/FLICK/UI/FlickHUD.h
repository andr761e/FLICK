#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/HUD.h"
#include "FlickHUD.generated.h"

class AFlickGameState;
class AFlickGameMode;
class AFlickPlayerController;
class SFlickGameLayer;
class SWidget;

struct FFlickHudEventMessage
{
	FString Message;
	FString PointsText;
	FLinearColor Color = FLinearColor::White;
	float CreatedAt = 0.0f;
	float ExpiresAt = 0.0f;
};

enum class EFlickMenuAction : uint8
{
	None,
	OpenLoadout,
	StartMatch,
	SelectMode,
	OpenSettings,
	Quit,
	Resume,
	Restart,
	NextRound,
	EditLoadout,
	MainMenu,
	Back,
	ToggleAimGuide,
	ToggleWorldEffects,
	SetCameraShake,
	SetMasterVolume,
	SetEffectsVolume,
	SetInterfaceVolume,
	ToggleVSync,
	PreviousWindowMode,
	NextWindowMode,
	PreviousResolution,
	NextResolution,
	ApplyDisplay,
	PreviousLoadoutPiece,
	NextLoadoutPiece,
	ToggleLobbyReady,
	LeaveLobby,
	TogglePrivateSlot,
	PrivateSpectate
};

struct FFlickMenuHitRegion
{
	FBox2D Bounds;
	EFlickMenuAction Action = EFlickMenuAction::None;
	EFlickMatchVariant MatchVariant = EFlickMatchVariant::Classic;
	EFlickTeam Team = EFlickTeam::None;
	int32 PieceSlot = INDEX_NONE;
};

UCLASS()
class FLICK_API AFlickHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;
	void PushEventMessage(
		const FString& Message,
		const FLinearColor& Color,
		float Duration,
		const FString& PointsText = FString());
	void ResetPresentation();
	bool HandleMenuClick(const FVector2D& ScreenPosition);
	const TArray<FFlickHudEventMessage>& GetEventMessages() const { return EventMessages; }

private:
	void ObserveMatchState(const AFlickGameState& GameState, float Now);
	void DrawTopBar(const AFlickGameState& GameState, float Width, float Now);
	void DrawAimPresentation(const AFlickPlayerController& Controller);
	void DrawLockedKickoffPresentation(const AFlickGameMode& GameMode);
	void DrawTechnicalAimArrow(const FVector2D& Start, const FVector2D& End, const FLinearColor& Accent);
	void DrawPowerMeter(const AFlickPlayerController& Controller, float Width, float Height);
	void DrawEventFeed(float Width, float Now);
	void DrawTurnBanner(const AFlickGameState& GameState, float Width, float Height, float Now);
	void DrawRoundOver(const AFlickGameState& GameState, float Width, float Height);
	void DrawNetworkLobby(const AFlickGameState& GameState, const AFlickPlayerController& Controller, float Width, float Height);
	void DrawPrivateMatchFrontend(const AFlickGameState& GameState, const AFlickPlayerController& Controller, float Width, float Height);
	void DrawPartyFrontend(const AFlickGameState& GameState, const AFlickPlayerController& Controller, float Width, float Height);
	void DrawControls(float Height);
	void DrawTeamBlock(EFlickTeam Team, int32 ActivePieces, int32 StartingPieces, int32 ShotsTaken, int32 RoundsWon, int32 RoundsToWin, float X, float Y, float Width, bool bCurrent);
	void DrawMainMenu(const AFlickGameMode& GameMode, float Width, float Height);
	void DrawLoadoutMenu(const AFlickGameMode& GameMode, float Width, float Height);
	void DrawPauseMenu(const AFlickGameMode& GameMode, float Width, float Height);
	void DrawSettingsMenu(const AFlickGameMode& GameMode, float Width, float Height);
	void DrawMenuButton(EFlickMenuAction Action, const FString& Label, float X, float Y, float Width, float Height, const FLinearColor& Accent, bool bPrimary = false);
	void DrawModeCard(const AFlickGameMode& GameMode, EFlickMatchVariant Variant, float X, float Y, float Width, float Height);
	void DrawLoadoutSlot(const AFlickGameMode& GameMode, EFlickTeam Team, int32 SlotIndex, float X, float Y, float Width, float Height);
	void DrawArchetypeReference(float X, float Y, float Width);
	void DrawToggleRow(EFlickMenuAction Action, const FString& Label, bool bEnabled, float X, float Y, float Width);
	void DrawSliderRow(EFlickMenuAction Action, const FString& Label, float Value, float X, float Y, float Width);
	void DrawCycleRow(EFlickMenuAction PreviousAction, EFlickMenuAction NextAction, const FString& Label, const FString& Value, float X, float Y, float Width);
	void AddMenuHitRegion(
		EFlickMenuAction Action,
		const FBox2D& Bounds,
		EFlickMatchVariant Variant = EFlickMatchVariant::Classic,
		EFlickTeam Team = EFlickTeam::None,
		int32 PieceSlot = INDEX_NONE);
	bool IsMenuRegionHovered(const FBox2D& Bounds) const;
	FLinearColor GetModeColor(EFlickMatchVariant Variant) const;
	void DrawCenteredText(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Scale, bool bLargeFont = false);
	void DrawCircle(const FVector2D& Center, float Radius, const FLinearColor& Color, int32 Segments = 24, float Thickness = 1.5f);
	void DrawFilledTriangle(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color);
	void DrawPanel(float X, float Y, float Width, float Height, const FLinearColor& Color);
	void DrawShowcasePanel(float X, float Y, float Width, float Height, const FLinearColor& Background, const FLinearColor& Accent, float CutSize = 10.0f);
	FLinearColor GetPowerColor(float Power) const;

	TArray<FFlickHudEventMessage> EventMessages;
	EFlickMatchPhase LastObservedPhase = EFlickMatchPhase::WaitingToStart;
	EFlickTeam LastObservedTeam = EFlickTeam::None;
	int32 LastObservedDramaticEventSerial = 0;
	float StateChangedAt = -100.0f;
	bool bHasObservedState = false;
	TArray<FFlickMenuHitRegion> MenuHitRegions;
	FVector2D MenuCursorPosition = FVector2D(-1000.0f, -1000.0f);
	TSharedPtr<SFlickGameLayer> GameLayer;
	TSharedPtr<SWidget> GameLayerContainer;
};
