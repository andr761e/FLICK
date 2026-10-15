#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING
#include "Core/FlickDevelopmentRankSettings.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Ranking/FlickRankingSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickDevelopmentRankSettingsTest, "FLICK.Ranked.DevelopmentDisplayOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickDevelopmentRankSettingsTest::RunTest(const FString& Parameters)
{
	using namespace FlickDevelopmentRankSettings;
	const FString TestIni = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
		TEXT("RankPreview-") + FGuid::NewGuid().ToString() + TEXT(".ini"));
	GConfig->Add(TestIni, FConfigFile());
	if (FConfigFile* Config = GConfig->FindConfigFile(TestIni)) Config->bCanSaveAllSections = true;
	ON_SCOPE_EXIT { GConfig->UnloadFile(TestIni); IFileManager::Get().Delete(*TestIni); };
	TGuardValue<FString> IsolatedUserSettings(GGameUserSettingsIni, TestIni);
	FFlickRankProgress Actual;
	Actual.Rating = 1075;
	Actual.MatchesPlayed = 2;
	Actual.Wins = 1;
	TestFalse(TEXT("No override is enabled by default"), GetRating(EFlickMatchVariant::Classic, 1).IsSet());
	SetRating(EFlickMatchVariant::Classic, 1, 2400);
	SetRating(EFlickMatchVariant::Classic, 2, 1750);
	SetRating(EFlickMatchVariant::Classic, 3, 450);
	SetRating(EFlickMatchVariant::Bob, 1, 810);
	TestEqual(TEXT("Doubles has its own value"), GetRating(EFlickMatchVariant::Classic, 2).Get(0), 1750);
	TestEqual(TEXT("Trios has its own value"), GetRating(EFlickMatchVariant::Classic, 3).Get(0), 450);
	TestEqual(TEXT("BOB has its own value"), GetRating(EFlickMatchVariant::Bob, 1).Get(0), 810);
	const auto Display = ApplyToProgress(EFlickMatchVariant::Classic, 1, Actual);
	TestEqual(TEXT("Chosen MMR previews Zenith even during real placements"), FlickRankRules::GetTier(Display), EFlickRankTier::Zenith);
	TestEqual(TEXT("Real MMR remains untouched"), Actual.Rating, 1075);
	TestEqual(TEXT("Real placement count remains untouched"), Actual.MatchesPlayed, 2);
	TestEqual(TEXT("Display never awards a win"), Display.Wins, Actual.Wins);
	auto* Ranking = NewObject<UFlickRankingSubsystem>(NewObject<UGameInstance>());
	TestEqual(TEXT("Menu display uses the override"), Ranking->GetDisplayProgress(EFlickMatchVariant::Classic, 1).Rating, 2400);
	TestEqual(TEXT("Queue rating never uses a display override"), Ranking->GetQueueRating(EFlickMatchVariant::Classic, 1), FlickRankRules::DefaultRating);
	TestEqual(TEXT("Actual rank progress never uses a display override"), Ranking->GetProgress(EFlickMatchVariant::Classic, 1).MatchesPlayed, 0);
	TestTrue(TEXT("Menu labels clearly identify development overrides"), Ranking->GetDisplayProgressLabel(EFlickMatchVariant::Classic, 1).StartsWith(TEXT("DEV")));
	if (FConfigFile* Config = GConfig->FindConfigFile(TestIni)) Config->bCanSaveAllSections = true;
	GConfig->Flush(false, TestIni);
	GConfig->UnloadFile(TestIni);
	FConfigFile Reloaded;
	Reloaded.Read(TestIni);
	GConfig->Add(TestIni, Reloaded);
	TestEqual(TEXT("Display overrides survive config reload"), GetRating(EFlickMatchVariant::Bob, 1).Get(0), 810);
	ClearRating(EFlickMatchVariant::Classic, 1);
	TestEqual(TEXT("Reset restores real placement progress"), ApplyToProgress(EFlickMatchVariant::Classic, 1, Actual).MatchesPlayed, 2);
	TestTrue(TEXT("Resetting one playlist preserves the others"), GetRating(EFlickMatchVariant::Classic, 2).IsSet());
	SetRating(EFlickMatchVariant::Classic, 1, -1);
	TestEqual(TEXT("Negative MMR is clamped"), GetRating(EFlickMatchVariant::Classic, 1).GetValue(), 0);
	SetRating(EFlickMatchVariant::Classic, 1, MAX_int32);
	TestEqual(TEXT("Excessive input is bounded safely"), GetRating(EFlickMatchVariant::Classic, 1).GetValue(), FlickRankRules::MaximumRating);
	SetRating(EFlickMatchVariant::Bob, 2, 1000);
	TestFalse(TEXT("Unsupported BOB team sizes do not alias a real playlist"), GetRating(EFlickMatchVariant::Bob, 2).IsSet());
	ClearAll();
	for (int32 Size = 1; Size <= 3; ++Size)
		TestFalse(TEXT("Clear all removes Knockout overrides"), GetRating(EFlickMatchVariant::Classic, Size).IsSet());
	TestFalse(TEXT("Clear all removes BOB override"), GetRating(EFlickMatchVariant::Bob, 1).IsSet());
	return true;
}
#endif
