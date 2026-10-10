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
	TestEqual(TEXT("Default menu glare is restrained"), GetValue(EScene::Menu, EControl::Highlights, TestIni), 0.35f);
	TestEqual(TEXT("Default gameplay fill matches the menu"), GetValue(EScene::Gameplay, EControl::Fill, TestIni), 1.0f);
	TestEqual(TEXT("Default gameplay reflections match the menu"), GetValue(EScene::Gameplay, EControl::Highlights, TestIni), 0.35f);
	for (const EScene Scene : {EScene::Menu, EScene::Gameplay})
	{
		for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
		{
			const EControl Control = static_cast<EControl>(Index);
			const FControl& Definition = GetControl(Control);
			SetValue(Scene, Control, -4.0f, TestIni);
			TestEqual(TEXT("Negative lighting is clamped"), GetValue(Scene, Control, TestIni), 0.0f);
			SetValue(Scene, Control, 20.0f, TestIni);
			TestEqual(TEXT("Excessive lighting is clamped"), GetValue(Scene, Control, TestIni), Definition.Maximum);
		}
	}
	SetValue(EScene::Menu, EControl::Fill, 0.5f, TestIni);
	TestEqual(TEXT("Menu edits immediately carry into gameplay"), GetValue(EScene::Gameplay, EControl::Fill, TestIni), 0.5f);
	SetValue(EScene::Gameplay, EControl::Fill, 2.0f, TestIni);
	// New temporary files are not subject to the user's INI section-save whitelist.
	if (FConfigFile* File = GConfig->Find(TestIni)) File->bCanSaveAllSections = true;
	GConfig->Flush(false, TestIni);
	Reload();
	TestEqual(TEXT("Gameplay edits update the shared menu value after reload"), GetValue(EScene::Menu, EControl::Fill, TestIni), 2.0f);
	TestEqual(TEXT("Shared gameplay values survive reload"), GetValue(EScene::Gameplay, EControl::Fill, TestIni), 2.0f);
	GConfig->SetFloat(TEXT("FLICK.LightingSettings"), TEXT("Gameplay.Key"), 2.8f, TestIni);
	GConfig->SetFloat(TEXT("FLICK.LightingSettings"), TEXT("Menu.Key"), 0.57f, TestIni);
	TestEqual(TEXT("Legacy menu settings take precedence over old gameplay settings"), GetValue(EScene::Gameplay, EControl::Key, TestIni), 0.57f);
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
