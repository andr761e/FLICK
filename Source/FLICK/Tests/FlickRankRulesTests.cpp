#include "Core/FlickRankRules.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Ranking/FlickRankingService.h"
#include "Engine/Texture2D.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickRankPlacementTest,
	"FLICK.Ranked.PlacementsAndTiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankPlacementTest::RunTest(const FString& Parameters)
{
	FFlickRankProgress Progress;
	Progress.Rating = 1125;
	Progress.MatchesPlayed = 4;
	TestEqual(TEXT("Four matches remains unranked"), FlickRankRules::GetTier(Progress), EFlickRankTier::Unranked);
	TestEqual(TEXT("One placement remains"), FlickRankRules::GetPlacementsRemaining(Progress), 1);

	Progress.MatchesPlayed = 5;
	TestEqual(TEXT("Established 1125 rating is Tempered"), FlickRankRules::GetTier(Progress), EFlickRankTier::Tempered);
	TestEqual(TEXT("1125 is Tempered III"), FlickRankRules::GetDivision(Progress), 3);
	TestEqual(TEXT("Placements are complete"), FlickRankRules::GetPlacementsRemaining(Progress), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickRankRatingUpdateTest,
	"FLICK.Ranked.RatingUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankRatingUpdateTest::RunTest(const FString& Parameters)
{
	const int32 EvenWin = FlickRankRules::CalculateRatingDelta(1000, 1000, EFlickRankMatchResult::Win, false);
	const int32 EvenLoss = FlickRankRules::CalculateRatingDelta(1000, 1000, EFlickRankMatchResult::Loss, false);
	const int32 UpsetWin = FlickRankRules::CalculateRatingDelta(1000, 1300, EFlickRankMatchResult::Win, false);
	const int32 PlacementWin = FlickRankRules::CalculateRatingDelta(1000, 1000, EFlickRankMatchResult::Win, true);
	TestEqual(TEXT("Even established win awards sixteen"), EvenWin, 16);
	TestEqual(TEXT("Even established loss removes sixteen"), EvenLoss, -16);
	TestTrue(TEXT("Upset wins award more than even wins"), UpsetWin > EvenWin);
	TestTrue(TEXT("Placements move rating faster"), PlacementWin > EvenWin);
	TestEqual(
		TEXT("Even draw leaves rating unchanged"),
		FlickRankRules::CalculateRatingDelta(1000, 1000, EFlickRankMatchResult::Draw, false),
		0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickRankPlaylistTest,
	"FLICK.Ranked.PlaylistsAndSearch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankPlaylistTest::RunTest(const FString& Parameters)
{
	const FString KnockoutDoubles = FlickRankRules::MakePlaylistKey(EFlickMatchVariant::Classic, 2);
	const FString KnockoutTrios = FlickRankRules::MakePlaylistKey(EFlickMatchVariant::Classic, 3);
	const FString BobDoubles = FlickRankRules::MakePlaylistKey(EFlickMatchVariant::Bob, 2);
	TestNotEqual(TEXT("Team sizes have separate ratings"), KnockoutDoubles, KnockoutTrios);
	TestNotEqual(TEXT("Game variants have separate ratings"), KnockoutDoubles, BobDoubles);
	TestEqual(TEXT("Initial ranked range is narrow"), FlickRankRules::GetSearchRangeForAttempt(0), 100);
	TestTrue(
		TEXT("Repeated searches widen the rating range"),
		FlickRankRules::GetSearchRangeForAttempt(4) > FlickRankRules::GetSearchRangeForAttempt(0));
	TestEqual(TEXT("Search range is capped"), FlickRankRules::GetSearchRangeForAttempt(99), 600);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickRankIdempotencyTest,
	"FLICK.Ranked.IdempotentPersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankIdempotencyTest::RunTest(const FString& Parameters)
{
	const FString AccountId = TEXT("AUTOMATION_RANKING");
	const FString Playlist = FlickRankRules::MakePlaylistKey(EFlickMatchVariant::Classic, 2);
	const FString PlaylistSection = FString::Printf(TEXT("FLICK.Ranked.%s.PRESEASON.%s"), *AccountId, *Playlist);
	const FString TestPendingSection = FString::Printf(TEXT("FLICK.Ranked.Pending.%s"), *AccountId);
	GConfig->EmptySection(*PlaylistSection, GGameUserSettingsIni);
	GConfig->EmptySection(*TestPendingSection, GGameUserSettingsIni);

	FConfigFlickRankingService Service(AccountId);
	const FString MatchId = TEXT("AUTOMATION-IDEMPOTENT-MATCH");
	Service.SavePendingMatch(MatchId, EFlickMatchVariant::Classic, 2, 1000);
	FFlickRatingUpdate FirstUpdate;
	FFlickRatingUpdate DuplicateUpdate;
	TestTrue(
		TEXT("First authoritative result is accepted"),
		Service.SubmitMatchResult(
			MatchId,
			EFlickMatchVariant::Classic,
			2,
			1000,
			EFlickRankMatchResult::Win,
			false,
			FirstUpdate));
	TestFalse(
		TEXT("Duplicate match ID is rejected"),
		Service.SubmitMatchResult(
			MatchId,
			EFlickMatchVariant::Classic,
			2,
			1000,
			EFlickRankMatchResult::Win,
			false,
			DuplicateUpdate));
	TestEqual(TEXT("Only one match was recorded"), Service.LoadProgress(EFlickMatchVariant::Classic, 2).MatchesPlayed, 1);

	GConfig->EmptySection(*PlaylistSection, GGameUserSettingsIni);
	GConfig->EmptySection(*TestPendingSection, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRankLadderTest, "FLICK.Ranked.All22BadgeBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankLadderTest::RunTest(const FString& Parameters)
{
	const int32 Thresholds[] = {0, 200, 400, 600, 700, 800, 900, 1000, 1100, 1200, 1300,
		1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400};
	for (int32 Index = 0; Index < FlickRankRules::BadgeCount; ++Index)
	{
		FFlickRankProgress Progress = FlickRankRules::GetBadgeProgress(Index);
		TestEqual(FString::Printf(TEXT("Badge %d begins at agreed MMR"), Index), Progress.Rating, Thresholds[Index]);
		TestEqual(TEXT("Every symbol has a unique round-trip index"), FlickRankRules::GetBadgeIndex(Progress), Index);
		TestEqual(TEXT("All seven tiers have three subdivisions; Zenith has none"),
			FlickRankRules::GetDivision(Progress), Index == 21 ? 0 : Index % 3 + 1);
		if (Index > 0)
		{
			--Progress.Rating;
			TestEqual(TEXT("One MMR below each promotion remains the previous badge"), FlickRankRules::GetBadgeIndex(Progress), Index - 1);
		}
	}
	FFlickRankProgress Progress;
	Progress.MatchesPlayed = 4;
	Progress.Rating = 2400;
	TestEqual(TEXT("Placements never grant an earned badge"), FlickRankRules::GetBadgeIndex(Progress), INDEX_NONE);
	Progress.MatchesPlayed = 5;
	TestEqual(TEXT("Zenith has no roman numeral"), FlickRankRules::GetRankLabel(Progress), FString(TEXT("ZENITH")));
	Progress.Rating = 4000;
	TestEqual(TEXT("Above old cap remains Zenith"), FlickRankRules::GetBadgeIndex(Progress), 21);
	Progress.Rating = -5;
	TestEqual(TEXT("Negative display input is clamped to Carbon I"), FlickRankRules::GetBadgeIndex(Progress), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRankMetadataTest, "FLICK.Ranked.CanonicalMetadataAndPromotions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankMetadataTest::RunTest(const FString& Parameters)
{
	FFlickRatingUpdate Update;
	Update.OldRating = 2390;
	Update.NewRating = 2406;
	Update.MatchesPlayed = 20;
	Update.OldDivision = Update.NewDivision = 4; // Stale backend presentation metadata.
	FlickRankRules::RefreshRankMetadata(Update);
	TestEqual(TEXT("Promotion starts in Singularity"), Update.OldTier, EFlickRankTier::Singularity);
	TestEqual(TEXT("Promotion starts in III"), Update.OldDivision, 3);
	TestEqual(TEXT("Promotion ends in Zenith"), Update.NewTier, EFlickRankTier::Zenith);
	TestEqual(TEXT("Promotion derives the delta from canonical ratings"), Update.RatingDelta, 16);
	TestEqual(TEXT("Zenith division is zero"), Update.NewDivision, 0);
	Update.OldRating = 2406;
	Update.NewRating = 2390;
	FlickRankRules::RefreshRankMetadata(Update);
	TestEqual(TEXT("Demotion returns to Singularity III"), Update.NewDivision, 3);
	TestEqual(TEXT("Delta comes only from authoritative ratings"), Update.RatingDelta, -16);
	Update.MatchesPlayed = 5;
	FlickRankRules::RefreshRankMetadata(Update);
	TestEqual(TEXT("Fifth result completes placements, not an earlier rank"), Update.OldTier, EFlickRankTier::Unranked);
	TestEqual(TEXT("Fifth result awards the correct rank"), Update.NewTier, EFlickRankTier::Singularity);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRankAtlasTest, "FLICK.Ranked.BadgeAtlasAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankAtlasTest::RunTest(const FString& Parameters)
{
	const UTexture2D* Atlas = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Ranks/RankBadges.RankBadges"));
	if (!TestNotNull(TEXT("The cooked rank atlas exists"), Atlas)) return false;
#if WITH_EDITORONLY_DATA
	// NullRHI deliberately has no platform/GPU dimensions. Validate imported source.
	TestEqual(TEXT("Atlas import retains source width"), Atlas->Source.GetSizeX(), int64(887));
	TestEqual(TEXT("Atlas import retains source height"), Atlas->Source.GetSizeY(), int64(1774));
#endif
	TestEqual(TEXT("Atlas uses UI texture group"), Atlas->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
	TestTrue(TEXT("Rank symbols do not stream in late"), Atlas->NeverStream);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRankExistingProgressTest, "FLICK.Ranked.PreservesMMRAndPlaylistIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankExistingProgressTest::RunTest(const FString& Parameters)
{
	const FString Account = TEXT("AUTOMATION_LADDER_MIGRATION");
	const FString ClassicSection = TEXT("FLICK.Ranked.") + Account + TEXT(".PRESEASON.")
		+ FlickRankRules::MakePlaylistKey(EFlickMatchVariant::Classic, 1);
	const FString BobSection = TEXT("FLICK.Ranked.") + Account + TEXT(".PRESEASON.")
		+ FlickRankRules::MakePlaylistKey(EFlickMatchVariant::Bob, 1);
	GConfig->EmptySection(*ClassicSection, GGameUserSettingsIni);
	GConfig->EmptySection(*BobSection, GGameUserSettingsIni);
	FConfigFlickRankingService Service(Account);
	FFlickRankProgress Existing;
	Existing.Rating = 2995;
	Existing.MatchesPlayed = 20;
	Existing.Wins = 10;
	Service.CacheAuthoritativeProgress(EFlickMatchVariant::Classic, 1, Existing);
	TestEqual(TEXT("Installing a ladder never resets existing MMR"), Service.LoadProgress(EFlickMatchVariant::Classic, 1).Rating, 2995);
	FFlickRatingUpdate Update;
	TestTrue(TEXT("An established result settles through the existing service"),
		Service.SubmitMatchResult(TEXT("ladder-migration-match"), EFlickMatchVariant::Classic, 1, 2995, EFlickRankMatchResult::Win, false, Update));
	TestEqual(TEXT("Progression is not stopped by the old 3000 cap"), Update.NewRating, 3011);
	TestEqual(TEXT("Winner remains Zenith"), Update.NewTier, EFlickRankTier::Zenith);
	TestEqual(TEXT("A separate game variant keeps independent MMR"), Service.LoadProgress(EFlickMatchVariant::Bob, 1).Rating, FlickRankRules::DefaultRating);
	TestEqual(TEXT("Other variants still need their own placements"), Service.LoadProgress(EFlickMatchVariant::Bob, 1).MatchesPlayed, 0);
	TestEqual(TEXT("Other team sizes keep independent MMR"), Service.LoadProgress(EFlickMatchVariant::Classic, 2).MatchesPlayed, 0);
	FConfigFlickRankingService Reloaded(Account);
	TestEqual(TEXT("New MMR survives service recreation"), Reloaded.LoadProgress(EFlickMatchVariant::Classic, 1).Rating, 3011);
	GConfig->EmptySection(*ClassicSection, GGameUserSettingsIni);
	GConfig->EmptySection(*BobSection, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	return true;
}

#endif
