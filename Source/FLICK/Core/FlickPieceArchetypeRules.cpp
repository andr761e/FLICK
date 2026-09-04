#include "Core/FlickPieceArchetypeRules.h"

namespace
{
	const FFlickPieceArchetypeRules StandardRules;

	const FFlickPieceArchetypeRules HeavyRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.MassMultiplier = 1.7f;
		Rules.FrictionMultiplier = 1.3f;
		Rules.RestitutionMultiplier = 0.72f;
		Rules.LinearDampingMultiplier = 1.05f;
		Rules.AngularDampingMultiplier = 1.25f;
		Rules.LaunchSpeedMultiplier = 0.82f;
		Rules.RadiusMultiplier = 1.08f;
		Rules.ThicknessMultiplier = 1.18f;
		Rules.AccentColor = FLinearColor(1.0f, 0.72f, 0.14f, 1.0f);
		Rules.ClassLabel = TEXT("POWER");
		Rules.Summary = TEXT("MASSIVE | STABLE | SLOW");
		Rules.Strengths = TEXT("COLLISION FORCE | STABILITY");
		Rules.Weaknesses = TEXT("SPEED | MOBILITY");
		return Rules;
	}();

	const FFlickPieceArchetypeRules StrikerRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.MassMultiplier = 0.7f;
		Rules.FrictionMultiplier = 0.7f;
		Rules.RestitutionMultiplier = 1.25f;
		Rules.LinearDampingMultiplier = 0.78f;
		Rules.AngularDampingMultiplier = 0.78f;
		Rules.LaunchSpeedMultiplier = 1.18f;
		Rules.RadiusMultiplier = 0.92f;
		Rules.ThicknessMultiplier = 0.82f;
		Rules.AccentColor = FLinearColor(1.0f, 0.18f, 0.48f, 1.0f);
		Rules.ClassLabel = TEXT("SPEED");
		Rules.Summary = TEXT("FAST | LIGHT | LIVELY");
		Rules.Strengths = TEXT("SPEED | REBOUND");
		Rules.Weaknesses = TEXT("WEIGHT | DEFENSE");
		return Rules;
	}();

	const FFlickPieceArchetypeRules GrippyRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.FrictionMultiplier = 3.8f;
		Rules.RestitutionMultiplier = 0.58f;
		Rules.LinearDampingMultiplier = 1.45f;
		Rules.AngularDampingMultiplier = 1.4f;
		Rules.LaunchSpeedMultiplier = 0.9f;
		Rules.AccentColor = FLinearColor(0.12f, 0.92f, 0.48f, 1.0f);
		Rules.ClassLabel = TEXT("CONTROL");
		Rules.Summary = TEXT("CONTROL | GRIP | PRECISION");
		Rules.Strengths = TEXT("CONTROL | STOPPING");
		Rules.Weaknesses = TEXT("COAST | SPEED");
		return Rules;
	}();

	const FFlickPieceArchetypeRules SliderRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.FrictionMultiplier = 0.45f;
		Rules.RestitutionMultiplier = 0.85f;
		Rules.LinearDampingMultiplier = 0.58f;
		Rules.AngularDampingMultiplier = 0.8f;
		Rules.AccentColor = FLinearColor(0.12f, 0.9f, 0.92f, 1.0f);
		Rules.ClassLabel = TEXT("MOMENTUM");
		Rules.Summary = TEXT("LONG COAST | EDGE RISK");
		Rules.Strengths = TEXT("COAST | POSITIONING");
		Rules.Weaknesses = TEXT("STOPPING | EDGE CONTROL");
		return Rules;
	}();

	const FFlickPieceArchetypeRules BlockerRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.MassMultiplier = 0.75f;
		Rules.RestitutionMultiplier = 0.78f;
		Rules.LinearDampingMultiplier = 1.05f;
		Rules.LaunchSpeedMultiplier = 0.84f;
		Rules.RadiusMultiplier = 1.18f;
		Rules.ThicknessMultiplier = 0.92f;
		Rules.AccentColor = FLinearColor(0.48f, 0.38f, 1.0f, 1.0f);
		Rules.ClassLabel = TEXT("DEFENSE");
		Rules.Summary = TEXT("WIDE | LIGHT | DEFENSIVE");
		Rules.Strengths = TEXT("BOARD COVERAGE | DEFENSE");
		Rules.Weaknesses = TEXT("WEIGHT | SPEED");
		return Rules;
	}();

	const FFlickPieceArchetypeRules CompactRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.FrictionMultiplier = 0.88f;
		Rules.LinearDampingMultiplier = 0.92f;
		Rules.AngularDampingMultiplier = 0.9f;
		Rules.LaunchSpeedMultiplier = 1.05f;
		Rules.RadiusMultiplier = 0.8f;
		Rules.ThicknessMultiplier = 0.9f;
		Rules.AccentColor = FLinearColor(0.84f, 1.0f, 0.16f, 1.0f);
		Rules.ClassLabel = TEXT("PRECISION");
		Rules.Summary = TEXT("SMALL | DENSE | ACCURATE");
		Rules.Strengths = TEXT("PRECISION | GAP PLAY");
		Rules.Weaknesses = TEXT("COVERAGE | IMPACT");
		return Rules;
	}();

	const FFlickPieceArchetypeRules BouncerRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.MassMultiplier = 0.85f;
		Rules.FrictionMultiplier = 0.82f;
		Rules.RestitutionMultiplier = 1.45f;
		Rules.LinearDampingMultiplier = 0.78f;
		Rules.AngularDampingMultiplier = 0.82f;
		Rules.LaunchSpeedMultiplier = 1.02f;
		Rules.RadiusMultiplier = 0.96f;
		Rules.ThicknessMultiplier = 0.9f;
		Rules.AccentColor = FLinearColor(1.0f, 0.43f, 0.07f, 1.0f);
		Rules.ClassLabel = TEXT("TRICKSHOT");
		Rules.Summary = TEXT("REBOUND | BANK | CHAOS");
		Rules.Strengths = TEXT("BANK SHOTS | REBOUNDS");
		Rules.Weaknesses = TEXT("CONTROL | STABILITY");
		return Rules;
	}();

	const FFlickPieceArchetypeRules TopplerRules = []
	{
		FFlickPieceArchetypeRules Rules;
		Rules.MassMultiplier = 0.9f;
		Rules.FrictionMultiplier = 1.05f;
		Rules.RestitutionMultiplier = 0.85f;
		Rules.LinearDampingMultiplier = 0.9f;
		Rules.AngularDampingMultiplier = 0.62f;
		Rules.LaunchSpeedMultiplier = 0.95f;
		Rules.ThicknessMultiplier = 1.35f;
		Rules.CenterOfMassHeightFraction = 0.14f;
		Rules.AccentColor = FLinearColor(1.0f, 0.1f, 0.16f, 1.0f);
		Rules.ClassLabel = TEXT("RISK");
		Rules.Summary = TEXT("TALL | UNSTABLE | FORCE");
		Rules.Strengths = TEXT("DISRUPTION | HEIGHT");
		Rules.Weaknesses = TEXT("STABILITY | RECOVERY");
		return Rules;
	}();

	const TArray<EFlickPieceArchetype> BalancedPreset = {
		EFlickPieceArchetype::Standard,
		EFlickPieceArchetype::Heavy,
		EFlickPieceArchetype::Striker,
		EFlickPieceArchetype::Grippy};
	const TArray<EFlickPieceArchetype> PowerPreset = {
		EFlickPieceArchetype::Heavy,
		EFlickPieceArchetype::Blocker,
		EFlickPieceArchetype::Slider,
		EFlickPieceArchetype::Standard};
	const TArray<EFlickPieceArchetype> SpeedPreset = {
		EFlickPieceArchetype::Striker,
		EFlickPieceArchetype::Slider,
		EFlickPieceArchetype::Compact,
		EFlickPieceArchetype::Bouncer};
	const TArray<EFlickPieceArchetype> ControlPreset = {
		EFlickPieceArchetype::Grippy,
		EFlickPieceArchetype::Blocker,
		EFlickPieceArchetype::Compact,
		EFlickPieceArchetype::Toppler};
}

const FFlickPieceArchetypeRules& FlickPieceArchetypeRules::Get(const EFlickPieceArchetype Archetype)
{
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy:
		return HeavyRules;
	case EFlickPieceArchetype::Striker:
		return StrikerRules;
	case EFlickPieceArchetype::Grippy:
		return GrippyRules;
	case EFlickPieceArchetype::Slider:
		return SliderRules;
	case EFlickPieceArchetype::Blocker:
		return BlockerRules;
	case EFlickPieceArchetype::Compact:
		return CompactRules;
	case EFlickPieceArchetype::Bouncer:
		return BouncerRules;
	case EFlickPieceArchetype::Toppler:
		return TopplerRules;
	case EFlickPieceArchetype::Standard:
	default:
		return StandardRules;
	}
}

FLinearColor FlickPieceArchetypeRules::GetVisualAccent(
	const EFlickPieceArchetype Archetype,
	const FLinearColor& TeamColor)
{
	switch (Archetype)
	{
	case EFlickPieceArchetype::Toppler: return FLinearColor(0.72f, 0.32f, 1.0f, 1.0f);
	case EFlickPieceArchetype::Bouncer: return FLinearColor(1.0f, 0.66f, 0.08f, 1.0f);
	case EFlickPieceArchetype::Compact: return FLinearColor(0.16f, 0.92f, 1.0f, 1.0f);
	case EFlickPieceArchetype::Blocker: return FLinearColor(0.74f, 0.91f, 1.0f, 1.0f);
	case EFlickPieceArchetype::Slider: return FLinearColor(0.04f, 0.9f, 0.94f, 1.0f);
	case EFlickPieceArchetype::Grippy: return FLinearColor(0.16f, 1.0f, 0.7f, 1.0f);
	case EFlickPieceArchetype::Striker: return FLinearColor(0.74f, 0.28f, 1.0f, 1.0f);
	case EFlickPieceArchetype::Heavy: return FLinearColor(1.0f, 0.82f, 0.56f, 1.0f);
	case EFlickPieceArchetype::Standard:
	default: return FMath::Lerp(TeamColor, FLinearColor::White, 0.16f);
	}
}

FFlickPieceDisplayStats FlickPieceArchetypeRules::GetDisplayStats(const EFlickPieceArchetype Archetype)
{
	const FFlickPieceArchetypeRules& Rules = Get(Archetype);
	const auto Normalize = [](const float Value, const float Minimum, const float Maximum)
	{
		return FMath::Clamp((Value - Minimum) / FMath::Max(Maximum - Minimum, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	};
	const auto Present = [](const float Value)
	{
		return 0.08f + FMath::Clamp(Value, 0.0f, 1.0f) * 0.9f;
	};

	FFlickPieceDisplayStats Stats;
	Stats.Speed = Present(Normalize(Rules.LaunchSpeedMultiplier, 0.78f, 1.2f));
	Stats.Weight = Present(Normalize(Rules.MassMultiplier, 0.65f, 1.75f));
	Stats.Impact = Present(Normalize(
		Rules.MassMultiplier * Rules.LaunchSpeedMultiplier,
		0.6f,
		1.45f));

	const float FrictionControl = Normalize(Rules.FrictionMultiplier, 0.45f, 3.8f);
	const float DampingControl = Normalize(Rules.LinearDampingMultiplier, 0.58f, 1.45f);
	const float ReboundControl = 1.0f - Normalize(Rules.RestitutionMultiplier, 0.58f, 1.45f);
	Stats.Control = Present(
		FrictionControl * 0.4f
		+ DampingControl * 0.35f
		+ ReboundControl * 0.25f);

	const float CoastResistance = FMath::Sqrt(
		Rules.FrictionMultiplier * Rules.LinearDampingMultiplier);
	Stats.Coast = Present(1.0f - Normalize(CoastResistance, 0.5f, 2.35f));

	const float FootprintStability = Normalize(
		Rules.RadiusMultiplier / FMath::Max(Rules.ThicknessMultiplier, KINDA_SMALL_NUMBER),
		0.7f,
		1.3f);
	const float AngularStability = Normalize(Rules.AngularDampingMultiplier, 0.6f, 1.45f);
	const float MassStability = Normalize(Rules.MassMultiplier, 0.65f, 1.75f);
	const float CenterOfMassPenalty = Normalize(Rules.CenterOfMassHeightFraction, 0.0f, 0.15f);
	const float ReboundPenalty = Normalize(Rules.RestitutionMultiplier, 0.58f, 1.45f);
	Stats.Stability = Present(
		FootprintStability * 0.35f
		+ AngularStability * 0.25f
		+ MassStability * 0.25f
		+ (1.0f - CenterOfMassPenalty) * 0.15f
		- ReboundPenalty * 0.12f);
	return Stats;
}

const TArray<EFlickPieceArchetype>& FlickPieceArchetypeRules::GetPreset(const EFlickLineupPreset Preset)
{
	switch (Preset)
	{
	case EFlickLineupPreset::Power:
		return PowerPreset;
	case EFlickLineupPreset::Speed:
		return SpeedPreset;
	case EFlickLineupPreset::Control:
		return ControlPreset;
	case EFlickLineupPreset::Balanced:
	case EFlickLineupPreset::Custom:
	default:
		return BalancedPreset;
	}
}

EFlickPieceArchetype FlickPieceArchetypeRules::Cycle(
	const EFlickPieceArchetype Archetype,
	const int32 Direction)
{
	const int32 Current = FMath::Clamp(static_cast<int32>(Archetype), 0, ArchetypeCount - 1);
	const int32 Step = Direction < 0 ? -1 : 1;
	return static_cast<EFlickPieceArchetype>((Current + Step + ArchetypeCount) % ArchetypeCount);
}
