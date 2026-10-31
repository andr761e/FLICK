#include "Game/FlickGameUserSettings.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"

void UFlickGameUserSettings::EnforceBorderless()
{
	if (FullscreenMode == 0) FullscreenMode = 1;
	if (LastConfirmedFullscreenMode == 0) LastConfirmedFullscreenMode = 1;
	PreferredFullscreenMode = 1;
	if (auto* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("r.FullScreenMode")))
		Mode->Set(1, ECVF_SetByGameSetting);
}

void UFlickGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	WindowedResolutionX = 1280; WindowedResolutionY = 720;
	LastValidatedMode = EWindowMode::WindowedFullscreen;
	EnforceBorderless();
}

void UFlickGameUserSettings::LoadSettings(bool bForceReload)
{
	Super::LoadSettings(bForceReload);
	EnforceBorderless();
	LastValidatedMode = GetFullscreenMode();
	if (LastValidatedMode == EWindowMode::Windowed) SetWindowedResolutionIntent(GetScreenResolution());
}

void UFlickGameUserSettings::SetWindowedResolutionIntent(FIntPoint Resolution)
{
	if (Resolution.X > 0 && Resolution.Y > 0)
	{
		WindowedResolutionX = Resolution.X;
		WindowedResolutionY = Resolution.Y;
	}
}

FIntPoint UFlickGameUserSettings::GetRememberedWindowedResolution() const
{
	return WindowedResolutionX > 0 && WindowedResolutionY > 0
		? FIntPoint(WindowedResolutionX, WindowedResolutionY) : FIntPoint(1280, 720);
}

void UFlickGameUserSettings::ValidateSettings()
{
	Super::ValidateSettings();
	EnforceBorderless();
	const EWindowMode::Type Mode = GetFullscreenMode();
	if (Mode == EWindowMode::Windowed)
	{
		// Unreal's hotkey toggle otherwise carries the desktop-sized fullscreen
		// resolution back into a decorated window. Restore our last window size.
		if (LastValidatedMode != EWindowMode::Windowed)
		{
			SetScreenResolution(GetRememberedWindowedResolution());
			// Alt+Enter confirms its desktop-sized request before ApplySettings.
			// Keep that confirmed size in sync with the window we actually restore.
			if (LastConfirmedFullscreenMode == 2)
			{
				LastUserConfirmedResolutionSizeX = ResolutionSizeX;
				LastUserConfirmedResolutionSizeY = ResolutionSizeY;
			}
		}
		SetWindowedResolutionIntent(GetScreenResolution());
	}
	else if (LastValidatedMode == EWindowMode::Windowed)
	{
		// Include manual window resizing, not only changes made in the menu.
		if (GEngine && GEngine->GetGameUserSettings() == this && GEngine->GameViewport
			&& GEngine->GameViewport->Viewport && !GEngine->GameViewport->Viewport->IsFullscreen())
			SetWindowedResolutionIntent(GEngine->GameViewport->Viewport->GetSizeXY());
	}
	LastValidatedMode = Mode;
}
