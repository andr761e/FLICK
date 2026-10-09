#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Online/FlickSessionSubsystem.h"
#include "Online/FlickMatchmakingCoordinatorSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickMatchmakingFeedbackTest, "FLICK.Online.MatchmakingFeedback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMatchmakingFeedbackTest::RunTest(const FString& Parameters)
{
	// Uninitialised fixtures: no Steam requests, HTTP calls, travel or live matchmaking.
	auto* Instance = NewObject<UGameInstance>();
	auto* Sessions = NewObject<UFlickSessionSubsystem>(Instance);
	TestFalse(TEXT("No stale search feedback before first attempt"), Sessions->HasMatchmakingAttempt());
	TestEqual(TEXT("Initial elapsed time is zero"), Sessions->GetMatchmakingElapsedSeconds(), 0.f);
	Sessions->MatchmakingStartedSeconds = FPlatformTime::Seconds() - 75.;
	Sessions->SetState(EFlickSessionState::Searching, TEXT("SEARCHING"));
	TestTrue(TEXT("Search elapsed survives presentation navigation"), Sessions->GetMatchmakingElapsedSeconds() >= 75.f);
	const double Started = Sessions->GetMatchmakingStartedSeconds();
	Sessions->SetState(EFlickSessionState::Joining, TEXT("JOINING"));
	TestEqual(TEXT("Joining has a distinct presentation state"), Sessions->GetState(), EFlickSessionState::Joining);
	TestEqual(TEXT("Joining does not reset the search clock"), Sessions->GetMatchmakingStartedSeconds(), Started);
	Sessions->SetState(EFlickSessionState::Error, TEXT("SERVER CONNECTION TIMED OUT"));
	TestTrue(TEXT("Failure reason remains available after search stops"), Sessions->GetStatusMessage().Contains(TEXT("TIMED OUT")));
	TestTrue(TEXT("Failure retains its matchmaking context"), Sessions->HasMatchmakingAttempt());
	Sessions->MatchmakingStartedSeconds = FPlatformTime::Seconds();
	TestTrue(TEXT("New attempt can redisplay an identical dismissed error"), Sessions->GetMatchmakingStartedSeconds() != Started);
	auto* Coordinator = NewObject<UFlickMatchmakingCoordinatorSubsystem>(Instance);
	Coordinator->Mode = EFlickCoordinatorMode::LocalHttp;
	FFlickCoordinatorQueueRequest Request;
	TestFalse(TEXT("Unconfigured server rejects the queue without HTTP"), Coordinator->QueueParty(Request, FFlickCoordinatorQueueCallback()));
	TestEqual(TEXT("Missing server is a visible error"), Coordinator->GetQueueState(), EFlickCoordinatorQueueState::Error);
	TestTrue(TEXT("Missing server has an actionable reason"), Coordinator->GetStatusMessage().Contains(TEXT("NOT CONFIGURED")));
	TestTrue(TEXT("Even immediate coordinator failure records an attempt"), Coordinator->HasQueueAttempt());
	Coordinator->SearchStartedSeconds = FPlatformTime::Seconds() - 90.;
	TestTrue(TEXT("Coordinator clock reports elapsed time"), Coordinator->GetSearchElapsedSeconds() >= 90.f);
	return true;
}
#endif
