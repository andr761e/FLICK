#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickControlBindings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickControlBindingsTest,
	"FLICK.Settings.ControlBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickControlBindingsTest::RunTest(const FString& Parameters)
{
	const auto& Controls = FlickControlBindings::GetControls();
	TestEqual(TEXT("All currently active controls are described"), Controls.Num(), 26);
	TSet<FName> Ids;
	TSet<FKey> Defaults;
	for (const auto& Control : Controls)
	{
		TestFalse(TEXT("Control ID is unique"), Ids.Contains(FName(Control.Id)));
		TestFalse(TEXT("Default key is unique"), Defaults.Contains(Control.DefaultKey));
		Ids.Add(FName(Control.Id));
		Defaults.Add(Control.DefaultKey);
		TestTrue(TEXT("Default key is valid"), Control.DefaultKey.IsValid());
	}
	TestFalse(TEXT("Unknown action cannot be rebound"), FlickControlBindings::SetKey(TEXT("NotAnAction"), EKeys::K));
	TestFalse(TEXT("Duplicate key cannot be assigned"),
		FlickControlBindings::SetKey(TEXT("MoveForward"), FlickControlBindings::GetKey(TEXT("MoveBack"))));
	return true;
}

#endif
