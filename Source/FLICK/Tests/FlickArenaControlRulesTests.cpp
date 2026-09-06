#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickArenaControlRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickArenaControlRulesTest,
	"FLICK.Arena.ControlZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickArenaControlRulesTest::RunTest(const FString& Parameters)
{
	const FVector2D ZoneCenter(100.0f, -50.0f);
	const FVector2D ZoneHalfExtents(60.0f, 20.0f);
	constexpr float ZoneAngle = PI * 0.25f;
	constexpr float PuckRadius = 16.0f;
	const FVector2D ZoneLengthAxis(FMath::Cos(ZoneAngle), FMath::Sin(ZoneAngle));
	const FVector2D ZoneWidthAxis(-FMath::Sin(ZoneAngle), FMath::Cos(ZoneAngle));
	constexpr float ActivationDotRadius = 13.0f;

	TestTrue(
		TEXT("A puck touching the circular activation dot activates the switch"),
		FlickArenaControlRules::DoCirclesOverlap(
			ZoneCenter + FVector2D(PuckRadius + ActivationDotRadius, 0.0f),
			PuckRadius,
			ZoneCenter,
			ActivationDotRadius));
	TestFalse(
		TEXT("Crossing the outer switch ring without touching its dot does not activate it"),
		FlickArenaControlRules::DoCirclesOverlap(
			ZoneCenter + FVector2D(PuckRadius + ActivationDotRadius + 0.1f, 0.0f),
			PuckRadius,
			ZoneCenter,
			ActivationDotRadius));
	TestTrue(
		TEXT("A fast puck crossing the activation dot between frames is detected"),
		FlickArenaControlRules::DoesSweptCircleCrossCircle(
			ZoneCenter - FVector2D(100.0f, 0.0f),
			ZoneCenter + FVector2D(100.0f, 0.0f),
			PuckRadius,
			ZoneCenter,
			ActivationDotRadius));
	TestFalse(
		TEXT("A fast puck missing the activation dot does not activate the switch"),
		FlickArenaControlRules::DoesSweptCircleCrossCircle(
			ZoneCenter + FVector2D(-100.0f, PuckRadius + ActivationDotRadius + 0.1f),
			ZoneCenter + FVector2D(100.0f, PuckRadius + ActivationDotRadius + 0.1f),
			PuckRadius,
			ZoneCenter,
			ActivationDotRadius));

	TestTrue(
		TEXT("A puck partially overlapping an angular switch activates it"),
		FlickArenaControlRules::DoesCircleOverlapOrientedBox(
			ZoneCenter + ZoneLengthAxis * 75.0f, PuckRadius, ZoneCenter, ZoneHalfExtents, ZoneAngle));
	TestFalse(
		TEXT("A puck beyond the switch and its radius does not activate it"),
		FlickArenaControlRules::DoesCircleOverlapOrientedBox(
			ZoneCenter + ZoneLengthAxis * 76.1f, PuckRadius, ZoneCenter, ZoneHalfExtents, ZoneAngle));
	TestTrue(
		TEXT("A fast puck crossing the whole switch between frames is detected"),
		FlickArenaControlRules::DoesSweptCircleCrossOrientedBox(
			ZoneCenter - ZoneLengthAxis * 120.0f,
			ZoneCenter + ZoneLengthAxis * 120.0f,
			PuckRadius,
			ZoneCenter,
			ZoneHalfExtents,
			ZoneAngle));
	TestFalse(
		TEXT("A swept puck outside the expanded switch misses it"),
		FlickArenaControlRules::DoesSweptCircleCrossOrientedBox(
			ZoneCenter - ZoneLengthAxis * 120.0f + ZoneWidthAxis * 37.0f,
			ZoneCenter + ZoneLengthAxis * 120.0f + ZoneWidthAxis * 37.0f,
			PuckRadius,
			ZoneCenter,
			ZoneHalfExtents,
			ZoneAngle));

	return true;
}

#endif
