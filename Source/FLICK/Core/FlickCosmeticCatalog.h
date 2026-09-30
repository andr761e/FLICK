#pragma once

#include "CoreMinimal.h"
#include "Core/FlickPieceArchetypeRules.h"

// Stable category and item indices preserve the existing profile settings.
// Add new owned appearances to a category's item list; do not reorder entries.
namespace FlickCosmeticCatalog
{
	constexpr int32 PuckCategoryStart = 3;
	constexpr int32 TrailCategory = PuckCategoryStart + FlickPieceArchetypeRules::ArchetypeCount;
	constexpr int32 SpawnCategory = TrailCategory + 1;
	constexpr int32 KnockoutCategory = SpawnCategory + 1;
	constexpr int32 CategoryCount = KnockoutCategory + 1;

	FLICK_API bool IsPuckCategory(int32 Category);
	FLICK_API EFlickPieceArchetype GetPuckArchetype(int32 Category);
	FLICK_API FString GetCategoryName(int32 Category);
	FLICK_API FString GetConfigKey(int32 Category);
	FLICK_API const TArray<FString>& GetItems(int32 Category);
	FLICK_API TArray<int32> LoadPuckSkins();
	FLICK_API TArray<int32> LoadPuckEffects();
	FLICK_API FLinearColor GetPuckSkinColor(int32 Skin);
}
