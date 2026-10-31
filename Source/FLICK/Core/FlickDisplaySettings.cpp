#include "Core/FlickDisplaySettings.h"

#include "GameFramework/GameUserSettings.h"
#include "Core/FlickLog.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

void FlickDisplaySettings::MigrateConfig(FConfigCacheIni& Config, const FString& Filename)
{
	constexpr const TCHAR* Legacy = TEXT("/Script/Engine.GameUserSettings");
	constexpr const TCHAR* Current = TEXT("/Script/FLICK.FlickGameUserSettings");
	int32 Version = 0;
	if (!Config.GetInt(Current, TEXT("Version"), Version, Filename)
		&& Config.GetInt(Legacy, TEXT("Version"), Version, Filename))
	{
		// Preserve all existing video preferences and repeated array entries.
		TArray<FString> Entries;
		Config.GetSection(Legacy, Entries, Filename);
		Config.EmptySection(Current, Filename);
		for (const FString& Entry : Entries)
		{
			FString Key, Value;
			if (Entry.Split(TEXT("="), &Key, &Value)) Config.AddToSection(Current, FName(*Key), Value, Filename);
		}
	}
	for (const TCHAR* Section : {Legacy, Current})
	{
		for (const TCHAR* Key : {TEXT("FullscreenMode"), TEXT("LastConfirmedFullscreenMode")})
		{
			int32 Mode = 1;
			if (Config.GetInt(Section, Key, Mode, Filename) && Mode == 0) Config.SetInt(Section, Key, 1, Filename);
		}
		Config.SetInt(Section, TEXT("PreferredFullscreenMode"), 1, Filename);
	}
}

void FlickDisplaySettings::PrepareStartupConfig()
{
	if (!GConfig) return;
	UGameUserSettings::LoadConfigIni();
	if (GConfig && !GGameUserSettingsIni.IsEmpty()) MigrateConfig(*GConfig, GGameUserSettingsIni);
	if (auto* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode")))
		Mode->Set(1, ECVF_SetByGameSetting);
	UE_LOG(LogFlick, Log, TEXT("DISPLAY_POLICY_READY: fullscreen is borderless; saved exclusive modes migrated before window creation"));
}
