#include "Core/FlickControlBindings.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickControlBindings
{
	static const TCHAR* Section = TEXT("FLICK.ControlBindings");

	const TArray<FControl>& GetControls()
	{
		static const TArray<FControl> Controls = {
			{TEXT("Shoot"), TEXT("AIM / SHOOT"), TEXT("MATCH"), EKeys::LeftMouseButton},
			{TEXT("Secondary"), TEXT("SECONDARY ACTION"), TEXT("MATCH"), EKeys::RightMouseButton},
			{TEXT("Menu"), TEXT("PAUSE / BACK"), TEXT("MATCH"), EKeys::Escape},
			{TEXT("Scoreboard"), TEXT("SHOW SCOREBOARD"), TEXT("MATCH"), EKeys::Tab},
			{TEXT("TopView"), TEXT("TOP VIEW"), TEXT("CAMERA"), EKeys::V},
			{TEXT("FreeCamera"), TEXT("FREE CAMERA"), TEXT("CAMERA"), EKeys::X},
			{TEXT("CameraReset"), TEXT("RESET CAMERA"), TEXT("CAMERA"), EKeys::F},
			{TEXT("CameraUp"), TEXT("CAMERA HIGHER / ZOOM"), TEXT("CAMERA"), EKeys::MouseScrollUp},
			{TEXT("CameraDown"), TEXT("CAMERA LOWER / ZOOM"), TEXT("CAMERA"), EKeys::MouseScrollDown},
			{TEXT("OrbitLeft"), TEXT("ORBIT LEFT"), TEXT("CAMERA"), EKeys::Q},
			{TEXT("OrbitRight"), TEXT("ORBIT RIGHT"), TEXT("CAMERA"), EKeys::E},
			{TEXT("MoveForward"), TEXT("FREE CAMERA FORWARD"), TEXT("FREE CAMERA"), EKeys::W},
			{TEXT("MoveBack"), TEXT("FREE CAMERA BACK"), TEXT("FREE CAMERA"), EKeys::S},
			{TEXT("MoveLeft"), TEXT("FREE CAMERA LEFT"), TEXT("FREE CAMERA"), EKeys::A},
			{TEXT("MoveRight"), TEXT("FREE CAMERA RIGHT"), TEXT("FREE CAMERA"), EKeys::D},
			{TEXT("MoveUp"), TEXT("FREE CAMERA UP"), TEXT("FREE CAMERA"), EKeys::SpaceBar},
			{TEXT("MoveDown"), TEXT("FREE CAMERA DOWN"), TEXT("FREE CAMERA"), EKeys::LeftControl},
			{TEXT("MoveDownAlt"), TEXT("FREE CAMERA DOWN (ALT)"), TEXT("FREE CAMERA"), EKeys::RightControl},
			{TEXT("MoveBoost"), TEXT("FREE CAMERA BOOST"), TEXT("FREE CAMERA"), EKeys::LeftShift},
			{TEXT("MoveBoostAlt"), TEXT("FREE CAMERA BOOST (ALT)"), TEXT("FREE CAMERA"), EKeys::RightShift},
			{TEXT("Restart"), TEXT("RESTART TRAINING"), TEXT("TRAINING"), EKeys::R},
			{TEXT("Editor"), TEXT("TOGGLE BOARD EDITOR"), TEXT("TRAINING"), EKeys::T},
			{TEXT("OwnPuck"), TEXT("PLACE OWN PUCK"), TEXT("TRAINING"), EKeys::One},
			{TEXT("TargetPuck"), TEXT("PLACE TARGET PUCK"), TEXT("TRAINING"), EKeys::Two},
			{TEXT("Clear"), TEXT("CLEAR BOARD"), TEXT("TRAINING"), EKeys::C},
			{TEXT("Remove"), TEXT("REMOVE PUCK"), TEXT("TRAINING"), EKeys::Delete}};
		return Controls;
	}

	FKey GetKey(const TCHAR* Id)
	{
		for (const FControl& Control : GetControls())
		{
			if (FCString::Strcmp(Control.Id, Id) == 0)
			{
				FString Saved;
				if (GConfig && GConfig->GetString(Section, Id, Saved, GGameUserSettingsIni))
				{
					const FKey Key(*Saved);
					if (Key.IsValid()) return Key;
				}
				return Control.DefaultKey;
			}
		}
		return EKeys::Invalid;
	}

	bool SetKey(const TCHAR* Id, const FKey Key)
	{
		if (!GConfig || !Key.IsValid() || Key.IsGamepadKey()) return false;
		bool bKnown = false;
		for (const FControl& Control : GetControls())
		{
			bKnown |= FCString::Strcmp(Control.Id, Id) == 0;
			if (FCString::Strcmp(Control.Id, Id) != 0 && GetKey(Control.Id) == Key) return false;
		}
		if (!bKnown) return false;
		GConfig->SetString(Section, Id, *Key.GetFName().ToString(), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		return true;
	}

	void ResetToDefaults()
	{
		if (!GConfig) return;
		GConfig->EmptySection(Section, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}
