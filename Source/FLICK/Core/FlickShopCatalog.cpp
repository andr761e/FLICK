#include "Core/FlickShopCatalog.h"

FString FFlickShopItem::GetTexturePath() const
{
	return FString::Printf(TEXT("/Game/UI/Shop/%s.%s"), ArtName, ArtName);
}

TConstArrayView<FFlickShopItem> FlickShopCatalog::GetItems()
{
	static const FFlickShopItem Items[] =
	{
		{TEXT("ion_shift"), TEXT("ION SHIFT"), TEXT("PUCK FINISH"), TEXT("EXOTIC"), TEXT("IonShift"),
		 TEXT("Precision, with a pulse."), TEXT("Obsidian housing. Brushed titanium. Ice-cyan light with a sharp lime signature. A clean new look for the Standard puck."),
		 EFlickShopFilter::Finishes, 900, FLinearColor(.02f, .85f, 1.f)},
		{TEXT("solar_wake"), TEXT("SOLAR WAKE"), TEXT("PUCK TRAIL"), TEXT("RARE"), TEXT("SolarWake"),
		 TEXT("Leave a little heat behind."), TEXT("Braided ribbons of solar plasma and drifting gold embers. A molten signature that follows every shot."),
		 EFlickShopFilter::Effects, 450, FLinearColor(1.f, .42f, .06f)},
		{TEXT("zero_point"), TEXT("ZERO POINT"), TEXT("SPAWN EFFECT"), TEXT("EPIC"), TEXT("ZeroPoint"),
		 TEXT("Your arrival. Reimagined."), TEXT("An orbital gate of violet light, suspended crystal fragments and concentric energy rings. Make an entrance before making your move."),
		 EFlickShopFilter::Effects, 650, FLinearColor(.65f, .28f, 1.f)},
		{TEXT("supernova"), TEXT("SUPERNOVA"), TEXT("KNOCKOUT EFFECT"), TEXT("LEGENDARY"), TEXT("Supernova"),
		 TEXT("Go out in a blaze of color."), TEXT("A stellar corona of blue-white energy, prismatic fragments and magenta stardust. A knockout with its own gravitational pull."),
		 EFlickShopFilter::Effects, 800, FLinearColor(1.f, .23f, .65f)},
		{TEXT("nightfall"), TEXT("NIGHTFALL"), TEXT("PLAYER BANNER"), TEXT("RARE"), TEXT("Nightfall"),
		 TEXT("A quieter kind of statement."), TEXT("Arctic silhouettes beneath an electric aurora, set in a fine titanium frame. A cinematic backdrop for your player identity."),
		 EFlickShopFilter::Identity, 300, FLinearColor(.12f, .9f, .76f)},
		{TEXT("prism_halo"), TEXT("PRISM HALO"), TEXT("AVATAR BORDER"), TEXT("EPIC"), TEXT("PrismHalo"),
		 TEXT("Stand out, from every angle."), TEXT("Faceted amethyst crystals, silver edges and refracted cyan light. Frame your avatar with a little controlled brilliance."),
		 EFlickShopFilter::Identity, 250, FLinearColor(.66f, .43f, 1.f)}
	};
	return Items;
}

const FFlickShopItem* FlickShopCatalog::Find(const int32 Index)
{
	const auto Items = GetItems();
	return Index >= 0 && Index < Items.Num() ? &Items[Index] : nullptr;
}

const TCHAR* FlickShopCatalog::GetFilterName(const EFlickShopFilter Filter)
{
	switch (Filter)
	{
	case EFlickShopFilter::Finishes: return TEXT("PUCK FINISHES");
	case EFlickShopFilter::Effects: return TEXT("EFFECTS");
	case EFlickShopFilter::Identity: return TEXT("PLAYER IDENTITY");
	case EFlickShopFilter::Collection: return TEXT("DEMO COLLECTION");
	default: return TEXT("FEATURED");
	}
}

bool FFlickShopDemoState::Owns(const int32 Index) const
{
	const auto* Item = FlickShopCatalog::Find(Index);
	return Item && Owned.Contains(Item->Id);
}

bool FFlickShopDemoState::Matches(const int32 Index, const EFlickShopFilter Filter) const
{
	const auto* Item = FlickShopCatalog::Find(Index);
	return Item && (Filter == EFlickShopFilter::All || (Filter == EFlickShopFilter::Collection ? Owns(Index) : Item->Category == Filter));
}

EFlickShopDemoResult FFlickShopDemoState::Acquire(const int32 Index)
{
	const auto* Item = FlickShopCatalog::Find(Index);
	if (!Item) return EFlickShopDemoResult::InvalidItem;
	if (Owns(Index)) return EFlickShopDemoResult::AlreadyOwned;
	if (Credits < Item->Price) return EFlickShopDemoResult::InsufficientCredits;
	Credits -= Item->Price;
	Owned.Add(Item->Id);
	return EFlickShopDemoResult::Acquired;
}
