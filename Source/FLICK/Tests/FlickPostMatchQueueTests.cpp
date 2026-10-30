#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPostMatchQueueTest, "FLICK.Online.PostMatchQueueIntent",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickPostMatchQueueTest::RunTest(const FString&)
{
 const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
 if (!TestNotNull(TEXT("Queue test world"), World)) return false;
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* Instance = NewObject<UFlickGameInstance>();
 World->SetGameInstance(Instance); // Deliberately no session/HTTP subsystems: no live queue.
 World->SetGameMode(FURL(nullptr, TEXT("/Engine/Maps/Entry?game=/Script/FLICK.FlickGameMode"), TRAVEL_Absolute));
 auto* Mode = Cast<AFlickGameMode>(World->GetAuthGameMode());
 if (!TestNotNull(TEXT("Authoritative mode"), Mode)) { World->DestroyWorld(false); return false; }
 auto* State = World->SpawnActor<AFlickGameState>();
 World->SetGameState(State); Mode->GameState = State;
 auto* Host = World->SpawnActor<AFlickPlayerController>();
 Host->SetAsLocalPlayerController(); World->AddController(Host);
 for (bool bRanked : {false, true})
 for (EFlickMatchVariant Variant : {EFlickMatchVariant::Classic, EFlickMatchVariant::Bob})
 for (int32 Size = 1; Size <= (Variant == EFlickMatchVariant::Bob ? 1 : 3); ++Size)
 {
  Mode->SelectedMatchVariant = Variant; Mode->MatchmakingPlayersPerTeam = Size;
  Mode->ApplyMatchConfiguration(Variant, Size);
  State->SetTeamFormat(Size); State->ActiveMatchVariant = Variant;
  Mode->FrontendScreen = EFlickFrontendScreen::Playing;
  State->bSeriesComplete = true; State->bMatchmakingLobby = true; State->bRankedMatch = bRanked;
  Host->PlayAgain();
  TestTrue(TEXT("A public result stores a queue intent rather than a rematch vote"), Instance->bPostMatchQueuePending && State->RematchVotes.IsEmpty());
  TestEqual(TEXT("Same variant"), Instance->PostMatchQueueVariant, Variant);
  TestEqual(TEXT("Same team size"), Instance->PostMatchQueueTeamSize, Size);
  TestEqual(TEXT("Same casual/competitive playlist"), Instance->bPostMatchQueueRanked, bRanked);
  TestEqual(TEXT("Player returns to the browsable main menu"), Mode->FrontendScreen, EFlickFrontendScreen::MainMenu);
  Host->PlayAgain();
  Mode->Tick(0);
  TestFalse(TEXT("Frontend consumes the intent once"), Instance->bPostMatchQueuePending);
  TestEqual(TEXT("Queue selection preserves ranked flag"), Mode->bRankedQueueSelected, bRanked);
  TestEqual(TEXT("Queue selection preserves size"), Mode->MatchmakingPlayersPerTeam, Size);
 }
 // The fallback listen lobby keeps searching even while its host browses Home.
 Mode->bNetworkMatchRequested = true; Mode->bMatchmakingRequested = true; Mode->bNetworkMatchStarted = false;
 Mode->FrontendScreen = EFlickFrontendScreen::NetworkLobby;
 Mode->ReturnToMainMenu();
 TestEqual(TEXT("Browse does not cancel the fallback queue"), Mode->FrontendScreen, EFlickFrontendScreen::MainMenu);
 TestTrue(TEXT("Queue remains active while browsing"), Mode->bMatchmakingRequested);
 Mode->MatchmakingTimeoutSeconds = 1; Mode->MatchmakingQueueElapsed = 0;
 State->SetMatchmakingState(true); Mode->Tick(2);
 TestTrue(TEXT("Queue timeout still advances while browsing"), State->bMatchmakingTimedOut);
 Mode->bNetworkMatchRequested = false; Mode->bMatchmakingRequested = false;
 auto* Player = World->SpawnActor<AFlickPlayerState>();
 Player->SetRankedIdentity(true, 1504, 21);
 TestEqual(TEXT("Verified placement history accompanies MMR"), Player->GetRankedMatchesPlayed(), 21);
 Player->SetRankedIdentity(false, 1000);
 TestEqual(TEXT("Unverified identity cannot retain an earned rank"), Player->GetRankedMatchesPlayed(), 0);
 GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
 return true;
}
#endif
