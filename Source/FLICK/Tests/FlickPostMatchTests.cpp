#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPostMatchTest, "FLICK.Match.PostMatchSummaryAndRematch",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPostMatchTest::RunTest(const FString& Parameters)
{
 const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
  .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
 if (!TestNotNull(TEXT("Test world"), World)) return false;
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 AFlickGameState* State = World->SpawnActor<AFlickGameState>();
 for (const EFlickMatchVariant Variant : {EFlickMatchVariant::Classic, EFlickMatchVariant::Bob})
 {
  for (int32 Size = 1; Size <= (Variant == EFlickMatchVariant::Bob ? 1 : 3); ++Size)
  {
   State->PlayerArray.Reset();
   State->SetTeamFormat(Size);
   State->ResetSeriesState(3);
   State->ActiveMatchVariant = Variant;
   TArray<AFlickPlayerState*> Players;
   for (EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
    for (int32 Slot = 0; Slot < Size; ++Slot)
    {
     AFlickPlayerState* Player = World->SpawnActor<AFlickPlayerState>();
     Player->SetTeam(Team); Player->SetTeamPlayerSlot(Slot);
     Players.Add(Player);
    }
   State->PlayerArray.Reset();
   for (AFlickPlayerState* Player : Players) State->PlayerArray.Add(Player);
   TestFalse(TEXT("Mid-match requests cannot restart the game"), State->RegisterRematchVote(Players[0], true));
   State->RecordPlayerKnockout(EFlickTeam::Player1, 0);
   const int32 EarnedScore = State->FindPlayerMatchStats(EFlickTeam::Player1, 0)->Score;
   State->RecordPlayerSelfKnockout(EFlickTeam::Player1, 0);
   State->RecordPlayerSwitchActivation(EFlickTeam::Player1, 0);
   TestEqual(TEXT("Context counters give no reward"), State->FindPlayerMatchStats(EFlickTeam::Player1, 0)->Score, EarnedScore);
   State->CompleteRound(EFlickMatchOutcome::Player1Wins);
   TestEqual(TEXT("Stats survive round transitions"), State->FindPlayerMatchStats(EFlickTeam::Player1, 0)->SwitchActivations, 1);
   State->CompleteRound(EFlickMatchOutcome::Player1Wins);
   State->CompleteRound(EFlickMatchOutcome::Player1Wins);
   AFlickPlayerState* Spectator = World->SpawnActor<AFlickPlayerState>();
   State->PlayerArray.AddUnique(Spectator);
   TestFalse(TEXT("Spectators cannot vote"), State->RegisterRematchVote(Spectator, false));
   State->RegisterRematchVote(Players[0], true);
   State->RegisterRematchVote(Players[0], false);
   TestEqual(TEXT("Repeated requests count once"), State->RematchVotes.Num(), 1);
   TestEqual(TEXT("BOB has no class selection"), State->bRematchChangeLineup, Variant == EFlickMatchVariant::Classic);
   TestFalse(TEXT("One player cannot force a rematch"), State->HasRematchConsensus());
   for (AFlickPlayerState* Player : Players) State->RegisterRematchVote(Player, false);
   TestTrue(TEXT("Full roster is ready"), State->HasRematchConsensus());
   State->PlayerArray.Remove(Players.Last());
   TestFalse(TEXT("A departed seat cannot silently reduce the rematch roster"), State->HasRematchConsensus());
   State->ResetSeriesState(3);
   TestEqual(TEXT("New matches clear votes"), State->RematchVotes.Num(), 0);
   TestEqual(TEXT("New matches clear context counters"), State->FindPlayerMatchStats(EFlickTeam::Player1, 0)->SelfKnockouts, 0);
   for (AFlickPlayerState* Player : Players) Player->Destroy();
   Spectator->Destroy();
  }
 }
 World->DestroyWorld(false);
 GEngine->DestroyWorldContext(World);
 return true;
}
#endif
