#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameState.h"
#include "GameFramework/PlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickReplayVotingTest, "FLICK.Presentation.ReplaySkipVoting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickReplayVotingTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickGameState* State = World->SpawnActor<AFlickGameState>();
	APlayerState* First = World->SpawnActor<APlayerState>();
	APlayerState* Second = World->SpawnActor<APlayerState>();
	APlayerState* Bot = World->SpawnActor<APlayerState>();
	APlayerState* Outsider = World->SpawnActor<APlayerState>();
	Bot->SetIsABot(true);
	State->PlayerArray = {First, Second, Bot};
	State->ReplaySerial = 1;
	TestEqual(TEXT("Bots do not hold up voting"), State->GetPendingReplayPlayers().Num(), 2);
	TestFalse(TEXT("No votes is not consensus"), State->HasReplaySkipConsensus());
	TestFalse(TEXT("Nonmembers cannot vote"), State->RegisterReplaySkipVote(Outsider, 1));
	TestFalse(TEXT("Old replay votes are rejected"), State->RegisterReplaySkipVote(First, 0));
	TestFalse(TEXT("Bots cannot vote"), State->RegisterReplaySkipVote(Bot, 1));
	TestTrue(TEXT("First human can vote"), State->RegisterReplaySkipVote(First, 1));
	State->RegisterReplaySkipVote(First, 1);
	TestEqual(TEXT("Duplicate votes count once"), State->ReplaySkipVotes.Num(), 1);
	TestFalse(TEXT("One vote cannot skip for two people"), State->HasReplaySkipConsensus());
	Second->SetIsOnlyASpectator(true);
	TestEqual(TEXT("Human spectators still vote"), State->GetPendingReplayPlayers().Num(), 1);
	State->RegisterReplaySkipVote(Second, 1);
	TestTrue(TEXT("Everyone voting reaches consensus"), State->HasReplaySkipConsensus());
	++State->ReplaySerial;
	State->ReplaySkipVotes.Reset();
	TestFalse(TEXT("Votes reset next replay"), State->HasReplaySkipConsensus());
	TestFalse(TEXT("Delayed vote cannot skip next replay"), State->RegisterReplaySkipVote(First, 1));
	State->RegisterReplaySkipVote(First, 2);
	State->PlayerArray.Remove(Second);
	TestTrue(TEXT("A disconnected player cannot hold up the replay"), State->HasReplaySkipConsensus());
	State->PlayerArray = {Bot};
	TestFalse(TEXT("An empty human roster is not automatic consent"), State->HasReplaySkipConsensus());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
