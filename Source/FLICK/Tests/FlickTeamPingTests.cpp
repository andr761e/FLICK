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
#include "Core/FlickQuickChats.h"

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
	// Quick chats use the same private delivery, lifetime and anti-spam budget.
	Feed->Messages.Reset(); Mate->Messages.Reset();
	Feed->LastServerPing -= 3;
	Feed->ServerQuickChat_Implementation(TEXT("NiceShot"));
	TestEqual(TEXT("Quick chat is server formatted"), Feed->GetMessage(0), FString(TEXT("Player0: Nice shot!")));
	TestTrue(TEXT("Teammate receives quick chat"), Mate->HasMessages());
	TestFalse(TEXT("Enemy cannot see quick chat"), Enemy->HasMessages());
	TestFalse(TEXT("Spectator cannot see quick chat"), Spectator->HasMessages());
	Feed->ServerQuickChat_Implementation(TEXT("Thanks"));
	Feed->ServerPingTarget_Implementation(INDEX_NONE, 0);
	TestEqual(TEXT("Quick chats and pings share cooldown"), Feed->Messages.Num(), 1);
	Feed->LastServerPing -= 3;
	Feed->ServerQuickChat_Implementation(TEXT("arbitrary forged text"));
	TestEqual(TEXT("Unlisted network text is rejected"), Feed->Messages.Num(), 1);
	Feed->QuickChatInput(2);
	TestEqual(TEXT("First key opens group"), Feed->GetQuickChatGroup(), 2);
	TestTrue(TEXT("Escape cancels open group"), Feed->CancelQuickChat());
	Feed->QuickChatInput(1); Feed->QuickChatOpenedAt -= 4;
	TestEqual(TEXT("Chooser times out"), Feed->GetQuickChatGroup(), INDEX_NONE);
	Feed->LastLocalPing -= 3; Feed->LastServerPing -= 3;
	Feed->QuickChatInput(0); Feed->QuickChatInput(0);
	TestEqual(TEXT("Second key closes chooser and sends configured phrase"), Feed->Messages.Num(), 2);
	const auto* Configured = FlickQuickChats::Find(FlickQuickChats::GetSlot(0, 0));
	TestTrue(TEXT("Configured slot text used"), Configured && Feed->GetMessage(1).Contains(Configured->Text));
	Spectator->LastServerPing -= 3; Spectator->ServerQuickChat_Implementation(TEXT("NiceShot"));
	TestEqual(TEXT("Spectators cannot send"), Feed->Messages.Num(), 2);
	Feed->Messages[1].ReceivedAt -= 6;
	TestTrue(TEXT("Quick chat fades"), Feed->GetOpacity(1) > 0 && Feed->GetOpacity(1) < 1);
	Feed->Messages[1].ReceivedAt -= 2;
	TestEqual(TEXT("Quick chat expires"), Feed->GetOpacity(1), 0.f);
	State->bNetworkClassSelectionActive = true;
	Feed->LastServerPing -= 3; Feed->ServerQuickChat_Implementation(TEXT("Thanks"));
	Feed->QuickChatInput(1);
	TestEqual(TEXT("Menus block quick chats"), Feed->Messages.Num(), 2);
	TestEqual(TEXT("Menus block chooser"), Feed->GetQuickChatGroup(), INDEX_NONE);
	for (int32 Group = 0; Group < 4; ++Group)
		for (int32 Slot = 0; Slot < 4; ++Slot)
			TestNotNull(TEXT("All sixteen slots resolve to valid phrases"), FlickQuickChats::Find(FlickQuickChats::GetSlot(Group, Slot)));
	TestFalse(TEXT("Invalid slot customization rejected"), FlickQuickChats::SetSlot(4, 0, TEXT("NiceShot")));
	TestFalse(TEXT("Unknown phrase customization rejected"), FlickQuickChats::SetSlot(0, 0, TEXT("unknown")));
	State->bNetworkClassSelectionActive = false;
	Feed->Messages.Reset(); Mate->Messages.Reset();
	Feed->LastLocalPing = World->GetTimeSeconds();
	Feed->QuickChatInput(0); Feed->QuickChatInput(0);
	TestTrue(TEXT("Blocked send shows local cooldown seconds"), Feed->GetCooldownNotice().Contains(TEXT("2.0 s")));
	TestTrue(TEXT("Cooldown notice is not sent to teammates"), Mate->GetCooldownNotice().IsEmpty() && Mate->Messages.IsEmpty());
	TestTrue(TEXT("Cooldown notice does not occupy a history row"), Feed->Messages.IsEmpty());
	Feed->LastLocalPing -= 1.2;
	TestTrue(TEXT("Cooldown notice updates remaining time"), Feed->GetCooldownNotice().Contains(TEXT("0.8 s")));
	const double CooldownStart = Feed->LastLocalPing;
	Feed->QuickChatInput(0); Feed->QuickChatInput(0);
	TestEqual(TEXT("Spam does not extend cooldown"), Feed->LastLocalPing, CooldownStart);
	Feed->LastLocalPing -= 1;
	TestTrue(TEXT("Notice disappears when ready"), Feed->GetCooldownNotice().IsEmpty());
	for (int32 Index = 0; Index < 6; ++Index)
	{
		Feed->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, FString::Printf(TEXT("History%d"), Index));
		for (auto& Entry : Feed->Messages) Entry.ReceivedAt -= 8;
		TestFalse(TEXT("History fades without being removed"), Feed->HasMessages());
	}
	TestEqual(TEXT("History retains only five messages"), Feed->Messages.Num(), 5);
	Feed->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, TEXT("Newest"));
	TestEqual(TEXT("Oldest message is evicted"), Feed->GetMessage(0), FString(TEXT("History2")));
	TestEqual(TEXT("Newest message appears at bottom"), Feed->GetMessage(4), FString(TEXT("Newest")));
	for (int32 Row = 0; Row < 5; ++Row)
		TestEqual(TEXT("All five faded messages reappear at full opacity"), Feed->GetOpacity(Row), 1.f);
	State->MatchId = TEXT("history-next-match");
	Feed->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player1, TEXT("Fresh match"));
	TestEqual(TEXT("Previous match history is not revived"), Feed->Messages.Num(), 1);
	Controllers[0]->GetPlayerState<AFlickPlayerState>()->SetTeam(EFlickTeam::Player2);
	Feed->ClientReceivePing_Implementation(State->MatchId, EFlickTeam::Player2, TEXT("Fresh team"));
	TestEqual(TEXT("Previous team history is not revived"), Feed->Messages.Num(), 1);
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif
