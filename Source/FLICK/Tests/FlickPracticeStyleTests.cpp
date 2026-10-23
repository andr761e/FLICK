#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/FlickPracticeStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPracticeLayoutTest,"FLICK.UI.PracticeLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickPracticeLayoutTest::RunTest(const FString& Parameters)
{
	using namespace FlickPracticeStyle;
	TestTrue(TEXT("Practice overview fits the contained frontend frame"),ContentWidth<=1504 && ContentHeight<=840);
	TestTrue(TEXT("Illustrated cards have room for named difficulty scores"),CardWidth>=490);
	TestTrue(TEXT("Difficulty panel retains readable descriptions and scores"),DetailWidth-2*DetailPadding>=400);
	const float Body=ContentHeight-HeaderHeight-18-FooterHeight-24;
	const float Details=2*DetailPadding+40+22+17+70+FlickPracticeCatalog::DifficultyCount*DifficultyHeight+20+37+StartHeight;
	TestTrue(TEXT("All three difficulties, instructions and Start fit without clipping"),Details<=Body);
	TestTrue(TEXT("Both focus rows retain the concept's generous illustration height"),(Body-CardGap)/2>=265);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPracticeCopyTest,"FLICK.UI.PracticeFocusCopy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickPracticeCopyTest::RunTest(const FString& Parameters)
{
	for (int32 Category=0;Category<FlickPracticeCatalog::CategoryCount;++Category)
	{
		TestTrue(TEXT("Every focus has descriptive card copy"),FCString::Strlen(FlickPracticeStyle::FocusDescription(Category))>30);
		for (int32 Difficulty=0;Difficulty<FlickPracticeCatalog::DifficultyCount;++Difficulty)
			TestTrue(TEXT("Each difficulty has focus-specific help"),FCString::Strlen(FlickPracticeStyle::DifficultyDescription(Category,Difficulty))>20);
	}
	TestTrue(TEXT("Expert Switch Control preserves the exactly-one-switch rule"),
		FString(FlickPracticeStyle::DifficultyDescription(2,2)).Contains(TEXT("exactly one switch")));
	TestTrue(TEXT("BOB explains striker safety"),FString(FlickPracticeStyle::FocusDescription(3)).Contains(TEXT("without losing your striker")));
	return true;
}
#endif
