#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickTrailStyle.h"
#include "Core/FlickSpawnStyle.h"
#include "Core/FlickKnockoutStyle.h"
#include "Misc/ConfigCacheIni.h"

bool FlickCosmeticCatalog::IsPuckCategory(const int32 Category)
{
	return Category >= PuckCategoryStart && Category < TrailCategory;
}

EFlickPieceArchetype FlickCosmeticCatalog::GetPuckArchetype(const int32 Category)
{
	return IsPuckCategory(Category)
		? static_cast<EFlickPieceArchetype>(Category - PuckCategoryStart)
		: EFlickPieceArchetype::Standard;
}

FString FlickCosmeticCatalog::GetCategoryName(const int32 Category)
{
	switch (Category)
	{
	case 0: return TEXT("BANNER");
	case 1: return TEXT("BANNER TAG");
	case 2: return TEXT("AVATAR BORDER");
	case TrailCategory: return TEXT("PUCK TRAIL");
	case SpawnCategory: return TEXT("SPAWN EFFECT");
	case KnockoutCategory: return TEXT("KNOCKOUT EFFECT");
	default: return IsPuckCategory(Category)
		? GetPieceArchetypeName(GetPuckArchetype(Category)).ToUpper() + TEXT(" PUCK")
		: TEXT("UNKNOWN");
	}
}

FString FlickCosmeticCatalog::GetConfigKey(const int32 Category)
{
	switch (Category)
	{
	case 0: return TEXT("BannerStyle");
	case 1: return TEXT("BannerTag");
	case 2: return TEXT("AvatarBorder");
	case TrailCategory: return TEXT("PuckTrail");
	case SpawnCategory: return TEXT("SpawnEffect");
	case KnockoutCategory: return TEXT("KnockoutEffect");
	default: return IsPuckCategory(Category)
		? TEXT("PuckSkin.") + GetPieceArchetypeName(GetPuckArchetype(Category))
		: FString();
	}
}

const TArray<FString>& FlickCosmeticCatalog::GetItems(const int32 Category)
{
	static const TArray<FString> Banners = {TEXT("CARBON"), TEXT("ARENA"), TEXT("GLACIER"),
		TEXT("BLUE ICE"), TEXT("COLD"), TEXT("CRYSTAL"), TEXT("FIRE RED"),
		TEXT("RED CRYSTAL"), TEXT("ROBOTIC"), TEXT("ROCK"), TEXT("TUNDRA"),
		TEXT("CRYO CIRCUIT"), TEXT("SOLAR FORGE"), TEXT("PHASE RIFT")};
	// Keep the original three indices stable for existing saved profiles.
	static const TArray<FString> Tags = {
		TEXT("READY TO FLICK"), TEXT("TABLE TACTICIAN"), TEXT("RIVAL INCOMING"),
		TEXT("ANGLE ARCHITECT"), TEXT("RIM RUNNER"), TEXT("BANK SHOT ARTIST"),
		TEXT("LAST PUCK STANDING"), TEXT("POCKET SPECIALIST"), TEXT("NO EASY SHOTS"),
		TEXT("CALCULATED CHAOS"), TEXT("ONE MORE ROUND"), TEXT("THE CLEAN SWEEP"),
		TEXT("ICE IN MY VEINS"), TEXT("COLD CALCULATION"), TEXT("FORGED TO WIN"),
		TEXT("HEAT CHECK"), TEXT("PHASE SHIFT"), TEXT("BEYOND THE RIM")};
	static const TArray<FString> Borders = {TEXT("STANDARD"), TEXT("CYAN CIRCUIT"), TEXT("LIME CHAMPION"),
		TEXT("GOLD"), TEXT("ICE"), TEXT("PURPLE CRYSTAL"), TEXT("ROBOTIC"),
		TEXT("CRYO CIRCUIT"), TEXT("SOLAR FORGE"), TEXT("PHASE RIFT")};
	static const TArray<FString> OriginalPuck = {TEXT("CLASSIC BLUE"), TEXT("CLASSIC ORANGE")};
	static const TArray<FString> StandardPuck = {TEXT("CLASSIC BLUE"), TEXT("CLASSIC ORANGE"),
		TEXT("EMERALD"), TEXT("AMETHYST"), TEXT("CRIMSON"), TEXT("AMBER")};
	static const TArray<FString> Trails = {TEXT("NONE"), TEXT("ION WAKE"), TEXT("EMBER WAKE"),
		TEXT("CRYO RIBBON"), TEXT("SOLAR CINDERS"), TEXT("PHASE STREAM"),
		TEXT("LIGHTNING"), TEXT("PRISM"), TEXT("GOLD RUSH"), TEXT("GALAXY"),
		TEXT("PIXEL STREAM"), TEXT("HEARTBEAT"), TEXT("SPIRIT WAKE")};
	static const TArray<FString> Spawns = {TEXT("BASIC DROP"), TEXT("PULSE ARRIVAL"), TEXT("SPARK ARRIVAL"),
		TEXT("CRYO LOCK"), TEXT("SOLAR FLARE"), TEXT("PHASE GATE"),
		TEXT("ENERGY BEAM"), TEXT("LIGHTNING STRIKE"), TEXT("PRISM ARRIVAL"), TEXT("GOLDEN ASCENT"),
		TEXT("HEARTFALL"), TEXT("PIXEL ASSEMBLY"), TEXT("GALAXY GATE"), TEXT("SPIRIT ARRIVAL")};
	static const TArray<FString> Knockouts = {TEXT("BASIC BURST"), TEXT("SHOCKWAVE"), TEXT("SPARK SHOWER"),
		TEXT("CRYO SHATTER"), TEXT("SOLAR NOVA"), TEXT("PHASE COLLAPSE"),
		TEXT("LIGHTNING"), TEXT("TOXIC MELT"), TEXT("GOLD BURST"), TEXT("PIXEL BREAK"),
		TEXT("HEART POP"), TEXT("WATER SPLASH"), TEXT("SPIRIT RELEASE"), TEXT("GALAXY RIFT"),
		TEXT("COMIC POW"), TEXT("CONFETTI POP"), TEXT("HOLOGRAPHIC FRACTURE"), TEXT("ARCANE SEAL"),
		TEXT("PRISM FLASH"), TEXT("SMOKE PUFF")};
	switch (Category)
	{
	case 0: return Banners;
	case 1: return Tags;
	case 2: return Borders;
	case PuckCategoryStart: return StandardPuck;
	case TrailCategory: return Trails;
	case SpawnCategory: return Spawns;
	case KnockoutCategory: return Knockouts;
	default: return OriginalPuck;
	}
}

TArray<int32> FlickCosmeticCatalog::LoadPuckSkins()
{
	TArray<int32> Skins;
	Skins.SetNumZeroed(FlickPieceArchetypeRules::ArchetypeCount);
	for (int32 Index = 0; Index < Skins.Num(); ++Index)
	{
		GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), *GetConfigKey(PuckCategoryStart + Index), Skins[Index], GGameUserSettingsIni);
		if (!GetItems(PuckCategoryStart + Index).IsValidIndex(Skins[Index])) Skins[Index] = 0;
	}
	return Skins;
}

TArray<int32> FlickCosmeticCatalog::LoadPuckEffects()
{
	TArray<int32> Effects;
	Effects.SetNumZeroed(3);
	for (int32 Index = 0; Index < Effects.Num(); ++Index)
	{
		const int32 Category = TrailCategory + Index;
		GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), *GetConfigKey(Category), Effects[Index], GGameUserSettingsIni);
		if (!GetItems(Category).IsValidIndex(Effects[Index])) Effects[Index] = 0;
	}
	return Effects;
}

FLinearColor FlickCosmeticCatalog::GetPuckSkinColor(const int32 Skin)
{
	switch (Skin)
	{
	case 1: return FLinearColor(1.0f, .18f, .003f);
	case 2: return FLinearColor(.003f, 1.f, .12f);
	case 3: return FLinearColor(.30f, .015f, 1.f);
	case 4: return FLinearColor(1.f, .008f, .035f);
	case 5: return FLinearColor(1.f, .32f, .004f);
	default: return FLinearColor(0.0f, .55f, 1.0f);
	}
}

FLinearColor FlickCosmeticCatalog::GetCollectionColor(const int32 Collection)
{
	switch (Collection)
	{
	case 0: return FLinearColor(0.055f, 0.78f, 1.0f);
	case 1: return FLinearColor(1.0f, 0.36f, 0.035f);
	case 2: return FLinearColor(0.64f, 0.12f, 1.0f);
	default: return FLinearColor::White;
	}
}

int32 FlickCosmeticCatalog::GetCollection(const int32 Category, const int32 Item)
{
	if (!GetItems(Category).IsValidIndex(Item)) return INDEX_NONE;
	if (Category == 0 && Item >= 11) return Item - 11;
	if (Category == 1 && Item >= 12) return (Item - 12) / 2;
	if (Category == 2 && Item >= 7) return Item - 7;
	if (Category >= TrailCategory && Category <= KnockoutCategory && Item >= 3 && Item <= 5) return Item - 3;
	return INDEX_NONE;
}

FString FlickCosmeticCatalog::GetDescription(const int32 Category, const int32 Item)
{
	if (Category == TrailCategory && GetItems(Category).IsValidIndex(Item)) return FlickTrailStyle::Get(Item).Description;
	if (Category == SpawnCategory && GetItems(Category).IsValidIndex(Item)) return FlickSpawnStyle::Get(Item).Description;
	if (Category == KnockoutCategory && GetItems(Category).IsValidIndex(Item)) return FlickKnockoutStyle::Get(Item).Description;
	const int32 Collection = GetCollection(Category, Item);
	if (Collection == INDEX_NONE) return GetItems(Category).IsValidIndex(Item) ? GetItems(Category)[Item] : FString();
	static const TCHAR* Descriptions[3][3] = {
		{TEXT("Twin icy filaments, a silver core and a tapered cyan wake."), TEXT("A molten ribbon shedding golden cinders."), TEXT("Interwoven violet and cyan ribbons that ripple behind the puck.")},
		{TEXT("Concentric scanner rings lock into place beneath an orbit of ice shards."), TEXT("A rising crown of embers and two expanding amber halos."), TEXT("Counter-rotating portals spiral inward as the puck arrives.")},
		{TEXT("A crystalline fracture fans outward through a double frost shockwave."), TEXT("A hot white core releases a golden shock ring and falling embers."), TEXT("An inward spiral collapses, then releases a violet aftershock.")}};
	if (Category >= TrailCategory && Category <= KnockoutCategory) return Descriptions[Category - TrailCategory][Collection];
	return FString::Printf(TEXT("%s collection // mix and match freely"),
		Collection == 0 ? TEXT("Cryo Circuit") : Collection == 1 ? TEXT("Solar Forge") : TEXT("Phase Rift"));
}
