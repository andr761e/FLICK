#include "Core/FlickCosmeticCatalog.h"

bool FlickCosmeticCatalog::IsPuckCategory(const int32 Category)
{
	return Category >= PuckCategoryStart && Category < CategoryCount;
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
	static const TArray<FString> OriginalPuck = {TEXT("ORIGINAL")};
	switch (Category)
	{
	case 0: return Banners;
	case 1: return Tags;
	case 2: return Borders;
	default: return OriginalPuck;
	}
}
