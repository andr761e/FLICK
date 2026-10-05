#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPrivateSpectatorComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPrivateSpectatorTest, "FLICK.Match.PrivateSpectatorView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPrivateSpectatorTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Spectator world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* State = World->SpawnActor<AFlickGameState>();
	World->SetGameState(State);
	State->bPrivateMatchActive = true;
	State->PlayersPerTeam = 3;
	State->MatchPhase = EFlickMatchPhase::Aiming;
	auto* Controller = World->SpawnActor<AFlickPlayerController>();
	Controller->SetAsLocalPlayerController();
	World->AddController(Controller);
	auto* Local = World->SpawnActor<AFlickPlayerState>();
	Controller->PlayerState = Local;
	Local->SetPrivateRoleChosen(true);
	Local->SetTeam(EFlickTeam::None);
	auto* Camera = World->SpawnActor<AFlickCameraPawn>();
	Controller->Possess(Camera);
	auto* Spectator = Controller->FindComponentByClass<UFlickPrivateSpectatorComponent>();
	TestNotNull(TEXT("Default spectator component"), Spectator);
	if (!Spectator) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false; }
	auto* Human = World->SpawnActor<AFlickPlayerState>();
	State->AddPlayerState(Human);
	Human->SetTeam(EFlickTeam::Player1);
	Human->SetTeamPlayerSlot(0);
	Human->SetPrivateControlledSlots({0});
	Human->SetPlayerName(TEXT("Camera Tester"));

	TestTrue(TEXT("Chosen private spectator recognized"), Spectator->IsSpectator());
	Controller->CyclePrivateSpectatorPlayer(1);
	TestEqual(TEXT("First seat blue"), Spectator->GetTargetTeam(), EFlickTeam::Player1);
	TestEqual(TEXT("Human card name"), Spectator->GetTargetName(), FString(TEXT("Camera Tester")));
	FFlickSpectatorView View;
	View.Location = FVector(350, -1100, 950);
	View.Rotation = FRotator(-31, 71, 0);
	View.FieldOfView = 62;
	TestTrue(TEXT("Human camera accepted in private gameplay"), Human->SetPrivateCameraView(View));
	TestFalse(TEXT("Camera updates rate limited"), Human->SetPrivateCameraView(View));
	TestFalse(TEXT("Spectators cannot publish cameras"), Local->SetPrivateCameraView(View));
	Spectator->TickComponent(.1f, LEVELTICK_All, nullptr);
	Camera->Tick(.5f);
	const auto Mirrored = Camera->CaptureSpectatorView();
	TestTrue(TEXT("Follows actual human camera location"), FVector(Mirrored.Location).Equals(View.Location, 1));
	TestTrue(TEXT("Follows actual human camera rotation"), Mirrored.Rotation.Equals(View.Rotation, .1));
	TestTrue(TEXT("Follows human zoom"), FMath::IsNearlyEqual(Mirrored.FieldOfView, View.FieldOfView, .1f));
	State->RemovePlayerState(Human);
	Spectator->TickComponent(.1f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Departed player falls back to bot in same seat"), Spectator->GetTargetName(), FString(TEXT("BLUE BOT 1")));
	State->AddPlayerState(Human);
	Spectator->TickComponent(.1f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Reoccupied seat follows its human again"), Spectator->GetTargetName(), FString(TEXT("Camera Tester")));

	Controller->CyclePrivateSpectatorPlayer(-1);
	TestEqual(TEXT("Previous wraps across both teams"), Spectator->GetTargetTeam(), EFlickTeam::Player2);
	TestEqual(TEXT("Wrap includes last bot"), Spectator->GetTargetSlot(), 2);
	TestEqual(TEXT("Bot card name"), Spectator->GetTargetName(), FString(TEXT("ORANGE BOT 3")));
	Spectator->TickComponent(.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Bot retains follow mode"), Spectator->IsFollowing());
	TestTrue(TEXT("Bot uses orange team viewpoint"), FMath::Abs(FRotator::NormalizeAxis(Camera->GetGameplayOrbitAngle() - 180)) < .1f);
	Controller->CyclePrivateSpectatorPlayer(1);
	Controller->TogglePrivateSpectatorFreeCamera();
	TestTrue(TEXT("Free camera preserved"), Camera->IsFreeCameraEnabled());
	TestFalse(TEXT("Free camera stops follow"), Spectator->IsFollowing());
	Controller->TogglePrivateSpectatorFreeCamera();
	TestTrue(TEXT("Exiting free camera resumes follow"), Spectator->IsFollowing());
	TestEqual(TEXT("Resumes same target"), Spectator->GetTargetName(), FString(TEXT("Camera Tester")));
	Camera->BeginCinematicReplay(FVector(0, 0, 250), EFlickTeam::Player1);
	Camera->Tick(.5f);
	TestFalse(TEXT("Replay camera takes priority over follow"), FVector(Camera->CaptureSpectatorView().Location).Equals(View.Location, 1));
	Camera->EndCinematicReplay();
	Spectator->TickComponent(.1f, LEVELTICK_All, nullptr);
	Camera->Tick(.5f);
	TestTrue(TEXT("Follow resumes after replay"), FVector(Camera->CaptureSpectatorView().Location).Equals(View.Location, 1));

	View.FieldOfView = NAN;
	TestFalse(TEXT("Nonfinite camera rejected"), View.IsSafe());
	View.FieldOfView = 300;
	TestFalse(TEXT("Invalid camera zoom rejected"), View.IsSafe());
	View.FieldOfView = 62;
	View.Location = FVector(999999, 0, 0);
	TestFalse(TEXT("Out-of-bounds camera rejected"), View.IsSafe());
	State->PlayersPerTeam = 1;
	Controller->CyclePrivateSpectatorPlayer(1);
	TestEqual(TEXT("1v1 visits only orange seat zero"), Spectator->GetTargetSlot(), 0);
	Controller->CyclePrivateSpectatorPlayer(1);
	TestEqual(TEXT("1v1 wraps to blue zero"), Spectator->GetTargetTeam(), EFlickTeam::Player1);
	State->bSeriesComplete = true;
	State->MatchPhase = EFlickMatchPhase::RoundOver;
	Controller->CyclePrivateSpectatorPlayer(1);
	TestEqual(TEXT("Post-match cannot change target"), Spectator->GetTargetTeam(), EFlickTeam::Player1);
	State->bPrivateMatchActive = false;
	TestFalse(TEXT("Public modes never allow private follow"), Spectator->IsSpectator());
	TestFalse(TEXT("Public camera publishing rejected"), Human->SetPrivateCameraView(Camera->CaptureSpectatorView()));
	Spectator->TickComponent(.1f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Leaving private match clears follow"), Spectator->IsFollowing());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
