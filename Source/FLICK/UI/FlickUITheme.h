#pragma once

#include "CoreMinimal.h"

// Electric / Coral menu presentation. World lighting, team identity and cosmetics
// remain independent; coral identifies actions while blue establishes surfaces.
namespace FlickUITheme
{
	inline const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(2, 7, 10, 252));
	inline const FLinearColor Electric = FLinearColor::FromSRGBColor(FColor(2, 151, 222));
	inline const FLinearColor Panel = FLinearColor::FromSRGBColor(FColor(6, 65, 101, 248));
	inline const FLinearColor PanelRaised = FLinearColor::FromSRGBColor(FColor(7, 80, 118, 252));
	inline const FLinearColor PanelSheen = FLinearColor::FromSRGBColor(FColor(8, 125, 173));
	inline const FLinearColor PanelSelected = FLinearColor::FromSRGBColor(FColor(34, 62, 85, 255));
	inline const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(249, 252, 255));
	inline const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(201, 225, 239));
	inline const FLinearColor Brand = FLinearColor::FromSRGBColor(FColor(252, 115, 77));
	inline const FLinearColor BrandHover = FLinearColor::FromSRGBColor(FColor(255, 148, 112));
	inline const FLinearColor Ice = FLinearColor::FromSRGBColor(FColor(92, 242, 255));
	inline const FLinearColor Cyan(0.0f, 0.82f, 1.0f, 1.0f);
	inline const FLinearColor Orange(1.0f, 0.31f, 0.055f, 1.0f);
	inline const FLinearColor Hairline = FLinearColor::FromSRGBColor(FColor(55, 167, 210, 210));
	inline const FLinearColor Track = FLinearColor::FromSRGBColor(FColor(28, 63, 85));
	inline bool IsMenuSurface(const FLinearColor Fill)
	{
		for (const FLinearColor Surface : {Panel, PanelRaised, PanelSelected, Ink})
			if (FMath::IsNearlyEqual(Fill.R, Surface.R) && FMath::IsNearlyEqual(Fill.G, Surface.G)
				&& FMath::IsNearlyEqual(Fill.B, Surface.B)) return true;
		return false;
	}

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
		// Match only named UI surfaces. Never infer a material's purpose from its
		// hue: cosmetic and team colours can also be blue.
		const auto SameSurface = [Fill](const FLinearColor Surface) {
			return FMath::IsNearlyEqual(Fill.R, Surface.R) && FMath::IsNearlyEqual(Fill.G, Surface.G)
				&& FMath::IsNearlyEqual(Fill.B, Surface.B);
		};
		const bool bSurface = SameSurface(Panel) || SameSurface(PanelRaised) || SameSurface(PanelSelected);
		FLinearColor Highlight = bSurface ? FMath::Lerp(Fill, PanelSheen, 0.28f) : Fill;
		Highlight.A = Fill.A;
		return Highlight;
	}
	constexpr float ReferenceWidth = 1600.0f;
	constexpr float ReferenceHeight = 900.0f;
}
