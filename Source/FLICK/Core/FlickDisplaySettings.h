#pragma once

#include "CoreMinimal.h"

class FConfigCacheIni;

namespace FlickDisplaySettings
{
	// Migrate legacy preferences before the settings object and game window are created.
	void PrepareStartupConfig();
	void MigrateConfig(FConfigCacheIni& Config, const FString& Filename);
}
