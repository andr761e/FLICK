#pragma once

#include "CoreMinimal.h"
#include "Core/FlickPieceArchetypeRules.h"

// Stable category and item indices preserve the existing profile settings.
// Add new owned appearances to a category's item list; do not reorder entries.
namespace FlickCosmeticCatalog
{
	constexpr int32 PuckCategoryStart = 3;
	constexpr int32 CategoryCount = PuckCategoryStart + FlickPieceArchetypeRules::ArchetypeCount;

	FLICK_API bool IsPuckCategory(int32 Category);
	FLICK_API EFlickPieceArchetype GetPuckArchetype(int32 Category);
	FLICK_API FString GetCategoryName(int32 Category);
	FLICK_API FString GetConfigKey(int32 Category);
	FLICK_API const TArray<FString>& GetItems(int32 Category);
}
