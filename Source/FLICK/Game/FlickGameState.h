#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/GameStateBase.h"
#include "FlickGameState.generated.h"

USTRUCT(BlueprintType)
struct FFlickPlayerMatchStats
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	EFlickTeam Team = EFlickTeam::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	int32 PlayerSlot = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	int32 Score = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	int32 Knockouts = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	int32 Shots = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	int32 Impacts = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	int32 SurvivingPucks = 0;
};

UCLASS()
class FLICK_API AFlickGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFlickGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ResetMatchState();
	void ResetSeriesState(int32 InRoundsToWin = 3);
	void ResetRoundState();
	void CompleteRound(EFlickMatchOutcome Outcome);
	bool AdvanceToNextRound();
	void SetMatchPhase(EFlickMatchPhase InPhase);
	void SetCurrentTeam(EFlickTeam InTeam);
	void SetShotClockState(bool bInActive, float InDuration = 10.0f);
	float GetShotClockTimeRemaining() const;
	float GetShotClockFraction(EFlickTeam Team) const;
	void SetRoundAdvanceTimerState(bool bInActive, float InDuration = 10.0f);
	float GetRoundAdvanceTimeRemaining() const;
	void SetNetworkClassSelectionState(bool bInActive, float InDuration = 10.0f);
	float GetNetworkClassSelectionTimeRemaining() const;
	void SetTeamFormat(int32 InPlayersPerTeam);
	void SetCurrentTeamPlayerSlot(int32 InPlayerSlot);
	void SetActivePieceCounts(int32 InPlayer1Count, int32 InPlayer2Count);
	void SetStartingPiecesPerTeam(int32 InStartingPieces);
	void BeginShot(EFlickTeam ShootingTeam, int32 PieceId, float NormalizedPower);
	void RecordImpact(float ImpactStrength);
	void RecordElimination(EFlickTeam EliminatedTeam);
	void InitializePlayerMatchStats(int32 InPlayersPerTeam);
	void RecordPlayerShot(EFlickTeam Team, int32 PlayerSlot);
	void RecordPlayerImpact(EFlickTeam Team, int32 PlayerSlot);
	void RecordPlayerKnockout(EFlickTeam Team, int32 PlayerSlot);
	void RecordPlayerSurvivingPucks(EFlickTeam Team, int32 PlayerSlot, int32 SurvivingPuckCount);
	const FFlickPlayerMatchStats* FindPlayerMatchStats(EFlickTeam Team, int32 PlayerSlot) const;
	int32 GetTeamScore(EFlickTeam Team) const;
	int32 GetLastShootingPlayerSlot(EFlickTeam Team) const;
	void AdvanceTurn();
	void SetMatchConfiguration(
		EFlickMatchVariant InVariant,
		float InArenaSurfaceZ,
		float InMaxDragDistance,
		float InMinDragDistance,
		float InPowerExponent,
		float InMaxLaunchSpeed);
	void SetNetworkLobbyState(bool bInLobbyActive, EFlickMatchVariant InSelectedVariant);
	void SetPartyState(bool bInPartyActive, const FString& InLeaderUserId, int32 InMaximumMembers = FlickMaximumPartyMembers);
	void SetPrivateMatchLobbyState(bool bInActive, const FFlickPrivateMatchSettings& InSettings);
	void SetMatchmakingState(
		bool bInMatchmakingLobby,
		bool bInTimedOut = false,
		bool bInRankedMatch = false,
		int32 InRankedQueueRating = 1000,
		int32 InRankedSearchRange = 0);
	void BeginAuthoritativeMatch(const FString& InMatchId);
	void FinalizeAuthoritativeMatch(EFlickMatchOutcome Outcome, bool bInForfeit = false);
	void CompleteMatchByForfeit(EFlickTeam ForfeitingTeam);

	bool IsGameplayActive() const { return MatchPhase != EFlickMatchPhase::WaitingToStart; }

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	EFlickMatchPhase MatchPhase = EFlickMatchPhase::WaitingToStart;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	EFlickTeam CurrentTeam = EFlickTeam::Player1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 PlayersPerTeam = 1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 CurrentTeamPlayerSlot = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	EFlickTeam WinnerTeam = EFlickTeam::None;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	bool bDraw = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Series")
	bool bSeriesComplete = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Series")
	int32 RoundNumber = 1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Series")
	int32 RoundsToWin = 3;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Series")
	int32 Player1RoundsWon = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Series")
	int32 Player2RoundsWon = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Series")
	EFlickTeam RoundStartingTeam = EFlickTeam::Player1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 Player1ActivePieces = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 Player2ActivePieces = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 StartingPiecesPerTeam = 4;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 TurnNumber = 1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers")
	bool bShotClockActive = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers")
	float ShotClockDuration = 10.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers")
	float ShotClockEndServerTime = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers")
	bool bRoundAdvanceTimerActive = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers")
	float RoundAdvanceTimerDuration = 10.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Timers")
	float RoundAdvanceTimerEndServerTime = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Class Selection")
	bool bNetworkClassSelectionActive = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Class Selection")
	float NetworkClassSelectionDuration = 10.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match|Class Selection")
	float NetworkClassSelectionEndServerTime = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 Player1ShotsTaken = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	int32 Player2ShotsTaken = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	EFlickTeam LastShotTeam = EFlickTeam::None;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	int32 LastShotPieceId = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	float LastShotPower = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	int32 ImpactsThisShot = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	float StrongestImpactThisShot = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	int32 Player1EliminatedThisShot = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Last Shot")
	int32 Player2EliminatedThisShot = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Scoreboard")
	TArray<FFlickPlayerMatchStats> PlayerMatchStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 ScorePerShot = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 ScorePerImpact = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 ScorePerKnockout = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FLICK|Scoreboard", meta = (ClampMin = "0"))
	int32 ScorePerSurvivingPuck = 50;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Match")
	EFlickMatchVariant ActiveMatchVariant = EFlickMatchVariant::Classic;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Lobby")
	bool bNetworkLobbyActive = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Lobby")
	EFlickMatchVariant LobbySelectedVariant = EFlickMatchVariant::Classic;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Matchmaking")
	bool bMatchmakingLobby = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Matchmaking")
	bool bMatchmakingTimedOut = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Matchmaking")
	bool bRankedMatch = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Matchmaking")
	int32 RankedQueueRating = 1000;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Matchmaking")
	int32 RankedSearchRange = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Result")
	FString MatchId;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Result")
	bool bMatchResultFinalized = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Result")
	EFlickMatchOutcome FinalMatchOutcome = EFlickMatchOutcome::Continue;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Result")
	bool bMatchEndedByForfeit = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Result")
	int64 MatchCompletedUnixTime = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Party")
	bool bPartyActive = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Party")
	FString PartyLeaderUserId;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Party")
	int32 PartyMaximumMembers = FlickMaximumPartyMembers;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	bool bPrivateMatchLobbyActive = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	FFlickPrivateMatchSettings PrivateMatchSettings;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Tuning")
	float ArenaSurfaceZ = 250.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Tuning")
	float MaxDragDistance = 280.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Tuning")
	float MinDragDistance = 15.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Tuning")
	float PowerExponent = 1.2f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Tuning")
	float MaxLaunchSpeed = 1450.0f;

private:
	FFlickPlayerMatchStats* FindMutablePlayerMatchStats(EFlickTeam Team, int32 PlayerSlot);
	int32 Player1LastShootingPlayerSlot = 0;
	int32 Player2LastShootingPlayerSlot = 0;
};
