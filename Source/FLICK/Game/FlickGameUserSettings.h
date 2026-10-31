#pragma once

#include "GameFramework/GameUserSettings.h"
#include "FlickGameUserSettings.generated.h"

// Fullscreen is always borderless. Keeping this policy in settings also covers
// Unreal's Alt+Enter/F11 path, not just our video menu.
UCLASS(config=GameUserSettings)
class FLICK_API UFlickGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
public:
	virtual void LoadSettings(bool bForceReload = false) override;
	virtual void ValidateSettings() override;
	virtual void SetToDefaults() override;
	void SetWindowedResolutionIntent(FIntPoint Resolution);
	FIntPoint GetRememberedWindowedResolution() const;
private:
	friend class FFlickDisplaySettingsTest;
	void EnforceBorderless();
	UPROPERTY(config)
	int32 WindowedResolutionX = 1280;
	UPROPERTY(config)
	int32 WindowedResolutionY = 720;
	EWindowMode::Type LastValidatedMode = EWindowMode::WindowedFullscreen;
};
