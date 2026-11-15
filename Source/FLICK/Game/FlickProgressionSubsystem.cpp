#include "Game/FlickProgressionSubsystem.h"
#include "Core/FlickChallengeCatalog.h"
#include "Core/FlickPracticeCatalog.h"
#include "Core/FlickLog.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#if WITH_FLICK_STEAMWORKS
#include "steam/steam_api.h"
#endif

void UFlickProgressionSubsystem::Initialize(FSubsystemCollectionBase& C)
{
 Super::Initialize(C);
 bPreview=FApp::IsUnattended() || IsRunningCommandlet() || IsRunningDedicatedServer();
 Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UFlickProgressionSubsystem::Tick),30.f);
}
void UFlickProgressionSubsystem::Deinitialize()
{
 if(Data && !bPreview) { Save(); SyncCloud(); }
 FTSTicker::GetCoreTicker().RemoveTicker(Ticker); Super::Deinitialize();
}
void UFlickProgressionSubsystem::SelectAccount()
{
 FString Id;
 if(!FParse::Param(FCommandLine::Get(),TEXT("nosteam")))
  if(auto* OSS=IOnlineSubsystem::Get(FName(TEXT("STEAM"))))
   if(auto Identity=OSS->GetIdentityInterface(); Identity.IsValid())
    if(auto User=Identity->GetUniquePlayerId(0); User.IsValid()) Id=User->ToString();
 if(Id.IsEmpty()) Id=Account.IsEmpty() ? TEXT("Local") : Account;
 if(Data && Id==Account) return;
 if(Data && !bPreview) Save();
 Account=Id; Slot=TEXT("Career-")+Id; TotalXP=0; CachedCount=-1; Seen.Reset(); ClearLastAward(); bSynced=false;
 Data=UGameplayStatics::DoesSaveGameExist(Slot,0)?Cast<UFlickCareerSave>(UGameplayStatics::LoadGameFromSlot(Slot,0)):nullptr;
 if(!Data && UGameplayStatics::DoesSaveGameExist(Slot+TEXT("-backup"),0)) Data=Cast<UFlickCareerSave>(UGameplayStatics::LoadGameFromSlot(Slot+TEXT("-backup"),0));
 if(!Data) Data=NewObject<UFlickCareerSave>(this);
 // Rebuild a monotonic total from unique events, not a client-settable level.
 TArray<FFlickCareerEvent> Loaded=MoveTemp(Data->Events); Data->Events.Reset();
 for(const auto& E:Loaded) Add(E);
 if(!bPreview) SyncCloud();
 if(!Data->bHasLegacy)
 {
  FString Owner; if(GConfig) GConfig->GetString(TEXT("FLICK.Progression"),TEXT("LegacyOwner"),Owner,GGameUserSettingsIni);
  if(Owner.IsEmpty())
  {
   if(const auto* GI=Cast<UFlickGameInstance>(GetGameInstance()))
   {
    Data->LegacyCasual=GI->GetProfileStatsForView(EFlickProfileStatsView::Casual);
    Data->LegacyCompetitive=GI->GetProfileStatsForView(EFlickProfileStatsView::Competitive);
    for(int32 C=0;C<4;++C) for(int32 D=0;D<3;++D) Data->LegacyPractice.Add(FlickPracticeCatalog::BestScore(C,D));
   }
   if(!bPreview && GConfig) { GConfig->SetString(TEXT("FLICK.Progression"),TEXT("LegacyOwner"),*Account,GGameUserSettingsIni); GConfig->Flush(false,GGameUserSettingsIni); }
  }
  Data->bHasLegacy=true; bDirty=true;
 }
#if !UE_BUILD_SHIPPING
 if(bPreview && FParse::Param(FCommandLine::Get(),TEXT("FlickCareerPreview")))
 {
  FFlickCareerEvent Base; Base.Kind=2;Base.Id=TEXT("PreviewHistory");Base.BonusXP=10500;Add(Base);
  for(int32 I=0;I<24;++I) { FFlickCareerEvent E;E.Id=FString::Printf(TEXT("Preview-%d"),I);E.bEligible=true;E.Playlist=I%3;E.Format=I%4;E.Seconds=300+I*8;E.Shots=8;E.bWon=I%3!=0;E.Points=400;E.Knockouts=E.Format==3?0:3;E.Pockets=E.Format==3?5:0;E.Accolades.Init(0,FlickAccoladeCount);E.Accolades[I%FlickAccoladeCount]=1;FlickProgression::RewardMatch(E);Add(E); }
  AwardStartXP=GetXP(); FFlickCareerEvent E;E.Id=TEXT("post-match-preview");E.bEligible=true;E.Seconds=420;E.Shots=10;E.bWon=true;FlickProgression::RewardMatch(E);Add(E);LastAward=E;LastAwardAt=FPlatformTime::Seconds();
 }
#endif
 if(!bPreview) Save();
}
bool UFlickProgressionSubsystem::Add(const FFlickCareerEvent& E)
{
 if(!Data || E.Id.IsEmpty() || Seen.Contains(E.Id) || E.Kind<0 || E.Kind>3 || E.XP()<0 || E.XP()>100000) return false;
 if(E.Kind==0 && (!E.bEligible || E.Playlist<0 || E.Playlist>2 || E.Format<0 || E.Format>3)) return false;
 Seen.Add(E.Id); Data->Events.Add(E); TotalXP+=E.XP(); bDirty=true; return true;
}
bool UFlickProgressionSubsystem::Merge(const UFlickCareerSave* Other)
{
 if(!Other || Other->Version!=1) return false;
 bool Changed=false;
 for(const auto& E:Other->Events) Changed=Add(E)||Changed;
 if(Other->bHasLegacy)
 {
  // Historical snapshots can overlap across devices: take monotonic maxima,
  // never add snapshots. New results are merged exclusively by event ID.
  const auto MergeStats=[&Changed](FFlickProfileStats& L,const FFlickProfileStats& R)
  {
   int32* A[]={&L.MatchesPlayed,&L.Wins,&L.Points,&L.Shots,&L.Knockouts,&L.DoubleKnockouts};
   const int32 B[]={R.MatchesPlayed,R.Wins,R.Points,R.Shots,R.Knockouts,R.DoubleKnockouts};
   for(int32 I=0;I<6;++I) if(*A[I]<B[I]) { *A[I]=B[I];Changed=true; }
   if(L.AccoladeCounts.Num()<FlickAccoladeCount) L.AccoladeCounts.SetNumZeroed(FlickAccoladeCount);
   for(int32 I=0;I<FMath::Min(R.AccoladeCounts.Num(),FlickAccoladeCount);++I) if(L.AccoladeCounts[I]<R.AccoladeCounts[I]) { L.AccoladeCounts[I]=R.AccoladeCounts[I];Changed=true; }
  };
  MergeStats(Data->LegacyCasual,Other->LegacyCasual);MergeStats(Data->LegacyCompetitive,Other->LegacyCompetitive);
  if(Data->LegacyPractice.Num()<12) Data->LegacyPractice.SetNumZeroed(12);
  for(int32 I=0;I<FMath::Min(12,Other->LegacyPractice.Num());++I) if(Data->LegacyPractice[I]<Other->LegacyPractice[I]) {Data->LegacyPractice[I]=Other->LegacyPractice[I];Changed=true;}
  if(!Data->bHasLegacy) { Data->bHasLegacy=true;Changed=true; }
  if(Changed) CachedCount=-1;
 }
 bDirty|=Changed; return Changed;
}
void UFlickProgressionSubsystem::Save()
{
 if(!Data || bPreview || !bDirty) return;
 if(!UGameplayStatics::SaveGameToSlot(Data,Slot+TEXT("-backup"),0)) UE_LOG(LogFlick,Warning,TEXT("Career backup save failed: %s"),*Slot);
 if(!UGameplayStatics::SaveGameToSlot(Data,Slot,0)) { UE_LOG(LogFlick,Error,TEXT("Career save failed: %s"),*Slot); return; }
 bDirty=false;
}
void UFlickProgressionSubsystem::SyncCloud()
{
 if(bPreview || !Data || Account==TEXT("Local")) return;
#if WITH_FLICK_STEAMWORKS
 auto* Cloud=SteamRemoteStorage();
 if(!Cloud || !SteamUser() || !SteamUser()->BLoggedOn() || !SteamUtils() || SteamUtils()->GetAppID()!=5548920
  || !Cloud->IsCloudEnabledForAccount() || !Cloud->IsCloudEnabledForApp()) { SyncStatus=TEXT("SAVED LOCALLY // CLOUD UNAVAILABLE"); return; }
 const FString Name=Slot+TEXT(".sav"); const auto File=StringCast<ANSICHAR>(*Name);
 const int32 Size=Cloud->GetFileSize(File.Get());
 if(Size>0 && Size<=32*1024*1024)
 {
  TArray<uint8> Bytes; Bytes.SetNumUninitialized(Size);
  if(Cloud->FileRead(File.Get(),Bytes.GetData(),Size)!=Size) { SyncStatus=TEXT("SAVED LOCALLY // CLOUD RETRY PENDING"); return; }
  auto* Remote=Cast<UFlickCareerSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
  if(!Remote || Remote->Version!=1) { SyncStatus=TEXT("SAVED LOCALLY // CLOUD VERSION UNAVAILABLE"); return; }
  Merge(Remote); Save();
 }
 TArray<uint8> Bytes;
 if(UGameplayStatics::SaveGameToMemory(Data,Bytes) && Cloud->FileWrite(File.Get(),Bytes.GetData(),Bytes.Num())) { SyncStatus=TEXT("STEAM CLOUD SYNCED"); bSynced=true; }
 else SyncStatus=TEXT("SAVED LOCALLY // CLOUD RETRY PENDING");
#endif
}
bool UFlickProgressionSubsystem::Tick(float) { if(!bPreview) { SelectAccount(); Save(); SyncCloud(); } return true; }
int64 UFlickProgressionSubsystem::GetXP() { SelectAccount(); return TotalXP; }
const TArray<FFlickCareerEvent>& UFlickProgressionSubsystem::GetEvents() { SelectAccount(); return Data->Events; }
FFlickProfileStats UFlickProgressionSubsystem::GetLegacy(int32 View)
{
 SelectAccount(); FFlickProfileStats S;
 for(int32 P=0;P<2;++P) if(View==0 || View==P+1)
 {
  const auto& L=P==0 ? Data->LegacyCasual : Data->LegacyCompetitive;
  S.MatchesPlayed+=L.MatchesPlayed; S.Wins+=L.Wins; S.Points+=L.Points; S.Shots+=L.Shots; S.Knockouts+=L.Knockouts; S.DoubleKnockouts+=L.DoubleKnockouts; S.RecordAccolades(L.AccoladeCounts);
 }
 return S;
}
FFlickProfileStats UFlickProgressionSubsystem::GetEligibleStats(int32 View)
{
 SelectAccount();
 if(CachedCount!=Data->Events.Num())
 {
  for(int32 V=0;V<3;++V) EligibleCache[V]=GetLegacy(V);
  for(const auto& E:Data->Events) if(E.Kind==0 && E.Playlist<2)
   for(int32 V=0;V<3;++V) if(V==0 || V==E.Playlist+1)
   { auto& S=EligibleCache[V]; S.RecordMatch(E.Points,E.Knockouts,E.DoubleKnockouts,E.Shots,E.bWon); S.RecordAccolades(E.Accolades); }
  CachedCount=Data->Events.Num();
 }
 return EligibleCache[FMath::Clamp(View,0,2)];
}
void UFlickProgressionSubsystem::Challenges()
{
 auto S=GetEligibleStats();
 for(int32 I=0;I<UE_ARRAY_COUNT(FlickChallengeCatalog::Challenges);++I)
 {
  const auto& C=FlickChallengeCatalog::Challenges[I];
  if(FlickChallengeCatalog::GetProgress(C,S)>=C.Goal)
  { FFlickCareerEvent E; E.Id=FString::Printf(TEXT("Challenge-%d"),I); E.Kind=3; E.BonusXP=C.Goal<=5 ? 250 : C.Goal<=25 ? 500 : 1000; Add(E); }
 }
}
void UFlickProgressionSubsystem::AcceptMatch(const FFlickCareerEvent& Report)
{
 if(bPreview) return;
 SelectAccount();
 if(Seen.Contains(Report.Id)) return;
 ClearLastAward();
 if(!Report.bEligible || Report.Id.IsEmpty()) return;
 FFlickCareerEvent E=Report; E.Kind=0; // XP was calculated by the match authority, using its tuning.
 AwardStartXP=TotalXP;
 if(Add(E)) { const int64 Before=TotalXP; if(E.CompletionXP>0) Challenges(); E.BonusXP=int32(TotalXP-Before); LastAward=E; LastAwardAt=FPlatformTime::Seconds(); Save(); }
}
void UFlickProgressionSubsystem::RecordPractice(int32 C,int32 D,int32 Score)
{
 if(bPreview || C<0 || C>=4 || D<0 || D>=3 || Score<0 || Score>5) return;
 SelectAccount(); // Called only after all five challenges finish; no restart farming.
 FFlickCareerEvent E; E.Id=FString::Printf(TEXT("Practice-%d-%d"),C,D); E.Kind=1; E.Format=C; E.Points=D; E.Shots=Score; E.BonusXP=D==0?150:D==1?300:500;
 AwardStartXP=TotalXP; if(Add(E)) { LastAward=E; LastAwardAt=FPlatformTime::Seconds(); }
 if(Score>GetPracticeBest(C,D)) { E.Id=FString::Printf(TEXT("PracticeBest-%d-%d-%d"),C,D,Score); E.BonusXP=0;Add(E); }
 Save();
}
int32 UFlickProgressionSubsystem::GetPracticeBest(int32 C,int32 D)
{
 SelectAccount(); if(C<0 || C>=4 || D<0 || D>=3) return 0;
 int32 Best=Data->LegacyPractice.IsValidIndex(C*3+D)?Data->LegacyPractice[C*3+D]:0;
 for(const auto& E:Data->Events) if(E.Kind==1 && E.Format==C && E.Points==D) Best=FMath::Max(Best,E.Shots);
 return Best;
}
void UFlickProgressionSubsystem::RecordTutorial()
{
 if(bPreview) return; SelectAccount(); FFlickCareerEvent E; E.Id=TEXT("Tutorial"); E.Kind=2; E.BonusXP=250;
 AwardStartXP=TotalXP; if(Add(E)) { LastAward=E; LastAwardAt=FPlatformTime::Seconds(); Save(); }
}
