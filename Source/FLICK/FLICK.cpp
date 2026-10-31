// Copyright Epic Games, Inc. All Rights Reserved.

#include "FLICK.h"
#include "Modules/ModuleManager.h"
#include "Core/FlickDisplaySettings.h"

class FFlickGameModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FlickDisplaySettings::PrepareStartupConfig();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFlickGameModule, FLICK, "FLICK");
