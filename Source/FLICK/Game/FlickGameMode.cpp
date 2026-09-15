#include "Game/FlickGameMode.h"
#include "Game/FlickGameModePrivate.h"

using namespace FlickGameModePrivate;

AFlickGameMode::AFlickGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	TrainingBotEasySettings.ThinkDelay = 1.65f;
	TrainingBotEasySettings.MinimumPower = 0.34f;
	TrainingBotEasySettings.MaximumPower = 0.78f;
	TrainingBotEasySettings.AimErrorDegrees = 12.0f;
	TrainingBotEasySettings.PowerVariation = 0.17f;
	TrainingBotEasySettings.DecisionNoise = 1.9f;
	TrainingBotEasySettings.DividerAwareness = 0.05f;
	TrainingBotEasySettings.BankShotSkill = 0.0f;

	TrainingBotNormalSettings.ThinkDelay = 1.1f;
	TrainingBotNormalSettings.MinimumPower = 0.48f;
	TrainingBotNormalSettings.MaximumPower = 0.89f;
	TrainingBotNormalSettings.AimErrorDegrees = 3.0f;
	TrainingBotNormalSettings.PowerVariation = 0.055f;
	TrainingBotNormalSettings.DecisionNoise = 0.32f;
	TrainingBotNormalSettings.DividerAwareness = 0.42f;
	TrainingBotNormalSettings.BankShotSkill = 0.1f;

	TrainingBotHardSettings.ThinkDelay = 0.9f;
	TrainingBotHardSettings.MinimumPower = 0.52f;
	TrainingBotHardSettings.MaximumPower = 0.94f;
	TrainingBotHardSettings.AimErrorDegrees = 1.1f;
	TrainingBotHardSettings.PowerVariation = 0.024f;
	TrainingBotHardSettings.DecisionNoise = 0.08f;
	TrainingBotHardSettings.DividerAwareness = 0.82f;
	TrainingBotHardSettings.BankShotSkill = 0.52f;

	TrainingBotExpertSettings.ThinkDelay = 0.72f;
	TrainingBotExpertSettings.MinimumPower = 0.55f;
	TrainingBotExpertSettings.MaximumPower = 0.98f;
	TrainingBotExpertSettings.AimErrorDegrees = 0.12f;
	TrainingBotExpertSettings.PowerVariation = 0.004f;
	TrainingBotExpertSettings.DecisionNoise = 0.004f;
	TrainingBotExpertSettings.DividerAwareness = 1.0f;
	TrainingBotExpertSettings.BankShotSkill = 1.0f;

	GameStateClass = AFlickGameState::StaticClass();
	PlayerControllerClass = AFlickPlayerController::StaticClass();
	PlayerStateClass = AFlickPlayerState::StaticClass();
	DefaultPawnClass = AFlickCameraPawn::StaticClass();
	HUDClass = AFlickHUD::StaticClass();
}
void AFlickGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bNetworkMatchRequested = UGameplayStatics::HasOption(Options, TEXT("FlickNetworkMatch"));
	bPartyRequested = UGameplayStatics::HasOption(Options, TEXT("FlickParty"));
	bMatchmakingRequested = UGameplayStatics::HasOption(Options, TEXT("FlickMatchmaking"));
	bRankedRequested = bMatchmakingRequested && UGameplayStatics::HasOption(Options, TEXT("FlickRanked"));
	bRankedQueueSelected = bRankedRequested;
	const FString PlayersPerTeamOption = UGameplayStatics::ParseOption(Options, TEXT("FlickPlayersPerTeam"));
	if (!PlayersPerTeamOption.IsEmpty())
	{
		CurrentPlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(FCString::Atoi(*PlayersPerTeamOption));
	}
	MatchmakingPlayersPerTeam = CurrentPlayersPerTeam;
	const FString VariantOption = UGameplayStatics::ParseOption(Options, TEXT("FlickVariant"));
	if (!VariantOption.IsEmpty())
	{
		SelectedMatchVariant = NormalizeMatchVariant(
			static_cast<EFlickMatchVariant>(FMath::Clamp(FCString::Atoi(*VariantOption), 0, 2)));
		bVariantProvidedByTravel = true;
	}
	CoordinatorMatchId = UGameplayStatics::ParseOption(Options, TEXT("FlickCoordinatorMatchId"));
	int32 CoordinatorOptionSeparator = INDEX_NONE;
	if (CoordinatorMatchId.FindChar(TEXT('?'), CoordinatorOptionSeparator))
	{
		CoordinatorMatchId.LeftInline(CoordinatorOptionSeparator);
	}
	CoordinatorMatchId.LeftInline(128);
	const FString QueueRatingOption = UGameplayStatics::ParseOption(Options, TEXT("FlickQueueRating"));
	if (!QueueRatingOption.IsEmpty())
	{
		RankedQueueRating = FMath::Clamp(FCString::Atoi(*QueueRatingOption), 0, 3000);
	}
}

FString AFlickGameMode::InitNewPlayer(
	APlayerController* NewPlayerController,
	const FUniqueNetIdRepl& UniqueId,
	const FString& Options,
	const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	if (!NewPlayerController)
	{
		return Result;
	}
	const FString PartyId = UGameplayStatics::ParseOption(Options, TEXT("FlickPartyId"));
	if (!PartyId.IsEmpty())
	{
		IncomingPartyIds.Add(NewPlayerController, PartyId.Left(64));
		IncomingPartySlots.Add(
			NewPlayerController,
			FMath::Clamp(FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("FlickPartySlot"))), 0, FlickMaximumPartyMembers - 1));
		IncomingPartySizes.Add(
			NewPlayerController,
			FMath::Clamp(FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("FlickPartySize"))), 1, FlickMaximumPartyMembers));
		IncomingPartyLeaders.Add(
			NewPlayerController,
			UGameplayStatics::ParseOption(Options, TEXT("FlickPartyLeader")) == TEXT("1"));
	}
	const FString RankedRatingOption = UGameplayStatics::ParseOption(Options, TEXT("FlickQueueRating"));
	if (!RankedRatingOption.IsEmpty())
	{
		IncomingRankedRatings.Add(
			NewPlayerController,
			FMath::Clamp(FCString::Atoi(*RankedRatingOption), 0, 3000));
	}
	const FString CoordinatorAccountId = UGameplayStatics::ParseOption(Options, TEXT("FlickAccountId")).Left(128);
	const FString CoordinatorReservation = UGameplayStatics::ParseOption(Options, TEXT("FlickReservation")).Left(512);
	if (!CoordinatorAccountId.IsEmpty() && !CoordinatorReservation.IsEmpty())
	{
		IncomingCoordinatorAccountIds.Add(NewPlayerController, CoordinatorAccountId);
		IncomingCoordinatorReservations.Add(NewPlayerController, CoordinatorReservation);
		IncomingCoordinatorTeams.Add(
			NewPlayerController,
			static_cast<EFlickTeam>(FMath::Clamp(
				FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("FlickTeam"))),
				0,
				2)));
		IncomingCoordinatorSlots.Add(
			NewPlayerController,
			FMath::Clamp(
				FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("FlickPlayerSlot"))),
				0,
				2));
	}
	return Result;
}

void AFlickGameMode::BeginPlay()
{
	Super::BeginPlay();
	bNetworkMatchRequested |= FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkMatch"));
	bPartyRequested |= FParse::Param(FCommandLine::Get(), TEXT("FlickParty"));
	bMatchmakingRequested |= FParse::Param(FCommandLine::Get(), TEXT("FlickMatchmaking"));
	bRankedRequested |= bMatchmakingRequested && FParse::Param(FCommandLine::Get(), TEXT("FlickRanked"));
	bRankedQueueSelected = bRankedRequested;
	FString CommandLineMatchId;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickCoordinatorMatchId="), CommandLineMatchId))
	{
		CoordinatorMatchId = CommandLineMatchId.Left(128);
	}
	int32 CommandLineVariant = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickVariant="), CommandLineVariant))
	{
		SelectedMatchVariant = NormalizeMatchVariant(
			static_cast<EFlickMatchVariant>(FMath::Clamp(CommandLineVariant, 0, 2)));
		bVariantProvidedByTravel = true;
	}
	int32 CommandLinePlayersPerTeam = CurrentPlayersPerTeam;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickPlayersPerTeam="), CommandLinePlayersPerTeam))
	{
		CurrentPlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(CommandLinePlayersPerTeam);
		MatchmakingPlayersPerTeam = CurrentPlayersPerTeam;
	}
	if (!bVariantProvidedByTravel)
	{
		if (const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
		{
			SelectedMatchVariant = NormalizeMatchVariant(FlickGameInstance->GetSelectedMatchVariant());
		}
	}
	MenuPreviewVariant = SelectedMatchVariant;
	MenuPreviewPlayersPerTeam = CurrentPlayersPerTeam;
	ApplySelectedMatchConfiguration();
	StartMatch();
	FrontendScreen = bNetworkMatchRequested
		? EFlickFrontendScreen::NetworkLobby
		: EFlickFrontendScreen::MainMenu;
	SetCameraForFrontend();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
		FlickGameState->SetTeamFormat(CurrentPlayersPerTeam);
		FlickGameState->SetNetworkLobbyState(bNetworkMatchRequested, SelectedMatchVariant);
		FlickGameState->SetPartyState(bPartyRequested, FString(), FlickMaximumPartyMembers);
		FlickGameState->SetMatchmakingState(
			bMatchmakingRequested,
			false,
			bRankedRequested,
			RankedQueueRating,
			bRankedRequested ? FlickRankRules::GetSearchRangeForAttempt(0) : 0);
	}
	if (!CoordinatorMatchId.IsEmpty())
	{
		if (UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem())
		{
			Coordinator->BeginServerHeartbeat(CoordinatorMatchId);
		}
	}
	if (bPartyRequested)
	{
		FTimerHandle InviteTimer;
		GetWorldTimerManager().SetTimer(InviteTimer, [this]()
		{
			if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
			{
				Sessions->SendPendingPartyInvite();
			}
		}, 1.0f, false);
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickMenuCyclePreview")))
	{
		MenuPreviewDuration = 0.8f;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickCoordinatorAutoQueue"))
		&& !bCoordinatorAutoQueueConsumed
		&& !bNetworkMatchRequested)
	{
		bCoordinatorAutoQueueConsumed = true;
		int32 AutoQueueTeamSize = 1;
		FParse::Value(FCommandLine::Get(), TEXT("FlickCoordinatorTeamSize="), AutoQueueTeamSize);
		MatchmakingPlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(AutoQueueTeamSize);
		bRankedQueueSelected = FParse::Param(FCommandLine::Get(), TEXT("FlickRanked"));
		FTimerHandle AutoQueueTimer;
		GetWorldTimerManager().SetTimer(AutoQueueTimer, [this]()
		{
			OpenOnlineBrowser();
			StartSelectedMatchmaking();
		}, 1.5f, false);
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("FlickOnlineHostSmokeTest")) && !bNetworkMatchRequested)
	{
		OpenModeSelect();
		HostOnlineNetworkMatch();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickOnlineBrowserPreview")))
	{
		OpenModeSelect();
		OpenOnlineBrowser();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickClassSelectPreview")))
	{
		int32 PreviewPlayersPerTeam = 3;
		FParse::Value(FCommandLine::Get(), TEXT("FlickClassPlayers="), PreviewPlayersPerTeam);
		SelectMatchVariant(EFlickMatchVariant::Classic);
		SetMatchmakingPlayersPerTeam(PreviewPlayersPerTeam);
		PrepareClassSelection(false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickClassFlowTest")))
	{
		SelectMatchVariant(EFlickMatchVariant::Classic);
		SetMatchmakingPlayersPerTeam(3);
		PrepareClassSelection(false);
		SelectPlayerClass(EFlickTeam::Player1, 0, EFlickLineupPreset::Power);
		SelectPlayerClass(EFlickTeam::Player1, 1, EFlickLineupPreset::Speed);
		SelectPlayerClass(EFlickTeam::Player1, 2, EFlickLineupPreset::Control);
		SelectPlayerClass(EFlickTeam::Player2, 0, EFlickLineupPreset::Control);
		ConfirmClassSelection();
		const auto IsPlayableClass = [](const EFlickLineupPreset Preset)
		{
			return Preset == EFlickLineupPreset::Balanced
				|| Preset == EFlickLineupPreset::Power
				|| Preset == EFlickLineupPreset::Speed
				|| Preset == EFlickLineupPreset::Control;
		};
		const bool bInitialClassesApplied = GetPlayerClass(EFlickTeam::Player1, 0) == EFlickLineupPreset::Power
			&& IsPlayableClass(GetPlayerClass(EFlickTeam::Player1, 1))
			&& IsPlayableClass(GetPlayerClass(EFlickTeam::Player1, 2))
			&& IsPlayableClass(GetPlayerClass(EFlickTeam::Player2, 0))
			&& IsPlayableClass(GetPlayerClass(EFlickTeam::Player2, 1))
			&& IsPlayableClass(GetPlayerClass(EFlickTeam::Player2, 2));
		const TArray<EFlickLineupPreset> InitialBlueClasses = Player1ActiveClasses;
		const TArray<EFlickLineupPreset> InitialOrangeClasses = Player2ActiveClasses;

		if (AFlickGameState* State = GetFlickGameState())
		{
			State->CompleteRound(EFlickMatchOutcome::Player1Wins);
		}
		TogglePauseMenu();
		OpenClassChange();
		SelectPlayerClass(EFlickTeam::Player1, 0, EFlickLineupPreset::Control);
		ConfirmClassSelection();
		bool bOtherBlueClassesUnchanged = Player1PendingClasses.Num() == InitialBlueClasses.Num();
		for (int32 PlayerSlot = 1; bOtherBlueClassesUnchanged && PlayerSlot < InitialBlueClasses.Num(); ++PlayerSlot)
		{
			bOtherBlueClassesUnchanged = Player1PendingClasses[PlayerSlot] == InitialBlueClasses[PlayerSlot];
		}
		const bool bChangeQueued = bHasPendingClassChanges
			&& GetPlayerClass(EFlickTeam::Player1, 0) == EFlickLineupPreset::Power
			&& Player1PendingClasses.IsValidIndex(0)
			&& Player1PendingClasses[0] == EFlickLineupPreset::Control
			&& bOtherBlueClassesUnchanged
			&& Player2PendingClasses == InitialOrangeClasses;
		TogglePauseMenu();
		StartNextRound();
		const bool bChangeApplied = !bHasPendingClassChanges
			&& GetPlayerClass(EFlickTeam::Player1, 0) == EFlickLineupPreset::Control
			&& Pieces.ContainsByPredicate([](const TObjectPtr<AFlickPiece>& Piece)
			{
				return Piece
					&& Piece->GetTeam() == EFlickTeam::Player1
					&& Piece->GetOwningPlayerSlot() == 0
					&& Piece->GetArchetype() == EFlickPieceArchetype::Grippy;
			});
		const bool bClassFlowPassed = bInitialClassesApplied && bChangeQueued && bChangeApplied;
		if (bClassFlowPassed)
		{
			UE_LOG(LogFlick, Log, TEXT("CLASS_SELECTION_FLOW_TEST PASS: initial=1 queued=1 applied=1"));
		}
		else
		{
			UE_LOG(
				LogFlick,
				Error,
				TEXT("CLASS_SELECTION_FLOW_TEST FAIL: initial=%d queued=%d applied=%d"),
				bInitialClassesApplied ? 1 : 0,
				bChangeQueued ? 1 : 0,
				bChangeApplied ? 1 : 0);
		}
		FTimerHandle ClassFlowQuitTimer;
		GetWorldTimerManager().SetTimer(ClassFlowQuitTimer, [this]() { QuitGame(); }, 0.5f, false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickModeSelectPreview")))
	{
		OpenModeSelect();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickSettingsPreview")))
	{
		FrontendScreen = EFlickFrontendScreen::Settings;
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickProfilePreview")))
	{
		OpenProfile();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickItemShopPreview")))
	{
		OpenItemShop();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("Flick4v4LoadoutPreview")))
	{
		SelectMatchVariant(EFlickMatchVariant::Classic);
		OpenLoadout();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickLoadoutPreview")))
	{
		OpenLoadout();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickMatchResultPreview")))
	{
		StartSelectedMatch();
		if (AFlickGameState* FlickGameState = GetFlickGameState())
		{
			for (int32 WinIndex = 0; WinIndex < CurrentRoundsToWin; ++WinIndex)
			{
				FlickGameState->CompleteRound(EFlickMatchOutcome::Player1Wins);
				if (WinIndex + 1 < CurrentRoundsToWin)
				{
					FlickGameState->AdvanceToNextRound();
				}
			}
		}
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickBobTableStickTest")))
	{
		SelectMatchVariant(EFlickMatchVariant::Bob);
		StartSelectedMatch();
		FTimerHandle BobTableLaunchTimer;
		GetWorldTimerManager().SetTimer(BobTableLaunchTimer, [this]()
		{
			AFlickGameState* FlickGameState = GetFlickGameState();
			AFlickPiece* BobStriker = GetBobStriker(FlickGameState ? FlickGameState->CurrentTeam : EFlickTeam::None);
			if (!BobStriker)
			{
				UE_LOG(LogFlick, Error, TEXT("BOB_TABLE_STICK_TEST FAIL: no active striker"));
				QuitGame();
				return;
			}

			AFlickPiece* SideTestPiece = nullptr;
			AFlickPiece* VerticalLineTestPiece = nullptr;
			AFlickPiece* HorizontalLineTestPiece = nullptr;
			for (AFlickPiece* Piece : Pieces)
			{
				if (Piece && Piece->IsActive() && !Piece->IsBobStriker())
				{
					if (!SideTestPiece) SideTestPiece = Piece;
					else if (!VerticalLineTestPiece) VerticalLineTestPiece = Piece;
					else if (!HorizontalLineTestPiece)
					{
						HorizontalLineTestPiece = Piece;
						break;
					}
				}
			}
			if (!SideTestPiece || !VerticalLineTestPiece || !HorizontalLineTestPiece)
			{
				UE_LOG(LogFlick, Error, TEXT("BOB_TABLE_STICK_TEST FAIL: insufficient physics test pucks"));
				QuitGame();
				return;
			}
			SideTestPiece->SetActorLocationAndRotation(
				FVector(300.0f, 300.0f, ArenaSurfaceZ + SideTestPiece->GetPieceRadius() + 1.0f),
				FRotator(90.0f, 0.0f, 0.0f),
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			if (UPrimitiveComponent* SideTestPrimitive = Cast<UPrimitiveComponent>(SideTestPiece->GetRootComponent()))
			{
				SideTestPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
				SideTestPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
			const TWeakObjectPtr<AFlickPiece> SideTestPieceWeak(SideTestPiece);
			const float FlatTestZ = ArenaSurfaceZ + PieceThickness * 0.5f + 1.0f;
			VerticalLineTestPiece->SetActorLocationAndRotation(
				FVector(-300.0f, -350.0f, FlatTestZ),
				FRotator::ZeroRotator,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			HorizontalLineTestPiece->SetActorLocationAndRotation(
				FVector(-350.0f, 300.0f, FlatTestZ),
				FRotator::ZeroRotator,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			for (AFlickPiece* LineTestPiece : {VerticalLineTestPiece, HorizontalLineTestPiece})
			{
				if (UPrimitiveComponent* LineTestPrimitive = Cast<UPrimitiveComponent>(LineTestPiece->GetRootComponent()))
				{
					LineTestPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
					LineTestPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				}
			}
			const TWeakObjectPtr<AFlickPiece> VerticalLineTestPieceWeak(VerticalLineTestPiece);
			const TWeakObjectPtr<AFlickPiece> HorizontalLineTestPieceWeak(HorizontalLineTestPiece);

			const TSharedRef<float> MaximumObservedZ = MakeShared<float>(BobStriker->GetActorLocation().Z);
			const TSharedRef<float> MaximumLineDeviation = MakeShared<float>(0.0f);
			const TSharedRef<bool> bSampleLineTrajectory = MakeShared<bool>(true);
			const TSharedRef<FTimerHandle> SampleTimer = MakeShared<FTimerHandle>();
			GetWorldTimerManager().SetTimer(*SampleTimer, [
				this,
				MaximumObservedZ,
				MaximumLineDeviation,
				bSampleLineTrajectory,
				SideTestPieceWeak,
				VerticalLineTestPieceWeak,
				HorizontalLineTestPieceWeak]()
			{
				for (const AFlickPiece* Piece : Pieces)
				{
					if (Piece && Piece->IsActive() && Piece != SideTestPieceWeak.Get())
					{
						*MaximumObservedZ = FMath::Max(*MaximumObservedZ, static_cast<float>(Piece->GetActorLocation().Z));
					}
				}
				if (*bSampleLineTrajectory)
				{
					if (const AFlickPiece* VerticalPiece = VerticalLineTestPieceWeak.Get())
					{
						*MaximumLineDeviation = FMath::Max(
							*MaximumLineDeviation,
							FMath::Abs(static_cast<float>(VerticalPiece->GetActorLocation().X) + 300.0f));
					}
					if (const AFlickPiece* HorizontalPiece = HorizontalLineTestPieceWeak.Get())
					{
						*MaximumLineDeviation = FMath::Max(
							*MaximumLineDeviation,
							FMath::Abs(static_cast<float>(HorizontalPiece->GetActorLocation().Y) - 300.0f));
					}
				}
			}, 0.01f, true);
			FTimerHandle StopLineSamplingTimer;
			GetWorldTimerManager().SetTimer(StopLineSamplingTimer, [bSampleLineTrajectory]()
			{
				*bSampleLineTrajectory = false;
			}, 0.55f, false);

			FVector Direction = -BobStriker->GetActorLocation();
			Direction.Z = 0.0f;
			TryLaunchPiece(BobStriker, Direction.GetSafeNormal(), 1.0f);
			VerticalLineTestPiece->Launch(FVector(0.0f, 1.0f, 0.0f), 0.45f, MaxLaunchSpeed);
			HorizontalLineTestPiece->Launch(FVector(1.0f, 0.0f, 0.0f), 0.45f, MaxLaunchSpeed);

			FTimerHandle BobTableVerificationTimer;
			GetWorldTimerManager().SetTimer(BobTableVerificationTimer, [
				this,
				MaximumObservedZ,
				MaximumLineDeviation,
				SampleTimer,
				SideTestPieceWeak]()
			{
				GetWorldTimerManager().ClearTimer(*SampleTimer);
				const float MaximumAllowedZ = ArenaSurfaceZ + PieceThickness * 0.5f + 12.0f;
				const AFlickPiece* TestedSidePiece = SideTestPieceWeak.Get();
				const float UprightAlignment = TestedSidePiece
					? FMath::Abs(FVector::DotProduct(TestedSidePiece->GetActorUpVector(), FVector::UpVector))
					: 0.0f;
				const bool bPassed = *MaximumObservedZ <= MaximumAllowedZ
					&& UprightAlignment >= 0.9f
					&& *MaximumLineDeviation <= 1.0f;
				if (bPassed)
				{
					UE_LOG(
						LogFlick,
						Log,
						TEXT("BOB_TABLE_STICK_TEST PASS: maxZ=%.1f allowedZ=%.1f upright=%.3f lineDeviation=%.3f"),
						*MaximumObservedZ,
						MaximumAllowedZ,
						UprightAlignment,
						*MaximumLineDeviation);
				}
				else
				{
					UE_LOG(
						LogFlick,
						Error,
						TEXT("BOB_TABLE_STICK_TEST FAIL: maxZ=%.1f allowedZ=%.1f upright=%.3f lineDeviation=%.3f"),
						*MaximumObservedZ,
						MaximumAllowedZ,
						UprightAlignment,
						*MaximumLineDeviation);
				}
				QuitGame();
			}, 2.0f, false);
		}, 0.6f, false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickBobPocketTest")))
	{
		SelectMatchVariant(EFlickMatchVariant::Bob);
		StartSelectedMatch();
		FTimerHandle BobPocketTimer;
		GetWorldTimerManager().SetTimer(BobPocketTimer, [this]()
		{
			AFlickGameState* FlickGameState = GetFlickGameState();
			AFlickPiece* BobStriker = GetBobStriker(FlickGameState ? FlickGameState->CurrentTeam : EFlickTeam::None);
			if (!BobStriker || !BobArenaActor || !FlickGameState)
			{
				return;
			}
			AFlickPiece* ObjectivePiece = nullptr;
			for (AFlickPiece* Piece : Pieces)
			{
				if (Piece && Piece->IsActive() && !Piece->IsBobStriker()
					&& Piece->GetTeam() == FlickGameState->CurrentTeam)
				{
					ObjectivePiece = Piece;
					break;
				}
			}
			FVector Direction = -BobStriker->GetActorLocation();
			Direction.Z = 0.0f;
			if (ObjectivePiece && TryLaunchPiece(BobStriker, Direction.GetSafeNormal(), 0.18f))
			{
				FVector PocketLocation = BobArenaActor->GetPocketWorldLocation(0);
				PocketLocation.Z = ArenaSurfaceZ + PieceThickness * 0.5f + 3.0f;
				ObjectivePiece->SetActorLocation(PocketLocation, false, nullptr, ETeleportType::TeleportPhysics);

				const TWeakObjectPtr<AFlickPiece> PocketTestPiece(ObjectivePiece);
				FTimerHandle BobPocketVerificationTimer;
				GetWorldTimerManager().SetTimer(BobPocketVerificationTimer, [PocketTestPiece]()
				{
					const AFlickPiece* TestedPiece = PocketTestPiece.Get();
					const bool bPassed = TestedPiece && TestedPiece->IsEliminated();
					if (bPassed)
					{
						UE_LOG(
							LogFlick,
							Log,
							TEXT("BOB_POCKET_PHYSICS_TEST PASS: droppedZ=%.1f"),
							TestedPiece->GetActorLocation().Z);
					}
					else
					{
						UE_LOG(
							LogFlick,
							Error,
							TEXT("BOB_POCKET_PHYSICS_TEST FAIL: droppedZ=%.1f"),
							TestedPiece ? TestedPiece->GetActorLocation().Z : -1.0f);
					}
				}, 1.0f, false);
			}
		}, 0.6f, false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickBobPreview")))
	{
		SelectMatchVariant(EFlickMatchVariant::Bob);
		StartSelectedMatch();
		FTimerHandle BobTimer;
		GetWorldTimerManager().SetTimer(BobTimer, [this]()
		{
			AFlickGameState* FlickGameState = GetFlickGameState();
			AFlickPiece* BobStriker = GetBobStriker(FlickGameState ? FlickGameState->CurrentTeam : EFlickTeam::None);
			if (BobStriker && BobStriker->IsActive())
			{
				FVector Direction = -BobStriker->GetActorLocation();
				Direction.Z = 0.0f;
				TryLaunchPiece(BobStriker, Direction.GetSafeNormal(), 0.82f);
			}
		}, 0.6f, false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickBobPersistenceTest")))
	{
		SelectMatchVariant(EFlickMatchVariant::Bob);
		StartSelectedMatch();
		AFlickGameState* FlickGameState = GetFlickGameState();
		AFlickPiece* BobStriker = GetBobStriker(FlickGameState ? FlickGameState->CurrentTeam : EFlickTeam::None);
		const FVector StrikerStart = BobStriker ? BobStriker->GetActorLocation() : FVector::ZeroVector;
		if (!BobStriker || !TryLaunchPiece(BobStriker, FVector(1.0f, 0.0f, 0.0f), 0.08f))
		{
			UE_LOG(LogFlick, Error, TEXT("BOB_STRIKER_PERSISTENCE_TEST FAIL: launch setup failed"));
			QuitGame();
		}
		else
		{
			FTimerHandle PersistenceTimer;
			GetWorldTimerManager().SetTimer(PersistenceTimer, [this, StrikerStart]()
			{
				const AFlickGameState* State = GetFlickGameState();
				const AFlickPiece* Player1Striker = GetBobStriker(EFlickTeam::Player1);
				const float TravelDistance = Player1Striker
					? FVector2D::Distance(
						FVector2D(StrikerStart.X, StrikerStart.Y),
						FVector2D(Player1Striker->GetActorLocation().X, Player1Striker->GetActorLocation().Y))
					: 0.0f;
				const bool bPassed = State
					&& State->MatchPhase == EFlickMatchPhase::Aiming
					&& State->CurrentTeam == EFlickTeam::Player2
					&& Player1Striker
					&& Player1Striker->IsActive()
					&& Player1Striker->GetTeam() == EFlickTeam::Player1
					&& TravelDistance > PieceRadius;
				if (bPassed)
				{
					UE_LOG(
						LogFlick,
						Log,
						TEXT("BOB_STRIKER_PERSISTENCE_TEST PASS: Player 1 striker remained %.1f cm from its start after turn handoff"),
						TravelDistance);
				}
				else
				{
					UE_LOG(
						LogFlick,
						Error,
						TEXT("BOB_STRIKER_PERSISTENCE_TEST FAIL: phase=%d team=%d distance=%.1f"),
						State ? static_cast<int32>(State->MatchPhase) : -1,
						State ? static_cast<int32>(State->CurrentTeam) : -1,
						TravelDistance);
				}
				QuitGame();
			}, 14.0f, false);
		}
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickTestArenaPreview"))
		|| FParse::Param(FCommandLine::Get(), TEXT("FlickTestArenaSettingsPreview")))
	{
		bTestArenaMode = true;
		SelectedMatchVariant = EFlickMatchVariant::Classic;
		int32 TestPlayersPerTeam = MatchmakingPlayersPerTeam;
		FParse::Value(FCommandLine::Get(), TEXT("FlickPlayersPerTeam="), TestPlayersPerTeam);
		MatchmakingPlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(TestPlayersPerTeam);
		BeginTrainingActivity(true);
		if (FParse::Param(FCommandLine::Get(), TEXT("FlickTestArenaSettingsPreview")))
		{
			// Keep preview time advancing for the capture timer, as with FlickPausePreview.
			SettingsReturnScreen = EFlickFrontendScreen::Paused;
			FrontendScreen = EFlickFrontendScreen::Settings;
			SetCameraForFrontend();
		}
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("Flick4v4Preview")))
	{
		SelectMatchVariant(EFlickMatchVariant::Classic);
		StartSelectedMatch();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickTrainingPreview")))
	{
		SelectMatchVariant(EFlickMatchVariant::Classic);
		StartTrainingMode();
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickPausePreview")))
	{
		SelectMatchVariant(EFlickMatchVariant::Classic);
		StartSelectedMatch();
		FrontendScreen = EFlickFrontendScreen::Paused;
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickArchetypePreview")))
	{
		SelectMatchVariant(EFlickMatchVariant::Classic);
		StartSelectedMatch();
		DestroyPieces();
		constexpr float PreviewSpacing = 230.0f;
		for (int32 Index = 0; Index < FlickPieceArchetypeRules::ArchetypeCount; ++Index)
		{
			const EFlickPieceArchetype Archetype = static_cast<EFlickPieceArchetype>(Index);
			const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(Archetype);
			const int32 Column = Index % 3;
			const int32 Row = Index / 3;
			const float SpawnZ = ArenaSurfaceZ
				+ PieceThickness * ArchetypeRules.ThicknessMultiplier * 0.5f
				+ 3.0f;
			SpawnPiece(
				Index % 2 == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2,
				Index + 1,
				FVector((Column - 1) * PreviewSpacing, (Row - 1) * PreviewSpacing, SpawnZ),
				Archetype);
		}
		if (AFlickGameState* FlickGameState = GetFlickGameState())
		{
			UpdateGameStateCounts();
			FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
		}
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickAutoKickoff")))
	{
		StartSelectedMatch();
		FTimerHandle KickoffTimer;
		GetWorldTimerManager().SetTimer(KickoffTimer, [this]()
		{
			const int32 RequiredShots = FlickTeamRules::GetSimultaneousKickoffShotCount(CurrentPlayersPerTeam);
			for (int32 ShotIndex = 0; ShotIndex < RequiredShots; ++ShotIndex)
			{
				const AFlickGameState* FlickGameState = GetFlickGameState();
				if (!FlickGameState || FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning)
				{
					break;
				}
				AFlickPiece* PlanningPiece = nullptr;
				for (AFlickPiece* Piece : Pieces)
				{
					if (Piece && Piece->IsActive()
						&& Piece->GetTeam() == FlickGameState->CurrentTeam
						&& Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot)
					{
						PlanningPiece = Piece;
						break;
					}
				}
				if (!PlanningPiece)
				{
					break;
				}
				FVector Direction = -PlanningPiece->GetActorLocation();
				Direction.Z = 0.0f;
				TryLaunchPiece(PlanningPiece, Direction.GetSafeNormal(), 0.72f);
			}
		}, 0.6f, false);
	}
	else if (FParse::Param(FCommandLine::Get(), TEXT("FlickAutoStart")))
	{
		StartSelectedMatch();
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("FlickCaptureFrame")))
	{
		const TWeakObjectPtr<AFlickGameMode> WeakThis(this);
		FTimerHandle CaptureTimer;
		GetWorldTimerManager().SetTimer(CaptureTimer, [WeakThis]()
		{
			AFlickGameMode* FlickGameMode = WeakThis.Get();
			if (!FlickGameMode)
			{
				return;
			}
			if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(FlickGameMode, 0))
			{
				PlayerController->ConsoleCommand(TEXT("Shot showui"));
			}

			FTimerHandle QuitTimer;
			FlickGameMode->GetWorldTimerManager().SetTimer(QuitTimer, [WeakThis]()
			{
				if (AFlickGameMode* ValidGameMode = WeakThis.Get())
				{
					ValidGameMode->QuitGame();
				}
			}, 1.0f, false);
		}, 2.0f, false);
	}

	int32 CameraViewPreview = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("FlickCameraView="), CameraViewPreview) && CameraPawn)
	{
		CameraPawn->SetGameplayViewIndex(CameraViewPreview, true);
	}
#endif

	TryStartNetworkMatch();

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickNetworkSmokeTest")))
	{
		FTimerHandle NetworkSmokeTimer;
		GetWorldTimerManager().SetTimer(NetworkSmokeTimer, [this]()
		{
			const AFlickGameState* State = GetFlickGameState();
			const bool bPassed = State
				&& State->Player1ShotsTaken > 0
				&& State->Player2ShotsTaken > 0
				&& State->MatchPhase != EFlickMatchPhase::KickoffPlanning;
			UE_LOG(
				LogFlick,
				Log,
				TEXT("NETWORK_MULTIPLAYER_SMOKE_TEST %s: P1Shots=%d P2Shots=%d Phase=%d"),
				bPassed ? TEXT("PASS") : TEXT("FAIL"),
				State ? State->Player1ShotsTaken : -1,
				State ? State->Player2ShotsTaken : -1,
				State ? static_cast<int32>(State->MatchPhase) : -1);
			ReturnToNetworkLobby();
			const AFlickGameState* LobbyState = GetFlickGameState();
			const AFlickPlayerState* Player1 = GetLobbyPlayer(EFlickTeam::Player1);
			const AFlickPlayerState* Player2 = GetLobbyPlayer(EFlickTeam::Player2);
			const bool bReturnedToLobby = LobbyState
				&& LobbyState->bNetworkLobbyActive
				&& LobbyState->MatchPhase == EFlickMatchPhase::WaitingToStart
				&& Player1 && !Player1->IsLobbyReady()
				&& Player2 && !Player2->IsLobbyReady();
			UE_LOG(LogFlick, Log, TEXT("NETWORK_RETURN_TO_LOBBY_SMOKE_TEST %s"), bReturnedToLobby ? TEXT("PASS") : TEXT("FAIL"));
			FTimerHandle QuitTimer;
			GetWorldTimerManager().SetTimer(QuitTimer, [this]() { QuitGame(); }, 1.0f, false);
		}, 24.0f, false);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickMatchmakingSmokeTest")))
	{
		FTimerHandle MatchmakingSmokeTimer;
		GetWorldTimerManager().SetTimer(MatchmakingSmokeTimer, [this]()
		{
			AFlickGameState* State = GetFlickGameState();
			const bool bStartedAuthoritatively = State
				&& State->bMatchmakingLobby
				&& !State->bNetworkLobbyActive
				&& !State->MatchId.IsEmpty()
				&& State->MatchPhase != EFlickMatchPhase::WaitingToStart;
			if (bStartedAuthoritatively)
			{
				State->CompleteMatchByForfeit(EFlickTeam::Player2);
				DispatchRankedMatchResults();
			}
			const bool bResultFinalized = State
				&& State->bMatchResultFinalized
				&& State->bMatchEndedByForfeit
				&& State->WinnerTeam == EFlickTeam::Player1
				&& State->FinalMatchOutcome == EFlickMatchOutcome::Player1Wins
				&& State->MatchCompletedUnixTime > 0;
			UE_LOG(
				LogFlick,
				Log,
				TEXT("MATCHMAKING_LIFECYCLE_SMOKE_TEST %s: started=%d result=%d id=%s"),
				bStartedAuthoritatively && bResultFinalized ? TEXT("PASS") : TEXT("FAIL"),
				bStartedAuthoritatively ? 1 : 0,
				bResultFinalized ? 1 : 0,
				State ? *State->MatchId : TEXT("NONE"));
			QuitGame();
		}, 18.0f, false);
	}
#endif
}

void AFlickGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	const FString CoordinatorAccountForAdmission = IncomingCoordinatorAccountIds.FindRef(NewPlayer);
	const FString CoordinatorReservationForAdmission = IncomingCoordinatorReservations.FindRef(NewPlayer);
	const bool bHasCoordinatorAdmission = !CoordinatorMatchId.IsEmpty()
		&& !CoordinatorAccountForAdmission.IsEmpty()
		&& !CoordinatorReservationForAdmission.IsEmpty();
	if (AFlickPlayerState* FlickPlayerState = NewPlayer ? NewPlayer->GetPlayerState<AFlickPlayerState>() : nullptr)
	{
		const FString IncomingPartyId = IncomingPartyIds.FindRef(NewPlayer);
		const int32 IncomingPartySlot = IncomingPartySlots.FindRef(NewPlayer);
		const int32 IncomingPartySize = IncomingPartySizes.FindRef(NewPlayer);
		const bool bIncomingPartyLeader = IncomingPartyLeaders.FindRef(NewPlayer);
		const int32 IncomingRankedRating = IncomingRankedRatings.Contains(NewPlayer)
			? IncomingRankedRatings.FindRef(NewPlayer)
			: RankedQueueRating;
		IncomingPartyIds.Remove(NewPlayer);
		IncomingPartySlots.Remove(NewPlayer);
		IncomingPartySizes.Remove(NewPlayer);
		IncomingPartyLeaders.Remove(NewPlayer);
		IncomingRankedRatings.Remove(NewPlayer);
		IncomingCoordinatorAccountIds.Remove(NewPlayer);
		IncomingCoordinatorReservations.Remove(NewPlayer);
		IncomingCoordinatorTeams.Remove(NewPlayer);
		IncomingCoordinatorSlots.Remove(NewPlayer);
		if (bRankedRequested)
		{
			PlayerRankedRatings.Add(NewPlayer, IncomingRankedRating);
		}
		EFlickTeam AssignedTeam = EFlickTeam::None;
		int32 AssignedPlayerSlot = INDEX_NONE;
		bool bPremadeAdmissionRejected = false;
		if (!bPartyRequested)
		{
			if (bHasCoordinatorAdmission)
			{
				AssignedTeam = EFlickTeam::None;
				AssignedPlayerSlot = INDEX_NONE;
			}
			else if (!IncomingPartyId.IsEmpty())
			{
				FlickPlayerState->SetPartyIdentity(
					IncomingPartyId,
					bIncomingPartyLeader,
					IncomingPartySlot);
				bPremadeAdmissionRejected = !FindPremadeTeamAndSlot(
					IncomingPartyId,
					IncomingPartySlot,
					IncomingPartySize,
					AssignedTeam,
					AssignedPlayerSlot);
			}
			else
			{
				const FString RejoiningPlayerId = GetPlayerOnlineId(FlickPlayerState);
				const EFlickTeam* ReservedTeam = DisconnectedPlayerTeams.Find(RejoiningPlayerId);
				const int32* ReservedSlot = DisconnectedPlayerSlots.Find(RejoiningPlayerId);
				if (ReservedTeam && ReservedSlot && !GetLobbyPlayer(*ReservedTeam, *ReservedSlot))
				{
					AssignedTeam = *ReservedTeam;
					AssignedPlayerSlot = *ReservedSlot;
					DisconnectedPlayerTeams.Remove(RejoiningPlayerId);
					DisconnectedPlayerSlots.Remove(RejoiningPlayerId);
					UE_LOG(LogFlick, Log, TEXT("Restored returning player to team %d slot %d"), GetTeamNumber(AssignedTeam), AssignedPlayerSlot + 1);
				}
				else
				{
					FindAvailableTeamAndSlot(NewPlayer, AssignedTeam, AssignedPlayerSlot);
				}
			}
		}
		FlickPlayerState->SetTeam(AssignedTeam);
		FlickPlayerState->SetTeamPlayerSlot(AssignedPlayerSlot);
		FlickPlayerState->SetLobbyReady(false);
		FString PlayerName;
		IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
		const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
		const FUniqueNetIdRepl& UniqueId = FlickPlayerState->GetUniqueId();
		if (Identity.IsValid() && UniqueId.IsValid())
		{
			PlayerName = Identity->GetPlayerNickname(*UniqueId.GetUniqueNetId());
		}
		if (PlayerName.IsEmpty())
		{
			PlayerName = bPartyRequested
				? FString::Printf(TEXT("Party Member %d"), GetPartyMemberCount())
				: AssignedTeam == EFlickTeam::None ? TEXT("Spectator")
				: FString::Printf(TEXT("Team %d Player %d"), GetTeamNumber(AssignedTeam), AssignedPlayerSlot + 1);
		}
		FlickPlayerState->SetPlayerName(PlayerName);
		if (bPartyRequested)
		{
			int32 PartySlot = 0;
			while (PartySlot < FlickMaximumPartyMembers && GetPartyMember(PartySlot))
			{
				++PartySlot;
			}
			bool bHasLeader = false;
			if (const AFlickGameState* State = GetFlickGameState())
			{
				for (const APlayerState* StatePlayer : State->PlayerArray)
				{
					const AFlickPlayerState* PartyPlayer = Cast<AFlickPlayerState>(StatePlayer);
					bHasLeader |= PartyPlayer && PartyPlayer != FlickPlayerState && PartyPlayer->IsPartyLeader();
				}
			}
			UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
			FString PartyId = Sessions ? Sessions->GetPersistentPartyId() : FString();
			if (PartyId.IsEmpty())
			{
				PartyId = ActivePartyId;
			}
			if (PartyId.IsEmpty() && !IncomingPartyId.IsEmpty())
			{
				PartyId = IncomingPartyId;
			}
			if (PartyId.IsEmpty())
			{
				PartyId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
			}
			ActivePartyId = PartyId;
			if (!IncomingPartyId.IsEmpty()
				&& IncomingPartyId == PartyId
				&& IncomingPartySlot >= 0
				&& IncomingPartySlot < FlickMaximumPartyMembers
				&& !GetPartyMember(IncomingPartySlot))
			{
				PartySlot = IncomingPartySlot;
			}
			const int32 NewPartySize = FMath::Clamp(GetPartyMemberCount() + 1, 1, FlickMaximumPartyMembers);
			FlickPlayerState->SetPartyIdentity(PartyId, !bHasLeader, FMath::Clamp(PartySlot, 0, FlickMaximumPartyMembers - 1));
			if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(NewPlayer))
			{
				FlickController->SetPersistentPartyIdentityFromServer(
					PartyId,
					FMath::Clamp(PartySlot, 0, FlickMaximumPartyMembers - 1),
					NewPartySize,
					!bHasLeader);
			}
			const FString RejoiningPlayerId = GetPlayerOnlineId(FlickPlayerState);
			if (bPrivateMatchActive)
			{
				if (const TArray<int32>* ReservedPrivateSlots = DisconnectedPrivateControlledSlots.Find(RejoiningPlayerId))
				{
					FlickPlayerState->SetPrivateControlledSlots(*ReservedPrivateSlots);
					DisconnectedPrivateControlledSlots.Remove(RejoiningPlayerId);
					RefreshPrivatePrimaryAssignments();
					UE_LOG(LogFlick, Log, TEXT("Restored private-match controls for returning player %s"), *PlayerName);
				}
			}
			if (!bHasLeader)
			{
				const FString LeaderId = GetPlayerOnlineId(FlickPlayerState);
				if (AFlickGameState* State = GetFlickGameState())
				{
					State->SetPartyState(true, LeaderId, FlickMaximumPartyMembers);
				}
			}
		}
		else if (!IncomingPartyId.IsEmpty())
		{
			if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(NewPlayer))
			{
				FlickController->SetPersistentPartyIdentityFromServer(
					IncomingPartyId,
					IncomingPartySlot,
					IncomingPartySize,
					bIncomingPartyLeader);
				if (bPremadeAdmissionRejected)
				{
					UE_LOG(
						LogFlick,
						Warning,
						TEXT("PARTY_MATCH_HANDOFF_REJECTED: no team has %d contiguous slot(s) for party %s"),
						IncomingPartySize,
						*IncomingPartyId);
					FlickController->BeginPartyRestoreFromServer(bIncomingPartyLeader);
				}
			}
		}
		if (bPartyRequested)
		{
			UE_LOG(LogFlick, Log, TEXT("Party member %s joined"), *PlayerName);
			SynchronizePartyState();
		}
		else
		{
			UE_LOG(
				LogFlick,
				Log,
				TEXT("Network player joined and was assigned to %s slot %d"),
				*GetTeamDisplayName(AssignedTeam),
				AssignedPlayerSlot + 1);
		}

		if (const AFlickGameState* State = GetFlickGameState())
		{
			for (APlayerState* StatePlayer : State->PlayerArray)
			{
				AFlickPlayerState* OtherState = Cast<AFlickPlayerState>(StatePlayer);
				if (!OtherState || OtherState == FlickPlayerState)
				{
					continue;
				}
				const FString NewId = GetPlayerOnlineId(FlickPlayerState);
				const FString OtherId = GetPlayerOnlineId(OtherState);
				if (AFlickPlayerController* OtherController = Cast<AFlickPlayerController>(OtherState->GetOwner()))
				{
					OtherController->RecordRecentPlayer(NewId, PlayerName);
				}
				if (AFlickPlayerController* JoiningController = Cast<AFlickPlayerController>(NewPlayer))
				{
					JoiningController->RecordRecentPlayer(OtherId, OtherState->GetPlayerName());
				}
			}
		}
	}
	if (!bPartyRequested)
	{
		if (bHasCoordinatorAdmission)
		{
			VerifyCoordinatorReservation(NewPlayer, CoordinatorAccountForAdmission, CoordinatorReservationForAdmission);
		}
		else if (bRankedRequested)
		{
			if (AFlickPlayerState* RankedPlayerState = NewPlayer
				? NewPlayer->GetPlayerState<AFlickPlayerState>()
				: nullptr)
			{
				RankedPlayerState->SetRankedIdentity(false, FlickRankRules::DefaultRating);
			}
			if (UFlickRankedBackendSubsystem* Backend = GetFlickRankedBackendSubsystem())
			{
				if (Backend->IsRemoteAuthorityEnabled())
				{
					if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(NewPlayer))
					{
						FlickController->RequestRankedAuthenticationFromServer(
							FString::Printf(TEXT("WebAPI:%s"), *Backend->GetSteamTicketAudience()));
					}
				}
				else
				{
					SubmitRankedAuthentication(NewPlayer, FString());
				}
			}
		}
		TryStartNetworkMatch();
	}
}

void AFlickGameMode::Logout(AController* Exiting)
{
	const AFlickPlayerState* ExitingState = Exiting ? Exiting->GetPlayerState<AFlickPlayerState>() : nullptr;
	const FString ExitingAccountId = GetPlayerOnlineId(ExitingState);
	const EFlickTeam ExitingTeam = ExitingState ? ExitingState->GetTeam() : EFlickTeam::None;
	const bool bMatchmakingForfeit = bNetworkMatchRequested
		&& bMatchmakingRequested
		&& bNetworkMatchStarted
		&& ExitingState
		&& ExitingState->GetTeam() != EFlickTeam::None
		&& (!GetFlickGameState() || !GetFlickGameState()->bMatchResultFinalized);
	const bool bCoordinatorReconnectGrace = bMatchmakingForfeit && !CoordinatorMatchId.IsEmpty();
	const bool bLeaderLeft = ExitingState && ExitingState->IsPartyLeader();
	if (bPrivateMatchActive && ExitingState && !ExitingAccountId.IsEmpty())
	{
		DisconnectedPrivateControlledSlots.Add(
			ExitingAccountId,
			ExitingState->GetPrivateControlledSlots());
	}
	const int32 RemainingPartyMembers = bPartyRequested
		? FMath::Max(0, GetPartyMemberCount() - (ExitingState ? 1 : 0))
		: 0;
	if (bPartyRequested)
	{
		if (bLeaderLeft)
		{
			AFlickPlayerState* NewLeader = nullptr;
			if (AFlickGameState* State = GetFlickGameState())
			{
				for (APlayerState* StatePlayer : State->PlayerArray)
				{
					AFlickPlayerState* Candidate = Cast<AFlickPlayerState>(StatePlayer);
					if (Candidate && Candidate != ExitingState
						&& (!NewLeader || Candidate->GetPartySlot() < NewLeader->GetPartySlot()))
					{
						NewLeader = Candidate;
					}
				}
				if (NewLeader)
				{
					NewLeader->SetPartyRole(true, NewLeader->GetPartySlot());
					State->SetPartyState(
						true,
						GetPlayerOnlineId(NewLeader),
						FlickMaximumPartyMembers);
				}
			}
		}
		Super::Logout(Exiting);
		SynchronizePartyState();
		if (bPrivateMatchActive && !ExitingAccountId.IsEmpty())
		{
			FTimerHandle PrivateReconnectTimer;
			GetWorldTimerManager().SetTimer(
				PrivateReconnectTimer,
				[this, ExitingAccountId]()
				{
					if (DisconnectedPrivateControlledSlots.Remove(ExitingAccountId) > 0)
					{
						UE_LOG(LogFlick, Log, TEXT("Private-match reconnect reservation expired for %s"), *ExitingAccountId);
					}
				},
				PrivateMatchReconnectGraceSeconds,
				false);
		}
		UE_LOG(LogFlick, Log, TEXT("Party member disconnected; %d member(s) remain"), RemainingPartyMembers);
		return;
	}
	if (bNetworkMatchRequested && ExitingState && ExitingState->GetTeam() != EFlickTeam::None)
	{
		const FString PlayerId = GetPlayerOnlineId(ExitingState);
		if (!PlayerId.IsEmpty())
		{
			DisconnectedPlayerTeams.Add(PlayerId, ExitingState->GetTeam());
			DisconnectedPlayerSlots.Add(PlayerId, ExitingState->GetTeamPlayerSlot());
		}
	}
	VerifiedCoordinatorPlayers.Remove(Cast<APlayerController>(Exiting));
	if (bMatchmakingForfeit && !bCoordinatorReconnectGrace)
	{
		FinalizeDisconnectedPlayerForfeit(ExitingState);
	}
	PlayerRankedRatings.Remove(Cast<APlayerController>(Exiting));
	Super::Logout(Exiting);
	if (bNetworkMatchRequested)
	{
		if (bCoordinatorReconnectGrace)
		{
			const float GraceSeconds = GetFlickMatchmakingCoordinatorSubsystem()
				? GetFlickMatchmakingCoordinatorSubsystem()->GetReconnectGraceSeconds()
				: 45.0f;
			FTimerHandle ReconnectGraceTimer;
			GetWorldTimerManager().SetTimer(
				ReconnectGraceTimer,
				[this, ExitingAccountId, ExitingTeam]()
				{
					if (!DisconnectedPlayerTeams.Contains(ExitingAccountId))
					{
						return;
					}
					FinalizeDisconnectedTeamForfeit(ExitingTeam);
					bNetworkMatchStarted = false;
					UE_LOG(
						LogFlick,
						Warning,
						TEXT("COORDINATOR_RECONNECT_EXPIRED: account=%s team=%d"),
						*ExitingAccountId,
						GetTeamNumber(ExitingTeam));
				},
				GraceSeconds,
				false);
			UE_LOG(
				LogFlick,
				Warning,
				TEXT("COORDINATOR_RECONNECT_WINDOW: account=%s seconds=%.0f"),
				*ExitingAccountId,
				GraceSeconds);
			return;
		}
		if (bMatchmakingForfeit)
		{
			bNetworkMatchStarted = false;
			FrontendScreen = EFlickFrontendScreen::Playing;
			UE_LOG(
				LogFlick,
				Warning,
				TEXT("%s_MATCH_FORFEIT: disconnected player's team forfeited the active match"),
				bRankedRequested ? TEXT("RANKED") : TEXT("CASUAL"));
			return;
		}
		bNetworkMatchStarted = false;
		FrontendScreen = EFlickFrontendScreen::NetworkLobby;
		ResetLobbyReadiness();
		if (AFlickGameState* FlickGameState = GetFlickGameState())
		{
			FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
			FlickGameState->SetNetworkLobbyState(true, SelectedMatchVariant);
		}
		if (bMatchmakingRequested && bMatchmakingLobbyLocked)
		{
			bMatchmakingLobbyLocked = false;
			MatchmakingQueueElapsed = 0.0f;
			MatchFoundConfirmationElapsed = 0.0f;
			if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
			{
				Sessions->ReopenMatchmakingLobby();
			}
			if (AFlickGameState* State = GetFlickGameState())
			{
				UpdateReplicatedMatchmakingState(false);
			}
		}
		UE_LOG(LogFlick, Warning, TEXT("Network session returned to the lobby because a player disconnected"));
	}
}

void AFlickGameMode::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateMainMenuPreview(DeltaSeconds);

	AFlickGameState* FlickGameState = GetFlickGameState();
	if (FrontendScreen == EFlickFrontendScreen::NetworkLobby
		&& bMatchmakingRequested
		&& FlickGameState
		&& !FlickGameState->bMatchmakingTimedOut
		&& (GetLobbyPlayerCount(EFlickTeam::Player1) < CurrentPlayersPerTeam
			|| GetLobbyPlayerCount(EFlickTeam::Player2) < CurrentPlayersPerTeam))
	{
		MatchmakingQueueElapsed += DeltaSeconds;
		if (MatchmakingQueueElapsed >= MatchmakingTimeoutSeconds)
		{
			UpdateReplicatedMatchmakingState(true);
			UE_LOG(
				LogFlick,
				Warning,
				TEXT("%s_QUEUE_TIMEOUT: no complete roster after %.0f seconds"),
				bRankedRequested ? TEXT("RANKED") : TEXT("CASUAL"),
				MatchmakingQueueElapsed);
		}
	}
	if (FrontendScreen == EFlickFrontendScreen::NetworkLobby
		&& bMatchmakingRequested
		&& bMatchmakingLobbyLocked
		&& !bNetworkMatchStarted
		&& FlickGameState
		&& !FlickGameState->bMatchmakingTimedOut)
	{
		MatchFoundConfirmationElapsed += DeltaSeconds;
		if (MatchFoundConfirmationElapsed >= MatchFoundConfirmationTimeoutSeconds)
		{
			UpdateReplicatedMatchmakingState(true);
			UE_LOG(LogFlick, Warning, TEXT("MATCH_CONFIRMATION_TIMEOUT: players did not ready within %.0f seconds"), MatchFoundConfirmationElapsed);
		}
	}
	UpdateInitialClassSelectionTimer(DeltaSeconds);
	UpdateNetworkClassSelectionTimer();
	UpdateRoundAdvanceTimer();
	UpdateShotClock();
	UpdateTrainingBot(DeltaSeconds);
	UpdateTutorial(DeltaSeconds);
	if (bCinematicReplayActive)
	{
		UpdateCinematicRoundReplay(DeltaSeconds);
		return;
	}
	if (FrontendScreen != EFlickFrontendScreen::Playing
		|| !FlickGameState
		|| FlickGameState->MatchPhase != EFlickMatchPhase::ResolvingPhysics)
	{
		return;
	}

	TrackTestArenaControlZones(DeltaSeconds);
	ResolutionElapsed += DeltaSeconds;
	if (IsBobMode())
	{
		UpdateBobPieceStability();
		UpdateBobPockets();
	}
	else
	{
		UpdateClassicPieceStability();
		UpdateEliminations();
	}
	CaptureRoundReplayFrame();

	if (AreActivePiecesSettled())
	{
		SettledElapsed += DeltaSeconds;
	}
	else
	{
		SettledElapsed = 0.0f;
	}

	if (SettledElapsed >= SettledDuration)
	{
		FinishPhysicsResolution(false);
		return;
	}

	if (ResolutionElapsed >= MaximumResolutionDuration && ApplyResolutionTimeoutCleanup())
	{
		FinishPhysicsResolution(true);
	}
}
