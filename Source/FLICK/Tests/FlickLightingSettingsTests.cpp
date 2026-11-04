#if WITH_DEV_AUTOMATION_TESTS
#include "Core/FlickLightingSettings.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLightingSettingsTest, "FLICK.Settings.Lighting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickLightingSettingsTest::RunTest(const FString& Parameters)
{
	using namespace FlickLightingSettings;
	TestEqual(TEXT("Approved ambient is the centred default"), AmbientIntensityScale(3.f), 3.f);
	TestTrue(TEXT("Ambient minimum stays within twenty-five percent"), FMath::IsNearlyEqual(AmbientIntensityScale(0.f), 2.25f, .0001f));
	TestTrue(TEXT("Ambient maximum stays within twenty-five percent"), FMath::IsNearlyEqual(AmbientIntensityScale(20.f), 3.75f, .0001f));
	const FString TestIni = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("LightingSettings-") + FGuid::NewGuid().ToString() + TEXT(".ini"));
	ON_SCOPE_EXIT { GConfig->UnloadFile(TestIni); IFileManager::Get().Delete(*TestIni); };
	// GConfig deliberately does not create arbitrary missing files on SetFloat.
	// Register a writable, isolated configuration rather than touching the user's profile.
	GConfig->Add(TestIni, FConfigFile());
	const auto Reload = [&TestIni]()
	{
		GConfig->UnloadFile(TestIni);
		FConfigFile File;
		File.Read(TestIni);
		GConfig->Add(TestIni, File);
	};
	TestEqual(TEXT("Default menu glare is restrained"), GetValue(EScene::Menu, EControl::Highlights, TestIni), 0.48f);
	TestEqual(TEXT("Default gameplay fill matches the menu"), GetValue(EScene::Gameplay, EControl::Fill, TestIni), 3.0f);
	TestEqual(TEXT("Default gameplay reflections match the menu"), GetValue(EScene::Gameplay, EControl::Highlights, TestIni), 0.48f);
	for (const EScene Scene : {EScene::Menu, EScene::Gameplay})
	{
		for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
		{
			const EControl Control = static_cast<EControl>(Index);
			const FControl& Definition = GetControl(Control);
			TestTrue(TEXT("Default sits exactly in the centre"), FMath::IsNearlyEqual(Definition.MenuDefault, (Definition.Minimum + Definition.Maximum) * .5f, .0001f));
			if (Control != EControl::Ambient)
			{
				TestTrue(TEXT("Minimum stays within twenty-five percent"), FMath::IsNearlyEqual(Definition.Minimum / Definition.MenuDefault, .75f, .0001f));
				TestTrue(TEXT("Maximum stays within twenty-five percent"), FMath::IsNearlyEqual(Definition.Maximum / Definition.MenuDefault, 1.25f, .0001f));
			}
			SetValue(Scene, Control, -4.0f, TestIni);
			TestEqual(TEXT("Negative lighting is clamped"), GetValue(Scene, Control, TestIni), Definition.Minimum);
			SetValue(Scene, Control, 20.0f, TestIni);
			TestEqual(TEXT("Excessive lighting is clamped"), GetValue(Scene, Control, TestIni), Definition.Maximum);
		}
	}
	SetValue(EScene::Menu, EControl::Fill, 2.8f, TestIni);
	TestEqual(TEXT("Menu edits immediately carry into gameplay"), GetValue(EScene::Gameplay, EControl::Fill, TestIni), 2.8f);
	SetValue(EScene::Gameplay, EControl::Fill, 3.2f, TestIni);
	// New temporary files are not subject to the user's INI section-save whitelist.
	if (FConfigFile* File = GConfig->Find(TestIni)) File->bCanSaveAllSections = true;
	GConfig->Flush(false, TestIni);
	Reload();
	TestEqual(TEXT("Gameplay edits update the shared menu value after reload"), GetValue(EScene::Menu, EControl::Fill, TestIni), 3.2f);
	TestEqual(TEXT("Shared gameplay values survive reload"), GetValue(EScene::Gameplay, EControl::Fill, TestIni), 3.2f);
	ResetToDefaults(TestIni);
	GConfig->SetFloat(TEXT("FLICK.LightingSettings"), TEXT("Gameplay.Key"), 2.8f, TestIni);
	GConfig->SetFloat(TEXT("FLICK.LightingSettings"), TEXT("Menu.Key"), 0.57f, TestIni);
	TestEqual(TEXT("Old wide-range settings cannot override the approved preset"), GetValue(EScene::Gameplay, EControl::Key, TestIni), 3.f);
	ResetToDefaults(TestIni);
	Reload();
	for (const EScene Scene : {EScene::Menu, EScene::Gameplay})
		for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
		{
			const EControl Control = static_cast<EControl>(Index);
			const FControl& Definition = GetControl(Control);
			TestEqual(TEXT("Reset restores every setting in both presets"), GetValue(Scene, Control, TestIni),
				Scene == EScene::Menu ? Definition.MenuDefault : Definition.GameplayDefault);
		}
	return true;
}
#endif
