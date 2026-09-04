#include "Game/FlickGameState.h"

#include "Core/FlickSeriesRules.h"
#include "Core/FlickTeamRules.h"
#include "Net/UnrealNetwork.h"

AFlickGameState::AFlickGameState()
{
	bReplicates = true;
}

void AFlickGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickGameState, MatchPhase);
	DOREPLIFETIME(AFlickGameState, CurrentTeam);
	DOREPLIFETIME(AFlickGameState, PlayersPerTeam);
	DOREPLIFETIME(AFlickGameState, CurrentTeamPlayerSlot);
	DOREPLIFETIME(AFlickGameState, WinnerTeam);
	DOREPLIFETIME(AFlickGameState, bDraw);
	DOREPLIFETIME(AFlickGameState, bSeriesComplete);
	DOREPLIFETIME(AFlickGameState, RoundNumber);
	DOREPLIFETIME(AFlickGameState, RoundsToWin);
	DOREPLIFETIME(AFlickGameState, Player1RoundsWon);
	DOREPLIFETIME(AFlickGameState, Player2RoundsWon);
	DOREPLIFETIME(AFlickGameState, RoundStartingTeam);
	DOREPLIFETIME(AFlickGameState, Player1ActivePieces);
	DOREPLIFETIME(AFlickGameState, Player2ActivePieces);
	DOREPLIFETIME(AFlickGameState, StartingPiecesPerTeam);
	DOREPLIFETIME(AFlickGameState, TurnNumber);
	DOREPLIFETIME(AFlickGameState, KickoffShotsLocked);
	DOREPLIFETIME(AFlickGameState, KickoffShotsRequired);
	DOREPLIFETIME(AFlickGameState, DramaticEvent);
	DOREPLIFETIME(AFlickGameState, DramaticEventTeam);
	DOREPLIFETIME(AFlickGameState, DramaticEventValue);
	DOREPLIFETIME(AFlickGameState, DramaticEventImpactCount);
	DOREPLIFETIME(AFlickGameState, DramaticEventBonusPoints);
	DOREPLIFETIME(AFlickGameState, DramaticEventSerial);
	DOREPLIFETIME(AFlickGameState, DramaticEventDuration);
	DOREPLIFETIME(AFlickGameState, DramaticEventEndServerTime);
	DOREPLIFETIME(AFlickGameState, bShotClockActive);
	DOREPLIFETIME(AFlickGameState, ShotClockDuration);
	DOREPLIFETIME(AFlickGameState, ShotClockEndServerTime);
	DOREPLIFETIME(AFlickGameState, bRoundAdvanceTimerActive);
	DOREPLIFETIME(AFlickGameState, RoundAdvanceTimerDuration);
	DOREPLIFETIME(AFlickGameState, RoundAdvanceTimerEndServerTime);
	DOREPLIFETIME(AFlickGameState, bNetworkClassSelectionActive);
	DOREPLIFETIME(AFlickGameState, NetworkClassSelectionDuration);
	DOREPLIFETIME(AFlickGameState, NetworkClassSelectionEndServerTime);
	DOREPLIFETIME(AFlickGameState, Player1ShotsTaken);
	DOREPLIFETIME(AFlickGameState, Player2ShotsTaken);
	DOREPLIFETIME(AFlickGameState, LastShotTeam);
	DOREPLIFETIME(AFlickGameState, LastShotPieceId);
	DOREPLIFETIME(AFlickGameState, LastShotPower);
	DOREPLIFETIME(AFlickGameState, ImpactsThisShot);
	DOREPLIFETIME(AFlickGameState, StrongestImpactThisShot);
	DOREPLIFETIME(AFlickGameState, Player1EliminatedThisShot);
	DOREPLIFETIME(AFlickGameState, Player2EliminatedThisShot);
	DOREPLIFETIME(AFlickGameState, PlayerMatchStats);
	DOREPLIFETIME(AFlickGameState, ActiveMatchVariant);
	DOREPLIFETIME(AFlickGameState, bNetworkLobbyActive);
	DOREPLIFETIME(AFlickGameState, LobbySelectedVariant);
	DOREPLIFETIME(AFlickGameState, bMatchmakingLobby);
	DOREPLIFETIME(AFlickGameState, bMatchmakingTimedOut);
	DOREPLIFETIME(AFlickGameState, bRankedMatch);
	DOREPLIFETIME(AFlickGameState, RankedQueueRating);
	DOREPLIFETIME(AFlickGameState, RankedSearchRange);
	DOREPLIFETIME(AFlickGameState, MatchId);
	DOREPLIFETIME(AFlickGameState, bMatchResultFinalized);
	DOREPLIFETIME(AFlickGameState, FinalMatchOutcome);
	DOREPLIFETIME(AFlickGameState, bMatchEndedByForfeit);
	DOREPLIFETIME(AFlickGameState, MatchCompletedUnixTime);
	DOREPLIFETIME(AFlickGameState, bPartyActive);
	DOREPLIFETIME(AFlickGameState, PartyLeaderUserId);
	DOREPLIFETIME(AFlickGameState, PartyMaximumMembers);
	DOREPLIFETIME(AFlickGameState, bPrivateMatchLobbyActive);
	DOREPLIFETIME(AFlickGameState, PrivateMatchSettings);
	DOREPLIFETIME(AFlickGameState, ArenaSurfaceZ);
	DOREPLIFETIME(AFlickGameState, MaxDragDistance);
	DOREPLIFETIME(AFlickGameState, MinDragDistance);
	DOREPLIFETIME(AFlickGameState, PowerExponent);
	DOREPLIFETIME(AFlickGameState, MaxLaunchSpeed);
}

void AFlickGameState::ResetMatchState()
{
	ResetSeriesState(RoundsToWin);
}

void AFlickGameState::ResetSeriesState(const int32 InRoundsToWin)
{
	RoundNumber = 1;
	RoundsToWin = FMath::Max(1, InRoundsToWin);
	Player1RoundsWon = 0;
	Player2RoundsWon = 0;
	bSeriesComplete = false;
	RoundStartingTeam = FlickSeriesRules::GetStartingTeam(RoundNumber);
	StartingPiecesPerTeam = 4;
	InitializePlayerMatchStats(PlayersPerTeam);
	ResetRoundState();
}

void AFlickGameState::ResetRoundState()
{
	MatchPhase = EFlickMatchPhase::WaitingToStart;
	CurrentTeam = RoundStartingTeam;
	CurrentTeamPlayerSlot = 0;
	WinnerTeam = EFlickTeam::None;
	bDraw = false;
	Player1ActivePieces = 0;
	Player2ActivePieces = 0;
	TurnNumber = 1;
	KickoffShotsLocked = 0;
	KickoffShotsRequired = 0;
	ClearDramaticEvent();
	bShotClockActive = false;
	ShotClockEndServerTime = 0.0f;
	bRoundAdvanceTimerActive = false;
	RoundAdvanceTimerEndServerTime = 0.0f;
	Player1ShotsTaken = 0;
	Player2ShotsTaken = 0;
	LastShotTeam = EFlickTeam::None;
	LastShotPieceId = 0;
	LastShotPower = 0.0f;
	ImpactsThisShot = 0;
	StrongestImpactThisShot = 0.0f;
	Player1EliminatedThisShot = 0;
	Player2EliminatedThisShot = 0;
}

void AFlickGameState::CompleteRound(const EFlickMatchOutcome Outcome)
{
	const FFlickSeriesRoundResult Result = FlickSeriesRules::ApplyRoundOutcome(
		Outcome,
		Player1RoundsWon,
		Player2RoundsWon,
		RoundsToWin,
		RoundNumber,
		GetTeamScore(EFlickTeam::Player1),
		GetTeamScore(EFlickTeam::Player2));
	Player1RoundsWon = Result.Player1RoundsWon;
	Player2RoundsWon = Result.Player2RoundsWon;
	WinnerTeam = Result.bSeriesComplete ? Result.SeriesWinner : Result.RoundWinner;
	bDraw = Result.bSeriesComplete ? Result.bSeriesDraw : Result.bRoundDraw;
	bSeriesComplete = Result.bSeriesComplete;
	MatchPhase = EFlickMatchPhase::RoundOver;
}

bool AFlickGameState::AdvanceToNextRound()
{
	if (bSeriesComplete || MatchPhase != EFlickMatchPhase::RoundOver)
	{
		return false;
	}

	++RoundNumber;
	RoundStartingTeam = FlickSeriesRules::GetStartingTeam(RoundNumber);
	ResetRoundState();
	return true;
}

void AFlickGameState::SetMatchPhase(const EFlickMatchPhase InPhase)
{
	MatchPhase = InPhase;
}

void AFlickGameState::SetCurrentTeam(const EFlickTeam InTeam)
{
	CurrentTeam = InTeam;
}

void AFlickGameState::SetShotClockState(const bool bInActive, const float InDuration)
{
	ShotClockDuration = FMath::Max(0.1f, InDuration);
	bShotClockActive = bInActive;
	ShotClockEndServerTime = bInActive
		? GetServerWorldTimeSeconds() + ShotClockDuration
		: 0.0f;
	ForceNetUpdate();
}

float AFlickGameState::GetShotClockTimeRemaining() const
{
	return bShotClockActive
		? FMath::Max(0.0f, ShotClockEndServerTime - GetServerWorldTimeSeconds())
		: 0.0f;
}

float AFlickGameState::GetShotClockFraction(const EFlickTeam Team) const
{
	if (!bShotClockActive || Team != CurrentTeam)
	{
		return 1.0f;
	}
	return FMath::Clamp(
		GetShotClockTimeRemaining() / FMath::Max(0.1f, ShotClockDuration),
		0.0f,
		1.0f);
}

void AFlickGameState::SetRoundAdvanceTimerState(const bool bInActive, const float InDuration)
{
	RoundAdvanceTimerDuration = FMath::Max(0.1f, InDuration);
	bRoundAdvanceTimerActive = bInActive;
	RoundAdvanceTimerEndServerTime = bInActive
		? GetServerWorldTimeSeconds() + RoundAdvanceTimerDuration
		: 0.0f;
	ForceNetUpdate();
}

float AFlickGameState::GetRoundAdvanceTimeRemaining() const
{
	return bRoundAdvanceTimerActive
		? FMath::Max(0.0f, RoundAdvanceTimerEndServerTime - GetServerWorldTimeSeconds())
		: 0.0f;
}

void AFlickGameState::SetNetworkClassSelectionState(
	const bool bInActive,
	const float InDuration)
{
	NetworkClassSelectionDuration = FMath::Max(0.1f, InDuration);
	bNetworkClassSelectionActive = bInActive;
	NetworkClassSelectionEndServerTime = bInActive
		? GetServerWorldTimeSeconds() + NetworkClassSelectionDuration
		: 0.0f;
	ForceNetUpdate();
}

float AFlickGameState::GetNetworkClassSelectionTimeRemaining() const
{
	return bNetworkClassSelectionActive
		? FMath::Max(0.0f, NetworkClassSelectionEndServerTime - GetServerWorldTimeSeconds())
		: 0.0f;
}

void AFlickGameState::SetTeamFormat(const int32 InPlayersPerTeam)
{
	PlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(InPlayersPerTeam);
	CurrentTeamPlayerSlot = FMath::Clamp(CurrentTeamPlayerSlot, 0, PlayersPerTeam - 1);
	ForceNetUpdate();
}

void AFlickGameState::SetCurrentTeamPlayerSlot(const int32 InPlayerSlot)
{
	CurrentTeamPlayerSlot = FMath::Clamp(InPlayerSlot, 0, FMath::Max(0, PlayersPerTeam - 1));
	ForceNetUpdate();
}

void AFlickGameState::SetActivePieceCounts(const int32 InPlayer1Count, const int32 InPlayer2Count)
{
	Player1ActivePieces = InPlayer1Count;
	Player2ActivePieces = InPlayer2Count;
}

void AFlickGameState::SetStartingPiecesPerTeam(const int32 InStartingPieces)
{
	StartingPiecesPerTeam = FMath::Max(1, InStartingPieces);
}

void AFlickGameState::SetKickoffProgress(const int32 InLockedShots, const int32 InRequiredShots)
{
	KickoffShotsRequired = FMath::Max(0, InRequiredShots);
	KickoffShotsLocked = FMath::Clamp(InLockedShots, 0, KickoffShotsRequired);
	ForceNetUpdate();
}

void AFlickGameState::ShowDramaticEvent(
	const EFlickDramaticEvent InEvent,
	const EFlickTeam InHighlightedTeam,
	const int32 InValue,
	const int32 InImpactCount,
	const int32 InBonusPoints,
	const float InDuration)
{
	DramaticEvent = InEvent;
	DramaticEventTeam = InHighlightedTeam;
	DramaticEventValue = FMath::Max(0, InValue);
	DramaticEventImpactCount = FMath::Max(0, InImpactCount);
	DramaticEventBonusPoints = FMath::Max(0, InBonusPoints);
	DramaticEventDuration = FMath::Max(0.1f, InDuration);
	DramaticEventEndServerTime = GetServerWorldTimeSeconds() + DramaticEventDuration;
	++DramaticEventSerial;
	ForceNetUpdate();
}

void AFlickGameState::ClearDramaticEvent()
{
	DramaticEvent = EFlickDramaticEvent::None;
	DramaticEventTeam = EFlickTeam::None;
	DramaticEventValue = 0;
	DramaticEventImpactCount = 0;
	DramaticEventBonusPoints = 0;
	DramaticEventDuration = 0.0f;
	DramaticEventEndServerTime = 0.0f;
	ForceNetUpdate();
}

float AFlickGameState::GetDramaticEventTimeRemaining() const
{
	return DramaticEvent == EFlickDramaticEvent::None
		? 0.0f
		: FMath::Max(0.0f, DramaticEventEndServerTime - GetServerWorldTimeSeconds());
}

void AFlickGameState::BeginShot(const EFlickTeam ShootingTeam, const int32 PieceId, const float NormalizedPower)
{
	LastShotTeam = ShootingTeam;
	LastShotPieceId = PieceId;
	LastShotPower = FMath::Clamp(NormalizedPower, 0.0f, 1.0f);
	ImpactsThisShot = 0;
	StrongestImpactThisShot = 0.0f;
	Player1EliminatedThisShot = 0;
	Player2EliminatedThisShot = 0;

	if (ShootingTeam == EFlickTeam::Player1)
	{
		++Player1ShotsTaken;
	}
	else if (ShootingTeam == EFlickTeam::Player2)
	{
		++Player2ShotsTaken;
	}
}

void AFlickGameState::RecordImpact(const float ImpactStrength)
{
	++ImpactsThisShot;
	StrongestImpactThisShot = FMath::Max(StrongestImpactThisShot, FMath::Max(0.0f, ImpactStrength));
}

void AFlickGameState::RecordElimination(const EFlickTeam EliminatedTeam)
{
	if (EliminatedTeam == EFlickTeam::Player1)
	{
		++Player1EliminatedThisShot;
	}
	else if (EliminatedTeam == EFlickTeam::Player2)
	{
		++Player2EliminatedThisShot;
	}
}

void AFlickGameState::InitializePlayerMatchStats(const int32 InPlayersPerTeam)
{
	if (!HasAuthority())
	{
		return;
	}

	PlayerMatchStats.Reset();
	const int32 SafePlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(InPlayersPerTeam);
	PlayerMatchStats.Reserve(SafePlayersPerTeam * 2);
	for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
	{
		for (int32 PlayerSlot = 0; PlayerSlot < SafePlayersPerTeam; ++PlayerSlot)
		{
			FFlickPlayerMatchStats& Stats = PlayerMatchStats.AddDefaulted_GetRef();
			Stats.Team = Team;
			Stats.PlayerSlot = PlayerSlot;
		}
	}
	Player1LastShootingPlayerSlot = 0;
	Player2LastShootingPlayerSlot = 0;
	ForceNetUpdate();
}

void AFlickGameState::RecordPlayerShot(const EFlickTeam Team, const int32 PlayerSlot)
{
	if (!HasAuthority())
	{
		return;
	}
	if (FFlickPlayerMatchStats* Stats = FindMutablePlayerMatchStats(Team, PlayerSlot))
	{
		++Stats->Shots;
		Stats->Score += FMath::Max(0, ScorePerShot);
		if (Team == EFlickTeam::Player1)
		{
			Player1LastShootingPlayerSlot = PlayerSlot;
		}
		else if (Team == EFlickTeam::Player2)
		{
			Player2LastShootingPlayerSlot = PlayerSlot;
		}
		ForceNetUpdate();
	}
}

void AFlickGameState::RecordPlayerImpact(const EFlickTeam Team, const int32 PlayerSlot)
{
	if (!HasAuthority())
	{
		return;
	}
	if (FFlickPlayerMatchStats* Stats = FindMutablePlayerMatchStats(Team, PlayerSlot))
	{
		++Stats->Impacts;
		Stats->Score += FMath::Max(0, ScorePerImpact);
		ForceNetUpdate();
	}
}

void AFlickGameState::RecordPlayerKnockout(const EFlickTeam Team, const int32 PlayerSlot)
{
	if (!HasAuthority())
	{
		return;
	}
	if (FFlickPlayerMatchStats* Stats = FindMutablePlayerMatchStats(Team, PlayerSlot))
	{
		++Stats->Knockouts;
		Stats->Score += FMath::Max(0, ScorePerKnockout);
		ForceNetUpdate();
	}
}

void AFlickGameState::RecordPlayerDoubleKnockout(const EFlickTeam Team, const int32 PlayerSlot)
{
	if (!HasAuthority())
	{
		return;
	}
	if (FFlickPlayerMatchStats* Stats = FindMutablePlayerMatchStats(Team, PlayerSlot))
	{
		++Stats->DoubleKnockouts;
		ForceNetUpdate();
	}
}

void AFlickGameState::RecordPlayerBonus(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const int32 BonusPoints)
{
	if (!HasAuthority() || BonusPoints <= 0)
	{
		return;
	}
	if (FFlickPlayerMatchStats* Stats = FindMutablePlayerMatchStats(Team, PlayerSlot))
	{
		Stats->Score += BonusPoints;
		ForceNetUpdate();
	}
}

void AFlickGameState::RecordPlayerSurvivingPucks(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const int32 SurvivingPuckCount)
{
	if (!HasAuthority() || SurvivingPuckCount <= 0)
	{
		return;
	}
	if (FFlickPlayerMatchStats* Stats = FindMutablePlayerMatchStats(Team, PlayerSlot))
	{
		Stats->SurvivingPucks += SurvivingPuckCount;
		Stats->Score += SurvivingPuckCount * FMath::Max(0, ScorePerSurvivingPuck);
		ForceNetUpdate();
	}
}

const FFlickPlayerMatchStats* AFlickGameState::FindPlayerMatchStats(
	const EFlickTeam Team,
	const int32 PlayerSlot) const
{
	return PlayerMatchStats.FindByPredicate([Team, PlayerSlot](const FFlickPlayerMatchStats& Stats)
	{
		return Stats.Team == Team && Stats.PlayerSlot == PlayerSlot;
	});
}

int32 AFlickGameState::GetTeamScore(const EFlickTeam Team) const
{
	int32 TeamScore = 0;
	for (const FFlickPlayerMatchStats& Stats : PlayerMatchStats)
	{
		if (Stats.Team == Team)
		{
			TeamScore += FMath::Max(0, Stats.Score);
		}
	}
	return TeamScore;
}

int32 AFlickGameState::GetLastShootingPlayerSlot(const EFlickTeam Team) const
{
	return Team == EFlickTeam::Player2
		? Player2LastShootingPlayerSlot
		: Player1LastShootingPlayerSlot;
}

FFlickPlayerMatchStats* AFlickGameState::FindMutablePlayerMatchStats(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	return PlayerMatchStats.FindByPredicate([Team, PlayerSlot](const FFlickPlayerMatchStats& Stats)
	{
		return Stats.Team == Team && Stats.PlayerSlot == PlayerSlot;
	});
}

void AFlickGameState::AdvanceTurn()
{
	++TurnNumber;
}

void AFlickGameState::SetMatchConfiguration(
	const EFlickMatchVariant InVariant,
	const float InArenaSurfaceZ,
	const float InMaxDragDistance,
	const float InMinDragDistance,
	const float InPowerExponent,
	const float InMaxLaunchSpeed)
{
	ActiveMatchVariant = InVariant;
	ArenaSurfaceZ = InArenaSurfaceZ;
	MaxDragDistance = FMath::Max(InMaxDragDistance, 1.0f);
	MinDragDistance = FMath::Clamp(InMinDragDistance, 0.0f, MaxDragDistance);
	PowerExponent = FMath::Max(InPowerExponent, KINDA_SMALL_NUMBER);
	MaxLaunchSpeed = FMath::Max(InMaxLaunchSpeed, 0.0f);
}

void AFlickGameState::SetNetworkLobbyState(
	const bool bInLobbyActive,
	const EFlickMatchVariant InSelectedVariant)
{
	bNetworkLobbyActive = bInLobbyActive;
	LobbySelectedVariant = InSelectedVariant;
	if (bInLobbyActive)
	{
		bNetworkClassSelectionActive = false;
		NetworkClassSelectionEndServerTime = 0.0f;
	}
	ForceNetUpdate();
}

void AFlickGameState::SetPartyState(
	const bool bInPartyActive,
	const FString& InLeaderUserId,
	const int32 InMaximumMembers)
{
	bPartyActive = bInPartyActive;
	PartyLeaderUserId = bInPartyActive ? InLeaderUserId : FString();
	PartyMaximumMembers = FMath::Max(1, InMaximumMembers);
	ForceNetUpdate();
}

void AFlickGameState::SetPrivateMatchLobbyState(
	const bool bInActive,
	const FFlickPrivateMatchSettings& InSettings)
{
	if (HasAuthority())
	{
		bPrivateMatchLobbyActive = bInActive;
		PrivateMatchSettings = InSettings;
		ForceNetUpdate();
	}
}

void AFlickGameState::SetMatchmakingState(
	const bool bInMatchmakingLobby,
	const bool bInTimedOut,
	const bool bInRankedMatch,
	const int32 InRankedQueueRating,
	const int32 InRankedSearchRange)
{
	bMatchmakingLobby = bInMatchmakingLobby;
	bMatchmakingTimedOut = bInMatchmakingLobby && bInTimedOut;
	bRankedMatch = bInMatchmakingLobby && bInRankedMatch;
	RankedQueueRating = FMath::Clamp(InRankedQueueRating, 0, 3000);
	RankedSearchRange = bRankedMatch ? FMath::Max(0, InRankedSearchRange) : 0;
	ForceNetUpdate();
}

void AFlickGameState::BeginAuthoritativeMatch(const FString& InMatchId)
{
	MatchId = InMatchId;
	bMatchResultFinalized = false;
	FinalMatchOutcome = EFlickMatchOutcome::Continue;
	bMatchEndedByForfeit = false;
	MatchCompletedUnixTime = 0;
	ForceNetUpdate();
}

void AFlickGameState::FinalizeAuthoritativeMatch(
	const EFlickMatchOutcome Outcome,
	const bool bInForfeit)
{
	if (bMatchResultFinalized || Outcome == EFlickMatchOutcome::Continue)
	{
		return;
	}
	bMatchResultFinalized = true;
	FinalMatchOutcome = Outcome;
	bMatchEndedByForfeit = bInForfeit;
	MatchCompletedUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
	ForceNetUpdate();
}

void AFlickGameState::CompleteMatchByForfeit(const EFlickTeam ForfeitingTeam)
{
	if (bMatchResultFinalized || ForfeitingTeam == EFlickTeam::None)
	{
		return;
	}
	WinnerTeam = GetOpposingTeam(ForfeitingTeam);
	bDraw = false;
	bSeriesComplete = true;
	if (WinnerTeam == EFlickTeam::Player1)
	{
		Player1RoundsWon = RoundsToWin;
	}
	else
	{
		Player2RoundsWon = RoundsToWin;
	}
	MatchPhase = EFlickMatchPhase::RoundOver;
	FinalizeAuthoritativeMatch(
		WinnerTeam == EFlickTeam::Player1
			? EFlickMatchOutcome::Player1Wins
			: EFlickMatchOutcome::Player2Wins,
		true);
}
