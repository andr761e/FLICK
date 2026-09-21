#include "Game/FlickGameModePrivate.h"

using namespace FlickGameModePrivate;

void AFlickGameMode::StartNextRound()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (FlickGameState)
	{
		FlickGameState->SetRoundAdvanceTimerState(false, RoundAdvanceTimeLimit);
	}
	if (FrontendScreen != EFlickFrontendScreen::Playing
		|| !FlickGameState
		|| !FlickGameState->AdvanceToNextRound())
	{
		return;
	}

	ApplyPendingPlayerClasses();
	ClearControllerAiming();
	if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (AFlickHUD* FlickHUD = Cast<AFlickHUD>(FlickController->GetHUD()))
		{
			FlickHUD->ResetPresentation();
		}
	}

	DestroyPieces();
	SpawnPieces();
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	LastImpactFeedbackTime = -100.0f;
	LastStrongImpactEventTime = -100.0f;
	FlickGameState->SetStartingPiecesPerTeam(CurrentStartingPiecesPerTeam);
	UpdateGameStateCounts();
	BeginOpeningPhase();
	PushHudEvent(
		FString::Printf(TEXT("ROUND %d KICKOFF"), FlickGameState->RoundNumber),
		GetTeamColor(FlickGameState->CurrentTeam),
		1.8f);
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(FlickGameState->CurrentTeam);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("Round %d started. %s opens."),
		FlickGameState->RoundNumber,
		*GetTeamDisplayName(FlickGameState->CurrentTeam));
}

void AFlickGameMode::ReturnToLoadout()
{
	if (!DoesSelectedModeSupportLoadouts())
	{
		ReturnToMainMenu();
		return;
	}
	ReturnToMainMenu();
	OpenLoadout();
}

void AFlickGameMode::TogglePauseMenu()
{
	if (FrontendScreen == EFlickFrontendScreen::Playing)
	{
		ClearControllerAiming();
		FrontendScreen = EFlickFrontendScreen::Paused;
		UGameplayStatics::SetGamePaused(this, true);
	}
	else if (FrontendScreen == EFlickFrontendScreen::Paused)
	{
		UGameplayStatics::SetGamePaused(this, false);
		FrontendScreen = EFlickFrontendScreen::Playing;
	}
}

void AFlickGameMode::OpenSettings()
{
	if (FrontendScreen == EFlickFrontendScreen::Playing)
	{
		TogglePauseMenu();
	}

	SettingsReturnScreen = FrontendScreen;
	FrontendScreen = EFlickFrontendScreen::Settings;
	SetCameraForFrontend();
}

void AFlickGameMode::CloseSettings()
{
	if (FrontendScreen == EFlickFrontendScreen::Settings)
	{
		FrontendScreen = SettingsReturnScreen;
		SetCameraForFrontend();
	}
}

void AFlickGameMode::ReturnToMainMenu()
{
	if (bNetworkMatchRequested)
	{
		AFlickGameState* State = GetFlickGameState();
		if (bRankedRequested && bNetworkMatchStarted && State && !State->bMatchResultFinalized)
		{
			const APlayerController* LocalController = UGameplayStatics::GetPlayerController(this, 0);
			const AFlickPlayerState* LocalPlayerState = LocalController
				? LocalController->GetPlayerState<AFlickPlayerState>()
				: nullptr;
			if (LocalPlayerState && LocalPlayerState->GetTeam() != EFlickTeam::None)
			{
				State->CompleteMatchByForfeit(LocalPlayerState->GetTeam());
				DispatchRankedMatchResults();
			}
		}
		ReturnToNetworkLobby();
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	if (bTrainingEditMode && CameraPawn)
	{
		CameraPawn->SetGameplayElevationLocked(false);
		CameraPawn->SetGameplayElevation(TrainingPreviousCameraElevation, true);
	}
	bTrainingMode = false;
	bTrainingEditMode = false;
	bTrainingBotMatch = false;
	bTutorialMode = false;
	bTutorialCompleted = false;
	bTutorialAdvancePending = false;
	TutorialTransitionRemaining = 0.0f;
	bClassSelectionStartsTrainingBotMatch = false;
	bTestArenaMode = false;
	ResetTrainingBotThinking();
	bPlayerClassesActiveForMatch = false;
	bClassSelectionForNextRound = false;
	bHasPendingClassChanges = false;
	Player1PendingClasses.Reset();
	Player2PendingClasses.Reset();
	ClearControllerAiming();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	FrontendScreen = EFlickFrontendScreen::MainMenu;
	SetCameraForFrontend();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
}

void AFlickGameMode::QuitGame()
{
	UKismetSystemLibrary::QuitGame(
		this,
		UGameplayStatics::GetPlayerController(this, 0),
		EQuitPreference::Quit,
		false);
}

void AFlickGameMode::SetAimGuideEnabled(const bool bEnabled)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetAimGuideEnabled(bEnabled);
	}
}

void AFlickGameMode::SetImpactEffectsEnabled(const bool bEnabled)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetImpactEffectsEnabled(bEnabled);
	}
}

void AFlickGameMode::SetControlOverviewEnabled(const bool bEnabled)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetControlOverviewEnabled(bEnabled);
	}
}

void AFlickGameMode::CycleBotDifficulty(const int32 Direction)
{
	if (Direction == 0)
	{
		return;
	}
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		constexpr int32 DifficultyCount = static_cast<int32>(EFlickBotDifficulty::Expert) + 1;
		const int32 Step = Direction < 0 ? -1 : 1;
		const int32 Current = static_cast<int32>(FlickGameInstance->GetBotDifficulty());
		const int32 Next = (Current + Step + DifficultyCount) % DifficultyCount;
		FlickGameInstance->SetBotDifficulty(static_cast<EFlickBotDifficulty>(Next));
	}
}

void AFlickGameMode::SetCameraShakeIntensity(const float Intensity)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetCameraShakeIntensity(Intensity);
	}
}

void AFlickGameMode::SetMasterVolume(const float Volume)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetMasterVolume(Volume);
	}
}

void AFlickGameMode::SetEffectsVolume(const float Volume)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetEffectsVolume(Volume);
	}
}

void AFlickGameMode::SetInterfaceVolume(const float Volume)
{
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetInterfaceVolume(Volume);
	}
}

void AFlickGameMode::PlayMenuSound(const bool bConfirm) const
{
	if (AudioDirector)
	{
		AudioDirector->PlayUi(bConfirm);
	}
}

void AFlickGameMode::ToggleVSync()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled());
	}
}

void AFlickGameMode::CycleWindowMode(const int32 Direction)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	// FLICK is presented as a fullscreen title. Borderless fullscreen adapts to
	// ultrawide and unusual desktop resolutions without a disruptive mode switch.
	Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
}

void AFlickGameMode::CycleResolution(const int32 Direction)
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	const TArray<FIntPoint> Resolutions = {
		FIntPoint(1280, 720),
		FIntPoint(1600, 900),
		FIntPoint(1920, 1080),
		FIntPoint(2560, 1440)
	};
	int32 CurrentIndex = Resolutions.IndexOfByKey(Settings->GetScreenResolution());
	CurrentIndex = CurrentIndex == INDEX_NONE ? 0 : CurrentIndex;
	const int32 Step = Direction < 0 ? -1 : 1;
	Settings->SetScreenResolution(Resolutions[(CurrentIndex + Step + Resolutions.Num()) % Resolutions.Num()]);
}

void AFlickGameMode::ApplyDisplaySettings()
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
		Settings->SetScreenResolution(Settings->GetDesktopResolution());
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}

AFlickGameState* AFlickGameMode::GetFlickGameState() const
{
	return GetGameState<AFlickGameState>();
}

int32 AFlickGameMode::CountActivePieces(const EFlickTeam Team) const
{
	int32 Count = 0;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive()
			&& !Piece->IsBobStriker() && Piece->GetTeam() == Team)
		{
			++Count;
		}
	}
	return Count;
}

void AFlickGameMode::SpawnCameraIfNeeded()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !GetWorld())
	{
		return;
	}

	CameraPawn = Cast<AFlickCameraPawn>(PlayerController->GetPawn());
	if (!CameraPawn)
	{
		CameraPawn = GetWorld()->SpawnActor<AFlickCameraPawn>(AFlickCameraPawn::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		if (CameraPawn)
		{
			PlayerController->Possess(CameraPawn);
		}
	}

	if (CameraPawn)
	{
		CameraPawn->SetBobGameplayFraming(ActiveMatchVariant == EFlickMatchVariant::Bob);
		CameraPawn->SetTestArenaPresentation(bTestArenaMode);
		CameraPawn->SetCompactGameplayFraming(false);
		CameraPawn->SetArenaFramingScale(
			ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
		CameraPawn->ApplyCameraSettings();
	}
}

void AFlickGameMode::SpawnAudioIfNeeded()
{
	if (!GetWorld() || (AudioDirector && IsValid(AudioDirector)))
	{
		return;
	}

	AudioDirector = GetWorld()->SpawnActor<AFlickAudioDirector>(
		AFlickAudioDirector::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
}

void AFlickGameMode::SpawnLightingIfNeeded()
{
	if (!GetWorld())
	{
		return;
	}
	const bool bClassicArenaLighting = ActiveMatchVariant == EFlickMatchVariant::Classic;
	const bool bBobArenaLighting = ActiveMatchVariant == EFlickMatchVariant::Bob;
	const bool bSettingsOverMatch = FrontendScreen == EFlickFrontendScreen::Settings
		&& SettingsReturnScreen == EFlickFrontendScreen::Paused;
	const bool bClassSelectionOverMatch = FrontendScreen == EFlickFrontendScreen::ClassSelect
		&& ClassSelectionReturnScreen == EFlickFrontendScreen::Paused;
	const bool bFrontendShowcase = FrontendScreen != EFlickFrontendScreen::Playing
		&& FrontendScreen != EFlickFrontendScreen::Paused
		&& !bSettingsOverMatch
		&& !bClassSelectionOverMatch;
	const float DirectionalMultiplier = bFrontendShowcase
		? FrontendArenaDirectionalLightMultiplier : 1.0f;
	const float SkyMultiplier = bFrontendShowcase ? FrontendArenaSkyLightMultiplier : 1.0f;
	const float FillMultiplier = bFrontendShowcase ? FrontendArenaFillLightMultiplier : 1.0f;

	if (!DirectionalLightActor || !IsValid(DirectionalLightActor))
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			DirectionalLightActor = *It;
			break;
		}
		if (!DirectionalLightActor)
		{
			DirectionalLightActor = GetWorld()->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(),
				FVector(-300.0f, -500.0f, 900.0f),
				FRotator(-55.0f, -30.0f, 0.0f));
		}
	}

	if (!SkyLightActor || !IsValid(SkyLightActor))
	{
		for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
		{
			SkyLightActor = *It;
			break;
		}
		if (!SkyLightActor)
		{
			SkyLightActor = GetWorld()->SpawnActor<ASkyLight>(
				ASkyLight::StaticClass(),
				FVector(0.0f, 0.0f, 900.0f),
				FRotator::ZeroRotator);
		}
	}

	if (UDirectionalLightComponent* Light = DirectionalLightActor
		? Cast<UDirectionalLightComponent>(DirectionalLightActor->GetLightComponent())
		: nullptr)
	{
		Light->SetMobility(EComponentMobility::Movable);
		DirectionalLightActor->SetActorRotation(FRotator(-72.0f, -25.0f, 0.0f));
		Light->SetLightColor(bTestArenaMode
			? FLinearColor(0.92f, 0.95f, 1.0f)
			: bClassicArenaLighting
				? FLinearColor(0.82f, 0.88f, 0.96f)
				: FLinearColor(0.9f, 0.94f, 1.0f));
		Light->SetIntensity(
			(bTestArenaMode ? 1.45f : bClassicArenaLighting ? 0.78f : bBobArenaLighting ? 1.32f : 1.15f)
			* DirectionalMultiplier);
		Light->SetLightSourceAngle(bTestArenaMode ? 5.0f : 3.0f);
		Light->SetSpecularScale(bTestArenaMode ? 0.60f : bClassicArenaLighting ? 0.14f : 0.32f);
		Light->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.72f : 0.8f);
		Light->SetCastShadows(true);
	}
	if (SkyLightActor && SkyLightActor->GetLightComponent())
	{
		auto* Sky = SkyLightActor->GetLightComponent();
		Sky->SetMobility(EComponentMobility::Movable);
		// Metallic workshop surfaces need an environment to reflect even on an empty map.
		if (bTestArenaMode)
		{
			if (auto* Environment = LoadObject<UTextureCube>(nullptr,
				TEXT("/Game/TestArena/Pucks/T_PuckEnvironment.T_PuckEnvironment")))
			{
				Sky->SourceType = SLS_SpecifiedCubemap;
				Sky->SetCubemap(Environment);
			}
		}
		else if (Sky->SourceType == SLS_SpecifiedCubemap && Sky->Cubemap
			&& Sky->Cubemap->GetPathName().StartsWith(TEXT("/Game/TestArena/Pucks/")))
		{
			Sky->SourceType = SLS_CapturedScene;
			Sky->SetCubemap(nullptr);
		}
		Sky->SetIntensity(
			(bTestArenaMode ? 1.05f : bClassicArenaLighting ? 0.34f : bBobArenaLighting ? 0.55f : 0.28f)
			* SkyMultiplier);
	}

	const auto SpawnAccentLight = [this, bClassicArenaLighting, bBobArenaLighting, bFrontendShowcase](
		TObjectPtr<APointLight>& LightActor,
		const FVector& Location,
		const FLinearColor& Color)
	{
		if (!LightActor || !IsValid(LightActor))
		{
			LightActor = GetWorld()->SpawnActor<APointLight>(
				APointLight::StaticClass(), Location, FRotator::ZeroRotator);
		}
		if (LightActor && LightActor->PointLightComponent)
		{
			UPointLightComponent* Light = LightActor->PointLightComponent;
			Light->SetMobility(EComponentMobility::Movable);
			LightActor->SetActorLocation(Location);
			// Preserve the neutral gameplay wash, but let the main-menu orbit show
			// the arena's team colors and material highlights without a screen tint.
			Light->SetLightColor(FMath::Lerp(
				Color, FLinearColor::White,
				bTestArenaMode ? 0.38f : bClassicArenaLighting ? (bFrontendShowcase ? 0.52f : 0.88f) : 0.72f));
			Light->SetIntensity(bTestArenaMode ? 190.0f : bClassicArenaLighting ? (bFrontendShowcase ? 90.0f : 52.0f) : bBobArenaLighting ? 205.0f : 165.0f);
			Light->SetAttenuationRadius(
				(bTestArenaMode ? 700.0f : bClassicArenaLighting ? 720.0f : bBobArenaLighting ? 1500.0f : 820.0f)
					* ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
			Light->SetSourceRadius((bTestArenaMode ? 100.0f : bClassicArenaLighting ? 260.0f : 120.0f)
				* ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
			Light->SetSpecularScale(bTestArenaMode ? 0.52f : bClassicArenaLighting ? (bFrontendShowcase ? 0.24f : 0.04f) : 0.48f);
			Light->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.15f : 0.42f);
			Light->SetCastShadows(false);
		}
	};

	const float ArenaScale = ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius;
	SpawnAccentLight(Player1AccentLight, FVector(0.0f, -690.0f * ArenaScale, 620.0f * ArenaScale), GetTeamColor(EFlickTeam::Player1));
	SpawnAccentLight(Player2AccentLight, FVector(0.0f, 690.0f * ArenaScale, 620.0f * ArenaScale), GetTeamColor(EFlickTeam::Player2));
	if (!ArenaFillLight || !IsValid(ArenaFillLight))
	{
		ArenaFillLight = GetWorld()->SpawnActor<APointLight>(
			APointLight::StaticClass(),
			FVector(0.0f, 0.0f, 920.0f),
			FRotator::ZeroRotator);
	}
	if (ArenaFillLight && ArenaFillLight->PointLightComponent)
	{
		ArenaFillLight->PointLightComponent->SetMobility(EComponentMobility::Movable);
		ArenaFillLight->SetActorLocation(FVector(0.0f, 0.0f, 920.0f * ArenaScale));
		ArenaFillLight->PointLightComponent->SetLightColor(bClassicArenaLighting
			? FLinearColor(0.62f, 0.7f, 0.82f)
			: FLinearColor(0.72f, 0.78f, 0.88f));
		ArenaFillLight->PointLightComponent->SetIntensity(
			(bTestArenaMode ? 440.0f : bClassicArenaLighting ? 112.0f : bBobArenaLighting ? 260.0f : 190.0f)
			* FillMultiplier);
		ArenaFillLight->PointLightComponent->SetAttenuationRadius(
			(bBobArenaLighting ? 2400.0f : 1280.0f) * ArenaScale);
		ArenaFillLight->PointLightComponent->SetSourceRadius((bTestArenaMode ? 240.0f : 180.0f) * ArenaScale);
		ArenaFillLight->PointLightComponent->SetSpecularScale(bTestArenaMode ? 0.32f : bClassicArenaLighting ? 0.06f : 0.26f);
		ArenaFillLight->PointLightComponent->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.45f : 0.34f);
		ArenaFillLight->PointLightComponent->SetCastShadows(false);
	}

	// Large neutral cards create narrow, moving highlight bands on the prototype's
	// machined rings and graphite bevels. They are specular-first fixtures rather
	// than another arena flood, and are disabled outside Switchyard.
	const auto ConfigurePuckSoftbox = [this, ArenaScale](
		TObjectPtr<ARectLight>& LightActor,
		const FVector& Location,
		const FLinearColor& Color,
		const float Intensity,
		const float Width,
		const float Height)
	{
		if (!LightActor || !IsValid(LightActor))
		{
			LightActor = GetWorld()->SpawnActor<ARectLight>(
				ARectLight::StaticClass(), Location, FRotator::ZeroRotator);
		}
		if (!LightActor || !LightActor->RectLightComponent)
		{
			return;
		}
		URectLightComponent* Light = LightActor->RectLightComponent;
		Light->SetMobility(EComponentMobility::Movable);
		const FVector ScaledLocation = Location * ArenaScale;
		LightActor->SetActorLocation(ScaledLocation);
		LightActor->SetActorRotation(
			(FVector(0.0f, 0.0f, 45.0f * ArenaScale) - ScaledLocation).Rotation());
		Light->SetLightColor(Color);
		Light->SetIntensity(bTestArenaMode ? Intensity : 0.0f);
		Light->SetAttenuationRadius(1250.0f * ArenaScale);
		Light->SetSourceWidth(Width * ArenaScale);
		Light->SetSourceHeight(Height * ArenaScale);
		Light->SetSpecularScale(1.0f);
		Light->SetIndirectLightingIntensity(0.05f);
		Light->SetCastShadows(false);
	};
	ConfigurePuckSoftbox(TestPuckKeyLight, FVector(-620.0f, -420.0f, 560.0f),
		FLinearColor(0.82f, 0.91f, 1.0f), TestPuckKeyLightIntensity, 460.0f, 170.0f);
	ConfigurePuckSoftbox(TestPuckRimLight, FVector(600.0f, 300.0f, 450.0f),
		FLinearColor(1.0f, 0.86f, 0.72f), TestPuckRimLightIntensity, 360.0f, 130.0f);
}

void AFlickGameMode::SetFreeCameraLookSensitivity(const float Sensitivity)
{
	if (UFlickGameInstance* Instance = GetFlickGameInstance())
	{
		Instance->SetFreeCameraLookSensitivity(Sensitivity);
	}
}

void AFlickGameMode::SetFreeCameraMoveSensitivity(const float Sensitivity)
{
	if (UFlickGameInstance* Instance = GetFlickGameInstance())
	{
		Instance->SetFreeCameraMoveSensitivity(Sensitivity);
	}
}

void AFlickGameMode::SetShotMouseSensitivity(const float Sensitivity)
{
	if (UFlickGameInstance* Instance = GetFlickGameInstance())
	{
		Instance->SetShotMouseSensitivity(Sensitivity);
	}
}

void AFlickGameMode::SetGameplayCameraSensitivity(const float Sensitivity)
{
	if (UFlickGameInstance* Instance = GetFlickGameInstance())
	{
		Instance->SetGameplayCameraSensitivity(Sensitivity);
	}
}

void AFlickGameMode::SetClassName(const EFlickLineupPreset Preset, const FString& Name)
{
	if (UFlickGameInstance* Instance = GetFlickGameInstance())
	{
		Instance->SetClassName(Preset, Name);
	}
}

bool AFlickGameMode::ToggleTrainingDivider(const FVector& WorldLocation)
{
	if (!bTrainingEditMode || !CanEditTrainingBoard() || !TestArenaActor)
	{
		return false;
	}

	bool bEnabled = false;
	bool bChanged = false;
	if (!TestArenaActor->ToggleTrainingMechanismAtWorldLocation(WorldLocation, bEnabled, bChanged))
	{
		return false;
	}

	PushHudEvent(
		!bChanged
			? (TestArenaActor->GetMechanismCount() >= AFlickTestArena::MaxMechanismCount
				? TEXT("DIVIDER LIMIT REACHED")
				: TEXT("KEEP AT LEAST ONE DIVIDER"))
			: bEnabled ? TEXT("DIVIDER ACTIVATED") : TEXT("DIVIDER DEACTIVATED"),
		bChanged && bEnabled
			? FLinearColor(0.18f, 0.9f, 1.0f, 1.0f)
			: FLinearColor(0.62f, 0.68f, 0.72f, 1.0f),
		1.1f);
	return true;
}

void AFlickGameMode::SpawnArenaIfNeeded()
{
	if (!GetWorld())
	{
		return;
	}

	if (IsBobMode())
	{
		if (BobArenaActor && IsValid(BobArenaActor))
		{
			return;
		}
		BobArenaActor = GetWorld()->SpawnActor<AFlickBobArena>(
			AFlickBobArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		if (BobArenaActor)
		{
			BobArenaActor->InitializeArena(ArenaRadius, ArenaThickness, ArenaSurfaceZ);
		}
		return;
	}

	if (ArenaActor && IsValid(ArenaActor))
	{
		return;
	}
	// Every Knockout format now uses the promoted Switchyard arena. AFlickArena remains
	// the shared collision/base class behind AFlickTestArena, but is no longer spawned
	// as the old procedural presentation.
	TestArenaActor = GetWorld()->SpawnActor<AFlickTestArena>(
		AFlickTestArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	ArenaActor = TestArenaActor;
	if (ArenaActor)
	{
		TestArenaActor->InitializeTestArena(
			ArenaRadius, ArenaThickness, ArenaSurfaceZ, CurrentPlayersPerTeam);
	}
}

void AFlickGameMode::SpawnPieces()
{
	if (!GetWorld())
	{
		return;
	}
	if (IsBobMode())
	{
		SpawnBobPieces();
		return;
	}

	const float FormationDistance = ArenaRadius * FormationRadiusFraction;
	const FFlickModeRules& Rules = FlickModeRules::Get(ActiveMatchVariant);
	const int32 PiecesPerPlayer = Rules.StartingPiecesPerTeam;
	const TArray<FVector2D> PlayerPieceOffsets =
		FlickModeRules::BuildFormationOffsets(PiecesPerPlayer);
	TArray<FVector2D> TeamFormationPositions;
	if (CurrentPlayersPerTeam > 1)
	{
		TeamFormationPositions = FlickModeRules::BuildMultiplayerFormationPositions(
			ActiveMatchVariant,
			CurrentPlayersPerTeam,
			FormationDistance);
	}
	else
	{
		TeamFormationPositions.Reserve(PlayerPieceOffsets.Num());
		for (const FVector2D& Offset : PlayerPieceOffsets)
		{
			TeamFormationPositions.Add(FVector2D(Offset.X, FormationDistance + Offset.Y));
		}
	}
	int32 TeamPieceIndex = 0;
	for (int32 PlayerSlot = 0; PlayerSlot < CurrentPlayersPerTeam; ++PlayerSlot)
	{
		for (int32 LoadoutSlot = 0; LoadoutSlot < PlayerPieceOffsets.Num(); ++LoadoutSlot)
		{
			const FVector2D FormationPosition = TeamFormationPositions[TeamPieceIndex];
			const EFlickPieceArchetype Player1Archetype = GetPlayerClassPiece(
				EFlickTeam::Player1,
				PlayerSlot,
				LoadoutSlot);
			const EFlickPieceArchetype Player2Archetype = GetPlayerClassPiece(
				EFlickTeam::Player2,
				PlayerSlot,
				LoadoutSlot);
			const float Player1SpawnZ = ArenaSurfaceZ
				+ PieceThickness * FlickPieceArchetypeRules::Get(Player1Archetype).ThicknessMultiplier * 0.5f
				+ 3.0f;
			const float Player2SpawnZ = ArenaSurfaceZ
				+ PieceThickness * FlickPieceArchetypeRules::Get(Player2Archetype).ThicknessMultiplier * 0.5f
				+ 3.0f;
			SpawnPiece(
				EFlickTeam::Player1,
				TeamPieceIndex + 1,
				FVector(FormationPosition.X, -FormationPosition.Y, Player1SpawnZ),
				Player1Archetype,
				false,
				PlayerSlot,
				CurrentPlayersPerTeam > 1);
			SpawnPiece(
				EFlickTeam::Player2,
				TeamPieceIndex + 1 + CurrentStartingPiecesPerTeam,
				FVector(FormationPosition.X, FormationPosition.Y, Player2SpawnZ),
				Player2Archetype,
				false,
				PlayerSlot,
				CurrentPlayersPerTeam > 1);
			++TeamPieceIndex;
		}
	}
}

void AFlickGameMode::SpawnBobPieces()
{
	if (!GetWorld() || !BobArenaActor)
	{
		return;
	}

	const int32 RowCounts[] = {4, 5, 6, 5, 4};
	const float Spacing = PieceRadius * 2.28f;
	const float RowSpacing = Spacing * 0.86f;
	int32 RackIndex = 0;
	for (int32 RowIndex = 0; RowIndex < UE_ARRAY_COUNT(RowCounts); ++RowIndex)
	{
		const int32 CountInRow = RowCounts[RowIndex];
		const float Y = (static_cast<float>(RowIndex) - 2.0f) * RowSpacing;
		for (int32 ColumnIndex = 0; ColumnIndex < CountInRow; ++ColumnIndex)
		{
			const float X = (static_cast<float>(ColumnIndex) - (CountInRow - 1) * 0.5f) * Spacing;
			const EFlickTeam Team = RackIndex % 2 == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2;
			SpawnPiece(
				Team,
				RackIndex + 1,
				FVector(X, Y, ArenaSurfaceZ + PieceThickness * 0.5f + 3.0f),
				EFlickPieceArchetype::Standard);
			++RackIndex;
		}
	}

	Player1BobStriker = SpawnPiece(
		EFlickTeam::Player1,
		RackIndex + 1,
		BobArenaActor->GetStrikerStart(EFlickTeam::Player1, PieceThickness),
		EFlickPieceArchetype::Standard,
		true,
		0);
	Player2BobStriker = SpawnPiece(
		EFlickTeam::Player2,
		RackIndex + 2,
		BobArenaActor->GetStrikerStart(EFlickTeam::Player2, PieceThickness),
		EFlickPieceArchetype::Standard,
		true,
		1);
}

void AFlickGameMode::DestroyPieces()
{
	bMenuPartyDisplayInitialized = false;
	MenuPartyRosterKey.Reset();
	if (bCinematicReplayActive)
	{
		FinishCinematicRoundReplay(false);
	}
	ResetKickoffState();
	Player1BobStriker = nullptr;
	Player2BobStriker = nullptr;
	bPlayer1BobStrikerPocketed = false;
	bPlayer2BobStrikerPocketed = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->Destroy();
		}
	}
	Pieces.Empty();
}

void AFlickGameMode::StartMatch()
{
	SpawnCameraIfNeeded();
	SpawnAudioIfNeeded();
	SpawnLightingIfNeeded();
	SpawnArenaIfNeeded();
	DestroyPieces();
	SpawnPieces();

	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	LastImpactFeedbackTime = -100.0f;
	LastStrongImpactEventTime = -100.0f;

	AFlickGameState* FlickGameState = GetFlickGameState();
	if (FlickGameState)
	{
		ResetShotClock();
		FlickGameState->SetRoundAdvanceTimerState(false, RoundAdvanceTimeLimit);
		FlickGameState->SetTeamFormat(CurrentPlayersPerTeam);
		FlickGameState->SetMatchConfiguration(
			ActiveMatchVariant,
			ArenaSurfaceZ,
			MaxDragDistance,
			MinDragDistance,
			PowerExponent,
			MaxLaunchSpeed);
		FlickGameState->ResetSeriesState(CurrentRoundsToWin);
		FlickGameState->SetStartingPiecesPerTeam(CurrentStartingPiecesPerTeam);
		UpdateGameStateCounts();
		BeginOpeningPhase();
	}

	UE_LOG(LogFlick, Log, TEXT("Match started"));
}

void AFlickGameMode::TryStartNetworkMatch()
{
	if (!bNetworkMatchRequested || bNetworkMatchStarted)
	{
		return;
	}

	const AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bLobbyFilled = GetLobbyPlayerCount(EFlickTeam::Player1) == CurrentPlayersPerTeam
		&& GetLobbyPlayerCount(EFlickTeam::Player2) == CurrentPlayersPerTeam;
	if (!bLobbyFilled)
	{
		UE_LOG(
			LogFlick,
			Log,
			TEXT("Network match waiting for %d player(s) on each team (%d/%d vs %d/%d)"),
			CurrentPlayersPerTeam,
			GetLobbyPlayerCount(EFlickTeam::Player1),
			CurrentPlayersPerTeam,
			GetLobbyPlayerCount(EFlickTeam::Player2),
			CurrentPlayersPerTeam);
		return;
	}

	UE_LOG(
		LogFlick,
		Log,
		TEXT("NETWORK_LOBBY_FILLED: %s %dv%d roster connected"),
		bMatchmakingRequested
			? bRankedRequested ? TEXT("ranked") : TEXT("casual")
			: TEXT("private"),
		CurrentPlayersPerTeam,
		CurrentPlayersPerTeam);
	if (bMatchmakingRequested && !bMatchmakingLobbyLocked)
	{
		bMatchmakingLobbyLocked = true;
		MatchFoundConfirmationElapsed = 0.0f;
		if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
		{
			Sessions->LockMatchmakingLobby();
		}
		if (AFlickGameState* State = GetFlickGameState())
		{
			UpdateReplicatedMatchmakingState(false);
		}
	}
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickPartyReservationSmokeTest")))
	{
		TMap<FString, EFlickTeam> ObservedPartyTeams;
		TSet<FString> ObservedPartySlots;
		bool bReservationPassed = true;
		int32 PremadeMembers = 0;
		if (const AFlickGameState* State = GetFlickGameState())
		{
			for (const APlayerState* StatePlayer : State->PlayerArray)
			{
				const AFlickPlayerState* Player = Cast<AFlickPlayerState>(StatePlayer);
				if (!Player || Player->GetPartyId().IsEmpty())
				{
					continue;
				}
				++PremadeMembers;
				if (const EFlickTeam* ExistingTeam = ObservedPartyTeams.Find(Player->GetPartyId()))
				{
					bReservationPassed &= *ExistingTeam == Player->GetTeam();
				}
				else
				{
					ObservedPartyTeams.Add(Player->GetPartyId(), Player->GetTeam());
				}
				bReservationPassed &= Player->GetPartySlot() >= 0 && Player->GetTeamPlayerSlot() >= 0;
				const FString SlotKey = FString::Printf(TEXT("%s:%d"), *Player->GetPartyId(), Player->GetTeamPlayerSlot());
				bReservationPassed &= !ObservedPartySlots.Contains(SlotKey);
				ObservedPartySlots.Add(SlotKey);
			}
		}
		bReservationPassed &= PremadeMembers >= 2;
		UE_LOG(
			LogFlick,
			Log,
			TEXT("PARTY_TEAM_RESERVATION_SMOKE_TEST %s: premadeMembers=%d groups=%d"),
			bReservationPassed ? TEXT("PASS") : TEXT("FAIL"),
			PremadeMembers,
			ObservedPartyTeams.Num());
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkSmokeTest")))
	{
		const bool bLobbyPassed = IsNetworkLobby()
			&& FlickGameState
			&& FlickGameState->bNetworkLobbyActive
			&& FlickGameState->MatchPhase == EFlickMatchPhase::WaitingToStart;
		UE_LOG(LogFlick, Log, TEXT("NETWORK_LOBBY_SMOKE_TEST %s: connected players remain in lobby until ready"), bLobbyPassed ? TEXT("PASS") : TEXT("FAIL"));
	}
#endif
	if (bMatchmakingRequested && CanStartNetworkMatch())
	{
		if (bRankedRequested)
		{
			RegisterRankedMatchThenStart();
		}
		else
		{
			CompleteNetworkMatchStart(CoordinatorMatchId.IsEmpty()
				? FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens)
				: CoordinatorMatchId);
		}
	}
}

bool AFlickGameMode::FindAvailableTeamAndSlot(
	const APlayerController* PlayerToIgnore,
	EFlickTeam& OutTeam,
	int32& OutPlayerSlot) const
{
	OutTeam = EFlickTeam::None;
	OutPlayerSlot = INDEX_NONE;
	bool Player1Slots[3] = {false, false, false};
	bool Player2Slots[3] = {false, false, false};
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (FlickGameState)
	{
		for (const APlayerState* PlayerState : FlickGameState->PlayerArray)
		{
			if (!PlayerState || PlayerState == (PlayerToIgnore ? PlayerToIgnore->PlayerState : nullptr))
			{
				continue;
			}
			const AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(PlayerState);
			if (!FlickPlayerState)
			{
				continue;
			}
			const int32 Slot = FlickPlayerState->GetTeamPlayerSlot();
			if (Slot < 0 || Slot >= CurrentPlayersPerTeam)
			{
				continue;
			}
			if (FlickPlayerState->GetTeam() == EFlickTeam::Player1)
			{
				Player1Slots[Slot] = true;
			}
			else if (FlickPlayerState->GetTeam() == EFlickTeam::Player2)
			{
				Player2Slots[Slot] = true;
			}
		}
	}
	for (const TPair<FString, TArray<int32>>& Reservation : PremadePartySlots)
	{
		const EFlickTeam* ReservedTeam = PremadePartyTeams.Find(Reservation.Key);
		bool* Slots = ReservedTeam && *ReservedTeam == EFlickTeam::Player1
			? Player1Slots
			: ReservedTeam && *ReservedTeam == EFlickTeam::Player2 ? Player2Slots : nullptr;
		if (!Slots)
		{
			continue;
		}
		for (const int32 ReservedSlot : Reservation.Value)
		{
			if (ReservedSlot >= 0 && ReservedSlot < CurrentPlayersPerTeam)
			{
				Slots[ReservedSlot] = true;
			}
		}
	}

	const int32 Player1Count = GetLobbyPlayerCount(EFlickTeam::Player1);
	const int32 Player2Count = GetLobbyPlayerCount(EFlickTeam::Player2);
	EFlickTeam PreferredTeam = EFlickTeam::Player1;
	if (Player1Count > 0 && Player2Count < CurrentPlayersPerTeam)
	{
		PreferredTeam = EFlickTeam::Player2;
	}
	else if (Player1Count >= CurrentPlayersPerTeam && Player2Count < CurrentPlayersPerTeam)
	{
		PreferredTeam = EFlickTeam::Player2;
	}

	auto FindOpenSlot = [this](const bool Slots[3])
	{
		for (int32 Slot = 0; Slot < CurrentPlayersPerTeam; ++Slot)
		{
			if (!Slots[Slot])
			{
				return Slot;
			}
		}
		return static_cast<int32>(INDEX_NONE);
	};

	OutPlayerSlot = PreferredTeam == EFlickTeam::Player1
		? FindOpenSlot(Player1Slots)
		: FindOpenSlot(Player2Slots);
	if (OutPlayerSlot != INDEX_NONE)
	{
		OutTeam = PreferredTeam;
		return true;
	}

	const EFlickTeam OtherTeam = GetOpposingTeam(PreferredTeam);
	OutPlayerSlot = OtherTeam == EFlickTeam::Player1
		? FindOpenSlot(Player1Slots)
		: FindOpenSlot(Player2Slots);
	if (OutPlayerSlot != INDEX_NONE)
	{
		OutTeam = OtherTeam;
		return true;
	}
	return false;
}

int32& AFlickGameMode::GetNextPlayerSlot(const EFlickTeam Team)
{
	return Team == EFlickTeam::Player2 ? Player2NextPlayerSlot : Player1NextPlayerSlot;
}

int32 AFlickGameMode::GetNextPlayerSlot(const EFlickTeam Team) const
{
	return Team == EFlickTeam::Player2 ? Player2NextPlayerSlot : Player1NextPlayerSlot;
}

bool AFlickGameMode::GetNextScheduledTurn(EFlickTeam& OutTeam, int32& OutPlayerSlot) const
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State || CurrentPlayersPerTeam <= 1
		|| State->CurrentTeam == EFlickTeam::None
		|| State->MatchPhase == EFlickMatchPhase::RoundOver)
	{
		OutTeam = EFlickTeam::None;
		OutPlayerSlot = INDEX_NONE;
		return false;
	}

	OutTeam = GetOpposingTeam(State->CurrentTeam);
	OutPlayerSlot = FindEligiblePlayerSlot(OutTeam, GetNextPlayerSlot(OutTeam));
	return OutTeam != EFlickTeam::None && OutPlayerSlot != INDEX_NONE;
}

bool AFlickGameMode::HasActivePieceForPlayer(
	const EFlickTeam Team,
	const int32 TeamPlayerSlot) const
{
	if (IsBobMode())
	{
		const AFlickPiece* Striker = GetBobStriker(Team);
		return Striker && Striker->IsActive();
	}
	for (const AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive()
			&& Piece->GetTeam() == Team
			&& Piece->GetOwningPlayerSlot() == TeamPlayerSlot)
		{
			return true;
		}
	}
	return false;
}

int32 AFlickGameMode::FindEligiblePlayerSlot(
	const EFlickTeam Team,
	const int32 StartingSlot) const
{
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(CurrentPlayersPerTeam);
	int32 Candidate = FMath::Clamp(StartingSlot, 0, TeamSize - 1);
	for (int32 Attempt = 0; Attempt < TeamSize; ++Attempt)
	{
		const bool bConnected = GetNetMode() == NM_Standalone || GetLobbyPlayer(Team, Candidate) != nullptr;
		if (bConnected && HasActivePieceForPlayer(Team, Candidate))
		{
			return Candidate;
		}
		Candidate = FlickTeamRules::AdvancePlayerSlot(Candidate, TeamSize);
	}
	return FMath::Clamp(StartingSlot, 0, TeamSize - 1);
}

void AFlickGameMode::AdvanceCompletedPlayerTurn(
	const EFlickTeam Team,
	const int32 CompletedPlayerSlot)
{
	GetNextPlayerSlot(Team) = FlickTeamRules::AdvancePlayerSlot(
		CompletedPlayerSlot,
		CurrentPlayersPerTeam);
}

void AFlickGameMode::ActivateNextPlayerForTeam(const EFlickTeam Team)
{
	if (AFlickGameState* State = GetFlickGameState())
	{
		const int32 PlayerSlot = FindEligiblePlayerSlot(Team, GetNextPlayerSlot(Team));
		GetNextPlayerSlot(Team) = PlayerSlot;
		State->SetCurrentTeamPlayerSlot(PlayerSlot);
	}
}

void AFlickGameMode::BeginOpeningPhase()
{
	ResetKickoffState();
	ResetTrainingBotThinking();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		if (IsFreePlayTraining() || IsTutorialMode())
		{
			Player1NextPlayerSlot = 0;
			Player2NextPlayerSlot = 0;
			FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
			ActivateNextPlayerForTeam(EFlickTeam::Player1);
			SetCameraViewForTeam(EFlickTeam::Player1);
			FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
			return;
		}
		const int32 OpeningPlayerSlot = (FlickGameState->RoundNumber - 1)
			% FlickTeamRules::ClampPlayersPerTeam(CurrentPlayersPerTeam);
		Player1NextPlayerSlot = OpeningPlayerSlot;
		Player2NextPlayerSlot = OpeningPlayerSlot;
		FlickGameState->SetCurrentTeam(FlickGameState->RoundStartingTeam);
		ActivateNextPlayerForTeam(FlickGameState->CurrentTeam);
		SetCameraViewForTeam(FlickGameState->CurrentTeam);
		if (IsBobMode())
		{
			FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
			return;
		}
		const bool bSimultaneousKickoff = bUseSimultaneousKickoff;
		FlickGameState->SetKickoffProgress(
			0,
			bSimultaneousKickoff
				? FlickTeamRules::GetSimultaneousKickoffShotCount(CurrentPlayersPerTeam)
				: 0);
		FlickGameState->SetMatchPhase(
			bSimultaneousKickoff ? EFlickMatchPhase::KickoffPlanning : EFlickMatchPhase::Aiming);
	}
}

void AFlickGameMode::ResetKickoffState()
{
	for (const FFlickLockedKickoffShot& Shot : LockedKickoffShots)
	{
		if (Shot.Piece.IsValid())
		{
			Shot.Piece->SetKickoffLocked(false);
		}
	}
	LockedKickoffShots.Reset();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetKickoffProgress(0, 0);
	}
	bResolvingKickoff = false;
}

void AFlickGameMode::RebuildMatch()
{
	ClearControllerAiming();
	if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		if (AFlickHUD* FlickHUD = Cast<AFlickHUD>(FlickController->GetHUD()))
		{
			FlickHUD->ResetPresentation();
		}
	}

	DestroyPieces();
	if (ArenaActor && IsValid(ArenaActor))
	{
		ArenaActor->Destroy();
		ArenaActor = nullptr;
		TestArenaActor = nullptr;
	}
	if (BobArenaActor && IsValid(BobArenaActor))
	{
		BobArenaActor->Destroy();
		BobArenaActor = nullptr;
	}
	StartMatch();
}

void AFlickGameMode::ApplySelectedMatchConfiguration()
{
	ApplyMatchConfiguration(SelectedMatchVariant, CurrentPlayersPerTeam);
}

void AFlickGameMode::ApplyMatchConfiguration(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam)
{
	ActiveMatchVariant = NormalizeMatchVariant(Variant);
	const FFlickModeRules& Rules = FlickModeRules::Get(ActiveMatchVariant);
	// Bob has its own arena; all Knockout team sizes use Switchyard.
	bTestArenaMode = Rules.bUseSwitchyardArena;
	CurrentPlayersPerTeam = ActiveMatchVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	CurrentStartingPiecesPerTeam = FlickModeRules::GetStartingPiecesPerTeam(
		ActiveMatchVariant,
		CurrentPlayersPerTeam);
	CurrentRoundsToWin = Rules.RoundsToWin;
	bCurrentModeSupportsLoadouts = Rules.bSupportsLoadouts;
	ArenaRadius = FlickModeRules::GetArenaRadius(ActiveMatchVariant, CurrentPlayersPerTeam);
	PieceRadius = Rules.PieceRadius;
	PieceThickness = Rules.PieceThickness;
	MaxLaunchSpeed = Rules.MaxLaunchSpeed;
	MaxDragDistance = Rules.MaxDragDistance;
	PieceFriction = Rules.PieceFriction;
	PieceRestitution = Rules.PieceRestitution;
	LinearDamping = Rules.LinearDamping;
	AngularDamping = Rules.AngularDamping;
	MaximumResolutionDuration = Rules.MaximumResolutionDuration;
	bUseSimultaneousKickoff = Rules.bUseSimultaneousKickoff;
	if (bPrivateMatchActive && ActiveMatchVariant == EFlickMatchVariant::Classic)
	{
		CurrentRoundsToWin = FMath::Clamp(PrivateMatchSettings.RoundsToWin, 1, 5);
		ArenaRadius *= FMath::Clamp(PrivateMatchSettings.ArenaScale, 0.85f, 1.3f);
		MaxLaunchSpeed *= FMath::Clamp(PrivateMatchSettings.LaunchSpeedScale, 0.75f, 1.5f);
		PieceFriction *= FMath::Clamp(PrivateMatchSettings.FrictionScale, 0.5f, 2.0f);
		PieceRestitution *= FMath::Clamp(PrivateMatchSettings.RestitutionScale, 0.5f, 1.5f);
		bUseSimultaneousKickoff = PrivateMatchSettings.bSimultaneousKickoff;
	}
	if (CameraPawn)
	{
		CameraPawn->SetArenaFramingScale(ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
	}
}

void AFlickGameMode::UpdateMainMenuPresentation()
{
	if (FrontendScreen != EFlickFrontendScreen::MainMenu || bNetworkMatchRequested)
	{
		return;
	}
	// The frontend always returns to one stable arena. Play-mode selection can
	// still preview its arena, but the main menu no longer cycles or rebuilds it.
	if (ActiveMatchVariant != EFlickMatchVariant::Classic || CurrentPlayersPerTeam != 1)
	{
		ApplyMatchConfiguration(EFlickMatchVariant::Classic, 1);
		RebuildMatch();
		SetCameraForFrontend();
	}
	const UFlickPartySubsystem* Party = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
	const UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
	FString RosterKey;
	TArray<EFlickPieceArchetype> DisplayArchetypes;
	if (Party && Party->IsActive())
	{
		for (const FFlickPartyMember& Member : Party->GetMembers())
		{
			if (DisplayArchetypes.Num() >= 6) break;
			DisplayArchetypes.Add(Member.ShowcaseArchetype);
			RosterKey += FString::Printf(TEXT("%s:%d|"), *Member.UserId,
				static_cast<int32>(Member.ShowcaseArchetype));
		}
	}
	else
	{
		const EFlickPieceArchetype LocalArchetype = Sessions
			? Sessions->GetShowcaseArchetype() : EFlickPieceArchetype::Standard;
		DisplayArchetypes.Add(LocalArchetype);
		RosterKey = FString::Printf(TEXT("SOLO:%d"), static_cast<int32>(LocalArchetype));
	}
	if (bMenuPartyDisplayInitialized && MenuPartyRosterKey == RosterKey)
	{
		return;
	}
	DestroyPieces();
	const int32 DisplayCount = DisplayArchetypes.Num();
	for (int32 Index = 0; Index < DisplayCount; ++Index)
	{
		const EFlickPieceArchetype Archetype = DisplayArchetypes[Index];
		const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
		const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(DisplayCount) - PI * 0.5f;
		const float Radius = DisplayCount == 1 ? 0.0f : 175.0f;
		const FVector Location(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle),
			ArenaSurfaceZ + PieceThickness * Rules.ThicknessMultiplier * 0.5f + 3.0f);
		AFlickPiece* Piece = SpawnPiece(Index % 2 == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2,
			Index + 1, Location, Archetype);
		if (Piece)
		{
			Piece->BeginReplayPresentation();
		}
	}
	MenuPartyRosterKey = MoveTemp(RosterKey);
	bMenuPartyDisplayInitialized = true;
}

void AFlickGameMode::ShowModePreview(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam)
{
	const EFlickMatchVariant PreviewVariant = NormalizeMatchVariant(Variant);
	const int32 PreviewPlayersPerTeam = PreviewVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	ApplyMatchConfiguration(PreviewVariant, PreviewPlayersPerTeam);
	RebuildMatch();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	SetCameraForFrontend();
	UE_LOG(
		LogFlick,
		Log,
		TEXT("Frontend preview changed to %s %dv%d"),
		*GetMatchVariantName(PreviewVariant),
		PreviewPlayersPerTeam,
		PreviewPlayersPerTeam);
}

void AFlickGameMode::SetCameraForFrontend()
{
	if (CameraPawn)
	{
		const bool bSettingsOverMatch = FrontendScreen == EFlickFrontendScreen::Settings
			&& SettingsReturnScreen == EFlickFrontendScreen::Paused;
		const bool bClassSelectionOverMatch = FrontendScreen == EFlickFrontendScreen::ClassSelect
			&& ClassSelectionReturnScreen == EFlickFrontendScreen::Paused;
		CameraPawn->SetMenuPresentation(FrontendScreen != EFlickFrontendScreen::Playing
			&& FrontendScreen != EFlickFrontendScreen::Paused
			&& !bSettingsOverMatch
			&& !bClassSelectionOverMatch);
		CameraPawn->SetMenuOrbitEnabled(FrontendScreen == EFlickFrontendScreen::MainMenu);
	}
	// Existing light actors are reused across frontend and gameplay. Retune them
	// whenever presentation state changes so showcase exposure cannot leak into
	// a live match (or vice versa).
	SpawnLightingIfNeeded();
}

void AFlickGameMode::SetCameraViewForTeam(const EFlickTeam Team, const bool bSnap)
{
	if (Team == EFlickTeam::None)
	{
		return;
	}

	// Network players keep the camera on their own side. Turn ownership must not
	// move one player's viewpoint to the opponent's end of the table.
	if (GetNetMode() != NM_Standalone)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get());
			const AFlickPlayerState* PlayerState = Controller
				? Controller->GetPlayerState<AFlickPlayerState>()
				: nullptr;
			if (Controller && PlayerState && PlayerState->GetTeam() != EFlickTeam::None)
			{
				Controller->SetGameplayCameraTeamFromServer(PlayerState->GetTeam(), bSnap);
			}
		}
		return;
	}

	if (!CameraPawn)
	{
		return;
	}
	const EFlickTeam CameraTeam = IsTrainingBotMatch() ? EFlickTeam::Player1 : Team;
	CameraPawn->SetGameplayViewIndex(CameraTeam == EFlickTeam::Player2 ? 2 : 0, bSnap);
}

void AFlickGameMode::ClearControllerAiming() const
{
	if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		FlickController->ClearAiming();
	}
}

void AFlickGameMode::AddCameraFeedback(const float Strength) const
{
	if (CameraPawn)
	{
		CameraPawn->AddCameraImpulse(Strength * GetCameraShakeIntensity());
	}
}

void AFlickGameMode::AddControllerFeedback(const float Strength, const float Duration) const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	PlayerController->PlayDynamicForceFeedback(
		FMath::Clamp(Strength, 0.0f, 1.0f),
		FMath::Max(Duration, 0.01f),
		true,
		true,
		true,
		true,
		EDynamicForceFeedbackAction::Start);
}

