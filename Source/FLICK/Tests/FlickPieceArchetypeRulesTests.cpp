#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickPieceArchetypeRules.h"
#include "Misc/AutomationTest.h"

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

	return true;
}

#endif
