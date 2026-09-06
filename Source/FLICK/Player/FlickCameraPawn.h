#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "FlickCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
enum class EFlickTeam : uint8;

UCLASS()
class FLICK_API AFlickCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AFlickCameraPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	void ApplyCameraSettings();
	void AddCameraImpulse(float Strength);
	void SetMenuPresentation(bool bInMenuPresentation);
	void SetBobGameplayFraming(bool bInBobGameplayFraming);
	void SetCompactGameplayFraming(bool bInCompactGameplayFraming);
	void SetArenaFramingScale(float InArenaFramingScale);
	void SetGameplayViewIndex(int32 InViewIndex, bool bSnap = false);
	void RotateGameplayOrbit(float Direction, float DeltaSeconds);
	void ResetGameplayView(int32 InViewIndex, bool bResetElevation = true);
	void AdjustGameplayElevation(int32 StepDirection);
	void SetGameplayElevation(float InElevation, bool bSnap = false);
	void SetGameplayElevationLocked(bool bLocked) { bGameplayElevationLocked = bLocked; }
	void BeginCinematicReplay(const FVector& InitialFocus, EFlickTeam ShootingTeam);
	void UpdateCinematicReplay(const FVector& Focus, float NormalizedProgress, float PullbackAlpha);
	void EndCinematicReplay();
	float GetGameplayOrbitAngle() const { return GameplayOrbitAngle; }
	float GetGameplayElevationAngle() const { return GameplayElevationAngle; }
	bool IsGameplayViewTransitioning() const { return bGameplayViewTransitioning; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	FVector CameraLocation = FVector(0.0f, -1840.0f, 1425.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	FRotator CameraRotation = FRotator(-33.2f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	float FieldOfView = 49.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera", meta = (ClampMin = "0.75", ClampMax = "1.25"))
	float GameplayDistanceScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	FVector BobCameraLocation = FVector(0.0f, -1840.0f, 1425.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	FRotator BobCameraRotation = FRotator(-34.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	float BobFieldOfView = 46.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	float CompactFieldOfView = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	FVector MenuCameraLocation = FVector(170.0f, -1600.0f, 1140.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	FRotator MenuCameraRotation = FRotator(-28.5f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	float MenuFieldOfView = 47.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Image", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float ClassicBloomIntensity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Image", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float ClassicBloomThreshold = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Image", meta = (ClampMin = "-3.0", ClampMax = "3.0"))
	float ClassicExposureBias = -0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Image", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BobBloomIntensity = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Image", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float BobBloomThreshold = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Image", meta = (ClampMin = "-3.0", ClampMax = "3.0"))
	float BobExposureBias = -0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera")
	float PresentationBlendSpeed = 3.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Orbit", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float GameplayOrbitDegreesPerSecond = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Elevation", meta = (ClampMin = "-30.0", ClampMax = "0.0"))
	float LowGameplayElevation = -9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Elevation", meta = (ClampMin = "-15.0", ClampMax = "15.0"))
	float TacticalGameplayElevation = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Elevation", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float OverviewGameplayElevation = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Elevation", meta = (ClampMin = "-30.0", ClampMax = "0.0"))
	float MinimumGameplayElevation = -18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Elevation", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float MaximumGameplayElevation = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera|Elevation", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ElevationHeightPerDegree = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera Feedback")
	float MaximumShakeLocation = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera Feedback")
	float MaximumShakeRotation = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Camera Feedback")
	float ShakeDecayPerSecond = 2.8f;

private:
	void ApplyArenaPostProcess();
	FVector GetGameplayTargetLocation() const;
	FRotator GetGameplayTargetRotation() const;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UCameraComponent> Camera;

	float ShakeTrauma = 0.0f;
	float ShakeTime = 0.0f;
	float CurrentFieldOfView = 46.0f;
	float GameplayElevationAngle = 0.0f;
	float GameplayOrbitAngle = 0.0f;
	int32 GameplayElevationPresetIndex = 1;
	float ArenaFramingScale = 1.0f;
	FVector ReplayFocus = FVector::ZeroVector;
	float ReplayProgress = 0.0f;
	float ReplayPullbackAlpha = 0.0f;
	float ReplayOrbitAngle = 0.0f;
	bool bGameplayViewTransitioning = false;
	bool bGameplayOrbitManuallyControlled = false;
	bool bGameplayElevationLocked = false;
	bool bMenuPresentation = false;
	bool bBobGameplayFraming = false;
	bool bCompactGameplayFraming = false;
	bool bCinematicReplay = false;
};
