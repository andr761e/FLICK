#if WITH_DEV_AUTOMATION_TESTS

#include "Game/FlickGameInstance.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickProfileStatsAccumulateTest,
	"FLICK.Profile.StatsAccumulateCompletedMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickProfileStatsAccumulateTest::RunTest(const FString& Parameters)
{
	FFlickProfileStats Stats;
	Stats.RecordMatch(480, 3, 1, 7, true);
	Stats.RecordMatch(260, 1, 0, 5, false);

	TestEqual(TEXT("Matches accumulate"), Stats.MatchesPlayed, 2);
	TestEqual(TEXT("Only wins accumulate"), Stats.Wins, 1);
	TestEqual(TEXT("Points accumulate"), Stats.Points, 740);
	TestEqual(TEXT("Knockouts accumulate"), Stats.Knockouts, 4);
	TestEqual(TEXT("Double knockouts accumulate"), Stats.DoubleKnockouts, 1);
	TestEqual(TEXT("Shots accumulate"), Stats.Shots, 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickProfileStatsIgnoreNegativeTest,
	"FLICK.Profile.StatsIgnoreNegativeMatchValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickProfileStatsIgnoreNegativeTest::RunTest(const FString& Parameters)
{
	FFlickProfileStats Stats;
	Stats.RecordMatch(-100, -2, -1, -5, false);

	TestEqual(TEXT("A completed match is still counted"), Stats.MatchesPlayed, 1);
	TestEqual(TEXT("Negative points are ignored"), Stats.Points, 0);
	TestEqual(TEXT("Negative knockouts are ignored"), Stats.Knockouts, 0);
	TestEqual(TEXT("Negative double knockouts are ignored"), Stats.DoubleKnockouts, 0);
	TestEqual(TEXT("Negative shots are ignored"), Stats.Shots, 0);
	return true;
}

#endif
