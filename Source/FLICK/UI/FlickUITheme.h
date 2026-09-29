#pragma once

#include "CoreMinimal.h"

// Presentation only. Team identity remains cyan/orange; lime identifies actions.
namespace FlickUITheme
{
	inline const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(5, 12, 17, 246));
	inline const FLinearColor Panel = FLinearColor::FromSRGBColor(FColor(9, 22, 29, 242));
	inline const FLinearColor PanelRaised = FLinearColor::FromSRGBColor(FColor(20, 36, 44, 246));
	inline const FLinearColor PanelSheen = FLinearColor::FromSRGBColor(FColor(30, 49, 57));
	inline const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(241, 245, 240));
	inline const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(177, 194, 200));
	inline const FLinearColor Brand = FLinearColor::FromSRGBColor(FColor(215, 255, 12));
	inline const FLinearColor Ice = FLinearColor::FromSRGBColor(FColor(92, 242, 255));
	inline const FLinearColor Cyan(0.0f, 0.82f, 1.0f, 1.0f);
	inline const FLinearColor Orange(1.0f, 0.31f, 0.055f, 1.0f);
	inline const FLinearColor Hairline = FLinearColor::FromSRGBColor(FColor(99, 136, 149, 170));
	inline const FLinearColor Track = FLinearColor::FromSRGBColor(FColor(62, 83, 94));

	// Shared framing, not layout: small controls and large pages retain their
	// existing sizes. The menu's swept navigation remains home-specific.
	inline TArray<FVector2D> GetPanelOutline(const FVector2D Size, const float RequestedCut)
	{
		const float Cut = FMath::Clamp(RequestedCut, 0.0f, FMath::Min(Size.X, Size.Y) * 0.2f);
		return {{Cut, 0}, {Size.X - Cut, 0}, {Size.X, Cut}, {Size.X, Size.Y - Cut},
			{Size.X - Cut, Size.Y}, {Cut, Size.Y}, {0, Size.Y - Cut}, {0, Cut}};
	}

	inline FLinearColor GetSurfaceHighlight(const FLinearColor Fill)
	{
		// Never recolour cosmetics, team identity, warning states or lime CTAs.
		const bool bGraphite = FMath::Max3(Fill.R, Fill.G, Fill.B) < 0.08f
			&& Fill.G >= Fill.R && Fill.B >= Fill.G && FMath::Abs(Fill.R - Fill.G) < 0.025f;
		FLinearColor Highlight = bGraphite ? FMath::Lerp(Fill, PanelSheen, 0.28f) : Fill;
		Highlight.A = Fill.A;
		return Highlight;
	}
	constexpr float ReferenceWidth = 1600.0f;
	constexpr float ReferenceHeight = 900.0f;
}
