#pragma once

#include "CoreMinimal.h"
#include "FlickTypes.generated.h"

UENUM(BlueprintType)
enum class EFlickTeam : uint8
{
	None UMETA(DisplayName = "None"),
	Player1 UMETA(DisplayName = "Player 1"),
	Player2 UMETA(DisplayName = "Player 2")
};

UENUM(BlueprintType)
enum class EFlickMatchPhase : uint8
{
	WaitingToStart UMETA(DisplayName = "Waiting To Start"),
	KickoffPlanning UMETA(DisplayName = "Kickoff Planning"),
	Aiming UMETA(DisplayName = "Aiming"),
	ResolvingPhysics UMETA(DisplayName = "Resolving Physics"),
	RoundOver UMETA(DisplayName = "Round Over")
};

UENUM(BlueprintType)
enum class EFlickBotDifficulty : uint8
{
	Easy UMETA(DisplayName = "Easy"),
	Normal UMETA(DisplayName = "Normal"),
	Hard UMETA(DisplayName = "Hard"),
	Expert UMETA(DisplayName = "Expert")
};

UENUM(BlueprintType)
enum class EFlickMatchOutcome : uint8
{
	Continue UMETA(DisplayName = "Continue"),
	Player1Wins UMETA(DisplayName = "Player 1 Wins"),
	Player2Wins UMETA(DisplayName = "Player 2 Wins"),
	Draw UMETA(DisplayName = "Draw")
};

UENUM(BlueprintType)
enum class EFlickDramaticEvent : uint8
{
	None UMETA(DisplayName = "None"),
	DoubleKnockout UMETA(DisplayName = "Double Knockout"),
	Trade UMETA(DisplayName = "Trade"),
	MultiKnockout UMETA(DisplayName = "Multi Knockout"),
	SelfKnockout UMETA(DisplayName = "Self Knockout"),
	LastPuckStanding UMETA(DisplayName = "Last Puck Standing"),
	ChainReaction UMETA(DisplayName = "Chain Reaction")
};

UENUM(BlueprintType)
enum class EFlickAccolade : uint8
{
	SwitchKnockout UMETA(DisplayName = "Switch Knockout"),
	DividerBank UMETA(DisplayName = "Divider Bank"),
	TrapShot UMETA(DisplayName = "Trap Shot"),
	DominoKnockout UMETA(DisplayName = "Domino Knockout"),
	TeamWipeout UMETA(DisplayName = "Team Wipeout"),
	PerfectTrade UMETA(DisplayName = "Perfect Trade"),
	LongRangeKnockout UMETA(DisplayName = "Long Range Knockout"),
	Pinball UMETA(DisplayName = "Pinball"),
	PrecisionKnockout UMETA(DisplayName = "Precision Knockout"),
	BuzzerBeater UMETA(DisplayName = "Buzzer Beater"),
	FlawlessRound UMETA(DisplayName = "Flawless Round"),
	Count UMETA(Hidden)
};

inline constexpr int32 FlickAccoladeCount = static_cast<int32>(EFlickAccolade::Count);

inline const TCHAR* GetFlickAccoladeName(const EFlickAccolade Accolade)
{
	switch (Accolade)
	{
	case EFlickAccolade::SwitchKnockout: return TEXT("SWITCH KNOCKOUT");
	case EFlickAccolade::DividerBank: return TEXT("DIVIDER BANK");
	case EFlickAccolade::TrapShot: return TEXT("TRAP SHOT");
	case EFlickAccolade::DominoKnockout: return TEXT("DOMINO KNOCKOUT");
	case EFlickAccolade::TeamWipeout: return TEXT("TEAM WIPEOUT");
	case EFlickAccolade::PerfectTrade: return TEXT("PERFECT TRADE");
	case EFlickAccolade::LongRangeKnockout: return TEXT("LONG RANGE KNOCKOUT");
	case EFlickAccolade::Pinball: return TEXT("PINBALL");
	case EFlickAccolade::PrecisionKnockout: return TEXT("PRECISION KNOCKOUT");
	case EFlickAccolade::BuzzerBeater: return TEXT("BUZZER BEATER");
	case EFlickAccolade::FlawlessRound: return TEXT("FLAWLESS ROUND");
	default: return TEXT("ACCOLADE");
	}
}

inline int32 GetFlickAccoladeBonusPoints(const EFlickAccolade Accolade)
{
	return Accolade == EFlickAccolade::TeamWipeout ? 100 : 0;
}

USTRUCT(BlueprintType)
struct FFlickAccoladeFeedEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EFlickAccolade Accolade = EFlickAccolade::SwitchKnockout;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EFlickTeam Team = EFlickTeam::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 BonusPoints = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Serial = 0;
};

struct FFlickDramaticEventResult
{
	EFlickDramaticEvent Event = EFlickDramaticEvent::None;
	EFlickTeam HighlightedTeam = EFlickTeam::None;
	int32 Value = 0;

	bool IsValid() const { return Event != EFlickDramaticEvent::None; }
};

UENUM(BlueprintType)
enum class EFlickRankTier : uint8
{
	Unranked UMETA(DisplayName = "Unranked"),
	Bronze UMETA(DisplayName = "Bronze"),
	Silver UMETA(DisplayName = "Silver"),
	Gold UMETA(DisplayName = "Gold"),
	Platinum UMETA(DisplayName = "Platinum"),
	Diamond UMETA(DisplayName = "Diamond"),
	Champion UMETA(DisplayName = "Champion"),
	GrandChampion UMETA(DisplayName = "Grand Champion")
};

UENUM(BlueprintType)
enum class EFlickFrontendScreen : uint8
{
	MainMenu UMETA(DisplayName = "Main Menu"),
	ModeSelect UMETA(DisplayName = "Mode Select"),
	OnlineBrowser UMETA(DisplayName = "Online Browser"),
	NetworkLobby UMETA(DisplayName = "Network Lobby"),
	Loadout UMETA(DisplayName = "Loadout"),
	Playing UMETA(DisplayName = "Playing"),
	Paused UMETA(DisplayName = "Paused"),
	Settings UMETA(DisplayName = "Settings"),
	Profile UMETA(DisplayName = "Profile"),
	ItemShop UMETA(DisplayName = "Item Shop"),
	ClassSelect UMETA(DisplayName = "Class Select"),
	PrivateMatch UMETA(DisplayName = "Private Match")
};

inline constexpr int32 FlickMaximumPartyMembers = 6;

UENUM(BlueprintType)
enum class EFlickPrivateMatchSetting : uint8
{
	TeamSize UMETA(DisplayName = "Team Size"),
	RoundsToWin UMETA(DisplayName = "Rounds To Win"),
	ArenaScale UMETA(DisplayName = "Arena Size"),
	FrictionScale UMETA(DisplayName = "Friction"),
	LaunchSpeedScale UMETA(DisplayName = "Launch Power"),
	RestitutionScale UMETA(DisplayName = "Bounce"),
	SimultaneousKickoff UMETA(DisplayName = "Simultaneous Kickoff")
};

USTRUCT(BlueprintType)
struct FFlickPrivateMatchSettings
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	int32 PlayersPerTeam = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	int32 RoundsToWin = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	float ArenaScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	float FrictionScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	float LaunchSpeedScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	float RestitutionScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Private Match")
	bool bSimultaneousKickoff = true;
};

inline int32 EncodePrivatePlayerSlot(const EFlickTeam Team, const int32 PlayerSlot)
{
	if (PlayerSlot < 0 || PlayerSlot >= 3)
	{
		return INDEX_NONE;
	}
	return Team == EFlickTeam::Player1
		? PlayerSlot
		: Team == EFlickTeam::Player2 ? 3 + PlayerSlot : INDEX_NONE;
}

UENUM(BlueprintType)
enum class EFlickMatchVariant : uint8
{
	Classic UMETA(DisplayName = "Knockout"),
	Bob UMETA(DisplayName = "BOB"),
	// Retained only so old settings and network data using value 2 can migrate safely.
	Blitz UMETA(Hidden)
};

inline EFlickMatchVariant NormalizeMatchVariant(const EFlickMatchVariant Variant)
{
	return Variant == EFlickMatchVariant::Bob
		? EFlickMatchVariant::Bob
		: EFlickMatchVariant::Classic;
}

UENUM(BlueprintType)
enum class EFlickPieceArchetype : uint8
{
	Standard UMETA(DisplayName = "Standard"),
	Heavy UMETA(DisplayName = "Heavy"),
	Striker UMETA(DisplayName = "Striker"),
	Grippy UMETA(DisplayName = "Grippy"),
	Slider UMETA(DisplayName = "Slider"),
	Blocker UMETA(DisplayName = "Blocker"),
	Compact UMETA(DisplayName = "Compact"),
	Bouncer UMETA(DisplayName = "Bouncer"),
	Toppler UMETA(DisplayName = "Toppler")
};

UENUM(BlueprintType)
enum class EFlickLineupPreset : uint8
{
	Balanced UMETA(DisplayName = "Balanced"),
	Power UMETA(DisplayName = "Power"),
	Speed UMETA(DisplayName = "Speed"),
	Control UMETA(DisplayName = "Control"),
	Custom UMETA(DisplayName = "Custom")
};

inline EFlickTeam GetOpposingTeam(const EFlickTeam Team)
{
	switch (Team)
	{
	case EFlickTeam::Player1:
		return EFlickTeam::Player2;
	case EFlickTeam::Player2:
		return EFlickTeam::Player1;
	default:
		return EFlickTeam::None;
	}
}

inline int32 GetTeamNumber(const EFlickTeam Team)
{
	switch (Team)
	{
	case EFlickTeam::Player1:
		return 1;
	case EFlickTeam::Player2:
		return 2;
	default:
		return 0;
	}
}

inline FString GetTeamDisplayName(const EFlickTeam Team)
{
	switch (Team)
	{
	case EFlickTeam::Player1:
		return TEXT("Player 1");
	case EFlickTeam::Player2:
		return TEXT("Player 2");
	default:
		return TEXT("None");
	}
}

inline FString GetBotDifficultyName(const EFlickBotDifficulty Difficulty)
{
	switch (Difficulty)
	{
	case EFlickBotDifficulty::Easy:
		return TEXT("EASY");
	case EFlickBotDifficulty::Hard:
		return TEXT("HARD");
	case EFlickBotDifficulty::Expert:
		return TEXT("EXPERT");
	case EFlickBotDifficulty::Normal:
	default:
		return TEXT("NORMAL");
	}
}

inline FLinearColor GetTeamColor(const EFlickTeam Team)
{
	switch (Team)
	{
	case EFlickTeam::Player1:
		return FLinearColor(0.0f, 0.62f, 0.95f, 1.0f);
	case EFlickTeam::Player2:
		return FLinearColor(1.0f, 0.22f, 0.07f, 1.0f);
	default:
		return FLinearColor(0.55f, 0.58f, 0.62f, 1.0f);
	}
}

inline FString GetMatchVariantName(const EFlickMatchVariant Variant)
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob:
		return TEXT("BOB");
	case EFlickMatchVariant::Classic:
	default:
		return TEXT("KNOCKOUT");
	}
}

inline FString GetMatchVariantSummary(const EFlickMatchVariant Variant)
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob:
		return TEXT("POCKET YOUR COLOR  |  STANDARD PUCKS ONLY");
	case EFlickMatchVariant::Classic:
	default:
		return TEXT("4 PUCKS PER PLAYER  |  BEST OF 5");
	}
}

inline FString GetMatchVariantFormatLabel(const EFlickMatchVariant Variant)
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob:
		return TEXT("12 STANDARD PUCKS EACH");
	case EFlickMatchVariant::Classic:
	default:
		return TEXT("4 PUCKS EACH");
	}
}

inline FString GetMatchVariantSeriesLabel(const EFlickMatchVariant Variant)
{
	return Variant == EFlickMatchVariant::Bob
		? TEXT("ONE BOARD  |  CLEAR YOUR COLOR")
		: TEXT("BEST OF 5  |  FIRST TO 3 ROUNDS");
}

inline FString GetPieceArchetypeName(const EFlickPieceArchetype Archetype)
{
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy:
		return TEXT("HEAVY");
	case EFlickPieceArchetype::Striker:
		return TEXT("STRIKER");
	case EFlickPieceArchetype::Grippy:
		return TEXT("GRIPPY");
	case EFlickPieceArchetype::Slider:
		return TEXT("SLIDER");
	case EFlickPieceArchetype::Blocker:
		return TEXT("BLOCKER");
	case EFlickPieceArchetype::Compact:
		return TEXT("COMPACT");
	case EFlickPieceArchetype::Bouncer:
		return TEXT("BOUNCER");
	case EFlickPieceArchetype::Toppler:
		return TEXT("TOPPLER");
	case EFlickPieceArchetype::Standard:
	default:
		return TEXT("STANDARD");
	}
}

inline FString GetPieceArchetypeMark(const EFlickPieceArchetype Archetype)
{
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy:
		return TEXT("H");
	case EFlickPieceArchetype::Striker:
		return TEXT("X");
	case EFlickPieceArchetype::Grippy:
		return TEXT("G");
	case EFlickPieceArchetype::Slider:
		return TEXT("L");
	case EFlickPieceArchetype::Blocker:
		return TEXT("B");
	case EFlickPieceArchetype::Compact:
		return TEXT("C");
	case EFlickPieceArchetype::Bouncer:
		return TEXT("R");
	case EFlickPieceArchetype::Toppler:
		return TEXT("T");
	case EFlickPieceArchetype::Standard:
	default:
		return TEXT("S");
	}
}

inline FString GetLineupPresetName(const EFlickLineupPreset Preset)
{
	switch (Preset)
	{
	case EFlickLineupPreset::Power:
		return TEXT("CLASS 2");
	case EFlickLineupPreset::Speed:
		return TEXT("CLASS 3");
	case EFlickLineupPreset::Control:
		return TEXT("CLASS 4");
	case EFlickLineupPreset::Custom:
		return TEXT("CUSTOM");
	case EFlickLineupPreset::Balanced:
	default:
		return TEXT("CLASS 1");
	}
}

inline FString GetLineupPresetRole(const EFlickLineupPreset Preset)
{
	switch (Preset)
	{
	case EFlickLineupPreset::Power:
		return TEXT("POWER");
	case EFlickLineupPreset::Speed:
		return TEXT("SPEED");
	case EFlickLineupPreset::Control:
		return TEXT("CONTROL");
	case EFlickLineupPreset::Custom:
		return TEXT("CUSTOM");
	case EFlickLineupPreset::Balanced:
	default:
		return TEXT("BALANCED");
	}
}

inline FString GetPhaseDisplayName(const EFlickMatchPhase Phase)
{
	switch (Phase)
	{
	case EFlickMatchPhase::WaitingToStart:
		return TEXT("Waiting To Start");
	case EFlickMatchPhase::KickoffPlanning:
		return TEXT("Kickoff Planning");
	case EFlickMatchPhase::Aiming:
		return TEXT("Aiming");
	case EFlickMatchPhase::ResolvingPhysics:
		return TEXT("Resolving");
	case EFlickMatchPhase::RoundOver:
		return TEXT("Round Over");
	default:
		return TEXT("Unknown");
	}
}

inline EFlickMatchOutcome EvaluateMatchOutcome(const int32 Player1Pieces, const int32 Player2Pieces)
{
	if (Player1Pieces <= 0 && Player2Pieces <= 0)
	{
		return EFlickMatchOutcome::Draw;
	}

	if (Player1Pieces <= 0)
	{
		return EFlickMatchOutcome::Player2Wins;
	}

	if (Player2Pieces <= 0)
	{
		return EFlickMatchOutcome::Player1Wins;
	}

	return EFlickMatchOutcome::Continue;
}

inline FFlickDramaticEventResult EvaluateDramaticEvent(
	const int32 Player1Eliminated,
	const int32 Player2Eliminated,
	const int32 Player1Remaining,
	const int32 Player2Remaining,
	const int32 ImpactCount,
	const int32 ChainImpactThreshold,
	const EFlickTeam ShootingTeam,
	const bool bSimultaneousShot)
{
	const int32 SafePlayer1Eliminated = FMath::Max(0, Player1Eliminated);
	const int32 SafePlayer2Eliminated = FMath::Max(0, Player2Eliminated);
	const int32 TotalEliminated = SafePlayer1Eliminated + SafePlayer2Eliminated;
	if (TotalEliminated >= 3)
	{
		const EFlickTeam HighlightedTeam = SafePlayer1Eliminated > SafePlayer2Eliminated
			? EFlickTeam::Player2
			: SafePlayer2Eliminated > SafePlayer1Eliminated
				? EFlickTeam::Player1
				: EFlickTeam::None;
		return {EFlickDramaticEvent::MultiKnockout, HighlightedTeam, TotalEliminated};
	}
	if (TotalEliminated == 2 && SafePlayer1Eliminated > 0 && SafePlayer2Eliminated > 0)
	{
		return {EFlickDramaticEvent::Trade, EFlickTeam::None, TotalEliminated};
	}
	if (TotalEliminated == 2)
	{
		return {
			EFlickDramaticEvent::DoubleKnockout,
			SafePlayer1Eliminated == 2 ? EFlickTeam::Player2 : EFlickTeam::Player1,
			TotalEliminated};
	}

	const EFlickMatchOutcome Outcome = EvaluateMatchOutcome(Player1Remaining, Player2Remaining);
	if (TotalEliminated > 0 && Outcome == EFlickMatchOutcome::Player1Wins && Player1Remaining == 1)
	{
		return {EFlickDramaticEvent::LastPuckStanding, EFlickTeam::Player1, 1};
	}
	if (TotalEliminated > 0 && Outcome == EFlickMatchOutcome::Player2Wins && Player2Remaining == 1)
	{
		return {EFlickDramaticEvent::LastPuckStanding, EFlickTeam::Player2, 1};
	}

	if (!bSimultaneousShot && TotalEliminated > 0 && ShootingTeam != EFlickTeam::None)
	{
		const int32 OwnEliminations = ShootingTeam == EFlickTeam::Player1
			? SafePlayer1Eliminated : SafePlayer2Eliminated;
		const int32 OpponentEliminations = ShootingTeam == EFlickTeam::Player1
			? SafePlayer2Eliminated : SafePlayer1Eliminated;
		if (OwnEliminations > 0 && OpponentEliminations == 0)
		{
			return {EFlickDramaticEvent::SelfKnockout, ShootingTeam, OwnEliminations};
		}
	}

	if (ImpactCount >= FMath::Max(2, ChainImpactThreshold))
	{
		return {EFlickDramaticEvent::ChainReaction, ShootingTeam, FMath::Max(0, ImpactCount)};
	}
	return {};
}
