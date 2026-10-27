#include "Player/FlickCameraPawn.h"
#include "Arena/FlickArenaLighting.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickPuckFinish.h"
#include "UI/FlickLockerStyle.h"
#include "Engine/RectLight.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Core/FlickTypes.h"
#include "Core/FlickPresentationFrame.h"
#include "Engine/Engine.h"
#include "Game/FlickGameState.h"
#include "Game/FlickGameInstance.h"
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
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
		{
			const float ViewportAspect = ViewportSize.X / ViewportSize.Y;
			const float FramedAspect = FMath::Clamp(ViewportAspect,
				FlickPresentationFrame::MinimumAspect, FlickPresentationFrame::MaximumAspect);
			Camera->SetConstraintAspectRatio(!FMath::IsNearlyEqual(ViewportAspect, FramedAspect));
			Camera->SetAspectRatio(FramedAspect);
		}
	}
	if (bFreeCameraEnabled)
	{
		UpdatePuckStudioLighting();
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->SetFieldOfView(CurrentFieldOfView);
		bGameplayViewTransitioning = false;
		return;
	}

	ShakeTime += DeltaSeconds;
	if (bSpectatorView && !bMenuPresentation && !bCinematicReplay && !bPostMatchPresentation)
	{
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), SpectatorView.Location, DeltaSeconds, SpectatorViewBlendSpeed));
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), SpectatorView.Rotation, DeltaSeconds, SpectatorViewBlendSpeed));
		CurrentFieldOfView = FMath::FInterpTo(CurrentFieldOfView, SpectatorView.FieldOfView, DeltaSeconds, SpectatorViewBlendSpeed);
		UpdatePuckStudioLighting();
		Camera->SetRelativeLocation(FVector::ZeroVector);
		Camera->SetRelativeRotation(FRotator::ZeroRotator);
		Camera->SetFieldOfView(CurrentFieldOfView);
		bGameplayViewTransitioning = false;
		return;
	}
	FVector MenuTargetLocation = MenuCameraLocation * ArenaFramingScale;
	FRotator MenuTargetRotation = MenuCameraRotation;
	if (bMenuPresentation && bMenuOrbitEnabled)
	{
		const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
		// Frame the real display puck, not the historical board datum below it.
		const FVector MenuFocus(0.0f, 0.0f, bOneVsOneArenaPresentation
			? (State ? State->ArenaSurfaceZ : 250.0f) + OneVsOneMenuFocusHeight : MenuOrbitFocusHeight);
		const FVector OpticalFocus = bOneVsOneArenaPresentation
			? FVector(0.0f, 0.0f, (State ? State->ArenaSurfaceZ : 250.0f) + 20.0f) : MenuFocus;
		Camera->PostProcessSettings.DepthOfFieldFocalDistance = bOneVsOneArenaPresentation
			? FMath::Max(1.0f, FVector::DotProduct(OpticalFocus - GetActorLocation(), GetActorForwardVector()))
			: FVector::Distance(GetActorLocation(), MenuFocus);
		if (!bMenuOrbitInitialized)
		{
			MenuOrbitAngle = FMath::Atan2(MenuCameraLocation.Y, MenuCameraLocation.X);
			bMenuOrbitInitialized = true;
		}
		MenuOrbitAngle = FMath::Fmod(MenuOrbitAngle + FMath::DegreesToRadians(MenuOrbitDegreesPerSecond) * DeltaSeconds, 2.0f * PI);
		const float Radius = FVector2D(MenuCameraLocation.X, MenuCameraLocation.Y).Size() * ArenaFramingScale * MenuOrbitRadiusScale
			* (bOneVsOneArenaPresentation ? OneVsOneMenuDistanceMultiplier : 1.0f);
		MenuTargetLocation = FVector(Radius * FMath::Cos(MenuOrbitAngle), Radius * FMath::Sin(MenuOrbitAngle),
			MenuCameraLocation.Z * ArenaFramingScale * MenuOrbitHeightScale
			* (bOneVsOneArenaPresentation ? OneVsOneMenuHeightMultiplier : 1.0f));
		MenuTargetLocation = MenuFocus + (MenuTargetLocation - MenuFocus) * MenuOrbitZoomScale;
		MenuTargetRotation = UKismetMathLibrary::FindLookAtRotation(MenuTargetLocation, MenuFocus);
	}
	FVector TargetLocation = bMenuPresentation ? MenuTargetLocation : GetGameplayTargetLocation();
	FRotator TargetRotation = bMenuPresentation ? MenuTargetRotation : GetGameplayTargetRotation();
	if (bMenuPresentation && LockerPreviewMode > 0)
	{
		const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
		const float SurfaceZ = State ? State->ArenaSurfaceZ : 250.0f;
		const float EdgePreviewFocus = FlickModeRules::GetArenaRadius(
			EFlickMatchVariant::Classic, State ? State->PlayersPerTeam : 1) - 130.0f;
		const FVector Focus = LockerPreviewMode == 2
			? FVector(EdgePreviewFocus, 0.0f, SurfaceZ) : FVector(0.0f, 0.0f, SurfaceZ);
		TargetLocation = Focus + FVector(-300.0f, -680.0f, 480.0f) * ArenaFramingScale
			* (LockerPreviewMode == 2 ? 1.35f : LockerPreviewMode == 3 ? 1.15f : LockerPreviewMode == 4 ? 1.1f : 1.0f);
		TargetRotation = UKismetMathLibrary::FindLookAtRotation(TargetLocation, Focus
			+ (LockerPreviewMode == 2 ? FVector(500.0f, 0.0f, -110.0f)
				: LockerPreviewMode == 4 ? FVector(240.0f, 0.0f, -20.0f)
				: LockerPreviewMode == 3 ? FVector(300.0f, 0.0f, -110.0f) : FVector(200.0f, 0.0f, -110.0f)));
		if (LockerPreviewMode == 1)
		{
			// A close product view of the actual puck. Other previews retain
			// their wider framing for travel, arrival and knockout effects.
			FlickLockerStyle::PuckView(SurfaceZ, ArenaFramingScale, Camera->AspectRatio, TargetLocation, TargetRotation);
		}
	}
	float TargetFieldOfView = bMenuPresentation
		? MenuFieldOfView
		: bAimPresentation && bTestArenaPresentation
			? AimFieldOfView
			: bBobGameplayFraming ? BobFieldOfView : bCompactGameplayFraming ? CompactFieldOfView : FieldOfView;
	if (bMenuPresentation && bMenuOrbitEnabled && bOneVsOneArenaPresentation)
	{
		// Keep the reference's vertical board composition on ultrawide screens,
		// revealing extra stadium at the sides rather than cropping both rims.
		const float AspectExpansion = FMath::Max(1.0f, Camera->AspectRatio / (16.0f / 9.0f));
		TargetFieldOfView = FMath::RadiansToDegrees(2.0f * FMath::Atan(
			FMath::Tan(FMath::DegreesToRadians(MenuFieldOfView * 0.5f)) * AspectExpansion));
	}
	if (bMenuPresentation && LockerPreviewMode > 0)
		TargetFieldOfView = LockerPreviewMode == 2 ? 46.0f : LockerPreviewMode == 3 ? 44.0f : LockerPreviewMode == 4 ? 42.0f : 38.0f;
	if (bMenuPresentation && LockerPreviewMode == 1)
		TargetFieldOfView = FlickLockerStyle::PuckFieldOfView(Camera->AspectRatio);
	if (!bMenuPresentation && !bCinematicReplay)
	{
		if (const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>())
		{
			TargetFieldOfView += Instance->GetGameplayCameraFieldOfView() - FieldOfView;
		}
	}
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
	if (bPostMatchPresentation && !bCinematicReplay)
	{
		const float Alpha = FMath::SmoothStep(0.0f, 1.0f, PostMatchPullback);
		const FVector Focus(0.0f, 0.0f, PostMatchSurfaceZ + 22.0f);
		const float ShowcaseDistance = 480.0f + PostMatchWinnerCount * 170.0f;
		TargetLocation = Focus + FVector(0.0f, -FMath::Lerp(ShowcaseDistance, 1850.0f * ArenaFramingScale, Alpha),
			FMath::Lerp(ShowcaseDistance * 0.52f, 1250.0f * ArenaFramingScale, Alpha));
		TargetRotation = UKismetMathLibrary::FindLookAtRotation(TargetLocation, Focus);
		TargetFieldOfView = FMath::Lerp(38.0f, 49.0f, Alpha);
		BlendSpeed = 4.5f;
		ShakeTrauma = 0.0f;
	}
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaSeconds, BlendSpeed);
	const FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaSeconds, BlendSpeed);
	SetActorLocation(NewLocation);
	SetActorRotation(NewRotation);
	if (bMenuPresentation && LockerPreviewMode == 1)
	{
		const auto* State = GetWorld()->GetGameState<AFlickGameState>();
		const FVector PuckFocus(0, 0, (State ? State->ArenaSurfaceZ : 250.f) + 20.f);
		Camera->PostProcessSettings.DepthOfFieldFocalDistance = FMath::Max(1.f, FVector::DotProduct(PuckFocus - NewLocation, GetActorForwardVector()));
	}
	if (bTestArenaPresentation || (bMenuPresentation && bMenuOrbitEnabled && bOneVsOneArenaPresentation))
	{
		if (!MenuKeyLight.IsValid() || !MenuRimLight.IsValid() || !MenuDeckReflectionLight.IsValid())
		{
			const FlickArenaLighting::FRig Rig = FlickArenaLighting::FindRig(GetWorld());
			MenuKeyLight = Rig.TestPuckKeyLight;
			MenuRimLight = Rig.TestPuckRimLight;
			MenuDeckReflectionLight = Rig.DeckReflectionLight;
		}
		const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
		const float Radius = FlickModeRules::GetArenaRadius(EFlickMatchVariant::Classic, State ? State->PlayersPerTeam : 1)
			* (State && State->bPrivateMatchActive ? FMath::Clamp(State->PrivateMatchSettings.ArenaScale, 0.85f, 1.3f) : 1.0f);
		FlickArenaLighting::UpdateMenuSoftboxes(MenuKeyLight.Get(), MenuRimLight.Get(),
			GetActorLocation(), Radius, State ? State->ArenaSurfaceZ : 250.0f, MenuDeckReflectionLight.Get());
	}
	UpdatePuckStudioLighting();
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

void AFlickCameraPawn::UpdatePuckStudioLighting()
{
	if (!(bTestArenaPresentation || bBobGameplayFraming || (bMenuPresentation && bOneVsOneArenaPresentation))) return;
	if (!PuckStudioKeyLight.IsValid() || !PuckStudioFillLight.IsValid())
	{
		const auto Rig = FlickArenaLighting::FindRig(GetWorld());
		PuckStudioKeyLight = Rig.PuckFinishLight;
		PuckStudioFillLight = Rig.PuckFillLight;
	}
	const AFlickGameState* State = GetWorld()->GetGameState<AFlickGameState>();
	const auto Variant = bBobGameplayFraming ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
	float Radius = FlickModeRules::GetArenaRadius(Variant, State ? State->PlayersPerTeam : 1);
	if (State && State->bPrivateMatchActive && Variant == EFlickMatchVariant::Classic)
		Radius *= FMath::Clamp(State->PrivateMatchSettings.ArenaScale, .85f, 1.3f);
	FQuat ProductFrame;
	const bool bUseProductFrame = !bMenuPresentation || (bHomePuckPresentation && LockerPreviewMode == 0);
	if (bUseProductFrame)
	{
		// Retain the locker's authored frame for menus. Gameplay instead aligns
		// the key with the reflection of the actual viewing ray across the deck.
		FVector ReferenceLocation;
		FRotator ReferenceRotation;
		FlickLockerStyle::PuckView(250.f, 1.f, Camera->AspectRatio, ReferenceLocation, ReferenceRotation);
		// The puck sits off the locker's optical axis. Its actual viewing ray,
		// rather than the UI-shifted camera's forward vector, defines the finish.
		ReferenceRotation = (FVector(0.f, 0.f, 250.f + FlickLockerStyle::PuckFocusHeight) - ReferenceLocation).Rotation();
		const float ReferenceAzimuth = FMath::RadiansToDegrees(FMath::Atan2(ReferenceLocation.Y, ReferenceLocation.X));
		// A slight side-key offset gives the silver band some shape at game size.
		const float HighlightYaw = bMenuPresentation ? 0.f : 12.f;
		FRotator View = GetActorRotation();
		if (!bMenuPresentation && !bFreeCameraEnabled)
		{
			// The camera's tilt and height are independently framed. Use the ray
			// toward the puck plane, not the optical axis, for the silver reflection.
			const FVector Focus = bCinematicReplay ? ReplayFocus + FVector(0, 0, 20.f)
				: FVector(0, 0, (State ? State->ArenaSurfaceZ : 250.f) + 20.f);
			View = (Focus - GetActorLocation()).Rotation();
		}
		ProductFrame = FlickArenaLighting::MakePuckStudioFrame(View, ReferenceRotation,
			ReferenceAzimuth, HighlightYaw, !bMenuPresentation);
	}
	FlickArenaLighting::UpdatePuckStudioLights(PuckStudioKeyLight.Get(), PuckStudioFillLight.Get(),
		GetActorLocation(), Radius / (bBobGameplayFraming ? 620.f : 650.f), State ? State->ArenaSurfaceZ : 250.f,
		bUseProductFrame ? &ProductFrame : nullptr, !bMenuPresentation);
}

void AFlickCameraPawn::ApplyCameraSettings()
{
	ShakeTrauma = 0.0f;
	SetActorLocation(bMenuPresentation ? MenuCameraLocation * ArenaFramingScale : GetGameplayTargetLocation());
	SetActorRotation(bMenuPresentation ? MenuCameraRotation : GetGameplayTargetRotation());
	CurrentFieldOfView = bMenuPresentation
		? MenuFieldOfView
		: bBobGameplayFraming ? BobFieldOfView : bCompactGameplayFraming ? CompactFieldOfView : FieldOfView;
	if (!bMenuPresentation)
	{
		if (const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>())
		{
			CurrentFieldOfView += Instance->GetGameplayCameraFieldOfView() - FieldOfView;
		}
	}
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
	ApplyArenaPostProcess();
}

void AFlickCameraPawn::SetMenuOrbitEnabled(const bool bEnabled)
{
	bMenuOrbitEnabled = bEnabled;
	ApplyArenaPostProcess();
}

void AFlickCameraPawn::SetHomePuckPresentation(const bool bEnabled)
{
	if (bHomePuckPresentation == bEnabled) return;
	bHomePuckPresentation = bEnabled;
	ApplyArenaPostProcess();
}

void AFlickCameraPawn::SetPostMatchPresentation(bool bEnabled, float Pullback, int32 WinnerCount, float SurfaceZ)
{
	bPostMatchPresentation = bEnabled;
	PostMatchPullback = FMath::Clamp(Pullback, 0.0f, 1.0f);
	PostMatchWinnerCount = FMath::Clamp(WinnerCount, 1, 3);
	PostMatchSurfaceZ = SurfaceZ;
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

void AFlickCameraPawn::SetOneVsOneArenaPresentation(const bool bEnabled)
{
	bOneVsOneArenaPresentation = bEnabled;
	ApplyArenaPostProcess();
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
	const bool bPremiumMenu = bMenuPresentation && bMenuOrbitEnabled;
	const bool bShowcaseGrade = bPremiumMenu || bTestArenaPresentation;
	const bool bOneVsOneMenu = bTestArenaPresentation || (bPremiumMenu && bOneVsOneArenaPresentation);
	const bool bBobGrade = bBobGameplayFraming && !bTestArenaPresentation;
	Camera->PostProcessSettings.ColorGamma = bOneVsOneMenu || bBobGrade ? FVector4(1.0f, 1.0f, 1.0f, 1.0f) : FVector4(0.96f, 0.975f, 1.0f, 1.0f);
	if (bOneVsOneMenu) Camera->PostProcessSettings.AutoExposureBias = 0.0f;
	// Architectural metal carries its own restrained roughness detail. Avoid
	// artificial grain/sharpened contrast competing with the real reflections.
	Camera->PostProcessSettings.FilmGrainIntensity = bOneVsOneMenu || bBobGrade ? 0.f : .07f;
	Camera->PostProcessSettings.AmbientOcclusionIntensity = bOneVsOneMenu || bBobGrade ? 1.05f : 1.28f;
	Camera->PostProcessSettings.bOverride_ScreenSpaceReflectionQuality = bOneVsOneMenu;
	Camera->PostProcessSettings.ScreenSpaceReflectionQuality = 75.f;
	Camera->PostProcessSettings.bOverride_LumenReflectionQuality = bOneVsOneMenu;
	// Use Epic's normal reconstruction budget rather than multiplying its five
	// samples to eight. Temporal filtering and the stable scene-trace path below
	// remain enabled; reflection tracing layout belongs to scalability settings.
	Camera->PostProcessSettings.LumenReflectionQuality = 1.0f;
	// Tiny rim LEDs fluctuate when screen-space hits alternate with Lumen's
	// scene cache. Use scene traces consistently in Switchyard; retain Lumen
	// reflections and the player's hardware/software tracing preference.
	// Clear the overrides on exit so BOB/other scenes keep their normal path.
	Camera->PostProcessSettings.bOverride_LumenFinalGatherScreenTraces = bOneVsOneMenu;
	Camera->PostProcessSettings.LumenFinalGatherScreenTraces = false;
	Camera->PostProcessSettings.bOverride_LumenReflectionsScreenTraces = bOneVsOneMenu;
	Camera->PostProcessSettings.LumenReflectionsScreenTraces = false;
	Camera->PostProcessSettings.ColorSaturation = bShowcaseGrade
		? FVector4(MenuColorSaturation, MenuColorSaturation, MenuColorSaturation, 1.0f) : FVector4(1.02f, 1.02f, 1.02f, 1.0f);
	Camera->PostProcessSettings.ColorContrast = bShowcaseGrade
		? (bOneVsOneMenu ? FVector4(1.12f, 1.12f, 1.12f, 1.0f) : FVector4(1.24f, 1.24f, 1.24f, 1.0f))
		: bBobGrade ? FVector4(1.10f, 1.10f, 1.10f, 1.0f) : FVector4(1.16f, 1.16f, 1.16f, 1.0f);
	Camera->PostProcessSettings.VignetteIntensity = bBobGrade ? .25f : bShowcaseGrade ? 0.36f : 0.43f;
	Camera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = bPremiumMenu;
	Camera->PostProcessSettings.bOverride_DepthOfFieldFstop = bPremiumMenu;
	Camera->PostProcessSettings.bOverride_DepthOfFieldSensorWidth = bPremiumMenu && bOneVsOneMenu;
	Camera->PostProcessSettings.DepthOfFieldSensorWidth = OneVsOneMenuSensorWidth;
	Camera->PostProcessSettings.DepthOfFieldFstop = bMenuPresentation && LockerPreviewMode == 1 ? 2.4f
		: bOneVsOneArenaPresentation ? OneVsOneMenuFstop : 3.2f;
	if (bShowcaseGrade)
	{
		Camera->PostProcessSettings.BloomIntensity = bOneVsOneMenu ? OneVsOneMenuBloomIntensity : MenuBloomIntensity;
		Camera->PostProcessSettings.BloomThreshold = bOneVsOneMenu ? 1.15f : 0.8f;
	}
	if (bMenuPresentation && LockerPreviewMode > 0)
	{
		Camera->PostProcessSettings.AutoExposureBias += FlickPuckFinish::ReferenceExposureBias;
		Camera->PostProcessSettings.BloomIntensity = FlickPuckFinish::ReferenceBloomIntensity;
		Camera->PostProcessSettings.BloomThreshold = FlickPuckFinish::ReferenceBloomThreshold;
	}
	if (bTestArenaPresentation && (!bMenuPresentation || (bHomePuckPresentation && LockerPreviewMode == 0)))
	{
		// Home, gameplay and replays share the approved locker exposure/bloom.
		// Locker's existing compensation above is applied exactly once.
		Camera->PostProcessSettings.AutoExposureBias = FlickPuckFinish::ReferenceExposureBias;
		Camera->PostProcessSettings.BloomIntensity = FlickPuckFinish::ReferenceBloomIntensity;
		Camera->PostProcessSettings.BloomThreshold = FlickPuckFinish::ReferenceBloomThreshold;
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
	if (bTopDownView) return;
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

void AFlickCameraPawn::RotateGameplayOrbit(const float Direction, const float DeltaSeconds, const float Sensitivity)
{
	if (FMath::IsNearlyZero(Direction) || DeltaSeconds <= 0.0f || bMenuPresentation)
	{
		return;
	}

	float& OrbitAngle = bTopDownView ? TopDownOrbitAngle : GameplayOrbitAngle;
	OrbitAngle = FRotator::NormalizeAxis(
		OrbitAngle
		+ FMath::Clamp(Direction, -1.0f, 1.0f) * GameplayOrbitDegreesPerSecond
			* FMath::Lerp(0.35f, 1.5f, FMath::Clamp(Sensitivity, 0.0f, 1.0f)) * DeltaSeconds);
	bGameplayOrbitManuallyControlled = true;
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = true;
}

void AFlickCameraPawn::MoveGameplayElevation(const float Direction, const float DeltaSeconds, const float Sensitivity)
{
	if (FMath::IsNearlyZero(Direction) || DeltaSeconds <= 0.f || bMenuPresentation || bGameplayElevationLocked || bTopDownView || bFreeCameraEnabled || bCinematicReplay) return;
	SetGameplayElevation(GameplayElevationAngle + FMath::Clamp(Direction, -1.f, 1.f) * GameplayElevationDegreesPerSecond
		* FMath::Lerp(.35f, 1.5f, FMath::Clamp(Sensitivity, 0.f, 1.f)) * DeltaSeconds);
	GameplayElevationPresetIndex = 1;
}

void AFlickCameraPawn::AdjustGameplayZoom(const int32 StepDirection)
{
	if (StepDirection == 0 || bMenuPresentation || bFreeCameraEnabled || bCinematicReplay) return;
	GameplayZoomScale = FMath::Clamp(GameplayZoomScale - FMath::Sign(StepDirection) * GameplayZoomStep,
		GameplayMinimumZoomScale, GameplayMaximumZoomScale);
	ShakeTrauma = 0.f;
	bGameplayViewTransitioning = true;
}

void AFlickCameraPawn::AdjustGameplayElevationFine(const int32 StepDirection, const float Sensitivity)
{
	if (StepDirection == 0 || bMenuPresentation || bGameplayElevationLocked || bTopDownView)
	{
		return;
	}
	const float StepDegrees = FMath::Lerp(0.75f, 5.0f, FMath::Clamp(Sensitivity, 0.0f, 1.0f));
	SetGameplayElevation(GameplayElevationAngle + FMath::Sign(StepDirection) * StepDegrees);
	GameplayElevationPresetIndex = 1;
}

void AFlickCameraPawn::ToggleTopDownView()
{
	if (bMenuPresentation || bCinematicReplay || bFreeCameraEnabled) return;
	if (!bTopDownView)
	{
		SavedGameplayOrbitAngle = GameplayOrbitAngle;
		SavedGameplayElevationAngle = GameplayElevationAngle;
		TopDownOrbitAngle = GameplayOrbitAngle;
		bTopDownView = true;
	}
	else
	{
		bTopDownView = false;
		GameplayOrbitAngle = SavedGameplayOrbitAngle;
		GameplayElevationAngle = SavedGameplayElevationAngle;
	}
	ShakeTrauma = 0.0f;
	bGameplayViewTransitioning = true;
}

void AFlickCameraPawn::ResetGameplayView(const int32 InViewIndex, const bool bResetElevation)
{
	GameplayZoomScale = 1.f;
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

void AFlickCameraPawn::ResetRoundView(const int32 InViewIndex)
{
	if (bFreeCameraEnabled) SetFreeCameraEnabled(false);
	bTopDownView = false;
	bGameplayOrbitManuallyControlled = false;
	ResetGameplayView(InViewIndex, true);
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
	if (bEnabled) ClearSpectatorView();
	bAimPresentation = false;
	ShakeTrauma = 0.0f;
	if (!bFreeCameraEnabled)
	{
		bGameplayViewTransitioning = true;
	}
}

FFlickSpectatorView AFlickCameraPawn::CaptureSpectatorView() const
{
	FFlickSpectatorView View;
	if (Camera)
	{
		View.Location = Camera->GetComponentLocation();
		View.Rotation = Camera->GetComponentRotation();
		View.FieldOfView = Camera->FieldOfView;
		View.bValid = true;
	}
	return View;
}

void AFlickCameraPawn::SetSpectatorView(const FFlickSpectatorView& View)
{
	if (!View.bValid || !View.IsSafe()) return;
	SpectatorView = View;
	bSpectatorView = true;
	bAimPresentation = false;
	ShakeTrauma = 0.0f;
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
		70.0f);
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

void AFlickCameraPawn::SetFreeCameraSensitivity(const float LookSensitivity, const float MoveSensitivity)
{
	FreeCameraMouseSensitivity = FMath::Lerp(0.08f, 0.75f, FMath::Clamp(LookSensitivity, 0.0f, 1.0f));
	FreeCameraMoveSpeed = FMath::Lerp(300.0f, 1400.0f, FMath::Clamp(MoveSensitivity, 0.0f, 1.0f));
}

FVector AFlickCameraPawn::GetGameplayTargetLocation() const
{
	const AFlickGameState* MatchState = GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
	const FVector ArenaFocus(0.f, 0.f, MatchState ? MatchState->ArenaSurfaceZ : 250.f);
	if (bTopDownView)
	{
		const float Height = (bBobGameplayFraming ? 2450.0f : 2200.0f) * ArenaFramingScale;
		return ArenaFocus + (FVector(0.0f, 0.0f, Height) - ArenaFocus) * GameplayZoomScale;
	}
	const float TeamSizeDistance = MatchState
		&& MatchState->ActiveMatchVariant == EFlickMatchVariant::Classic
		&& MatchState->PlayersPerTeam > 1
		? FMath::Clamp(MultiplayerGameplayDistanceMultiplier, 0.85f, 1.0f)
		: 1.0f;
	const UFlickGameInstance* Instance = GetGameInstance<UFlickGameInstance>();
	const float DistanceScale = Instance ? Instance->GetGameplayCameraDistance() : GameplayDistanceScale;
	FVector TargetLocation = (bBobGameplayFraming ? BobCameraLocation : CameraLocation)
		* ArenaFramingScale
		* FMath::Clamp(DistanceScale, 0.75f, 1.25f)
		* TeamSizeDistance
		* (bTestArenaPresentation ? FMath::Clamp(TestArenaDistanceMultiplier, 0.75f, 1.0f) : 1.0f)
		* (bAimPresentation ? FMath::Clamp(AimDistanceMultiplier, 0.82f, 1.0f) : 1.0f);
	TargetLocation.Z += GameplayElevationAngle * ElevationHeightPerDegree;
	TargetLocation = TargetLocation.RotateAngleAxis(GameplayOrbitAngle, FVector::UpVector);
	if (bAimPresentation)
	{
		TargetLocation += AimFocusPoint * FMath::Clamp(AimFocusPanStrength, 0.0f, 0.5f);
	}
	return ArenaFocus + (TargetLocation - ArenaFocus) * GameplayZoomScale;
}

FRotator AFlickCameraPawn::GetGameplayTargetRotation() const
{
	if (bTopDownView) return FRotator(-90.0f, TopDownOrbitAngle + 90.0f, 0.0f);
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
