#include "Core/FlickLightingSettings.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickLightingSettings
{
	namespace
	{
		const TCHAR* Section = TEXT("FLICK.LightingSettings");
		const FControl Controls[] = {
			{TEXT("Ambient"), TEXT("AMBIENT LIGHT"), TEXT("Even environment lighting across the metallic surface."), 3.0f, 1.0f, 1.0f},
			{TEXT("Fill"), TEXT("ARENA FILL"), TEXT("Broad, reflection-free light. Raise this if the board is too dark."), 3.0f, 1.0f, 1.0f},
			{TEXT("Key"), TEXT("COOL SOFTBOX"), TEXT("Cool overhead illumination and satin-metal highlight bands."), 3.0f, 1.0f, 1.0f},
			{TEXT("Rim"), TEXT("WARM SOFTBOX"), TEXT("Warm illumination for colour separation and metallic depth."), 3.0f, 1.0f, 1.0f},
			{TEXT("Accents"), TEXT("TEAM ACCENT LIGHTS"), TEXT("Cyan and orange light cast onto the arena; not the glowing rim strips."), 3.0f, 1.0f, 1.0f},
			{TEXT("Direct"), TEXT("OVERHEAD LIGHT"), TEXT("The neutral directional light and its shadows."), 3.0f, 1.0f, 1.0f},
			{TEXT("Highlights"), TEXT("DIRECT LIGHT REFLECTIONS"), TEXT("Lower this to tame camera-angle-dependent glare on the floor and pucks."), 1.0f, 0.35f, 0.35f}
		};
		static_assert(UE_ARRAY_COUNT(Controls) == static_cast<int32>(EControl::Count));
		FString Key(EScene Scene, EControl Control)
		{
			// Keep the original menu keys so existing menu adjustments become the
			// shared preset without overwriting them with older gameplay values.
			return FString(TEXT("Menu.")) + GetControl(Control).Key;
		}
		float Default(EScene Scene, EControl Control)
		{
			return Scene == EScene::Menu ? GetControl(Control).MenuDefault : GetControl(Control).GameplayDefault;
		}
		const FString& File(const FString& IniFile) { return IniFile.IsEmpty() ? GGameUserSettingsIni : IniFile; }
		float Validate(float Value, EScene Scene, EControl Control)
		{
			return FMath::IsFinite(Value) ? FMath::Clamp(Value, 0.0f, GetControl(Control).Maximum) : Default(Scene, Control);
		}
	}
	const FControl& GetControl(EControl Control)
	{
		return Controls[FMath::Clamp(static_cast<int32>(Control), 0, static_cast<int32>(EControl::Count) - 1)];
	}
	float GetValue(EScene Scene, EControl Control, const FString& IniFile)
	{
		float Value = Default(Scene, Control);
		if (GConfig) GConfig->GetFloat(Section, *Key(Scene, Control), Value, File(IniFile));
		return Validate(Value, Scene, Control);
	}
	void SetValue(EScene Scene, EControl Control, float Value, const FString& IniFile)
	{
		if (!GConfig) return;
		GConfig->SetFloat(Section, *Key(Scene, Control), Validate(Value, Scene, Control), File(IniFile));
		GConfig->Flush(false, File(IniFile));
	}
	void ResetToDefaults(const FString& IniFile)
	{
		if (!GConfig) return;
		GConfig->EmptySection(Section, File(IniFile));
		GConfig->Flush(false, File(IniFile));
	}
}
