#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

namespace FlickLineupRules
{
	inline constexpr int32 PiecesPerLineup = 4;
	FLICK_API bool IsValid(const TArray<EFlickPieceArchetype>& Lineup);
}
