#include "Core/FlickBotShotPlanner.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickBotShotPlannerBasicAttackTest,
	"FLICK.Bot.ShotPlanner.BasicAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBotShotPlannerBasicAttackTest::RunTest(const FString& Parameters)
{
	TArray<FFlickBotPieceState> Pieces = {
		{1, EFlickTeam::Player1, FVector2D(0.0f, -250.0f), 45.0f},
		{2, EFlickTeam::Player2, FVector2D(0.0f, 250.0f), 45.0f}
	};
	FFlickBotShotTuning Tuning;
	Tuning.AimErrorDegrees = 0.0f;
	Tuning.PowerVariation = 0.0f;
	FRandomStream RandomStream(7);

	const FFlickBotShotPlan Plan = FlickBotShotPlanner::PlanShot(
		Pieces,
		EFlickTeam::Player2,
		Tuning,
		RandomStream);
	TestTrue(TEXT("Bot creates a shot"), Plan.IsValid());
	TestEqual(TEXT("Bot shoots its own puck"), Plan.ShooterPieceId, 2);
	TestEqual(TEXT("Bot targets the opponent"), Plan.TargetPieceId, 1);
	TestTrue(TEXT("Bot aims toward the target"), Plan.Direction.Y < -0.99f);
	TestTrue(TEXT("Power respects minimum"), Plan.NormalizedPower >= Tuning.MinimumPower);
	TestTrue(TEXT("Power respects maximum"), Plan.NormalizedPower <= Tuning.MaximumPower);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickBotShotPlannerBlockerAwarenessTest,
	"FLICK.Bot.ShotPlanner.BlockerAwareness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBotShotPlannerBlockerAwarenessTest::RunTest(const FString& Parameters)
{
	TArray<FFlickBotPieceState> Pieces = {
		{10, EFlickTeam::Player2, FVector2D(0.0f, 300.0f), 45.0f},
		{11, EFlickTeam::Player2, FVector2D(250.0f, 300.0f), 45.0f},
		{12, EFlickTeam::Player2, FVector2D(0.0f, 150.0f), 45.0f},
		{20, EFlickTeam::Player1, FVector2D(0.0f, -180.0f), 45.0f},
		{21, EFlickTeam::Player1, FVector2D(250.0f, -180.0f), 45.0f}
	};
	FFlickBotShotTuning Tuning;
	Tuning.AimErrorDegrees = 0.0f;
	Tuning.PowerVariation = 0.0f;
	FRandomStream RandomStream(11);

	const FFlickBotShotPlan Plan = FlickBotShotPlanner::PlanShot(
		Pieces,
		EFlickTeam::Player2,
		Tuning,
		RandomStream);
	TestTrue(TEXT("Bot creates a shot around blockers"), Plan.IsValid());
	TestTrue(TEXT("Bot avoids the friendly-blocked lane"), Plan.ShooterPieceId != 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickBotShotPlannerDividerAwarenessTest,
	"FLICK.Bot.ShotPlanner.DividerAwareness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBotShotPlannerDividerAwarenessTest::RunTest(const FString& Parameters)
{
	TArray<FFlickBotPieceState> Pieces = {
		{10, EFlickTeam::Player2, FVector2D(0.0f, 300.0f), 40.0f},
		{11, EFlickTeam::Player2, FVector2D(400.0f, 300.0f), 40.0f},
		{20, EFlickTeam::Player1, FVector2D(0.0f, -220.0f), 40.0f}
	};
	FFlickBotDividerState RaisedDivider;
	RaisedDivider.Center = FVector2D(0.0f, 30.0f);
	RaisedDivider.Tangent = FVector2D(1.0f, 0.0f);
	RaisedDivider.HalfLength = 105.0f;
	RaisedDivider.HalfThickness = 9.0f;
	RaisedDivider.bRaised = true;

	FFlickBotShotTuning UnawareTuning;
	UnawareTuning.AimErrorDegrees = 0.0f;
	UnawareTuning.PowerVariation = 0.0f;
	UnawareTuning.DecisionNoise = 0.0f;
	UnawareTuning.DividerAwareness = 0.0f;
	UnawareTuning.BankShotSkill = 0.0f;
	UnawareTuning.Dividers.Add(RaisedDivider);
	FRandomStream UnawareRandom(23);
	const FFlickBotShotPlan UnawarePlan = FlickBotShotPlanner::PlanShot(
		Pieces, EFlickTeam::Player2, UnawareTuning, UnawareRandom);
	TestEqual(TEXT("An unaware bot prefers the otherwise strongest blocked lane"), UnawarePlan.ShooterPieceId, 10);

	FFlickBotShotTuning ExpertTuning = UnawareTuning;
	ExpertTuning.DividerAwareness = 1.0f;
	ExpertTuning.BankShotSkill = 1.0f;
	FRandomStream ExpertRandom(23);
	const FFlickBotShotPlan ExpertPlan = FlickBotShotPlanner::PlanShot(
		Pieces, EFlickTeam::Player2, ExpertTuning, ExpertRandom);
	TestTrue(TEXT("An expert bot rejects the divider-blocked approach"), ExpertPlan.IsValid());
	TestEqual(TEXT("An expert bot selects the clear shooter"), ExpertPlan.ShooterPieceId, 11);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickBotShotPlannerBobTest,
	"FLICK.Bot.ShotPlanner.BobOwnColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBotShotPlannerBobTest::RunTest(const FString& Parameters)
{
	TArray<FFlickBotPieceState> Pieces = {
		{1, EFlickTeam::Player1, FVector2D(-100.0f, 0.0f), 45.0f},
		{2, EFlickTeam::Player2, FVector2D(0.0f, 300.0f), 45.0f},
		{3, EFlickTeam::Player2, FVector2D(0.0f, 100.0f), 45.0f}
	};
	TArray<FVector2D> Pockets = {FVector2D(0.0f, -500.0f)};
	FFlickBotShotTuning Tuning;
	Tuning.AimErrorDegrees = 0.0f;
	Tuning.PowerVariation = 0.0f;
	FRandomStream RandomStream(17);

	const FFlickBotShotPlan Plan = FlickBotShotPlanner::PlanBobShot(
		Pieces,
		EFlickTeam::Player2,
		2,
		Pockets,
		Tuning,
		RandomStream);
	TestTrue(TEXT("BOB bot creates a shot"), Plan.IsValid());
	TestEqual(TEXT("BOB bot only shoots its striker"), Plan.ShooterPieceId, 2);
	TestEqual(TEXT("BOB bot targets its own objective color"), Plan.TargetPieceId, 3);
	TestTrue(TEXT("BOB bot aims through its objective toward the pocket"), Plan.Direction.Y < -0.99f);
	return true;
}

#endif
