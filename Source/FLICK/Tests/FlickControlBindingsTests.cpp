#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickControlBindings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickControlBindingsTest,
	"FLICK.Settings.ControlBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickControlBindingsTest::RunTest(const FString& Parameters)
{
	const auto& Controls = FlickControlBindings::GetControls();
	const FString PreviousIni = GGameUserSettingsIni;
	const FString TestIni = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("CameraBindings-") + FGuid::NewGuid().ToString() + TEXT(".ini"));
	GGameUserSettingsIni = TestIni;
	GConfig->Add(TestIni, FConfigFile());
	ON_SCOPE_EXIT { GGameUserSettingsIni = PreviousIni; GConfig->UnloadFile(TestIni); IFileManager::Get().Delete(*TestIni); };
	TestEqual(TEXT("All currently active controls are described"), Controls.Num(), 37);
	TSet<FName> Ids;
	TSet<FKey> Defaults;
	TSet<FKey> FreeCameraDefaults;
	for (const auto& Control : Controls)
	{
		TestFalse(TEXT("Control ID is unique"), Ids.Contains(FName(Control.Id)));
		auto& ModeDefaults = FCString::Strcmp(Control.Group, TEXT("FREE CAMERA")) == 0 ? FreeCameraDefaults : Defaults;
		TestFalse(TEXT("Default key is unique within its camera mode"), ModeDefaults.Contains(Control.DefaultKey));
		Ids.Add(FName(Control.Id));
		ModeDefaults.Add(Control.DefaultKey);
		TestTrue(TEXT("Default key is valid"), Control.DefaultKey.IsValid());
	}
	TestFalse(TEXT("Unknown action cannot be rebound"), FlickControlBindings::SetKey(TEXT("NotAnAction"), EKeys::K));
	TestFalse(TEXT("Duplicate key cannot be assigned"),
		FlickControlBindings::SetKey(TEXT("MoveForward"), FlickControlBindings::GetKey(TEXT("MoveBack"))));
	TestEqual(TEXT("A orbits left by default"), FlickControlBindings::GetKey(TEXT("OrbitLeft")), EKeys::A);
	TestEqual(TEXT("D orbits right by default"), FlickControlBindings::GetKey(TEXT("OrbitRight")), EKeys::D);
	TestEqual(TEXT("W raises the camera by default"), FlickControlBindings::GetKey(TEXT("CameraUp")), EKeys::W);
	TestEqual(TEXT("S lowers the camera by default"), FlickControlBindings::GetKey(TEXT("CameraDown")), EKeys::S);
	TestEqual(TEXT("Wheel up zooms in"), FlickControlBindings::GetKey(TEXT("CameraZoomIn")), EKeys::MouseScrollUp);
	TestTrue(TEXT("Exclusive camera modes can share A"), FlickControlBindings::SetKey(TEXT("OrbitLeft"), EKeys::A));
	TestFalse(TEXT("Simultaneous camera controls cannot share A"), FlickControlBindings::SetKey(TEXT("CameraUp"), EKeys::A));
	TestFalse(TEXT("Shared gameplay actions cannot steal movement keys"), FlickControlBindings::SetKey(TEXT("Shoot"), EKeys::W));
	return true;
}

#endif
