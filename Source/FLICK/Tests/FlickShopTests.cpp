#if WITH_DEV_AUTOMATION_TESTS
#include "Core/FlickShopCatalog.h"
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickShopCatalogTest, "FLICK.Shop.CatalogAndFilters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickShopCatalogTest::RunTest(const FString& Parameters)
{
	const auto Items = FlickShopCatalog::GetItems();
	TestEqual(TEXT("Six distinct fictional items"), Items.Num(), 6);
	TSet<FName> Ids;
	FFlickShopDemoState Demo;
	int32 Finishes = 0, Effects = 0, Identity = 0;
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		const auto& Item = Items[Index];
		TestFalse(TEXT("Item IDs are unique"), Ids.Contains(Item.Id));
		Ids.Add(Item.Id);
		TestTrue(TEXT("Prices are positive and affordable from a fresh demo"), Item.Price > 0 && Item.Price <= FFlickShopDemoState::StartingCredits);
		TestTrue(TEXT("Featured includes every item"), Demo.Matches(Index, EFlickShopFilter::All));
		TestFalse(TEXT("Collection starts empty"), Demo.Matches(Index, EFlickShopFilter::Collection));
		Finishes += Demo.Matches(Index, EFlickShopFilter::Finishes) ? 1 : 0;
		Effects += Demo.Matches(Index, EFlickShopFilter::Effects) ? 1 : 0;
		Identity += Demo.Matches(Index, EFlickShopFilter::Identity) ? 1 : 0;
	}
	TestEqual(TEXT("One puck finish"), Finishes, 1);
	TestEqual(TEXT("Three effect types"), Effects, 3);
	TestEqual(TEXT("Two identity items"), Identity, 2);
	TestNull(TEXT("Negative selection is safe"), FlickShopCatalog::Find(-1));
	TestNull(TEXT("Out of range selection is safe"), FlickShopCatalog::Find(6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickShopDemoTest, "FLICK.Shop.DemoAcquisitionAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickShopDemoTest::RunTest(const FString& Parameters)
{
	FFlickShopDemoState Demo;
	TestEqual(TEXT("Demo starts with clearly fictional credits"), Demo.GetCredits(), 2000);
	TestEqual(TEXT("Inspecting/invalid selection never charges"), Demo.Acquire(-1), EFlickShopDemoResult::InvalidItem);
	TestEqual(TEXT("First demo acquisition succeeds"), Demo.Acquire(0), EFlickShopDemoResult::Acquired);
	TestEqual(TEXT("Exactly one price is deducted"), Demo.GetCredits(), 1100);
	TestTrue(TEXT("Acquired concept appears in demo collection"), Demo.Matches(0, EFlickShopFilter::Collection));
	TestEqual(TEXT("Duplicate acquisition is rejected"), Demo.Acquire(0), EFlickShopDemoResult::AlreadyOwned);
	TestEqual(TEXT("Duplicate acquisition never charges again"), Demo.GetCredits(), 1100);
	TestEqual(TEXT("Second purchase succeeds"), Demo.Acquire(3), EFlickShopDemoResult::Acquired);
	TestEqual(TEXT("Exact remaining balance can be spent"), Demo.Acquire(4), EFlickShopDemoResult::Acquired);
	TestEqual(TEXT("Balance can reach zero, never negative"), Demo.GetCredits(), 0);
	TestEqual(TEXT("Insufficient balance has explicit result"), Demo.Acquire(1), EFlickShopDemoResult::InsufficientCredits);
	TestFalse(TEXT("Insufficient balance never awards an item"), Demo.Owns(1));
	TestEqual(TEXT("Only successful acquisitions are counted"), Demo.GetOwnedCount(), 3);
	FFlickShopDemoState IndependentSession;
	TestEqual(TEXT("Other sessions are unaffected"), IndependentSession.GetOwnedCount(), 0);
	Demo.Reset();
	TestEqual(TEXT("Reset restores demo balance"), Demo.GetCredits(), 2000);
	TestEqual(TEXT("Reset clears only demo ownership"), Demo.GetOwnedCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickShopArtworkTest, "FLICK.Shop.ImportedArtwork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickShopArtworkTest::RunTest(const FString& Parameters)
{
	for (const auto& Item : FlickShopCatalog::GetItems())
	{
		const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Item.GetTexturePath());
		if (!TestNotNull(FString::Printf(TEXT("Product render exists: %s"), Item.Name), Texture)) continue;
#if WITH_EDITORONLY_DATA
		TestEqual(TEXT("Product art retains its source width"), Texture->Source.GetSizeX(), int64(1536));
		TestEqual(TEXT("Product art retains its source height"), Texture->Source.GetSizeY(), int64(1024));
#endif
		TestEqual(TEXT("Shop art is compressed, not raw UI RGBA"), Texture->CompressionSettings, static_cast<uint8>(TC_BC7));
		TestEqual(TEXT("Shop art uses the UI texture group"), Texture->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
	}
	return true;
}
#endif
