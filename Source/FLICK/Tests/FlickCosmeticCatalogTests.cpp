#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickCosmeticCatalog.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickCosmeticCatalogTest,
	"FLICK.Profile.CosmeticCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickCosmeticCatalogTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Locker contains profile, puck, and effect categories"),
		FlickCosmeticCatalog::CategoryCount, 6 + FlickPieceArchetypeRules::ArchetypeCount);
	TestEqual(TEXT("Existing banner config key remains compatible"), FlickCosmeticCatalog::GetConfigKey(0), FString(TEXT("BannerStyle")));
	TestEqual(TEXT("Existing tag config key remains compatible"), FlickCosmeticCatalog::GetConfigKey(1), FString(TEXT("BannerTag")));
	TestEqual(TEXT("Existing border config key remains compatible"), FlickCosmeticCatalog::GetConfigKey(2), FString(TEXT("AvatarBorder")));
	TestTrue(TEXT("Banner tag locker has a varied collection"), FlickCosmeticCatalog::GetItems(1).Num() >= 10);
	TestEqual(TEXT("First saved tag keeps its original meaning"), FlickCosmeticCatalog::GetItems(1)[0], FString(TEXT("READY TO FLICK")));
	TestEqual(TEXT("Second saved tag keeps its original meaning"), FlickCosmeticCatalog::GetItems(1)[1], FString(TEXT("TABLE TACTICIAN")));
	TestEqual(TEXT("Third saved tag keeps its original meaning"), FlickCosmeticCatalog::GetItems(1)[2], FString(TEXT("RIVAL INCOMING")));
	TestEqual(TEXT("Three collection banners follow the original eleven"), FlickCosmeticCatalog::GetItems(0).Num(), 14);
	TestEqual(TEXT("Three collection avatar borders follow the original seven"), FlickCosmeticCatalog::GetItems(2).Num(), 10);
	TestEqual(TEXT("Six new tags follow the original twelve"), FlickCosmeticCatalog::GetItems(1).Num(), 18);
	for (int32 Collection = 0; Collection < 3; ++Collection)
	{
		TestEqual(TEXT("Banner collection maps to its frame"), FlickCosmeticCatalog::GetCollection(0, 11 + Collection), Collection);
		TestEqual(TEXT("Frame collection maps to its banner"), FlickCosmeticCatalog::GetCollection(2, 7 + Collection), Collection);
		for (int32 Category = FlickCosmeticCatalog::TrailCategory; Category <= FlickCosmeticCatalog::KnockoutCategory; ++Category)
		{
			TestEqual(TEXT("Effect collections include standalone styles"), FlickCosmeticCatalog::GetItems(Category).Num(), Category == FlickCosmeticCatalog::TrailCategory ? 13 : Category == FlickCosmeticCatalog::SpawnCategory ? 14 : 20);
			TestEqual(TEXT("Effect shares its collection palette"), FlickCosmeticCatalog::GetCollection(Category, 3 + Collection), Collection);
			TestTrue(TEXT("Every collection effect explains its motion"), FlickCosmeticCatalog::GetDescription(Category, 3 + Collection).Len() > 30);
		}
	}
	TestEqual(TEXT("Invalid items cannot select a collection"), FlickCosmeticCatalog::GetCollection(0, 99), INDEX_NONE);
	for (int32 Trail = 6; Trail < 13; ++Trail)
	{
		TestEqual(TEXT("Standalone trails do not index the three collection descriptions"), FlickCosmeticCatalog::GetCollection(FlickCosmeticCatalog::TrailCategory, Trail), INDEX_NONE);
		TestTrue(TEXT("Standalone trail describes its appearance"), FlickCosmeticCatalog::GetDescription(FlickCosmeticCatalog::TrailCategory, Trail).Len() > 30);
	}
	TestEqual(TEXT("Original banner index is stable"), FlickCosmeticCatalog::GetItems(0)[0], FString(TEXT("CARBON")));
	TestEqual(TEXT("Original border index is stable"), FlickCosmeticCatalog::GetItems(2)[0], FString(TEXT("STANDARD")));
	TestEqual(TEXT("Default trail preserves the original look"), FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::TrailCategory)[0], FString(TEXT("NONE")));
	TestEqual(TEXT("Default spawn preserves the original drop"), FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::SpawnCategory)[0], FString(TEXT("BASIC DROP")));
	TestEqual(TEXT("Default knockout preserves the original burst"), FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::KnockoutCategory)[0], FString(TEXT("BASIC BURST")));
	for (int32 Category = 0; Category < FlickCosmeticCatalog::CategoryCount; ++Category)
	{
		TestFalse(TEXT("Every category has a name"), FlickCosmeticCatalog::GetCategoryName(Category).IsEmpty());
		TestFalse(TEXT("Every category has a persistent key"), FlickCosmeticCatalog::GetConfigKey(Category).IsEmpty());
		TestTrue(TEXT("Every category has an owned item"), FlickCosmeticCatalog::GetItems(Category).Num() > 0);
		if (FlickCosmeticCatalog::IsPuckCategory(Category))
		{
			TestEqual(TEXT("Puck category maps to its archetype"),
				static_cast<int32>(FlickCosmeticCatalog::GetPuckArchetype(Category)),
				Category - FlickCosmeticCatalog::PuckCategoryStart);
			TestEqual(TEXT("Four palettes are available only for Standard"), FlickCosmeticCatalog::GetItems(Category).Num(), Category == FlickCosmeticCatalog::PuckCategoryStart ? 6 : 2);
			TestEqual(TEXT("Existing saved skin index is still Classic Blue"), FlickCosmeticCatalog::GetItems(Category)[0], FString(TEXT("CLASSIC BLUE")));
			TestEqual(TEXT("Orange is freely selectable"), FlickCosmeticCatalog::GetItems(Category)[1], FString(TEXT("CLASSIC ORANGE")));
		}
	}
	return true;
}

#endif
