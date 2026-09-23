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
