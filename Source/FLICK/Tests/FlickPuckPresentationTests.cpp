#if WITH_DEV_AUTOMATION_TESTS
#include "Player/FlickCameraPawn.h"
#include "Core/FlickPuckFinish.h"
#include "Core/FlickTypes.h"
#include "Camera/CameraComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPuckPresentationTest, "FLICK.Visuals.PuckLockerPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPuckPresentationTest::RunTest(const FString& Parameters)
{
	auto* Pawn = NewObject<AFlickCameraPawn>();
	auto* Camera = Pawn->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Puck presentation camera"), Camera)) return false;
	Pawn->SetTestArenaPresentation(true);
	const float GameplayEV = Camera->PostProcessSettings.AutoExposureBias;
	const float GameplayBloom = Camera->PostProcessSettings.BloomIntensity;
	const float GameplayThreshold = Camera->PostProcessSettings.BloomThreshold;
	TestEqual(TEXT("Live Knockout uses the approved locker exposure"), GameplayEV, FlickPuckFinish::ReferenceExposureBias);
	Pawn->BeginCinematicReplay(FVector(100, 300, 270), EFlickTeam::Player1);
	TestEqual(TEXT("Replay retains the same exposure"), Camera->PostProcessSettings.AutoExposureBias, GameplayEV);
	Pawn->EndCinematicReplay();
	Pawn->SetMenuPresentation(true);
	Pawn->SetMenuOrbitEnabled(true);
	TestEqual(TEXT("Ordinary frontend cameras retain their exposure"), Camera->PostProcessSettings.AutoExposureBias, 0.f);
	Pawn->SetHomePuckPresentation(true);
	TestEqual(TEXT("Home showcase uses locker exposure"), Camera->PostProcessSettings.AutoExposureBias, GameplayEV);
	TestEqual(TEXT("Home showcase uses locker bloom"), Camera->PostProcessSettings.BloomIntensity, GameplayBloom);
	TestEqual(TEXT("Home showcase uses locker bloom threshold"), Camera->PostProcessSettings.BloomThreshold, GameplayThreshold);
	Pawn->SetHomePuckPresentation(false);
	TestEqual(TEXT("Other frontend screens retain their exposure"), Camera->PostProcessSettings.AutoExposureBias, 0.f);
	Pawn->SetLockerPreviewMode(1);
	Pawn->SetMenuPresentation(true);
	TestEqual(TEXT("Locker and live exposure agree without double compensation"), Camera->PostProcessSettings.AutoExposureBias, GameplayEV);
	TestEqual(TEXT("Locker and live LED bloom agree"), Camera->PostProcessSettings.BloomIntensity, GameplayBloom);
	TestEqual(TEXT("Locker and live bloom threshold agree"), Camera->PostProcessSettings.BloomThreshold, GameplayThreshold);
	Pawn->SetHomePuckPresentation(true);
	Pawn->SetMenuPresentation(true);
	TestEqual(TEXT("Locker never receives Home compensation twice"), Camera->PostProcessSettings.AutoExposureBias, GameplayEV);
	Pawn->SetHomePuckPresentation(false);
	Pawn->SetLockerPreviewMode(0);
	Pawn->SetMenuPresentation(false);
	TestEqual(TEXT("Leaving locker restores the same live exposure"), Camera->PostProcessSettings.AutoExposureBias, GameplayEV);
	Pawn->SetTestArenaPresentation(false);
	TestEqual(TEXT("Other arenas retain their own exposure"), Camera->PostProcessSettings.AutoExposureBias, Pawn->ClassicExposureBias);
	return true;
}
#endif
