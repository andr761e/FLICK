#include "Game/FlickGameMode.h"

#include "Arena/FlickArena.h"
#include "Arena/FlickBobArena.h"
#include "Arena/FlickTestArena.h"
#include "Audio/FlickAudioDirector.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/FlickLog.h"
#include "Core/FlickBobRules.h"
#include "Core/FlickBotShotPlanner.h"
#include "Core/FlickAccoladeRules.h"
#include "Core/FlickMatchmakingRules.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickRankRules.h"
#include "Core/FlickTeamRules.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "EngineUtils.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Game/FlickGameState.h"
#include "Game/FlickGameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/GameSession.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Online/FlickMatchmakingCoordinatorSubsystem.h"
#include "Online/FlickSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "Ranking/FlickRankedBackendSubsystem.h"
#include "TimerManager.h"
#include "UI/FlickHUD.h"

namespace
{
	bool GCoordinatorAutoQueueConsumed = false;

	FString GetPlayerOnlineId(const APlayerState* PlayerState)
	{
		if (const AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(PlayerState);
			FlickPlayerState && !FlickPlayerState->GetVerifiedOnlineAccountId().IsEmpty())
		{
			return FlickPlayerState->GetVerifiedOnlineAccountId();
		}
		if (PlayerState)
		{
			const TSharedPtr<const FUniqueNetId> OnlineId = PlayerState->GetUniqueId().GetUniqueNetId();
			if (OnlineId.IsValid())
			{
				return OnlineId->ToString();
			}
		}
		return PlayerState ? PlayerState->GetPlayerName() : FString();
	}
}

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
		&& !GCoordinatorAutoQueueConsumed
		&& !bNetworkMatchRequested)
	{
		GCoordinatorAutoQueueConsumed = true;
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
		MatchmakingPlayersPerTeam = 1;
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
				PartyId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
			}
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

void AFlickGameMode::ResetTrainingBotThinking()
{
	TrainingBotThinkElapsed = 0.0f;
	bTrainingBotThinkingAnnounced = false;
}

void AFlickGameMode::ResetShotClock()
{
	bShotClockTrackingActive = false;
	ShotClockTrackedTeam = EFlickTeam::None;
	ShotClockTrackedPlayerSlot = INDEX_NONE;
	ShotClockTrackedPhase = EFlickMatchPhase::WaitingToStart;
	if (AFlickGameState* FlickGameState = GetFlickGameState(); FlickGameState && FlickGameState->bShotClockActive)
	{
		FlickGameState->SetShotClockState(false, ShotTimeLimit);
	}
}

float AFlickGameMode::GetShotTimeRemaining() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->GetShotClockTimeRemaining() : 0.0f;
}

float AFlickGameMode::GetShotClockFraction(const EFlickTeam Team) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->GetShotClockFraction(Team) : 1.0f;
}

float AFlickGameMode::GetRoundAdvanceTimeRemaining() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return FlickGameState ? FlickGameState->GetRoundAdvanceTimeRemaining() : 0.0f;
}

void AFlickGameMode::UpdateInitialClassSelectionTimer(const float DeltaSeconds)
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bShouldRun = FrontendScreen == EFlickFrontendScreen::ClassSelect
		&& !bClassSelectionForNextRound
		&& !(FlickGameState && FlickGameState->bNetworkClassSelectionActive);
	if (!bShouldRun)
	{
		bInitialClassSelectionTimerActive = false;
		return;
	}
	if (!bInitialClassSelectionTimerActive)
	{
		InitialClassSelectionTimeRemaining = FMath::Max(3.0f, InitialClassSelectionTimeLimit);
		bInitialClassSelectionTimerActive = true;
	}
	InitialClassSelectionTimeRemaining = FMath::Max(
		0.0f,
		InitialClassSelectionTimeRemaining - FMath::Max(0.0f, DeltaSeconds));
	if (InitialClassSelectionTimeRemaining <= 0.0f)
	{
		bInitialClassSelectionTimerActive = false;
		ConfirmClassSelection();
	}
}

void AFlickGameMode::UpdateNetworkClassSelectionTimer()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !FlickGameState->bNetworkClassSelectionActive)
	{
		return;
	}
	if (AreNetworkClassesConfirmed()
		|| FlickGameState->GetNetworkClassSelectionTimeRemaining() <= 0.0f)
	{
		FinalizeNetworkClassSelection();
	}
}

void AFlickGameMode::UpdateRoundAdvanceTimer()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !FlickGameState->bRoundAdvanceTimerActive)
	{
		return;
	}
	if (FrontendScreen != EFlickFrontendScreen::Playing
		|| FlickGameState->MatchPhase != EFlickMatchPhase::RoundOver
		|| FlickGameState->bSeriesComplete)
	{
		FlickGameState->SetRoundAdvanceTimerState(false, RoundAdvanceTimeLimit);
		return;
	}
	if (FlickGameState->GetRoundAdvanceTimeRemaining() <= 0.0f)
	{
		FlickGameState->SetRoundAdvanceTimerState(false, RoundAdvanceTimeLimit);
		StartNextRound();
	}
}

void AFlickGameMode::UpdateShotClock()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bClockPhase = FlickGameState
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning);
	if (!FlickGameState || IsFreePlayTraining()
		|| FrontendScreen != EFlickFrontendScreen::Playing || !bClockPhase)
	{
		ResetShotClock();
		return;
	}

	const bool bTurnChanged = !bShotClockTrackingActive
		|| ShotClockTrackedTeam != FlickGameState->CurrentTeam
		|| ShotClockTrackedPlayerSlot != FlickGameState->CurrentTeamPlayerSlot
		|| ShotClockTrackedPhase != FlickGameState->MatchPhase;
	if (bTurnChanged)
	{
		bShotClockTrackingActive = true;
		ShotClockTrackedTeam = FlickGameState->CurrentTeam;
		ShotClockTrackedPlayerSlot = FlickGameState->CurrentTeamPlayerSlot;
		ShotClockTrackedPhase = FlickGameState->MatchPhase;
		FlickGameState->SetShotClockState(true, ShotTimeLimit);
	}

	if (FlickGameState->GetShotClockTimeRemaining() <= 0.0f)
	{
		ExpireCurrentShot();
	}
}

void AFlickGameMode::ExpireCurrentShot()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsFreePlayTraining()
		|| FlickGameState->CurrentTeam == EFlickTeam::None
		|| (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning))
	{
		return;
	}

	ClearControllerAiming();
	const EFlickTeam ExpiredTeam = FlickGameState->CurrentTeam;
	const int32 ExpiredPlayerSlot = FlickGameState->CurrentTeamPlayerSlot;
	bool bTurnAdvanced = false;
	if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		AFlickPiece* WastedKickoffPiece = nullptr;
		for (AFlickPiece* Piece : Pieces)
		{
			if (Piece && IsValid(Piece) && Piece->IsActive()
				&& Piece->IsSelectableBy(ExpiredTeam)
				&& Piece->GetOwningPlayerSlot() == ExpiredPlayerSlot)
			{
				WastedKickoffPiece = Piece;
				break;
			}
		}
		if (WastedKickoffPiece)
		{
			FVector WastedDirection = -WastedKickoffPiece->GetActorLocation();
			WastedDirection.Z = 0.0f;
			if (!WastedDirection.Normalize())
			{
				WastedDirection = ExpiredTeam == EFlickTeam::Player1
					? FVector::ForwardVector : -FVector::ForwardVector;
			}
			bTurnAdvanced = LockKickoffShot(WastedKickoffPiece, WastedDirection, 0.0f, false);
		}
	}
	else
	{
		const EFlickTeam NextTeam = GetOpposingTeam(ExpiredTeam);
		FlickGameState->BeginShot(ExpiredTeam, INDEX_NONE, 0.0f);
		FlickGameState->RecordPlayerShot(ExpiredTeam, ExpiredPlayerSlot);
		AdvanceCompletedPlayerTurn(ExpiredTeam, ExpiredPlayerSlot);
		FlickGameState->SetCurrentTeam(NextTeam);
		ActivateNextPlayerForTeam(NextTeam);
		FlickGameState->AdvanceTurn();
		FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
		SetCameraViewForTeam(NextTeam);
		if (AudioDirector)
		{
			AudioDirector->PlayTurn(NextTeam);
		}
		bTurnAdvanced = true;
	}

	ResetShotClock();
	if (bTurnAdvanced)
	{
		ResetTrainingBotThinking();
		PushHudEvent(
			TEXT("SHOT CLOCK EXPIRED  |  TURN FORFEITED"),
			FLinearColor(1.0f, 0.34f, 0.12f, 1.0f),
			2.2f);
		UE_LOG(
			LogFlick,
			Log,
			TEXT("%s Player %d forfeited the shot because the 10-second clock expired"),
			*GetTeamDisplayName(ExpiredTeam),
			ExpiredPlayerSlot + 1);
	}
}

void AFlickGameMode::UpdateTrainingBot(const float DeltaSeconds)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bBotCanPlan = IsTrainingBotMatch()
		&& FrontendScreen == EFlickFrontendScreen::Playing
		&& FlickGameState
		&& FlickGameState->CurrentTeam == EFlickTeam::Player2
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning);
	if (!bBotCanPlan)
	{
		ResetTrainingBotThinking();
		return;
	}

	if (!bTrainingBotThinkingAnnounced)
	{
		bTrainingBotThinkingAnnounced = true;
		const float ThinkDelay = GetTrainingBotDifficultySettings().ThinkDelay;
		PushHudEvent(
			FString::Printf(TEXT("%s BOT IS LINING UP A SHOT"), *GetBotDifficultyLabel()),
			GetTeamColor(EFlickTeam::Player2),
			ThinkDelay);
	}
	const float ThinkDelay = GetTrainingBotDifficultySettings().ThinkDelay;
	TrainingBotThinkElapsed += DeltaSeconds;
	if (TrainingBotThinkElapsed < ThinkDelay)
	{
		return;
	}

	if (TryExecuteTrainingBotShot())
	{
		ResetTrainingBotThinking();
	}
	else
	{
		// A transient camera or state transition can briefly leave the board
		// unavailable. Retry shortly instead of stalling the match.
		TrainingBotThinkElapsed = FMath::Max(0.0f, ThinkDelay - 0.2f);
	}
}

const FFlickBotDifficultySettings& AFlickGameMode::GetTrainingBotDifficultySettings() const
{
	switch (GetBotDifficulty())
	{
	case EFlickBotDifficulty::Easy:
		return TrainingBotEasySettings;
	case EFlickBotDifficulty::Hard:
		return TrainingBotHardSettings;
	case EFlickBotDifficulty::Expert:
		return TrainingBotExpertSettings;
	case EFlickBotDifficulty::Normal:
	default:
		return TrainingBotNormalSettings;
	}
}

bool AFlickGameMode::TryExecuteTrainingBotShot()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!IsTrainingBotMatch() || !FlickGameState
		|| FlickGameState->CurrentTeam != EFlickTeam::Player2
		|| (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning))
	{
		return false;
	}

	TArray<FFlickBotPieceState> BotPieces;
	BotPieces.Reserve(Pieces.Num());
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}
		const FVector Location = Piece->GetActorLocation();
		BotPieces.Add({
			Piece->GetPieceId(),
			Piece->GetTeam(),
			FVector2D(Location.X, Location.Y),
			Piece->GetPieceRadius()});
	}

	const FFlickBotDifficultySettings& DifficultySettings = GetTrainingBotDifficultySettings();
	FFlickBotShotTuning BotTuning;
	BotTuning.ArenaRadius = ArenaRadius;
	BotTuning.MinimumPower = FMath::Min(DifficultySettings.MinimumPower, DifficultySettings.MaximumPower);
	BotTuning.MaximumPower = FMath::Max(DifficultySettings.MinimumPower, DifficultySettings.MaximumPower);
	BotTuning.AimErrorDegrees = FMath::Max(0.0f, DifficultySettings.AimErrorDegrees);
	BotTuning.PowerVariation = FMath::Max(0.0f, DifficultySettings.PowerVariation);
	BotTuning.DecisionNoise = FMath::Max(0.0f, DifficultySettings.DecisionNoise);
	BotTuning.DividerAwareness = FMath::Clamp(DifficultySettings.DividerAwareness, 0.0f, 1.0f);
	BotTuning.BankShotSkill = FMath::Clamp(DifficultySettings.BankShotSkill, 0.0f, 1.0f);
	if (bTestArenaMode && TestArenaActor && IsValid(TestArenaActor))
	{
		BotTuning.Dividers.Reserve(TestArenaActor->GetMechanismCount());
		for (int32 DividerIndex = 0; DividerIndex < TestArenaActor->GetMechanismCount(); ++DividerIndex)
		{
			const FVector DividerCenter = TestArenaActor->GetDividerWorldCenter(DividerIndex);
			const FVector SwitchCenter = TestArenaActor->GetSwitchWorldCenter(DividerIndex);
			FFlickBotDividerState& Divider = BotTuning.Dividers.AddDefaulted_GetRef();
			Divider.Center = FVector2D(DividerCenter.X, DividerCenter.Y);
			Divider.Tangent = TestArenaActor->GetDividerWorldTangent(DividerIndex);
			Divider.SwitchPosition = FVector2D(SwitchCenter.X, SwitchCenter.Y);
			Divider.HalfLength = TestArenaActor->GetDividerLength(DividerIndex) * 0.5f;
			Divider.HalfThickness = TestArenaActor->GetDividerCollisionThickness() * 0.5f;
			Divider.bRaised = TestArenaActor->IsDividerRaised(DividerIndex);
		}
	}
	FFlickBotShotPlan Plan;
	if (IsBobMode())
	{
		AFlickPiece* BobStriker = GetBobStriker(EFlickTeam::Player2);
		TArray<FVector2D> PocketPositions;
		if (BobArenaActor)
		{
			BotTuning.ArenaRadius = BobArenaActor->GetHalfExtent();
			PocketPositions.Reserve(4);
			for (int32 PocketIndex = 0; PocketIndex < 4; ++PocketIndex)
			{
				const FVector PocketLocation = BobArenaActor->GetPocketWorldLocation(PocketIndex);
				PocketPositions.Emplace(PocketLocation.X, PocketLocation.Y);
			}
		}
		Plan = FlickBotShotPlanner::PlanBobShot(
			BotPieces,
			EFlickTeam::Player2,
			BobStriker ? BobStriker->GetPieceId() : INDEX_NONE,
			PocketPositions,
			BotTuning,
			TrainingBotRandom);
	}
	else
	{
		Plan = FlickBotShotPlanner::PlanShot(
			BotPieces,
			EFlickTeam::Player2,
			BotTuning,
			TrainingBotRandom);
	}
	if (!Plan.IsValid())
	{
		UE_LOG(LogFlick, Warning, TEXT("Training bot could not find a valid shot"));
		return false;
	}

	AFlickPiece* Shooter = nullptr;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive()
			&& Piece->GetPieceId() == Plan.ShooterPieceId
			&& Piece->IsSelectableBy(EFlickTeam::Player2))
		{
			Shooter = Piece;
			break;
		}
	}
	if (!Shooter)
	{
		return false;
	}

	UE_LOG(
		LogFlick,
		Log,
		TEXT("TRAINING_BOT_SHOT: difficulty=%s shooter=%d target=%d power=%.2f score=%.2f"),
		*GetBotDifficultyLabel(),
		Plan.ShooterPieceId,
		Plan.TargetPieceId,
		Plan.NormalizedPower,
		Plan.Score);
	return ExecuteValidatedLaunch(
		Shooter,
		FVector(Plan.Direction.X, Plan.Direction.Y, 0.0f),
		Plan.NormalizedPower);
}

bool AFlickGameMode::CanSelectPiece(const AFlickPiece* Piece) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (IsTrainingBotMatch() && FlickGameState && FlickGameState->CurrentTeam == EFlickTeam::Player2)
	{
		return false;
	}
	const bool bCanUseEitherTrainingBobStriker = IsFreePlayTraining()
		&& IsBobMode()
		&& Piece
		&& Piece->IsBobStriker();
	const bool bCommonSelectionValid = FlickGameState
		&& FrontendScreen == EFlickFrontendScreen::Playing
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
		&& (!CameraPawn || !CameraPawn->IsGameplayViewTransitioning())
		&& Piece
		&& (bCanUseEitherTrainingBobStriker || Piece->IsSelectableBy(FlickGameState->CurrentTeam))
		&& (IsFreePlayTraining() || IsBobMode() || Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot);
	return bCommonSelectionValid && (!IsBobMode() || Piece->IsBobStriker());
}

bool AFlickGameMode::CanSelectPieceForController(
	const APlayerController* RequestingPlayer,
	const AFlickPiece* Piece) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	const bool bCanUseEitherTrainingBobStriker = IsFreePlayTraining()
		&& IsBobMode()
		&& Piece
		&& Piece->IsBobStriker();
	if (!RequestingPlayer || !Piece || !FlickGameState
		|| (IsTrainingBotMatch() && FlickGameState->CurrentTeam == EFlickTeam::Player2)
		|| FrontendScreen != EFlickFrontendScreen::Playing
		|| (FlickGameState->MatchPhase != EFlickMatchPhase::Aiming
			&& FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning)
		|| (!bCanUseEitherTrainingBobStriker && !Piece->IsSelectableBy(FlickGameState->CurrentTeam))
		|| (IsBobMode() && !Piece->IsBobStriker()))
	{
		return false;
	}

	if (GetNetMode() == NM_Standalone)
	{
		return IsFreePlayTraining() || IsBobMode()
			|| Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot;
	}

	const AFlickPlayerState* FlickPlayerState = RequestingPlayer->GetPlayerState<AFlickPlayerState>();
	if (bPrivateMatchActive)
	{
		return FlickPlayerState
			&& FlickPlayerState->ControlsPrivateSlot(
				FlickGameState->CurrentTeam,
				FlickGameState->CurrentTeamPlayerSlot)
			&& (IsBobMode() || Piece->GetOwningPlayerSlot() == FlickGameState->CurrentTeamPlayerSlot);
	}
	return FlickPlayerState
		&& FlickPlayerState->GetTeam() == FlickGameState->CurrentTeam
		&& FlickTeamRules::IsActivePlayerSlot(
			FlickPlayerState->GetTeamPlayerSlot(),
			FlickGameState->CurrentTeamPlayerSlot,
			CurrentPlayersPerTeam)
		&& (IsBobMode() || Piece->GetOwningPlayerSlot() == FlickPlayerState->GetTeamPlayerSlot());
}

bool AFlickGameMode::TryLaunchPiece(AFlickPiece* Piece, const FVector& Direction, const float NormalizedPower)
{
	if (!CanSelectPiece(Piece))
	{
		return false;
	}
	return ExecuteValidatedLaunch(Piece, Direction, NormalizedPower);
}

bool AFlickGameMode::ExecuteValidatedLaunch(
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!Piece || !FlickGameState)
	{
		return false;
	}
	if (FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		return LockKickoffShot(Piece, Direction, NormalizedPower);
	}

	const bool bFreePlayTraining = IsFreePlayTraining();
	const EFlickTeam ShootingTeam = bFreePlayTraining ? Piece->GetTeam() : FlickGameState->CurrentTeam;
	const int32 ShootingPlayerSlot = bFreePlayTraining
		? Piece->GetOwningPlayerSlot()
		: FlickGameState->CurrentTeamPlayerSlot;
	CaptureTestArenaControlZones();
	BeginResolutionTracking(ShootingTeam, false, Piece);
	FFlickReplayShotSetup& ReplayShot = ReplayShotSetups.AddDefaulted_GetRef();
	ReplayShot.Piece = Piece;
	ReplayShot.Direction = Direction.GetSafeNormal();
	ReplayShot.Power = FMath::Clamp(NormalizedPower, 0.0f, 1.0f);
	if (bFreePlayTraining)
	{
		FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
		FlickGameState->SetCurrentTeamPlayerSlot(ShootingPlayerSlot);
	}
	const FVector LaunchLocation = Piece->GetActorLocation() + FVector(0.0f, 0.0f, PieceThickness * 0.65f);
	FlickGameState->BeginShot(ShootingTeam, Piece->GetPieceId(), NormalizedPower);
	FlickGameState->RecordPlayerShot(ShootingTeam, ShootingPlayerSlot);
	AdvanceCompletedPlayerTurn(ShootingTeam, ShootingPlayerSlot);
	Piece->SetSelected(false);
	Piece->SetHovered(false);
	Piece->Launch(Direction, NormalizedPower, MaxLaunchSpeed);
	if (AudioDirector)
	{
		AudioDirector->PlayLaunch(Piece->GetArchetype(), NormalizedPower, LaunchLocation);
	}
	SpawnWorldFeedback(
		LaunchLocation,
		GetTeamColor(ShootingTeam),
		EFlickFeedbackKind::Launch,
		NormalizedPower,
		-Direction);
	AddCameraFeedback(FMath::Lerp(0.035f, 0.095f, NormalizedPower));
	AddControllerFeedback(FMath::Lerp(0.08f, 0.28f, NormalizedPower), 0.09f);

	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	FlickGameState->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);

	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s flicked Piece %d at power %.2f"),
		*GetTeamDisplayName(ShootingTeam),
		Piece->GetPieceId(),
		NormalizedPower);
	return true;
}

bool AFlickGameMode::TryLaunchPieceForController(
	APlayerController* RequestingPlayer,
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower)
{
	if (!CanSelectPieceForController(RequestingPlayer, Piece)
		|| Direction.ContainsNaN()
		|| Direction.SizeSquared2D() <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogFlick, Warning, TEXT("Rejected invalid or out-of-turn network flick request"));
		return false;
	}

	const bool bAccepted = ExecuteValidatedLaunch(
		Piece,
		Direction.GetSafeNormal2D(),
		FMath::Clamp(NormalizedPower, 0.0f, 1.0f));
	if (bAccepted)
	{
		UE_LOG(
			LogFlick,
			Log,
			TEXT("NETWORK_SHOT_ACCEPTED: %s Piece %d"),
			*GetTeamDisplayName(Piece->GetTeam()),
			Piece->GetPieceId());
	}
	return bAccepted;
}

bool AFlickGameMode::TryLaunchPieceByIdForController(
	APlayerController* RequestingPlayer,
	const int32 PieceId,
	const FVector& Direction,
	const float NormalizedPower)
{
	AFlickPiece* RequestedPiece = nullptr;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->GetPieceId() == PieceId)
		{
			RequestedPiece = Piece;
			break;
		}
	}
	return TryLaunchPieceForController(RequestingPlayer, RequestedPiece, Direction, NormalizedPower);
}

const AFlickPiece* AFlickGameMode::GetLockedKickoffPiece() const
{
	return LockedKickoffShots.IsEmpty() ? nullptr : LockedKickoffShots.Last().Piece.Get();
}

FVector AFlickGameMode::GetLockedKickoffDirection() const
{
	return LockedKickoffShots.IsEmpty() ? FVector::ZeroVector : LockedKickoffShots.Last().Direction;
}

float AFlickGameMode::GetLockedKickoffPower() const
{
	return LockedKickoffShots.IsEmpty() ? 0.0f : LockedKickoffShots.Last().Power;
}

bool AFlickGameMode::LockKickoffShot(
	AFlickPiece* Piece,
	const FVector& Direction,
	const float NormalizedPower,
	const bool bAnnounce)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!Piece || !FlickGameState || FlickGameState->MatchPhase != EFlickMatchPhase::KickoffPlanning)
	{
		return false;
	}

	FVector SafeDirection(Direction.X, Direction.Y, 0.0f);
	if (!SafeDirection.Normalize())
	{
		return false;
	}
	const float SafePower = FMath::Clamp(NormalizedPower, 0.0f, 1.0f);
	const EFlickTeam PlanningTeam = FlickGameState->CurrentTeam;
	const int32 PlanningPlayerSlot = FlickGameState->CurrentTeamPlayerSlot;
	if (Piece->GetTeam() != PlanningTeam
		|| Piece->GetOwningPlayerSlot() != PlanningPlayerSlot
		|| LockedKickoffShots.ContainsByPredicate([PlanningTeam, PlanningPlayerSlot](const FFlickLockedKickoffShot& Shot)
		{
			return Shot.Team == PlanningTeam && Shot.PlayerSlot == PlanningPlayerSlot;
		}))
	{
		return false;
	}

	FFlickLockedKickoffShot& LockedShot = LockedKickoffShots.AddDefaulted_GetRef();
	LockedShot.Piece = Piece;
	LockedShot.Direction = SafeDirection;
	LockedShot.Power = SafePower;
	LockedShot.Team = PlanningTeam;
	LockedShot.PlayerSlot = PlanningPlayerSlot;
	AdvanceCompletedPlayerTurn(PlanningTeam, PlanningPlayerSlot);
	Piece->SetSelected(false);
	Piece->SetHovered(false);
	Piece->SetKickoffLocked(true);

	const int32 RequiredShots = FlickTeamRules::GetSimultaneousKickoffShotCount(CurrentPlayersPerTeam);
	FlickGameState->SetKickoffProgress(LockedKickoffShots.Num(), RequiredShots);
	if (bAnnounce)
	{
		PushHudEvent(
			FString::Printf(
				TEXT("TEAM %d  /  PLAYER %d KICKOFF LOCKED  /  %d OF %d"),
				GetTeamNumber(PlanningTeam),
				PlanningPlayerSlot + 1,
				LockedKickoffShots.Num(),
				RequiredShots),
			GetTeamColor(PlanningTeam),
			1.6f);
	}
	if (AudioDirector)
	{
		AudioDirector->PlayUi(true);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s Player %d locked kickoff Piece %d at power %.2f (%d/%d)"),
		*GetTeamDisplayName(PlanningTeam),
		PlanningPlayerSlot + 1,
		Piece->GetPieceId(),
		SafePower,
		LockedKickoffShots.Num(),
		RequiredShots);

	if (LockedKickoffShots.Num() >= RequiredShots)
	{
		ReleaseKickoffShots();
		return true;
	}

	FlickGameState->SetCurrentTeam(GetOpposingTeam(PlanningTeam));
	ActivateNextPlayerForTeam(FlickGameState->CurrentTeam);
	SetCameraViewForTeam(FlickGameState->CurrentTeam);
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(FlickGameState->CurrentTeam);
	}
	return true;
}

void AFlickGameMode::ReleaseKickoffShots()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	const int32 RequiredShots = FlickTeamRules::GetSimultaneousKickoffShotCount(CurrentPlayersPerTeam);
	const bool bShotsValid = FlickGameState
		&& LockedKickoffShots.Num() == RequiredShots
		&& !LockedKickoffShots.ContainsByPredicate([](const FFlickLockedKickoffShot& Shot)
		{
			return !Shot.Piece.IsValid() || !Shot.Piece->IsActive();
		});
	if (!bShotsValid)
	{
		ResetKickoffState();
		BeginOpeningPhase();
		return;
	}

	CaptureTestArenaControlZones();
	BeginResolutionTracking(EFlickTeam::None, true, nullptr);
	float StrongestPower = 0.0f;
	for (const FFlickLockedKickoffShot& Shot : LockedKickoffShots)
	{
		AFlickPiece* Piece = Shot.Piece.Get();
		FFlickReplayShotSetup& ReplayShot = ReplayShotSetups.AddDefaulted_GetRef();
		ReplayShot.Piece = Piece;
		ReplayShot.Direction = Shot.Direction.GetSafeNormal();
		ReplayShot.Power = FMath::Clamp(Shot.Power, 0.0f, 1.0f);
		const FVector LaunchLocation = Piece->GetActorLocation() + FVector(0.0f, 0.0f, PieceThickness * 0.65f);
		FlickGameState->BeginShot(Shot.Team, Piece->GetPieceId(), Shot.Power);
		FlickGameState->RecordPlayerShot(Shot.Team, Shot.PlayerSlot);
		Piece->SetKickoffLocked(false);
		Piece->SetSelected(false);
		Piece->SetHovered(false);
		Piece->Launch(Shot.Direction, Shot.Power, MaxLaunchSpeed);
		if (AudioDirector)
		{
			AudioDirector->PlayLaunch(Piece->GetArchetype(), Shot.Power, LaunchLocation);
		}
		SpawnWorldFeedback(
			LaunchLocation,
			GetTeamColor(Shot.Team),
			EFlickFeedbackKind::Launch,
			Shot.Power,
			-Shot.Direction);
		StrongestPower = FMath::Max(StrongestPower, Shot.Power);
	}

	AddCameraFeedback(FMath::Lerp(0.07f, 0.16f, StrongestPower));
	AddControllerFeedback(FMath::Lerp(0.14f, 0.38f, StrongestPower), 0.14f);
	PushHudEvent(
		FString::Printf(TEXT("ALL %d KICKOFF SHOTS RELEASED"), RequiredShots),
		FLinearColor::White,
		1.9f);
	LockedKickoffShots.Reset();
	bResolvingKickoff = true;
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	FlickGameState->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);

	UE_LOG(LogFlick, Log, TEXT("Simultaneous kickoff released %d player shots"), RequiredShots);
}

void AFlickGameMode::NotifyPieceImpact(
	AFlickPiece* Piece,
	AFlickPiece* OtherPiece,
	const FVector& ImpactLocation,
	const float ImpactVelocityChange)
{
	if (!Piece || !OtherPiece || ImpactVelocityChange < 35.0f || !GetWorld())
	{
		return;
	}
	const int32 PieceId = Piece->GetPieceId();
	const int32 OtherPieceId = OtherPiece->GetPieceId();
	if (bTestArenaMode)
	{
		const AFlickGameState* State = GetFlickGameState();
		if (State && State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
		{
			if (!ResolutionFirstImpactTimes.Contains(PieceId))
			{
				ResolutionFirstImpactTimes.Add(PieceId, ResolutionElapsed);
			}
			if (!ResolutionFirstImpactTimes.Contains(OtherPieceId))
			{
				ResolutionFirstImpactTimes.Add(OtherPieceId, ResolutionElapsed);
			}
			if (Piece->GetTeam() != OtherPiece->GetTeam())
			{
				if (!ResolutionFirstOpponentImpactTimes.Contains(PieceId))
				{
					ResolutionFirstOpponentImpactTimes.Add(PieceId, ResolutionElapsed);
				}
				if (!ResolutionFirstOpponentImpactTimes.Contains(OtherPieceId))
				{
					ResolutionFirstOpponentImpactTimes.Add(OtherPieceId, ResolutionElapsed);
				}
			}
		}
	}
	if (PieceId == ResolutionShotPieceId)
	{
		ResolutionDirectContactPieceIds.Add(OtherPieceId);
	}
	else if (OtherPieceId == ResolutionShotPieceId)
	{
		ResolutionDirectContactPieceIds.Add(PieceId);
	}
	const int32* PieceDepth = ResolutionContactDepths.Find(PieceId);
	const int32* OtherDepth = ResolutionContactDepths.Find(OtherPieceId);
	if (PieceDepth && !OtherDepth)
	{
		ResolutionContactDepths.Add(OtherPieceId, *PieceDepth + 1);
	}
	else if (OtherDepth && !PieceDepth)
	{
		ResolutionContactDepths.Add(PieceId, *OtherDepth + 1);
	}
	if (ImpactVelocityChange < MinimumImpactFeedback)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastImpactFeedbackTime < 0.07f)
	{
		return;
	}
	LastImpactFeedbackTime = Now;

	const float Strength = FMath::Clamp(
		(ImpactVelocityChange - MinimumImpactFeedback)
		/ FMath::Max(StrongImpactFeedback - MinimumImpactFeedback, 1.0f),
		0.08f,
		1.0f);
	const FLinearColor ImpactColor = FLinearColor::LerpUsingHSV(
		GetTeamColor(Piece->GetTeam()),
		FLinearColor(1.0f, 0.82f, 0.2f, 1.0f),
		Strength * 0.7f);

	SpawnWorldFeedback(
		FVector(ImpactLocation.X, ImpactLocation.Y, FMath::Max(ImpactLocation.Z, ArenaSurfaceZ + 12.0f)),
		ImpactColor,
		EFlickFeedbackKind::Impact,
		Strength);
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->RecordImpact(ImpactVelocityChange);
		FlickGameState->RecordPlayerImpact(
			FlickGameState->LastShotTeam,
			FlickGameState->GetLastShootingPlayerSlot(FlickGameState->LastShotTeam));
	}
	++ResolutionImpactCount;
	AddCameraFeedback(FMath::Lerp(0.055f, 0.24f, Strength));
	AddControllerFeedback(FMath::Lerp(0.1f, 0.72f, Strength), FMath::Lerp(0.055f, 0.16f, Strength));
	if (AudioDirector)
	{
		AudioDirector->PlayImpact(
			Strength,
			Piece->PieceMassKg + OtherPiece->PieceMassKg,
			Piece->GetArchetype(),
			OtherPiece->GetArchetype(),
			ImpactLocation);
	}

	if (Strength >= 0.78f && Now - LastStrongImpactEventTime > 0.45f)
	{
		LastStrongImpactEventTime = Now;
		PushHudEvent(TEXT("HEAVY HIT"), ImpactColor, 1.1f);
	}
}

void AFlickGameMode::NotifyArenaImpact(
	AFlickPiece* Piece,
	UPrimitiveComponent* OtherComponent,
	const FVector& ImpactLocation,
	const float ImpactVelocityChange)
{
	if (!Piece || ImpactVelocityChange < 45.0f)
	{
		return;
	}
	if (bTestArenaMode && TestArenaActor && OtherComponent)
	{
		int32 DividerIndex = INDEX_NONE;
		if (TestArenaActor->FindDividerIndex(OtherComponent, DividerIndex))
		{
			ResolutionDividerContactPieceIds.Add(Piece->GetPieceId());
			const uint8 DividerBit = static_cast<uint8>(1 << DividerIndex);
			if ((ResolutionNewlyRaisedDividerMask & DividerBit) != 0)
			{
				ResolutionNewDividerContactPieceIds.Add(Piece->GetPieceId());
			}
		}
	}
	if (!AudioDirector)
	{
		return;
	}
	const float Strength = FMath::Clamp((ImpactVelocityChange - 45.0f) / 720.0f, 0.05f, 1.0f);
	AudioDirector->PlayRimImpact(
		Strength,
		Piece->PieceMassKg,
		Piece->GetArchetype(),
		ImpactLocation);
}

void AFlickGameMode::NotifyPieceSelected(const AFlickPiece* Piece) const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!Piece || !FlickGameState)
	{
		return;
	}

	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s selected Piece %d"),
		*GetTeamDisplayName(FlickGameState->CurrentTeam),
		Piece->GetPieceId());
}

void AFlickGameMode::RestartMatch()
{
	if (FrontendScreen == EFlickFrontendScreen::MainMenu
		|| FrontendScreen == EFlickFrontendScreen::ModeSelect)
	{
		return;
	}
	if (bNetworkMatchRequested && bRankedRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Ranked restart rejected; return to the lobby to register a new authoritative match."));
		return;
	}
	if (IsFreePlayTraining())
	{
		UGameplayStatics::SetGamePaused(this, false);
		FrontendScreen = EFlickFrontendScreen::Playing;
		ResetTrainingBoard();
		UE_LOG(LogFlick, Log, TEXT("Training board manually reset"));
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	FrontendScreen = EFlickFrontendScreen::Playing;
	ApplyPendingPlayerClasses();
	MenuPreviewElapsed = 0.0f;
	MenuPreviewVariant = SelectedMatchVariant;
	MenuPreviewPlayersPerTeam = CurrentPlayersPerTeam;
	SetCameraForFrontend();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	if (bNetworkMatchRequested)
	{
		if (AFlickGameState* State = GetFlickGameState())
		{
			State->BeginAuthoritativeMatch(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
		}
		BeginRankedMatchForPlayers();
	}
	if (AudioDirector && GetFlickGameState())
	{
		AudioDirector->PlayTurn(GetFlickGameState()->CurrentTeam);
	}
	UE_LOG(LogFlick, Log, TEXT("Match restarted"));
}

void AFlickGameMode::OpenModeSelect()
{
	if (FrontendScreen == EFlickFrontendScreen::MainMenu
		|| FrontendScreen == EFlickFrontendScreen::ModeSelect)
	{
		FrontendScreen = EFlickFrontendScreen::ModeSelect;
		if (ActiveMatchVariant != SelectedMatchVariant
			|| CurrentPlayersPerTeam != MatchmakingPlayersPerTeam)
		{
			ShowModePreview(SelectedMatchVariant, MatchmakingPlayersPerTeam);
		}
		MenuPreviewElapsed = 0.0f;
		MenuPreviewVariant = SelectedMatchVariant;
		MenuPreviewPlayersPerTeam = MatchmakingPlayersPerTeam;
		SetCameraForFrontend();
	}
}

void AFlickGameMode::CloseModeSelect()
{
	if (FrontendScreen == EFlickFrontendScreen::ModeSelect)
	{
		FrontendScreen = EFlickFrontendScreen::MainMenu;
		MenuPreviewElapsed = 0.0f;
		SetCameraForFrontend();
	}
}

void AFlickGameMode::OpenOnlineBrowser()
{
	if (bPartyRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Online browser is disabled while a pre-match party is active"));
		return;
	}
	if (FrontendScreen != EFlickFrontendScreen::ModeSelect
		&& FrontendScreen != EFlickFrontendScreen::OnlineBrowser)
	{
		return;
	}
	FrontendScreen = EFlickFrontendScreen::OnlineBrowser;
	SetCameraForFrontend();
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
	{
		Sessions->FindSessions();
	}
}

void AFlickGameMode::CloseOnlineBrowser()
{
	if (FrontendScreen == EFlickFrontendScreen::OnlineBrowser)
	{
		FrontendScreen = EFlickFrontendScreen::ModeSelect;
		SetCameraForFrontend();
	}
}

bool AFlickGameMode::DoesSelectedModeSupportLoadouts() const
{
	return FlickModeRules::Get(SelectedMatchVariant).bSupportsLoadouts;
}

int32 AFlickGameMode::GetLoadoutEditingPieceCount() const
{
	return FlickModeRules::Get(LoadoutEditingVariant).StartingPiecesPerTeam;
}

bool AFlickGameMode::CanChangeCameraView() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return CameraPawn
		&& FrontendScreen == EFlickFrontendScreen::Playing
		&& FlickGameState
		&& (FlickGameState->MatchPhase == EFlickMatchPhase::Aiming
			|| FlickGameState->MatchPhase == EFlickMatchPhase::KickoffPlanning
			|| FlickGameState->MatchPhase == EFlickMatchPhase::ResolvingPhysics);
}

bool AFlickGameMode::IsAimGuideEnabled() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return !FlickGameInstance || FlickGameInstance->IsAimGuideEnabled();
}

bool AFlickGameMode::AreImpactEffectsEnabled() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return !FlickGameInstance || FlickGameInstance->AreImpactEffectsEnabled();
}

bool AFlickGameMode::IsControlOverviewEnabled() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return !FlickGameInstance || FlickGameInstance->IsControlOverviewEnabled();
}

EFlickBotDifficulty AFlickGameMode::GetBotDifficulty() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetBotDifficulty() : EFlickBotDifficulty::Normal;
}

float AFlickGameMode::GetCameraShakeIntensity() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetCameraShakeIntensity() : 1.0f;
}

float AFlickGameMode::GetMasterVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetMasterVolume() : 1.0f;
}

float AFlickGameMode::GetEffectsVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetEffectsVolume() : 1.0f;
}

float AFlickGameMode::GetInterfaceVolume() const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance ? FlickGameInstance->GetInterfaceVolume() : 1.0f;
}

bool AFlickGameMode::IsVSyncEnabled() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return Settings && Settings->IsVSyncEnabled();
}

FString AFlickGameMode::GetWindowModeLabel() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return TEXT("WINDOWED");
	}

	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen:
		return TEXT("FULLSCREEN");
	case EWindowMode::WindowedFullscreen:
		return TEXT("BORDERLESS");
	case EWindowMode::Windowed:
	default:
		return TEXT("WINDOWED");
	}
}

FString AFlickGameMode::GetResolutionLabel() const
{
	const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	const FIntPoint Resolution = Settings ? Settings->GetScreenResolution() : FIntPoint(1280, 720);
	return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}

void AFlickGameMode::SelectMatchVariant(const EFlickMatchVariant Variant)
{
	const EFlickMatchVariant NormalizedVariant = NormalizeMatchVariant(Variant);
	const bool bLobbyModeChanged = IsNetworkLobby() && SelectedMatchVariant != NormalizedVariant;
	SelectedMatchVariant = NormalizedVariant;
	if (bLobbyModeChanged)
	{
		ResetLobbyReadiness();
	}
	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetSelectedMatchVariant(NormalizedVariant);
	}
	if (FrontendScreen == EFlickFrontendScreen::MainMenu
		|| FrontendScreen == EFlickFrontendScreen::ModeSelect
		|| FrontendScreen == EFlickFrontendScreen::NetworkLobby)
	{
		ShowModePreview(
			NormalizedVariant,
			NormalizedVariant == EFlickMatchVariant::Bob ? 1 : MatchmakingPlayersPerTeam);
	}
	else
	{
		ApplySelectedMatchConfiguration();
	}
	if (bNetworkMatchRequested)
	{
		if (bLobbyModeChanged)
		{
			if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem(); Sessions && Sessions->HasActiveSession())
			{
				Sessions->UpdateSessionVariant(NormalizedVariant);
			}
		}
		if (AFlickGameState* FlickGameState = GetFlickGameState())
		{
			FlickGameState->SetNetworkLobbyState(!bNetworkMatchStarted, NormalizedVariant);
		}
	}
}

EFlickPieceArchetype AFlickGameMode::GetLoadoutPiece(
	const EFlickTeam Team,
	const int32 SlotIndex) const
{
	if (FrontendScreen == EFlickFrontendScreen::Loadout)
	{
		return GetClassLoadoutPiece(LoadoutEditingPreset, SlotIndex);
	}
	if (IsBobMode() && FrontendScreen != EFlickFrontendScreen::Loadout)
	{
		return EFlickPieceArchetype::Standard;
	}
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance
		? FlickGameInstance->GetLoadoutPiece(Team, SlotIndex)
		: EFlickPieceArchetype::Standard;
}

EFlickLineupPreset AFlickGameMode::GetLoadoutPreset(const EFlickTeam Team) const
{
	if (FrontendScreen == EFlickFrontendScreen::Loadout)
	{
		return LoadoutEditingPreset;
	}
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance
		? FlickGameInstance->GetLoadoutPreset(Team)
		: EFlickLineupPreset::Balanced;
}

EFlickPieceArchetype AFlickGameMode::GetClassLoadoutPiece(
	const EFlickLineupPreset Preset,
	const int32 SlotIndex) const
{
	const UFlickGameInstance* FlickGameInstance = GetFlickGameInstance();
	return FlickGameInstance
		? FlickGameInstance->GetClassLoadoutPiece(Preset, SlotIndex)
		: FlickPieceArchetypeRules::GetPreset(Preset).IsValidIndex(SlotIndex)
			? FlickPieceArchetypeRules::GetPreset(Preset)[SlotIndex]
			: EFlickPieceArchetype::Standard;
}

EFlickLineupPreset AFlickGameMode::GetPlayerClass(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const bool bUseDraft) const
{
	const TArray<EFlickLineupPreset>* Classes = nullptr;
	if (bUseDraft)
	{
		Classes = Team == EFlickTeam::Player1 ? &Player1ClassDraft
			: Team == EFlickTeam::Player2 ? &Player2ClassDraft : nullptr;
	}
	else
	{
		Classes = Team == EFlickTeam::Player1 ? &Player1ActiveClasses
			: Team == EFlickTeam::Player2 ? &Player2ActiveClasses : nullptr;
	}
	if (Classes && Classes->IsValidIndex(PlayerSlot))
	{
		return (*Classes)[PlayerSlot];
	}

	const EFlickLineupPreset TeamPreset = GetLoadoutPreset(Team);
	return TeamPreset == EFlickLineupPreset::Custom
		? EFlickLineupPreset::Balanced
		: TeamPreset;
}

int32 AFlickGameMode::GetClassSelectionPlayersPerTeam() const
{
	return bClassSelectionForNextRound
		? CurrentPlayersPerTeam
		: FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
}

bool AFlickGameMode::CanOpenClassChange() const
{
	const AFlickGameState* State = GetFlickGameState();
	return FrontendScreen == EFlickFrontendScreen::Paused
		&& bPlayerClassesActiveForMatch
		&& (!bTrainingMode || bTrainingBotMatch)
		&& ActiveMatchVariant == EFlickMatchVariant::Classic
		&& (!State || !State->bSeriesComplete);
}

void AFlickGameMode::EnsureActivePlayerClasses(const int32 PlayersPerTeam)
{
	const int32 SafePlayerCount = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	const auto EnsureTeamClasses = [this, SafePlayerCount](
		TArray<EFlickLineupPreset>& Classes,
		const EFlickTeam Team)
	{
		EFlickLineupPreset DefaultClass = GetLoadoutPreset(Team);
		if (DefaultClass == EFlickLineupPreset::Custom)
		{
			DefaultClass = EFlickLineupPreset::Balanced;
		}
		while (Classes.Num() < SafePlayerCount)
		{
			Classes.Add(DefaultClass);
		}
		Classes.SetNum(SafePlayerCount);
	};
	EnsureTeamClasses(Player1ActiveClasses, EFlickTeam::Player1);
	EnsureTeamClasses(Player2ActiveClasses, EFlickTeam::Player2);
}

EFlickLineupPreset AFlickGameMode::PickRandomPlayerClass() const
{
	static constexpr EFlickLineupPreset PlayableClasses[] = {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control
	};
	return PlayableClasses[FMath::RandHelper(UE_ARRAY_COUNT(PlayableClasses))];
}

void AFlickGameMode::RandomizeOtherLocalPlayerClasses(const int32 PlayersPerTeam)
{
	const int32 SafePlayerCount = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	EnsureActivePlayerClasses(SafePlayerCount);

	// The local class picker owns blue Player 1. Every other simulated local
	// player receives one class for the match and keeps it between rounds.
	for (int32 PlayerSlot = 1; PlayerSlot < SafePlayerCount; ++PlayerSlot)
	{
		Player1ActiveClasses[PlayerSlot] = PickRandomPlayerClass();
	}
	for (int32 PlayerSlot = 0; PlayerSlot < SafePlayerCount; ++PlayerSlot)
	{
		Player2ActiveClasses[PlayerSlot] = PickRandomPlayerClass();
	}
}

void AFlickGameMode::PrepareClassSelection(const bool bForNextRound)
{
	bClassSelectionForNextRound = bForNextRound;
	bInitialClassSelectionTimerActive = !bForNextRound;
	InitialClassSelectionTimeRemaining = FMath::Max(3.0f, InitialClassSelectionTimeLimit);
	const int32 PlayerCount = bForNextRound
		? CurrentPlayersPerTeam
		: FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
	EnsureActivePlayerClasses(PlayerCount);

	if (bForNextRound && bHasPendingClassChanges
		&& Player1PendingClasses.Num() == PlayerCount
		&& Player2PendingClasses.Num() == PlayerCount)
	{
		Player1ClassDraft = Player1PendingClasses;
		Player2ClassDraft = Player2PendingClasses;
	}
	else
	{
		Player1ClassDraft = Player1ActiveClasses;
		Player2ClassDraft = Player2ActiveClasses;
	}

	ClassSelectionReturnScreen = bForNextRound
		? EFlickFrontendScreen::Paused
		: EFlickFrontendScreen::ModeSelect;
	FrontendScreen = EFlickFrontendScreen::ClassSelect;
	SetCameraForFrontend();
	PlayMenuSound(false);
}

void AFlickGameMode::SelectPlayerClass(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const EFlickLineupPreset Preset)
{
	if (FrontendScreen != EFlickFrontendScreen::ClassSelect
		|| Team != EFlickTeam::Player1
		|| PlayerSlot != 0
		|| Preset == EFlickLineupPreset::Custom)
	{
		return;
	}

	TArray<EFlickLineupPreset>& Draft = Team == EFlickTeam::Player1
		? Player1ClassDraft
		: Player2ClassDraft;
	if (Draft.IsValidIndex(PlayerSlot))
	{
		Draft[PlayerSlot] = Preset;
		PlayMenuSound(false);
	}
}

void AFlickGameMode::ConfirmClassSelection()
{
	if (FrontendScreen != EFlickFrontendScreen::ClassSelect)
	{
		return;
	}
	bInitialClassSelectionTimerActive = false;

	if (bClassSelectionForNextRound)
	{
		// Changing class is a local blue Player 1 action. Preserve every random
		// teammate/opponent assignment made when the match began.
		Player1PendingClasses = Player1ActiveClasses;
		Player2PendingClasses = Player2ActiveClasses;
		if (Player1PendingClasses.IsValidIndex(0) && Player1ClassDraft.IsValidIndex(0))
		{
			Player1PendingClasses[0] = Player1ClassDraft[0];
		}
		bHasPendingClassChanges = true;
		FrontendScreen = EFlickFrontendScreen::Paused;
		SetCameraForFrontend();
		PlayMenuSound(true);
		return;
	}

	const int32 PlayerCount = FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
	EnsureActivePlayerClasses(PlayerCount);
	if (Player1ActiveClasses.IsValidIndex(0) && Player1ClassDraft.IsValidIndex(0))
	{
		Player1ActiveClasses[0] = Player1ClassDraft[0];
	}
	RandomizeOtherLocalPlayerClasses(PlayerCount);
	bHasPendingClassChanges = false;
	Player1PendingClasses.Reset();
	Player2PendingClasses.Reset();
	PlayMenuSound(true);
	if (bClassSelectionStartsTrainingBotMatch)
	{
		bClassSelectionStartsTrainingBotMatch = false;
		BeginTrainingActivity(true);
	}
	else
	{
		BeginSelectedMatch();
	}
}

void AFlickGameMode::BeginNetworkClassSelection()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !bNetworkMatchRequested || !bNetworkMatchStarted)
	{
		return;
	}

	EnsureActivePlayerClasses(CurrentPlayersPerTeam);
	bPlayerClassesActiveForMatch = true;
	bClassSelectionForNextRound = false;
	bInitialClassSelectionTimerActive = false;
	for (APlayerState* BasePlayerState : FlickGameState->PlayerArray)
	{
		AFlickPlayerState* PlayerState = Cast<AFlickPlayerState>(BasePlayerState);
		if (!PlayerState || PlayerState->GetTeam() == EFlickTeam::None)
		{
			continue;
		}
		const EFlickLineupPreset ExistingClass = GetPlayerClass(
			PlayerState->GetTeam(),
			PlayerState->GetTeamPlayerSlot());
		PlayerState->ResetNetworkClassSelection(ExistingClass);
	}

	FrontendScreen = EFlickFrontendScreen::ClassSelect;
	FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	FlickGameState->SetNetworkClassSelectionState(true, InitialClassSelectionTimeLimit);
	SetCameraForFrontend();
	UE_LOG(LogFlick, Log, TEXT("NETWORK_CLASS_SELECTION_STARTED: %.0f second limit"), InitialClassSelectionTimeLimit);
}

void AFlickGameMode::SetNetworkPlayerClass(
	APlayerController* RequestingPlayer,
	const EFlickLineupPreset Preset)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	AFlickPlayerState* PlayerState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (!FlickGameState || !FlickGameState->bNetworkClassSelectionActive
		|| !PlayerState || PlayerState->GetTeam() == EFlickTeam::None
		|| Preset == EFlickLineupPreset::Custom)
	{
		return;
	}
	PlayerState->SetNetworkSelectedClass(Preset);
}

void AFlickGameMode::ConfirmNetworkPlayerClass(APlayerController* RequestingPlayer)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	AFlickPlayerState* PlayerState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (!FlickGameState || !FlickGameState->bNetworkClassSelectionActive
		|| !PlayerState || PlayerState->GetTeam() == EFlickTeam::None)
	{
		return;
	}
	PlayerState->SetNetworkClassConfirmed(true);
	if (AreNetworkClassesConfirmed())
	{
		FinalizeNetworkClassSelection();
	}
}

bool AFlickGameMode::AreNetworkClassesConfirmed() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !FlickGameState->bNetworkClassSelectionActive)
	{
		return false;
	}
	int32 ParticipantCount = 0;
	for (const APlayerState* BasePlayerState : FlickGameState->PlayerArray)
	{
		const AFlickPlayerState* PlayerState = Cast<AFlickPlayerState>(BasePlayerState);
		if (!PlayerState || PlayerState->GetTeam() == EFlickTeam::None)
		{
			continue;
		}
		++ParticipantCount;
		if (!PlayerState->IsNetworkClassConfirmed())
		{
			return false;
		}
	}
	return ParticipantCount == CurrentPlayersPerTeam * 2;
}

void AFlickGameMode::FinalizeNetworkClassSelection()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || !FlickGameState->bNetworkClassSelectionActive)
	{
		return;
	}

	EnsureActivePlayerClasses(CurrentPlayersPerTeam);
	for (APlayerState* BasePlayerState : FlickGameState->PlayerArray)
	{
		AFlickPlayerState* PlayerState = Cast<AFlickPlayerState>(BasePlayerState);
		if (!PlayerState || PlayerState->GetTeam() == EFlickTeam::None)
		{
			continue;
		}
		TArray<EFlickLineupPreset>& TeamClasses = PlayerState->GetTeam() == EFlickTeam::Player1
			? Player1ActiveClasses
			: Player2ActiveClasses;
		if (TeamClasses.IsValidIndex(PlayerState->GetTeamPlayerSlot()))
		{
			TeamClasses[PlayerState->GetTeamPlayerSlot()] = PlayerState->GetNetworkSelectedClass();
		}
		PlayerState->SetNetworkClassConfirmed(true);
	}

	FlickGameState->SetNetworkClassSelectionState(false, InitialClassSelectionTimeLimit);
	FrontendScreen = EFlickFrontendScreen::Playing;
	BeginSelectedMatch();
	BeginRankedMatchForPlayers();
	UE_LOG(LogFlick, Log, TEXT("NETWORK_CLASS_SELECTION_COMPLETE: starting authoritative match"));
}

void AFlickGameMode::CancelClassSelection()
{
	if (FrontendScreen != EFlickFrontendScreen::ClassSelect)
	{
		return;
	}
	bInitialClassSelectionTimerActive = false;
	bClassSelectionStartsTrainingBotMatch = false;
	bTestArenaMode = false;
	FrontendScreen = ClassSelectionReturnScreen;
	SetCameraForFrontend();
	PlayMenuSound(false);
}

void AFlickGameMode::OpenClassChange()
{
	if (CanOpenClassChange())
	{
		PrepareClassSelection(true);
	}
}

void AFlickGameMode::ApplyPendingPlayerClasses()
{
	if (!bHasPendingClassChanges)
	{
		return;
	}
	Player1ActiveClasses = Player1PendingClasses;
	Player2ActiveClasses = Player2PendingClasses;
	Player1PendingClasses.Reset();
	Player2PendingClasses.Reset();
	bHasPendingClassChanges = false;
	PushHudEvent(TEXT("CLASS CHANGES APPLIED"), FLinearColor(0.08f, 0.82f, 1.0f, 1.0f), 2.0f);
}

EFlickPieceArchetype AFlickGameMode::GetPlayerClassPiece(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const int32 PieceSlot) const
{
	if (!bPlayerClassesActiveForMatch)
	{
		return GetLoadoutPiece(Team, PieceSlot);
	}
	return GetClassLoadoutPiece(GetPlayerClass(Team, PlayerSlot), PieceSlot);
}

void AFlickGameMode::OpenLoadout()
{
	if (FrontendScreen != EFlickFrontendScreen::MainMenu
		&& FrontendScreen != EFlickFrontendScreen::ModeSelect)
	{
		return;
	}
	if (DoesSelectedModeSupportLoadouts())
	{
		LoadoutEditingVariant = SelectedMatchVariant;
	}
	LoadoutEditingPreset = EFlickLineupPreset::Balanced;
	LoadoutReturnScreen = FrontendScreen;

	if (DoesSelectedModeSupportLoadouts() && ActiveMatchVariant != SelectedMatchVariant)
	{
		ShowModePreview(
			SelectedMatchVariant,
			SelectedMatchVariant == EFlickMatchVariant::Bob ? 1 : MatchmakingPlayersPerTeam);
	}
	else if (DoesSelectedModeSupportLoadouts())
	{
		ApplySelectedMatchConfiguration();
	}
	FrontendScreen = EFlickFrontendScreen::Loadout;
	SetCameraForFrontend();
}

void AFlickGameMode::CloseLoadout()
{
	if (FrontendScreen == EFlickFrontendScreen::Loadout)
	{
		FrontendScreen = LoadoutReturnScreen == EFlickFrontendScreen::ModeSelect
			? EFlickFrontendScreen::ModeSelect
			: EFlickFrontendScreen::MainMenu;
		MenuPreviewElapsed = 0.0f;
		SetCameraForFrontend();
	}
}

void AFlickGameMode::SelectLoadoutEditingVariant(const EFlickMatchVariant Variant)
{
	if (FrontendScreen != EFlickFrontendScreen::Loadout
		|| !FlickModeRules::Get(Variant).bSupportsLoadouts)
	{
		return;
	}

	LoadoutEditingVariant = Variant;
	PlayMenuSound(false);
}

void AFlickGameMode::SelectLoadoutEditingPreset(const EFlickLineupPreset Preset)
{
	if (FrontendScreen != EFlickFrontendScreen::Loadout
		|| Preset == EFlickLineupPreset::Custom)
	{
		return;
	}

	LoadoutEditingPreset = Preset;
	PlayMenuSound(false);
}

void AFlickGameMode::OpenItemShop()
{
	if (FrontendScreen != EFlickFrontendScreen::MainMenu)
	{
		return;
	}
	FrontendScreen = EFlickFrontendScreen::ItemShop;
	SetCameraForFrontend();
}

void AFlickGameMode::CloseItemShop()
{
	if (FrontendScreen == EFlickFrontendScreen::ItemShop)
	{
		FrontendScreen = EFlickFrontendScreen::MainMenu;
		MenuPreviewElapsed = 0.0f;
		SetCameraForFrontend();
	}
}

void AFlickGameMode::OpenProfile()
{
	if (FrontendScreen != EFlickFrontendScreen::MainMenu)
	{
		return;
	}
	FrontendScreen = EFlickFrontendScreen::Profile;
	SetCameraForFrontend();
	PlayMenuSound(false);
}

void AFlickGameMode::CloseProfile()
{
	if (FrontendScreen == EFlickFrontendScreen::Profile)
	{
		FrontendScreen = EFlickFrontendScreen::MainMenu;
		MenuPreviewElapsed = 0.0f;
		SetCameraForFrontend();
		PlayMenuSound(false);
	}
}

void AFlickGameMode::CycleLoadoutPiece(
	const EFlickTeam Team,
	const int32 SlotIndex,
	const int32 Direction)
{
	if (FrontendScreen != EFlickFrontendScreen::Loadout
		|| Team == EFlickTeam::None
		|| SlotIndex < 0
		|| SlotIndex >= GetLoadoutEditingPieceCount())
	{
		return;
	}

	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		const EFlickPieceArchetype CurrentPiece = FlickGameInstance->GetClassLoadoutPiece(
			LoadoutEditingPreset,
			SlotIndex);
		FlickGameInstance->SetClassLoadoutPiece(
			LoadoutEditingPreset,
			SlotIndex,
			FlickPieceArchetypeRules::Cycle(CurrentPiece, Direction));
	}
}

void AFlickGameMode::SetLoadoutPiece(
	const EFlickTeam Team,
	const int32 SlotIndex,
	const EFlickPieceArchetype Archetype)
{
	if (FrontendScreen != EFlickFrontendScreen::Loadout
		|| Team == EFlickTeam::None
		|| SlotIndex < 0
		|| SlotIndex >= GetLoadoutEditingPieceCount())
	{
		return;
	}

	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->SetClassLoadoutPiece(LoadoutEditingPreset, SlotIndex, Archetype);
	}
}

void AFlickGameMode::ApplyLoadoutPreset(
	const EFlickTeam Team,
	const EFlickLineupPreset Preset)
{
	if (FrontendScreen != EFlickFrontendScreen::Loadout
		|| Team == EFlickTeam::None
		|| Preset == EFlickLineupPreset::Custom)
	{
		return;
	}

	if (UFlickGameInstance* FlickGameInstance = GetFlickGameInstance())
	{
		FlickGameInstance->ApplyLoadoutPreset(Team, Preset);
	}
}

void AFlickGameMode::StartSelectedMatch()
{
	if (bPartyRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Party match start rejected: team matchmaking is the next multiplayer milestone"));
		return;
	}
	if (bNetworkMatchRequested && !bNetworkMatchStarted)
	{
		UE_LOG(LogFlick, Log, TEXT("Network lobby is waiting for Player 2 before starting"));
		return;
	}
	if (GetNetMode() == NM_Standalone
		&& FrontendScreen == EFlickFrontendScreen::ModeSelect
		&& SelectedMatchVariant == EFlickMatchVariant::Classic)
	{
		PrepareClassSelection(false);
		return;
	}

	BeginSelectedMatch();
}

void AFlickGameMode::BeginSelectedMatch()
{
	bClassSelectionStartsTrainingBotMatch = false;
	bTestArenaMode = false;
	if (SelectedMatchVariant == EFlickMatchVariant::Classic)
	{
		EnsureActivePlayerClasses(MatchmakingPlayersPerTeam);
		bPlayerClassesActiveForMatch = true;
	}
	else
	{
		bPlayerClassesActiveForMatch = false;
		bHasPendingClassChanges = false;
	}

	bTrainingMode = false;
	bTrainingEditMode = false;
	bTrainingBotMatch = false;
	ResetTrainingBotThinking();
	UGameplayStatics::SetGamePaused(this, false);
	FrontendScreen = EFlickFrontendScreen::Playing;
	CurrentPlayersPerTeam = SelectedMatchVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
	MenuPreviewElapsed = 0.0f;
	MenuPreviewVariant = SelectedMatchVariant;
	MenuPreviewPlayersPerTeam = CurrentPlayersPerTeam;
	SetCameraForFrontend();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	if (AudioDirector && GetFlickGameState())
	{
		AudioDirector->PlayTurn(GetFlickGameState()->CurrentTeam);
	}
	UE_LOG(LogFlick, Log, TEXT("Started %s mode"), *GetMatchVariantName(SelectedMatchVariant));
}

void AFlickGameMode::StartTrainingMode()
{
	bTestArenaMode = false;
	BeginTrainingActivity(false);
}

void AFlickGameMode::StartTrainingBotMatch()
{
	bTestArenaMode = false;
	if (GetNetMode() != NM_Standalone
		|| bNetworkMatchRequested
		|| bPartyRequested
		|| bMatchmakingRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Bot training class selection rejected because the current session is not offline"));
		return;
	}

	MatchmakingPlayersPerTeam = 1;
	if (NormalizeMatchVariant(SelectedMatchVariant) == EFlickMatchVariant::Bob)
	{
		bClassSelectionStartsTrainingBotMatch = false;
		BeginTrainingActivity(true);
		return;
	}

	SelectedMatchVariant = EFlickMatchVariant::Classic;
	bClassSelectionStartsTrainingBotMatch = true;
	PrepareClassSelection(false);
}

void AFlickGameMode::StartTestArenaBotMatch()
{
	if (GetNetMode() != NM_Standalone
		|| bNetworkMatchRequested
		|| bPartyRequested
		|| bMatchmakingRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Test arena bot match rejected because the current session is not offline"));
		return;
	}

	bTestArenaMode = true;
	MatchmakingPlayersPerTeam = 1;
	SelectedMatchVariant = EFlickMatchVariant::Classic;
	bClassSelectionStartsTrainingBotMatch = true;
	PrepareClassSelection(false);
}

void AFlickGameMode::BeginTrainingActivity(const bool bAgainstBot)
{
	if (GetNetMode() != NM_Standalone
		|| bNetworkMatchRequested
		|| bPartyRequested
		|| bMatchmakingRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Training start rejected because the current session is not offline"));
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	bTrainingMode = true;
	bTrainingEditMode = false;
	bTrainingBotMatch = bAgainstBot;
	bClassSelectionStartsTrainingBotMatch = false;
	ResetTrainingBotThinking();
	ResetShotClock();
	TrainingBotRandom.Initialize(FMath::Rand());
	TrainingPlacementTeam = EFlickTeam::Player1;
	TrainingPlacementArchetype = EFlickPieceArchetype::Standard;
	TrainingResetSnapshot.Reset();
	bHasTrainingResetSnapshot = false;
	if (bAgainstBot)
	{
		SelectedMatchVariant = NormalizeMatchVariant(SelectedMatchVariant);
		MatchmakingPlayersPerTeam = 1;
		if (SelectedMatchVariant == EFlickMatchVariant::Classic)
		{
			EnsureActivePlayerClasses(1);
			RandomizeOtherLocalPlayerClasses(1);
			bPlayerClassesActiveForMatch = true;
		}
		else
		{
			bPlayerClassesActiveForMatch = false;
		}
	}
	else
	{
		bPlayerClassesActiveForMatch = false;
	}
	bHasPendingClassChanges = false;
	bRankedQueueSelected = false;
	FrontendScreen = EFlickFrontendScreen::Playing;
	CurrentPlayersPerTeam = SelectedMatchVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
	MenuPreviewElapsed = 0.0f;
	MenuPreviewVariant = SelectedMatchVariant;
	MenuPreviewPlayersPerTeam = CurrentPlayersPerTeam;
	SetCameraForFrontend();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	if (!bAgainstBot)
	{
		CaptureTrainingResetSnapshot();
	}
	if (!bTestArenaMode)
	{
		PushHudEvent(bAgainstBot ? TEXT("TRAINING  |  PLAY AGAINST BOT") : TEXT("TRAINING  |  FREE PLAY"),
			bAgainstBot ? GetTeamColor(EFlickTeam::Player2) : FLinearColor(0.2f, 0.78f, 0.5f, 1.0f), 2.6f);
	}

	if (AudioDirector)
	{
		AudioDirector->PlayTurn(EFlickTeam::Player1);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("Started offline %s in %s %dv%d"),
		bTestArenaMode ? TEXT("test arena bot match") : bAgainstBot ? TEXT("bot training") : TEXT("free-play training"),
		*GetMatchVariantName(SelectedMatchVariant),
		CurrentPlayersPerTeam,
		CurrentPlayersPerTeam);
}

bool AFlickGameMode::CanEditTrainingBoard() const
{
	const AFlickGameState* FlickGameState = GetFlickGameState();
	return IsFreePlayTraining()
		&& GetNetMode() == NM_Standalone
		&& FrontendScreen == EFlickFrontendScreen::Playing
		&& FlickGameState
		&& FlickGameState->MatchPhase == EFlickMatchPhase::Aiming;
}

void AFlickGameMode::ToggleTrainingEditMode()
{
	if (!IsFreePlayTraining())
	{
		return;
	}
	if (!CanEditTrainingBoard())
	{
		PushHudEvent(TEXT("WAIT FOR PUCKS TO SETTLE"), FLinearColor(1.0f, 0.72f, 0.12f, 1.0f), 1.5f);
		return;
	}

	ClearControllerAiming();
	const bool bEnteringEditor = !bTrainingEditMode;
	if (bEnteringEditor)
	{
		if (IsBobMode())
		{
			TrainingPlacementArchetype = EFlickPieceArchetype::Standard;
		}
		bTrainingEditMode = true;
		if (CameraPawn)
		{
			TrainingPreviousCameraElevation = CameraPawn->GetGameplayElevationAngle();
			CameraPawn->SetGameplayElevation(TrainingEditCameraElevation, true);
			CameraPawn->SetGameplayElevationLocked(true);
		}
	}
	else
	{
		CaptureTrainingResetSnapshot();
		bTrainingEditMode = false;
		if (CameraPawn)
		{
			CameraPawn->SetGameplayElevationLocked(false);
			CameraPawn->SetGameplayElevation(TrainingPreviousCameraElevation, true);
		}
	}
	PushHudEvent(
		bTrainingEditMode ? TEXT("BOARD EDITOR ENABLED") : TEXT("SETUP SAVED  |  SHOT MODE ENABLED"),
		bTrainingEditMode ? FLinearColor(0.18f, 0.9f, 1.0f, 1.0f) : FLinearColor(0.2f, 0.78f, 0.5f, 1.0f),
		1.4f);
}

void AFlickGameMode::CycleTrainingPlacementArchetype(const int32 Direction)
{
	if (!bTrainingEditMode || Direction == 0)
	{
		return;
	}
	if (IsBobMode())
	{
		TrainingPlacementArchetype = EFlickPieceArchetype::Standard;
		return;
	}

	TrainingPlacementArchetype = FlickPieceArchetypeRules::Cycle(
		TrainingPlacementArchetype,
		Direction);
	PushHudEvent(
		FString::Printf(TEXT("PUCK TYPE  |  %s"), *GetPieceArchetypeName(TrainingPlacementArchetype)),
		GetTeamColor(TrainingPlacementTeam),
		1.0f);
}

void AFlickGameMode::SetTrainingPlacementTeam(const EFlickTeam Team)
{
	if (!bTrainingMode || (Team != EFlickTeam::Player1 && Team != EFlickTeam::Player2))
	{
		return;
	}

	TrainingPlacementTeam = Team;
	PushHudEvent(
		Team == EFlickTeam::Player1 ? TEXT("PLACING YOUR PUCKS") : TEXT("PLACING TARGET PUCKS"),
		GetTeamColor(Team),
		1.2f);
}

int32 AFlickGameMode::AllocateTrainingPieceId() const
{
	int32 NextId = 1;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			NextId = FMath::Max(NextId, Piece->GetPieceId() + 1);
		}
	}
	return NextId;
}

bool AFlickGameMode::ResolveTrainingPlacement(
	const FVector& RequestedWorldLocation,
	const float Radius,
	const AFlickPiece* IgnoredPiece,
	FVector& OutWorldLocation) const
{
	if (!bTrainingMode || Radius <= 0.0f)
	{
		return false;
	}

	OutWorldLocation = RequestedWorldLocation;
	if (IsBobMode())
	{
		if (!BobArenaActor)
		{
			return false;
		}

		FVector LocalLocation = BobArenaActor->GetActorTransform().InverseTransformPosition(RequestedWorldLocation);
		const float MaximumCoordinate = FMath::Max(
			0.0f,
			BobArenaActor->GetHalfExtent() - Radius - TrainingArenaInset);
		LocalLocation.X = FMath::Clamp(LocalLocation.X, -MaximumCoordinate, MaximumCoordinate);
		LocalLocation.Y = FMath::Clamp(LocalLocation.Y, -MaximumCoordinate, MaximumCoordinate);
		OutWorldLocation = BobArenaActor->GetActorTransform().TransformPosition(LocalLocation);

		const float PocketClearance = BobArenaActor->GetPocketRadius() + Radius * 0.42f;
		for (int32 PocketIndex = 0; PocketIndex < 4; ++PocketIndex)
		{
			if (FVector::DistSquared2D(OutWorldLocation, BobArenaActor->GetPocketWorldLocation(PocketIndex))
				< FMath::Square(PocketClearance))
			{
				return false;
			}
		}
	}
	else
	{
		const FTransform ArenaTransform = ArenaActor
			? ArenaActor->GetActorTransform()
			: FTransform::Identity;
		FVector LocalLocation = ArenaTransform.InverseTransformPosition(RequestedWorldLocation);
		const float MaximumRadius = FMath::Max(
			0.0f,
			(ArenaActor ? ArenaActor->GetRadius() : ArenaRadius) - Radius - TrainingArenaInset);
		const FVector2D LocalPlanar(LocalLocation.X, LocalLocation.Y);
		if (LocalPlanar.SizeSquared() > FMath::Square(MaximumRadius))
		{
			const FVector2D ClampedPlanar = LocalPlanar.GetSafeNormal() * MaximumRadius;
			LocalLocation.X = ClampedPlanar.X;
			LocalLocation.Y = ClampedPlanar.Y;
		}
		OutWorldLocation = ArenaTransform.TransformPosition(LocalLocation);
	}

	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive() || Piece == IgnoredPiece)
		{
			continue;
		}
		const float MinimumSeparation = Radius + Piece->GetPieceRadius() + TrainingPlacementGap;
		if (FVector::DistSquared2D(OutWorldLocation, Piece->GetActorLocation())
			< FMath::Square(MinimumSeparation))
		{
			return false;
		}
	}
	return true;
}

bool AFlickGameMode::PlaceTrainingPuck(const FVector& WorldLocation)
{
	if (!bTrainingEditMode || !CanEditTrainingBoard())
	{
		return false;
	}

	int32 ActivePieceCount = 0;
	for (const AFlickPiece* Piece : Pieces)
	{
		ActivePieceCount += Piece && IsValid(Piece) && Piece->IsActive() ? 1 : 0;
	}
	if (ActivePieceCount >= MaxTrainingPieces)
	{
		PushHudEvent(TEXT("TRAINING PUCK LIMIT REACHED"), FLinearColor(1.0f, 0.72f, 0.12f, 1.0f), 1.5f);
		return false;
	}

	const EFlickPieceArchetype PlacementArchetype = IsBobMode()
		? EFlickPieceArchetype::Standard
		: TrainingPlacementArchetype;
	const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(PlacementArchetype);
	const float PlacementRadius = PieceRadius * ArchetypeRules.RadiusMultiplier;
	const float PlacementThickness = PieceThickness * ArchetypeRules.ThicknessMultiplier;
	FVector PlacementLocation;
	if (!ResolveTrainingPlacement(WorldLocation, PlacementRadius, nullptr, PlacementLocation))
	{
		return false;
	}
	if (FVector::DistSquared2D(PlacementLocation, WorldLocation) > 1.0f)
	{
		return false;
	}
	PlacementLocation.Z = ArenaSurfaceZ + PlacementThickness * 0.5f + 3.0f;

	AFlickPiece* Piece = SpawnPiece(
		TrainingPlacementTeam,
		AllocateTrainingPieceId(),
		PlacementLocation,
		PlacementArchetype,
		false,
		0,
		false);
	if (!Piece)
	{
		return false;
	}
	if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
	{
		Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Primitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Primitive->PutRigidBodyToSleep();
	}
	UpdateGameStateCounts();
	return true;
}

bool AFlickGameMode::BeginTrainingPuckMove(AFlickPiece* Piece)
{
	if (!bTrainingEditMode || !CanEditTrainingBoard() || !Piece || !IsValid(Piece)
		|| !Piece->IsActive() || !Pieces.Contains(Piece))
	{
		return false;
	}

	ClearControllerAiming();
	if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
	{
		Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Primitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Primitive->SetSimulatePhysics(false);
		Primitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	Piece->SetActorRotation(FRotator::ZeroRotator);
	Piece->SetSelected(true);
	return true;
}

bool AFlickGameMode::MoveTrainingPuck(AFlickPiece* Piece, const FVector& WorldLocation)
{
	if (!bTrainingEditMode || !CanEditTrainingBoard() || !Piece || !IsValid(Piece)
		|| !Piece->IsActive() || !Pieces.Contains(Piece))
	{
		return false;
	}

	FVector PlacementLocation;
	if (!ResolveTrainingPlacement(WorldLocation, Piece->GetPieceRadius(), Piece, PlacementLocation))
	{
		return false;
	}
	PlacementLocation.Z = ArenaSurfaceZ + Piece->GetPieceThickness() * 0.5f + 3.0f;
	Piece->SetActorLocationAndRotation(
		PlacementLocation,
		FRotator::ZeroRotator,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	return true;
}

void AFlickGameMode::FinishTrainingPuckMove(AFlickPiece* Piece)
{
	if (!Piece || !IsValid(Piece) || !Piece->IsActive() || !Pieces.Contains(Piece))
	{
		return;
	}

	Piece->SetSelected(false);
	Piece->SetActorRotation(FRotator::ZeroRotator, ETeleportType::TeleportPhysics);
	if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
	{
		Primitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Primitive->SetSimulatePhysics(true);
		Piece->ApplyPhysicsSettings();
		Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Primitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Primitive->PutRigidBodyToSleep();
	}
	Piece->ForceNetUpdate();
}

bool AFlickGameMode::RemoveTrainingPuck(AFlickPiece* Piece)
{
	if (!bTrainingEditMode || !CanEditTrainingBoard() || !Piece || !IsValid(Piece)
		|| !Pieces.Contains(Piece))
	{
		return false;
	}
	if (IsBobMode() && Piece->IsBobStriker())
	{
		PushHudEvent(TEXT("BOB STRIKERS CANNOT BE REMOVED"), GetTeamColor(Piece->GetTeam()), 1.2f);
		return false;
	}

	if (Piece == Player1BobStriker)
	{
		Player1BobStriker = nullptr;
	}
	if (Piece == Player2BobStriker)
	{
		Player2BobStriker = nullptr;
	}
	Pieces.Remove(Piece);
	Piece->Destroy();
	UpdateGameStateCounts();
	return true;
}

void AFlickGameMode::ClearTrainingPucks()
{
	if (!bTrainingEditMode || !CanEditTrainingBoard())
	{
		return;
	}

	ClearControllerAiming();
	if (IsBobMode())
	{
		ResetKickoffState();
		for (int32 PieceIndex = Pieces.Num() - 1; PieceIndex >= 0; --PieceIndex)
		{
			AFlickPiece* Piece = Pieces[PieceIndex];
			if (Piece && IsValid(Piece) && Piece->IsBobStriker())
			{
				if (Piece->GetTeam() == EFlickTeam::Player1)
				{
					Player1BobStriker = Piece;
				}
				else if (Piece->GetTeam() == EFlickTeam::Player2)
				{
					Player2BobStriker = Piece;
				}
				continue;
			}
			if (Piece && IsValid(Piece))
			{
				Piece->Destroy();
			}
			Pieces.RemoveAtSwap(PieceIndex, 1, EAllowShrinking::No);
		}
		bPlayer1BobStrikerPocketed = false;
		bPlayer2BobStrikerPocketed = false;
	}
	else
	{
		DestroyPieces();
	}
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
		FlickGameState->SetCurrentTeamPlayerSlot(0);
		FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	}
	UpdateGameStateCounts();
	PushHudEvent(
		IsBobMode() ? TEXT("TRAINING BOARD CLEARED  |  STRIKERS PRESERVED") : TEXT("TRAINING BOARD CLEARED"),
		FLinearColor(0.18f, 0.9f, 1.0f, 1.0f),
		1.5f);
}

void AFlickGameMode::HostLocalNetworkMatch()
{
	if (GetNetMode() != NM_Standalone || !GetWorld() || bPartyRequested)
	{
		return;
	}

	const FString CurrentMap = GetWorld()->GetOutermost()->GetName();
	UE_LOG(LogFlick, Log, TEXT("Opening localhost lobby for %s"), *GetMatchVariantName(SelectedMatchVariant));
	UGameplayStatics::OpenLevel(
		this,
		FName(*CurrentMap),
		true,
		TEXT("listen?FlickNetworkMatch"));
}

void AFlickGameMode::HostOnlineNetworkMatch()
{
	if (GetNetMode() != NM_Standalone || bPartyRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Online match hosting is disabled while a pre-match party is active"));
		return;
	}
	FrontendScreen = EFlickFrontendScreen::OnlineBrowser;
	SetCameraForFrontend();
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
	{
		Sessions->HostSession(SelectedMatchVariant, 2);
	}
}

void AFlickGameMode::CycleMatchmakingPlayersPerTeam(const int32 Direction)
{
	if (Direction == 0)
	{
		return;
	}
	const int32 MinimumTeamSize = bPartyRequested ? FMath::Clamp(GetPartyMemberCount(), 1, 3) : 1;
	int32 Candidate = MatchmakingPlayersPerTeam;
	do
	{
		Candidate += FMath::Sign(Direction);
		if (Candidate > 3) Candidate = 1;
		if (Candidate < 1) Candidate = 3;
	}
	while (Candidate < MinimumTeamSize);
	MatchmakingPlayersPerTeam = Candidate;
}

void AFlickGameMode::SetMatchmakingPlayersPerTeam(const int32 PlayersPerTeam)
{
	MatchmakingPlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
}

void AFlickGameMode::ToggleRankedQueue()
{
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
		Sessions && Sessions->IsMatchmakingActive())
	{
		return;
	}
	bRankedQueueSelected = !bRankedQueueSelected;
}

void AFlickGameMode::SetRankedQueueSelected(const bool bRanked)
{
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
		Sessions && Sessions->IsMatchmakingActive())
	{
		return;
	}
	bRankedQueueSelected = bRanked;
}

void AFlickGameMode::StartSelectedMatchmaking()
{
	bTrainingMode = false;
	bTrainingEditMode = false;
	bTrainingBotMatch = false;
	bClassSelectionStartsTrainingBotMatch = false;
	bTestArenaMode = false;
	ResetTrainingBotThinking();
	if (bPartyRequested)
	{
		QueuePartyForMatchmaking(MatchmakingPlayersPerTeam);
		return;
	}
	if (GetNetMode() != NM_Standalone)
	{
		return;
	}
	FrontendScreen = EFlickFrontendScreen::OnlineBrowser;
	SetCameraForFrontend();
	RankedQueueRating = FlickRankRules::DefaultRating;
	if (bRankedQueueSelected)
	{
		if (const UFlickRankingSubsystem* Ranking = GetFlickRankingSubsystem())
		{
			RankedQueueRating = Ranking->GetQueueRating(SelectedMatchVariant, MatchmakingPlayersPerTeam);
		}
	}
	if (BeginCoordinatorQueue(MatchmakingPlayersPerTeam))
	{
		return;
	}
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
	{
		Sessions->StartMatchmaking(
			SelectedMatchVariant,
			MatchmakingPlayersPerTeam,
			1,
			bRankedQueueSelected,
			RankedQueueRating);
	}
}

void AFlickGameMode::StartUnrankedMatchmaking()
{
	bRankedQueueSelected = false;
	StartSelectedMatchmaking();
}

void AFlickGameMode::CancelUnrankedMatchmaking()
{
	if (UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem();
		Coordinator && Coordinator->IsQueueActive())
	{
		Coordinator->CancelQueue();
		bCoordinatorQueuePending = false;
		CoordinatorAuthenticationTickets.Reset();
		CoordinatorQueueAccountIds.Reset();
		if (AFlickGameState* State = GetFlickGameState())
		{
			State->SetMatchmakingState(false);
		}
		return;
	}
	if (bNetworkMatchRequested && bMatchmakingRequested)
	{
		CancelNetworkLobby();
		return;
	}
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
	{
		Sessions->CancelMatchmaking(false);
	}
}

bool AFlickGameMode::BeginCoordinatorQueue(const int32 PlayersPerTeam)
{
	UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem();
	if (!Coordinator || !Coordinator->ShouldUseCoordinator())
	{
		return false;
	}
	if (Coordinator->IsQueueActive() || !GetWorld())
	{
		return true;
	}

	TArray<APlayerController*> QueueControllers;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		const AFlickPlayerState* PlayerState = Controller
			? Controller->GetPlayerState<AFlickPlayerState>()
			: nullptr;
		if (!Controller || !PlayerState)
		{
			continue;
		}
		if (bPartyRequested)
		{
			if (!PlayerState->GetPartyId().IsEmpty())
			{
				QueueControllers.Add(Controller);
			}
		}
		else if (Controller->IsLocalController())
		{
			QueueControllers.Add(Controller);
		}
	}
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	if (QueueControllers.IsEmpty() || QueueControllers.Num() > TeamSize)
	{
		UE_LOG(LogFlick, Warning, TEXT("Coordinator queue rejected: party does not fit %dv%d"), TeamSize, TeamSize);
		return true;
	}
	QueueControllers.Sort([](const APlayerController& Left, const APlayerController& Right)
	{
		const AFlickPlayerState* LeftState = Left.GetPlayerState<AFlickPlayerState>();
		const AFlickPlayerState* RightState = Right.GetPlayerState<AFlickPlayerState>();
		return (LeftState ? LeftState->GetPartySlot() : 0) < (RightState ? RightState->GetPartySlot() : 0);
	});

	bCoordinatorQueuePending = true;
	PendingCoordinatorTeamSize = TeamSize;
	CoordinatorAuthenticationTickets.Reset();
	CoordinatorQueueAccountIds.Reset();
	if (CoordinatorAllocatedHandle.IsValid())
	{
		Coordinator->OnAllocated.Remove(CoordinatorAllocatedHandle);
	}
	CoordinatorAllocatedHandle = Coordinator->OnAllocated.AddUObject(this, &AFlickGameMode::HandleCoordinatorAllocation);
	for (APlayerController* Controller : QueueControllers)
	{
		CoordinatorAuthenticationTickets.Add(Controller, FString());
		if (Coordinator->RequiresSteamTickets())
		{
			if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(Controller))
			{
				FlickController->RequestCoordinatorAuthenticationFromServer(
					FString::Printf(TEXT("WebAPI:%s"), *Coordinator->GetSteamTicketAudience()));
			}
		}
	}
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetMatchmakingState(true, false, bRankedQueueSelected, RankedQueueRating, 0);
	}
	TrySubmitCoordinatorQueue();
	return true;
}

void AFlickGameMode::SubmitCoordinatorAuthentication(
	APlayerController* RequestingPlayer,
	const FString& SteamAuthTicket)
{
	UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem();
	if (!bCoordinatorQueuePending || !Coordinator || !CoordinatorAuthenticationTickets.Contains(RequestingPlayer))
	{
		return;
	}
	CoordinatorAuthenticationTickets[RequestingPlayer] = SteamAuthTicket.Left(8192);
	if (Coordinator->RequiresSteamTickets() && SteamAuthTicket.IsEmpty())
	{
		UE_LOG(LogFlick, Error, TEXT("Coordinator queue authentication failed for one party member"));
		Coordinator->CancelQueue();
		bCoordinatorQueuePending = false;
		return;
	}
	TrySubmitCoordinatorQueue();
}

void AFlickGameMode::TrySubmitCoordinatorQueue()
{
	UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem();
	if (!bCoordinatorQueuePending || !Coordinator || Coordinator->IsQueueActive())
	{
		return;
	}
	if (Coordinator->RequiresSteamTickets())
	{
		for (const TPair<APlayerController*, FString>& Pair : CoordinatorAuthenticationTickets)
		{
			if (Pair.Value.IsEmpty())
			{
				return;
			}
		}
	}

	FFlickCoordinatorQueueRequest Request;
	Request.Variant = SelectedMatchVariant;
	Request.PlayersPerTeam = PendingCoordinatorTeamSize;
	Request.bRanked = bRankedQueueSelected;
	Request.Region = TEXT("auto");
	for (const TPair<APlayerController*, FString>& Pair : CoordinatorAuthenticationTickets)
	{
		APlayerController* Controller = Pair.Key;
		const AFlickPlayerState* PlayerState = Controller
			? Controller->GetPlayerState<AFlickPlayerState>()
			: nullptr;
		if (!PlayerState)
		{
			continue;
		}
		if (Request.PartyId.IsEmpty())
		{
			Request.PartyId = PlayerState->GetPartyId();
		}
		FFlickCoordinatorPartyMember Member;
		Member.AccountId = GetPlayerOnlineId(PlayerState);
		FString LocalAccountOverride;
		if (CoordinatorAuthenticationTickets.Num() == 1
			&& Controller->IsLocalController()
			&& FParse::Value(FCommandLine::Get(), TEXT("FlickLocalAccountId="), LocalAccountOverride))
		{
			Member.AccountId = LocalAccountOverride.Left(128);
		}
		if (Member.AccountId.IsEmpty())
		{
			Member.AccountId = FString::Printf(
				TEXT("dev-%s-%d"),
				Request.PartyId.IsEmpty() ? TEXT("solo") : *Request.PartyId,
				Request.Members.Num());
		}
		Member.DisplayName = PlayerState->GetPlayerName();
		Member.SteamAuthTicket = Pair.Value;
		Member.PartySlot = PlayerState->GetPartySlot() == INDEX_NONE
			? Request.Members.Num()
			: PlayerState->GetPartySlot();
		Member.Rating = RankedQueueRating;
		CoordinatorQueueAccountIds.Add(Controller, Member.AccountId);
		Request.Members.Add(MoveTemp(Member));
	}
	if (Request.PartyId.IsEmpty())
	{
		Request.PartyId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
	}
	if (Request.Members.Num() != CoordinatorAuthenticationTickets.Num())
	{
		bCoordinatorQueuePending = false;
		return;
	}
	bCoordinatorQueuePending = false;
	Coordinator->QueueParty(Request,
		[this](const bool bAccepted, const FString& Error)
		{
			if (!bAccepted)
			{
				UE_LOG(LogFlick, Error, TEXT("COORDINATOR_QUEUE_REJECTED: %s"), *Error);
				CoordinatorQueueAccountIds.Reset();
			}
		});
}

void AFlickGameMode::HandleCoordinatorAllocation(const FFlickCoordinatorAllocation& Allocation)
{
	if (!GetWorld() || Allocation.MatchId.IsEmpty())
	{
		return;
	}
	int32 TravellingPlayers = 0;
	for (const TPair<APlayerController*, FString>& Pair : CoordinatorQueueAccountIds)
	{
		AFlickPlayerController* Controller = Cast<AFlickPlayerController>(Pair.Key);
		AFlickPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AFlickPlayerState>() : nullptr;
		const FFlickCoordinatorReservation* Reservation = Allocation.Reservations.FindByPredicate(
			[&Pair](const FFlickCoordinatorReservation& Candidate)
			{
				return Candidate.AccountId == Pair.Value;
			});
		if (!Controller || !PlayerState || !Reservation)
		{
			continue;
		}
		Controller->TravelToCoordinatorMatchFromServer(
			Allocation.MatchId,
			Allocation.ServerId,
			Allocation.Address,
			Allocation.Variant,
			Allocation.PlayersPerTeam,
			Allocation.bRanked,
			Allocation.ExpiresUnixTime,
			Reservation->AccountId,
			Reservation->Token,
			Reservation->Team,
			Reservation->PlayerSlot,
			PlayerState->GetPartyId(),
			PlayerState->GetPartySlot() == INDEX_NONE ? 0 : PlayerState->GetPartySlot(),
			CoordinatorQueueAccountIds.Num(),
			PlayerState->IsPartyLeader() || CoordinatorQueueAccountIds.Num() == 1);
		++TravellingPlayers;
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("COORDINATOR_ALLOCATION_ACCEPTED: match=%s server=%s players=%d"),
		*Allocation.MatchId,
		*Allocation.ServerId,
		TravellingPlayers);
	CoordinatorQueueAccountIds.Reset();
}

void AFlickGameMode::VerifyCoordinatorReservation(
	APlayerController* Player,
	const FString& AccountId,
	const FString& ReservationToken)
{
	UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem();
	if (!Coordinator || !Player || CoordinatorMatchId.IsEmpty())
	{
		return;
	}
	Coordinator->VerifyReservation(
		CoordinatorMatchId,
		AccountId,
		ReservationToken,
		[WeakThis = TWeakObjectPtr<AFlickGameMode>(this), WeakPlayer = TWeakObjectPtr<APlayerController>(Player)](
			const FFlickCoordinatorReservationResult& Result)
		{
			AFlickGameMode* GameMode = WeakThis.Get();
			APlayerController* Controller = WeakPlayer.Get();
			AFlickPlayerState* PlayerState = Controller
				? Controller->GetPlayerState<AFlickPlayerState>()
				: nullptr;
			if (!GameMode || !Controller || !PlayerState)
			{
				return;
			}
			const bool bSlotAvailable = Result.Team != EFlickTeam::None
				&& Result.PlayerSlot >= 0
				&& Result.PlayerSlot < GameMode->CurrentPlayersPerTeam
				&& (!GameMode->GetLobbyPlayer(Result.Team, Result.PlayerSlot)
					|| GameMode->GetLobbyPlayer(Result.Team, Result.PlayerSlot) == PlayerState);
			if (!Result.bAccepted || Result.AccountId.IsEmpty() || !bSlotAvailable)
			{
				UE_LOG(
					LogFlick,
					Error,
					TEXT("COORDINATOR_RESERVATION_REJECTED: account=%s error=%s"),
					*Result.AccountId,
					*Result.Error);
				if (GameMode->GameSession)
				{
					GameMode->GameSession->KickPlayer(
						Controller,
						FText::FromString(TEXT("The dedicated-server reservation is invalid or expired.")));
				}
				return;
			}
			PlayerState->SetVerifiedOnlineAccountId(Result.AccountId);
			PlayerState->SetTeam(Result.Team);
			PlayerState->SetTeamPlayerSlot(Result.PlayerSlot);
			PlayerState->SetLobbyReady(false);
			GameMode->VerifiedCoordinatorPlayers.Add(Controller);
			GameMode->DisconnectedPlayerTeams.Remove(Result.AccountId);
			GameMode->DisconnectedPlayerSlots.Remove(Result.AccountId);
			UE_LOG(
				LogFlick,
				Log,
				TEXT("COORDINATOR_RESERVATION_ACCEPTED: account=%s team=%d slot=%d"),
				*Result.AccountId,
				GetTeamNumber(Result.Team),
				Result.PlayerSlot + 1);
			if (GameMode->bRankedRequested)
			{
				if (UFlickRankedBackendSubsystem* Backend = GameMode->GetFlickRankedBackendSubsystem())
				{
					if (Backend->IsRemoteAuthorityEnabled())
					{
						if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(Controller))
						{
							FlickController->RequestRankedAuthenticationFromServer(
								FString::Printf(TEXT("WebAPI:%s"), *Backend->GetSteamTicketAudience()));
						}
					}
					else
					{
						GameMode->SubmitRankedAuthentication(Controller, FString());
					}
				}
			}
			else
			{
				GameMode->TryStartNetworkMatch();
			}
		});
}

bool AFlickGameMode::AreCoordinatorReservationsVerified() const
{
	if (CoordinatorMatchId.IsEmpty())
	{
		return true;
	}
	return VerifiedCoordinatorPlayers.Num() == CurrentPlayersPerTeam * 2;
}

void AFlickGameMode::JoinLocalNetworkMatch(const FString& Address)
{
	if (GetNetMode() != NM_Standalone || Address.IsEmpty() || bPartyRequested)
	{
		return;
	}

	if (APlayerController* LocalController = UGameplayStatics::GetPlayerController(this, 0))
	{
		UE_LOG(LogFlick, Log, TEXT("Joining localhost lobby at %s"), *Address);
		LocalController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	}
}

AFlickPlayerState* AFlickGameMode::GetLobbyPlayer(const EFlickTeam Team) const

{
	return GetLobbyPlayer(Team, 0);
}

AFlickPlayerState* AFlickGameMode::GetLobbyPlayer(const EFlickTeam Team, const int32 TeamPlayerSlot) const
{
	if (bPrivateMatchSetupActive || bPrivateMatchActive)
	{
		return GetPrivateSlotOwner(Team, TeamPlayerSlot);
	}
	const AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return nullptr;
	}
	for (APlayerState* PlayerState : FlickGameState->PlayerArray)
	{
		AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(PlayerState);
		if (FlickPlayerState
			&& FlickPlayerState->GetTeam() == Team
			&& FlickPlayerState->GetTeamPlayerSlot() == TeamPlayerSlot)
		{
			return FlickPlayerState;
		}
	}
	return nullptr;
}

int32 AFlickGameMode::GetLobbyPlayerCount(const EFlickTeam Team) const
{
	if (bPrivateMatchSetupActive || bPrivateMatchActive)
	{
		int32 Count = 0;
		for (int32 PlayerSlot = 0; PlayerSlot < CurrentPlayersPerTeam; ++PlayerSlot)
		{
			Count += GetPrivateSlotOwner(Team, PlayerSlot) ? 1 : 0;
		}
		return Count;
	}
	const AFlickGameState* State = GetFlickGameState();
	if (!State)
	{
		return 0;
	}
	int32 Count = 0;
	for (const APlayerState* PlayerState : State->PlayerArray)
	{
		const AFlickPlayerState* Player = Cast<AFlickPlayerState>(PlayerState);
		Count += Player && Player->GetTeam() == Team ? 1 : 0;
	}
	return Count;
}

AFlickPlayerState* AFlickGameMode::GetPrivateSlotOwner(
	const EFlickTeam Team,
	const int32 PlayerSlot) const
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State)
	{
		return nullptr;
	}
	for (APlayerState* PlayerState : State->PlayerArray)
	{
		AFlickPlayerState* Player = Cast<AFlickPlayerState>(PlayerState);
		if (Player && Player->ControlsPrivateSlot(Team, PlayerSlot))
		{
			return Player;
		}
	}
	return nullptr;
}

TArray<AFlickPlayerState*> AFlickGameMode::GetPrivateMatchParticipants() const
{
	TArray<AFlickPlayerState*> Participants;
	if (const AFlickGameState* State = GetFlickGameState())
	{
		for (APlayerState* PlayerState : State->PlayerArray)
		{
			if (AFlickPlayerState* Player = Cast<AFlickPlayerState>(PlayerState))
			{
				Participants.Add(Player);
			}
		}
	}
	Participants.Sort([](const AFlickPlayerState& Left, const AFlickPlayerState& Right)
	{
		const int32 LeftSlot = Left.GetPartySlot() == INDEX_NONE ? FlickMaximumPartyMembers : Left.GetPartySlot();
		const int32 RightSlot = Right.GetPartySlot() == INDEX_NONE ? FlickMaximumPartyMembers : Right.GetPartySlot();
		return LeftSlot < RightSlot;
	});
	return Participants;
}

void AFlickGameMode::RefreshPrivatePrimaryAssignments()
{
	for (AFlickPlayerState* Player : GetPrivateMatchParticipants())
	{
		if (!Player)
		{
			continue;
		}
		const TArray<int32>& ControlledSlots = Player->GetPrivateControlledSlots();
		if (ControlledSlots.IsEmpty())
		{
			Player->SetTeam(EFlickTeam::None);
			Player->SetTeamPlayerSlot(INDEX_NONE);
			continue;
		}
		const int32 EncodedSlot = ControlledSlots[0];
		Player->SetTeam(EncodedSlot < 3 ? EFlickTeam::Player1 : EFlickTeam::Player2);
		Player->SetTeamPlayerSlot(EncodedSlot % 3);
	}
}

void AFlickGameMode::ResetPrivateMatchReadiness()
{
	for (AFlickPlayerState* Player : GetPrivateMatchParticipants())
	{
		if (Player)
		{
			Player->SetLobbyReady(false);
		}
	}
}

void AFlickGameMode::AutoAssignPrivateMatchSlots()
{
	TArray<AFlickPlayerState*> Participants = GetPrivateMatchParticipants();
	for (AFlickPlayerState* Player : Participants)
	{
		if (Player)
		{
			Player->ClearPrivateControlledSlots();
		}
	}
	if (Participants.IsEmpty())
	{
		return;
	}

	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PrivateMatchSettings.PlayersPerTeam);
	const int32 ActiveParticipantCount = FMath::Min(Participants.Num(), TeamSize * 2);
	const int32 BlueParticipantCount = FMath::Clamp((ActiveParticipantCount + 1) / 2, 1, TeamSize);
	const int32 OrangeParticipantCount = FMath::Max(1, ActiveParticipantCount - BlueParticipantCount);
	const int32 OrangeParticipantStart = ActiveParticipantCount > BlueParticipantCount
		? BlueParticipantCount
		: 0;
	for (int32 PlayerSlot = 0; PlayerSlot < TeamSize; ++PlayerSlot)
	{
		AFlickPlayerState* BlueOwner = Participants[PlayerSlot % BlueParticipantCount];
		AFlickPlayerState* OrangeOwner = Participants[OrangeParticipantStart
			+ (PlayerSlot % OrangeParticipantCount)];
		TArray<int32> BlueSlots = BlueOwner->GetPrivateControlledSlots();
		BlueSlots.Add(EncodePrivatePlayerSlot(EFlickTeam::Player1, PlayerSlot));
		BlueOwner->SetPrivateControlledSlots(BlueSlots);
		TArray<int32> OrangeSlots = OrangeOwner->GetPrivateControlledSlots();
		OrangeSlots.Add(EncodePrivatePlayerSlot(EFlickTeam::Player2, PlayerSlot));
		OrangeOwner->SetPrivateControlledSlots(OrangeSlots);
	}
	RefreshPrivatePrimaryAssignments();
	ResetPrivateMatchReadiness();
}

void AFlickGameMode::PushPrivateMatchState()
{
	CurrentPlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(PrivateMatchSettings.PlayersPerTeam);
	MatchmakingPlayersPerTeam = CurrentPlayersPerTeam;
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetTeamFormat(CurrentPlayersPerTeam);
		State->SetPrivateMatchLobbyState(bPrivateMatchSetupActive, PrivateMatchSettings);
		State->SetNetworkLobbyState(false, SelectedMatchVariant);
		State->SetPartyState(bPartyRequested, State->PartyLeaderUserId, FlickMaximumPartyMembers);
		State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
}

void AFlickGameMode::OpenPrivateMatchSetup()
{
	if (bMatchmakingRequested || bNetworkMatchStarted)
	{
		return;
	}
	const TArray<AFlickPlayerState*> Participants = GetPrivateMatchParticipants();
	if (Participants.IsEmpty())
	{
		return;
	}
	if (bPartyRequested && (!Participants[0] || !Participants[0]->IsPartyLeader()))
	{
		return;
	}

	PrivateMatchReturnVariant = SelectedMatchVariant;
	PrivateMatchReturnPlayersPerTeam = SelectedMatchVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
	PrivateMatchSettings = FFlickPrivateMatchSettings();
	PrivateMatchSettings.PlayersPerTeam = FMath::Clamp((Participants.Num() + 1) / 2, 1, 3);
	SelectedMatchVariant = EFlickMatchVariant::Classic;
	bPrivateMatchSetupActive = true;
	bPrivateMatchActive = false;
	bNetworkMatchRequested = true;
	bNetworkMatchStarted = false;
	bMatchmakingRequested = false;
	bRankedRequested = false;
	FrontendScreen = EFlickFrontendScreen::PrivateMatch;
	AutoAssignPrivateMatchSlots();
	PushPrivateMatchState();
	SetCameraForFrontend();
	PlayMenuSound(true);
}

void AFlickGameMode::ClosePrivateMatchSetup()
{
	if (!bPrivateMatchSetupActive)
	{
		return;
	}
	UGameplayStatics::SetGamePaused(this, false);
	for (AFlickPlayerState* Player : GetPrivateMatchParticipants())
	{
		if (Player)
		{
			Player->ClearPrivateControlledSlots();
			Player->SetTeam(EFlickTeam::None);
			Player->SetTeamPlayerSlot(INDEX_NONE);
			Player->SetLobbyReady(false);
		}
	}
	bPrivateMatchSetupActive = false;
	bPrivateMatchActive = false;
	bNetworkMatchRequested = false;
	bNetworkMatchStarted = false;
	SelectedMatchVariant = PrivateMatchReturnVariant;
	MatchmakingPlayersPerTeam = PrivateMatchReturnPlayersPerTeam;
	FrontendScreen = EFlickFrontendScreen::ModeSelect;
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetPrivateMatchLobbyState(false, PrivateMatchSettings);
		State->SetNetworkLobbyState(false, SelectedMatchVariant);
	}
	ShowModePreview(PrivateMatchReturnVariant, PrivateMatchReturnPlayersPerTeam);
	if (CameraPawn)
	{
		// Private gameplay can leave the presentation camera a long way from its
		// menu target. Snap here so its interpolation does not create a blurred,
		// shake-like arena background after leaving the private lobby.
		CameraPawn->ApplyCameraSettings();
	}
	PlayMenuSound(false);
}

void AFlickGameMode::CyclePrivateMatchSetting(
	const EFlickPrivateMatchSetting Setting,
	const int32 Direction)
{
	if (!bPrivateMatchSetupActive || Direction == 0)
	{
		return;
	}
	const int32 Step = FMath::Sign(Direction);
	const auto CycleFloat = [Step](float& Value, const TArray<float>& Values)
	{
		int32 Index = Values.IndexOfByPredicate([Value](const float Candidate)
		{
			return FMath::IsNearlyEqual(Value, Candidate);
		});
		Index = Index == INDEX_NONE ? 0 : Index;
		Value = Values[(Index + Step + Values.Num()) % Values.Num()];
	};

	switch (Setting)
	{
	case EFlickPrivateMatchSetting::TeamSize:
		PrivateMatchSettings.PlayersPerTeam += Step;
		if (PrivateMatchSettings.PlayersPerTeam > 3) PrivateMatchSettings.PlayersPerTeam = 1;
		if (PrivateMatchSettings.PlayersPerTeam < 1) PrivateMatchSettings.PlayersPerTeam = 3;
		AutoAssignPrivateMatchSlots();
		break;
	case EFlickPrivateMatchSetting::RoundsToWin:
		PrivateMatchSettings.RoundsToWin += Step;
		if (PrivateMatchSettings.RoundsToWin > 5) PrivateMatchSettings.RoundsToWin = 1;
		if (PrivateMatchSettings.RoundsToWin < 1) PrivateMatchSettings.RoundsToWin = 5;
		break;
	case EFlickPrivateMatchSetting::ArenaScale:
		CycleFloat(PrivateMatchSettings.ArenaScale, {0.85f, 1.0f, 1.15f, 1.3f});
		break;
	case EFlickPrivateMatchSetting::FrictionScale:
		CycleFloat(PrivateMatchSettings.FrictionScale, {0.5f, 0.75f, 1.0f, 1.5f, 2.0f});
		break;
	case EFlickPrivateMatchSetting::LaunchSpeedScale:
		CycleFloat(PrivateMatchSettings.LaunchSpeedScale, {0.75f, 1.0f, 1.25f, 1.5f});
		break;
	case EFlickPrivateMatchSetting::RestitutionScale:
		CycleFloat(PrivateMatchSettings.RestitutionScale, {0.5f, 1.0f, 1.5f});
		break;
	case EFlickPrivateMatchSetting::SimultaneousKickoff:
		PrivateMatchSettings.bSimultaneousKickoff = !PrivateMatchSettings.bSimultaneousKickoff;
		break;
	default:
		break;
	}
	ResetPrivateMatchReadiness();
	PushPrivateMatchState();
	PlayMenuSound(false);
}

void AFlickGameMode::TogglePrivateMatchSlot(
	APlayerController* RequestingPlayer,
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	AFlickPlayerState* RequestingState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (!bPrivateMatchSetupActive || !RequestingState || Team == EFlickTeam::None
		|| PlayerSlot < 0 || PlayerSlot >= PrivateMatchSettings.PlayersPerTeam)
	{
		return;
	}
	const int32 EncodedSlot = EncodePrivatePlayerSlot(Team, PlayerSlot);
	const bool bReleaseSlot = RequestingState->GetPrivateControlledSlots().Contains(EncodedSlot);
	for (AFlickPlayerState* Player : GetPrivateMatchParticipants())
	{
		if (!Player)
		{
			continue;
		}
		TArray<int32> Slots = Player->GetPrivateControlledSlots();
		Slots.Remove(EncodedSlot);
		if (Player == RequestingState && !bReleaseSlot)
		{
			Slots.Add(EncodedSlot);
		}
		Player->SetPrivateControlledSlots(Slots);
	}
	RefreshPrivatePrimaryAssignments();
	ResetPrivateMatchReadiness();
	PushPrivateMatchState();
}

void AFlickGameMode::SetPrivateMatchSpectating(APlayerController* RequestingPlayer)
{
	AFlickPlayerState* RequestingState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (!bPrivateMatchSetupActive || !RequestingState)
	{
		return;
	}
	RequestingState->ClearPrivateControlledSlots();
	RefreshPrivatePrimaryAssignments();
	ResetPrivateMatchReadiness();
	PushPrivateMatchState();
}

bool AFlickGameMode::CanStartPrivateMatch() const
{
	if (!bPrivateMatchSetupActive || !IsNetworkLobby())
	{
		return false;
	}
	for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
	{
		for (int32 PlayerSlot = 0; PlayerSlot < PrivateMatchSettings.PlayersPerTeam; ++PlayerSlot)
		{
			const AFlickPlayerState* SlotOwner = GetPrivateSlotOwner(Team, PlayerSlot);
			if (!SlotOwner || !SlotOwner->IsLobbyReady())
			{
				return false;
			}
		}
	}
	return true;
}

void AFlickGameMode::StartPrivateMatch(APlayerController* RequestingPlayer)
{
	const AFlickPlayerState* RequestingState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	const bool bHost = RequestingPlayer && !RequestingPlayer->GetNetConnection();
	if (!CanStartPrivateMatch()
		|| (!bHost && (!RequestingState || !RequestingState->IsPartyLeader())))
	{
		return;
	}
	PrivateMatchSettings.PlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(PrivateMatchSettings.PlayersPerTeam);
	MatchmakingPlayersPerTeam = PrivateMatchSettings.PlayersPerTeam;
	CompleteNetworkMatchStart(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
}

int32 AFlickGameMode::GetPartyMemberCount() const
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State)
	{
		return 0;
	}
	int32 Count = 0;
	for (const APlayerState* PlayerState : State->PlayerArray)
	{
		const AFlickPlayerState* PartyPlayer = Cast<AFlickPlayerState>(PlayerState);
		Count += PartyPlayer && PartyPlayer->GetPartySlot() != INDEX_NONE ? 1 : 0;
	}
	return Count;
}

int32 AFlickGameMode::GetPartyMemberCountForId(const FString& PartyId) const
{
	if (PartyId.IsEmpty())
	{
		return 0;
	}
	const AFlickGameState* State = GetFlickGameState();
	if (!State)
	{
		return 0;
	}
	int32 Count = 0;
	for (const APlayerState* PlayerState : State->PlayerArray)
	{
		const AFlickPlayerState* PartyPlayer = Cast<AFlickPlayerState>(PlayerState);
		Count += PartyPlayer && PartyPlayer->GetPartyId() == PartyId ? 1 : 0;
	}
	return Count;
}

bool AFlickGameMode::FindPremadeTeamAndSlot(
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	EFlickTeam& OutTeam,
	int32& OutPlayerSlot)
{
	OutTeam = EFlickTeam::None;
	OutPlayerSlot = INDEX_NONE;
	const int32 SafePartySize = FMath::Clamp(PartySize, 1, CurrentPlayersPerTeam);
	if (PartyId.IsEmpty() || PartySlot < 0 || PartySlot >= SafePartySize)
	{
		return false;
	}

	if (const EFlickTeam* ExistingTeam = PremadePartyTeams.Find(PartyId))
	{
		OutTeam = *ExistingTeam;
		const TArray<int32>* ReservedSlots = PremadePartySlots.Find(PartyId);
		if (!ReservedSlots || !ReservedSlots->IsValidIndex(PartySlot))
		{
			return false;
		}
		const int32 ReservedSlot = (*ReservedSlots)[PartySlot];
		if (ReservedSlot == INDEX_NONE || GetLobbyPlayer(OutTeam, ReservedSlot))
		{
			return false;
		}
		OutPlayerSlot = ReservedSlot;
		return true;
	}

	bool Player1Slots[3] = {false, false, false};
	bool Player2Slots[3] = {false, false, false};
	if (const AFlickGameState* State = GetFlickGameState())
	{
		for (const APlayerState* StatePlayer : State->PlayerArray)
		{
			const AFlickPlayerState* Player = Cast<AFlickPlayerState>(StatePlayer);
			if (!Player || Player->GetTeamPlayerSlot() < 0 || Player->GetTeamPlayerSlot() >= CurrentPlayersPerTeam)
			{
				continue;
			}
			bool* Slots = Player->GetTeam() == EFlickTeam::Player1
				? Player1Slots
				: Player->GetTeam() == EFlickTeam::Player2 ? Player2Slots : nullptr;
			if (Slots)
			{
				Slots[Player->GetTeamPlayerSlot()] = true;
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

	auto CountUsedSlots = [this](const bool Slots[3])
	{
		int32 Count = 0;
		for (int32 Slot = 0; Slot < CurrentPlayersPerTeam; ++Slot)
		{
			Count += Slots[Slot] ? 1 : 0;
		}
		return Count;
	};
	OutTeam = FlickMatchmakingRules::ChooseTeamForPremade(
		CountUsedSlots(Player1Slots),
		CountUsedSlots(Player2Slots),
		CurrentPlayersPerTeam,
		SafePartySize);
	if (OutTeam == EFlickTeam::None)
	{
		return false;
	}

	const bool* SelectedSlots = OutTeam == EFlickTeam::Player1 ? Player1Slots : Player2Slots;
	TArray<int32> ReservedSlots;
	ReservedSlots.Reserve(SafePartySize);
	for (int32 Slot = 0; Slot < CurrentPlayersPerTeam && ReservedSlots.Num() < SafePartySize; ++Slot)
	{
		if (!SelectedSlots[Slot])
		{
			ReservedSlots.Add(Slot);
		}
	}
	if (ReservedSlots.Num() != SafePartySize)
	{
		return false;
	}
	PremadePartyTeams.Add(PartyId, OutTeam);
	PremadePartySlots.Add(PartyId, ReservedSlots);
	OutPlayerSlot = ReservedSlots[PartySlot];
	return true;
}

AFlickPlayerState* AFlickGameMode::GetPartyMember(const int32 PartySlot) const
{
	const AFlickGameState* State = GetFlickGameState();
	if (!State)
	{
		return nullptr;
	}
	for (APlayerState* PlayerState : State->PlayerArray)
	{
		AFlickPlayerState* PartyPlayer = Cast<AFlickPlayerState>(PlayerState);
		if (PartyPlayer && PartyPlayer->GetPartySlot() == PartySlot)
		{
			return PartyPlayer;
		}
	}
	return nullptr;
}

bool AFlickGameMode::CanOpenPartyTeamLobby(const int32 PlayersPerTeam) const
{
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	return bPartyRequested
		&& (TeamSize == 2 || TeamSize == 3)
		&& GetPartyMemberCount() > 0
		&& GetPartyMemberCount() <= TeamSize
		&& GetPartyMember(0)
		&& GetPartyMember(0)->IsPartyLeader();
}

void AFlickGameMode::OpenPartyTeamLobby(const int32 PlayersPerTeam)
{
	if (!CanOpenPartyTeamLobby(PlayersPerTeam))
	{
		UE_LOG(LogFlick, Warning, TEXT("Team lobby requires a party leader and no more members than the selected team size"));
		return;
	}

	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
	{
		if (Sessions->HasActiveSession() && !Sessions->ConvertPartyToPrivateMatch(SelectedMatchVariant, TeamSize))
		{
			return;
		}
	}

	for (int32 PartySlot = 0; PartySlot < TeamSize; ++PartySlot)
	{
		if (AFlickPlayerState* PartyMember = GetPartyMember(PartySlot))
		{
			PartyMember->SetTeam(EFlickTeam::Player1);
			PartyMember->SetTeamPlayerSlot(PartySlot);
			PartyMember->SetLobbyReady(false);
		}
	}

	CurrentPlayersPerTeam = TeamSize;
	bPartyRequested = false;
	bMatchmakingRequested = false;
	bRankedRequested = false;
	bNetworkMatchRequested = true;
	bNetworkMatchStarted = false;
	FrontendScreen = EFlickFrontendScreen::NetworkLobby;
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetPartyState(false, FString(), TeamSize);
		State->SetMatchmakingState(false);
		State->SetTeamFormat(TeamSize);
		State->SetNetworkLobbyState(true, SelectedMatchVariant);
		State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	SetCameraForFrontend();
	UE_LOG(LogFlick, Log, TEXT("Private %dv%d team lobby opened; invite the opposing team through Steam"), TeamSize, TeamSize);
}

bool AFlickGameMode::CanQueuePartyForMatchmaking(const int32 PlayersPerTeam) const
{
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	return bPartyRequested
		&& GetPartyMemberCount() >= 1
		&& GetPartyMemberCount() <= TeamSize
		&& GetPartyMember(0)
		&& GetPartyMember(0)->IsPartyLeader();
}

void AFlickGameMode::QueuePartyForMatchmaking(const int32 PlayersPerTeam)
{
	if (!CanQueuePartyForMatchmaking(PlayersPerTeam))
	{
		UE_LOG(LogFlick, Warning, TEXT("The current party does not fit that matchmaking team size"));
		return;
	}
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	const int32 PartySize = GetPartyMemberCount();
	RankedQueueRating = FlickRankRules::DefaultRating;
	if (bRankedQueueSelected)
	{
		if (const UFlickRankingSubsystem* Ranking = GetFlickRankingSubsystem())
		{
			RankedQueueRating = Ranking->GetQueueRating(SelectedMatchVariant, TeamSize);
		}
	}
	if (BeginCoordinatorQueue(TeamSize))
	{
		return;
	}
	UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
	if (!Sessions || !Sessions->ConvertPartyToMatchmakingQueue(
		SelectedMatchVariant,
		TeamSize,
		PartySize,
		bRankedQueueSelected,
		RankedQueueRating))
	{
		return;
	}
	for (int32 PartySlot = 0; PartySlot < TeamSize; ++PartySlot)
	{
		if (AFlickPlayerState* PartyMember = GetPartyMember(PartySlot))
		{
			PartyMember->SetTeam(EFlickTeam::Player1);
			PartyMember->SetTeamPlayerSlot(PartySlot);
			PartyMember->SetLobbyReady(false);
			if (!PartyMember->GetPartyId().IsEmpty())
			{
				PremadePartyTeams.Add(PartyMember->GetPartyId(), EFlickTeam::Player1);
				TArray<int32>& ReservedSlots = PremadePartySlots.FindOrAdd(PartyMember->GetPartyId());
				if (ReservedSlots.Num() != PartySize)
				{
					ReservedSlots.Init(INDEX_NONE, PartySize);
				}
				if (ReservedSlots.IsValidIndex(PartyMember->GetPartySlot()))
				{
					ReservedSlots[PartyMember->GetPartySlot()] = PartyMember->GetTeamPlayerSlot();
				}
			}
			if (AFlickPlayerController* PartyController = Cast<AFlickPlayerController>(PartyMember->GetOwner()))
			{
				PartyController->SetPersistentPartyIdentityFromServer(
					PartyMember->GetPartyId(),
					PartyMember->GetPartySlot(),
					PartySize,
					PartyMember->IsPartyLeader());
			}
		}
	}
	CurrentPlayersPerTeam = TeamSize;
	MatchmakingPlayersPerTeam = TeamSize;
	bPartyRequested = false;
	bNetworkMatchRequested = true;
	bNetworkMatchStarted = false;
	bMatchmakingRequested = true;
	bRankedRequested = bRankedQueueSelected;
	bMatchmakingLobbyLocked = false;
	MatchmakingQueueElapsed = 0.0f;
	FrontendScreen = EFlickFrontendScreen::NetworkLobby;
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetPartyState(false, FString(), FlickMaximumPartyMembers);
		State->SetTeamFormat(TeamSize);
		State->SetNetworkLobbyState(true, SelectedMatchVariant);
		UpdateReplicatedMatchmakingState(false);
		State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	SetCameraForFrontend();
	UE_LOG(
		LogFlick,
		Log,
		TEXT("Premade party entered the public %s %dv%d queue"),
		bRankedRequested ? TEXT("ranked") : TEXT("casual"),
		TeamSize,
		TeamSize);
}

void AFlickGameMode::PreparePartyMigrationToMatch(const FString& TargetSessionId)
{
	if (TargetSessionId.IsEmpty() || !GetWorld())
	{
		return;
	}
	const int32 PartySize = GetPartyMemberCount();
	int32 MigratingMembers = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get());
		AFlickPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AFlickPlayerState>() : nullptr;
		if (!Controller || !PlayerState || PlayerState->GetPartyId().IsEmpty()
			|| PlayerState->GetPartySlot() == INDEX_NONE)
		{
			continue;
		}
		Controller->BeginPartyMatchMigrationFromServer(
			TargetSessionId,
			PlayerState->GetPartyId(),
			PlayerState->GetPartySlot(),
			PartySize,
			PlayerState->IsPartyLeader());
		++MigratingMembers;
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("PARTY_MATCH_HANDOFF: moving %d member(s) into Steam session %s"),
		MigratingMembers,
		*TargetSessionId);
}

void AFlickGameMode::RemovePartyMember(const int32 PartySlot)
{
	if (!bPartyRequested || PartySlot <= 0)
	{
		return;
	}
	AFlickPlayerState* PartyMember = GetPartyMember(PartySlot);
	AFlickPlayerController* PartyController = PartyMember ? Cast<AFlickPlayerController>(PartyMember->GetOwner()) : nullptr;
	if (PartyController && PartyController->GetNetConnection())
	{
		UE_LOG(LogFlick, Log, TEXT("Removing %s from the party"), *PartyMember->GetPlayerName());
		PartyController->ReturnToFrontendFromServer(true);
	}
}

void AFlickGameMode::DisbandParty()
{
	if (!bPartyRequested || !GetWorld())
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* PartyController = Cast<AFlickPlayerController>(It->Get());
		if (PartyController && PartyController->GetNetConnection())
		{
			PartyController->ReturnToFrontendFromServer(true);
		}
	}
	FTimerHandle DisbandTimer;
	GetWorldTimerManager().SetTimer(DisbandTimer, [this]()
	{
		if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
		{
			Sessions->LeaveSession(true);
		}
	}, 0.25f, false);
}

bool AFlickGameMode::CanStartNetworkMatch() const
{
	if (bPrivateMatchSetupActive)
	{
		return CanStartPrivateMatch();
	}
	if (!IsNetworkLobby()
		|| bRankedMatchRegistrationPending
		|| GetLobbyPlayerCount(EFlickTeam::Player1) != CurrentPlayersPerTeam
		|| GetLobbyPlayerCount(EFlickTeam::Player2) != CurrentPlayersPerTeam
		|| (bMatchmakingRequested
			&& GetFlickGameState()
			&& GetFlickGameState()->bMatchmakingTimedOut))
	{
		return false;
	}
	if (bRankedRequested && !AreRankedPlayersAuthenticated())
	{
		return false;
	}
	if (!AreCoordinatorReservationsVerified())
	{
		return false;
	}
	for (int32 PlayerSlot = 0; PlayerSlot < CurrentPlayersPerTeam; ++PlayerSlot)
	{
		const AFlickPlayerState* Player1 = GetLobbyPlayer(EFlickTeam::Player1, PlayerSlot);
		const AFlickPlayerState* Player2 = GetLobbyPlayer(EFlickTeam::Player2, PlayerSlot);
		if (!Player1 || !Player1->IsLobbyReady() || !Player2 || !Player2->IsLobbyReady())
		{
			return false;
		}
	}
	return true;
}

void AFlickGameMode::SetLobbyReady(APlayerController* RequestingPlayer, const bool bReady)
{
	if (!IsNetworkLobby() || !RequestingPlayer)
	{
		return;
	}
	if (AFlickPlayerState* FlickPlayerState = RequestingPlayer->GetPlayerState<AFlickPlayerState>())
	{
		if (FlickPlayerState->GetTeam() != EFlickTeam::None
			|| (bPrivateMatchSetupActive && !FlickPlayerState->GetPrivateControlledSlots().IsEmpty()))
		{
			FlickPlayerState->SetLobbyReady(bReady);
			UE_LOG(LogFlick, Log, TEXT("%s is %s in the network lobby"), *FlickPlayerState->GetPlayerName(), bReady ? TEXT("ready") : TEXT("not ready"));
			TryStartNetworkMatch();
		}
	}
}

void AFlickGameMode::StartNetworkMatch(APlayerController* RequestingPlayer)
{
	const AFlickPlayerState* RequestingState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (bPrivateMatchSetupActive)
	{
		StartPrivateMatch(RequestingPlayer);
		return;
	}
	if (!RequestingState
		|| RequestingState->GetTeam() != EFlickTeam::Player1
		|| RequestingState->GetTeamPlayerSlot() != 0
		|| !CanStartNetworkMatch())
	{
		UE_LOG(LogFlick, Warning, TEXT("Network match start rejected: host and both ready players are required"));
		return;
	}

	if (bRankedRequested)
	{
		RegisterRankedMatchThenStart();
		return;
	}
	CompleteNetworkMatchStart(CoordinatorMatchId.IsEmpty()
		? FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens)
		: CoordinatorMatchId);
}

void AFlickGameMode::SubmitRankedAuthentication(
	APlayerController* RequestingPlayer,
	const FString& SteamAuthTicket)
{
	AFlickPlayerState* PlayerState = RequestingPlayer
		? RequestingPlayer->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	UFlickRankedBackendSubsystem* Backend = GetFlickRankedBackendSubsystem();
	if (!bRankedRequested || !IsNetworkLobby() || !RequestingPlayer || !PlayerState || !Backend)
	{
		return;
	}
	const FString AccountId = GetPlayerOnlineId(PlayerState);
	Backend->AuthenticatePlayer(
		AccountId,
		SteamAuthTicket,
		SelectedMatchVariant,
		CurrentPlayersPerTeam,
		[WeakThis = TWeakObjectPtr<AFlickGameMode>(this), WeakController = TWeakObjectPtr<APlayerController>(RequestingPlayer)](
			const FFlickRankedAuthenticationResult& Result)
		{
			AFlickGameMode* GameMode = WeakThis.Get();
			APlayerController* Controller = WeakController.Get();
			AFlickPlayerState* RankedPlayerState = Controller
				? Controller->GetPlayerState<AFlickPlayerState>()
				: nullptr;
			if (!GameMode || !Controller || !RankedPlayerState || !GameMode->IsNetworkLobby())
			{
				return;
			}
			if (!Result.bAuthenticated)
			{
				RankedPlayerState->SetRankedIdentity(false, FlickRankRules::DefaultRating);
				UE_LOG(
					LogFlick,
					Error,
					TEXT("RANKED_AUTH_REJECTED: player=%s error=%s"),
					*RankedPlayerState->GetPlayerName(),
					*Result.Error);
				return;
			}
			GameMode->PlayerRankedRatings.Add(Controller, Result.Progress.Rating);
			RankedPlayerState->SetRankedIdentity(true, Result.Progress.Rating);
			if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(Controller))
			{
				FlickController->ApplyTrustedRankedProgressFromServer(
					GameMode->SelectedMatchVariant,
					GameMode->CurrentPlayersPerTeam,
					Result.Progress);
			}
			UE_LOG(
				LogFlick,
				Log,
				TEXT("RANKED_AUTH_ACCEPTED: account=%s rating=%d"),
				*Result.AccountId,
				Result.Progress.Rating);
			GameMode->TryStartNetworkMatch();
		});
}

bool AFlickGameMode::AreRankedPlayersAuthenticated() const
{
	if (!bRankedRequested || !GetWorld())
	{
		return !bRankedRequested;
	}
	int32 VerifiedPlayers = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AFlickPlayerState* PlayerState = It->Get()
			? It->Get()->GetPlayerState<AFlickPlayerState>()
			: nullptr;
		if (PlayerState && PlayerState->GetTeam() != EFlickTeam::None)
		{
			if (!PlayerState->IsRankedIdentityVerified())
			{
				return false;
			}
			++VerifiedPlayers;
		}
	}
	return VerifiedPlayers == CurrentPlayersPerTeam * 2;
}

FFlickRankedMatchRequest AFlickGameMode::BuildRankedMatchRequest(const FString& MatchId) const
{
	FFlickRankedMatchRequest Request;
	Request.MatchId = MatchId;
	Request.Variant = SelectedMatchVariant;
	Request.PlayersPerTeam = CurrentPlayersPerTeam;
	Request.StartedUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
	if (const UFlickRankedBackendSubsystem* Backend = GetFlickRankedBackendSubsystem())
	{
		Request.SeasonId = Backend->GetSeasonId();
	}
	if (!GetWorld())
	{
		return Request;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		const AFlickPlayerState* PlayerState = Controller
			? Controller->GetPlayerState<AFlickPlayerState>()
			: nullptr;
		if (!PlayerState || PlayerState->GetTeam() == EFlickTeam::None)
		{
			continue;
		}
		FFlickRankedParticipant Participant;
		Participant.AccountId = GetPlayerOnlineId(PlayerState);
		Participant.Team = PlayerState->GetTeam();
		Participant.PlayerSlot = PlayerState->GetTeamPlayerSlot();
		Participant.Rating = PlayerState->GetAuthoritativeRankedRating();
		Request.Participants.Add(MoveTemp(Participant));
	}
	return Request;
}

void AFlickGameMode::RegisterRankedMatchThenStart()
{
	UFlickRankedBackendSubsystem* Backend = GetFlickRankedBackendSubsystem();
	if (!Backend || bRankedMatchRegistrationPending || !CanStartNetworkMatch())
	{
		return;
	}
	bRankedMatchRegistrationPending = true;
	const FFlickRankedMatchRequest Request = BuildRankedMatchRequest(
		CoordinatorMatchId.IsEmpty()
			? FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens)
			: CoordinatorMatchId);
	Backend->RegisterMatch(
		Request,
		[WeakThis = TWeakObjectPtr<AFlickGameMode>(this), Request](const bool bAccepted, const FString& Error)
		{
			AFlickGameMode* GameMode = WeakThis.Get();
			if (!GameMode)
			{
				return;
			}
			GameMode->bRankedMatchRegistrationPending = false;
			if (!bAccepted || !GameMode->CanStartNetworkMatch())
			{
				UE_LOG(
					LogFlick,
					Error,
					TEXT("RANKED_MATCH_REGISTRATION_REJECTED: %s"),
					Error.IsEmpty() ? TEXT("lobby changed while registering") : *Error);
				return;
			}
			GameMode->ActiveRankedMatchRequest = Request;
			GameMode->bHasActiveRankedMatchRequest = true;
			GameMode->CompleteNetworkMatchStart(Request.MatchId);
		});
}

void AFlickGameMode::CompleteNetworkMatchStart(const FString& MatchId)
{
	if (!CanStartNetworkMatch() || MatchId.IsEmpty())
	{
		return;
	}
	const bool bStartingPrivateMatch = bPrivateMatchSetupActive;
	bNetworkMatchStarted = true;
	if (bStartingPrivateMatch)
	{
		bPrivateMatchSetupActive = false;
		bPrivateMatchActive = true;
		bPartyRequested = false;
	}
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetNetworkLobbyState(false, SelectedMatchVariant);
		FlickGameState->SetPrivateMatchLobbyState(false, PrivateMatchSettings);
		FlickGameState->SetPartyState(false, FString(), FlickMaximumPartyMembers);
		FlickGameState->BeginAuthoritativeMatch(MatchId);
	}
	if (SelectedMatchVariant == EFlickMatchVariant::Classic)
	{
		BeginNetworkClassSelection();
	}
	else
	{
		StartSelectedMatch();
		BeginRankedMatchForPlayers();
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("NETWORK_MATCH_READY: authority started match %s"),
		*MatchId);
}

void AFlickGameMode::ResetLobbyReadiness()
{
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		for (APlayerState* PlayerState : FlickGameState->PlayerArray)
		{
			if (AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(PlayerState))
			{
				FlickPlayerState->SetLobbyReady(false);
			}
		}
	}
}

void AFlickGameMode::CancelNetworkLobby()
{
	if (!bNetworkMatchRequested || !GetWorld())
	{
		return;
	}
	if (bMatchmakingRequested && GetPartyMemberCount() > 0)
	{
		if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem(); Sessions && Sessions->IsPartySession())
		{
			Sessions->CancelMatchmaking(false);
		}
		RestorePremadePartyAfterMatch();
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(It->Get());
		if (FlickController && FlickController->GetNetConnection())
		{
			FlickController->ReturnToFrontendFromServer();
		}
	}

	const FName CurrentMap(*GetWorld()->GetOutermost()->GetName());
	FTimerHandle ReturnTimer;
	GetWorldTimerManager().SetTimer(ReturnTimer, [this, CurrentMap]()
	{
		if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem(); Sessions && Sessions->HasActiveSession())
		{
			Sessions->LeaveSession(true);
		}
		else
		{
			UGameplayStatics::OpenLevel(this, CurrentMap, true);
		}
	}, 0.25f, false);
}

void AFlickGameMode::ReturnToNetworkLobby()
{
	if (!bNetworkMatchRequested)
	{
		return;
	}
	// This path is commonly entered through the paused in-game menu. If the
	// world remains paused, the frontend UI changes but the arena showcase and
	// camera stay frozen on the last gameplay frame.
	UGameplayStatics::SetGamePaused(this, false);
	ClearControllerAiming();
	if (bMatchmakingRequested)
	{
		RestorePremadePartyAfterMatch();
		return;
	}
	if (bPrivateMatchActive)
	{
		bNetworkMatchStarted = false;
		bPrivateMatchActive = false;
		bPrivateMatchSetupActive = true;
		bPartyRequested = GetPartyMemberCount() > 0;
		FrontendScreen = EFlickFrontendScreen::PrivateMatch;
		ResetPrivateMatchReadiness();
		SetCameraForFrontend();
		ApplyMatchConfiguration(EFlickMatchVariant::Classic, PrivateMatchSettings.PlayersPerTeam);
		RebuildMatch();
		PushPrivateMatchState();
		if (CameraPawn)
		{
			CameraPawn->ApplyCameraSettings();
		}
		return;
	}
	bNetworkMatchStarted = false;
	FrontendScreen = EFlickFrontendScreen::NetworkLobby;
	ResetLobbyReadiness();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	SetCameraForFrontend();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
		FlickGameState->SetNetworkLobbyState(true, SelectedMatchVariant);
	}
}

void AFlickGameMode::RestorePremadePartyAfterMatch()
{
	if (!bMatchmakingRequested || !GetWorld())
	{
		return;
	}

	FString HostPartyId;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get());
		const AFlickPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AFlickPlayerState>() : nullptr;
		if (Controller && !Controller->GetNetConnection() && PlayerState)
		{
			HostPartyId = PlayerState->GetPartyId();
			break;
		}
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get());
		AFlickPlayerState* PlayerState = Controller ? Controller->GetPlayerState<AFlickPlayerState>() : nullptr;
		if (!Controller || !PlayerState || !Controller->GetNetConnection())
		{
			continue;
		}
		if (!PlayerState->GetPartyId().IsEmpty() && PlayerState->GetPartyId() != HostPartyId)
		{
			Controller->BeginPartyRestoreFromServer(PlayerState->IsPartyLeader());
		}
		else if (PlayerState->GetPartyId().IsEmpty())
		{
			Controller->ReturnToFrontendFromServer();
		}
	}

	const int32 HostPremadeSize = GetPartyMemberCountForId(HostPartyId);
	if (HostPartyId.IsEmpty() || HostPremadeSize <= 0)
	{
		bMatchmakingRequested = false;
		bNetworkMatchRequested = false;
		FTimerHandle SoloReturnTimer;
		GetWorldTimerManager().SetTimer(SoloReturnTimer, [this]()
		{
			if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem(); Sessions && Sessions->HasActiveSession())
			{
				Sessions->LeaveSession(true);
			}
			else if (GetWorld())
			{
				UGameplayStatics::OpenLevel(this, FName(*GetWorld()->GetOutermost()->GetName()), true);
			}
		}, 0.25f, false);
		return;
	}

	UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
	if (Sessions && Sessions->HasActiveSession()
		&& !Sessions->IsPartySession()
		&& !Sessions->ConvertMatchmakingToParty(FlickMaximumPartyMembers))
	{
		UE_LOG(LogFlick, Error, TEXT("Could not restore the premade party after matchmaking"));
		return;
	}

	AFlickPlayerState* PartyLeader = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(It->Get());
		AFlickPlayerState* PlayerState = FlickController ? FlickController->GetPlayerState<AFlickPlayerState>() : nullptr;
		if (!FlickController || !PlayerState || PlayerState->GetPartyId() != HostPartyId)
		{
			continue;
		}
		PlayerState->SetTeam(EFlickTeam::None);
		PlayerState->SetTeamPlayerSlot(INDEX_NONE);
		PlayerState->SetLobbyReady(false);
		if (PlayerState->IsPartyLeader())
		{
			PartyLeader = PlayerState;
		}
	}
	PremadePartyTeams.Reset();
	PremadePartySlots.Reset();
	PlayerRankedRatings.Reset();
	bHasActiveRankedMatchRequest = false;
	bRankedMatchRegistrationPending = false;
	ActiveRankedMatchRequest = FFlickRankedMatchRequest();

	bNetworkMatchRequested = false;
	bNetworkMatchStarted = false;
	bMatchmakingRequested = false;
	bRankedRequested = false;
	bRankedResultsDispatched = false;
	bMatchmakingLobbyLocked = false;
	bPartyRequested = true;
	MatchmakingQueueElapsed = 0.0f;
	MatchFoundConfirmationElapsed = 0.0f;
	FrontendScreen = EFlickFrontendScreen::MainMenu;
	ClearControllerAiming();
	ApplySelectedMatchConfiguration();
	RebuildMatch();
	SetCameraForFrontend();
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetNetworkLobbyState(false, SelectedMatchVariant);
		State->SetMatchmakingState(false);
		State->SetPartyState(true, PartyLeader ? GetPlayerOnlineId(PartyLeader) : FString(), FlickMaximumPartyMembers);
		State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	UE_LOG(LogFlick, Log, TEXT("MATCHMAKING_PARTY_RESTORED: %d host-party member(s) returned together"), HostPremadeSize);
}

void AFlickGameMode::FinalizeDisconnectedPlayerForfeit(const AFlickPlayerState* ExitingState)
{
	if (!ExitingState)
	{
		return;
	}
	FinalizeDisconnectedTeamForfeit(ExitingState->GetTeam());
}

void AFlickGameMode::FinalizeDisconnectedTeamForfeit(const EFlickTeam ExitingTeam)
{
	AFlickGameState* State = GetFlickGameState();
	if (!State || State->bMatchResultFinalized || ExitingTeam == EFlickTeam::None)
	{
		return;
	}
	State->CompleteMatchByForfeit(ExitingTeam);
	DispatchRankedMatchResults();
	if (UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem())
	{
		Coordinator->NotifyServerMatchComplete(State->FinalMatchOutcome, true);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("AUTHORITATIVE_MATCH_RESULT: id=%s outcome=%d forfeit=1"),
		*State->MatchId,
		static_cast<int32>(State->FinalMatchOutcome));
}

void AFlickGameMode::UpdateReplicatedMatchmakingState(const bool bTimedOut) const
{
	AFlickGameState* State = GetFlickGameState();
	if (!State)
	{
		return;
	}
	const UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem();
	State->SetMatchmakingState(
		bMatchmakingRequested,
		bTimedOut,
		bRankedRequested,
		RankedQueueRating,
		bRankedRequested && Sessions ? Sessions->GetRankedSearchRange() : 0);
}

int32 AFlickGameMode::GetAverageRankedRating(const EFlickTeam Team) const
{
	if (!GetWorld() || Team == EFlickTeam::None)
	{
		return RankedQueueRating;
	}
	int32 TotalRating = 0;
	int32 PlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		const AFlickPlayerState* PlayerState = Controller
			? Controller->GetPlayerState<AFlickPlayerState>()
			: nullptr;
		if (!PlayerState || PlayerState->GetTeam() != Team)
		{
			continue;
		}
		TotalRating += PlayerRankedRatings.Contains(Controller)
			? PlayerRankedRatings.FindRef(Controller)
			: RankedQueueRating;
		++PlayerCount;
	}
	return PlayerCount > 0
		? FMath::RoundToInt(static_cast<float>(TotalRating) / PlayerCount)
		: RankedQueueRating;
}

void AFlickGameMode::BeginRankedMatchForPlayers()
{
	bRankedResultsDispatched = false;
	AFlickGameState* State = GetFlickGameState();
	if (!bRankedRequested || !State || State->MatchId.IsEmpty() || !GetWorld())
	{
		return;
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get());
		const AFlickPlayerState* PlayerState = Controller
			? Controller->GetPlayerState<AFlickPlayerState>()
			: nullptr;
		if (!Controller || !PlayerState || PlayerState->GetTeam() == EFlickTeam::None)
		{
			continue;
		}
		Controller->BeginRankedMatchFromServer(
			State->MatchId,
			State->ActiveMatchVariant,
			CurrentPlayersPerTeam,
			GetAverageRankedRating(GetOpposingTeam(PlayerState->GetTeam())));
	}
}

void AFlickGameMode::DispatchRankedMatchResults()
{
	AFlickGameState* State = GetFlickGameState();
	if (!bRankedRequested || bRankedResultsDispatched || !State
		|| !State->bMatchResultFinalized || State->FinalMatchOutcome == EFlickMatchOutcome::Continue
		|| !GetWorld() || !bHasActiveRankedMatchRequest
		|| ActiveRankedMatchRequest.MatchId != State->MatchId)
	{
		return;
	}
	UFlickRankedBackendSubsystem* Backend = GetFlickRankedBackendSubsystem();
	if (!Backend)
	{
		UE_LOG(LogFlick, Error, TEXT("RANKED_SETTLEMENT_FAILED: no ranked backend subsystem"));
		return;
	}
	bRankedResultsDispatched = true;
	FFlickRankedMatchResultRequest ResultRequest;
	ResultRequest.Match = ActiveRankedMatchRequest;
	ResultRequest.Outcome = State->FinalMatchOutcome;
	ResultRequest.bForfeit = State->bMatchEndedByForfeit;
	ResultRequest.CompletedUnixTime = State->MatchCompletedUnixTime;
	Backend->SubmitMatchResult(
		ResultRequest,
		[WeakThis = TWeakObjectPtr<AFlickGameMode>(this), MatchId = State->MatchId](
			const FFlickRankedSettlementResult& Result)
		{
			AFlickGameMode* GameMode = WeakThis.Get();
			if (!GameMode)
			{
				return;
			}
			if (!Result.bAccepted)
			{
				GameMode->bRankedResultsDispatched = false;
				UE_LOG(
					LogFlick,
					Error,
					TEXT("RANKED_SETTLEMENT_FAILED: match=%s error=%s"),
					*MatchId,
					*Result.Error);
				if (UWorld* World = GameMode->GetWorld())
				{
					FTimerHandle RetryTimer;
					World->GetTimerManager().SetTimer(RetryTimer, [WeakThis]()
					{
						if (AFlickGameMode* RetryGameMode = WeakThis.Get())
						{
							RetryGameMode->DispatchRankedMatchResults();
						}
					}, 10.0f, false);
				}
				return;
			}
			for (const FFlickRankedPlayerUpdate& PlayerUpdate : Result.PlayerUpdates)
			{
				for (FConstPlayerControllerIterator It = GameMode->GetWorld()->GetPlayerControllerIterator(); It; ++It)
				{
					AFlickPlayerController* Controller = Cast<AFlickPlayerController>(It->Get());
					AFlickPlayerState* PlayerState = Controller
						? Controller->GetPlayerState<AFlickPlayerState>()
						: nullptr;
					if (Controller && PlayerState
						&& GetPlayerOnlineId(PlayerState) == PlayerUpdate.AccountId)
					{
						PlayerState->SetRankedIdentity(true, PlayerUpdate.RatingUpdate.NewRating);
						Controller->ApplyTrustedRankedUpdateFromServer(PlayerUpdate.RatingUpdate);
						break;
					}
				}
			}
			UE_LOG(
				LogFlick,
				Log,
				TEXT("RANKED_SETTLEMENT_ACCEPTED: match=%s players=%d duplicate=%d"),
				*MatchId,
				Result.PlayerUpdates.Num(),
				Result.bDuplicate ? 1 : 0);
		});
}

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
	bClassSelectionStartsTrainingBotMatch = false;
	bTestArenaMode = false;
	ResetTrainingBotThinking();
	bPlayerClassesActiveForMatch = false;
	bClassSelectionForNextRound = false;
	bHasPendingClassChanges = false;
	Player1PendingClasses.Reset();
	Player2PendingClasses.Reset();
	ClearControllerAiming();
	MenuPreviewElapsed = 0.0f;
	MenuPreviewVariant = SelectedMatchVariant;
	MenuPreviewPlayersPerTeam = CurrentPlayersPerTeam;
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

	const TArray<EWindowMode::Type> Modes = {
		EWindowMode::Windowed,
		EWindowMode::WindowedFullscreen,
		EWindowMode::Fullscreen
	};
	int32 CurrentIndex = Modes.IndexOfByKey(Settings->GetFullscreenMode());
	CurrentIndex = CurrentIndex == INDEX_NONE ? 0 : CurrentIndex;
	const int32 Step = Direction < 0 ? -1 : 1;
	Settings->SetFullscreenMode(Modes[(CurrentIndex + Step + Modes.Num()) % Modes.Num()]);
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
			? FLinearColor(0.78f, 0.87f, 1.0f)
			: bClassicArenaLighting
				? FLinearColor(0.82f, 0.88f, 0.96f)
				: FLinearColor(0.9f, 0.94f, 1.0f));
		Light->SetIntensity(bTestArenaMode ? 1.1f : bClassicArenaLighting ? 0.78f : 1.15f);
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
		Sky->SetIntensity(bTestArenaMode ? 0.82f : bClassicArenaLighting ? 0.34f : 0.28f);
	}

	const auto SpawnAccentLight = [this, bClassicArenaLighting](
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
			// Classic formations sit directly below these fixtures. Keep them as a
			// soft neutral wash so their highlights cannot bleach the puck markings.
			Light->SetLightColor(FMath::Lerp(
				Color, FLinearColor::White,
				bTestArenaMode ? 0.38f : bClassicArenaLighting ? 0.88f : 0.72f));
			Light->SetIntensity(bTestArenaMode ? 220.0f : bClassicArenaLighting ? 52.0f : 165.0f);
			Light->SetAttenuationRadius(
				(bClassicArenaLighting ? 720.0f : 820.0f)
				* ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
			Light->SetSourceRadius((bTestArenaMode ? 125.0f : bClassicArenaLighting ? 260.0f : 120.0f)
				* ArenaRadius / FlickModeRules::Get(EFlickMatchVariant::Classic).ArenaRadius);
			Light->SetSpecularScale(bTestArenaMode ? 0.52f : bClassicArenaLighting ? 0.04f : 0.48f);
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
		ArenaFillLight->PointLightComponent->SetIntensity(bTestArenaMode ? 300.0f : bClassicArenaLighting ? 112.0f : 190.0f);
		ArenaFillLight->PointLightComponent->SetAttenuationRadius(1280.0f * ArenaScale);
		ArenaFillLight->PointLightComponent->SetSourceRadius((bTestArenaMode ? 240.0f : 180.0f) * ArenaScale);
		ArenaFillLight->PointLightComponent->SetSpecularScale(bTestArenaMode ? 0.38f : bClassicArenaLighting ? 0.06f : 0.26f);
		ArenaFillLight->PointLightComponent->SetIndirectLightingIntensity(bClassicArenaLighting ? 0.45f : 0.34f);
		ArenaFillLight->PointLightComponent->SetCastShadows(false);
	}
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
	if (bTestArenaMode)
	{
		TestArenaActor = GetWorld()->SpawnActor<AFlickTestArena>(
			AFlickTestArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		ArenaActor = TestArenaActor;
	}
	else
	{
		TestArenaActor = nullptr;
		ArenaActor = GetWorld()->SpawnActor<AFlickArena>(
			AFlickArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
	if (ArenaActor)
	{
		if (TestArenaActor)
		{
			TestArenaActor->InitializeTestArena(ArenaRadius, ArenaThickness, ArenaSurfaceZ);
		}
		else
		{
			ArenaActor->InitializeArena(ArenaRadius, ArenaThickness, ArenaSurfaceZ, CurrentPlayersPerTeam);
		}
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
		true);
	Player2BobStriker = SpawnPiece(
		EFlickTeam::Player2,
		RackIndex + 2,
		BobArenaActor->GetStrikerStart(EFlickTeam::Player2, PieceThickness),
		EFlickPieceArchetype::Standard,
		true);
}

void AFlickGameMode::DestroyPieces()
{
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

EFlickTeam AFlickGameMode::FindAvailableTeam(const APlayerController* PlayerToIgnore) const
{
	EFlickTeam Team = EFlickTeam::None;
	int32 PlayerSlot = INDEX_NONE;
	FindAvailableTeamAndSlot(PlayerToIgnore, Team, PlayerSlot);
	return Team;
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
		if (IsFreePlayTraining())
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

void AFlickGameMode::UpdateMainMenuPreview(const float DeltaSeconds)
{
	if (FrontendScreen != EFlickFrontendScreen::MainMenu || MenuPreviewDuration <= 0.0f)
	{
		bMenuPreviewTransitionActive = false;
		bMenuPreviewSwapApplied = false;
		MenuPreviewTransitionElapsed = 0.0f;
		return;
	}

	if (bMenuPreviewTransitionActive)
	{
		MenuPreviewTransitionElapsed += DeltaSeconds;
		const float HalfDuration = MenuPreviewTransitionDuration * 0.5f;
		if (!bMenuPreviewSwapApplied && MenuPreviewTransitionElapsed >= HalfDuration)
		{
			bMenuPreviewSwapApplied = true;
			CommitModePreview(PendingMenuPreviewVariant, PendingMenuPreviewPlayersPerTeam);
		}
		if (MenuPreviewTransitionElapsed >= MenuPreviewTransitionDuration)
		{
			bMenuPreviewTransitionActive = false;
			bMenuPreviewSwapApplied = false;
			MenuPreviewTransitionElapsed = 0.0f;
		}
		return;
	}

	MenuPreviewElapsed += DeltaSeconds;
	if (MenuPreviewElapsed < MenuPreviewDuration)
	{
		return;
	}

	EFlickMatchVariant NextVariant = EFlickMatchVariant::Classic;
	int32 NextPlayersPerTeam = 1;
	if (MenuPreviewVariant == EFlickMatchVariant::Classic && MenuPreviewPlayersPerTeam < 3)
	{
		NextPlayersPerTeam = MenuPreviewPlayersPerTeam + 1;
	}
	else if (MenuPreviewVariant == EFlickMatchVariant::Classic)
	{
		NextVariant = EFlickMatchVariant::Bob;
	}
	else
	{
		NextVariant = EFlickMatchVariant::Classic;
	}
	PendingMenuPreviewVariant = NextVariant;
	PendingMenuPreviewPlayersPerTeam = NextPlayersPerTeam;
	bMenuPreviewTransitionActive = true;
	bMenuPreviewSwapApplied = false;
	MenuPreviewTransitionElapsed = 0.0f;
}

void AFlickGameMode::ShowModePreview(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam)
{
	bMenuPreviewTransitionActive = false;
	bMenuPreviewSwapApplied = false;
	MenuPreviewTransitionElapsed = 0.0f;
	CommitModePreview(Variant, PlayersPerTeam);
}

void AFlickGameMode::CommitModePreview(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam)
{
	MenuPreviewVariant = NormalizeMatchVariant(Variant);
	MenuPreviewPlayersPerTeam = MenuPreviewVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	MenuPreviewElapsed = 0.0f;
	ApplyMatchConfiguration(MenuPreviewVariant, MenuPreviewPlayersPerTeam);
	RebuildMatch();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	SetCameraForFrontend();
	UE_LOG(
		LogFlick,
		Log,
		TEXT("Main menu preview changed to %s %dv%d"),
		*GetMatchVariantName(MenuPreviewVariant),
		MenuPreviewPlayersPerTeam,
		MenuPreviewPlayersPerTeam);
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
	}
}

void AFlickGameMode::SetCameraViewForTeam(const EFlickTeam Team, const bool bSnap)
{
	if (!CameraPawn || Team == EFlickTeam::None)
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

void AFlickGameMode::BeginResolutionTracking(
	const EFlickTeam ShootingTeam,
	const bool bSimultaneousShot,
	AFlickPiece* ShotPiece)
{
	if (bCinematicReplayActive)
	{
		FinishCinematicRoundReplay(false);
	}
	RoundReplayFrames.Reset();
	ReplayShotSetups.Reset();
	LastReplayCaptureTime = -100.0f;
	bReplayPlayedForResolution = false;
	ResolutionElapsed = 0.0f;
	ResolutionPlayer1Eliminated = 0;
	ResolutionPlayer2Eliminated = 0;
	ResolutionImpactCount = 0;
	ResolutionShootingTeam = ShootingTeam;
	bResolutionWasSimultaneous = bSimultaneousShot;
	ResolutionShotPieceId = ShotPiece ? ShotPiece->GetPieceId() : INDEX_NONE;
	ResolutionShotStart = ShotPiece
		? FVector2D(ShotPiece->GetActorLocation().X, ShotPiece->GetActorLocation().Y)
		: FVector2D::ZeroVector;
	ResolutionActivatedSwitchMask = 0;
	ResolutionNewlyRaisedDividerMask = 0;
	ResolutionInitialPieceLocations.Reset();
	ResolutionContactDepths.Reset();
	ResolutionDirectContactPieceIds.Reset();
	ResolutionEliminatedPieceIds.Reset();
	ResolutionDividerContactPieceIds.Reset();
	ResolutionNewDividerContactPieceIds.Reset();
	ResolutionFirstImpactTimes.Reset();
	ResolutionFirstOpponentImpactTimes.Reset();
	ResolutionEliminationTimes.Reset();
	ReplayPresentedEliminationPieceIds.Reset();
	ReplayPrimaryFocusPieceId = INDEX_NONE;
	ReplayKnockoutFocusPieceId = INDEX_NONE;
	ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		bResolutionBuzzerRelease = !bSimultaneousShot
			&& FlickGameState->bShotClockActive
			&& FlickGameState->GetShotClockTimeRemaining() <= BuzzerBeaterTimeThreshold;
		FlickGameState->ClearDramaticEvent();
	}
	else
	{
		bResolutionBuzzerRelease = false;
	}
	for (const AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive())
		{
			const FVector Location = Piece->GetActorLocation();
			ResolutionInitialPieceLocations.Add(Piece->GetPieceId(), FVector2D(Location.X, Location.Y));
		}
	}
	if (ResolutionShotPieceId != INDEX_NONE)
	{
		ResolutionContactDepths.Add(ResolutionShotPieceId, 0);
	}
	CaptureRoundReplayFrame(true);
}

float AFlickGameMode::GetCinematicReplayProgress() const
{
	return CinematicReplayPlaybackDuration > KINDA_SMALL_NUMBER
		? FMath::Clamp(CinematicReplayElapsed / CinematicReplayPlaybackDuration, 0.0f, 1.0f)
		: 0.0f;
}

bool AFlickGameMode::IsCinematicReplayPullbackActive() const
{
	return bCinematicReplayActive && CinematicReplayElapsed < ReplayPullbackDuration;
}

float AFlickGameMode::GetCinematicReplayPullbackAlpha() const
{
	if (!bCinematicReplayActive)
	{
		return 0.0f;
	}
	if (!IsCinematicReplayPullbackActive())
	{
		return 1.0f;
	}
	const float BuildDuration = FMath::Max(ReplayPullbackDuration * 0.7f, 0.01f);
	const float Alpha = FMath::Clamp(CinematicReplayElapsed / BuildDuration, 0.0f, 1.0f);
	return Alpha * Alpha * (3.0f - 2.0f * Alpha);
}

const AFlickPiece* AFlickGameMode::GetCinematicReplayShotPiece(const int32 ShotIndex) const
{
	return ReplayShotSetups.IsValidIndex(ShotIndex) ? ReplayShotSetups[ShotIndex].Piece.Get() : nullptr;
}

FVector AFlickGameMode::GetCinematicReplayShotDirection(const int32 ShotIndex) const
{
	return ReplayShotSetups.IsValidIndex(ShotIndex) ? ReplayShotSetups[ShotIndex].Direction : FVector::ZeroVector;
}

float AFlickGameMode::GetCinematicReplayShotPower(const int32 ShotIndex) const
{
	return ReplayShotSetups.IsValidIndex(ShotIndex) ? ReplayShotSetups[ShotIndex].Power : 0.0f;
}

void AFlickGameMode::CaptureTestArenaControlZones()
{
	if (bTestArenaMode && TestArenaActor && IsValid(TestArenaActor))
	{
		TestArenaActor->BeginControlZoneTracking(Pieces);
	}
}

void AFlickGameMode::TrackTestArenaControlZones(const float DeltaSeconds)
{
	if (bTestArenaMode && IsValid(TestArenaActor))
	{
		uint8 DeployedMechanisms = 0;
		ResolutionActivatedSwitchMask |= TestArenaActor->TrackControlZoneCrossings(Pieces, DeltaSeconds, DeployedMechanisms);
		for (int32 Index = 0; Index < TestArenaActor->GetMechanismCount(); ++Index)
		{
			const uint8 Bit = static_cast<uint8>(1 << Index);
			if ((DeployedMechanisms & Bit) != 0 && TestArenaActor->IsDividerRaised(Index))
			{
				ResolutionNewlyRaisedDividerMask |= Bit;
			}
		}
	}
}

void AFlickGameMode::ResolveTestArenaControlZones()
{
	if (bTestArenaMode && IsValid(TestArenaActor))
	{
		TestArenaActor->CommitPendingControlZoneToggles(Pieces);
	}
}

void AFlickGameMode::CaptureRoundReplayFrame(const bool bForce)
{
	if (!bTestArenaMode || bCinematicReplayActive || !TestArenaActor || !IsValid(TestArenaActor))
	{
		return;
	}

	const float CaptureInterval = 1.0f / FMath::Max(ReplayCaptureRate, 1.0f);
	if (!bForce && ResolutionElapsed - LastReplayCaptureTime < CaptureInterval)
	{
		return;
	}
	if (bForce && !RoundReplayFrames.IsEmpty()
		&& FMath::IsNearlyEqual(RoundReplayFrames.Last().Time, ResolutionElapsed, KINDA_SMALL_NUMBER))
	{
		RoundReplayFrames.Pop(EAllowShrinking::No);
	}

	FFlickRoundReplayFrame& Frame = RoundReplayFrames.AddDefaulted_GetRef();
	Frame.Time = FMath::Max(0.0f, ResolutionElapsed);
	Frame.RaisedDividerMask = TestArenaActor->GetRaisedDividerMask();
	Frame.Pieces.Reserve(Pieces.Num());
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece))
		{
			continue;
		}
		FFlickReplayPieceState& PieceState = Frame.Pieces.AddDefaulted_GetRef();
		PieceState.Piece = Piece;
		PieceState.Transform = Piece->GetActorTransform();
		PieceState.bVisible = Piece->IsActive();
	}
	LastReplayCaptureTime = Frame.Time;
}

void AFlickGameMode::BeginCinematicRoundReplay(const EFlickMatchOutcome Outcome)
{
	if (!bTestArenaMode || bCinematicReplayActive || bReplayPlayedForResolution
		|| !TestArenaActor || !IsValid(TestArenaActor) || RoundReplayFrames.Num() < 2
		|| (Outcome != EFlickMatchOutcome::Player1Wins && Outcome != EFlickMatchOutcome::Player2Wins))
	{
		return;
	}

	const float FirstTime = RoundReplayFrames[0].Time;
	const float LastTime = RoundReplayFrames.Last().Time;
	CinematicReplaySourceStart = FirstTime;
	CinematicReplaySourceDuration = FMath::Max(LastTime - FirstTime, 0.01f);
	CinematicReplayMotionDuration = FMath::Clamp(
		CinematicReplaySourceDuration / FMath::Clamp(ReplaySlowMotionRate, 0.25f, 1.0f),
		1.25f,
		FMath::Max(ReplayMaximumPlaybackDuration, 2.0f));
	CinematicReplayPlaybackDuration = ReplayPullbackDuration + CinematicReplayMotionDuration;
	CinematicReplayElapsed = 0.0f;
	PendingReplayOutcome = Outcome;
	bCinematicReplayActive = true;
	bReplayPlayedForResolution = true;
	bReplayLaunchCuePlayed = false;
	ReplayPresentedEliminationPieceIds.Reset();
	const EFlickTeam WinningTeam = Outcome == EFlickMatchOutcome::Player1Wins
		? EFlickTeam::Player1 : EFlickTeam::Player2;
	const EFlickTeam LosingTeam = GetOpposingTeam(WinningTeam);
	ReplayPrimaryFocusPieceId = ResolutionShotPieceId;
	if (ReplayPrimaryFocusPieceId == INDEX_NONE)
	{
		const FFlickReplayShotSetup* WinningShot = ReplayShotSetups.FindByPredicate([WinningTeam](const FFlickReplayShotSetup& Shot)
		{
			return Shot.Piece.IsValid() && Shot.Piece->GetTeam() == WinningTeam;
		});
		if (!WinningShot && !ReplayShotSetups.IsEmpty())
		{
			WinningShot = &ReplayShotSetups[0];
		}
		ReplayPrimaryFocusPieceId = WinningShot && WinningShot->Piece.IsValid()
			? WinningShot->Piece->GetPieceId() : INDEX_NONE;
	}

	float LatestLosingElimination = -1.0f;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || Piece->GetTeam() != LosingTeam
			|| !ResolutionEliminatedPieceIds.Contains(Piece->GetPieceId()))
		{
			continue;
		}
		const float EliminationTime = ResolutionEliminationTimes.FindRef(Piece->GetPieceId());
		if (EliminationTime >= LatestLosingElimination)
		{
			LatestLosingElimination = EliminationTime;
			ReplayKnockoutFocusPieceId = Piece->GetPieceId();
		}
	}

	const FFlickReplayShotSetup* SelfKnockoutShot = ReplayShotSetups.FindByPredicate([this, LosingTeam](const FFlickReplayShotSetup& Shot)
	{
		if (!Shot.Piece.IsValid() || Shot.Piece->GetTeam() != LosingTeam
			|| !ResolutionEliminatedPieceIds.Contains(Shot.Piece->GetPieceId()))
		{
			return false;
		}
		return !bResolutionWasSimultaneous
			|| !ResolutionFirstOpponentImpactTimes.Contains(Shot.Piece->GetPieceId());
	});
	if (SelfKnockoutShot && SelfKnockoutShot->Piece.IsValid())
	{
		ReplayPrimaryFocusPieceId = SelfKnockoutShot->Piece->GetPieceId();
	}

	const AFlickPiece* PrimaryPiece = nullptr;
	if (const TObjectPtr<AFlickPiece>* Entry = Pieces.FindByPredicate([this](const TObjectPtr<AFlickPiece>& Piece)
	{
		return Piece && Piece->GetPieceId() == ReplayPrimaryFocusPieceId;
	}))
	{
		PrimaryPiece = Entry->Get();
	}
	const bool bSelfKnockout = PrimaryPiece
		&& PrimaryPiece->GetTeam() == LosingTeam
		&& ResolutionEliminatedPieceIds.Contains(ReplayPrimaryFocusPieceId);
	if (bSelfKnockout)
	{
		ReplayKnockoutFocusPieceId = ReplayPrimaryFocusPieceId;
		ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	}
	else if (ReplayKnockoutFocusPieceId != INDEX_NONE
		&& ReplayKnockoutFocusPieceId != ReplayPrimaryFocusPieceId)
	{
		const float* FirstImpact = ResolutionFirstImpactTimes.Find(ReplayKnockoutFocusPieceId);
		ReplayFocusSwitchSourceTime = FirstImpact
			? *FirstImpact
			: FMath::Max(FirstTime, LatestLosingElimination - 0.25f);
		ReplayFocusSwitchSourceTime = FMath::Clamp(ReplayFocusSwitchSourceTime, FirstTime, LastTime);
	}

	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->BeginReplayPresentation();
		}
	}
	TestArenaActor->BeginReplayPresentation();
	ApplyCinematicReplayTime(CinematicReplaySourceStart);

	FVector InitialFocus = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
	if (const FFlickReplayPieceState* ShotState = RoundReplayFrames[0].Pieces.FindByPredicate([this](const FFlickReplayPieceState& State)
	{
		return State.Piece.IsValid() && State.Piece->GetPieceId() == ReplayPrimaryFocusPieceId;
	}))
	{
		InitialFocus = ShotState->Transform.GetLocation();
	}
	const EFlickTeam ReplayTeam = ResolutionShootingTeam != EFlickTeam::None
		? ResolutionShootingTeam
		: Outcome == EFlickMatchOutcome::Player1Wins ? EFlickTeam::Player1 : EFlickTeam::Player2;
	if (CameraPawn)
	{
		CameraPawn->BeginCinematicReplay(InitialFocus, ReplayTeam);
	}
	if (AudioDirector)
	{
		AudioDirector->PlayReplayMusic(CinematicReplayPlaybackDuration, ReplayTeam);
	}
	ClearControllerAiming();
	PushHudEvent(TEXT("REPLAY  //  ROUND-WINNING SHOT"), FLinearColor::White, CinematicReplayPlaybackDuration);
	UE_LOG(LogFlick, Log, TEXT("Test arena replay started: %.2f second pullback and %.2f seconds of shot playback"),
		ReplayPullbackDuration, CinematicReplayMotionDuration);
	UE_LOG(LogFlick, Log, TEXT("Replay camera narrative: primary=%d knockout=%d switch=%.2f self_ko=%d"),
		ReplayPrimaryFocusPieceId,
		ReplayKnockoutFocusPieceId,
		ReplayFocusSwitchSourceTime,
		bSelfKnockout ? 1 : 0);
}

void AFlickGameMode::UpdateCinematicRoundReplay(const float DeltaSeconds)
{
	if (!bCinematicReplayActive)
	{
		return;
	}

	CinematicReplayElapsed += FMath::Max(0.0f, DeltaSeconds);
	if (CinematicReplayElapsed < ReplayPullbackDuration)
	{
		ApplyCinematicReplayTime(CinematicReplaySourceStart);
	}
	else
	{
		if (!bReplayLaunchCuePlayed)
		{
			bReplayLaunchCuePlayed = true;
			if (AudioDirector)
			{
				for (const FFlickReplayShotSetup& ReplayShot : ReplayShotSetups)
				{
					if (const AFlickPiece* Piece = ReplayShot.Piece.Get())
					{
						AudioDirector->PlayLaunch(
							Piece->GetArchetype(),
							ReplayShot.Power,
							Piece->GetActorLocation() + FVector(0.0f, 0.0f, PieceThickness * 0.65f));
					}
				}
			}
		}
		const float MotionProgress = CinematicReplayMotionDuration > KINDA_SMALL_NUMBER
			? FMath::Clamp((CinematicReplayElapsed - ReplayPullbackDuration) / CinematicReplayMotionDuration, 0.0f, 1.0f)
			: 1.0f;
		ApplyCinematicReplayTime(CinematicReplaySourceStart + CinematicReplaySourceDuration * MotionProgress);
	}
	if (CinematicReplayElapsed >= CinematicReplayPlaybackDuration)
	{
		FinishCinematicRoundReplay(true);
	}
}

void AFlickGameMode::ApplyCinematicReplayTime(const float SourceTime)
{
	if (RoundReplayFrames.IsEmpty())
	{
		return;
	}

	int32 UpperIndex = 0;
	while (UpperIndex < RoundReplayFrames.Num() && RoundReplayFrames[UpperIndex].Time < SourceTime)
	{
		++UpperIndex;
	}
	UpperIndex = FMath::Clamp(UpperIndex, 0, RoundReplayFrames.Num() - 1);
	const int32 LowerIndex = FMath::Max(0, UpperIndex - 1);
	const FFlickRoundReplayFrame& LowerFrame = RoundReplayFrames[LowerIndex];
	const FFlickRoundReplayFrame& UpperFrame = RoundReplayFrames[UpperIndex];
	const float FrameSpan = UpperFrame.Time - LowerFrame.Time;
	const float Alpha = FrameSpan > KINDA_SMALL_NUMBER
		? FMath::Clamp((SourceTime - LowerFrame.Time) / FrameSpan, 0.0f, 1.0f)
		: 0.0f;

	const bool bFollowKnockout = ReplayKnockoutFocusPieceId != INDEX_NONE
		&& SourceTime >= ReplayFocusSwitchSourceTime;
	const int32 FocusPieceId = bFollowKnockout
		? ReplayKnockoutFocusPieceId
		: ReplayPrimaryFocusPieceId;
	FVector ReplayFocus = FVector::ZeroVector;
	FVector VisibleCenter = FVector::ZeroVector;
	int32 VisiblePieceCount = 0;
	bool bFoundFocusPiece = false;
	for (const FFlickReplayPieceState& LowerState : LowerFrame.Pieces)
	{
		AFlickPiece* Piece = LowerState.Piece.Get();
		if (!Piece || !IsValid(Piece))
		{
			continue;
		}
		const FFlickReplayPieceState* UpperState = UpperFrame.Pieces.FindByPredicate([Piece](const FFlickReplayPieceState& State)
		{
			return State.Piece.Get() == Piece;
		});
		const FTransform& EndTransform = UpperState ? UpperState->Transform : LowerState.Transform;
		FTransform BlendedTransform;
		BlendedTransform.Blend(LowerState.Transform, EndTransform, Alpha);
		bool bVisible = LowerState.bVisible;
		if (const float* EliminationTime = ResolutionEliminationTimes.Find(Piece->GetPieceId()))
		{
			bVisible = SourceTime < *EliminationTime;
		}
		Piece->ApplyReplayPresentation(BlendedTransform, bVisible);
		if (Piece->GetPieceId() == FocusPieceId)
		{
			ReplayFocus = BlendedTransform.GetLocation();
			bFoundFocusPiece = true;
		}
		if (bVisible)
		{
			VisibleCenter += BlendedTransform.GetLocation();
			++VisiblePieceCount;
		}
	}

	// Live elimination feedback is transient and has expired by the time the
	// replay starts. Recreate it exactly once when replay time crosses each
	// recorded ring-out, without re-running elimination gameplay or scoring.
	for (const TPair<int32, float>& Elimination : ResolutionEliminationTimes)
	{
		if (SourceTime < Elimination.Value
			|| ReplayPresentedEliminationPieceIds.Contains(Elimination.Key))
		{
			continue;
		}

		AFlickPiece* EliminatedPiece = nullptr;
		for (AFlickPiece* Piece : Pieces)
		{
			if (Piece && IsValid(Piece) && Piece->GetPieceId() == Elimination.Key)
			{
				EliminatedPiece = Piece;
				break;
			}
		}
		if (!EliminatedPiece)
		{
			continue;
		}

		const FVector ArenaLocation = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
		const float ActiveArenaRadius = ArenaActor ? ArenaActor->GetRadius() : ArenaRadius;
		FVector EdgeDirection(
			EliminatedPiece->GetActorLocation().X - ArenaLocation.X,
			EliminatedPiece->GetActorLocation().Y - ArenaLocation.Y,
			0.0f);
		if (!EdgeDirection.Normalize())
		{
			EdgeDirection = FVector::ForwardVector;
		}
		const FVector FeedbackLocation = ArenaLocation
			+ EdgeDirection * (ActiveArenaRadius - 20.0f)
			+ FVector(0.0f, 0.0f, ArenaSurfaceZ + 22.0f - ArenaLocation.Z);
		SpawnWorldFeedback(
			FeedbackLocation,
			GetTeamColor(EliminatedPiece->GetTeam()),
			EFlickFeedbackKind::Elimination,
			1.0f,
			EdgeDirection);
		ReplayPresentedEliminationPieceIds.Add(Elimination.Key);
	}
	if (!bFoundFocusPiece && VisiblePieceCount > 0)
	{
		ReplayFocus = VisibleCenter / static_cast<float>(VisiblePieceCount);
	}
	else if (!bFoundFocusPiece)
	{
		ReplayFocus = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
	}
	ReplayFocus.Z = ArenaSurfaceZ + PieceThickness;
	if (TestArenaActor)
	{
		TestArenaActor->ApplyReplayDividerState(LowerFrame.RaisedDividerMask);
	}
	if (CameraPawn)
	{
		CameraPawn->UpdateCinematicReplay(
			ReplayFocus,
			GetCinematicReplayProgress(),
			GetCinematicReplayPullbackAlpha());
	}
}

void AFlickGameMode::FinishCinematicRoundReplay(const bool bCompleteRound)
{
	if (!bCinematicReplayActive)
	{
		return;
	}

	const EFlickMatchOutcome CompletedOutcome = PendingReplayOutcome;
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->EndReplayPresentation();
		}
	}
	if (TestArenaActor && IsValid(TestArenaActor))
	{
		TestArenaActor->EndReplayPresentation();
	}
	if (CameraPawn)
	{
		CameraPawn->EndCinematicReplay();
	}
	if (AudioDirector)
	{
		AudioDirector->StopReplayMusic();
	}

	bCinematicReplayActive = false;
	CinematicReplayElapsed = 0.0f;
	CinematicReplaySourceStart = 0.0f;
	CinematicReplaySourceDuration = 0.0f;
	CinematicReplayMotionDuration = 0.0f;
	CinematicReplayPlaybackDuration = 0.0f;
	PendingReplayOutcome = EFlickMatchOutcome::Continue;
	RoundReplayFrames.Reset();
	ReplayShotSetups.Reset();
	ReplayPresentedEliminationPieceIds.Reset();
	bReplayLaunchCuePlayed = false;
	ReplayPrimaryFocusPieceId = INDEX_NONE;
	ReplayKnockoutFocusPieceId = INDEX_NONE;
	ReplayFocusSwitchSourceTime = TNumericLimits<float>::Max();
	if (bCompleteRound)
	{
		CompleteRoundForOutcome(CompletedOutcome);
	}
}

void AFlickGameMode::PresentDramaticResolutionEvent()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsBobMode())
	{
		return;
	}

	const FFlickDramaticEventResult Result = EvaluateDramaticEvent(
		ResolutionPlayer1Eliminated,
		ResolutionPlayer2Eliminated,
		CountActivePieces(EFlickTeam::Player1),
		CountActivePieces(EFlickTeam::Player2),
		ResolutionImpactCount,
		DramaticChainImpactThreshold,
		ResolutionShootingTeam,
		bResolutionWasSimultaneous);
	if (!Result.IsValid())
	{
		return;
	}

	int32 BonusPoints = 0;
	TArray<EFlickTeam, TInlineAllocator<2>> AwardedTeams;
	switch (Result.Event)
	{
	case EFlickDramaticEvent::Trade:
		BonusPoints = TradeBonusPoints;
		AwardedTeams.Add(EFlickTeam::Player1);
		AwardedTeams.Add(EFlickTeam::Player2);
		break;
	case EFlickDramaticEvent::DoubleKnockout:
		BonusPoints = DoubleKnockoutBonusPoints;
		if (Result.HighlightedTeam != EFlickTeam::None)
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::MultiKnockout:
		BonusPoints = MultiKnockoutBonusPerPuck * Result.Value;
		if (Result.HighlightedTeam == EFlickTeam::None)
		{
			AwardedTeams.Add(EFlickTeam::Player1);
			AwardedTeams.Add(EFlickTeam::Player2);
		}
		else
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::LastPuckStanding:
		BonusPoints = LastPuckStandingBonusPoints;
		if (Result.HighlightedTeam != EFlickTeam::None)
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::ChainReaction:
		BonusPoints = ChainReactionBonusPoints;
		if (Result.HighlightedTeam != EFlickTeam::None)
		{
			AwardedTeams.Add(Result.HighlightedTeam);
		}
		break;
	case EFlickDramaticEvent::SelfKnockout:
	case EFlickDramaticEvent::None:
	default:
		break;
	}

	BonusPoints = FMath::Max(0, BonusPoints);
	for (const EFlickTeam AwardedTeam : AwardedTeams)
	{
		const int32 PlayerSlot = FlickGameState->GetLastShootingPlayerSlot(AwardedTeam);
		if (Result.Event == EFlickDramaticEvent::DoubleKnockout)
		{
			FlickGameState->RecordPlayerDoubleKnockout(AwardedTeam, PlayerSlot);
		}
		FlickGameState->RecordPlayerBonus(
			AwardedTeam,
			PlayerSlot,
			BonusPoints);
	}

	FlickGameState->ShowDramaticEvent(
		Result.Event,
		Result.HighlightedTeam,
		Result.Value,
		ResolutionImpactCount,
		BonusPoints,
		DramaticEventDuration);
}

void AFlickGameMode::PresentShotAccolades()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsBobMode() || bResolutionWasSimultaneous
		|| ResolutionShootingTeam == EFlickTeam::None || ResolutionShotPieceId == INDEX_NONE)
	{
		return;
	}

	const bool bShooterIsPlayer1 = ResolutionShootingTeam == EFlickTeam::Player1;
	const int32 OpponentEliminated = bShooterIsPlayer1
		? ResolutionPlayer2Eliminated : ResolutionPlayer1Eliminated;
	const int32 OwnEliminated = bShooterIsPlayer1
		? ResolutionPlayer1Eliminated : ResolutionPlayer2Eliminated;
	const EFlickTeam OpponentTeam = GetOpposingTeam(ResolutionShootingTeam);

	bool bDominoKnockout = false;
	bool bLongRangeKnockout = false;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || Piece->GetTeam() != OpponentTeam
			|| !ResolutionEliminatedPieceIds.Contains(Piece->GetPieceId()))
		{
			continue;
		}
		if (const int32* ContactDepth = ResolutionContactDepths.Find(Piece->GetPieceId()))
		{
			bDominoKnockout |= *ContactDepth >= 2
				&& !ResolutionDirectContactPieceIds.Contains(Piece->GetPieceId());
		}
		if (const FVector2D* InitialTargetLocation = ResolutionInitialPieceLocations.Find(Piece->GetPieceId()))
		{
			bLongRangeKnockout |= FVector2D::Distance(ResolutionShotStart, *InitialTargetLocation)
				>= LongRangeKnockoutDistance;
		}
	}

	bool bPrecisionStop = false;
	const TObjectPtr<AFlickPiece>* ShotPieceEntry = Pieces.FindByPredicate([this](const TObjectPtr<AFlickPiece>& Piece)
	{
		return Piece && Piece->GetPieceId() == ResolutionShotPieceId;
	});
	if (const AFlickPiece* ShotPiece = ShotPieceEntry ? ShotPieceEntry->Get() : nullptr)
	{
		if (IsValid(ShotPiece) && ShotPiece->IsActive())
		{
			const FVector ArenaLocation = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
			const float Radius = FVector2D::Distance(
				FVector2D(ShotPiece->GetActorLocation().X, ShotPiece->GetActorLocation().Y),
				FVector2D(ArenaLocation.X, ArenaLocation.Y));
			const float ActiveRadius = ArenaActor ? ArenaActor->GetRadius() : ArenaRadius;
			bPrecisionStop = Radius >= ActiveRadius * PrecisionStopMinimumRadiusFraction
				&& Radius <= ActiveRadius * PrecisionStopMaximumRadiusFraction;
		}
	}

	FFlickShotAccoladeContext Context;
	Context.OpponentEliminated = OpponentEliminated;
	Context.OwnEliminated = OwnEliminated;
	Context.OpponentRemaining = CountActivePieces(OpponentTeam);
	Context.OwnRemaining = CountActivePieces(ResolutionShootingTeam);
	Context.DirectlyContactedPucks = ResolutionDirectContactPieceIds.Num();
	Context.bSwitchActivated = ResolutionActivatedSwitchMask != 0;
	Context.bShotPieceHitDivider = ResolutionDividerContactPieceIds.Contains(ResolutionShotPieceId);
	Context.bNewDividerAffectedPlay = !ResolutionNewDividerContactPieceIds.IsEmpty();
	Context.bDominoKnockout = bDominoKnockout;
	Context.bLongRangeKnockout = bLongRangeKnockout;
	Context.bShotPieceSurvivedNearEdge = bPrecisionStop;
	Context.bBuzzerRelease = bResolutionBuzzerRelease;

	const int32 ShootingPlayerSlot = FlickGameState->GetLastShootingPlayerSlot(ResolutionShootingTeam);
	for (const EFlickAccolade Accolade : FlickAccoladeRules::EvaluateShot(Context))
	{
		const int32 BonusPoints = GetFlickAccoladeBonusPoints(Accolade);
		FlickGameState->RecordPlayerAccolade(
			ResolutionShootingTeam,
			ShootingPlayerSlot,
			Accolade,
			BonusPoints);
		FlickGameState->ShowAccolade(Accolade, ResolutionShootingTeam, BonusPoints);
	}
}

void AFlickGameMode::AwardFlawlessRound(const EFlickMatchOutcome Outcome)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState || IsBobMode())
	{
		return;
	}
	const EFlickTeam WinningTeam = Outcome == EFlickMatchOutcome::Player1Wins
		? EFlickTeam::Player1
		: Outcome == EFlickMatchOutcome::Player2Wins ? EFlickTeam::Player2 : EFlickTeam::None;
	if (WinningTeam == EFlickTeam::None
		|| !FlickAccoladeRules::IsFlawlessRound(
			CountActivePieces(WinningTeam),
			CurrentStartingPiecesPerTeam))
	{
		return;
	}
	for (int32 PlayerSlot = 0; PlayerSlot < CurrentPlayersPerTeam; ++PlayerSlot)
	{
		FlickGameState->RecordPlayerAccolade(WinningTeam, PlayerSlot, EFlickAccolade::FlawlessRound);
	}
	FlickGameState->ShowAccolade(EFlickAccolade::FlawlessRound, WinningTeam);
}

void AFlickGameMode::UpdateEliminations()
{
	bool bAnyEliminated = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		const FVector PieceLocation = Piece->GetActorLocation();
		const FVector ArenaLocation = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
		const float ActiveArenaRadius = ArenaActor ? ArenaActor->GetRadius() : ArenaRadius;
		const bool bBelowKillPlane = PieceLocation.Z <= KillZ;
		const bool bOutsideTabletop = ArenaActor && FlickModeRules::IsPieceOutsideCircularTabletop(
			PieceLocation,
			Piece->GetActorUpVector(),
			Piece->GetPieceRadius(),
			Piece->GetPieceThickness(),
			ArenaLocation,
			ActiveArenaRadius,
			ArenaSurfaceZ,
			KnockoutBoundsTolerance);
		if (bBelowKillPlane || bOutsideTabletop)
		{
			ResolutionEliminatedPieceIds.Add(Piece->GetPieceId());
			ResolutionEliminationTimes.FindOrAdd(Piece->GetPieceId()) = ResolutionElapsed;
			const EFlickTeam EliminatedTeam = Piece->GetTeam();
			if (EliminatedTeam == EFlickTeam::Player1)
			{
				++ResolutionPlayer1Eliminated;
			}
			else if (EliminatedTeam == EFlickTeam::Player2)
			{
				++ResolutionPlayer2Eliminated;
			}
			FVector EdgeDirection(
				PieceLocation.X - ArenaLocation.X,
				PieceLocation.Y - ArenaLocation.Y,
				0.0f);
			if (!EdgeDirection.Normalize())
			{
				EdgeDirection = FVector::ForwardVector;
			}
			const FVector FeedbackLocation = ArenaLocation
				+ EdgeDirection * (ActiveArenaRadius - 20.0f)
				+ FVector(0.0f, 0.0f, ArenaSurfaceZ + 22.0f - ArenaLocation.Z);

			SpawnWorldFeedback(
				FeedbackLocation,
				GetTeamColor(EliminatedTeam),
				EFlickFeedbackKind::Elimination,
				1.0f,
				EdgeDirection);
			if (AFlickGameState* FlickGameState = GetFlickGameState())
			{
				FlickGameState->RecordElimination(EliminatedTeam);
				const EFlickTeam CreditingTeam = GetOpposingTeam(EliminatedTeam);
				if (bResolvingKickoff || FlickGameState->LastShotTeam == CreditingTeam)
				{
					FlickGameState->RecordPlayerKnockout(
						CreditingTeam,
						FlickGameState->GetLastShootingPlayerSlot(CreditingTeam));
				}
			}
			PushHudEvent(
				FString::Printf(TEXT("PLAYER %d PUCK OUT"), GetTeamNumber(EliminatedTeam)),
				GetTeamColor(EliminatedTeam),
				2.2f);
			AddCameraFeedback(0.32f);
			AddControllerFeedback(0.78f, 0.24f);
			if (AudioDirector)
			{
				AudioDirector->PlayRingOut(FeedbackLocation);
			}
			Piece->Eliminate();
			bAnyEliminated = true;
		}
	}

	if (bAnyEliminated)
	{
		UpdateGameStateCounts();
	}
}

void AFlickGameMode::UpdateBobPockets()
{
	if (!BobArenaActor)
	{
		return;
	}

	bool bPocketedAnyPiece = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		if (!BobArenaActor->IsCapturedByPocket(Piece->GetActorLocation(), Piece->GetPieceRadius()))
		{
			continue;
		}

		const FVector PocketLocation(
			Piece->GetActorLocation().X,
			Piece->GetActorLocation().Y,
			ArenaSurfaceZ + 8.0f);
		if (Piece->IsBobStriker())
		{
			const bool bShooterPocketedOwnStriker = GetFlickGameState()
				&& GetFlickGameState()->CurrentTeam == Piece->GetTeam();
			if (Piece->GetTeam() == EFlickTeam::Player1)
			{
				bPlayer1BobStrikerPocketed = true;
			}
			else if (Piece->GetTeam() == EFlickTeam::Player2)
			{
				bPlayer2BobStrikerPocketed = true;
			}
			PushHudEvent(
				bShooterPocketedOwnStriker
					? TEXT("STRIKER POCKETED  |  PENALTY")
					: FString::Printf(TEXT("PLAYER %d STRIKER RETURNED"), GetTeamNumber(Piece->GetTeam())),
				bShooterPocketedOwnStriker
					? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f)
					: GetTeamColor(Piece->GetTeam()),
				2.4f);
		}
		else
		{
			const EFlickTeam ScoringTeam = Piece->GetTeam();
			if (AFlickGameState* FlickGameState = GetFlickGameState())
			{
				FlickGameState->RecordElimination(ScoringTeam);
				if (FlickGameState->LastShotTeam == ScoringTeam)
				{
					FlickGameState->RecordPlayerKnockout(
						ScoringTeam,
						FlickGameState->GetLastShootingPlayerSlot(ScoringTeam));
				}
			}
			PushHudEvent(
				FString::Printf(TEXT("PLAYER %d PUCK POCKETED"), GetTeamNumber(ScoringTeam)),
				GetTeamColor(ScoringTeam),
				2.0f);
		}

		SpawnWorldFeedback(
			PocketLocation,
			Piece->IsBobStriker() ? FLinearColor::White : GetTeamColor(Piece->GetTeam()),
			EFlickFeedbackKind::Elimination,
			0.82f);
		if (AudioDirector)
		{
			AudioDirector->PlayRingOut(PocketLocation);
		}
		Piece->Eliminate();
		bPocketedAnyPiece = true;
	}

	if (bPocketedAnyPiece)
	{
		AddCameraFeedback(0.2f);
		AddControllerFeedback(0.55f, 0.2f);
		UpdateGameStateCounts();
	}
}

void AFlickGameMode::UpdateBobPieceStability()
{
	if (!BobArenaActor)
	{
		return;
	}

	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive()
			|| Piece->GetLinearVelocity().Size() > BobSelfRightingMaxLinearSpeed
			|| Piece->GetAngularVelocityDegrees().Size() > BobSelfRightingMaxAngularSpeed
			|| !BobArenaActor->IsSafeForTabletopSelfRighting(
				Piece->GetActorLocation(),
				Piece->GetPieceRadius()))
		{
			continue;
		}

		Piece->ApplyTabletopSelfRighting(
			BobSelfRightingTorque,
			BobSelfRightingDamping,
			BobSelfRightingMinimumTilt);
	}
}

bool AFlickGameMode::IsPieceSafeOnClassicTabletop(const AFlickPiece* Piece) const
{
	if (!Piece || !IsValid(Piece) || !Piece->IsActive() || !ArenaActor)
	{
		return false;
	}
	const FVector ArenaLocation = ArenaActor->GetActorLocation();
	const FVector PieceLocation = Piece->GetActorLocation();
	const float SafeRadius = FMath::Max(
		0.0f,
		ArenaActor->GetRadius() - Piece->GetPieceRadius() * 1.35f);
	return FVector2D::DistSquared(
		FVector2D(PieceLocation.X, PieceLocation.Y),
		FVector2D(ArenaLocation.X, ArenaLocation.Y)) <= FMath::Square(SafeRadius)
		&& PieceLocation.Z >= ArenaSurfaceZ - Piece->GetPieceThickness();
}

void AFlickGameMode::UpdateClassicPieceStability()
{
	for (AFlickPiece* Piece : Pieces)
	{
		if (!IsPieceSafeOnClassicTabletop(Piece))
		{
			continue;
		}
		Piece->ApplyTabletopFlightContainment(
			ArenaSurfaceZ,
			TabletopMaximumUpwardSpeed,
			TabletopDownwardAcceleration);
		if (Piece->GetLinearVelocity().Size() <= BobSelfRightingMaxLinearSpeed
			&& Piece->GetAngularVelocityDegrees().Size() <= BobSelfRightingMaxAngularSpeed)
		{
			Piece->ApplyTabletopSelfRighting(
				TabletopSelfRightingTorque,
				TabletopSelfRightingDamping,
				TabletopSelfRightingMinimumTilt);
		}
	}
}

void AFlickGameMode::SettleClassicPiecesOnTabletop()
{
	for (AFlickPiece* Piece : Pieces)
	{
		if (IsPieceSafeOnClassicTabletop(Piece))
		{
			Piece->SettleFlatOnTabletop(ArenaSurfaceZ);
		}
	}
}

void AFlickGameMode::UpdateGameStateCounts() const
{
	if (AFlickGameState* FlickGameState = GetFlickGameState())
	{
		FlickGameState->SetActivePieceCounts(
			CountActivePieces(EFlickTeam::Player1),
			CountActivePieces(EFlickTeam::Player2));
	}
}

bool AFlickGameMode::AreActivePiecesSettled() const
{
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		if (Piece->GetLinearVelocity().Size() > SleepLinearVelocityThreshold
			|| Piece->GetAngularVelocityDegrees().Size() > SleepAngularVelocityThreshold)
		{
			return false;
		}
	}
	return true;
}

bool AFlickGameMode::ApplyResolutionTimeoutCleanup()
{
	bool bHasMeaningfulMotion = false;
	for (AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		const float LinearSpeed = Piece->GetLinearVelocity().Size();
		const float AngularSpeed = Piece->GetAngularVelocityDegrees().Size();
		if (LinearSpeed > SleepLinearVelocityThreshold * 6.0f || AngularSpeed > SleepAngularVelocityThreshold * 4.0f)
		{
			bHasMeaningfulMotion = true;
			continue;
		}

		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
		{
			RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			RootPrimitive->PutRigidBodyToSleep();
		}
	}

	if (bHasMeaningfulMotion)
	{
		ResolutionElapsed = MaximumResolutionDuration * 0.5f;
		UE_LOG(LogFlick, Warning, TEXT("Resolution timeout reached, but at least one puck is still moving meaningfully. Extending resolution."));
		return false;
	}

	UE_LOG(LogFlick, Warning, TEXT("Resolution timeout cleanup used"));
	return true;
}

void AFlickGameMode::FinishPhysicsResolution(const bool bUsedTimeout)
{
	if (IsBobMode())
	{
		UpdateBobPockets();
	}
	else
	{
		UpdateEliminations();
		SettleClassicPiecesOnTabletop();
	}
	UpdateGameStateCounts();
	PresentDramaticResolutionEvent();
	PresentShotAccolades();
	ResolveTestArenaControlZones();
	CaptureRoundReplayFrame(true);
	// Send the authority's final sleeping/resting transforms immediately. This
	// prevents a remote physics proxy from keeping a locally divergent tilt after
	// the server has already declared the shot settled.
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece))
		{
			Piece->ForceNetUpdate();
		}
	}

	if (bLogPhysicsResolution)
	{
		UE_LOG(
			LogFlick,
			Log,
			TEXT("Physics resolved in %.2f s%s"),
			ResolutionElapsed,
			bUsedTimeout ? TEXT(" using timeout cleanup") : TEXT(""));
	}

	CheckWinOrAdvanceTurn();
}

void AFlickGameMode::CheckWinOrAdvanceTurn()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}
	if (IsFreePlayTraining())
	{
		ResolveTrainingTurn();
		return;
	}
	if (IsBobMode())
	{
		ResolveBobTurn();
		return;
	}
	const bool bCompletedKickoff = bResolvingKickoff;
	bResolvingKickoff = false;

	const int32 Player1Count = CountActivePieces(EFlickTeam::Player1);
	const int32 Player2Count = CountActivePieces(EFlickTeam::Player2);
	FlickGameState->SetActivePieceCounts(Player1Count, Player2Count);

	const EFlickMatchOutcome Outcome = EvaluateMatchOutcome(Player1Count, Player2Count);
	switch (Outcome)
	{
	case EFlickMatchOutcome::Draw:
	case EFlickMatchOutcome::Player2Wins:
	case EFlickMatchOutcome::Player1Wins:
		if (bTestArenaMode && Outcome != EFlickMatchOutcome::Draw && !bReplayPlayedForResolution)
		{
			BeginCinematicRoundReplay(Outcome);
			if (bCinematicReplayActive)
			{
				return;
			}
		}
		CompleteRoundForOutcome(Outcome);
		return;
	case EFlickMatchOutcome::Continue:
	default:
		break;
	}

	FlickGameState->SetCurrentTeam(GetOpposingTeam(FlickGameState->CurrentTeam));
	ActivateNextPlayerForTeam(FlickGameState->CurrentTeam);
	FlickGameState->AdvanceTurn();
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	SetCameraViewForTeam(FlickGameState->CurrentTeam);
	if (bCompletedKickoff)
	{
		PushHudEvent(
			TEXT("KICKOFF COMPLETE"),
			GetTeamColor(FlickGameState->CurrentTeam),
			1.5f);
	}
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(FlickGameState->CurrentTeam);
	}
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;

	UE_LOG(LogFlick, Log, TEXT("Turn changed to %s"), *GetTeamDisplayName(FlickGameState->CurrentTeam));
}

void AFlickGameMode::ResolveTrainingTurn()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}

	if (bPlayer1BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player1);
	}
	if (bPlayer2BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player2);
	}
	bPlayer1BobStrikerPocketed = false;
	bPlayer2BobStrikerPocketed = false;
	bResolvingKickoff = false;
	for (int32 PieceIndex = Pieces.Num() - 1; PieceIndex >= 0; --PieceIndex)
	{
		AFlickPiece* Piece = Pieces[PieceIndex];
		if (Piece && IsValid(Piece) && Piece->IsActive())
		{
			continue;
		}
		if (Piece && IsValid(Piece))
		{
			Piece->Destroy();
		}
		Pieces.RemoveAtSwap(PieceIndex, 1, EAllowShrinking::No);
	}
	UpdateGameStateCounts();

	FlickGameState->AdvanceTurn();
	FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
	ActivateNextPlayerForTeam(EFlickTeam::Player1);
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	SetCameraViewForTeam(EFlickTeam::Player1);
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(EFlickTeam::Player1);
	}
	UE_LOG(LogFlick, Log, TEXT("Training shot resolved; Player 1 remains active"));
}

void AFlickGameMode::CaptureTrainingResetSnapshot()
{
	if (!IsFreePlayTraining())
	{
		return;
	}

	TrainingResetSnapshot.Reset();
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}

		FFlickTrainingPieceSnapshot& Snapshot = TrainingResetSnapshot.AddDefaulted_GetRef();
		Snapshot.Team = Piece->GetTeam();
		Snapshot.Archetype = Piece->GetArchetype();
		Snapshot.Location = Piece->GetActorLocation();
		Snapshot.PieceId = Piece->GetPieceId();
		Snapshot.OwningPlayerSlot = Piece->GetOwningPlayerSlot();
		Snapshot.bBobStriker = Piece->IsBobStriker();
		Snapshot.bShowPlayerIdentity = Piece->ShowsPlayerIdentity();
	}
	bHasTrainingResetSnapshot = true;
	UE_LOG(LogFlick, Log, TEXT("Saved training reset setup with %d pucks"), TrainingResetSnapshot.Num());
}

void AFlickGameMode::RestoreTrainingResetSnapshot()
{
	if (!bTrainingMode || !bHasTrainingResetSnapshot)
	{
		return;
	}

	for (const FFlickTrainingPieceSnapshot& Snapshot : TrainingResetSnapshot)
	{
		AFlickPiece* Piece = SpawnPiece(
			Snapshot.Team,
			Snapshot.PieceId,
			Snapshot.Location,
			Snapshot.Archetype,
			Snapshot.bBobStriker,
			Snapshot.OwningPlayerSlot,
			Snapshot.bShowPlayerIdentity);
		if (!Piece)
		{
			continue;
		}
		if (Snapshot.bBobStriker)
		{
			if (Snapshot.Team == EFlickTeam::Player1)
			{
				Player1BobStriker = Piece;
			}
			else if (Snapshot.Team == EFlickTeam::Player2)
			{
				Player2BobStriker = Piece;
			}
		}
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
		{
			Primitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
			Primitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			Primitive->PutRigidBodyToSleep();
		}
	}
}

void AFlickGameMode::ResetTrainingBoard()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!bTrainingMode || !FlickGameState)
	{
		return;
	}

	ClearControllerAiming();
	DestroyPieces();
	if (bHasTrainingResetSnapshot)
	{
		RestoreTrainingResetSnapshot();
	}
	else
	{
		SpawnPieces();
		CaptureTrainingResetSnapshot();
	}
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	LastImpactFeedbackTime = -100.0f;
	LastStrongImpactEventTime = -100.0f;
	Player1NextPlayerSlot = 0;
	Player2NextPlayerSlot = 0;
	FlickGameState->SetCurrentTeam(EFlickTeam::Player1);
	FlickGameState->SetCurrentTeamPlayerSlot(0);
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	UpdateGameStateCounts();
	SetCameraViewForTeam(EFlickTeam::Player1);
	PushHudEvent(TEXT("TRAINING SETUP RESTORED"), FLinearColor(0.2f, 0.78f, 0.5f, 1.0f), 2.0f);
	UE_LOG(LogFlick, Log, TEXT("Training board reset to saved setup"));
}

void AFlickGameMode::ResolveBobTurn()
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}

	const EFlickTeam ShootingTeam = FlickGameState->CurrentTeam;
	const bool bShootingStrikerPocketed = ShootingTeam == EFlickTeam::Player1
		? bPlayer1BobStrikerPocketed
		: bPlayer2BobStrikerPocketed;
	if (bShootingStrikerPocketed)
	{
		const bool bRestoredPuck = RestoreBobPenaltyPiece(ShootingTeam);
		PushHudEvent(
			bRestoredPuck ? TEXT("PENALTY  |  OWN PUCK RETURNED") : TEXT("PENALTY  |  TURN LOST"),
			FLinearColor(1.0f, 0.72f, 0.12f, 1.0f),
			2.2f);
	}
	if (bPlayer1BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player1);
	}
	if (bPlayer2BobStrikerPocketed)
	{
		ResetBobStrikerForTeam(EFlickTeam::Player2);
	}
	bPlayer1BobStrikerPocketed = false;
	bPlayer2BobStrikerPocketed = false;
	UpdateGameStateCounts();

	const int32 Player1Remaining = CountActivePieces(EFlickTeam::Player1);
	const int32 Player2Remaining = CountActivePieces(EFlickTeam::Player2);
	const EFlickMatchOutcome Outcome = FlickBobRules::EvaluateOutcome(
		Player1Remaining,
		Player2Remaining,
		ShootingTeam);
	if (Outcome != EFlickMatchOutcome::Continue)
	{
		CompleteRoundForOutcome(Outcome);
		return;
	}

	FlickGameState->SetCurrentTeam(GetOpposingTeam(ShootingTeam));
	ActivateNextPlayerForTeam(FlickGameState->CurrentTeam);
	FlickGameState->AdvanceTurn();
	FlickGameState->SetMatchPhase(EFlickMatchPhase::Aiming);
	SetCameraViewForTeam(FlickGameState->CurrentTeam);
	PushHudEvent(
		FString::Printf(TEXT("PLAYER %d STRIKER ACTIVE"), GetTeamNumber(FlickGameState->CurrentTeam)),
		GetTeamColor(FlickGameState->CurrentTeam),
		1.6f);
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(FlickGameState->CurrentTeam);
	}
	ResolutionElapsed = 0.0f;
	SettledElapsed = 0.0f;
	UE_LOG(
		LogFlick,
		Log,
		TEXT("BOB turn changed to %s. Both strikers remain at their settled positions."),
		*GetTeamDisplayName(FlickGameState->CurrentTeam));
}

void AFlickGameMode::CompleteRoundForOutcome(const EFlickMatchOutcome Outcome)
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}
	if (!IsBobMode())
	{
		AwardFlawlessRound(Outcome);
		AwardRoundSurvivalPoints();
	}
	ResetShotClock();
	FlickGameState->CompleteRound(Outcome);
	FlickGameState->SetRoundAdvanceTimerState(
		!FlickGameState->bSeriesComplete,
		RoundAdvanceTimeLimit);
	if (FlickGameState->bSeriesComplete)
	{
		const EFlickMatchOutcome FinalOutcome = FlickGameState->WinnerTeam == EFlickTeam::Player1
			? EFlickMatchOutcome::Player1Wins
			: FlickGameState->WinnerTeam == EFlickTeam::Player2
				? EFlickMatchOutcome::Player2Wins
				: EFlickMatchOutcome::Draw;
		FlickGameState->FinalizeAuthoritativeMatch(FinalOutcome);
		DispatchRankedMatchResults();
		if (UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem())
		{
			Coordinator->NotifyServerMatchComplete(FinalOutcome, false);
		}
		UE_LOG(
			LogFlick,
			Log,
			TEXT("AUTHORITATIVE_MATCH_RESULT: id=%s outcome=%d forfeit=0"),
			*FlickGameState->MatchId,
			static_cast<int32>(FinalOutcome));
	}
	if (AudioDirector)
	{
		AudioDirector->PlayRoundResult(
			FlickGameState->WinnerTeam,
			FlickGameState->bDraw,
			FlickGameState->bSeriesComplete);
	}
	if (FlickGameState->bDraw)
	{
		PushHudEvent(
			FlickGameState->bSeriesComplete ? TEXT("MATCH DRAW") : TEXT("ROUND DRAW"),
			FLinearColor(1.0f, 0.8f, 0.15f, 1.0f),
			3.0f);
	}
	else
	{
		const FString ResultMessage = IsBobMode()
			? FString::Printf(TEXT("PLAYER %d CLEARS BOB"), GetTeamNumber(FlickGameState->WinnerTeam))
			: FlickGameState->bSeriesComplete
				? FString::Printf(TEXT("PLAYER %d WINS THE MATCH"), GetTeamNumber(FlickGameState->WinnerTeam))
				: FString::Printf(TEXT("PLAYER %d TAKES ROUND %d"), GetTeamNumber(FlickGameState->WinnerTeam), FlickGameState->RoundNumber);
		PushHudEvent(ResultMessage, GetTeamColor(FlickGameState->WinnerTeam), 3.0f);
	}
	UE_LOG(
		LogFlick,
		Log,
		TEXT("%s complete. Rounds %d-%d, scoreboard %d-%d%s"),
		IsBobMode() ? TEXT("BOB board") : *FString::Printf(TEXT("Round %d"), FlickGameState->RoundNumber),
		FlickGameState->Player1RoundsWon,
		FlickGameState->Player2RoundsWon,
		FlickGameState->GetTeamScore(EFlickTeam::Player1),
		FlickGameState->GetTeamScore(EFlickTeam::Player2),
		FlickGameState->bSeriesComplete ? TEXT(" (match complete)") : TEXT(""));
}

void AFlickGameMode::AwardRoundSurvivalPoints() const
{
	AFlickGameState* FlickGameState = GetFlickGameState();
	if (!FlickGameState)
	{
		return;
	}

	int32 SurvivorCounts[2][3] = {};
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive() || Piece->IsBobStriker())
		{
			continue;
		}
		const int32 TeamIndex = Piece->GetTeam() == EFlickTeam::Player1
			? 0
			: Piece->GetTeam() == EFlickTeam::Player2 ? 1 : INDEX_NONE;
		const int32 PlayerSlot = Piece->GetOwningPlayerSlot();
		if (TeamIndex != INDEX_NONE && PlayerSlot >= 0 && PlayerSlot < 3)
		{
			++SurvivorCounts[TeamIndex][PlayerSlot];
		}
	}

	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		const EFlickTeam Team = TeamIndex == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2;
		for (int32 PlayerSlot = 0; PlayerSlot < 3; ++PlayerSlot)
		{
			FlickGameState->RecordPlayerSurvivingPucks(
				Team,
				PlayerSlot,
				SurvivorCounts[TeamIndex][PlayerSlot]);
		}
	}
}

AFlickPiece* AFlickGameMode::GetBobStriker(const EFlickTeam Team) const
{
	return Team == EFlickTeam::Player1
		? Player1BobStriker.Get()
		: Team == EFlickTeam::Player2
			? Player2BobStriker.Get()
			: nullptr;
}

void AFlickGameMode::ResetBobStrikerForTeam(const EFlickTeam Team)
{
	AFlickPiece* BobStriker = GetBobStriker(Team);
	if (!BobStriker || !BobArenaActor || Team == EFlickTeam::None)
	{
		return;
	}
	ResetBobPieceAt(
		BobStriker,
		Team,
		BobArenaActor->GetStrikerStart(Team, PieceThickness),
		true);
}

bool AFlickGameMode::RestoreBobPenaltyPiece(const EFlickTeam Team)
{
	for (AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && !Piece->IsBobStriker()
			&& Piece->GetTeam() == Team && Piece->IsEliminated())
		{
			ResetBobPieceAt(Piece, Team, FindBobRespawnLocation(), false);
			UpdateGameStateCounts();
			return true;
		}
	}
	return false;
}

FVector AFlickGameMode::FindBobRespawnLocation() const
{
	const float CandidateSpacing = PieceRadius * 2.55f;
	for (int32 RingIndex = 0; RingIndex <= 4; ++RingIndex)
	{
		const int32 CandidateCount = RingIndex == 0 ? 1 : RingIndex * 8;
		for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount; ++CandidateIndex)
		{
			const float Angle = CandidateCount == 1
				? 0.0f
				: 2.0f * PI * static_cast<float>(CandidateIndex) / CandidateCount;
			const FVector Candidate(
				FMath::Cos(Angle) * CandidateSpacing * RingIndex,
				FMath::Sin(Angle) * CandidateSpacing * RingIndex,
				ArenaSurfaceZ + PieceThickness * 0.5f + 3.0f);
			bool bBlocked = false;
			for (const AFlickPiece* OtherPiece : Pieces)
			{
				if (!OtherPiece || !IsValid(OtherPiece) || !OtherPiece->IsActive())
				{
					continue;
				}
				const FVector Delta = OtherPiece->GetActorLocation() - Candidate;
				if (FVector2D(Delta.X, Delta.Y).Size() < PieceRadius + OtherPiece->GetPieceRadius() + 7.0f)
				{
					bBlocked = true;
					break;
				}
			}
			if (!bBlocked && (!BobArenaActor || !BobArenaActor->IsInsidePocket(Candidate)))
			{
				return Candidate;
			}
		}
	}
	return FVector(0.0f, 0.0f, ArenaSurfaceZ + PieceThickness * 0.5f + 3.0f);
}

void AFlickGameMode::ResetBobPieceAt(
	AFlickPiece* Piece,
	const EFlickTeam Team,
	const FVector& Location,
	const bool bIsStriker) const
{
	if (!Piece)
	{
		return;
	}
	if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Piece->GetRootComponent()))
	{
		Primitive->SetSimulatePhysics(false);
	}
	Piece->SetActorLocationAndRotation(
		Location,
		FRotator::ZeroRotator,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Piece->InitializePiece(
		Team,
		Piece->GetPieceId(),
		PieceRadius,
		PieceThickness,
		EFlickPieceArchetype::Standard,
		bIsStriker,
		Piece->GetOwningPlayerSlot());
}

void AFlickGameMode::SpawnWorldFeedback(
	const FVector& Location,
	const FLinearColor& Color,
	const EFlickFeedbackKind FeedbackKind,
	const float Strength,
	const FVector& BiasDirection) const
{
	if (!GetWorld() || !AreImpactEffectsEnabled())
	{
		return;
	}

	AFlickWorldFeedback* Feedback = GetWorld()->SpawnActor<AFlickWorldFeedback>(
		AFlickWorldFeedback::StaticClass(), Location, FRotator::ZeroRotator);
	if (Feedback)
	{
		Feedback->InitializeFeedback(FeedbackKind, Color, Strength, BiasDirection);
	}
}

void AFlickGameMode::PushHudEvent(
	const FString& Message,
	const FLinearColor& Color,
	const float Duration) const
{
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController)
	{
		if (AFlickHUD* FlickHUD = Cast<AFlickHUD>(PlayerController->GetHUD()))
		{
			FlickHUD->PushEventMessage(Message, Color, Duration);
		}
	}
}

AFlickPiece* AFlickGameMode::SpawnPiece(
	const EFlickTeam Team,
	const int32 PieceId,
	const FVector& Location,
	const EFlickPieceArchetype Archetype,
	const bool bIsBobStriker,
	const int32 OwningPlayerSlot,
	const bool bShowPlayerIdentity)
{
	AFlickPiece* Piece = GetWorld()->SpawnActor<AFlickPiece>(AFlickPiece::StaticClass(), Location, FRotator::ZeroRotator);
	if (!Piece)
	{
		return nullptr;
	}

	const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(Archetype);
	Piece->PieceMassKg = PieceMassKg * ArchetypeRules.MassMultiplier;
	Piece->PieceFriction = PieceFriction * ArchetypeRules.FrictionMultiplier;
	Piece->PieceRestitution = PieceRestitution * ArchetypeRules.RestitutionMultiplier;
	Piece->LinearDamping = LinearDamping * ArchetypeRules.LinearDampingMultiplier;
	Piece->AngularDamping = AngularDamping * ArchetypeRules.AngularDampingMultiplier;
	Piece->LaunchSpeedMultiplier = ArchetypeRules.LaunchSpeedMultiplier;
	Piece->CenterOfMassOffsetZ = PieceThickness
		* ArchetypeRules.ThicknessMultiplier
		* ArchetypeRules.CenterOfMassHeightFraction;
	Piece->bAllowEdgeTipping = bAllowEdgeTipping;
	Piece->bUseContinuousCollisionDetection = bUseContinuousCollisionDetection;
	Piece->PositionSolverIterations = PositionSolverIterations;
	Piece->VelocitySolverIterations = VelocitySolverIterations;
	Piece->InitializePiece(
		Team,
		PieceId,
		PieceRadius * ArchetypeRules.RadiusMultiplier,
		PieceThickness * ArchetypeRules.ThicknessMultiplier,
		Archetype,
		bIsBobStriker,
		OwningPlayerSlot,
		bShowPlayerIdentity);
	if (bTestArenaMode)
	{
		Piece->EnableTestArenaVisuals();
	}
	Pieces.Add(Piece);
	return Piece;
}

UFlickGameInstance* AFlickGameMode::GetFlickGameInstance() const
{
	return GetGameInstance<UFlickGameInstance>();
}

UFlickSessionSubsystem* AFlickGameMode::GetFlickSessionSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickSessionSubsystem>() : nullptr;
}

UFlickMatchmakingCoordinatorSubsystem* AFlickGameMode::GetFlickMatchmakingCoordinatorSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>() : nullptr;
}

UFlickRankingSubsystem* AFlickGameMode::GetFlickRankingSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickRankingSubsystem>() : nullptr;
}

UFlickRankedBackendSubsystem* AFlickGameMode::GetFlickRankedBackendSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFlickRankedBackendSubsystem>() : nullptr;
}
