#if WITH_DEV_AUTOMATION_TESTS
#include "Core/FlickAchievementProgress.h"
#include "Game/FlickAchievementTracker.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickAchievementRulesTest, "FLICK.Achievements.ProgressRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickAchievementRulesTest::RunTest(const FString&)
{
    TestEqual(TEXT("Catalogue contains all 48 created achievements"), FlickAchievements::Count, 48);
    TSet<FString> Ids, Stats;
    for (const auto& D : FlickAchievements::Definitions)
    {
        Ids.Add(D.Api);
        if (D.Goal == 1) TestTrue(TEXT("Binary unlocks have no Steam progress counter"), *D.Stat == 0);
        else if (*D.Stat) Stats.Add(D.Stat);
    }
    TestEqual(TEXT("Achievement IDs are unique"), Ids.Num(), 48);
    TestEqual(TEXT("Progress uses 22 published shared counters"), Stats.Num(), 22);
    TestTrue(TEXT("Corner Office unlocks without a Steam progress bar"), *FlickAchievements::Get(FlickAchievements::EId::CornerOffice).Stat == 0);
    TestTrue(TEXT("Rank and File unlocks without a Steam progress bar"), *FlickAchievements::Get(FlickAchievements::EId::RankAndFile).Stat == 0);
    FFlickAchievementProgress P;
    FFlickAchievementReport R;
    R.Values[FlickAchievements::Index(FlickAchievements::EId::FindYourAngle)] = 12; P.Apply(R);
    R.Values[FlickAchievements::Index(FlickAchievements::EId::FindYourAngle)] = 13; P.Apply(R);
    TestEqual(TEXT("Shots accumulate to the exact target"), P.Get(FlickAchievements::EId::FindYourAngle), 25);
    P.Apply(R); TestEqual(TEXT("Counters saturate at the schema maximum"), P.Get(FlickAchievements::EId::FindYourAngle), 25);
    R.Values[FlickAchievements::Index(FlickAchievements::EId::FindYourAngle)] = -100; P.Apply(R);
    TestEqual(TEXT("Negative reports cannot erase progress"), P.Get(FlickAchievements::EId::FindYourAngle), 25);
    P.Best(FlickAchievements::EId::CornerOffice, 3); P.Best(FlickAchievements::EId::CornerOffice, 2);
    TestEqual(TEXT("Corners in separate matches never sum into a false unlock"), P.Get(FlickAchievements::EId::CornerOffice), 3);
    P.RecordPlacements(0, 3); P.RecordPlacements(1, 3);
    TestEqual(TEXT("Partial placements cannot add across queues"), P.Get(FlickAchievements::EId::RankAndFile), 3);
    for (int32 Queue = 0; Queue < 4; ++Queue) { P.RecordPlacements(Queue, 5); P.RecordPlacements(Queue, 5); P.RecordRankedWin(Queue); P.RecordRankedWin(Queue); }
    TestEqual(TEXT("All four unique placed playlists count once"), P.Get(FlickAchievements::EId::FourOfAKind), 4);
    TestEqual(TEXT("Ranked wins count each playlist once"), P.Get(FlickAchievements::EId::AllRounder), 4);
    for (int32 C = 0; C < 4; ++C) for (int32 D = 0; D < 3; ++D) P.RecordPractice(C, D, 5);
    P.RecordPractice(0,0,5);
    TestEqual(TEXT("Repeated practice does not duplicate unique mastery"), P.Get(FlickAchievements::EId::MasterTheAngle), 12);
    TestEqual(TEXT("Foundation mastery is independent"), P.Get(FlickAchievements::EId::FoundationSolid), 4);
    TestEqual(TEXT("Skilled mastery is independent"), P.Get(FlickAchievements::EId::SkillSet), 4);
    TestEqual(TEXT("Expert mastery is independent"), P.Get(FlickAchievements::EId::ExpertJudgement), 4);
    TestEqual(TEXT("Completed practice runs are cumulative"), P.Get(FlickAchievements::EId::PracticeMakesPermanent), 13);
    P.RecordPractice(-1,0,5); TestEqual(TEXT("Invalid practice cannot add a run"), P.Get(FlickAchievements::EId::PracticeMakesPermanent), 13);
    TestEqual(TEXT("Perfect Expert run directly unlocks its binary achievement"), P.Get(FlickAchievements::EId::TheHardWay), 1);
    R.ArchetypeMask = 511; P.Apply(R); P.Apply(R);
    TestEqual(TEXT("Archetypes merge as a set"), P.Get(FlickAchievements::EId::FullSpectrum), 9);
    TestEqual(TEXT("New offline deltas are added to newer cloud progress"), FlickAchievements::MergeRemoteCounter(13,3,20,100), 23);
    TestEqual(TEXT("Local acknowledged progress is not added twice"), FlickAchievements::MergeRemoteCounter(20,0,20,100), 20);
    TestEqual(TEXT("Steam acknowledgements retain events earned while upload was in flight"), FlickAchievements::AcknowledgeCounter(7,4), 3);
    TestEqual(TEXT("Corrupt acknowledgements cannot make negative pending values"), FlickAchievements::AcknowledgeCounter(2,4), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickAchievementGameplayTest, "FLICK.Achievements.AuthorityEvents", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickAchievementGameplayTest::RunTest(const FString&)
{
    const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Achievement authority world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* State = World->SpawnActor<AFlickGameState>(); World->SetGameState(State);
    auto* Mode = World->SpawnActor<AFlickGameMode>(); Mode->GameState = State;
    Mode->FrontendScreen = EFlickFrontendScreen::Playing;
    auto* Tracker = Mode->GetAchievementTracker();
    // Deliberately enable just this in-memory fixture. Real unattended runs are excluded.
    Tracker->bEnabled = true;
    Tracker->Players.FindOrAdd(0).Report.EventId = FGuid::NewGuid();
    Tracker->Players.FindOrAdd(1).Report.EventId = FGuid::NewGuid();
    auto* Heavy = Mode->SpawnPiece(EFlickTeam::Player1, 1, FVector(0,0,300), EFlickPieceArchetype::Heavy);
    auto* Enemy = Mode->SpawnPiece(EFlickTeam::Player2, 2, FVector(200,0,300), EFlickPieceArchetype::Standard);
    auto* Other = Mode->SpawnPiece(EFlickTeam::Player2, 3, FVector(400,0,300), EFlickPieceArchetype::Standard);
    auto* Third = Mode->SpawnPiece(EFlickTeam::Player2, 5, FVector(600,0,300), EFlickPieceArchetype::Standard);
    Tracker->ObserveTurn(EFlickTeam::Player1);
    TestTrue(TEXT("One-versus-three comeback eligibility starts at turn entry, before a shot"), Tracker->ComebackEligible[0]);
    Third->Destroy();
    Tracker->BeginResolution(); Tracker->ShotLaunched(Heavy);
    Tracker->OpponentKnockedOut(Enemy, EFlickTeam::Player1, 0);
    Tracker->OpponentKnockedOut(Enemy, EFlickTeam::Player1, 0);
    Tracker->OpponentKnockedOut(Other, EFlickTeam::Player1, 0);
    Tracker->ResolutionFinished(); Tracker->ResolutionFinished();
    const auto Read = [Tracker](FlickAchievements::EId Id) { return Tracker->Players[0].Report.Values[FlickAchievements::Index(Id)]; };
    TestEqual(TEXT("Two actual opponents credit the Heavy achievement"), Read(FlickAchievements::EId::HeavyHitter), 1);
    TestEqual(TEXT("Double-knockout shot counted once despite repeated resolution callbacks"), Read(FlickAchievements::EId::DoubleTrouble), 1);
    TestEqual(TEXT("A repeated elimination event cannot duplicate an opponent"), Read(FlickAchievements::EId::TableControl), 2);
    TestEqual(TEXT("Another teammate receives no shooter's personal progress"), Tracker->Players[1].Report.Values[FlickAchievements::Index(FlickAchievements::EId::TableControl)], 0);
    Mode->bCinematicReplayActive = true;
    Tracker->BeginResolution(); Tracker->ShotLaunched(Heavy); Tracker->OpponentKnockedOut(Enemy, EFlickTeam::Player1, 0);
    TestEqual(TEXT("Replay physics cannot award a shot"), Read(FlickAchievements::EId::FindYourAngle), 1);
    TestEqual(TEXT("Replay physics cannot award an elimination"), Read(FlickAchievements::EId::TableControl), 2);
    Mode->bCinematicReplayActive = false; Mode->ActiveMatchVariant = EFlickMatchVariant::Bob;
    Tracker->Players[0].Shots = 0;
    Tracker->BeginResolution(); Tracker->ShotLaunched(Heavy);
    Tracker->BankedCoins.Add(Enemy->GetPieceId());
    // Coin ownership, not the opposing shooter, determines BOB progress.
    Enemy->InitializePiece(EFlickTeam::Player1,2,24,10,EFlickPieceArchetype::Standard,false,0);
    Other->InitializePiece(EFlickTeam::Player1,3,24,10,EFlickPieceArchetype::Standard,false,0);
    Tracker->BobPocket(Enemy,0); Tracker->BobPocket(Enemy,0); Tracker->BobPocket(Other,1);
    Tracker->ResolutionFinished();
    TestEqual(TEXT("Striker-free objective coins count exactly once"), Read(FlickAchievements::EId::PocketCollection), 2);
    TestEqual(TEXT("Two pockets on one shot unlock Double Deposit"), Read(FlickAchievements::EId::DoubleDeposit), 1);
    TestEqual(TEXT("First BOB shot earns the opening-shot goal"), Read(FlickAchievements::EId::Breakthrough), 1);
    TestEqual(TEXT("A rail contact before a real pocket unlocks Bank Statement"), Read(FlickAchievements::EId::BankStatement), 1);
    TestEqual(TEXT("Distinct holes are tracked within the match"), Read(FlickAchievements::EId::CornerOffice), 2);
    Tracker->BeginResolution();
    Other->InitializePiece(EFlickTeam::Player1,4,24,10,EFlickPieceArchetype::Standard,true,0);
    Tracker->BobPocket(Other,2);
    TestEqual(TEXT("A striker pocket is excluded from objective coin counters"), Read(FlickAchievements::EId::PocketCollection), 2);
    TestTrue(TEXT("A striker pocket invalidates a clean BOB win"), Tracker->Players[0].bStrikerPocketed);
    return true;
}
#endif
