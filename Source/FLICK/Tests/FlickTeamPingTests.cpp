#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Player/FlickTeamPingComponent.h"
#include "Pieces/FlickPiece.h"
#include "Arena/FlickTestArena.h"

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
	Feed->ServerPingTarget_Implementation(10, INDEX_NONE);
	TestTrue(TEXT("Server formats target puck type"), Feed->GetMessage(0).Contains(TEXT("Heavy")));
	// Even an incorrectly delivered message is rejected by opposing clients.
	Enemy->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, TEXT("Target"));
	Spectator->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, TEXT("Target"));
	TestTrue(TEXT("Sender sees ping"), Feed->HasMessages()); TestTrue(TEXT("Teammate sees ping"), Mate->HasMessages());
	TestFalse(TEXT("Enemy cannot receive team feed"), Enemy->HasMessages()); TestFalse(TEXT("Spectator cannot receive team feed"), Spectator->HasMessages());
	const int32 Count = Feed->Messages.Num();
	Feed->ServerPingTarget_Implementation(10, INDEX_NONE);
	TestEqual(TEXT("Server cooldown stops duplicate enemy pings"), Feed->Messages.Num(), Count);
	auto* Friendly = World->SpawnActor<AFlickPiece>();
	Friendly->InitializePiece(EFlickTeam::Player1, 11, 45, 20, EFlickPieceArchetype::Standard);
	Feed->LastServerPing -= 3;
	Feed->ServerPingTarget_Implementation(11, INDEX_NONE);
	TestEqual(TEXT("Friendly puck rejected"), Feed->Messages.Num(), Count);
	Target->Eliminate(); Feed->LastServerPing -= 3;
	Feed->ServerPingTarget_Implementation(10, INDEX_NONE);
	TestEqual(TEXT("Eliminated puck rejected"), Feed->Messages.Num(), Count);
	Feed->Messages[0].ReceivedAt -= 6;
	TestTrue(TEXT("Message fades before expiry"), Feed->GetOpacity(0) > 0 && Feed->GetOpacity(0) < 1);
	Feed->Messages[0].ReceivedAt -= 2;
	TestFalse(TEXT("Expired message disappears"), Feed->HasMessages());
	Feed->ServerPingTarget_Implementation(-123, INDEX_NONE);
	const double AcceptedTime = Feed->LastServerPing;
	Feed->ServerPingTarget_Implementation(-124, INDEX_NONE);
	TestEqual(TEXT("Server cooldown prevents repeated requests"), Feed->LastServerPing, AcceptedTime);

	Feed->Messages.Reset(); Mate->Messages.Reset();
	auto* Arena = World->SpawnActor<AFlickTestArena>();
	Arena->SetActorLocationAndRotation(FVector(100, 200, 0), FRotator(0, 32, 0));
	for (int32 TeamSize = 1; TeamSize <= 3; ++TeamSize)
	{
		Arena->InitializeTestArena(650.f * TeamSize, 50, 250, TeamSize);
		for (int32 Index = 0; Index < Arena->GetMechanismCount(); ++Index)
			TestEqual(TEXT("Switch picking follows each format's transformed layout"),
				Arena->FindSwitchAtWorldLocation(Arena->GetSwitchWorldCenter(Index)), Index);
	}
	TestEqual(TEXT("Empty arena space is not a switch"), Arena->FindSwitchAtWorldLocation(FVector(10000, 10000, 250)), INDEX_NONE);
	const uint16 RaisedBefore = Arena->GetRaisedDividerMask();
	Feed->LastServerPing -= 3;
	Feed->TryPing(nullptr, 0);
	TestTrue(TEXT("Middle-click switch request reaches team feed"), Feed->GetMessage(0).Contains(TEXT("Switch 01 /")));
	TestTrue(TEXT("Teammate sees switch ping"), Mate->HasMessages());
	TestFalse(TEXT("Opposition cannot see switch ping"), Enemy->HasMessages());
	TestFalse(TEXT("Spectator cannot see switch ping"), Spectator->HasMessages());
	TestEqual(TEXT("Switch ping uses sender's team color"), Feed->GetMessageTeam(0), EFlickTeam::Player1);
	TestEqual(TEXT("Pinging never activates a switch"), Arena->GetRaisedDividerMask(), RaisedBefore);
	const int32 SwitchCount = Feed->Messages.Num();
	auto* LiveEnemy = World->SpawnActor<AFlickPiece>();
	LiveEnemy->InitializePiece(EFlickTeam::Player2, 12, 45, 20, EFlickPieceArchetype::Standard);
	Feed->ServerPingTarget_Implementation(12, INDEX_NONE);
	Feed->ServerPingTarget_Implementation(INDEX_NONE, 1);
	TestEqual(TEXT("Pucks and switches share the server cooldown"), Feed->Messages.Num(), SwitchCount);
	const double LocalAcceptedTime = Feed->LastLocalPing;
	Feed->TryPing(nullptr, 1);
	TestEqual(TEXT("Switch pings share local input cooldown"), Feed->LastLocalPing, LocalAcceptedTime);
	Feed->LastServerPing -= 3;
	Feed->ServerPingTarget_Implementation(INDEX_NONE, Arena->GetMechanismCount());
	TestEqual(TEXT("Server rejects nonexistent switches"), Feed->Messages.Num(), SwitchCount);
	Feed->LastServerPing -= 3;
	Feed->ServerPingTarget_Implementation(10, 0);
	TestEqual(TEXT("Server rejects ambiguous targets"), Feed->Messages.Num(), SwitchCount);
	Feed->LastServerPing -= 3;
	State->bSeriesComplete = true;
	Feed->ServerPingTarget_Implementation(INDEX_NONE, 0);
	TestEqual(TEXT("Switch pings disabled after match"), Feed->Messages.Num(), SwitchCount);
	State->bSeriesComplete = false;
	Feed->Messages[0].ReceivedAt -= 6;
	TestTrue(TEXT("Switch message fades"), Feed->GetOpacity(0) > 0 && Feed->GetOpacity(0) < 1);
	Feed->Messages[0].ReceivedAt -= 2;
	TestFalse(TEXT("Switch message expires"), Feed->HasMessages());
	State->MatchId = TEXT("new-match"); TestFalse(TEXT("Old match feed cleared from view"), Mate->HasMessages());
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif
