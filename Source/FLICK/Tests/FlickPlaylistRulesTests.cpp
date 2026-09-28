#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickPlaylistRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPlaylistCareerEligibilityTest,
	"FLICK.Playlists.CareerEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPlaylistCareerEligibilityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Casual result counts"), FlickPlaylistRules::CountsForCareer(true, true, false));
	TestFalse(TEXT("Private result is excluded"), FlickPlaylistRules::CountsForCareer(true, false, true));
	TestFalse(TEXT("Private result stays excluded if matchmaking is flagged"), FlickPlaylistRules::CountsForCareer(true, true, true));
	TestFalse(TEXT("Training result is excluded"), FlickPlaylistRules::CountsForCareer(true, false, false));
	TestFalse(TEXT("Incomplete match is excluded"), FlickPlaylistRules::CountsForCareer(false, true, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPlaylistPauseEligibilityTest,
	"FLICK.Playlists.PauseEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPlaylistPauseEligibilityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Offline training pauses"), FlickPlaylistRules::CanPauseWorld(true, true, false, false, false, 1));
	TestTrue(TEXT("Solo private match pauses"), FlickPlaylistRules::CanPauseWorld(false, false, true, false, false, 1));
	TestFalse(TEXT("Private match with two humans continues"), FlickPlaylistRules::CanPauseWorld(false, false, true, false, false, 2));
	TestFalse(TEXT("Casual continues"), FlickPlaylistRules::CanPauseWorld(false, false, false, true, false, 1));
	TestFalse(TEXT("Competitive continues"), FlickPlaylistRules::CanPauseWorld(false, false, false, true, false, 2));
	TestFalse(TEXT("Dedicated authority continues"), FlickPlaylistRules::CanPauseWorld(false, false, true, false, true, 1));
	return true;
}

#endif
