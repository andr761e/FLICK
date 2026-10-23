#pragma once

#include "CoreMinimal.h"

namespace FlickLineupStyle
{
	inline FLinearColor PresetSurface(bool bSelected)
	{
		return FLinearColor::FromSRGBColor(bSelected ? FColor(23,35,20,255) : FColor(6,18,25,250));
	}
	constexpr float WorkspaceHeight = 356;
	constexpr float SlotSize = 138;
	constexpr float StatRowHeight = 25;
	constexpr float ChoiceHeight = 106;
	inline FVector2D SlotPosition(const int32 Slot, const bool bThreePucks)
	{
		const FVector2D Four[] = {{.32f, .25f}, {.72f, .25f}, {.32f, .75f}, {.72f, .75f}};
		const FVector2D Three[] = {{.5f, .25f}, {.29f, .75f}, {.71f, .75f}, {.5f, .9f}};
		return bThreePucks ? Three[FMath::Clamp(Slot, 0, 3)] : Four[FMath::Clamp(Slot, 0, 3)];
	}
}
