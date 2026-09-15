#include "Game/FlickGameModePrivate.h"
#include "Core/FlickLineupRules.h"

using namespace FlickGameModePrivate;

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
		TArray<EFlickPieceArchetype> DefaultLineup;
		for (int32 PieceSlot = 0; PieceSlot < FlickLineupRules::PiecesPerLineup; ++PieceSlot)
		{
			DefaultLineup.Add(GetClassLoadoutPiece(ExistingClass, PieceSlot));
		}
		PlayerState->SetNetworkSelectedLineup(DefaultLineup);
	}

	FrontendScreen = EFlickFrontendScreen::ClassSelect;
	FlickGameState->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	FlickGameState->SetNetworkClassSelectionState(true, InitialClassSelectionTimeLimit);
	SetCameraForFrontend();
	UE_LOG(LogFlick, Log, TEXT("NETWORK_CLASS_SELECTION_STARTED: %.0f second limit"), InitialClassSelectionTimeLimit);
}

void AFlickGameMode::SetNetworkPlayerClass(
	APlayerController* RequestingPlayer,
	const EFlickLineupPreset Preset,
	const TArray<EFlickPieceArchetype>& Lineup)
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
	if (!PlayerState->SetNetworkSelectedLineup(Lineup))
	{
		UE_LOG(LogFlick, Warning, TEXT("Rejected invalid or duplicate network lineup"));
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
		|| !PlayerState || PlayerState->GetTeam() == EFlickTeam::None
		|| !FlickLineupRules::IsValid(PlayerState->GetNetworkSelectedLineup()))
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
	// A private-match participant can own more than one arena slot. Class choice
	// belongs to the human participant, not to each controlled slot, so waiting
	// for PlayersPerTeam * 2 confirmations can only end via the timeout. Public
	// matchmaking still requires the complete one-player-per-slot roster.
	return FlickTeamRules::HasRequiredClassConfirmationCount(
		bPrivateMatchActive,
		ParticipantCount,
		CurrentPlayersPerTeam);
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
	if (bNetworkMatchRequested)
	{
		const AFlickPlayerState* SlotOwner = bPrivateMatchActive
			? GetPrivateSlotOwner(Team, PlayerSlot)
			: GetLobbyPlayer(Team, PlayerSlot);
		if (SlotOwner && FlickLineupRules::IsValid(SlotOwner->GetNetworkSelectedLineup())
			&& SlotOwner->GetNetworkSelectedLineup().IsValidIndex(PieceSlot))
		{
			return SlotOwner->GetNetworkSelectedLineup()[PieceSlot];
		}
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
	if (bNetworkMatchStarted && !CoordinatorMatchId.IsEmpty())
	{
		if (UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetFlickMatchmakingCoordinatorSubsystem())
		{
			Coordinator->NotifyServerMatchStarted();
		}
	}
	bClassSelectionStartsTrainingBotMatch = false;
	// Switchyard has graduated from the isolated Test playlist and is now the
	// authoritative arena for every Classic match format. BOB keeps its own
	// pocket board, but shares the premium puck presentation below.
	bTestArenaMode = FlickModeRules::Get(NormalizeMatchVariant(SelectedMatchVariant)).bUseSwitchyardArena;
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
	bTutorialMode = false;
	bTutorialCompleted = false;
	bTutorialAdvancePending = false;
	TutorialTransitionRemaining = 0.0f;
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
	bTutorialMode = false;
	BeginTrainingActivity(false);
}

float AFlickGameMode::GetFreeCameraLookSensitivity() const
{
	const UFlickGameInstance* Instance = GetFlickGameInstance();
	return Instance ? Instance->GetFreeCameraLookSensitivity() : 0.33f;
}

float AFlickGameMode::GetFreeCameraMoveSensitivity() const
{
	const UFlickGameInstance* Instance = GetFlickGameInstance();
	return Instance ? Instance->GetFreeCameraMoveSensitivity() : 0.38f;
}

float AFlickGameMode::GetShotMouseSensitivity() const
{
	const UFlickGameInstance* Instance = GetFlickGameInstance();
	return Instance ? Instance->GetShotMouseSensitivity() : 0.5f;
}

float AFlickGameMode::GetGameplayCameraSensitivity() const
{
	const UFlickGameInstance* Instance = GetFlickGameInstance();
	return Instance ? Instance->GetGameplayCameraSensitivity() : 0.35f;
}

FString AFlickGameMode::GetClassName(const EFlickLineupPreset Preset) const
{
	const UFlickGameInstance* Instance = GetFlickGameInstance();
	return Instance ? Instance->GetClassName(Preset) : GetLineupPresetName(Preset);
}

void AFlickGameMode::StartTrainingBotMatch()
{
	bTutorialMode = false;
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

void AFlickGameMode::StartTutorialMode()
{
	if (GetNetMode() != NM_Standalone
		|| bNetworkMatchRequested
		|| bPartyRequested
		|| bMatchmakingRequested)
	{
		UE_LOG(LogFlick, Warning, TEXT("Tutorial start rejected because the current session is not offline"));
		return;
	}

	SelectedMatchVariant = EFlickMatchVariant::Classic;
	MatchmakingPlayersPerTeam = 1;
	bTutorialMode = true;
	BeginTrainingActivity(false);
	SetupTutorialStage(0);
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
	bTestArenaMode = FlickModeRules::Get(NormalizeMatchVariant(SelectedMatchVariant)).bUseSwitchyardArena;

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
		if (!bTestArenaMode)
		{
			MatchmakingPlayersPerTeam = 1;
		}
		if (SelectedMatchVariant == EFlickMatchVariant::Classic)
		{
			EnsureActivePlayerClasses(MatchmakingPlayersPerTeam);
			RandomizeOtherLocalPlayerClasses(MatchmakingPlayersPerTeam);
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
	if (!bAgainstBot && !bTutorialMode)
	{
		CaptureTrainingResetSnapshot();
	}
	if (bTutorialMode)
	{
		PushHudEvent(TEXT("TRAINING  |  GUIDED TUTORIAL"), FLinearColor(0.15f, 0.9f, 1.0f, 1.0f), 2.6f);
	}
	else if (!bTestArenaMode)
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
		bTutorialMode ? TEXT("guided tutorial") : bTestArenaMode ? TEXT("test arena bot match") : bAgainstBot ? TEXT("bot training") : TEXT("free-play training"),
		*GetMatchVariantName(SelectedMatchVariant),
		CurrentPlayersPerTeam,
		CurrentPlayersPerTeam);
}

FString AFlickGameMode::GetTutorialTitle() const
{
	static const TCHAR* Titles[TutorialStageTotal] =
	{
		TEXT("DIRECT CONTACT"),
		TEXT("CONTROL THE POWER"),
		TEXT("SCORE A KNOCKOUT"),
		TEXT("USE THE SWITCHYARD")
	};
	return bTutorialCompleted ? TEXT("TUTORIAL COMPLETE") : Titles[FMath::Clamp(TutorialStageIndex, 0, TutorialStageTotal - 1)];
}

FString AFlickGameMode::GetTutorialObjective() const
{
	static const TCHAR* Objectives[TutorialStageTotal] =
	{
		TEXT("Hit the orange puck with your blue Standard puck."),
		TEXT("Release a controlled shot and stop inside the center circle."),
		TEXT("Use the Striker to knock the Compact puck out of the arena."),
		TEXT("Pass the Bouncer over the bright dot to activate its divider.")
	};
	return bTutorialCompleted
		? TEXT("You are ready for Training, Casual, and Competitive play.")
		: Objectives[FMath::Clamp(TutorialStageIndex, 0, TutorialStageTotal - 1)];
}

FString AFlickGameMode::GetTutorialHint() const
{
	static const TCHAR* Hints[TutorialStageTotal] =
	{
		TEXT("LMB aim  /  drag for power  /  release to shoot"),
		TEXT("A shorter drag gives a softer shot. Press R to retry."),
		TEXT("Aim through the target and use the Striker's extra launch speed."),
		TEXT("Any moving puck can press a switch; the divider rises immediately.")
	};
	return bTutorialCompleted
		? TEXT("Press R to run the tutorial again, or ESC to leave.")
		: Hints[FMath::Clamp(TutorialStageIndex, 0, TutorialStageTotal - 1)];
}

void AFlickGameMode::SetupTutorialStage(const int32 StageIndex)
{
	if (!IsTutorialMode() || !GetWorld())
	{
		return;
	}

	TutorialStageIndex = FMath::Clamp(StageIndex, 0, TutorialStageTotal - 1);
	bTutorialCompleted = false;
	bTutorialAdvancePending = false;
	TutorialTransitionRemaining = 0.0f;
	TutorialShotPieceId = INDEX_NONE;
	TutorialTargetPieceId = INDEX_NONE;
	ResetTrainingBotThinking();
	ResetShotClock();
	ClearControllerAiming();
	DestroyPieces();
	if (TestArenaActor && IsValid(TestArenaActor))
	{
		TestArenaActor->ResetMechanisms();
	}

	int32 NextPieceId = 1001;
	auto SpawnTutorialPiece = [this, &NextPieceId](
		const EFlickTeam Team,
		const EFlickPieceArchetype Archetype,
		const FVector2D Position)
	{
		const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
		const float SpawnZ = ArenaSurfaceZ + PieceThickness * Rules.ThicknessMultiplier * 0.5f + 3.0f;
		return SpawnPiece(Team, NextPieceId++, FVector(Position.X, Position.Y, SpawnZ), Archetype);
	};

	AFlickPiece* ShotPiece = nullptr;
	AFlickPiece* TargetPiece = nullptr;
	switch (TutorialStageIndex)
	{
	case 0:
		ShotPiece = SpawnTutorialPiece(EFlickTeam::Player1, EFlickPieceArchetype::Standard, FVector2D(0.0f, -330.0f));
		TargetPiece = SpawnTutorialPiece(EFlickTeam::Player2, EFlickPieceArchetype::Standard, FVector2D(0.0f, 120.0f));
		break;
	case 1:
		ShotPiece = SpawnTutorialPiece(EFlickTeam::Player1, EFlickPieceArchetype::Standard, FVector2D(0.0f, -390.0f));
		break;
	case 2:
		ShotPiece = SpawnTutorialPiece(EFlickTeam::Player1, EFlickPieceArchetype::Striker, FVector2D(0.0f, -255.0f));
		TargetPiece = SpawnTutorialPiece(EFlickTeam::Player2, EFlickPieceArchetype::Compact, FVector2D(0.0f, 535.0f));
		break;
	default:
	{
		FVector SwitchLocation(0.0f, -350.0f, ArenaSurfaceZ);
		if (TestArenaActor && IsValid(TestArenaActor))
		{
			float LowestY = TNumericLimits<float>::Max();
			for (int32 Index = 0; Index < TestArenaActor->GetMechanismCount(); ++Index)
			{
				const FVector Candidate = TestArenaActor->GetSwitchWorldCenter(Index);
				if (Candidate.Y < LowestY)
				{
					LowestY = Candidate.Y;
					SwitchLocation = Candidate;
				}
			}
		}
		FVector2D Outward(SwitchLocation.X, SwitchLocation.Y);
		if (!Outward.Normalize())
		{
			Outward = FVector2D(0.0f, -1.0f);
		}
		ShotPiece = SpawnTutorialPiece(
			EFlickTeam::Player1,
			EFlickPieceArchetype::Bouncer,
			FVector2D(SwitchLocation.X, SwitchLocation.Y) - Outward * 245.0f);
		TargetPiece = SpawnTutorialPiece(
			EFlickTeam::Player2,
			EFlickPieceArchetype::Blocker,
			FVector2D(SwitchLocation.X, SwitchLocation.Y) + Outward * 115.0f);
		break;
	}
	}

	TutorialShotPieceId = ShotPiece ? ShotPiece->GetPieceId() : INDEX_NONE;
	TutorialTargetPieceId = TargetPiece ? TargetPiece->GetPieceId() : INDEX_NONE;
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetCurrentTeam(EFlickTeam::Player1);
		State->SetCurrentTeamPlayerSlot(0);
		State->SetActivePieceCounts(ShotPiece ? 1 : 0, TargetPiece ? 1 : 0);
		State->SetMatchPhase(EFlickMatchPhase::Aiming);
	}
	SetCameraViewForTeam(EFlickTeam::Player1, true);
	if (AudioDirector)
	{
		AudioDirector->PlayTurn(EFlickTeam::Player1);
	}
	PushHudEvent(FString::Printf(TEXT("LESSON %d / %d  |  %s"), TutorialStageIndex + 1, TutorialStageTotal, *GetTutorialTitle()),
		FLinearColor(0.15f, 0.9f, 1.0f, 1.0f), 2.2f);
}

void AFlickGameMode::ResolveTutorialShot()
{
	if (!IsTutorialMode() || bTutorialAdvancePending || bTutorialCompleted)
	{
		return;
	}

	bool bSucceeded = false;
	switch (TutorialStageIndex)
	{
	case 0:
		bSucceeded = TutorialTargetPieceId != INDEX_NONE
			&& ResolutionDirectContactPieceIds.Contains(TutorialTargetPieceId);
		break;
	case 1:
		if (const TObjectPtr<AFlickPiece>* Entry = Pieces.FindByPredicate([this](const TObjectPtr<AFlickPiece>& Piece)
		{
			return Piece && IsValid(Piece) && Piece->GetPieceId() == TutorialShotPieceId;
		}))
		{
			const FVector Center = ArenaActor ? ArenaActor->GetActorLocation() : FVector::ZeroVector;
			const FVector Location = (*Entry)->GetActorLocation();
			bSucceeded = (*Entry)->IsActive()
				&& FVector2D::Distance(FVector2D(Location.X, Location.Y), FVector2D(Center.X, Center.Y)) <= 165.0f;
		}
		break;
	case 2:
		bSucceeded = TutorialTargetPieceId != INDEX_NONE
			&& ResolutionEliminatedPieceIds.Contains(TutorialTargetPieceId)
			&& TutorialShotPieceId != INDEX_NONE
			&& !ResolutionEliminatedPieceIds.Contains(TutorialShotPieceId);
		break;
	default:
		bSucceeded = ResolutionActivatedSwitchMask != 0;
		break;
	}

	bTutorialAdvancePending = bSucceeded;
	TutorialTransitionRemaining = bSucceeded ? 1.8f : 1.25f;
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	PushHudEvent(bSucceeded ? TEXT("LESSON COMPLETE") : TEXT("TRY AGAIN"),
		bSucceeded ? FLinearColor(0.2f, 1.0f, 0.55f, 1.0f) : FLinearColor(1.0f, 0.45f, 0.18f, 1.0f),
		TutorialTransitionRemaining);
}

void AFlickGameMode::UpdateTutorial(const float DeltaSeconds)
{
	if (!IsTutorialMode() || bTutorialCompleted || TutorialTransitionRemaining <= 0.0f)
	{
		return;
	}

	TutorialTransitionRemaining = FMath::Max(0.0f, TutorialTransitionRemaining - DeltaSeconds);
	if (TutorialTransitionRemaining > 0.0f)
	{
		return;
	}

	if (!bTutorialAdvancePending)
	{
		SetupTutorialStage(TutorialStageIndex);
		return;
	}
	if (TutorialStageIndex + 1 < TutorialStageTotal)
	{
		SetupTutorialStage(TutorialStageIndex + 1);
		return;
	}

	bTutorialCompleted = true;
	bTutorialAdvancePending = false;
	DestroyPieces();
	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetActivePieceCounts(0, 0);
		State->SetMatchPhase(EFlickMatchPhase::WaitingToStart);
	}
	PushHudEvent(TEXT("TUTORIAL COMPLETE  |  READY TO PLAY"), FLinearColor(0.2f, 1.0f, 0.55f, 1.0f), 4.0f);
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
		if (TestArenaActor)
		{
			TestArenaActor->SetTrainingBoardEditMode(true);
		}
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
		if (TestArenaActor)
		{
			TestArenaActor->SetTrainingBoardEditMode(false);
		}
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

