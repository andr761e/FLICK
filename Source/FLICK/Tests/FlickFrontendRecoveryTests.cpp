#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/FlickCosmeticCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickFrontendRecoveryTest, "FLICK.Frontend.PreviewRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickFrontendRecoveryTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Frontend world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
	auto* Mode = World->SpawnActor<AFlickGameMode>(); Mode->GameState = State;
	const auto CheckPreview = [this](AFlickPiece* Piece)
	{
		if (!TestNotNull(TEXT("Preview puck"), Piece)) return;
		TestTrue(TEXT("Preview has high-detail visuals"), Piece->HasTestArenaVisuals());
		Piece->Tick(.016f);
		for (UStaticMeshComponent* Component : TInlineComponentArray<UStaticMeshComponent*>(Piece))
			if (Component->GetName() == TEXT("SelectionHalo") || Component->GetName().StartsWith(TEXT("TeamIndicatorDash")))
				TestFalse(TEXT("Cosmetic preview has no team ring or pattern"), Component->IsVisible());
		for (UTextRenderComponent* Label : TInlineComponentArray<UTextRenderComponent*>(Piece))
			if (Label->GetName() == TEXT("TeamIndicatorLabel")) TestFalse(TEXT("Preview has no team label"), Label->IsVisible());
	};
	for (const EFlickMatchVariant Variant : {EFlickMatchVariant::Classic, EFlickMatchVariant::Bob})
	{
		Mode->SelectedMatchVariant = Variant;
		Mode->ReturnToMainMenu();
		Mode->UpdateMainMenuPresentation();
		if (!Mode->Pieces.IsEmpty()) CheckPreview(Mode->Pieces[0]);
		// Reproduce the stale mode flag from a failed queue without contacting a real service.
		Mode->bTestArenaMode = false;
		Mode->UpdateMainMenuPresentation();
		TestTrue(TEXT("Main menu repairs stale arena mode"), Mode->bTestArenaMode);
		if (!Mode->Pieces.IsEmpty()) CheckPreview(Mode->Pieces[0]);
		Mode->FrontendScreen = EFlickFrontendScreen::Profile;
		Mode->SetLockerPreview(FlickCosmeticCatalog::PuckCategoryStart);
		if (!Mode->Pieces.IsEmpty()) CheckPreview(Mode->Pieces[0]);
		Mode->SetLockerPreview(FlickCosmeticCatalog::KnockoutCategory);
		if (!Mode->Pieces.IsEmpty())
		{
			CheckPreview(Mode->Pieces[0]);
			TestTrue(TEXT("No-ring knockout preview retains physics"),
				Cast<UPrimitiveComponent>(Mode->Pieces[0]->GetRootComponent())->IsSimulatingPhysics());
		}
		Mode->UpdateLockerPreview(3.f);
		if (!Mode->Pieces.IsEmpty()) CheckPreview(Mode->Pieces[0]);
	}
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
