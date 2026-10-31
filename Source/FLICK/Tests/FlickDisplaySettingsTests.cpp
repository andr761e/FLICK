#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickDisplaySettings.h"
#include "Game/FlickGameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickDisplaySettingsTest, "FLICK.UI.SafeDisplaySettings",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickDisplaySettingsTest::RunTest(const FString&)
{
 FConfigCacheIni Config(EConfigCacheType::Temporary);
 const FString Filename = FPaths::ProjectSavedDir() / TEXT("Automation/DisplayMigration.ini");
 Config.Add(Filename, FConfigFile());
 const TCHAR* Legacy = TEXT("/Script/Engine.GameUserSettings");
 const TCHAR* Current = TEXT("/Script/FLICK.FlickGameUserSettings");
 Config.SetInt(Legacy, TEXT("Version"), 5, Filename);
 Config.SetInt(Legacy, TEXT("FullscreenMode"), 0, Filename);
 Config.SetInt(Legacy, TEXT("LastConfirmedFullscreenMode"), 0, Filename);
 Config.SetInt(Legacy, TEXT("PreferredFullscreenMode"), 0, Filename);
 Config.SetInt(Legacy, TEXT("ResolutionSizeX"), 1680, Filename);
 Config.SetInt(Legacy, TEXT("ResolutionSizeY"), 1050, Filename);
 Config.SetInt(Legacy, TEXT("FrameRateLimit"), 144, Filename);
 Config.AddToSection(Legacy, TEXT("WindowPositions"), TEXT("(X=30,Y=40)"), Filename);
 Config.AddToSection(Legacy, TEXT("WindowPositions"), TEXT("(X=60,Y=80)"), Filename);
 FlickDisplaySettings::MigrateConfig(Config, Filename);
 int32 Value = -1;
 Config.GetInt(Current, TEXT("FullscreenMode"), Value, Filename);
 TestEqual(TEXT("Existing exclusive mode migrates before the initial window"), Value, 1);
 Config.GetInt(Current, TEXT("PreferredFullscreenMode"), Value, Filename);
 TestEqual(TEXT("Hotkeys prefer borderless"), Value, 1);
 Config.GetInt(Current, TEXT("ResolutionSizeX"), Value, Filename);
 TestEqual(TEXT("Migration preserves the selected resolution"), Value, 1680);
 Config.GetInt(Current, TEXT("FrameRateLimit"), Value, Filename);
 TestEqual(TEXT("Migration preserves unrelated video preferences"), Value, 144);
 TArray<FString> Positions;
 Config.GetArray(Current, TEXT("WindowPositions"), Positions, Filename);
 TestEqual(TEXT("Repeated window positions survive migration"), Positions.Num(), 2);
 Config.SetInt(Current, TEXT("ResolutionSizeX"), 1440, Filename);
 FlickDisplaySettings::MigrateConfig(Config, Filename);
 Config.GetInt(Current, TEXT("ResolutionSizeX"), Value, Filename);
 TestEqual(TEXT("Existing migrated settings are never overwritten by stale legacy settings"), Value, 1440);

 // Validation only: no actual display change in this unit test.
 auto* Settings = NewObject<UFlickGameUserSettings>();
 Settings->SetToDefaults(); Settings->UpdateVersion();
 Settings->SetWindowedResolutionIntent(FIntPoint(1680, 1050));
 Settings->SetFullscreenMode(EWindowMode::Windowed); Settings->ValidateSettings();
 TestEqual(TEXT("Windowed intent is retained"), Settings->GetScreenResolution(), FIntPoint(1680, 1050));
 Settings->SetFullscreenMode(EWindowMode::Fullscreen);
 Settings->SetScreenResolution(FIntPoint(1920, 1080)); Settings->ValidateSettings();
 TestTrue(TEXT("Exclusive requests become borderless"), Settings->GetFullscreenMode() == EWindowMode::WindowedFullscreen);
 TestEqual(TEXT("Fullscreen does not overwrite the windowed size"), Settings->GetRememberedWindowedResolution(), FIntPoint(1680, 1050));
 Settings->SetFullscreenMode(EWindowMode::Windowed); Settings->ConfirmVideoMode(); Settings->ValidateSettings();
 TestEqual(TEXT("Alt+Enter style return restores the previous window size"), Settings->GetScreenResolution(), FIntPoint(1680, 1050));
 TestEqual(TEXT("Hotkey confirmation retains the restored window size"), Settings->GetLastConfirmedScreenResolution(), FIntPoint(1680, 1050));
 Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen); Settings->ValidateSettings();
 Settings->SetWindowedResolutionIntent(FIntPoint(1440, 900));
 Settings->SetFullscreenMode(EWindowMode::Windowed); Settings->ValidateSettings();
 TestEqual(TEXT("An explicit menu selection replaces the remembered size"), Settings->GetScreenResolution(), FIntPoint(1440, 900));
 TestTrue(TEXT("Preferred fullscreen mode stays borderless"), Settings->GetPreferredFullscreenMode() == EWindowMode::WindowedFullscreen);
 return true;
}
#endif
