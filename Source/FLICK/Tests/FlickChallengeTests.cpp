#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickChallengeCatalog.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickChallengeProgressTest,
	"FLICK.Challenges.ProgressAndFeaturedOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickChallengeProgressTest::RunTest(const FString& Parameters)
{
	FFlickProfileStats Stats;
	Stats.RecordMatch(200, 4, 0, 20, false);
	TestEqual(TEXT("First match complete"), FlickChallengeCatalog::GetProgress(FlickChallengeCatalog::Challenges[0], Stats), 1);
	TestEqual(TEXT("Shot progress uses saved career counter"), FlickChallengeCatalog::GetProgress(FlickChallengeCatalog::Challenges[1], Stats), 20);
	const TArray<int32> Featured = FlickChallengeCatalog::GetFeaturedIndices(Stats);
	TestEqual(TEXT("Three featured challenges"), Featured.Num(), 3);
	if (Featured.Num() == 3)
	{
		TestEqual(TEXT("Closest incomplete challenge is first"), Featured[0], 1);
		TestEqual(TEXT("Equal progress breaks ties by catalog order"), Featured[1], 2);
		TestEqual(TEXT("Completed challenge is excluded"), Featured.Contains(0), false);
	}
	return true;
}

#endif
