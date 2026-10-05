#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Player/FlickTeamPingComponent.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickTeamPingTest, "FLICK.Match.TeamPings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickTeamPingTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!World) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
	State->MatchPhase = EFlickMatchPhase::Aiming; State->MatchId = TEXT("ping-test");
	TArray<AFlickPlayerController*> Controllers;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		auto* Controller = World->SpawnActor<AFlickPlayerController>();
		Controller->SetAsLocalPlayerController(); World->AddController(Controller);
		auto* Player = World->SpawnActor<AFlickPlayerState>(); Controller->PlayerState = Player;
		Player->SetTeam(Index < 2 ? EFlickTeam::Player1 : Index == 2 ? EFlickTeam::Player2 : EFlickTeam::None);
		Player->SetPlayerName(FString::Printf(TEXT("Player%d"), Index)); Controllers.Add(Controller);
	}
	auto* Feed = Controllers[0]->FindComponentByClass<UFlickTeamPingComponent>();
	auto* Mate = Controllers[1]->FindComponentByClass<UFlickTeamPingComponent>();
	auto* Enemy = Controllers[2]->FindComponentByClass<UFlickTeamPingComponent>();
	auto* Spectator = Controllers[3]->FindComponentByClass<UFlickTeamPingComponent>();
	auto* Target = World->SpawnActor<AFlickPiece>();
	Target->InitializePiece(EFlickTeam::Player2, 10, 45, 20, EFlickPieceArchetype::Heavy);
	Feed->ServerPingEnemy_Implementation(10);
	TestTrue(TEXT("Server formats target puck type"), Feed->GetMessage(0).Contains(TEXT("Heavy")));
	// Even an incorrectly delivered message is rejected by opposing clients.
	Enemy->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, TEXT("Target"));
	Spectator->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, TEXT("Target"));
	TestTrue(TEXT("Sender sees ping"), Feed->HasMessages()); TestTrue(TEXT("Teammate sees ping"), Mate->HasMessages());
	TestFalse(TEXT("Enemy cannot receive team feed"), Enemy->HasMessages()); TestFalse(TEXT("Spectator cannot receive team feed"), Spectator->HasMessages());
	const int32 Count = Feed->Messages.Num();
	Feed->ServerPingEnemy_Implementation(10);
	TestEqual(TEXT("Server cooldown stops duplicate enemy pings"), Feed->Messages.Num(), Count);
	auto* Friendly = World->SpawnActor<AFlickPiece>();
	Friendly->InitializePiece(EFlickTeam::Player1, 11, 45, 20, EFlickPieceArchetype::Standard);
	Feed->LastServerPing -= 3;
	Feed->ServerPingEnemy_Implementation(11);
	TestEqual(TEXT("Friendly puck rejected"), Feed->Messages.Num(), Count);
	Target->Eliminate(); Feed->LastServerPing -= 3;
	Feed->ServerPingEnemy_Implementation(10);
	TestEqual(TEXT("Eliminated puck rejected"), Feed->Messages.Num(), Count);
	Feed->Messages[0].ReceivedAt -= 6;
	TestTrue(TEXT("Message fades before expiry"), Feed->GetOpacity(0) > 0 && Feed->GetOpacity(0) < 1);
	Feed->Messages[0].ReceivedAt -= 2;
	TestFalse(TEXT("Expired message disappears"), Feed->HasMessages());
	Feed->ServerPingEnemy_Implementation(-123);
	const double AcceptedTime = Feed->LastServerPing;
	Feed->ServerPingEnemy_Implementation(-124);
	TestEqual(TEXT("Server cooldown prevents repeated requests"), Feed->LastServerPing, AcceptedTime);
	State->MatchId = TEXT("new-match"); TestFalse(TEXT("Old match feed cleared from view"), Mate->HasMessages());
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif
