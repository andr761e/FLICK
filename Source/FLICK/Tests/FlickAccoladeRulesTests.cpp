#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickAccoladeRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickAccoladeRulesTest,
	"FLICK.MatchRules.Accolades",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickAccoladeRulesTest::RunTest(const FString& Parameters)
{
	FFlickShotAccoladeContext Context;
	Context.OpponentEliminated = 2;
	Context.OpponentRemaining = 0;
	Context.OwnRemaining = 3;
	Context.bSwitchActivated = true;
	Context.bShotPieceHitDivider = true;
	Context.bNewDividerAffectedPlay = true;
	Context.bDominoKnockout = true;
	Context.bLongRangeKnockout = true;
	Context.DirectlyContactedPucks = 3;
	Context.bShotPieceSurvivedNearEdge = true;
	Context.bBuzzerRelease = true;

	const TArray<EFlickAccolade> Accolades = FlickAccoladeRules::EvaluateShot(Context);
	TestTrue(TEXT("Switch knockout is recognized"), Accolades.Contains(EFlickAccolade::SwitchKnockout));
	TestTrue(TEXT("Divider bank is recognized"), Accolades.Contains(EFlickAccolade::DividerBank));
	TestTrue(TEXT("Trap shot is recognized"), Accolades.Contains(EFlickAccolade::TrapShot));
	TestTrue(TEXT("Domino knockout is recognized"), Accolades.Contains(EFlickAccolade::DominoKnockout));
	TestTrue(TEXT("Team wipeout is recognized"), Accolades.Contains(EFlickAccolade::TeamWipeout));
	TestTrue(TEXT("Long range knockout is recognized"), Accolades.Contains(EFlickAccolade::LongRangeKnockout));
	TestTrue(TEXT("Pinball is recognized"), Accolades.Contains(EFlickAccolade::Pinball));
	TestTrue(TEXT("Precision knockout is recognized"), Accolades.Contains(EFlickAccolade::PrecisionKnockout));
	TestTrue(TEXT("Buzzer beater is recognized"), Accolades.Contains(EFlickAccolade::BuzzerBeater));
	TestEqual(TEXT("Team wipeout is worth 100 points"), GetFlickAccoladeBonusPoints(EFlickAccolade::TeamWipeout), 100);
	TestEqual(TEXT("Style accolades do not inflate score"), GetFlickAccoladeBonusPoints(EFlickAccolade::TrapShot), 0);

	Context.OwnEliminated = 1;
	Context.OpponentEliminated = 1;
	TestTrue(
		TEXT("A winning one-for-one sacrifice is a perfect trade"),
		FlickAccoladeRules::EvaluateShot(Context).Contains(EFlickAccolade::PerfectTrade));
	TestTrue(TEXT("Full surviving lineup is flawless"), FlickAccoladeRules::IsFlawlessRound(4, 4));
	TestFalse(TEXT("One lost puck is not flawless"), FlickAccoladeRules::IsFlawlessRound(3, 4));

	FFlickShotAccoladeContext Miss;
	Miss.bSwitchActivated = true;
	Miss.bBuzzerRelease = true;
	TestTrue(TEXT("No knockout means no shot accolade"), FlickAccoladeRules::EvaluateShot(Miss).IsEmpty());
	return true;
}

#endif
