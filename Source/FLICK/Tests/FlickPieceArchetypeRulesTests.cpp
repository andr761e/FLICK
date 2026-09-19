#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickModeRules.h"
#include "Misc/AutomationTest.h"

namespace
{
	float CalculateHeadOnTargetSpeed(
		const FFlickPieceArchetypeRules& Attacker,
		const FFlickPieceArchetypeRules& Defender)
	{
		const FFlickModeRules& Mode = FlickModeRules::Get(EFlickMatchVariant::Classic);
		// The common base mass cancels from the collision ratio, so archetype
		// multipliers are sufficient here.
		const float AttackerMass = Attacker.MassMultiplier;
		const float DefenderMass = Defender.MassMultiplier;
		const float AttackerSpeed = Mode.MaxLaunchSpeed * Attacker.LaunchSpeedMultiplier;
		// Runtime physical materials combine restitution by averaging the two
		// puck values. This is the corresponding one-dimensional collision result.
		const float CombinedRestitution = Mode.PieceRestitution
			* (Attacker.RestitutionMultiplier + Defender.RestitutionMultiplier) * 0.5f;
		return (1.0f + CombinedRestitution)
			* AttackerMass / FMath::Max(AttackerMass + DefenderMass, KINDA_SMALL_NUMBER)
			* AttackerSpeed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPieceArchetypeRulesTest,
	"FLICK.Pieces.ArchetypeRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPieceArchetypeRulesTest::RunTest(const FString& Parameters)
{
	const FFlickPieceArchetypeRules& Standard = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Standard);
	const FFlickPieceArchetypeRules& Heavy = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Heavy);
	const FFlickPieceArchetypeRules& Striker = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Striker);
	const FFlickPieceArchetypeRules& Grippy = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Grippy);
	const FFlickPieceArchetypeRules& Slider = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Slider);
	const FFlickPieceArchetypeRules& Blocker = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Blocker);
	const FFlickPieceArchetypeRules& Compact = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Compact);
	const FFlickPieceArchetypeRules& Bouncer = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Bouncer);
	const FFlickPieceArchetypeRules& Toppler = FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Toppler);

	TestTrue(TEXT("Heavy has the greatest mass"), Heavy.MassMultiplier > Standard.MassMultiplier);
	TestTrue(TEXT("Heavy trades launch speed for momentum"), Heavy.LaunchSpeedMultiplier < Standard.LaunchSpeedMultiplier);
	TestTrue(TEXT("Striker is the lightest role"), Striker.MassMultiplier < Standard.MassMultiplier);
	TestTrue(TEXT("Striker has the fastest launch"), Striker.LaunchSpeedMultiplier > Standard.LaunchSpeedMultiplier);
	TestTrue(TEXT("Grippy has the greatest friction"), Grippy.FrictionMultiplier > Heavy.FrictionMultiplier);
	TestTrue(TEXT("Grippy settles faster than Standard"), Grippy.LinearDampingMultiplier > Standard.LinearDampingMultiplier);
	TestTrue(TEXT("Slider has the longest coast"), Slider.FrictionMultiplier < Striker.FrictionMultiplier && Slider.LinearDampingMultiplier < Striker.LinearDampingMultiplier);
	TestTrue(TEXT("Blocker owns the widest footprint"), Blocker.RadiusMultiplier > Heavy.RadiusMultiplier);
	TestTrue(TEXT("Blocker pays for coverage with low mass"), Blocker.MassMultiplier < Standard.MassMultiplier);
	TestTrue(TEXT("Compact is the smallest precision target"), Compact.RadiusMultiplier < Striker.RadiusMultiplier);
	TestTrue(TEXT("Bouncer has the highest restitution"), Bouncer.RestitutionMultiplier > Striker.RestitutionMultiplier);
	TestTrue(TEXT("Toppler is the tallest archetype"), Toppler.ThicknessMultiplier > Heavy.ThicknessMultiplier);
	TestTrue(TEXT("Toppler raises its center of mass"), Toppler.CenterOfMassHeightFraction > 0.0f);
	TestEqual(
		TEXT("Cycling forward wraps from Toppler to Standard"),
		FlickPieceArchetypeRules::Cycle(EFlickPieceArchetype::Toppler, 1),
		EFlickPieceArchetype::Standard);
	TestEqual(
		TEXT("Cycling backward wraps from Standard to Toppler"),
		FlickPieceArchetypeRules::Cycle(EFlickPieceArchetype::Standard, -1),
		EFlickPieceArchetype::Toppler);
	TestEqual(TEXT("Power preset has four slots"), FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Power).Num(), 4);
	TestEqual(TEXT("Control preset showcases Toppler"), FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Control)[3], EFlickPieceArchetype::Toppler);

	for (int32 Index = 0; Index < FlickPieceArchetypeRules::ArchetypeCount; ++Index)
	{
		const FFlickPieceDisplayStats Stats = FlickPieceArchetypeRules::GetDisplayStats(
			static_cast<EFlickPieceArchetype>(Index));
		TestTrue(TEXT("Display stats remain normalized"),
			Stats.Speed >= 0.0f && Stats.Speed <= 1.0f
			&& Stats.Weight >= 0.0f && Stats.Weight <= 1.0f
			&& Stats.Impact >= 0.0f && Stats.Impact <= 1.0f
			&& Stats.Control >= 0.0f && Stats.Control <= 1.0f
			&& Stats.Coast >= 0.0f && Stats.Coast <= 1.0f
			&& Stats.Stability >= 0.0f && Stats.Stability <= 1.0f);
	}
	TestTrue(TEXT("Heavy comparison shows more weight"),
		FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Heavy).Weight
		> FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Standard).Weight);
	TestTrue(TEXT("Heavy comparison shows less speed"),
		FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Heavy).Speed
		< FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Standard).Speed);
	TestTrue(TEXT("Slider comparison shows the longest coast"),
		FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Slider).Coast
		> FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Striker).Coast);
	TestTrue(TEXT("Toppler comparison exposes low stability"),
		FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Toppler).Stability
		< FlickPieceArchetypeRules::GetDisplayStats(EFlickPieceArchetype::Standard).Stability);

	const FLinearColor BlueTeam(0.0f, 0.82f, 1.0f, 1.0f);
	const FLinearColor OrangeTeam(1.0f, 0.31f, 0.055f, 1.0f);
	TestTrue(TEXT("Standard visual accent follows its team"),
		!FlickPieceArchetypeRules::GetVisualAccent(EFlickPieceArchetype::Standard, BlueTeam).Equals(
			FlickPieceArchetypeRules::GetVisualAccent(EFlickPieceArchetype::Standard, OrangeTeam)));
	TestTrue(TEXT("Archetype signature color remains stable across teams"),
		FlickPieceArchetypeRules::GetVisualAccent(EFlickPieceArchetype::Striker, BlueTeam).Equals(
			FlickPieceArchetypeRules::GetVisualAccent(EFlickPieceArchetype::Striker, OrangeTeam)));
	TestTrue(TEXT("Striker and Bouncer retain distinct signature colors"),
		!FlickPieceArchetypeRules::GetVisualAccent(EFlickPieceArchetype::Striker, BlueTeam).Equals(
			FlickPieceArchetypeRules::GetVisualAccent(EFlickPieceArchetype::Bouncer, BlueTeam)));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPieceBalanceMatrixTest,
	"FLICK.Pieces.BalanceMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPieceBalanceMatrixTest::RunTest(const FString& Parameters)
{
	const FFlickPieceArchetypeRules& Standard =
		FlickPieceArchetypeRules::Get(EFlickPieceArchetype::Standard);
	const float StandardBaseline = CalculateHeadOnTargetSpeed(Standard, Standard);
	float MinimumTransferRatio = TNumericLimits<float>::Max();
	float MaximumTransferRatio = 0.0f;
	FString MinimumMatchup;
	FString MaximumMatchup;

	for (int32 AttackerIndex = 0; AttackerIndex < FlickPieceArchetypeRules::ArchetypeCount; ++AttackerIndex)
	{
		const EFlickPieceArchetype AttackerType = static_cast<EFlickPieceArchetype>(AttackerIndex);
		const FFlickPieceArchetypeRules& Attacker = FlickPieceArchetypeRules::Get(AttackerType);
		const float MomentumRatio = Attacker.MassMultiplier * Attacker.LaunchSpeedMultiplier;
		const float CoastRatio = Attacker.LaunchSpeedMultiplier / FMath::Sqrt(
			FMath::Max(Attacker.FrictionMultiplier * Attacker.LinearDampingMultiplier, KINDA_SMALL_NUMBER));

		TestTrue(
			*FString::Printf(TEXT("%s has a bounded launch momentum budget"), *GetPieceArchetypeName(AttackerType)),
			MomentumRatio >= 0.6f && MomentumRatio <= 1.45f);
		TestTrue(
			*FString::Printf(TEXT("%s has a playable footprint"), *GetPieceArchetypeName(AttackerType)),
			Attacker.RadiusMultiplier >= 0.78f && Attacker.RadiusMultiplier <= 1.2f);
		TestTrue(
			*FString::Printf(TEXT("%s has finite travel behavior"), *GetPieceArchetypeName(AttackerType)),
			FMath::IsFinite(CoastRatio) && CoastRatio > 0.25f && CoastRatio < 2.1f);

		for (int32 DefenderIndex = 0; DefenderIndex < FlickPieceArchetypeRules::ArchetypeCount; ++DefenderIndex)
		{
			const EFlickPieceArchetype DefenderType = static_cast<EFlickPieceArchetype>(DefenderIndex);
			const float TransferRatio = CalculateHeadOnTargetSpeed(
				Attacker,
				FlickPieceArchetypeRules::Get(DefenderType)) / StandardBaseline;
			if (TransferRatio < MinimumTransferRatio)
			{
				MinimumTransferRatio = TransferRatio;
				MinimumMatchup = FString::Printf(
					TEXT("%s into %s"),
					*GetPieceArchetypeName(AttackerType),
					*GetPieceArchetypeName(DefenderType));
			}
			if (TransferRatio > MaximumTransferRatio)
			{
				MaximumTransferRatio = TransferRatio;
				MaximumMatchup = FString::Printf(
					TEXT("%s into %s"),
					*GetPieceArchetypeName(AttackerType),
					*GetPieceArchetypeName(DefenderType));
			}
		}
	}

	AddInfo(FString::Printf(
		TEXT("Balance matrix: weakest transfer %.3fx (%s), strongest transfer %.3fx (%s)"),
		MinimumTransferRatio,
		*MinimumMatchup,
		MaximumTransferRatio,
		*MaximumMatchup));
	TestTrue(TEXT("No matchup has negligible direct-impact authority"), MinimumTransferRatio >= 0.45f);
	TestTrue(TEXT("No matchup has excessive direct-impact authority"), MaximumTransferRatio <= 1.35f);

	return true;
}

#endif
