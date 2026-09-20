#include "Game/FlickGameModePrivate.h"
#include "Core/FlickPrivateMatchRules.h"

using namespace FlickGameModePrivate;

bool AFlickGameMode::HostPrivateMatchForPersistentParty(const FString& PartyId, const int32 PartySize)
{
	if (!HasAuthority() || !GetWorld() || PartyId.IsEmpty() || bNetworkMatchStarted)
	{
		return false;
	}
	bPartyRequested = true;
	bMatchmakingRequested = false;
	ActivePartyId = PartyId;
	APlayerController* LocalController = GetWorld()->GetFirstPlayerController();
	AFlickPlayerState* LocalState = LocalController
		? LocalController->GetPlayerState<AFlickPlayerState>() : nullptr;
	if (!LocalState)
	{
		return false;
	}
	LocalState->SetPartyIdentity(PartyId, true, 0);
	if (AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(LocalController))
	{
		FlickController->SetPersistentPartyIdentityFromServer(
			PartyId,
			0,
			FMath::Clamp(PartySize, 1, FlickMaximumPartyMembers),
			true);
	}
	SynchronizePartyState();
	OpenPrivateMatchSetup();
	return bPrivateMatchSetupActive;
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
	bTutorialMode = false;
	bTutorialCompleted = false;
	bTutorialAdvancePending = false;
	TutorialTransitionRemaining = 0.0f;
	bClassSelectionStartsTrainingBotMatch = false;
	bTestArenaMode = false;
	ResetTrainingBotThinking();
	const UFlickPartySubsystem* PersistentParty = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
	if (PersistentParty && PersistentParty->IsActive())
	{
		if (!PersistentParty->IsLocalLeader())
		{
			UE_LOG(LogFlick, Warning, TEXT("Only the party leader can start matchmaking"));
			return;
		}
		QueuePartyForMatchmaking(MatchmakingPlayersPerTeam);
		return;
	}
	if (bPartyRequested)
	{
		QueuePartyForMatchmaking(MatchmakingPlayersPerTeam);
		return;
	}
	if (GetNetMode() != NM_Standalone)
	{
		return;
	}
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

	const UFlickPartySubsystem* PersistentParty = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
	if (PersistentParty && PersistentParty->IsActive())
	{
		const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
		if (!PersistentParty->IsLocalLeader()
			|| !FlickMatchmakingRules::IsPartySizeValid(PersistentParty->GetMemberCount(), TeamSize))
		{
			UE_LOG(LogFlick, Warning, TEXT("Coordinator queue rejected: persistent party does not fit %dv%d"), TeamSize, TeamSize);
			return true;
		}
		if (Coordinator->RequiresSteamTickets() && PersistentParty->GetMemberCount() > 1)
		{
			UE_LOG(LogFlick, Error,
				TEXT("Authenticated party matchmaking requires per-member coordinator tickets; queue was not submitted"));
			return true;
		}

		FFlickCoordinatorQueueRequest Request;
		Request.PartyId = PersistentParty->GetPartyId();
		Request.Variant = SelectedMatchVariant;
		Request.PlayersPerTeam = TeamSize;
		Request.bRanked = bRankedQueueSelected;
		Request.Region = TEXT("auto");
		for (const FFlickPartyMember& PartyMember : PersistentParty->GetMembers())
		{
			FFlickCoordinatorPartyMember& Member = Request.Members.AddDefaulted_GetRef();
			Member.AccountId = PartyMember.UserId;
			Member.DisplayName = PartyMember.DisplayName;
			Member.PartySlot = PartyMember.Slot;
			Member.Rating = RankedQueueRating;
		}
		if (CoordinatorAllocatedHandle.IsValid())
		{
			Coordinator->OnAllocated.Remove(CoordinatorAllocatedHandle);
		}
		CoordinatorAllocatedHandle = Coordinator->OnAllocated.AddUObject(this, &AFlickGameMode::HandleCoordinatorAllocation);
		if (AFlickGameState* State = GetFlickGameState())
		{
			State->SetMatchmakingState(true, false, bRankedQueueSelected, RankedQueueRating, 0);
		}
		Coordinator->QueueParty(Request,
			[](const bool bAccepted, const FString& Error)
			{
				if (!bAccepted)
				{
					UE_LOG(LogFlick, Error, TEXT("COORDINATOR_PARTY_QUEUE_REJECTED: %s"), *Error);
				}
			});
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
	if (const UFlickPartySubsystem* Party = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
		Party && Party->IsActive())
	{
		if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem())
		{
			Sessions->PublishPartyCoordinatorAllocation(Allocation);
		}
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
			// Joining the public queue is the ready confirmation. Start as soon as
			// the dedicated server has verified the complete reserved roster.
			PlayerState->SetLobbyReady(true);
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

void AFlickGameMode::SynchronizePartyState()
{
	if (!bPartyRequested || !GetWorld())
	{
		return;
	}

	const int32 PartySize = GetPartyMemberCount();
	AFlickPlayerState* PartyLeader = nullptr;
	for (AFlickPlayerState* Member : GetPrivateMatchParticipants())
	{
		if (Member && Member->GetPartySlot() != INDEX_NONE
			&& (Member->IsPartyLeader()
				|| !PartyLeader
				|| Member->GetPartySlot() < PartyLeader->GetPartySlot()))
		{
			if (Member->IsPartyLeader() || !PartyLeader || !PartyLeader->IsPartyLeader())
			{
				PartyLeader = Member;
			}
		}
	}
	if (PartyLeader && !PartyLeader->IsPartyLeader())
	{
		PartyLeader->SetPartyRole(true, PartyLeader->GetPartySlot());
	}

	for (AFlickPlayerState* Member : GetPrivateMatchParticipants())
	{
		if (!Member || Member->GetPartySlot() == INDEX_NONE)
		{
			continue;
		}
		const bool bLeader = Member == PartyLeader;
		if (Member->IsPartyLeader() != bLeader)
		{
			Member->SetPartyRole(bLeader, Member->GetPartySlot());
		}
		if (AFlickPlayerController* Controller = Cast<AFlickPlayerController>(Member->GetOwner()))
		{
			Controller->SetPersistentPartyIdentityFromServer(
				Member->GetPartyId(),
				Member->GetPartySlot(),
				FMath::Max(PartySize, 1),
				bLeader);
		}
	}

	if (AFlickGameState* State = GetFlickGameState())
	{
		State->SetPartyState(
			PartySize > 0,
			PartyLeader ? GetPlayerOnlineId(PartyLeader) : FString(),
			FlickMaximumPartyMembers);
	}
	if (UFlickSessionSubsystem* Sessions = GetFlickSessionSubsystem(); Sessions && Sessions->IsPartySession())
	{
		Sessions->UpdatePartyMemberCount(FMath::Max(PartySize, 1));
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
	// Private matches reuse the party leader's listen server, never an
	// allocated public server. Standalone remains available for local play.
	if (GetNetMode() == NM_DedicatedServer || bMatchmakingRequested || bNetworkMatchStarted)
	{
		return;
	}
	const TArray<AFlickPlayerState*> Participants = GetPrivateMatchParticipants();
	if (Participants.IsEmpty())
	{
		return;
	}
	const APlayerController* HostController = GetWorld()->GetFirstPlayerController();
	const AFlickPlayerState* HostState = HostController
		? HostController->GetPlayerState<AFlickPlayerState>() : nullptr;
	if (bPartyRequested && (!HostController || !HostController->IsLocalController()
		|| !HostState || !HostState->IsPartyLeader()))
	{
		return;
	}

	PrivateMatchReturnVariant = SelectedMatchVariant;
	PrivateMatchReturnPlayersPerTeam = SelectedMatchVariant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(MatchmakingPlayersPerTeam);
	PrivateMatchSettings = FFlickPrivateMatchSettings();
	PrivateMatchSettings.Variant = EFlickMatchVariant::Classic;
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
	const APlayerController* HostController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const AFlickPlayerState* HostState = HostController
		? HostController->GetPlayerState<AFlickPlayerState>() : nullptr;
	if (!bPrivateMatchSetupActive || Direction == 0 || GetNetMode() == NM_DedicatedServer
		|| (bPartyRequested && (!HostController || !HostController->IsLocalController()
			|| !HostState || !HostState->IsPartyLeader())))
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
	case EFlickPrivateMatchSetting::Mode:
		FlickPrivateMatchRules::CycleMode(PrivateMatchSettings, Step);
		SelectedMatchVariant = PrivateMatchSettings.Variant;
		AutoAssignPrivateMatchSlots();
		break;
	case EFlickPrivateMatchSetting::TeamSize:
		PrivateMatchSettings.Variant = EFlickMatchVariant::Classic;
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
	AFlickPlayerState* ExistingOwner = GetPrivateSlotOwner(Team, PlayerSlot);
	if (ExistingOwner && ExistingOwner != RequestingState)
	{
		UE_LOG(LogFlick, Verbose, TEXT("Rejected attempt to claim another private-match player's slot"));
		return;
	}
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
	if (GetNetMode() == NM_DedicatedServer || !bPrivateMatchSetupActive || !IsNetworkLobby())
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
	const bool bHost = RequestingPlayer && RequestingPlayer->IsLocalController();
	if (!CanStartPrivateMatch()
		|| !bHost || (bPartyRequested && (!RequestingState || !RequestingState->IsPartyLeader())))
	{
		return;
	}
	FlickPrivateMatchRules::Normalize(PrivateMatchSettings);
	// Custom games must never register or settle a competitive result.
	bMatchmakingRequested = false;
	bRankedRequested = false;
	SelectedMatchVariant = NormalizeMatchVariant(PrivateMatchSettings.Variant);
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

bool AFlickGameMode::CanQueuePartyForMatchmaking(const int32 PlayersPerTeam) const
{
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	if (const UFlickPartySubsystem* Party = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
		Party && Party->IsActive())
	{
		return Party->IsLocalLeader()
			&& FlickMatchmakingRules::IsPartySizeValid(Party->GetMemberCount(), TeamSize);
	}
	const int32 PartySize = GetPartyMemberCount();
	if (!bPartyRequested || !FlickMatchmakingRules::IsPartySizeValid(PartySize, TeamSize))
	{
		return false;
	}

	for (int32 PartySlot = 0; PartySlot < FlickMaximumPartyMembers; ++PartySlot)
	{
		if (const AFlickPlayerState* Member = GetPartyMember(PartySlot);
			Member && Member->IsPartyLeader())
		{
			return true;
		}
	}
	return false;
}

void AFlickGameMode::QueuePartyForMatchmaking(const int32 PlayersPerTeam)
{
	if (!CanQueuePartyForMatchmaking(PlayersPerTeam))
	{
		UE_LOG(LogFlick, Warning, TEXT("The current party does not fit that matchmaking team size"));
		return;
	}
	const int32 TeamSize = FlickTeamRules::ClampPlayersPerTeam(PlayersPerTeam);
	const UFlickPartySubsystem* PersistentParty = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
	const int32 PartySize = PersistentParty && PersistentParty->IsActive()
		? PersistentParty->GetMemberCount() : GetPartyMemberCount();
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
	if (PersistentParty && PersistentParty->IsActive())
	{
		UE_LOG(LogFlick, Error, TEXT("Persistent parties require the matchmaking coordinator"));
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
		SelectedMatchVariant = NormalizeMatchVariant(PrivateMatchSettings.Variant);
		DisconnectedPrivateControlledSlots.Reset();
		ApplyMatchConfiguration(SelectedMatchVariant, PrivateMatchSettings.PlayersPerTeam);
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

