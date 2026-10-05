#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Debug/FlickPhysicsDiagnosticsComponent.h"
#include "HAL/IConsoleManager.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPhysicsDiagnosticsTest, "FLICK.Debug.PhysicsDiagnostics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPhysicsDiagnosticsTest::RunTest(const FString& Parameters)
{
	IConsoleVariable* Toggle = IConsoleManager::Get().FindConsoleVariable(TEXT("flick.Diagnostics"));
	if (!TestNotNull(TEXT("Developer diagnostics console option"), Toggle)) return false;
	const int32 Saved = Toggle->GetInt();
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Diagnostics world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickPlayerController* Viewer = World->SpawnActor<AFlickPlayerController>();
	Viewer->SetAsLocalPlayerController();
	World->AddController(Viewer);
	AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
	Piece->InitializePiece(EFlickTeam::Player1, 1, 45, 20, EFlickPieceArchetype::Standard);
	UPrimitiveComponent* Root = CastChecked<UPrimitiveComponent>(Piece->GetRootComponent());
	const FTransform Before = Piece->GetActorTransform();
	const FVector Velocity = Root->GetPhysicsLinearVelocity();
	const float Mass = Root->GetMass();
	const ECollisionEnabled::Type Collision = Root->GetCollisionEnabled();
	UFlickPhysicsDiagnosticsComponent* Diagnostics = Viewer->FindComponentByClass<UFlickPhysicsDiagnosticsComponent>();
	if (TestNotNull(TEXT("Local controller owns diagnostics"), Diagnostics))
	{
		Toggle->Set(1, ECVF_SetByConsole);
		Diagnostics->TickComponent(.016f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Enabled panel reports the actual active puck count"), Diagnostics->GetSummary().Contains(TEXT("1 active pucks")));
		TestTrue(TEXT("Diagnostics preserve transforms"), Piece->GetActorTransform().Equals(Before));
		TestTrue(TEXT("Diagnostics preserve velocity"), Root->GetPhysicsLinearVelocity().Equals(Velocity));
		TestEqual(TEXT("Diagnostics preserve mass"), Root->GetMass(), Mass);
		TestEqual(TEXT("Diagnostics preserve collision"), Root->GetCollisionEnabled(), Collision);
		Toggle->Set(0, ECVF_SetByConsole);
		Diagnostics->TickComponent(.016f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("Disabling clears the diagnostic state"), Diagnostics->GetSummary().IsEmpty());
	}
	Toggle->Set(Saved, ECVF_SetByConsole);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
