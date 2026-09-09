#include "Player/FlickCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Core/FlickTypes.h"
#include "Kismet/KismetMathLibrary.h"

AFlickCameraPawn::AFlickCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SceneRoot);
	Camera->bUsePawnControlRotation = false;
	Camera->PostProcessBlendWeight = 1.0f;
	Camera->PostProcessSettings.bOverride_VignetteIntensity = true;
	Camera->PostProcessSettings.VignetteIntensity = 0.43f;
	Camera->PostProcessSettings.bOverride_ColorContrast = true;
	Camera->PostProcessSettings.ColorContrast = FVector4(1.16f, 1.16f, 1.16f, 1.0f);
	Camera->PostProcessSettings.bOverride_ColorSaturation = true;
	Camera->PostProcessSettings.ColorSaturation = FVector4(1.02f, 1.02f, 1.02f, 1.0f);
	Camera->PostProcessSettings.bOverride_ColorGamma = true;
	Camera->PostProcessSettings.ColorGamma = FVector4(0.96f, 0.975f, 1.0f, 1.0f);
	Camera->PostProcessSettings.bOverride_BloomIntensity = true;
	Camera->PostProcessSettings.BloomIntensity = ClassicBloomIntensity;
	Camera->PostProcessSettings.bOverride_BloomThreshold = true;
	Camera->PostProcessSettings.BloomThreshold = ClassicBloomThreshold;
	Camera->PostProcessSettings.bOverride_AutoExposureBias = true;
	Camera->PostProcessSettings.AutoExposureBias = ClassicExposureBias;
	Camera->PostProcessSettings.bOverride_AmbientOcclusionIntensity = true;
	Camera->PostProcessSettings.AmbientOcclusionIntensity = 1.28f;
	Camera->PostProcessSettings.bOverride_AmbientOcclusionRadius = true;
	Camera->PostProcessSettings.AmbientOcclusionRadius = 64.0f;
	Camera->PostProcessSettings.bOverride_LensFlareIntensity = true;
	Camera->PostProcessSettings.LensFlareIntensity = 0.0f;
	Camera->PostProcessSettings.bOverride_FilmGrainIntensity = true;
	Camera->PostProcessSettings.FilmGrainIntensity = 0.07f;
}

void AFlickCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	ApplyCameraSettings();
}

void AFlickCameraPawn::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Camera)
	{
		return;
	}
	if (bFreeCameraEnabled)
	{
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->SetFieldOfView(CurrentFieldOfView);
		bGameplayViewTransitioning = false;
		return;
	}

	ShakeTime += DeltaSeconds;
	const float MenuDrift = bMenuPresentation ? FMath::Sin(ShakeTime * 0.2f) * 30.0f : 0.0f;
	const float MenuDepthDrift = bMenuPresentation ? FMath::Sin(ShakeTime * 0.15f + 1.1f) * 13.0f : 0.0f;
	const float MenuLift = bMenuPresentation ? FMath::Sin(ShakeTime * 0.18f + 2.0f) * 9.0f : 0.0f;
	const FVector ScaledMenuLocation = MenuCameraLocation * ArenaFramingScale;
	FVector TargetLocation = (bMenuPresentation ? ScaledMenuLocation : GetGameplayTargetLocation())
		+ FVector(MenuDrift, MenuDepthDrift, MenuLift);
	FRotator TargetRotation = (bMenuPresentation ? MenuCameraRotation : GetGameplayTargetRotation())
		+ (bMenuPresentation
			? FRotator(FMath::Sin(ShakeTime * 0.17f) * 0.18f, FMath::Sin(ShakeTime * 0.2f + 0.8f) * 0.48f, 0.0f)
			: FRotator::ZeroRotator);
	float TargetFieldOfView = bMenuPresentation
		? MenuFieldOfView
		: bAimPresentation && bTestArenaPresentation
			? AimFieldOfView
			: bBobGameplayFraming ? BobFieldOfView : bCompactGameplayFraming ? CompactFieldOfView : FieldOfView;
	float BlendSpeed = PresentationBlendSpeed;
	if (bCinematicReplay)
	{
		const float ArcAngle = ReplayOrbitAngle + FMath::Lerp(-7.0f, 11.0f, ReplayProgress);
		const float ReplayDistance = FMath::Lerp(720.0f, 1120.0f, ReplayPullbackAlpha) * ArenaFramingScale;
		const float ReplayHeight = FMath::Lerp(430.0f, 650.0f, ReplayPullbackAlpha) * ArenaFramingScale;
		const FVector CameraOffset = FVector(0.0f, -ReplayDistance, ReplayHeight)
			.RotateAngleAxis(ArcAngle, FVector::UpVector);
		TargetLocation = ReplayFocus + CameraOffset;
		TargetRotation = UKismetMathLibrary::FindLookAtRotation(
			TargetLocation,
			ReplayFocus + FVector(0.0f, 0.0f, 28.0f));
		const float EstablishedFieldOfView = FMath::Lerp(33.0f, 39.0f, ReplayPullbackAlpha);
		TargetFieldOfView = EstablishedFieldOfView - 3.5f * FMath::Sin(ReplayProgress * PI);
		BlendSpeed = 5.5f;
	}
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, BlendSpeed);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, BlendSpeed);
	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
	bGameplayViewTransitioning = !bMenuPresentation
		&& (!NewLocation.Equals(TargetLocation, 1.0f) || !NewRotation.Equals(TargetRotation, 0.08f));
	CurrentFieldOfView = FMath::FInterpTo(CurrentFieldOfView, TargetFieldOfView, DeltaSeconds, BlendSpeed);
	ShakeTrauma = FMath::Max(0.0f, ShakeTrauma - ShakeDecayPerSecond * DeltaSeconds);
	const float ShakeAmount = FMath::Square(ShakeTrauma);
	if (ShakeAmount <= KINDA_SMALL_NUMBER)
	{
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->SetFieldOfView(CurrentFieldOfView);
		return;
	}

	const float LocationWave = FMath::Sin(ShakeTime * 37.0f) * MaximumShakeLocation * ShakeAmount;
	const float SideWave = FMath::Sin(ShakeTime * 29.0f + 1.7f) * MaximumShakeLocation * 0.55f * ShakeAmount;
	const float PitchWave = FMath::Sin(ShakeTime * 31.0f + 0.6f) * MaximumShakeRotation * ShakeAmount;
	const float YawWave = FMath::Sin(ShakeTime * 23.0f + 2.1f) * MaximumShakeRotation * 0.65f * ShakeAmount;

	Camera->SetRelativeLocation(FVector(SideWave, LocationWave, 0.0f));
	Camera->SetRelativeRotation(FRotator(PitchWave, YawWave, -YawWave * 0.35f));
	Camera->SetFieldOfView(CurrentFieldOfView + ShakeAmount * 0.7f);
}

void AFlickCameraPawn::ApplyCameraSettings()
{
	ShakeTrauma = 0.0f;
	SetActorLocation(bMenuPresentation ? MenuCameraLocation * ArenaFramingScale : GetGameplayTargetLocation());
	SetActorRotation(bMenuPresentation ? MenuCameraRotation : GetGameplayTargetRotation());
	CurrentFieldOfView = bMenuPresentation
		? MenuFieldOfView
		: bBobGameplayFraming ? BobFieldOfView : bCompactGameplayFraming ? CompactFieldOfView : FieldOfView;
	bGameplayViewTransitioning = false;
	if (Camera)
	{
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->SetFieldOfView(CurrentFieldOfView);
	}
}

void AFlickCameraPawn::SetMenuPresentation(const bool bInMenuPresentation)
{
	bMenuPresentation = bInMenuPresentation;
	if (bMenuPresentation)
	{
		bGameplayOrbitManuallyControlled = false;
		bGameplayElevationLocked = false;
	}
	bGameplayViewTransitioning = !bMenuPresentation;
	ShakeTrauma = 0.0f;
}

void AFlickCameraPawn::SetBobGameplayFraming(const bool bInBobGameplayFraming)
{
	bBobGameplayFraming = bInBobGameplayFraming;
	ApplyArenaPostProcess();
}

void AFlickCameraPawn::SetTestArenaPresentation(const bool bInTestArenaPresentation)
{
	bTestArenaPresentation = bInTestArenaPresentation;
	if (!bTestArenaPresentation)
	{
		bAimPresentation = false;
		AimFocusPoint = FVector::ZeroVector;
	}
	ApplyArenaPostProcess();
}

void AFlickCameraPawn::SetAimPresentation(const bool bEnabled, const FVector& FocusPoint)
{
	bAimPresentation = bEnabled && bTestArenaPresentation && !bMenuPresentation && !bCinematicReplay;
	if (bAimPresentation)
	{
		AimFocusPoint = FVector(FocusPoint.X, FocusPoint.Y, 0.0f);
	}
}

void AFlickCameraPawn::ApplyArenaPostProcess()
{
	if (!Camera)
	{
		return;
	}

	Camera->PostProcessSettings.BloomIntensity = bTestArenaPresentation
		? TestArenaBloomIntensity
		: bBobGameplayFraming
		? BobBloomIntensity
		: ClassicBloomIntensity;
	Camera->PostProcessSettings.BloomThreshold = bTestArenaPresentation
		? TestArenaBloomThreshold
		: bBobGameplayFraming
		? BobBloomThreshold
		: ClassicBloomThreshold;
	Camera->PostProcessSettings.AutoExposureBias = bBobGameplayFraming
		? BobExposureBias
		: ClassicExposureBias;

	// The test arena's numerous emissive puck and divider accents cause the
	// histogram exposure to begin attractively bright, then rapidly darken. Keep
	// that initial EV stable in this arena only. Disabling both overrides again
	// restores the normal adaptive exposure used by every other mode.
	Camera->PostProcessSettings.bOverride_AutoExposureMinBrightness = bTestArenaPresentation;
	Camera->PostProcessSettings.bOverride_AutoExposureMaxBrightness = bTestArenaPresentation;
	if (bTestArenaPresentation)
	{
		Camera->PostProcessSettings.AutoExposureMinBrightness = TestArenaFixedExposure;
		Camera->PostProcessSettings.AutoExposureMaxBrightness = TestArenaFixedExposure;
	}

	// Tighten every Gaussian bloom stage in Switchyard so the halo hugs the
	// authored strips. Other modes inherit Unreal's normal kernel sizes.
	Camera->PostProcessSettings.bOverride_Bloom1Size = bTestArenaPresentation;
	Camera->PostProcessSettings.bOverride_Bloom2Size = bTestArenaPresentation;
	Camera->PostProcessSettings.bOverride_Bloom3Size = bTestArenaPresentation;
	Camera->PostProcessSettings.bOverride_Bloom4Size = bTestArenaPresentation;
	Camera->PostProcessSettings.bOverride_Bloom5Size = bTestArenaPresentation;
	Camera->PostProcessSettings.bOverride_Bloom6Size = bTestArenaPresentation;
	if (bTestArenaPresentation)
	{
		Camera->PostProcessSettings.Bloom1Size = 0.20f;
		Camera->PostProcessSettings.Bloom2Size = 0.62f;
		Camera->PostProcessSettings.Bloom3Size = 1.15f;
		Camera->PostProcessSettings.Bloom4Size = 4.0f;
		Camera->PostProcessSettings.Bloom5Size = 10.0f;
		Camera->PostProcessSettings.Bloom6Size = 22.0f;
	}
}

void AFlickCameraPawn::SetCompactGameplayFraming(const bool bInCompactGameplayFraming)
{
	bCompactGameplayFraming = bInCompactGameplayFraming;
}

void AFlickCameraPawn::SetArenaFramingScale(const float InArenaFramingScale)
{
	ArenaFramingScale = FMath::Clamp(InArenaFramingScale, 0.8f, 1.75f);
	bGameplayViewTransitioning = !bMenuPresentation;
}

void AFlickCameraPawn::SetGameplayViewIndex(const int32 InViewIndex, const bool bSnap)
{
	// Automatic turn-facing changes remain useful until the player chooses a
	// custom orbit. After that, preserve their chosen angle for the whole match.
	if (bGameplayOrbitManuallyControlled && !bSnap)
	{
		return;
	}

	constexpr int32 ViewCount = 4;
	const int32 NormalizedIndex = ((InViewIndex % ViewCount) + ViewCount) % ViewCount;
	const float NewOrbitAngle = static_cast<float>(NormalizedIndex) * 90.0f;
	if (FMath::IsNearlyEqual(GameplayOrbitAngle, NewOrbitAngle) && !bSnap)
	{
		return;
	}

	GameplayOrbitAngle = NewOrbitAngle;
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = !bMenuPresentation && !bSnap;
	if (bSnap && !bMenuPresentation)
	{
		SetActorLocation(GetGameplayTargetLocation());
		SetActorRotation(GetGameplayTargetRotation());
		bGameplayViewTransitioning = false;
	}
}

void AFlickCameraPawn::RotateGameplayOrbit(const float Direction, const float DeltaSeconds)
{
	if (FMath::IsNearlyZero(Direction) || DeltaSeconds <= 0.0f || bMenuPresentation)
	{
		return;
	}

	GameplayOrbitAngle = FRotator::NormalizeAxis(
		GameplayOrbitAngle
		+ FMath::Clamp(Direction, -1.0f, 1.0f) * GameplayOrbitDegreesPerSecond * DeltaSeconds);
	bGameplayOrbitManuallyControlled = true;
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = true;
}

void AFlickCameraPawn::ResetGameplayView(const int32 InViewIndex, const bool bResetElevation)
{
	bGameplayOrbitManuallyControlled = false;
	SetGameplayViewIndex(InViewIndex, false);
	if (bResetElevation && !bGameplayElevationLocked)
	{
		GameplayElevationPresetIndex = 1;
		SetGameplayElevation(
			bTestArenaPresentation ? TestArenaTacticalGameplayElevation : TacticalGameplayElevation,
			false);
	}
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = !bMenuPresentation;
}

void AFlickCameraPawn::AdjustGameplayElevation(const int32 StepDirection)
{
	if (StepDirection == 0 || bMenuPresentation || bGameplayElevationLocked)
	{
		return;
	}
	GameplayElevationPresetIndex = FMath::Clamp(
		GameplayElevationPresetIndex + (StepDirection > 0 ? 1 : -1),
		0,
		2);
	const float PresetElevations[] =
	{
		bTestArenaPresentation ? TestArenaLowGameplayElevation : LowGameplayElevation,
		bTestArenaPresentation ? TestArenaTacticalGameplayElevation : TacticalGameplayElevation,
		bTestArenaPresentation ? TestArenaOverviewGameplayElevation : OverviewGameplayElevation
	};
	SetGameplayElevation(PresetElevations[GameplayElevationPresetIndex]);
}

void AFlickCameraPawn::SetGameplayElevation(const float InElevation, const bool bSnap)
{
	if (bMenuPresentation)
	{
		return;
	}

	const float Minimum = FMath::Min(MinimumGameplayElevation, MaximumGameplayElevation);
	const float Maximum = FMath::Max(MinimumGameplayElevation, MaximumGameplayElevation);
	const float NewElevation = FMath::Clamp(InElevation, Minimum, Maximum);
	if (FMath::IsNearlyEqual(NewElevation, GameplayElevationAngle) && !bSnap)
	{
		return;
	}

	GameplayElevationAngle = NewElevation;
	const float PresetElevations[] =
	{
		bTestArenaPresentation ? TestArenaLowGameplayElevation : LowGameplayElevation,
		bTestArenaPresentation ? TestArenaTacticalGameplayElevation : TacticalGameplayElevation,
		bTestArenaPresentation ? TestArenaOverviewGameplayElevation : OverviewGameplayElevation
	};
	float ClosestDistance = TNumericLimits<float>::Max();
	for (int32 PresetIndex = 0; PresetIndex < UE_ARRAY_COUNT(PresetElevations); ++PresetIndex)
	{
		const float Distance = FMath::Abs(NewElevation - PresetElevations[PresetIndex]);
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			GameplayElevationPresetIndex = PresetIndex;
		}
	}
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = !bSnap;
	if (bSnap)
	{
		SetActorLocation(GetGameplayTargetLocation());
		SetActorRotation(GetGameplayTargetRotation());
		bGameplayViewTransitioning = false;
	}
}

void AFlickCameraPawn::BeginCinematicReplay(const FVector& InitialFocus, const EFlickTeam ShootingTeam)
{
	bCinematicReplay = true;
	ReplayFocus = InitialFocus;
	ReplayProgress = 0.0f;
	ReplayPullbackAlpha = 0.0f;
	ReplayOrbitAngle = ShootingTeam == EFlickTeam::Player2 ? 180.0f : 0.0f;
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = true;
}

void AFlickCameraPawn::UpdateCinematicReplay(
	const FVector& Focus,
	const float NormalizedProgress,
	const float PullbackAlpha)
{
	if (!bCinematicReplay)
	{
		return;
	}
	ReplayFocus = FMath::VInterpTo(ReplayFocus, Focus, GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f, 7.0f);
	ReplayProgress = FMath::Clamp(NormalizedProgress, 0.0f, 1.0f);
	ReplayPullbackAlpha = FMath::Clamp(PullbackAlpha, 0.0f, 1.0f);
}

void AFlickCameraPawn::EndCinematicReplay()
{
	bCinematicReplay = false;
	ReplayProgress = 0.0f;
	ReplayPullbackAlpha = 0.0f;
	bGameplayViewTransitioning = true;
}

void AFlickCameraPawn::SetFreeCameraEnabled(const bool bEnabled)
{
	if (bFreeCameraEnabled == bEnabled || (bEnabled && (bMenuPresentation || bCinematicReplay)))
	{
		return;
	}
	bFreeCameraEnabled = bEnabled;
	bAimPresentation = false;
	ShakeTrauma = 0.0f;
	if (!bFreeCameraEnabled)
	{
		bGameplayViewTransitioning = true;
	}
}

void AFlickCameraPawn::AddFreeCameraInput(
	const float Forward,
	const float Right,
	const float Up,
	const FVector2D& LookDelta,
	const bool bBoost,
	const float DeltaSeconds)
{
	if (!bFreeCameraEnabled || DeltaSeconds <= 0.0f)
	{
		return;
	}

	FRotator Rotation = GetActorRotation();
	Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + LookDelta.X * FreeCameraMouseSensitivity);
	Rotation.Pitch = FMath::Clamp(
		Rotation.Pitch - LookDelta.Y * FreeCameraMouseSensitivity,
		-85.0f,
		10.0f);
	Rotation.Roll = 0.0f;
	SetActorRotation(Rotation);

	FVector Movement = GetActorForwardVector() * FMath::Clamp(Forward, -1.0f, 1.0f)
		+ GetActorRightVector() * FMath::Clamp(Right, -1.0f, 1.0f)
		+ FVector::UpVector * FMath::Clamp(Up, -1.0f, 1.0f);
	if (!Movement.IsNearlyZero())
	{
		Movement.Normalize();
	}
	const float Speed = FreeCameraMoveSpeed * (bBoost ? FreeCameraBoostMultiplier : 1.0f);
	FVector NewLocation = GetActorLocation() + Movement * Speed * DeltaSeconds;
	const float MaximumRadius = FreeCameraMaximumRadius * ArenaFramingScale;
	FVector2D Planar(NewLocation.X, NewLocation.Y);
	if (Planar.SizeSquared() > FMath::Square(MaximumRadius))
	{
		Planar = Planar.GetSafeNormal() * MaximumRadius;
		NewLocation.X = Planar.X;
		NewLocation.Y = Planar.Y;
	}
	NewLocation.Z = FMath::Clamp(
		NewLocation.Z,
		FreeCameraMinimumHeight,
		FreeCameraMaximumHeight * ArenaFramingScale);
	SetActorLocation(NewLocation);
}

FVector AFlickCameraPawn::GetGameplayTargetLocation() const
{
	FVector TargetLocation = (bBobGameplayFraming ? BobCameraLocation : CameraLocation)
		* ArenaFramingScale
		* FMath::Clamp(GameplayDistanceScale, 0.75f, 1.25f)
		* (bTestArenaPresentation ? FMath::Clamp(TestArenaDistanceMultiplier, 0.75f, 1.0f) : 1.0f)
		* (bAimPresentation ? FMath::Clamp(AimDistanceMultiplier, 0.82f, 1.0f) : 1.0f);
	TargetLocation.Z += GameplayElevationAngle * ElevationHeightPerDegree;
	TargetLocation = TargetLocation.RotateAngleAxis(GameplayOrbitAngle, FVector::UpVector);
	if (bAimPresentation)
	{
		TargetLocation += AimFocusPoint * FMath::Clamp(AimFocusPanStrength, 0.0f, 0.5f);
	}
	return TargetLocation;
}

FRotator AFlickCameraPawn::GetGameplayTargetRotation() const
{
	const FRotator& BaseRotation = bBobGameplayFraming ? BobCameraRotation : CameraRotation;
	return FRotator(
		BaseRotation.Pitch - GameplayElevationAngle + (bAimPresentation ? AimPitchOffset : 0.0f),
		FRotator::NormalizeAxis(BaseRotation.Yaw + GameplayOrbitAngle),
		BaseRotation.Roll);
}

void AFlickCameraPawn::AddCameraImpulse(const float Strength)
{
	ShakeTrauma = FMath::Clamp(ShakeTrauma + FMath::Max(0.0f, Strength), 0.0f, 1.0f);
}
