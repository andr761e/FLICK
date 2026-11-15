#pragma once
#include "CoreMinimal.h"
#include "Core/FlickProgression.h"
#include "Game/FlickGameInstance.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "FlickProgressionSubsystem.generated.h"

UCLASS()
class UFlickCareerSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version = 1;
 UPROPERTY() TArray<FFlickCareerEvent> Events;
 UPROPERTY() FFlickProfileStats LegacyCasual;
 UPROPERTY() FFlickProfileStats LegacyCompetitive;
 UPROPERTY() bool bHasLegacy = false;
 UPROPERTY() TArray<int32> LegacyPractice;
};

UCLASS()
class FLICK_API UFlickProgressionSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 int64 GetXP();
 FlickProgression::FLevel GetLevel() { return FlickProgression::Level(GetXP()); }
 const TArray<FFlickCareerEvent>& GetEvents();
 FFlickProfileStats GetLegacy(int32 View);
 FFlickProfileStats GetEligibleStats(int32 View=0);
 void AcceptMatch(const FFlickCareerEvent& Event);
 void RecordPractice(int32 Category,int32 Difficulty,int32 Score);
 void RecordTutorial();
 int32 GetPracticeBest(int32 Category,int32 Difficulty);
 void ClearLastAward() { LastAward=FFlickCareerEvent(); LastAwardAt=0; }
 const FFlickCareerEvent& GetLastAward() const { return LastAward; }
 double GetAwardAge() const { return FPlatformTime::Seconds()-LastAwardAt; }
 int64 GetAwardStartXP() const { return AwardStartXP; }
 FString GetSyncStatus() const { return SyncStatus; }
private:
 friend class FFlickProgressionTest;
 bool Tick(float Delta);
 void SelectAccount();
 bool Add(const FFlickCareerEvent& Event);
 void Challenges();
 void Save();
 void SyncCloud();
 bool Merge(const UFlickCareerSave* Other);
 FString Slot,Account,SyncStatus=TEXT("SAVED ON THIS DEVICE");
 UPROPERTY() TObjectPtr<UFlickCareerSave> Data;
 TSet<FString> Seen;
 FFlickProfileStats EligibleCache[3];
 int32 CachedCount=-1;
 FFlickCareerEvent LastAward;
 int64 TotalXP=0,AwardStartXP=0;
 double LastAwardAt=0;
 bool bDirty=false,bPreview=false,bSynced=false;
 FTSTicker::FDelegateHandle Ticker;
};
