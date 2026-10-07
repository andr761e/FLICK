#if WITH_DEV_AUTOMATION_TESTS
#include "Core/FlickQuickChats.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickQuickChatSettingsTest, "FLICK.Settings.QuickChats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickQuickChatSettingsTest::RunTest(const FString& Parameters)
{
	// Exercise real persistence without touching the player's preferences.
	TGuardValue<FString> IsolatedConfig(GGameUserSettingsIni, FConfigCacheIni::NormalizeConfigIniPath(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Config/QuickChatAutomation.ini"))));
	GConfig->Add(GGameUserSettingsIni, FConfigFile());
	FlickQuickChats::Reset();
	for (int32 Group = 0; Group < 4; ++Group)
		for (int32 Slot = 0; Slot < 4; ++Slot)
		{
			TestEqual(TEXT("Default slots have stable phrase IDs"), FlickQuickChats::GetSlot(Group, Slot), FString(FlickQuickChats::GetPhrases()[Group * 4 + Slot].Id));
			TestTrue(TEXT("Every slot can be customized"), FlickQuickChats::SetSlot(Group, Slot, TEXT("NiceKO")));
			TestEqual(TEXT("Slot reads saved phrase"), FlickQuickChats::GetSlot(Group, Slot), FString(TEXT("NiceKO")));
		}
	GConfig->UnloadFile(GGameUserSettingsIni);
	GConfig->LoadFile(GGameUserSettingsIni);
	TestEqual(TEXT("Customization survives config reload"), FlickQuickChats::GetSlot(3, 3), FString(TEXT("NiceKO")));
	GConfig->SetString(TEXT("FLICK.QuickChats"), TEXT("Slot0_0"), TEXT("obsolete-id"), GGameUserSettingsIni);
	TestEqual(TEXT("Invalid saved ID falls back safely"), FlickQuickChats::GetSlot(0, 0), FString(TEXT("GotIt")));
	FlickQuickChats::Reset();
	TestEqual(TEXT("Reset restores defaults"), FlickQuickChats::GetSlot(3, 3), FString(TEXT("Rematch")));
	return true;
}
#endif
