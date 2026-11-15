#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickProgression.h"
#include "Game/FlickProgressionSubsystem.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickProgressionTest,"FLICK.Profile.AccountProgression",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlickProgressionTest::RunTest(const FString&)
{
 using namespace FlickProgression;
 TestEqual(TEXT("New accounts start at level one"),Level(0).Number,1);
 TestEqual(TEXT("Level boundary"),Level(1000).Number,2);
 TestEqual(TEXT("Boundary starts an empty bar"),Level(1000).Progress,0);
 TestEqual(TEXT("Level twenty-one plateau"),Level(39000).Number,21);
 TestEqual(TEXT("Plateau cost"),Level(39000).Required,3000);
 TestEqual(TEXT("Long-term levels remain available"),Level(39000+3000000).Number,1021);
 TestEqual(TEXT("Negative XP clamped"),Level(-5).Number,1);
 FFlickCareerEvent E;E.Id=TEXT("match-a");E.bEligible=true;E.Seconds=600;E.Shots=10;E.bWon=true;E.Accolades.Init(100,11);
 RewardMatch(E);TestEqual(TEXT("Full match reward cap"),E.XP(),700);
 E.Playlist=2;RewardMatch(E);TestEqual(TEXT("Bots earn half"),E.XP(),350);
 E.bEligible=false;RewardMatch(E);TestEqual(TEXT("Private and free play pay no match XP"),E.XP(),0);
 E.bEligible=true;E.Shots=0;RewardMatch(E);TestEqual(TEXT("No XP for inactive player"),E.XP(),0);
 E.Shots=5;E.Seconds=9;RewardMatch(E);TestEqual(TEXT("No fast completion farming"),E.XP(),0);
 E.Seconds=180;E.Playlist=0;E.bWon=false;E.Accolades.Reset();E.Format=3;E.Pockets=3;E.MultiPockets=1;
 RewardMatch(E);TestEqual(TEXT("BOB earns style from pockets"),E.StyleXP,45);
 TestEqual(TEXT("Active time uses seconds, not frames"),E.TimeXP,75);
 auto* A=NewObject<UFlickProgressionSubsystem>(NewObject<UFlickGameInstance>());A->bPreview=true;A->Data=NewObject<UFlickCareerSave>(A);
 TestTrue(TEXT("First authority report accepted"),A->Add(E));const int64 XP=A->TotalXP;
 TestFalse(TEXT("Duplicate authority report rejected"),A->Add(E));TestEqual(TEXT("Duplicate cannot increase XP"),A->TotalXP,XP);
 auto* B=NewObject<UFlickCareerSave>();B->Events.Add(E);
 auto Other=E;Other.Id=TEXT("offline-device-match");B->Events.Add(Other);
 TestTrue(TEXT("Offline device journal merges new match"),A->Merge(B));
 TestEqual(TEXT("Cloud overlap counted once"),A->TotalXP,XP*2);
 TestFalse(TEXT("Repeated sync is idempotent"),A->Merge(B));
 B->bHasLegacy=true;B->LegacyCasual.MatchesPlayed=20;B->LegacyCasual.Wins=12;
 TestTrue(TEXT("Late cloud connection restores historical totals"),A->Merge(B));
 TestEqual(TEXT("Historical career snapshot restored"),A->Data->LegacyCasual.MatchesPlayed,20);
 TestFalse(TEXT("Historical snapshot cannot be added twice"),A->Merge(B));
 TestEqual(TEXT("Legacy totals never double on repeated sync"),A->Data->LegacyCasual.MatchesPlayed,20);

 FFlickCareerEvent P;P.Kind=1;P.Id=TEXT("Practice-0-0");P.BonusXP=150;
 TestTrue(TEXT("First practice completion rewarded"),A->Add(P));TestFalse(TEXT("Practice farming blocked"),A->Add(P));
 P.Kind=2;P.Id=TEXT("Tutorial");P.BonusXP=250;A->Add(P);TestFalse(TEXT("Tutorial awarded once"),A->Add(P));
 P.Kind=3;P.Id=TEXT("Challenge-0");A->Add(P);TestFalse(TEXT("Challenge awarded once"),A->Add(P));
 auto* Isolated=NewObject<UFlickProgressionSubsystem>(NewObject<UFlickGameInstance>());Isolated->Data=NewObject<UFlickCareerSave>(Isolated);
 TestEqual(TEXT("Separate account starts without another account XP"),Isolated->TotalXP,int64(0));
 TArray<uint8> Bytes;TestTrue(TEXT("Career serializes for disk/cloud"),UGameplayStatics::SaveGameToMemory(A->Data,Bytes));
 auto* Loaded=Cast<UFlickCareerSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
 if(TestNotNull(TEXT("Career reload"),Loaded))
 {
  TestEqual(TEXT("Save version preserved"),Loaded->Version,1);
  TestEqual(TEXT("All award IDs preserved"),Loaded->Events.Num(),A->Data->Events.Num());
  TestTrue(TEXT("New device can restore journal"),Isolated->Merge(Loaded));TestEqual(TEXT("Lifetime XP survives save/load"),Isolated->TotalXP,A->TotalXP);
 }
 return true;
}
#endif
