#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickCosmeticCatalog.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickCosmeticCatalogTest,
	"FLICK.Profile.CosmeticCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickCosmeticCatalogTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Locker contains three profile categories and all puck types"),
		FlickCosmeticCatalog::CategoryCount, 3 + FlickPieceArchetypeRules::ArchetypeCount);
	TestEqual(TEXT("Existing banner config key remains compatible"), FlickCosmeticCatalog::GetConfigKey(0), FString(TEXT("BannerStyle")));
	TestEqual(TEXT("Existing tag config key remains compatible"), FlickCosmeticCatalog::GetConfigKey(1), FString(TEXT("BannerTag")));
	TestEqual(TEXT("Existing border config key remains compatible"), FlickCosmeticCatalog::GetConfigKey(2), FString(TEXT("AvatarBorder")));
	TestTrue(TEXT("Banner tag locker has a varied collection"), FlickCosmeticCatalog::GetItems(1).Num() >= 10);
	TestEqual(TEXT("First saved tag keeps its original meaning"), FlickCosmeticCatalog::GetItems(1)[0], FString(TEXT("READY TO FLICK")));
	TestEqual(TEXT("Second saved tag keeps its original meaning"), FlickCosmeticCatalog::GetItems(1)[1], FString(TEXT("TABLE TACTICIAN")));
	TestEqual(TEXT("Third saved tag keeps its original meaning"), FlickCosmeticCatalog::GetItems(1)[2], FString(TEXT("RIVAL INCOMING")));
	TestEqual(TEXT("Eight imported banners follow the original three"), FlickCosmeticCatalog::GetItems(0).Num(), 11);
	TestEqual(TEXT("Four imported avatar borders follow the original three"), FlickCosmeticCatalog::GetItems(2).Num(), 7);
	TestEqual(TEXT("Original banner index is stable"), FlickCosmeticCatalog::GetItems(0)[0], FString(TEXT("CARBON")));
	TestEqual(TEXT("Original border index is stable"), FlickCosmeticCatalog::GetItems(2)[0], FString(TEXT("STANDARD")));
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
			TestEqual(TEXT("Each puck currently has one original appearance"), FlickCosmeticCatalog::GetItems(Category).Num(), 1);
		}
	}
	return true;
}

#endif
