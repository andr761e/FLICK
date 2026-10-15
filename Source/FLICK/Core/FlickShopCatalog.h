#pragma once

#include "CoreMinimal.h"

enum class EFlickShopFilter : uint8 { All, Finishes, Effects, Identity, Collection };
enum class EFlickShopDemoResult : uint8 { Acquired, AlreadyOwned, InsufficientCredits, InvalidItem };

struct FFlickShopItem
{
	FName Id;
	const TCHAR* Name;
	const TCHAR* Type;
	const TCHAR* Rarity;
	const TCHAR* ArtName;
	const TCHAR* Tagline;
	const TCHAR* Description;
	EFlickShopFilter Category;
	int32 Price;
	FLinearColor Accent;
	FString GetTexturePath() const;
};

// Fictional catalog only: no relationship to inventory IDs, loadouts or payments.
namespace FlickShopCatalog
{
	TConstArrayView<FFlickShopItem> GetItems();
	const FFlickShopItem* Find(int32 Index);
	const TCHAR* GetFilterName(EFlickShopFilter Filter);
}

// Ephemeral UI-session state. Never saved, replicated or applied to real cosmetics.
struct FFlickShopDemoState
{
	static constexpr int32 StartingCredits = 2000;
	int32 GetCredits() const { return Credits; }
	int32 GetOwnedCount() const { return Owned.Num(); }
	bool Owns(int32 Index) const;
	bool Matches(int32 Index, EFlickShopFilter Filter) const;
	EFlickShopDemoResult Acquire(int32 Index);
	void Reset() { Credits = StartingCredits; Owned.Reset(); }
private:
	int32 Credits = StartingCredits;
	TSet<FName> Owned;
};
