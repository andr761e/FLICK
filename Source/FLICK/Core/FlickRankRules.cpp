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
		{ EFlickRankTier::Carbon, 0, 599 },
		{ EFlickRankTier::Alloy, 600, 899 },
		{ EFlickRankTier::Tempered, 900, 1199 },
		{ EFlickRankTier::Vector, 1200, 1499 },
		{ EFlickRankTier::Prism, 1500, 1799 },
		{ EFlickRankTier::Overdrive, 1800, 2099 },
		{ EFlickRankTier::Singularity, 2100, 2399 },
		{ EFlickRankTier::Zenith, 2400, FlickRankRules::MaximumRating }
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
	const double Difference = static_cast<double>(OpponentRating) - static_cast<double>(Rating);
	const double ExpectedScore = 1.0 / (1.0 + FMath::Pow(10.0, FMath::Clamp(Difference / 400.0, -20.0, 20.0)));
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
	if (GetTier(Progress) == EFlickRankTier::Unranked || GetTier(Progress) == EFlickRankTier::Zenith)
	{
		return 0;
	}
	const FFlickTierRange& Range = FindTierRange(FMath::Clamp(Progress.Rating, MinimumRating, MaximumRating));
	const int32 RangeSize = FMath::Max(1, Range.Maximum - Range.Minimum + 1);
	const int32 Rating = FMath::Clamp(Progress.Rating, MinimumRating, MaximumRating);
	return FMath::Clamp((Rating - Range.Minimum) / (RangeSize / 3) + 1, 1, 3);
}

int32 FlickRankRules::GetBadgeIndex(const FFlickRankProgress& Progress)
{
	const EFlickRankTier Tier = GetTier(Progress);
	if (Tier == EFlickRankTier::Unranked) return INDEX_NONE;
	if (Tier == EFlickRankTier::Zenith) return BadgeCount - 1;
	return (static_cast<int32>(Tier) - 1) * 3 + GetDivision(Progress) - 1;
}

FFlickRankProgress FlickRankRules::GetBadgeProgress(const int32 BadgeIndex)
{
	FFlickRankProgress Progress;
	if (BadgeIndex < 0 || BadgeIndex >= BadgeCount) return Progress;
	Progress.MatchesPlayed = PlacementMatchCount;
	const int32 TierIndex = BadgeIndex / 3;
	const FFlickTierRange& Range = TierRanges[TierIndex];
	Progress.Rating = Range.Minimum + (BadgeIndex % 3) * (TierIndex == 7 ? 0 : (Range.Maximum - Range.Minimum + 1) / 3);
	return Progress;
}

FString FlickRankRules::GetRatingRangeLabel(const FFlickRankProgress& Progress)
{
	const EFlickRankTier Tier = GetTier(Progress);
	if (Tier == EFlickRankTier::Unranked) return TEXT("5 PLACEMENT MATCHES");
	if (Tier == EFlickRankTier::Zenith) return TEXT("2400+ MMR");
	const FFlickTierRange& Range = FindTierRange(FMath::Clamp(Progress.Rating, MinimumRating, MaximumRating));
	const int32 Width = (Range.Maximum - Range.Minimum + 1) / 3;
	const int32 Minimum = Range.Minimum + (GetDivision(Progress) - 1) * Width;
	return FString::Printf(TEXT("%d - %d MMR"), Minimum, Minimum + Width - 1);
}

int32 FlickRankRules::GetPlacementsRemaining(const FFlickRankProgress& Progress)
{
	return FMath::Max(0, PlacementMatchCount - Progress.MatchesPlayed);
}

FString FlickRankRules::GetTierName(const EFlickRankTier Tier)
{
	switch (Tier)
	{
	case EFlickRankTier::Carbon: return TEXT("CARBON");
	case EFlickRankTier::Alloy: return TEXT("ALLOY");
	case EFlickRankTier::Tempered: return TEXT("TEMPERED");
	case EFlickRankTier::Vector: return TEXT("VECTOR");
	case EFlickRankTier::Prism: return TEXT("PRISM");
	case EFlickRankTier::Overdrive: return TEXT("OVERDRIVE");
	case EFlickRankTier::Singularity: return TEXT("SINGULARITY");
	case EFlickRankTier::Zenith: return TEXT("ZENITH");
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
	default: return FString();
	}
}

FString FlickRankRules::GetRankLabel(const FFlickRankProgress& Progress)
{
	const EFlickRankTier Tier = GetTier(Progress);
	if (Tier == EFlickRankTier::Zenith) return TEXT("ZENITH");
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

void FlickRankRules::RefreshRankMetadata(FFlickRatingUpdate& Update)
{
	Update.OldRating = FMath::Clamp(Update.OldRating, MinimumRating, MaximumRating);
	Update.NewRating = FMath::Clamp(Update.NewRating, MinimumRating, MaximumRating);
	Update.RatingDelta = Update.NewRating - Update.OldRating;
	Update.MatchesPlayed = FMath::Max(0, Update.MatchesPlayed);
	FFlickRankProgress Progress;
	Progress.Rating = Update.OldRating;
	Progress.MatchesPlayed = FMath::Max(0, Update.MatchesPlayed - 1);
	Update.OldTier = GetTier(Progress);
	Update.OldDivision = GetDivision(Progress);
	Progress.Rating = Update.NewRating;
	Progress.MatchesPlayed = Update.MatchesPlayed;
	Update.NewTier = GetTier(Progress);
	Update.NewDivision = GetDivision(Progress);
}
