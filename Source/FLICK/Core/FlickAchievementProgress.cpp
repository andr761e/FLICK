#include "Core/FlickAchievementProgress.h"


int32 FlickAchievements::MergeRemoteCounter(int32 Local, int32 Pending, int32 Remote, int32 Goal)
{
    const int64 Base = FMath::Max(0, Local - FMath::Max(0, Pending));
    return static_cast<int32>(FMath::Clamp<int64>(FMath::Max<int64>(Base, Remote) + FMath::Max(0, Pending), 0, Goal));
}

int32 FlickAchievements::AcknowledgeCounter(int32 Pending, int32 Submitted)
{
    return FMath::Max(0, Pending - FMath::Max(0, Submitted));
}

void FFlickAchievementProgress::Add(FlickAchievements::EId Id, int32 Amount)
{
    const int32 I = FlickAchievements::Index(Id);
    Values[I] = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(Values[I]) + FMath::Max(0, Amount), 0, FlickAchievements::Get(Id).Goal));
}

void FFlickAchievementProgress::Best(FlickAchievements::EId Id, int32 Value)
{
    Values[FlickAchievements::Index(Id)] = FMath::Max(Values[FlickAchievements::Index(Id)], FMath::Clamp(Value, 0, FlickAchievements::Get(Id).Goal));
}

void FFlickAchievementProgress::Apply(const FFlickAchievementReport& Report)
{
    if (Report.Values.Num() != FlickAchievements::Count) return;
    for (int32 I = 0; I < FlickAchievements::Count; ++I)
    {
        const auto Id = static_cast<FlickAchievements::EId>(I);
        if (FlickAchievements::Definitions[I].bBest || FlickAchievements::Definitions[I].Goal == 1) Best(Id, Report.Values[I]);
        else Add(Id, Report.Values[I]);
    }
    Archetypes |= static_cast<uint32>(Report.ArchetypeMask) & 511u;
    RefreshUniqueProgress();
}

void FFlickAchievementProgress::RecordPractice(int32 Category, int32 Difficulty, int32 Score)
{
    if (Category < 0 || Category >= 4 || Difficulty < 0 || Difficulty >= 3 || Score < 0 || Score > 5) return;
    Add(FlickAchievements::EId::PracticeMakesPermanent);
    if (Score == 5)
    {
        Practice |= 1u << (Category * 3 + Difficulty);
        Best(FlickAchievements::EId::PerfectFive, 1);
        if (Difficulty == 2) Best(FlickAchievements::EId::TheHardWay, 1);
    }
    RefreshUniqueProgress();
}

void FFlickAchievementProgress::RecordPlacements(int32 Playlist, int32 Completed)
{
    if (Playlist < 0 || Playlist >= 4 || Completed < 0) return;
    Best(FlickAchievements::EId::RankAndFile, FMath::Min(Completed, 5));
    if (Completed >= 5) Placed |= 1u << Playlist;
    RefreshUniqueProgress();
}

void FFlickAchievementProgress::RecordRankedWin(int32 Playlist)
{
    if (Playlist < 0 || Playlist >= 4) return;
    RankedWins |= 1u << Playlist;
    RefreshUniqueProgress();
}

void FFlickAchievementProgress::RefreshUniqueProgress()
{
    const auto Bits = [](uint32 Mask) { int32 N = 0; for (; Mask; Mask &= Mask - 1) ++N; return N; };
    Best(FlickAchievements::EId::FullSpectrum, Bits(Archetypes));
    Best(FlickAchievements::EId::MasterTheAngle, Bits(Practice));
    Best(FlickAchievements::EId::AllRounder, Bits(RankedWins));
    Best(FlickAchievements::EId::FourOfAKind, Bits(Placed));
    for (int32 D = 0; D < 3; ++D)
    {
        int32 N = 0;
        for (int32 C = 0; C < 4; ++C) N += (Practice >> (C * 3 + D)) & 1u;
        Best(D == 0 ? FlickAchievements::EId::FoundationSolid : D == 1 ? FlickAchievements::EId::SkillSet : FlickAchievements::EId::ExpertJudgement, N);
    }
}
