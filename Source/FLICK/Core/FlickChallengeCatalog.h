#pragma once

#include "Game/FlickGameInstance.h"

namespace FlickChallengeCatalog
{
	enum class ECategory : uint8 { FirstFlicks, Precision, Rivalry, Events };
	enum class EMetric : uint8 { Matches, Wins, Points, Knockouts, DoubleKnockouts, Shots };

	struct FDefinition
	{
		const TCHAR* Title;
		const TCHAR* Detail;
		ECategory Category;
		EMetric Metric;
		int32 Goal;
	};

	inline constexpr FDefinition Challenges[] = {
		{TEXT("FIRST WHISTLE"), TEXT("Complete a match in any mode"), ECategory::FirstFlicks, EMetric::Matches, 1},
		{TEXT("FIND YOUR ANGLE"), TEXT("Take 25 shots"), ECategory::FirstFlicks, EMetric::Shots, 25},
		{TEXT("ON THE BOARD"), TEXT("Earn 250 career points"), ECategory::FirstFlicks, EMetric::Points, 250},
		{TEXT("FIRST CONTACT"), TEXT("Knock out 5 opposing pucks"), ECategory::Precision, EMetric::Knockouts, 5},
		{TEXT("TABLE CONTROL"), TEXT("Knock out 20 opposing pucks"), ECategory::Precision, EMetric::Knockouts, 20},
		{TEXT("DOUBLE TROUBLE"), TEXT("Land 3 double knockouts"), ECategory::Precision, EMetric::DoubleKnockouts, 3},
		{TEXT("FIRST RIVALRY"), TEXT("Win your first match"), ECategory::Rivalry, EMetric::Wins, 1},
		{TEXT("WINNING FORM"), TEXT("Win 5 matches"), ECategory::Rivalry, EMetric::Wins, 5},
		{TEXT("ARENA VETERAN"), TEXT("Win 15 matches"), ECategory::Rivalry, EMetric::Wins, 15}
	};

	inline int32 GetProgress(const FDefinition& Challenge, const FFlickProfileStats& Stats)
	{
		switch (Challenge.Metric)
		{
		case EMetric::Matches: return Stats.MatchesPlayed;
		case EMetric::Wins: return Stats.Wins;
		case EMetric::Points: return Stats.Points;
		case EMetric::Knockouts: return Stats.Knockouts;
		case EMetric::DoubleKnockouts: return Stats.DoubleKnockouts;
		case EMetric::Shots: return Stats.Shots;
		default: return 0;
		}
	}

	inline TArray<int32> GetFeaturedIndices(const FFlickProfileStats& Stats, const int32 MaxCount = 3)
	{
		TArray<int32> Indices;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Challenges); ++Index)
		{
			if (GetProgress(Challenges[Index], Stats) < Challenges[Index].Goal)
			{
				Indices.Add(Index);
			}
		}
		Indices.Sort([&Stats](const int32 Left, const int32 Right)
		{
			const int64 LeftProgress = GetProgress(Challenges[Left], Stats);
			const int64 RightProgress = GetProgress(Challenges[Right], Stats);
			const int64 LeftRatio = LeftProgress * Challenges[Right].Goal;
			const int64 RightRatio = RightProgress * Challenges[Left].Goal;
			return LeftRatio == RightRatio ? Left < Right : LeftRatio > RightRatio;
		});
		Indices.SetNum(FMath::Min(Indices.Num(), FMath::Max(0, MaxCount)));
		return Indices;
	}
}
