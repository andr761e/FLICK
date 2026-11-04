#pragma once

#include "CoreMinimal.h"
#include "Core/FlickAchievementCatalog.h"
#include "FlickAchievementProgress.generated.h"

namespace FlickAchievements
{
    int32 MergeRemoteCounter(int32 Local, int32 Pending, int32 Remote, int32 Goal);
    int32 AcknowledgeCounter(int32 Pending, int32 Submitted);
}

// A completed authority-owned match report. Fixed catalogue order is versioned
// with the game; no client sends these values to another player's account.
USTRUCT()
struct FFlickAchievementReport
{
    GENERATED_BODY()
    UPROPERTY() FGuid EventId;
    UPROPERTY() TArray<int32> Values;
    UPROPERTY() int32 ArchetypeMask = 0;
    FFlickAchievementReport() { Values.Init(0, FlickAchievements::Count); }
};

// Pure progress rules, also exercised without Steam by automation tests.
struct FLICK_API FFlickAchievementProgress
{
    TArray<int32> Values;
    uint32 Archetypes = 0, Practice = 0, RankedWins = 0, Placed = 0;
    FFlickAchievementProgress() { Values.Init(0, FlickAchievements::Count); }
    int32 Get(FlickAchievements::EId Id) const { return Values[FlickAchievements::Index(Id)]; }
    void Add(FlickAchievements::EId Id, int32 Amount = 1);
    void Best(FlickAchievements::EId Id, int32 Value);
    void Apply(const FFlickAchievementReport& Report);
    void RecordPractice(int32 Category, int32 Difficulty, int32 Score);
    void RecordPlacements(int32 Playlist, int32 Count);
    void RecordRankedWin(int32 Playlist);
    void RefreshUniqueProgress();
};
