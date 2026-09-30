#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerState.h"
#include "Player/FlickPlayerController.h"
#include "UI/FlickPuckHoverWidget.h"
#include "Core/FlickVisualSettings.h"
#include "UI/FlickGameLayer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPuckInspectionTest, "FLICK.Cosmetics.InspectionAndOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPuckInspectionTest::RunTest(const FString& Parameters)
{
	const int32 SavedHoverSize = FlickVisualSettings::GetPuckHoverSize();
	const int32 SavedHoverDetail = FlickVisualSettings::GetPuckHoverDetail();
	const bool bSavedColorBlindAssist = FlickVisualSettings::IsColorBlindAssistEnabled();
	FlickVisualSettings::SetColorBlindAssistEnabled(false);
	FlickVisualSettings::SetPuckHoverDetail(3);
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Inspection world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickGameState* GameState = World->SpawnActor<AFlickGameState>();
	World->SetGameState(GameState);
	GameState->MatchPhase = EFlickMatchPhase::Aiming;
	GameState->CurrentTeam = EFlickTeam::Player2;
	AFlickPlayerState* Player = World->SpawnActor<AFlickPlayerState>();
	Player->SetTeam(EFlickTeam::Player1);
	Player->SetTeamPlayerSlot(0);
	AFlickPlayerController* Controller = World->SpawnActor<AFlickPlayerController>();
	Controller->SetAsLocalPlayerController();
	// This isolated world has not begun play, so register the viewport controller explicitly.
	World->AddController(Controller);
	TestTrue(TEXT("Fixture has a local viewing controller"), World->GetFirstPlayerController() == Controller && Controller->IsLocalController());
	Controller->PlayerState = Player;
	AFlickPiece* Own = World->SpawnActor<AFlickPiece>();
	AFlickPiece* Other = World->SpawnActor<AFlickPiece>();
	AFlickPiece* Teammate = World->SpawnActor<AFlickPiece>();
	Own->InitializePiece(EFlickTeam::Player1, 1, 45, 20, EFlickPieceArchetype::Standard);
	Other->InitializePiece(EFlickTeam::Player2, 2, 45, 20, EFlickPieceArchetype::Heavy);
	Teammate->InitializePiece(EFlickTeam::Player1, 3, 45, 20, EFlickPieceArchetype::Standard, false, 1);
	const auto RingColour = [](AFlickPiece* Piece)
	{
		Piece->Tick(0.016f);
		for (UStaticMeshComponent* Component : TInlineComponentArray<UStaticMeshComponent*>(Piece))
			if (Component->GetFName() == TEXT("SelectionHalo"))
				if (auto* Material = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0)))
					return Material->K2_GetVectorParameterValue(TEXT("Color"));
		return FLinearColor::Black;
	};
	for (int32 TeamSize : {2, 3})
	{
		GameState->PlayersPerTeam = TeamSize;
		TestTrue(TEXT("Own puck identified regardless of turn"), Controller->OwnsPieceLocally(Own));
		TestFalse(TEXT("Teammate is not locally owned"), Controller->OwnsPieceLocally(Teammate));
		TestFalse(TEXT("Opponent is not locally owned"), Controller->OwnsPieceLocally(Other));
		RingColour(Own);
		const FLinearColor TeammateColour = RingColour(Teammate);
		TestFalse(TEXT("Own puck has no ownership ring"), Own->SelectionHalo->IsVisible());
		TestTrue(TEXT("Teammate retains blue team ring"), TeammateColour.B > TeammateColour.G);
	}
	GameState->PlayersPerTeam = 1;
	RingColour(Own);
	TestFalse(TEXT("1v1 also hides the local ownership ring"), Own->SelectionHalo->IsVisible());
	FlickVisualSettings::SetColorBlindAssistEnabled(true);
	RingColour(Teammate);
	RingColour(Other);
	TestTrue(TEXT("Assistance distinguishes enemy ring by width"),
		Other->SelectionHalo->GetRelativeScale3D().X
		> Teammate->SelectionHalo->GetRelativeScale3D().X + 0.06f);
	FlickVisualSettings::SetColorBlindAssistEnabled(false);
	GameState->PlayersPerTeam = 3;
	Player->SetTeamPlayerSlot(1);
	TestTrue(TEXT("Ownership follows reassigned player slot"), Controller->OwnsPieceLocally(Teammate));
	TestFalse(TEXT("Previous slot no longer locally owned"), Controller->OwnsPieceLocally(Own));
	RingColour(Teammate);
	TestFalse(TEXT("Ring removal follows reassignment on the next frame"), Teammate->SelectionHalo->IsVisible());
	Player->SetTeamPlayerSlot(0);

	Controller->ApplyHoveredPiece(Other);
	TestTrue(TEXT("Opponent can be inspected during their turn"), Controller->GetInspectedPiece() == Other);
	TestNull(TEXT("Opponent cannot be hover-highlighted"), Controller->HoveredPiece.Get());
	TestFalse(TEXT("Opponent cannot be aimed from"), Controller->CanSelectPieceLocally(Other));
	TestFalse(TEXT("Inspection does not start aiming"), Controller->IsAimingShot());

	auto Widget = SNew(SFlickPuckHoverWidget).Controller(Controller)
		.OwnerName([](const AFlickPiece*) { return FString(TEXT("Test player")); });
	Widget->Tick(FGeometry(), 0, .016f);
	TestEqual(TEXT("Exact hover format uses the player's name and type"), Widget->GetLabelText(Other), FString(TEXT("Test player - Heavy")));
	FlickVisualSettings::SetColorBlindAssistEnabled(true);
	TestEqual(TEXT("Assistance spells out opponent relationship"), Widget->GetLabelText(Other), FString(TEXT("OPPONENT | Test player - Heavy")));
	TestEqual(TEXT("Assistance spells out local ownership"), Widget->GetLabelText(Own), FString(TEXT("YOU | Test player - Standard")));
	TestEqual(TEXT("Assistance spells out teammate relationship"), Widget->GetLabelText(Teammate), FString(TEXT("TEAMMATE | Test player - Standard")));
	FlickVisualSettings::SetColorBlindAssistEnabled(false);
	FlickVisualSettings::SetPuckHoverDetail(1);
	TestEqual(TEXT("Name-only preference"), Widget->GetLabelText(Other), FString(TEXT("Test player")));
	FlickVisualSettings::SetPuckHoverDetail(2);
	TestEqual(TEXT("Type-only preference"), Widget->GetLabelText(Other), FString(TEXT("Heavy")));
	FlickVisualSettings::SetPuckHoverDetail(0);
	TestTrue(TEXT("Off hides hover text"), Widget->GetLabelText(Other).IsEmpty());
	FlickVisualSettings::SetPuckHoverSize(1);
	TestEqual(TEXT("Hover size clamps at readable minimum"), FlickVisualSettings::GetPuckHoverSize(), 10);
	FlickVisualSettings::SetPuckHoverSize(100);
	TestEqual(TEXT("Hover size clamps at maximum"), FlickVisualSettings::GetPuckHoverSize(), 18);
	FlickVisualSettings::SetPuckHoverSize(SavedHoverSize);
	FlickVisualSettings::SetPuckHoverDetail(SavedHoverDetail);
	FlickVisualSettings::SetColorBlindAssistEnabled(bSavedColorBlindAssist);
	TestTrue(TEXT("Hover appears in the first frame, without a dwell timer"), Widget->Alpha > 0);
	Widget->Tick(FGeometry(), 0, .04f);
	TestEqual(TEXT("Short entrance fade completes within 56 ms"), Widget->Alpha, 1.0f);
	Controller->ApplyHoveredPiece(Teammate);
	Widget->Tick(FGeometry(), 0, .001f);
	TestTrue(TEXT("Fast switch updates the label target in the same frame"), Widget->LastPiece.Get() == Teammate);
	TestEqual(TEXT("Switching pucks does not fade out or leave an old label"), Widget->Alpha, 1.0f);
	TestNull(TEXT("Teammate cannot be hover-highlighted"), Controller->HoveredPiece.Get());

	GameState->CurrentTeam = EFlickTeam::Player1;
	Controller->ApplyHoveredPiece(Own);
	TestTrue(TEXT("Own puck remains selectable on own turn"), Controller->CanSelectPieceLocally(Own));
	TestTrue(TEXT("Own selectable puck retains its normal hover"), Controller->HoveredPiece == Own);
	GameState->MatchPhase = EFlickMatchPhase::KickoffPlanning;
	Controller->ApplyHoveredPiece(Other);
	TestNull(TEXT("Kickoff inspection cannot reveal an opponent highlight"), Controller->HoveredPiece.Get());
	TestFalse(TEXT("Kickoff cannot select an opponent"), Controller->CanSelectPieceLocally(Other));
	Controller->bCinematicReplayPresentationActive = true;
	Controller->UpdateHoveredPiece();
	TestNull(TEXT("Cinematic replay cannot show gameplay hover labels"), Controller->GetInspectedPiece());
	Controller->bCinematicReplayPresentationActive = false;
	Controller->ApplyHoveredPiece(nullptr);
	Widget->Tick(FGeometry(), 0, .001f);
	TestEqual(TEXT("Leaving puck removes label immediately"), Widget->Alpha, 0.0f);

	TArray<int32> Skins;
	Skins.Init(1, FlickPieceArchetypeRules::ArchetypeCount);
	Controller->ServerSetPuckSkins_Implementation(Skins);
	TestEqual(TEXT("Owner's choice updates own puck"), Own->GetPuckSkin(), 1);
	TestEqual(TEXT("Choice cannot change opponent cosmetics"), Other->GetPuckSkin(), 0);
	TestEqual(TEXT("Choice cannot change teammate cosmetics"), Teammate->GetPuckSkin(), 0);
	TArray<int32> Effects = {1, 2, 1};
	Controller->ServerSetPuckEffects_Implementation(Effects);
	TestEqual(TEXT("Owner's trail updates own puck"), Own->GetPuckEffect(0), 1);
	TestEqual(TEXT("Owner's arrival updates own puck"), Own->GetPuckEffect(1), 2);
	TestEqual(TEXT("Owner's knockout updates own puck"), Own->GetPuckEffect(2), 1);
	TestEqual(TEXT("Opponent keeps default effects"), Other->GetPuckEffect(0), 0);
	TestEqual(TEXT("Teammate keeps default effects"), Teammate->GetPuckEffect(0), 0);
	Effects[1] = 99;
	TestFalse(TEXT("Unknown effect IDs are rejected"), Player->SetPuckEffects(Effects));
	TestEqual(TEXT("Invalid update preserves accepted effects"), Player->GetPuckEffect(1), 2);
	Effects.Reset();
	TestFalse(TEXT("Incomplete effect selections are rejected"), Player->SetPuckEffects(Effects));
	Skins[0] = 99;
	TestFalse(TEXT("Unknown skin IDs rejected"), Player->SetPuckSkins(Skins));
	TestEqual(TEXT("Invalid update preserves accepted appearance"), Player->GetPuckSkin(EFlickPieceArchetype::Standard), 1);
	Skins.Reset();
	TestFalse(TEXT("Incomplete skin collections rejected"), Player->SetPuckSkins(Skins));
	Player->SetPrivateControlledSlots({EncodePrivatePlayerSlot(EFlickTeam::Player2, 0)});
	Skins.Init(1, FlickPieceArchetypeRules::ArchetypeCount);
	Controller->ServerSetPuckSkins_Implementation(Skins);
	TestEqual(TEXT("Private controlled seat receives owner's skin"), Other->GetPuckSkin(), 1);
	Effects = {2, 1, 2};
	Controller->ServerSetPuckEffects_Implementation(Effects);
	TestEqual(TEXT("Private controlled seat receives owner's trail"), Other->GetPuckEffect(0), 2);
	Own->SetPuckSkin(0);
	TestEqual(TEXT("Switching orange back to blue is supported"), Own->GetPuckSkin(), 0);
	TestTrue(TEXT("Changing cosmetics never changes ownership"), Own->GetTeam() == EFlickTeam::Player1);
	// Simulate a remote private-match client: it has a GameState but no GameMode.
	auto Layer = MakeShared<SFlickGameLayer>();
	Layer->PlayerController = Controller;
	GameState->bPrivateMatchActive = true;
	TestTrue(TEXT("Private owned seat is identified across teams"), Controller->OwnsPieceLocally(Other));
	TestFalse(TEXT("Private ownership does not fall back to team/slot"), Controller->OwnsPieceLocally(Own));
	GameState->MatchPhase = EFlickMatchPhase::KickoffPlanning;
	TestTrue(TEXT("Remote gameplay retains its HUD"), Layer->GetMatchHudVisibility() != EVisibility::Collapsed);
	GameState->bPrivateMatchAssignmentActive = true;
	TestTrue(TEXT("Private class selection suppresses gameplay HUD"), Layer->GetMatchHudVisibility() == EVisibility::Collapsed);
	TestFalse(TEXT("Private class selection cannot overlap control overview"), Layer->ShouldShowGameplayControls());
	GameState->bPrivateMatchAssignmentActive = false;
	GameState->bNetworkClassSelectionActive = true;
	TestFalse(TEXT("Network class selection suppresses controls"), Layer->ShouldShowGameplayControls());
	GameState->bNetworkClassSelectionActive = false;
	Layer->bSocialPanelOpen = true;
	TestFalse(TEXT("Social menu suppresses controls"), Layer->ShouldShowGameplayControls());
	Layer->bSocialPanelOpen = false;
	Layer->bChallengesOpen = true;
	TestFalse(TEXT("Challenges menu suppresses controls"), Layer->ShouldShowGameplayControls());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
