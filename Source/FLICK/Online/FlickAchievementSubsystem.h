#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Core/FlickAchievementProgress.h"
#include "Core/FlickTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlickAchievementSubsystem.generated.h"

class FFlickSteamAchievementBridge;

UCLASS()
class FLICK_API UFlickAchievementSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void AcceptMatch(const FFlickAchievementReport& Report);
    void RecordPractice(int32 Category, int32 Difficulty, int32 Score);
    void RecordPracticeShot();
    void RecordCustomLineup();
    void RecordPlacements(EFlickMatchVariant Variant, int32 TeamSize, int32 Matches);
    void RecordRankedWin(EFlickMatchVariant Variant, int32 TeamSize);
    const FFlickAchievementProgress& GetProgress() const { return Progress; }
private:
    friend class FFlickSteamAchievementBridge;
    bool TickSteam(float Delta);
    void SelectAccount();
    void Save() const;
    void Changed(const TArray<int32>& Before);
    FFlickAchievementProgress Progress;
    TArray<int32> Pending;
    TArray<FString> SeenReports;
    FString Account, Section;
    bool bDirty = false;
    FTSTicker::FDelegateHandle TickHandle;
    // Owned explicitly in Initialize/Deinitialize; callbacks never outlive the subsystem.
    FFlickSteamAchievementBridge* Bridge = nullptr;
};
