#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Arena/FlickTestArena.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Pieces/FlickPiece.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickLineupRules.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Player/FlickCameraPawn.h"
#include "UnrealClient.h"

namespace
{
 UWorld* MakeCosmeticTestWorld()
 {
  const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
   .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
  UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
   ERHIFeatureLevel::Num, &Settings);
  if (World) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
  return World;
 }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickReplayCosmeticsTest, "FLICK.Replay.Cosmetics",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickReplayCosmeticsTest::RunTest(const FString& Parameters)
{
 UWorld* World = MakeCosmeticTestWorld();
 if (!TestNotNull(TEXT("Replay world"), World)) return false;
 auto* State = World->SpawnActor<AFlickGameState>();
 World->SetGameState(State);
 auto* Mode = World->SpawnActor<AFlickGameMode>();
 Mode->GameState = State;
 Mode->bTestArenaMode = true;
 State->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);
 auto* Piece = World->SpawnActor<AFlickPiece>();
 Piece->InitializePiece(EFlickTeam::Player1, 1, 45, 20, EFlickPieceArchetype::Standard);
 Piece->SetPuckEffects({9, 12, 13});
 Piece->EnableTestArenaVisuals();
 Mode->Pieces.Add(Piece);
 for (int32 Index = 0; Index < 2; ++Index)
 {
  FFlickRoundReplayFrame& Frame = Mode->RoundReplayFrames.AddDefaulted_GetRef();
  Frame.Time = Index;
  FFlickReplayPieceState& Puck = Frame.Pieces.AddDefaulted_GetRef();
  Puck.Piece = Piece;
  Puck.Transform = Piece->GetActorTransform();
  Puck.Transform.SetLocation(FVector(100 + Index * 300, 100, 20));
 }
 Mode->ResolutionElapsed = .75f;
 const FVector EventLocation(440, 100, 20);
 Mode->SpawnWorldFeedback(EventLocation, FLinearColor::Blue,
  EFlickFeedbackKind::Elimination, .85f, FVector::ForwardVector, 13);
 TestEqual(TEXT("One original cosmetic event recorded"), Mode->ReplayEffectEvents.Num(), 1);
 Piece->Eliminate();
 const FTransform LiveTransform = Piece->GetActorTransform();
 Mode->ResolutionEliminationTimes.Add(1, .75f);
 Mode->TestArenaActor = World->SpawnActor<AFlickTestArena>();
 AFlickWorldFeedback* LiveBurst = Mode->ReplayEffectEvents[0].LiveFeedback.Get();
 Mode->BeginCinematicRoundReplay(EFlickMatchOutcome::Player1Wins);
 TestTrue(TEXT("Real replay entry succeeds"), Mode->bCinematicReplayActive);
 TestTrue(TEXT("Live burst removed before rewind"), !IsValid(LiveBurst) || LiveBurst->IsActorBeingDestroyed());
 Mode->ApplyCinematicReplayTime(0);
 for (int32 Index = 1; Index <= 30; ++Index)
 {
  Mode->ApplyCinematicReplayTime(Index / 60.0f);
  Piece->Tick(1.0f / 30.0f); // source time advances at half real speed
 }
 TestFalse(TEXT("Eliminated gameplay state is preserved"), Piece->IsActive());
 TestFalse(TEXT("Rewound puck is visible"), Piece->IsHidden());
 TestTrue(TEXT("Detailed eliminated puck is visible during replay"), Piece->GetPresentationMesh()->IsVisible());
 const auto Components = TInlineComponentArray<UProceduralMeshComponent*>(Piece);
 TestTrue(TEXT("Replayed eliminated puck produces its trail"),
  Components.Num() == 1 && Components[0]->IsVisible() && Components[0]->GetNumSections() == 2);
 TestFalse(TEXT("Future knockout is not presented early"), Mode->ReplayEffectEvents[0].Playback.IsValid());
 Mode->ApplyCinematicReplayTime(.8f);
 AFlickWorldFeedback* Burst = Mode->ReplayEffectEvents[0].Playback.Get();
 if (TestNotNull(TEXT("Knockout reappears at original event time"), Burst))
 {
  TestEqual(TEXT("Original equipped knockout retained"), Burst->Appearance.Style, 13);
  TestTrue(TEXT("Original event location retained"), Burst->GetActorLocation().Equals(EventLocation));
  TestTrue(TEXT("Original strength retained"), FMath::IsNearlyEqual(Burst->Appearance.Strength, .85f));
  Burst->Tick(.5f);
  TestTrue(TEXT("Real time does not advance a replay burst"), FMath::IsNearlyEqual(Burst->Age, .05f, KINDA_SMALL_NUMBER));
  Mode->ApplyCinematicReplayTime(.9f);
  TestTrue(TEXT("Same burst reused without duplicates"), Mode->ReplayEffectEvents[0].Playback.Get() == Burst);
  TestTrue(TEXT("Burst follows source time"), FMath::IsNearlyEqual(Burst->Age, .15f, KINDA_SMALL_NUMBER));
 }
 Mode->FinishCinematicRoundReplay(false);
 TestTrue(TEXT("Finishing/skipping clears replay event track"), Mode->ReplayEffectEvents.IsEmpty());
 TestTrue(TEXT("Replay burst destroyed on exit"), !IsValid(Burst) || Burst->IsActorBeingDestroyed());
 TestTrue(TEXT("Live transform restored"), Piece->GetActorTransform().Equals(LiveTransform));
 TestTrue(TEXT("Eliminated puck hidden again"), Piece->IsHidden());
 TestFalse(TEXT("Detailed eliminated puck hidden on exit"), Piece->GetPresentationMesh()->IsVisible());
 TestEqual(TEXT("Original trail remains equipped"), Piece->GetPuckEffect(0), 9);
 World->DestroyWorld(false);
 GEngine->DestroyWorldContext(World);
 return true;
}

// Opt-in real viewport review; the ordinary automation suite does not alter
// an active match or produce screenshots.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickReplayCosmeticReviewTest, "FLICK.Replay.CosmeticReview",
 EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FFlickReplayCosmeticReviewTest::RunTest(const FString& Parameters)
{
 if (!FParse::Param(FCommandLine::Get(), TEXT("FlickReplayCosmeticReview"))) return true;
 auto ModeRef = MakeShared<TWeakObjectPtr<AFlickGameMode>>();
 ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, ModeRef]()
 {
  for (const auto& Context : GEngine->GetWorldContexts())
   if (Context.WorldType == EWorldType::Game && Context.World())
    *ModeRef = Cast<AFlickGameMode>(Context.World()->GetAuthGameMode());
  AFlickGameMode* Mode = ModeRef->Get();
  if (!TestNotNull(TEXT("Real review match"), Mode) || !Mode->TestArenaActor) return;
  Mode->SetActorTickEnabled(false);
  Mode->DestroyPieces();
  AFlickGameState* State = Mode->GetFlickGameState();
  State->SetMatchPhase(EFlickMatchPhase::ResolvingPhysics);
  State->SetPuckArrivalState(false, 1.2f);
  Mode->FrontendScreen = EFlickFrontendScreen::Playing;
  Mode->bTestArenaMode = true;
  Mode->RoundReplayFrames.Reset();
  Mode->ReplayEffectEvents.Reset();
  Mode->bReplayPlayedForResolution = false;
  for (int32 Index = 0; Index < 2; ++Index)
  {
   auto* Puck = Mode->SpawnPiece(Index == 0 ? EFlickTeam::Player1 : EFlickTeam::Player2,
    Index + 1, FVector(-140, Index * 210 - 105, Mode->ArenaSurfaceZ + 20), EFlickPieceArchetype::Standard);
   Puck->SetPuckSkin(Index == 0 ? 3 : 0);
   Puck->SetPuckEffects({Index == 0 ? 9 : 4, 0, 13});
   Puck->SetActorTickEnabled(false);
  }
  for (int32 Index = 0; Index < 2; ++Index)
  {
   auto& Frame = Mode->RoundReplayFrames.AddDefaulted_GetRef();
   Frame.Time = Index;
   for (AFlickPiece* Puck : Mode->Pieces)
   {
    auto& Entry = Frame.Pieces.AddDefaulted_GetRef();
    Entry.Piece = Puck;
    Entry.Transform = Puck->GetActorTransform();
    Entry.Transform.AddToTranslation(FVector(Index * 360, 0, 0));
   }
  }
  Mode->ResolutionElapsed = .75f;
  Mode->SpawnWorldFeedback(FVector(130, 105, Mode->ArenaSurfaceZ + 20), FLinearColor::Blue,
   EFlickFeedbackKind::Elimination, 1, FVector::ForwardVector, 13);
  Mode->Pieces[1]->Eliminate();
  Mode->ResolutionEliminationTimes.Reset();
  Mode->ResolutionEliminationTimes.Add(2, .75f);
  Mode->BeginCinematicRoundReplay(EFlickMatchOutcome::Player1Wins);
  if (Mode->CameraPawn)
  {
   Mode->CameraPawn->BeginCinematicReplay(FVector(80, 0, Mode->ArenaSurfaceZ + 20), EFlickTeam::Player1);
   Mode->CameraPawn->UpdateCinematicReplay(FVector(80, 0, Mode->ArenaSurfaceZ + 20), .5f, .25f);
  }
 }, 4.f));
 for (int32 Phase = 0; Phase < 2; ++Phase)
 {
  ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([ModeRef, Phase]()
  {
   if (auto* Mode = ModeRef->Get())
   {
    const int32 First = Phase == 0 ? 0 : 31;
    const int32 Last = Phase == 0 ? 30 : 54;
    for (int32 Frame = First; Frame <= Last; ++Frame)
    {
     Mode->ApplyCinematicReplayTime(Frame / 60.f);
     for (AFlickPiece* Puck : Mode->Pieces) Puck->Tick(1.f / 30.f);
    }
   }
  }, .1f));
  ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Phase]()
  {
   FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() /
    FString::Printf(TEXT("ReplayCosmeticReview/phase-%d.png"), Phase), false, false);
  }, 1.f));
 }
 ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([ModeRef]()
 {
  if (auto* Mode = ModeRef->Get()) Mode->FinishCinematicRoundReplay(false);
 }, 1.f));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBotLoadoutsTest, "FLICK.Bots.MatchLoadouts",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickBotLoadoutsTest::RunTest(const FString& Parameters)
{
 UWorld* World = MakeCosmeticTestWorld();
 if (!TestNotNull(TEXT("Bot world"), World)) return false;
 auto* Mode = World->SpawnActor<AFlickGameMode>();
 Mode->bTrainingMode = true;
 Mode->bTrainingBotMatch = true;
 Mode->bPlayerClassesActiveForMatch = true;
 Mode->ResetBotLoadouts();
 TestFalse(TEXT("Human seat is never randomized"), Mode->IsBotLoadoutSeat(EFlickTeam::Player1, 0));
 TestTrue(TEXT("Training opponent gets bot loadout"), Mode->IsBotLoadoutSeat(EFlickTeam::Player2, 0));
 const FFlickBotLoadout First = Mode->GetBotLoadout(EFlickTeam::Player2, 0);
 for (int32 Round = 0; Round < 5; ++Round)
 {
  const FFlickBotLoadout& Same = Mode->GetBotLoadout(EFlickTeam::Player2, 0);
  TestEqual(TEXT("Skin stays fixed through series"), Same.Skin, First.Skin);
  TestTrue(TEXT("Effects stay fixed through series"), Same.Effects == First.Effects);
  TestEqual(TEXT("Lineup stays fixed through series"), Same.Lineup, First.Lineup);
  TestTrue(TEXT("Bot uses a valid four-puck preset"),
   FlickLineupRules::IsValid(FlickPieceArchetypeRules::GetPreset(Same.Lineup)));
  auto* Puck = Mode->SpawnPiece(EFlickTeam::Player2, 10 + Round, FVector(100, 100, 20),
   Mode->GetPlayerClassPiece(EFlickTeam::Player2, 0, 0), false, 0);
  TestEqual(TEXT("Respawn receives cached skin"), Puck->GetPuckSkin(), First.Skin);
  for (int32 Effect = 0; Effect < 3; ++Effect)
   TestEqual(TEXT("Respawn receives cached effects"), Puck->GetPuckEffect(Effect), First.Effects[Effect]);
 }
 TSet<FString> Themes;
 for (int32 Seat = 0; Seat < 6; ++Seat)
 {
  const FFlickBotLoadout& Loadout = Mode->GetBotLoadout(Seat < 3 ? EFlickTeam::Player2 : EFlickTeam::Player1, Seat % 3);
  Themes.Add(FString::Printf(TEXT("%d,%d,%d,%d"), Loadout.Skin,
   Loadout.Effects[0], Loadout.Effects[1], Loadout.Effects[2]));
  TestTrue(TEXT("Valid skin"), Loadout.Skin >= 0 && Loadout.Skin < 6);
  for (int32 Effect = 0; Effect < 3; ++Effect)
   TestTrue(TEXT("Valid active effect (zero is Basic for knockouts)"), Loadout.Effects[Effect] >= (Effect == 2 ? 0 : 1)
    && FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::TrailCategory + Effect).IsValidIndex(Loadout.Effects[Effect]));
 }
 TestEqual(TEXT("All six bot seats receive distinct coordinated themes"), Themes.Num(), 6);
 for (int32 Theme = 0; Theme < 20; ++Theme)
 {
  Mode->BotLoadouts.Reset();
  Mode->BotThemeOffset = Theme;
  const auto& Loadout = Mode->GetBotLoadout(EFlickTeam::Player2, 0);
  for (int32 Effect = 0; Effect < 3; ++Effect)
   TestTrue(TEXT("Every coordinated theme uses a catalog effect"),
    FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::TrailCategory + Effect).IsValidIndex(Loadout.Effects[Effect]));
 }
 Mode->bTutorialMode = true;
 TestFalse(TEXT("Prescribed tutorials/packs excluded"), Mode->IsBotLoadoutSeat(EFlickTeam::Player2, 0));
 Mode->bTutorialMode = false;
 Mode->bTrainingBotMatch = false;
 TestFalse(TEXT("Normal free play excluded"), Mode->IsBotLoadoutSeat(EFlickTeam::Player2, 0));
 Mode->bPrivateMatchActive = true;
 TestTrue(TEXT("Unoccupied private seats use bot loadouts"), Mode->IsBotLoadoutSeat(EFlickTeam::Player2, 0));
 World->DestroyWorld(false);
 GEngine->DestroyWorldContext(World);
 return true;
}
#endif
