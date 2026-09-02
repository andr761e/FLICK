#include "Core/FlickRankRules.h"

namespace
{
	struct FFlickTierRange
	{
		EFlickRankTier Tier;
		int32 Minimum;
		int32 Maximum;
	};

	constexpr FFlickTierRange TierRanges[] =
	{
		{ EFlickRankTier::Bronze, 0, 899 },
		{ EFlickRankTier::Silver, 900, 1049 },
		{ EFlickRankTier::Gold, 1050, 1199 },
		{ EFlickRankTier::Platinum, 1200, 1349 },
		{ EFlickRankTier::Diamond, 1350, 1499 },
		{ EFlickRankTier::Champion, 1500, 1699 },
		{ EFlickRankTier::GrandChampion, 1700, FlickRankRules::MaximumRating }
	};

	const FFlickTierRange& FindTierRange(const int32 Rating)
	{
		for (const FFlickTierRange& Range : TierRanges)
		{
			if (Rating >= Range.Minimum && Rating <= Range.Maximum)
			{
				return Range;
			}
		}
		return TierRanges[UE_ARRAY_COUNT(TierRanges) - 1];
	}
}

FString FlickRankRules::MakePlaylistKey(const EFlickMatchVariant Variant, const int32 PlayersPerTeam)
{
	return FString::Printf(
		TEXT("%s_%dV%d"),
		*GetMatchVariantName(Variant).Replace(TEXT(" "), TEXT("_")),
		FMath::Clamp(PlayersPerTeam, 1, 3),
		FMath::Clamp(PlayersPerTeam, 1, 3));
}

int32 FlickRankRules::GetSearchRangeForAttempt(const int32 SearchAttempt)
{
	return FMath::Min(600, 100 + FMath::Max(0, SearchAttempt) * 75);
}

int32 FlickRankRules::CalculateRatingDelta(
	const int32 Rating,
	const int32 OpponentRating,
	const EFlickRankMatchResult Result,
	const bool bInPlacements)
{
	const double ExpectedScore = 1.0 / (1.0 + FMath::Pow(10.0, (OpponentRating - Rating) / 400.0));
	const double ActualScore = Result == EFlickRankMatchResult::Win
		? 1.0
		: Result == EFlickRankMatchResult::Draw ? 0.5 : 0.0;
	const double KFactor = bInPlacements ? 56.0 : 32.0;
	return FMath::RoundToInt(KFactor * (ActualScore - ExpectedScore));
}

EFlickRankTier FlickRankRules::GetTier(const FFlickRankProgress& Progress)
{
	if (Progress.MatchesPlayed < PlacementMatchCount)
	{
		return EFlickRankTier::Unranked;
	}
	return FindTierRange(FMath::Clamp(Progress.Rating, MinimumRating, MaximumRating)).Tier;
}

int32 FlickRankRules::GetDivision(const FFlickRankProgress& Progress)
{
	if (GetTier(Progress) == EFlickRankTier::Unranked)
	{
		return 0;
	}
	const FFlickTierRange& Range = FindTierRange(FMath::Clamp(Progress.Rating, MinimumRating, MaximumRating));
	const int32 RangeSize = FMath::Max(1, Range.Maximum - Range.Minimum + 1);
	const float Alpha = static_cast<float>(Progress.Rating - Range.Minimum) / static_cast<float>(RangeSize);
	return FMath::Clamp(FMath::FloorToInt(Alpha * 4.0f) + 1, 1, 4);
}

int32 FlickRankRules::GetPlacementsRemaining(const FFlickRankProgress& Progress)
{
	return FMath::Max(0, PlacementMatchCount - Progress.MatchesPlayed);
}

FString FlickRankRules::GetTierName(const EFlickRankTier Tier)
{
	switch (Tier)
	{
	case EFlickRankTier::Bronze: return TEXT("BRONZE");
	case EFlickRankTier::Silver: return TEXT("SILVER");
	case EFlickRankTier::Gold: return TEXT("GOLD");
	case EFlickRankTier::Platinum: return TEXT("PLATINUM");
	case EFlickRankTier::Diamond: return TEXT("DIAMOND");
	case EFlickRankTier::Champion: return TEXT("CHAMPION");
	case EFlickRankTier::GrandChampion: return TEXT("GRAND CHAMPION");
	case EFlickRankTier::Unranked:
	default: return TEXT("UNRANKED");
	}
}

FString FlickRankRules::GetDivisionName(const int32 Division)
{
	switch (Division)
	{
	case 1: return TEXT("I");
	case 2: return TEXT("II");
	case 3: return TEXT("III");
	case 4: return TEXT("IV");
	default: return FString();
	}
}

FString FlickRankRules::GetRankLabel(const FFlickRankProgress& Progress)
{
	const EFlickRankTier Tier = GetTier(Progress);
	if (Tier == EFlickRankTier::Unranked)
	{
		return TEXT("UNRANKED");
	}
	return FString::Printf(TEXT("%s %s"), *GetTierName(Tier), *GetDivisionName(GetDivision(Progress)));
}

FString FlickRankRules::GetProgressLabel(const FFlickRankProgress& Progress)
{
	const int32 PlacementsRemaining = GetPlacementsRemaining(Progress);
	if (PlacementsRemaining > 0)
	{
		return FString::Printf(
			TEXT("PLACEMENT %d / %d  |  %d MMR"),
			PlacementMatchCount - PlacementsRemaining,
			PlacementMatchCount,
			Progress.Rating);
	}
	return FString::Printf(TEXT("%s  |  %d MMR"), *GetRankLabel(Progress), Progress.Rating);
}
