#include "UI/FlickGameLayerPrivate.h"
#include "Kismet/KismetSystemLibrary.h"

void SFlickGameLayer::RefreshDisplayOptions()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return;

	ResolutionOptions.Reset();
	TArray<FIntPoint> Detected;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Detected);
	TArray<FIntPoint> Windowed;
	UKismetSystemLibrary::GetConvenientWindowedResolutions(Windowed);
	Detected.Append(Windowed);
	Detected.Add(Settings->GetDesktopResolution());
	Detected.Add(Settings->GetScreenResolution());
	for (const FIntPoint Resolution : Detected)
	{
		if (Resolution.X <= 0 || Resolution.Y <= 0) continue;
		if ((Resolution.X < 1024 || Resolution.Y < 720)
			&& Resolution != Settings->GetScreenResolution()) continue;
		if (!ResolutionOptions.ContainsByPredicate([Resolution](const TSharedPtr<FIntPoint>& Option)
			{ return Option.IsValid() && *Option == Resolution; }))
		{
			ResolutionOptions.Add(MakeShared<FIntPoint>(Resolution));
		}
	}
	if (ResolutionOptions.IsEmpty()) ResolutionOptions.Add(MakeShared<FIntPoint>(FIntPoint(1280, 720)));
	ResolutionOptions.Sort([](const TSharedPtr<FIntPoint>& A, const TSharedPtr<FIntPoint>& B)
	{
		const int64 PixelsA = static_cast<int64>(A->X) * A->Y;
		const int64 PixelsB = static_cast<int64>(B->X) * B->Y;
		return PixelsA == PixelsB ? A->X > B->X : PixelsA > PixelsB;
	});

	WindowModeOptions = {MakeShared<int32>(0), MakeShared<int32>(1), MakeShared<int32>(2)};
	FrameLimitOptions.Reset();
	for (const int32 Limit : {0, 30, 60, 90, 120, 144, 165, 240})
	{
		FrameLimitOptions.Add(MakeShared<int32>(Limit));
	}
	PendingWindowMode = Settings->GetFullscreenMode() == EWindowMode::Fullscreen ? 0
		: Settings->GetFullscreenMode() == EWindowMode::Windowed ? 2 : 1;
	PendingResolution = PendingWindowMode == 1 ? Settings->GetDesktopResolution() : Settings->GetScreenResolution();
	PendingFrameLimit = FMath::RoundToInt(Settings->GetFrameRateLimit());
	if (!FrameLimitOptions.ContainsByPredicate([this](const TSharedPtr<int32>& Option)
		{ return Option.IsValid() && *Option == PendingFrameLimit; }))
	{
		FrameLimitOptions.Add(MakeShared<int32>(PendingFrameLimit));
		FrameLimitOptions.Sort([](const TSharedPtr<int32>& A, const TSharedPtr<int32>& B) { return *A < *B; });
	}
	bPendingVSync = Settings->IsVSyncEnabled();
	bDisplayOptionsInitialized = true;
}

void SFlickGameLayer::ApplyPendingDisplaySettings()
{
	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) return;
	const EWindowMode::Type Mode = PendingWindowMode == 0 ? EWindowMode::Fullscreen
		: PendingWindowMode == 2 ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen;
	Settings->SetFullscreenMode(Mode);
	Settings->SetScreenResolution(Mode == EWindowMode::WindowedFullscreen
		? Settings->GetDesktopResolution() : PendingResolution);
	Settings->SetVSyncEnabled(bPendingVSync);
	Settings->SetFrameRateLimit(static_cast<float>(PendingFrameLimit));
	Settings->ApplySettings(false);
	Settings->ConfirmVideoMode();
	Settings->SaveSettings();
	FlickVisualSettings::ApplySaved();
	RefreshDisplayOptions();
}
