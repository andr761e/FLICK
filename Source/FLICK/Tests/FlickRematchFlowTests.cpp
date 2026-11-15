#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickReplayController.h"
#include "Player/FlickPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRematchFlowTest, "FLICK.Match.PostMatchRematchFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRematchFlowTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Rematch world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickGameState* State = World->SpawnActor<AFlickGameState>();
	World->SetGameState(State);
	AFlickGameMode* Mode = World->SpawnActor<AFlickGameMode>();
	Mode->GameState = State;
	AFlickPlayerController* Host = World->SpawnActor<AFlickPlayerController>();
	Host->SetAsLocalPlayerController();
	World->AddController(Host);
	// Ending a recording destroys its internal spectator. It must not be treated
	// as a room participant disconnecting or dismiss the completed-match summary.
	for (const bool bPrivateRecording : {false, true})
	{
		Mode->bNetworkMatchRequested = true;
		Mode->bNetworkMatchStarted = true;
		Mode->bPrivateMatchActive = bPrivateRecording;
		Mode->FrontendScreen = EFlickFrontendScreen::Playing;
		State->ResetSeriesState(1);
		State->CompleteRound(EFlickMatchOutcome::Player2Wins);
		State->SetNetworkLobbyState(false, EFlickMatchVariant::Classic);
		auto* Recorder = World->SpawnActor<AFlickReplayController>();
		Recorder->PlayerState = World->SpawnActor<AFlickPlayerState>();
		Mode->Logout(Recorder);
		TestEqual(TEXT("Recorder cleanup keeps the post-match screen"), Mode->FrontendScreen, EFlickFrontendScreen::Playing);
		TestEqual(TEXT("Recorder cleanup preserves final result phase"), State->MatchPhase, EFlickMatchPhase::RoundOver);
		TestFalse(TEXT("Recorder cleanup does not reopen the lobby"), State->bNetworkLobbyActive);
		TestTrue(TEXT("Recorder cleanup keeps the current room match"), Mode->bNetworkMatchStarted);
		TestTrue(TEXT("Recorder cleanup preserves the finished series"), State->bSeriesComplete);
		Recorder->Destroy();
	}
	Mode->bNetworkMatchRequested = false;
	Mode->bNetworkMatchStarted = false;
	for (const bool bPrivate : {false, true})
	for (const EFlickMatchVariant Variant : {EFlickMatchVariant::Classic, EFlickMatchVariant::Bob})
	for (int32 Size = 1; Size <= (Variant == EFlickMatchVariant::Bob ? 1 : 3); ++Size)
	{
		Mode->SelectedMatchVariant = Variant;
		Mode->MatchmakingPlayersPerTeam = Size;
		Mode->bPrivateMatchActive = bPrivate;
		Mode->bPartyRequested = false;
		Mode->bPlayerClassesActiveForMatch = Variant == EFlickMatchVariant::Classic;
		Mode->bHasPendingClassChanges = false;
		Mode->EnsureActivePlayerClasses(Size);
		Mode->Player1ActiveClasses[0] = EFlickLineupPreset::Power;
		Mode->Player2ActiveClasses[0] = EFlickLineupPreset::Speed;
		Mode->PrivateMatchSettings.RoundsToWin = 2;
		Mode->ApplyMatchConfiguration(Variant, Size);
		Mode->FrontendScreen = EFlickFrontendScreen::Playing;
		State->BeginAuthoritativeMatch(TEXT("old-match"));
		State->ResetSeriesState(3);
		State->bSeriesComplete = true;
		State->MatchPhase = EFlickMatchPhase::RoundOver;
		State->RecordPlayerSelfKnockout(EFlickTeam::Player1, 0);
		Mode->RequestRematch(Host, false);
		TestFalse(TEXT("Play Again starts a fresh series"), State->bSeriesComplete);
		TestTrue(TEXT("Every rematch gets a fresh result ID"), State->MatchId != TEXT("old-match"));
		TestEqual(TEXT("Same team size"), State->PlayersPerTeam, Size);
		TestEqual(TEXT("Same game variant"), State->ActiveMatchVariant, Variant);
		TestEqual(TEXT("Context counters reset"), State->FindPlayerMatchStats(EFlickTeam::Player1, 0)->SelfKnockouts, 0);
		if (Variant == EFlickMatchVariant::Classic)
		{
			TestEqual(TEXT("Blue lineup preserved"), Mode->Player1ActiveClasses[0], EFlickLineupPreset::Power);
			TestEqual(TEXT("Orange lineup preserved"), Mode->Player2ActiveClasses[0], EFlickLineupPreset::Speed);
			if (bPrivate) TestEqual(TEXT("Private round rule preserved"), State->RoundsToWin, 2);
			State->bSeriesComplete = true;
			State->MatchPhase = EFlickMatchPhase::RoundOver;
			Mode->RequestRematch(Host, true);
			if (bPrivate)
			{
				TestTrue(TEXT("Private change flow reopens room role selection"), State->bPrivateMatchAssignmentActive);
				State->SetPrivateMatchAssignmentState(false, false, 0);
			}
			else
			{
				TestEqual(TEXT("Local change flow opens the existing class selector"), Mode->FrontendScreen, EFlickFrontendScreen::ClassSelect);
				const FString NewId = State->MatchId;
				Mode->CancelClassSelection();
				TestFalse(TEXT("Canceling lineup edit still starts the accepted rematch"), State->bSeriesComplete);
				TestEqual(TEXT("Lineup cancellation does not allocate another match ID"), State->MatchId, NewId);
				TestEqual(TEXT("Cancel preserves the original class"), Mode->Player1ActiveClasses[0], EFlickLineupPreset::Power);
			}
		}
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
