#include "Core/FlickVisualSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickVisualSettings
{
	static const TCHAR* Section = TEXT("FLICK.VisualSettings");
	static constexpr int32 DefaultRenderScale = 110;

	int32 GetRenderScale()
	{
		int32 Value = DefaultRenderScale;
		if (GConfig) GConfig->GetInt(Section, TEXT("RenderScale"), Value, GGameUserSettingsIni);
		return FMath::Clamp(Value, 50, 110);
	}

	bool IsHardwareLumenEnabled()
	{
		bool bEnabled = true;
		if (GConfig) GConfig->GetBool(Section, TEXT("HardwareLumen"), bEnabled, GGameUserSettingsIni);
		return bEnabled;
	}

	void ApplySaved()
	{
		if (IConsoleVariable* Scale = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
		{
			Scale->Set(GetRenderScale(), ECVF_SetByGameSetting);
		}
		if (IConsoleVariable* Lumen = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing")))
		{
			Lumen->Set(IsHardwareLumenEnabled() ? 1 : 0, ECVF_SetByGameSetting);
		}
	}

	void SetRenderScale(const int32 Percent)
	{
		if (!GConfig) return;
		GConfig->SetInt(Section, TEXT("RenderScale"), FMath::Clamp(Percent, 50, 110), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		ApplySaved();
	}

	void SetHardwareLumenEnabled(const bool bEnabled)
	{
		if (!GConfig) return;
		GConfig->SetBool(Section, TEXT("HardwareLumen"), bEnabled, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		ApplySaved();
	}
}
