#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameMode.h"
#include "GameMapsSettings.h"
#include "Online/FlickSessionSubsystem.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "UI/FlickGameLayer.h"
#include "UI/FlickHUD.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "TimerManager.h"

namespace
{
	bool ContainsLabel(const TSharedRef<SWidget>& Widget, const FString& Label)
	{
		if (Widget->GetTypeAsString() == TEXT("STextBlock")
			&& StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Label) return true;
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
			if (ContainsLabel(Children->GetChildAt(Index), Label)) return true;
		return false;
	}

	TSharedPtr<SButton> FindVisibleButton(const TSharedRef<SWidget>& Widget, const FString& Label)
	{
		if (!Widget->GetVisibility().IsVisible()) return nullptr;
		if (Widget->GetTypeAsString() == TEXT("SButton") && ContainsLabel(Widget, Label))
			return StaticCastSharedRef<SButton>(Widget);
		FChildren* Children = Widget->GetChildren();
		for (int32 Index = 0; Index < Children->Num(); ++Index)
			if (auto Button = FindVisibleButton(Children->GetChildAt(Index), Label)) return Button;
		return nullptr;
	}

	bool ActivateButton(const TSharedRef<SWidget>& Root, const FString& Label)
	{
		const auto Button = FindVisibleButton(Root, Label);
		if (!Button.IsValid() || !Button->IsEnabled()) return false;
		// Exercise the real Slate button callback, without moving the OS cursor
		// or taking focus from the developer's desktop during offscreen tests.
		const FKeyEvent Enter(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0);
		const bool bPressed = Button->OnKeyDown(Button->GetCachedGeometry(), Enter).IsEventHandled();
		return Button->OnKeyUp(Button->GetCachedGeometry(), Enter).IsEventHandled() && bPressed;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPackagingConfigurationTest, "FLICK.Frontend.PackagingConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPackagingConfigurationTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Runtime default is the empty bootstrap"), UGameMapsSettings::GetGameDefaultMap(), FString(TEXT("/Engine/Maps/Entry")));
	FString Command;
	if (TestTrue(TEXT("Packaging command exists"), FFileHelper::LoadFileToString(Command,
		*(FPaths::ProjectDir() / TEXT("package-flick-development.cmd")))))
	{
		TestTrue(TEXT("Cooks the startup map"), Command.Contains(TEXT("-map=/Engine/Maps/Entry")));
		TestFalse(TEXT("Does not skip the editor module needed for cooking"), Command.Contains(TEXT("-nocompileeditor")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPackagedStartupTest, "FLICK.Frontend.PackagedStartup",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FFlickPackagedStartupTest::RunTest(const FString& Parameters)
{
	// Run against the real game world and normal intro, never a synthetic test
	// world, preview mode, or FlickSkipIntro. Also usable via UnrealEditor -game.
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this]()
	{
		UWorld* World = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
			if (Context.WorldType == EWorldType::Game && Context.World()) { World = Context.World(); break; }
		if (!TestNotNull(TEXT("Running game world"), World)) return;
		auto* Mode = World->GetAuthGameMode<AFlickGameMode>();
		auto* Instance = Cast<UFlickGameInstance>(World->GetGameInstance());
		auto* Controller = Cast<AFlickPlayerController>(World->GetFirstPlayerController());
		auto* Hud = Controller ? Cast<AFlickHUD>(Controller->GetHUD()) : nullptr;
		if (!TestNotNull(TEXT("Frontend mode"), Mode) || !TestNotNull(TEXT("Game instance"), Instance)
			|| !TestNotNull(TEXT("Local controller"), Controller) || !TestNotNull(TEXT("Frontend HUD"), Hud)) return;
		if (!TestTrue(TEXT("Slate game layer exists"), Hud->GameLayer.IsValid())) return;
		const auto Layer = Hud->GameLayer.ToSharedRef();
		TestEqual(TEXT("Loads configured startup map"), World->GetOutermost()->GetName(), UGameMapsSettings::GetGameDefaultMap());
		TestEqual(TEXT("Empty bootstrap map, no template landscape"), UGameMapsSettings::GetGameDefaultMap(), FString(TEXT("/Engine/Maps/Entry")));
		TestFalse(TEXT("Intro finished without skipping it"), Instance->ShouldShowStartupPresentation());
		TestFalse(TEXT("No startup overlay blocking menu input"), Layer->bStartupOverlayVisible);
		TestFalse(TEXT("World keeps ticking"), World->IsPaused());
		TestTrue(TEXT("Main menu is active"), Mode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu);
		TestTrue(TEXT("Cursor enabled"), Controller->bShowMouseCursor);
		TestEqual(TEXT("Only the solo cosmetic showcase puck, not a match formation"), Mode->Pieces.Num(), 1);
		if (Mode->Pieces.Num() == 1)
		{
			const auto* Sessions = Instance->GetSubsystem<UFlickSessionSubsystem>();
			const auto Archetype = Sessions->GetShowcaseArchetype();
			TestTrue(TEXT("Shows equipped puck type"), Mode->Pieces[0]->GetArchetype() == Archetype);
			TestTrue(TEXT("High-detail visuals present"), Mode->Pieces[0]->HasTestArenaVisuals());
			TestEqual(TEXT("Shows equipped puck skin"), Mode->Pieces[0]->GetPuckSkin(),
				FlickCosmeticCatalog::LoadPuckSkins()[static_cast<int32>(Archetype)]);
			int32 ExpectedPuck = INDEX_NONE;
			if (FParse::Value(FCommandLine::Get(), TEXT("FlickExpectedShowcase="), ExpectedPuck))
				TestEqual(TEXT("Saved showcase preference loaded"), static_cast<int32>(Archetype), ExpectedPuck);
		}
		TestTrue(TEXT("Settings button accepts input"), ActivateButton(Layer, TEXT("SETTINGS")));
		TestTrue(TEXT("Settings opens"), Mode->GetFrontendScreen() == EFlickFrontendScreen::Settings);
		Mode->CloseSettings();
		TestTrue(TEXT("Play button accepts input"), ActivateButton(Layer, TEXT("PLAY")));
		TestTrue(TEXT("Play menu opens"), Mode->GetFrontendScreen() == EFlickFrontendScreen::ModeSelect);
		Mode->CloseModeSelect();
		Mode->UpdateMainMenuPresentation();
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("PackagedStartup.png"), true, false);
		UE_LOG(LogFlick, Log, TEXT("PACKAGED_STARTUP_SMOKE_TEST %s"), HasAnyErrors() ? TEXT("FAIL") : TEXT("PASS"));
		if (FParse::Param(FCommandLine::Get(), TEXT("FlickStartupSmokeQuit")))
		{
			FTimerHandle QuitTimer;
			World->GetTimerManager().SetTimer(QuitTimer, [Layer]()
			{
				UE_LOG(LogFlick, Log, TEXT("PACKAGED_STARTUP_QUIT_BUTTON %s"),
					ActivateButton(Layer, TEXT("QUIT")) ? TEXT("PASS") : TEXT("FAIL"));
			}, 2.0f, false);
		}
	}, 2.0f));
	return true;
}
#endif
