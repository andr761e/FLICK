#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FlickReplayController.generated.h"

enum class EFlickReplayCamera : uint8 { Overview, Player, Puck, Free, Cinematic };
class AFlickCameraPawn;
class UFlickReplaySubsystem;
class SWidget;

// Playback-only controller: no gameplay RPCs, progression or match inputs.
UCLASS()
class FLICK_API AFlickReplayController : public APlayerController
{
	GENERATED_BODY()
public:
	AFlickReplayController();
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UFlickReplaySubsystem* Replay() const;
	void SelectCamera(EFlickReplayCamera Value);
	EFlickReplayCamera GetCameraMode() const { return CameraMode; }
	void CycleTarget(int32 Direction);
	FString TargetLabel() const;
	FString PlayerLabel(int32 Seat) const;
	int32 PlayerCount() const;
	void SelectPlayer(int32 Seat);
	void ToggleControls();
	void ResetView();
	void ToggleFly();
	bool IsFlying() const { return bFlyLatched; }
	bool ShowsControls() const { return bControls; }
	void TogglePause();
	void Exit();
private:
	void LookPressed();
	void LookReleased();
	void WheelUp();
	void WheelDown();
	void UpdateViewLighting();
	UPROPERTY(Transient) TObjectPtr<AFlickCameraPawn> TheatreCamera;
	TSharedPtr<SWidget> TheatreUI;
	EFlickReplayCamera CameraMode = EFlickReplayCamera::Overview;
	int32 Target = 0;
	float Orbit = 0, OverviewZoom = 1;
	bool bControls = true, bLooking = false, bFlyLatched = false;
	int32 LightingContext = INDEX_NONE, LightingRevision = INDEX_NONE;
	float LastReplayTime = -1, LightingSurfaceZ = -10000;
	double NextRigCheck = 0;
	bool bSnapNextCamera = true;
	TArray<TWeakObjectPtr<class ARectLight>> ViewDeckLights;
	TArray<FTransform> DeckReferenceTransforms;
	TWeakObjectPtr<class ARectLight> ViewPuckKey, ViewPuckFill;
	FVector LastLightingEye = FVector::ZeroVector;
	float LightingArenaRadius = 650.f;
	bool bViewLightingReady = false;
	uint32 LightingSettingsHash = 0;
};
