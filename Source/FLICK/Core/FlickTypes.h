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
enum class EFlickMatchOutcome : uint8
{
	Continue UMETA(DisplayName = "Continue"),
	Player1Wins UMETA(DisplayName = "Player 1 Wins"),
	Player2Wins UMETA(DisplayName = "Player 2 Wins"),
	Draw UMETA(DisplayName = "Draw")
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
