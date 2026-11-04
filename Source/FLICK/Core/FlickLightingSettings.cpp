#include "Core/FlickLightingSettings.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickLightingSettings
{
	namespace
	{
		const TCHAR* Section = TEXT("FLICK.LightingSettings");
		const FControl Controls[] = {
			{TEXT("Ambient"), TEXT("AMBIENT LIGHT"), TEXT("Small brightness adjustment around the recommended environment lighting."), 1.875f, 4.125f, 3.f, 3.f},
			{TEXT("Fill"), TEXT("ARENA FILL"), TEXT("Broad, reflection-free light. Centre restores the recommended brightness."), 2.25f, 3.75f, 3.f, 3.f},
			{TEXT("Key"), TEXT("COOL SOFTBOX"), TEXT("Cool overhead illumination and satin-metal highlight bands."), 2.25f, 3.75f, 3.f, 3.f},
			{TEXT("Rim"), TEXT("WARM SOFTBOX"), TEXT("Warm softbox, stadium spill and amber light beneath the rim. Does not change the visible LED strips."), 2.25f, 3.75f, 3.f, 3.f},
			{TEXT("Accents"), TEXT("TEAM ACCENT LIGHTS"), TEXT("Cyan and orange light cast onto the arena; not the glowing rim strips."), 1.2375f, 2.0625f, 1.65f, 1.65f},
			{TEXT("Direct"), TEXT("OVERHEAD LIGHT"), TEXT("The neutral directional light and its shadows."), 1.6875f, 2.8125f, 2.25f, 2.25f},
			{TEXT("Highlights"), TEXT("DIRECT LIGHT REFLECTIONS"), TEXT("Controls arena and stadium reflections. Pucks retain their own consistent lighting."), .36f, .60f, .48f, .48f}
		};
		static_assert(UE_ARRAY_COUNT(Controls) == static_cast<int32>(EControl::Count));
		FString Key(EScene Scene, EControl Control)
		{
			// The approved preset replaces the old wide-range defaults once. Keep
			// its trims separate so old settings cannot pin the new sliders at an edge.
			return FString(TEXT("Balanced.Menu.")) + GetControl(Control).Key;
		}
		float Default(EScene Scene, EControl Control)
		{
			return Scene == EScene::Menu ? GetControl(Control).MenuDefault : GetControl(Control).GameplayDefault;
		}
		const FString& File(const FString& IniFile) { return IniFile.IsEmpty() ? GGameUserSettingsIni : IniFile; }
		float Validate(float Value, EScene Scene, EControl Control)
		{
			return FMath::IsFinite(Value) ? FMath::Clamp(Value, GetControl(Control).Minimum, GetControl(Control).Maximum) : Default(Scene, Control);
		}
	}
	const FControl& GetControl(EControl Control)
	{
		return Controls[FMath::Clamp(static_cast<int32>(Control), 0, static_cast<int32>(EControl::Count) - 1)];
	}
	float AmbientIntensityScale(float SliderValue)
	{
		// Preserve the existing response: the approved old 300% setting is 3x.
		const FControl& Ambient = GetControl(EControl::Ambient);
		return 1.f + (2.f / 3.f) * FMath::Clamp(SliderValue, Ambient.Minimum, Ambient.Maximum);
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
