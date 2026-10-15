#include "Core/FlickDevelopmentRankSettings.h"

#include "Misc/ConfigCacheIni.h"

namespace
{
#if !UE_BUILD_SHIPPING
	const TCHAR* Section = TEXT("FLICK.Development.RankPreview");
	const FString& ConfigFile(const FString& Ini) { return Ini.IsEmpty() ? GGameUserSettingsIni : Ini; }
	bool IsSupported(const EFlickMatchVariant Variant, const int32 PlayersPerTeam)
	{
		return (Variant == EFlickMatchVariant::Classic && PlayersPerTeam >= 1 && PlayersPerTeam <= 3)
			|| (Variant == EFlickMatchVariant::Bob && PlayersPerTeam == 1);
	}
#endif
}

TOptional<int32> FlickDevelopmentRankSettings::GetRating(
	const EFlickMatchVariant Variant, const int32 PlayersPerTeam, const FString& Ini)
{
#if !UE_BUILD_SHIPPING
	int32 Rating = FlickRankRules::DefaultRating;
	if (GConfig && IsSupported(Variant, PlayersPerTeam)
		&& GConfig->GetInt(Section, *FlickRankRules::MakePlaylistKey(Variant, PlayersPerTeam), Rating, ConfigFile(Ini)))
	{
		return FMath::Clamp(Rating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating);
	}
#endif
	return {};
}

FFlickRankProgress FlickDevelopmentRankSettings::ApplyToProgress(
	const EFlickMatchVariant Variant, const int32 PlayersPerTeam, const FFlickRankProgress& Actual, const FString& Ini)
{
	FFlickRankProgress Display = Actual;
	const auto Rating = GetRating(Variant, PlayersPerTeam, Ini);
	if (Rating.IsSet())
	{
		Display.Rating = Rating.GetValue();
		// A display override must show the chosen symbol even before placements.
		Display.MatchesPlayed = FMath::Max(Display.MatchesPlayed, FlickRankRules::PlacementMatchCount);
	}
	return Display;
}

#if !UE_BUILD_SHIPPING
void FlickDevelopmentRankSettings::SetRating(
	const EFlickMatchVariant Variant, const int32 PlayersPerTeam, const int32 Rating, const FString& Ini)
{
	if (!GConfig || !IsSupported(Variant, PlayersPerTeam)) return;
	GConfig->SetInt(Section, *FlickRankRules::MakePlaylistKey(Variant, PlayersPerTeam),
		FMath::Clamp(Rating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating), ConfigFile(Ini));
	GConfig->Flush(false, ConfigFile(Ini));
}

void FlickDevelopmentRankSettings::ClearRating(
	const EFlickMatchVariant Variant, const int32 PlayersPerTeam, const FString& Ini)
{
	if (!GConfig || !IsSupported(Variant, PlayersPerTeam)) return;
	GConfig->RemoveKey(Section, *FlickRankRules::MakePlaylistKey(Variant, PlayersPerTeam), ConfigFile(Ini));
	GConfig->Flush(false, ConfigFile(Ini));
}

void FlickDevelopmentRankSettings::ClearAll(const FString& Ini)
{
	if (!GConfig) return;
	GConfig->EmptySection(Section, ConfigFile(Ini));
	GConfig->Flush(false, ConfigFile(Ini));
}
#endif
