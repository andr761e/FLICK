#include "Player/FlickCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

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

	ShakeTime += DeltaSeconds;
	const float MenuDrift = bMenuPresentation ? FMath::Sin(ShakeTime * 0.2f) * 30.0f : 0.0f;
	const float MenuDepthDrift = bMenuPresentation ? FMath::Sin(ShakeTime * 0.15f + 1.1f) * 13.0f : 0.0f;
	const float MenuLift = bMenuPresentation ? FMath::Sin(ShakeTime * 0.18f + 2.0f) * 9.0f : 0.0f;
	const FVector ScaledMenuLocation = MenuCameraLocation * ArenaFramingScale;
	const FVector TargetLocation = (bMenuPresentation ? ScaledMenuLocation : GetGameplayTargetLocation())
		+ FVector(MenuDrift, MenuDepthDrift, MenuLift);
	const FRotator TargetRotation = (bMenuPresentation ? MenuCameraRotation : GetGameplayTargetRotation())
		+ (bMenuPresentation
			? FRotator(FMath::Sin(ShakeTime * 0.17f) * 0.18f, FMath::Sin(ShakeTime * 0.2f + 0.8f) * 0.48f, 0.0f)
			: FRotator::ZeroRotator);
	const float TargetFieldOfView = bMenuPresentation
		? MenuFieldOfView
		: bBobGameplayFraming ? BobFieldOfView : bCompactGameplayFraming ? CompactFieldOfView : FieldOfView;
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, PresentationBlendSpeed);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, PresentationBlendSpeed);
	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
	bGameplayViewTransitioning = !bMenuPresentation
		&& (!NewLocation.Equals(TargetLocation, 1.0f) || !NewRotation.Equals(TargetRotation, 0.08f));
	CurrentFieldOfView = FMath::FInterpTo(CurrentFieldOfView, TargetFieldOfView, DeltaSeconds, PresentationBlendSpeed);
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

void AFlickCameraPawn::ApplyArenaPostProcess()
{
	if (!Camera)
	{
		return;
	}

	Camera->PostProcessSettings.BloomIntensity = bBobGameplayFraming
		? BobBloomIntensity
		: ClassicBloomIntensity;
	Camera->PostProcessSettings.BloomThreshold = bBobGameplayFraming
		? BobBloomThreshold
		: ClassicBloomThreshold;
	Camera->PostProcessSettings.AutoExposureBias = bBobGameplayFraming
		? BobExposureBias
		: ClassicExposureBias;
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

void AFlickCameraPawn::AdjustGameplayElevation(const int32 StepDirection)
{
	if (StepDirection == 0 || bMenuPresentation || bGameplayElevationLocked)
	{
		return;
	}
	SetGameplayElevation(
		GameplayElevationAngle + FMath::Sign(StepDirection) * FMath::Max(GameplayElevationStep, 0.1f));
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
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = !bSnap;
	if (bSnap)
	{
		SetActorLocation(GetGameplayTargetLocation());
		SetActorRotation(GetGameplayTargetRotation());
		bGameplayViewTransitioning = false;
	}
}

FVector AFlickCameraPawn::GetGameplayTargetLocation() const
{
	FVector TargetLocation = (bBobGameplayFraming ? BobCameraLocation : CameraLocation)
		* ArenaFramingScale
		* FMath::Clamp(GameplayDistanceScale, 0.75f, 1.25f);
	TargetLocation.Z += GameplayElevationAngle * ElevationHeightPerDegree;
	return TargetLocation.RotateAngleAxis(GameplayOrbitAngle, FVector::UpVector);
}

FRotator AFlickCameraPawn::GetGameplayTargetRotation() const
{
	const FRotator& BaseRotation = bBobGameplayFraming ? BobCameraRotation : CameraRotation;
	return FRotator(
		BaseRotation.Pitch - GameplayElevationAngle,
		FRotator::NormalizeAxis(BaseRotation.Yaw + GameplayOrbitAngle),
		BaseRotation.Roll);
}

void AFlickCameraPawn::AddCameraImpulse(const float Strength)
{
	ShakeTrauma = FMath::Clamp(ShakeTrauma + FMath::Max(0.0f, Strength), 0.0f, 1.0f);
}
