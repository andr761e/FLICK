#pragma once

#include "CoreMinimal.h"

// Standard-only vector-parameter recolors. The original mesh and shader are shared.
namespace FlickPuckPalette
{
	inline FString MaterialPath(const int32 Skin, FString Slot)
	{
		if (Skin < 2 || Skin > 5) return FString();
		Slot.ReplaceInline(TEXT("_"), TEXT(" "));
		const TCHAR* Role = nullptr;
		if (Slot.Contains(TEXT("Cyan light diffuser"), ESearchCase::IgnoreCase)) Role = TEXT("Diffuser");
		else if (Slot.Contains(TEXT("Cyan center emblem"), ESearchCase::IgnoreCase)) Role = TEXT("Emblem");
		else if ((Skin == 3 || Skin == 5) && Slot.Contains(TEXT("Graphite anodized housing"), ESearchCase::IgnoreCase)) Role = TEXT("Housing");
		else if (Skin == 5 && Slot.Contains(TEXT("Circular brushed silver"), ESearchCase::IgnoreCase)) Role = TEXT("Crown");
		else if (Skin == 5 && Slot.Contains(TEXT("Machined edge highlights"), ESearchCase::IgnoreCase)) Role = TEXT("Edges");
		if (!Role) return FString();
		static const TCHAR* Sets[] = {TEXT("Emerald"), TEXT("Amethyst"), TEXT("Crimson"), TEXT("Amber")};
		return FString::Printf(TEXT("/Game/Cosmetics/Pucks/%s/MI_Standard_%s.MI_Standard_%s"), Sets[Skin - 2], Role, Role);
	}
}
