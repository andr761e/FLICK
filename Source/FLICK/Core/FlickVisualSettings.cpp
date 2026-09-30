#include "Core/FlickVisualSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickVisualSettings
{
	static const TCHAR* Section = TEXT("FLICK.VisualSettings");
	static constexpr int32 DefaultRenderScale = 110;
	static const TCHAR* InterfaceSection = TEXT("FLICK.InterfaceSettings");

	bool IsColorBlindAssistEnabled()
	{
		bool bEnabled = false;
		if (GConfig) GConfig->GetBool(InterfaceSection, TEXT("ColorBlindAssist"), bEnabled, GGameUserSettingsIni);
		return bEnabled;
	}

	void SetColorBlindAssistEnabled(bool bEnabled)
	{
		if (!GConfig) return;
		GConfig->SetBool(InterfaceSection, TEXT("ColorBlindAssist"), bEnabled, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	int32 GetPuckHoverSize()
	{
		int32 Value = 12;
		if (GConfig) GConfig->GetInt(InterfaceSection, TEXT("PuckHoverSize"), Value, GGameUserSettingsIni);
		return FMath::Clamp(Value, 10, 18);
	}

	void SetPuckHoverSize(int32 Size)
	{
		if (!GConfig) return;
		GConfig->SetInt(InterfaceSection, TEXT("PuckHoverSize"), FMath::Clamp(Size, 10, 18), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	int32 GetPuckHoverDetail()
	{
		int32 Value = 3;
		if (GConfig) GConfig->GetInt(InterfaceSection, TEXT("PuckHoverDetail"), Value, GGameUserSettingsIni);
		return FMath::Clamp(Value, 0, 3);
	}

	void SetPuckHoverDetail(int32 Detail)
	{
		if (!GConfig) return;
		GConfig->SetInt(InterfaceSection, TEXT("PuckHoverDetail"), FMath::Clamp(Detail, 0, 3), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	bool IsFpsVisible()
	{
		bool bVisible = false;
		if (GConfig) GConfig->GetBool(Section, TEXT("ShowFPS"), bVisible, GGameUserSettingsIni);
		return bVisible;
	}

	void SetFpsVisible(const bool bVisible)
	{
		if (!GConfig) return;
		GConfig->SetBool(Section, TEXT("ShowFPS"), bVisible, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

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
