#include "Core/FlickVisualSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickVisualSettings
{
	static const TCHAR* Section = TEXT("FLICK.VisualSettings");
	static constexpr int32 DefaultRenderScale = 110;
	static const TCHAR* InterfaceSection = TEXT("FLICK.InterfaceSettings");

	int32 GetPuckIndicatorStyle()
	{
		int32 Style = 0;
		if (GConfig) GConfig->GetInt(InterfaceSection, TEXT("PuckIndicatorStyle"), Style, GGameUserSettingsIni);
		return FMath::Clamp(Style, 0, 2);
	}

	void SetPuckIndicatorStyle(int32 Style)
	{
		if (!GConfig) return;
		GConfig->SetInt(InterfaceSection, TEXT("PuckIndicatorStyle"), FMath::Clamp(Style, 0, 2), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	int32 GetOpponentRingColor()
	{
		int32 Color = 0;
		if (GConfig) GConfig->GetInt(InterfaceSection, TEXT("OpponentRingColor"), Color, GGameUserSettingsIni);
		return FMath::Clamp(Color, 0, OpponentRingColorCount - 1);
	}

	void SetOpponentRingColor(int32 Color)
	{
		if (!GConfig) return;
		GConfig->SetInt(InterfaceSection, TEXT("OpponentRingColor"), FMath::Clamp(Color, 0, OpponentRingColorCount - 1), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	FLinearColor GetOpponentRingPaletteColor(int32 Color)
	{
		static const FLinearColor Colors[] = {
			FLinearColor(1.f, .18f, .015f), FLinearColor(1.f, .8f, .01f),
			FLinearColor(1.f, .015f, .45f), FLinearColor(.85f, .85f, .85f),
			FLinearColor(1.f, .015f, .01f), FLinearColor(.01f, .8f, 1.f),
			FLinearColor(.3f, 1.f, .01f), FLinearColor(.01f, .65f, .04f), FLinearColor(.01f, 1.f, .45f),
			FLinearColor(.01f, .35f, 1.f), FLinearColor(.04f, .07f, 1.f), FLinearColor(.32f, .02f, 1.f),
			FLinearColor(.7f, .01f, 1.f), FLinearColor(1.f, .01f, .8f), FLinearColor(1.f, .28f, .18f),
			FLinearColor(1.f, .55f, .08f), FLinearColor(.4f, .7f, 1.f), FLinearColor(.02f, .025f, .035f)};
		return Colors[FMath::Clamp(Color, 0, OpponentRingColorCount - 1)];
	}

	FText GetOpponentRingColorName(int32 Color)
	{
		static const TCHAR* Names[] = {TEXT("Orange"), TEXT("Yellow"), TEXT("Pink"), TEXT("White"), TEXT("Red"), TEXT("Cyan"),
			TEXT("Lime"), TEXT("Green"), TEXT("Mint"), TEXT("Sky blue"), TEXT("Blue"), TEXT("Indigo"),
			TEXT("Purple"), TEXT("Magenta"), TEXT("Coral"), TEXT("Gold"), TEXT("Ice blue"), TEXT("Graphite")};
		return FText::FromString(Names[FMath::Clamp(Color, 0, OpponentRingColorCount - 1)]);
	}

	FLinearColor GetOpponentRingTint() { return GetOpponentRingPaletteColor(GetOpponentRingColor()); }

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
