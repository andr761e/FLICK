#if WITH_DEV_AUTOMATION_TESTS
#include "Player/FlickCameraPawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickCameraMovementTest, "FLICK.Camera.MovementAndZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickCameraMovementTest::RunTest(const FString& Parameters)
{
	auto* Slow = NewObject<AFlickCameraPawn>();
	auto* Fast = NewObject<AFlickCameraPawn>();
	for (auto* Camera : {Slow, Fast}) Camera->SetTestArenaPresentation(true);
	const float InitialElevation = Slow->GetGameplayElevationAngle();
	for (int32 Frame = 0; Frame < 20; ++Frame)
	{
		Slow->RotateGameplayOrbit(1.f, 1.f / 40.f);
		Slow->MoveGameplayElevation(1.f, 1.f / 40.f);
	}
	for (int32 Frame = 0; Frame < 200; ++Frame)
	{
		Fast->RotateGameplayOrbit(1.f, 1.f / 400.f);
		Fast->MoveGameplayElevation(1.f, 1.f / 400.f);
	}
	TestTrue(TEXT("Elevation moves while held"), Slow->GetGameplayElevationAngle() > InitialElevation);
	TestTrue(TEXT("Orbit speed is consistent at 40 and 400 FPS"), FMath::IsNearlyEqual(Slow->GetGameplayOrbitAngle(), Fast->GetGameplayOrbitAngle(), .001f));
	TestTrue(TEXT("Elevation speed is consistent at 40 and 400 FPS"), FMath::IsNearlyEqual(Slow->GetGameplayElevationAngle(), Fast->GetGameplayElevationAngle(), .001f));
	const float Pitch = Slow->GetGameplayElevationAngle();
	Slow->SetGameplayViewIndex(0, true);
	const FVector ArenaFocus(0.f, 0.f, 250.f);
	const float OriginalDistance = FVector::Distance(Slow->GetActorLocation(), ArenaFocus);
	Slow->AdjustGameplayZoom(1);
	Slow->SetGameplayViewIndex(0, true);
	TestTrue(TEXT("Zoom actually moves the camera closer to the arena surface"),
		FVector::Distance(Slow->GetActorLocation(), ArenaFocus) < OriginalDistance);
	TestTrue(TEXT("Wheel up brings the camera closer"), Slow->GetGameplayZoomScale() < 1.f);
	TestEqual(TEXT("Zoom does not change elevation"), Slow->GetGameplayElevationAngle(), Pitch);
	for (int32 Step = 0; Step < 100; ++Step) Slow->AdjustGameplayZoom(1);
	TestEqual(TEXT("Zoom cannot enter the arena"), Slow->GetGameplayZoomScale(), Slow->GameplayMinimumZoomScale);
	for (int32 Step = 0; Step < 100; ++Step) Slow->AdjustGameplayZoom(-1);
	TestEqual(TEXT("Zoom out remains bounded"), Slow->GetGameplayZoomScale(), Slow->GameplayMaximumZoomScale);
	Slow->ResetGameplayView(0, true);
	TestEqual(TEXT("Camera reset restores standard distance"), Slow->GetGameplayZoomScale(), 1.f);
	Slow->ToggleTopDownView();
	Slow->AdjustGameplayZoom(1);
	TestTrue(TEXT("Top view also supports zoom"), Slow->GetGameplayZoomScale() < 1.f);
	Slow->SetMenuPresentation(true);
	const float MenuZoom = Slow->GetGameplayZoomScale();
	Slow->AdjustGameplayZoom(-1);
	TestEqual(TEXT("Gameplay wheel cannot alter menu camera"), Slow->GetGameplayZoomScale(), MenuZoom);
	return true;
}
#endif
