#include "Ranking/FlickRankingService.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
	const TCHAR* FlickRankingPendingSectionPrefix = TEXT("FLICK.Ranked.Pending");
	constexpr int32 FlickRankingMaximumProcessedMatches = 128;

	FString SanitizeFlickRankingConfigToken(FString Value)
	{
		for (TCHAR& Character : Value)
		{
			if (!FChar::IsAlnum(Character) && Character != TEXT('_') && Character != TEXT('-'))
			{
				Character = TEXT('_');
			}
		}
		return Value.IsEmpty() ? TEXT("LOCAL_DEVELOPMENT") : Value.Left(96);
	}
}

FConfigFlickRankingService::FConfigFlickRankingService(FString InAccountId, FString InSeasonId)
	: AccountId(SanitizeFlickRankingConfigToken(MoveTemp(InAccountId)))
	, SeasonId(SanitizeFlickRankingConfigToken(MoveTemp(InSeasonId)))
{
}

FString FConfigFlickRankingService::GetPlaylistSection(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam) const
{
	return FString::Printf(
		TEXT("FLICK.Ranked.%s.%s.%s"),
		*AccountId,
		*SeasonId,
		*FlickRankRules::MakePlaylistKey(Variant, PlayersPerTeam));
}

FFlickRankProgress FConfigFlickRankingService::LoadProgress(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam) const
{
	FFlickRankProgress Progress;
	const FString Section = GetPlaylistSection(Variant, PlayersPerTeam);
	GConfig->GetInt(*Section, TEXT("Rating"), Progress.Rating, GGameUserSettingsIni);
	GConfig->GetInt(*Section, TEXT("MatchesPlayed"), Progress.MatchesPlayed, GGameUserSettingsIni);
	GConfig->GetInt(*Section, TEXT("Wins"), Progress.Wins, GGameUserSettingsIni);
	GConfig->GetInt(*Section, TEXT("Losses"), Progress.Losses, GGameUserSettingsIni);
	GConfig->GetInt(*Section, TEXT("Draws"), Progress.Draws, GGameUserSettingsIni);
	Progress.Rating = FMath::Clamp(Progress.Rating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating);
	Progress.MatchesPlayed = FMath::Max(0, Progress.MatchesPlayed);
	Progress.Wins = FMath::Max(0, Progress.Wins);
	Progress.Losses = FMath::Max(0, Progress.Losses);
	Progress.Draws = FMath::Max(0, Progress.Draws);
	return Progress;
}

void FConfigFlickRankingService::SaveProgress(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const FFlickRankProgress& Progress) const
{
	const FString Section = GetPlaylistSection(Variant, PlayersPerTeam);
	GConfig->SetInt(*Section, TEXT("Rating"), Progress.Rating, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("MatchesPlayed"), Progress.MatchesPlayed, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("Wins"), Progress.Wins, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("Losses"), Progress.Losses, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("Draws"), Progress.Draws, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

bool FConfigFlickRankingService::HasProcessedMatch(const FString& Section, const FString& MatchId) const
{
	TArray<FString> ProcessedMatches;
	GConfig->GetArray(*Section, TEXT("ProcessedMatches"), ProcessedMatches, GGameUserSettingsIni);
	return ProcessedMatches.Contains(MatchId);
}

void FConfigFlickRankingService::RecordProcessedMatch(const FString& Section, const FString& MatchId) const
{
	TArray<FString> ProcessedMatches;
	GConfig->GetArray(*Section, TEXT("ProcessedMatches"), ProcessedMatches, GGameUserSettingsIni);
	ProcessedMatches.Remove(MatchId);
	ProcessedMatches.Insert(MatchId, 0);
	if (ProcessedMatches.Num() > FlickRankingMaximumProcessedMatches)
	{
		ProcessedMatches.SetNum(FlickRankingMaximumProcessedMatches);
	}
	GConfig->SetArray(*Section, TEXT("ProcessedMatches"), ProcessedMatches, GGameUserSettingsIni);
}

bool FConfigFlickRankingService::SubmitMatchResult(
	const FString& MatchId,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 OpponentRating,
	const EFlickRankMatchResult Result,
	const bool bForfeit,
	FFlickRatingUpdate& OutUpdate)
{
	OutUpdate = FFlickRatingUpdate();
	if (MatchId.IsEmpty())
	{
		return false;
	}

	const FString Section = GetPlaylistSection(Variant, PlayersPerTeam);
	if (HasProcessedMatch(Section, MatchId))
	{
		return false;
	}

	FFlickRankProgress Progress = LoadProgress(Variant, PlayersPerTeam);
	OutUpdate.MatchId = MatchId;
	OutUpdate.Variant = Variant;
	OutUpdate.PlayersPerTeam = FMath::Clamp(PlayersPerTeam, 1, 3);
	OutUpdate.OldRating = Progress.Rating;
	OutUpdate.OldTier = FlickRankRules::GetTier(Progress);
	OutUpdate.OldDivision = FlickRankRules::GetDivision(Progress);
	OutUpdate.bForfeit = bForfeit;

	const int32 Delta = FlickRankRules::CalculateRatingDelta(
		Progress.Rating,
		FMath::Clamp(OpponentRating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating),
		Result,
		Progress.MatchesPlayed < FlickRankRules::PlacementMatchCount);
	Progress.Rating = FMath::Clamp(
		Progress.Rating + Delta,
		FlickRankRules::MinimumRating,
		FlickRankRules::MaximumRating);
	++Progress.MatchesPlayed;
	if (Result == EFlickRankMatchResult::Win)
	{
		++Progress.Wins;
	}
	else if (Result == EFlickRankMatchResult::Draw)
	{
		++Progress.Draws;
	}
	else
	{
		++Progress.Losses;
	}

	OutUpdate.NewRating = Progress.Rating;
	OutUpdate.RatingDelta = Progress.Rating - OutUpdate.OldRating;
	OutUpdate.MatchesPlayed = Progress.MatchesPlayed;
	OutUpdate.NewTier = FlickRankRules::GetTier(Progress);
	OutUpdate.NewDivision = FlickRankRules::GetDivision(Progress);
	OutUpdate.bAccepted = true;
	SaveProgress(Variant, PlayersPerTeam, Progress);
	RecordProcessedMatch(Section, MatchId);
	ClearPendingMatch(MatchId);
	GConfig->Flush(false, GGameUserSettingsIni);
	return true;
}

void FConfigFlickRankingService::SavePendingMatch(
	const FString& MatchId,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 OpponentRating)
{
	const FString Section = FString::Printf(TEXT("%s.%s"), FlickRankingPendingSectionPrefix, *AccountId);
	GConfig->SetString(*Section, TEXT("MatchId"), *MatchId, GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("Variant"), static_cast<int32>(Variant), GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("PlayersPerTeam"), FMath::Clamp(PlayersPerTeam, 1, 3), GGameUserSettingsIni);
	GConfig->SetInt(*Section, TEXT("OpponentRating"), FMath::Clamp(OpponentRating, 0, 3000), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

bool FConfigFlickRankingService::LoadPendingMatch(
	FString& OutMatchId,
	EFlickMatchVariant& OutVariant,
	int32& OutPlayersPerTeam,
	int32& OutOpponentRating) const
{
	const FString Section = FString::Printf(TEXT("%s.%s"), FlickRankingPendingSectionPrefix, *AccountId);
	if (!GConfig->GetString(*Section, TEXT("MatchId"), OutMatchId, GGameUserSettingsIni)
		|| OutMatchId.IsEmpty())
	{
		return false;
	}
	int32 VariantValue = static_cast<int32>(EFlickMatchVariant::Classic);
	GConfig->GetInt(*Section, TEXT("Variant"), VariantValue, GGameUserSettingsIni);
	GConfig->GetInt(*Section, TEXT("PlayersPerTeam"), OutPlayersPerTeam, GGameUserSettingsIni);
	GConfig->GetInt(*Section, TEXT("OpponentRating"), OutOpponentRating, GGameUserSettingsIni);
	OutVariant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(FMath::Clamp(
		VariantValue,
		static_cast<int32>(EFlickMatchVariant::Classic),
		static_cast<int32>(EFlickMatchVariant::Blitz))));
	OutPlayersPerTeam = FMath::Clamp(OutPlayersPerTeam, 1, 3);
	OutOpponentRating = FMath::Clamp(OutOpponentRating, 0, 3000);
	return true;
}

void FConfigFlickRankingService::ClearPendingMatch(const FString& MatchId)
{
	const FString Section = FString::Printf(TEXT("%s.%s"), FlickRankingPendingSectionPrefix, *AccountId);
	FString PendingMatchId;
	GConfig->GetString(*Section, TEXT("MatchId"), PendingMatchId, GGameUserSettingsIni);
	if (MatchId.IsEmpty() || PendingMatchId == MatchId)
	{
		GConfig->EmptySection(*Section, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void FConfigFlickRankingService::CacheAuthoritativeUpdate(const FFlickRatingUpdate& Update)
{
	if (!Update.bAccepted || Update.MatchId.IsEmpty())
	{
		return;
	}
	const FString Section = GetPlaylistSection(Update.Variant, Update.PlayersPerTeam);
	if (HasProcessedMatch(Section, Update.MatchId))
	{
		ClearPendingMatch(Update.MatchId);
		return;
	}
	FFlickRankProgress Progress = LoadProgress(Update.Variant, Update.PlayersPerTeam);
	Progress.Rating = FMath::Clamp(Update.NewRating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating);
	Progress.MatchesPlayed = FMath::Max(Progress.MatchesPlayed, Update.MatchesPlayed);
	SaveProgress(Update.Variant, Update.PlayersPerTeam, Progress);
	RecordProcessedMatch(Section, Update.MatchId);
	ClearPendingMatch(Update.MatchId);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void FConfigFlickRankingService::CacheAuthoritativeProgress(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const FFlickRankProgress& Progress)
{
	FFlickRankProgress Sanitized = Progress;
	Sanitized.Rating = FMath::Clamp(Sanitized.Rating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating);
	Sanitized.MatchesPlayed = FMath::Max(0, Sanitized.MatchesPlayed);
	Sanitized.Wins = FMath::Max(0, Sanitized.Wins);
	Sanitized.Losses = FMath::Max(0, Sanitized.Losses);
	Sanitized.Draws = FMath::Max(0, Sanitized.Draws);
	SaveProgress(Variant, PlayersPerTeam, Sanitized);
}
