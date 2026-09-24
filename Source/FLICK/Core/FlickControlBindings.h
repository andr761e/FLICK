#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

// Keyboard/mouse bindings are stored independently of the current match.
// IDs are stable config keys; changing a label does not invalidate a saved binding.
namespace FlickControlBindings
{
	struct FControl
	{
		const TCHAR* Id;
		const TCHAR* Label;
		const TCHAR* Group;
		FKey DefaultKey;
	};

	FLICK_API const TArray<FControl>& GetControls();
	FLICK_API FKey GetKey(const TCHAR* Id);
	FLICK_API bool SetKey(const TCHAR* Id, FKey Key);
	FLICK_API void ResetToDefaults();
}
