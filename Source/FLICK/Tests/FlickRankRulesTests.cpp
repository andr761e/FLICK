#include "Core/FlickRankRules.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Ranking/FlickRankingService.h"

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
	TestEqual(TEXT("Established 1125 rating is Gold"), FlickRankRules::GetTier(Progress), EFlickRankTier::Gold);
	TestEqual(TEXT("1125 is Gold III"), FlickRankRules::GetDivision(Progress), 3);
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

#endif
