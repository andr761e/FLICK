#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Game/FlickPracticeComponent.h"
#include "Core/FlickPracticeCatalog.h"
#include "Arena/FlickTestArena.h"
#include "Arena/FlickBobArena.h"
#include "Pieces/FlickPiece.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPracticeTest, "FLICK.Training.PracticePacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPracticeTest::RunTest(const FString& Parameters)
{
	// Never write test scores into the player's real progress.
	TGuardValue<FString> Config(GGameUserSettingsIni, FConfigCacheIni::NormalizeConfigIniPath(
		FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Config/PracticeAutomation.ini"))));
	GConfig->Add(GGameUserSettingsIni, FConfigFile());
	TestEqual(TEXT("Initially no mastered packs"), FlickPracticeCatalog::PerfectPacks(), 0);
	FlickPracticeCatalog::RecordScore(0, 0, 4);
	FlickPracticeCatalog::RecordScore(0, 0, 2);
	TestEqual(TEXT("Lower scores cannot overwrite a best"), FlickPracticeCatalog::BestScore(0, 0), 4);
	TestEqual(TEXT("Other difficulty remains independent"), FlickPracticeCatalog::BestScore(0, 1), 0);
	FlickPracticeCatalog::RecordScore(-1, 0, 5);
	TestEqual(TEXT("Invalid pack rejected"), FlickPracticeCatalog::BestScore(-1, 0), 0);
	GConfig->UnloadFile(GGameUserSettingsIni); GConfig->LoadFile(GGameUserSettingsIni);
	TestEqual(TEXT("Personal best survives disk reload"), FlickPracticeCatalog::BestScore(0, 0), 4);
	GConfig->Add(GGameUserSettingsIni, FConfigFile());

	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Practice test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
	auto* Mode = World->SpawnActor<AFlickGameMode>(); Mode->GameState = State;
	Mode->bTrainingMode = Mode->bTutorialMode = true;
	Mode->FrontendScreen = EFlickFrontendScreen::Playing;
	Mode->ArenaSurfaceZ = 250;
	State->InitializePlayerMatchStats(1);
	auto* Practice = Mode->GetPractice();
	Mode->TestArenaActor = World->SpawnActor<AFlickTestArena>();
	Mode->TestArenaActor->InitializeTestArena(Mode->ArenaRadius, 50, 250, 1);
	Mode->BobArenaActor = World->SpawnActor<AFlickBobArena>();
	Mode->BobArenaActor->InitializeArena(620, 54, 250);
	const auto CleanTracking = [Mode]() { Mode->ResolutionEliminatedPieceIds.Reset(); Mode->ResolutionActivatedSwitchMask = 0;
		Mode->bPlayer1BobStrikerPocketed = Mode->bPlayer2BobStrikerPocketed = false; };
	const auto Pass = [Mode, Practice, CleanTracking]()
	{
		CleanTracking();
		if (Practice->Category == 0) Mode->Pieces[0]->SetActorLocation(FVector(Practice->ZoneCenter, 265), false, nullptr, ETeleportType::TeleportPhysics);
		else if (Practice->Category == 2) Mode->ResolutionActivatedSwitchMask = 1 << Practice->SwitchIndex;
		else if (Practice->Category == 3) Mode->Pieces[1]->Eliminate();
		else Mode->ResolutionEliminatedPieceIds.Add(Practice->TargetId);
		Mode->ResolveTutorialShot();
	};
	for (int32 Category = 0; Category < 4; ++Category)
	for (int32 Difficulty = 0; Difficulty < 3; ++Difficulty)
	{
		Mode->SelectedMatchVariant = Mode->ActiveMatchVariant = Category == 3 ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic;
		Practice->Start(Category, Difficulty);
		TestTrue(TEXT("Pack runs as offline training"), Mode->IsPracticeMode());
		TestFalse(TEXT("Scored packs cannot undo shots"), Mode->CanUndoTrainingShot());
		const float Radius = Mode->Pieces[0]->GetPieceRadius();
		for (int32 Shot = 0; Shot < 5; ++Shot)
		{
			TestEqual(TEXT("Challenge number advances correctly"), Practice->GetChallengeNumber(), Shot + 1);
			TestEqual(TEXT("Physics radius unchanged across difficulty"), Mode->Pieces[0]->GetPieceRadius(), Radius);
			TestEqual(TEXT("Practice marker never collides"), Practice->Marker->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestTrue(TEXT("Assigned shooter selectable"), Mode->CanSelectPiece(Mode->Pieces[0]));
			for (int32 Other = 1; Other < Mode->Pieces.Num(); ++Other)
				TestFalse(TEXT("Targets and friendly guards cannot be selected"), Mode->CanSelectPiece(Mode->Pieces[Other]));
			for (const auto& Piece : Mode->Pieces)
			{
				TestTrue(TEXT("All setups start on the arena"), Category == 3
					? FMath::Max(FMath::Abs(Piece->GetActorLocation().X), FMath::Abs(Piece->GetActorLocation().Y)) + Piece->GetPieceRadius() < 620
					: FVector2D(Piece->GetActorLocation()).Size() + Piece->GetPieceRadius() < Mode->ArenaRadius);
			}
			Pass();
			TestEqual(TEXT("Successful resolution adds one point"), Practice->GetScore(), Shot + 1);
			Pass(); TestEqual(TEXT("Duplicate resolution cannot score twice"), Practice->GetScore(), Shot + 1);
			TestFalse(TEXT("Result display blocks further shots"), Mode->CanSelectPiece(Mode->Pieces[0]));
			Practice->Update(Practice->ResultDisplaySeconds + .1f);
		}
		TestTrue(TEXT("Exactly five challenges complete the run"), Practice->IsComplete());
		TestEqual(TEXT("Perfect run saved independently"), FlickPracticeCatalog::BestScore(Category, Difficulty), 5);
		TestFalse(TEXT("Completed pack cannot be shot again"), Mode->CanSelectPiece(Mode->Pieces[0]));
		Practice->Restart();
		TestEqual(TEXT("Restart clears run score"), Practice->GetScore(), 0);
		TestEqual(TEXT("Restart clears pass indicators"), Practice->GetResult(0), -1);
		TestEqual(TEXT("Restart retains mastered best"), FlickPracticeCatalog::BestScore(Category, Difficulty), 5);
	}
	TestEqual(TEXT("All twelve packs have independent mastery"), FlickPracticeCatalog::PerfectPacks(), 12);
	Mode->SelectedMatchVariant = Mode->ActiveMatchVariant = EFlickMatchVariant::Classic;
	for (int32 Difficulty = 0; Difficulty < 3; ++Difficulty)
	{
		for (const float EdgeOffset : {-1.f, 0.f, .5f})
		{
			Practice->Start(0, Difficulty); CleanTracking();
			auto* Puck = Mode->Pieces[0].Get();
			const float CentreDistance = Practice->ZoneRadius - Puck->GetPieceRadius() + EdgeOffset;
			Puck->SetActorLocation(FVector(Practice->ZoneCenter + FVector2D(CentreDistance, 0), 265), false, nullptr, ETeleportType::TeleportPhysics);
			Practice->ResolveShot();
			TestEqual(TEXT("Precision requires the whole puck inside, including the exact boundary"), Practice->GetResult(0), EdgeOffset <= 0 ? 1 : 0);
			if (EdgeOffset > 0) TestTrue(TEXT("Partial overlap explains full containment"), Practice->GetFeedback().Contains(TEXT("entire puck")));
		}
	}
	Practice->Start(0, 2); CleanTracking(); Practice->ResolveShot();
	TestEqual(TEXT("Precision outside zone misses"), Practice->GetResult(0), 0);
	Practice->Update(3); TestEqual(TEXT("A miss also advances"), Practice->GetChallengeNumber(), 2);
	Practice->Start(1, 1); CleanTracking(); Mode->ResolutionEliminatedPieceIds.Add(Practice->TargetId); Mode->ResolutionEliminatedPieceIds.Add(Practice->GuardId);
	Practice->ResolveShot(); TestEqual(TEXT("Friendly collateral fails knockout"), Practice->GetScore(), 0);
	Practice->Restart(); CleanTracking(); Mode->ResolutionEliminatedPieceIds.Add(Practice->TargetId); Mode->ResolutionEliminatedPieceIds.Add(Practice->ShooterId);
	Practice->ResolveShot(); TestEqual(TEXT("Self knockout fails"), Practice->GetScore(), 0);
	Practice->Start(2, 2); CleanTracking(); Mode->ResolutionActivatedSwitchMask = (1 << Practice->SwitchIndex) | (1 << ((Practice->SwitchIndex + 1) % Mode->TestArenaActor->GetMechanismCount()));
	Practice->ResolveShot(); TestEqual(TEXT("Expert extra switch fails"), Practice->GetScore(), 0);
	Mode->SelectedMatchVariant = Mode->ActiveMatchVariant = EFlickMatchVariant::Bob;
	Practice->Start(3, 0); CleanTracking(); Mode->Pieces[1]->Eliminate(); Mode->bPlayer1BobStrikerPocketed = true;
	Practice->ResolveShot(); TestEqual(TEXT("BOB striker foul fails"), Practice->GetScore(), 0);
	Practice->Restart(); Mode->FrontendScreen = EFlickFrontendScreen::Paused;
	Pass(); Mode->UpdateTutorial(3); TestEqual(TEXT("Paused practice does not advance"), Practice->GetChallengeNumber(), 1);
	Mode->FrontendScreen = EFlickFrontendScreen::Playing; Mode->UpdateTutorial(3);
	TestEqual(TEXT("Practice resumes its result timer"), Practice->GetChallengeNumber(), 2);
	Mode->bNetworkMatchRequested = true; Mode->StartPractice(0, 0);
	TestEqual(TEXT("Network sessions cannot change to practice"), Practice->GetCategory(), 3);
	Mode->bNetworkMatchRequested = false; Practice->Stop();
	TestFalse(TEXT("Stopping exits practice"), Mode->IsPracticeMode()); TestNull(TEXT("Stopping removes marker"), Practice->Marker.Get());
	Mode->SetupTutorialStage(0);
	TestTrue(TEXT("Original guided tutorial remains usable"), Mode->IsTutorialMode() && !Mode->IsPracticeMode() && Mode->Pieces.Num() == 2);
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);

	// Exercise all sixty setups through real launches and Chaos, not just
	// injected scoring events. Search a small range of human-achievable powers.
	const auto PhysicsSettings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
	World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &PhysicsSettings);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
	Mode = World->SpawnActor<AFlickGameMode>(); Mode->GameState = State;
	Mode->SetActorTickEnabled(false);
	World->BeginPlay();
	Practice = Mode->GetPractice();
	const auto Step = [&]() { ++GFrameCounter; World->Tick(LEVELTICK_All, 1.f / 120.f); Mode->Tick(1.f / 120.f); };
	for (int32 Category = 0; Category < 4; ++Category)
	for (int32 Difficulty = 0; Difficulty < 3; ++Difficulty)
	{
		Mode->StartPractice(Category, Difficulty);
		for (int32 Shot = 0; Shot < 5; ++Shot)
		{
			bool bSolved = false;
			float LowPower = .05f, HighPower = .8f;
			for (int32 Attempt = 0; Attempt < (Category == 3 ? 44 : 12) && !bSolved; ++Attempt)
			{
				Practice->Challenge = Shot; Practice->Score = 0; Practice->Results.Reset(); Practice->bComplete = false; Practice->SetupChallenge();
				for (int32 Frame = 0; Frame < 36; ++Frame) Step();
				auto* Shooter = Mode->Pieces[0].Get();
				FVector AimPoint;
				if (Category == 0) AimPoint = FVector(Practice->ZoneCenter, Shooter->GetActorLocation().Z);
				else if (Category == 2) AimPoint = Mode->TestArenaActor->GetSwitchWorldCenter(Practice->SwitchIndex);
				else
				{
					auto* Target = Mode->Pieces[1].Get();
					const FVector Destination = Category == 3 ? Mode->BobArenaActor->GetPocketWorldLocation(Shot % 4)
						: FVector(Target->GetActorLocation().X, Target->GetActorLocation().Y, 0).GetSafeNormal() * (Mode->ArenaRadius + 100);
					FVector Outward = Destination - Target->GetActorLocation(); Outward.Z = 0; Outward.Normalize();
					AimPoint = Target->GetActorLocation() - Outward * (Shooter->GetPieceRadius() + Target->GetPieceRadius() - .5f);
				}
				FVector Direction = AimPoint - Shooter->GetActorLocation(); Direction.Z = 0; Direction.Normalize();
				const float Power = Category == 0 ? (LowPower + HighPower) * .5f : .12f + Attempt * (Category == 3 ? .02f : .08f);
				if (!TestTrue(TEXT("Practice launch accepted through normal gameplay"), Mode->TryLaunchPiece(Shooter, Direction, Power))) break;
				for (int32 Frame = 0; Frame < 1800 && State->MatchPhase == EFlickMatchPhase::ResolvingPhysics; ++Frame) Step();
				bSolved = Practice->GetResult(0) == 1;
				if (!bSolved && Category == 0)
				{
					if (!Shooter->IsActive() || FVector::DotProduct(AimPoint - Shooter->GetActorLocation(), Direction) < 0) HighPower = Power;
					else LowPower = Power;
				}
				if (bSolved) AddInfo(FString::Printf(TEXT("Physics-solvable: %s %s challenge %d, power %.3f"),
					FlickPracticeCatalog::Name(Category), FlickPracticeCatalog::DifficultyName(Difficulty), Shot + 1, Power));
			}
			TestTrue(*FString::Printf(TEXT("Physical solution exists: pack %d level %d challenge %d"), Category, Difficulty, Shot + 1), bSolved);
		}
	}
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
