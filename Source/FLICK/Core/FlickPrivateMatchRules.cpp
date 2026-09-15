#include "Core/FlickPrivateMatchRules.h"

#include "Core/FlickTeamRules.h"

void FlickPrivateMatchRules::Normalize(FFlickPrivateMatchSettings& Settings)
{
	Settings.Variant = NormalizeMatchVariant(Settings.Variant);
	Settings.PlayersPerTeam = Settings.Variant == EFlickMatchVariant::Bob
		? 1
		: FlickTeamRules::ClampPlayersPerTeam(Settings.PlayersPerTeam);
	Settings.RoundsToWin = FMath::Clamp(Settings.RoundsToWin, 1, 5);
	Settings.ArenaScale = FMath::Clamp(Settings.ArenaScale, 0.85f, 1.3f);
	Settings.FrictionScale = FMath::Clamp(Settings.FrictionScale, 0.5f, 2.0f);
	Settings.LaunchSpeedScale = FMath::Clamp(Settings.LaunchSpeedScale, 0.75f, 1.5f);
	Settings.RestitutionScale = FMath::Clamp(Settings.RestitutionScale, 0.5f, 1.5f);
}

void FlickPrivateMatchRules::CycleMode(FFlickPrivateMatchSettings& Settings, const int32 Direction)
{
	Normalize(Settings);
	if (Direction == 0)
	{
		return;
	}

	int32 ModeIndex = Settings.Variant == EFlickMatchVariant::Bob
		? 3
		: Settings.PlayersPerTeam - 1;
	ModeIndex = (ModeIndex + FMath::Sign(Direction) + 4) % 4;
	Settings.Variant = ModeIndex == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
	Settings.PlayersPerTeam = ModeIndex == 3 ? 1 : ModeIndex + 1;
}

int32 FlickPrivateMatchRules::GetRequiredPlayingSlots(const FFlickPrivateMatchSettings& Settings)
{
	FFlickPrivateMatchSettings Normalized = Settings;
	Normalize(Normalized);
	return Normalized.PlayersPerTeam * 2;
}
