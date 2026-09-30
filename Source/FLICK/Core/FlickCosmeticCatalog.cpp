#include "Core/FlickCosmeticCatalog.h"
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
		TEXT("RED CRYSTAL"), TEXT("ROBOTIC"), TEXT("ROCK"), TEXT("TUNDRA")};
	// Keep the original three indices stable for existing saved profiles.
	static const TArray<FString> Tags = {
		TEXT("READY TO FLICK"), TEXT("TABLE TACTICIAN"), TEXT("RIVAL INCOMING"),
		TEXT("ANGLE ARCHITECT"), TEXT("RIM RUNNER"), TEXT("BANK SHOT ARTIST"),
		TEXT("LAST PUCK STANDING"), TEXT("POCKET SPECIALIST"), TEXT("NO EASY SHOTS"),
		TEXT("CALCULATED CHAOS"), TEXT("ONE MORE ROUND"), TEXT("THE CLEAN SWEEP")};
	static const TArray<FString> Borders = {TEXT("STANDARD"), TEXT("CYAN CIRCUIT"), TEXT("LIME CHAMPION"),
		TEXT("GOLD"), TEXT("ICE"), TEXT("PURPLE CRYSTAL"), TEXT("ROBOTIC")};
	static const TArray<FString> OriginalPuck = {TEXT("CLASSIC BLUE"), TEXT("CLASSIC ORANGE")};
	static const TArray<FString> Trails = {TEXT("NONE"), TEXT("ION WAKE"), TEXT("EMBER WAKE")};
	static const TArray<FString> Spawns = {TEXT("BASIC DROP"), TEXT("PULSE ARRIVAL"), TEXT("SPARK ARRIVAL")};
	static const TArray<FString> Knockouts = {TEXT("BASIC BURST"), TEXT("SHOCKWAVE"), TEXT("SPARK SHOWER")};
	switch (Category)
	{
	case 0: return Banners;
	case 1: return Tags;
	case 2: return Borders;
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
	return Skin == 1 ? FLinearColor(1.0f, .18f, .003f) : FLinearColor(0.0f, .55f, 1.0f);
}
