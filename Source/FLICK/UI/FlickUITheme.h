#pragma once

#include "CoreMinimal.h"

// Presentation only. Team identity remains cyan/orange; lime identifies actions.
namespace FlickUITheme
{
	inline const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(9, 17, 19, 246));
	inline const FLinearColor Panel = FLinearColor::FromSRGBColor(FColor(14, 23, 25, 242));
	inline const FLinearColor PanelRaised = FLinearColor::FromSRGBColor(FColor(22, 33, 36, 242));
	inline const FLinearColor Paper(0.93f, 0.94f, 0.85f, 1.0f);
	inline const FLinearColor Muted(0.48f, 0.55f, 0.53f, 1.0f);
	inline const FLinearColor Brand(0.64f, 0.95f, 0.035f, 1.0f);
	inline const FLinearColor Cyan(0.0f, 0.82f, 1.0f, 1.0f);
	inline const FLinearColor Orange(1.0f, 0.31f, 0.055f, 1.0f);
	inline const FLinearColor Hairline(0.17f, 0.22f, 0.20f, 0.55f);
	constexpr float ReferenceWidth = 1600.0f;
	constexpr float ReferenceHeight = 900.0f;
}
