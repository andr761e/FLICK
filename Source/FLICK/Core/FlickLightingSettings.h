#pragma once

#include "CoreMinimal.h"

// Local presentation preferences. These never alter physics or replicate to peers.
namespace FlickLightingSettings
{
	enum class EScene : uint8 { Menu, Gameplay };
	enum class EControl : uint8 { Ambient, Fill, Key, Rim, Accents, Direct, Highlights, Count };
	struct FControl
	{
		const TCHAR* Key;
		const TCHAR* Label;
		const TCHAR* Description;
		float Maximum;
		float MenuDefault;
		float GameplayDefault;
	};
	const FControl& GetControl(EControl Control);
	float GetValue(EScene Scene, EControl Control, const FString& IniFile = FString());
	void SetValue(EScene Scene, EControl Control, float Value, const FString& IniFile = FString());
	void ResetToDefaults(const FString& IniFile = FString());
}
