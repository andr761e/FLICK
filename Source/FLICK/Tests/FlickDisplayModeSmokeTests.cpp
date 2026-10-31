#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Game/FlickGameUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// This test deliberately resizes the real game window. It is opt-in and can only
// run against an isolated profile, never during the ordinary UI automation suite.
class FFlickDisplayRoundTrip final : public IAutomationLatentCommand
{
public:
	explicit FFlickDisplayRoundTrip(FAutomationTestBase* InTest) : Test(InTest) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds() < NextStepAt) return false;
		auto* Settings = GEngine ? Cast<UFlickGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
		auto* Viewport = GEngine && GEngine->GameViewport ? GEngine->GameViewport->Viewport : nullptr;
		auto* World = GEngine && GEngine->GameViewport ? GEngine->GameViewport->GetWorld() : nullptr;
		auto* Controller = World ? World->GetFirstPlayerController() : nullptr;
		if (!Settings || !Viewport || !Controller)
		{
			Test->AddError(TEXT("Display smoke test requires a running game and FLICK settings"));
			return true;
		}
		const FIntPoint Desktop = Settings->GetDesktopResolution();
		if (Step == 0)
		{
			InitialDesktop = Desktop;
			Test->TestTrue(TEXT("Legacy exclusive profile starts borderless"), Viewport->GetWindowMode() == EWindowMode::WindowedFullscreen);
			Test->TestEqual(TEXT("Legacy frame limit survives migration"), FMath::RoundToInt(Settings->GetFrameRateLimit()), 60);
		}
		Test->TestEqual(TEXT("Desktop resolution remains unchanged"), Desktop, InitialDesktop);
		Test->TestTrue(TEXT("Exclusive fullscreen is never active"), Viewport->GetWindowMode() != EWindowMode::Fullscreen);
		if (Step == 1 || Step == 3 || Step == 4 || Step == 6)
		{
			const FIntPoint Expected = Step >= 4 ? FIntPoint(1440, 900) : FIntPoint(1280, 720);
			Test->TestTrue(TEXT("Returned to a decorated window"), Viewport->GetWindowMode() == EWindowMode::Windowed);
			Test->TestEqual(TEXT("Actual client size restores correctly"), Viewport->GetSizeXY(), Expected);
		}
		if (Step == 2 || Step == 5)
			Test->TestTrue(TEXT("Fullscreen uses a borderless window"), Viewport->GetWindowMode() == EWindowMode::WindowedFullscreen);
		UE_LOG(LogTemp, Display, TEXT("DISPLAY_MODE_SMOKE_STEP %d: mode=%d client=%dx%d desktop=%dx%d"),
			Step, int32(Viewport->GetWindowMode()), Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Desktop.X, Desktop.Y);
		auto ApplyWindowed = [Settings](FIntPoint Size)
		{
			Settings->SetWindowedResolutionIntent(Size);
			Settings->SetFullscreenMode(EWindowMode::Windowed);
			Settings->SetScreenResolution(Size);
			Settings->ApplySettings(false);
			Settings->ConfirmVideoMode();
		};
		switch (Step++)
		{
		case 0: ApplyWindowed(FIntPoint(1280, 720)); break;
		case 1: case 2: Controller->ConsoleCommand(TEXT("TOGGLE_FULLSCREEN")); break;
		case 3:
			Test->TestEqual(TEXT("Hotkey confirms the restored size"), Settings->GetLastConfirmedScreenResolution(), FIntPoint(1280, 720));
			ApplyWindowed(FIntPoint(1440, 900)); break;
		case 4:
			Settings->SetFullscreenMode(EWindowMode::Fullscreen);
			Settings->SetScreenResolution(FIntPoint(1280, 720));
			Settings->ApplySettings(false); Settings->ConfirmVideoMode(); break;
		case 5:
			Settings->SetFullscreenMode(EWindowMode::Windowed);
			Settings->SetScreenResolution(Desktop);
			Settings->ApplySettings(false); Settings->ConfirmVideoMode(); break;
		default:
			if (!Test->HasAnyErrors()) UE_LOG(LogTemp, Display, TEXT("DISPLAY_MODE_SMOKE_PASS"));
			return true;
		}
		NextStepAt = FPlatformTime::Seconds() + 1.0;
		return false;
	}
private:
	FAutomationTestBase* Test;
	int32 Step = 0;
	double NextStepAt = 0;
	FIntPoint InitialDesktop = FIntPoint::ZeroValue;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickDisplayModeSmokeTest, "FLICK.UI.DisplayModeTransitions",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FFlickDisplayModeSmokeTest::RunTest(const FString&)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("FlickDisplayModeSmoke")))
	{
		AddInfo(TEXT("Opt-in native display test; use Tools/Test-FlickDisplayModes.ps1"));
		return true;
	}
	FString UserDir;
	if (!FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir)
		|| !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(UserDir), FPaths::ProjectDir() / TEXT("Saved/DisplayModeQA")))
	{
		AddError(TEXT("Display mode smoke test requires an isolated Saved/DisplayModeQA profile"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FFlickDisplayRoundTrip(this));
	return true;
}
#endif
