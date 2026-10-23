#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/FlickSettingsStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickSettingsLayoutTest, "FLICK.UI.SettingsLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickSettingsLayoutTest::RunTest(const FString& Parameters)
{
	using namespace FlickSettingsStyle;
	TestTrue(TEXT("All ten categories fit in the body without shrinking labels"), 10 * (RailRowHeight + 1) + 16 <= 664);
	TestTrue(TEXT("Normal settings leave the right-hand product image uncovered"), 48 + RailWidth + 16 + PanelWidth <= 1160);
	TestTrue(TEXT("Wide settings and the rail fit the shared 1600 design frame"), 96 + RailWidth + 16 + WidePanelWidth <= 1600);
	TestTrue(TEXT("Help text retains a usable width next to its illustration"), PanelWidth - 40 - 24 - HelpImageWidth - 28 >= 420);
	return true;
}
#endif
