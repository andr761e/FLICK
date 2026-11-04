#pragma once

#include "CoreMinimal.h"
#include "UI/FlickUITheme.h"

// Home-only colour lighting, drawn in Slate rather than through scene bloom.
namespace FlickHomeSurface
{
	inline const FLinearColor Midnight = FLinearColor::FromSRGBColor(FColor(3, 17, 29, 248));
	inline const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor(224, 247, 73));
	inline const FLinearColor Blue = FLinearColor::FromSRGBColor(FColor(3, 133, 193));
	inline FLinearColor Shade(const FVector2D UV, const FLinearColor Fill, bool bPrimary = false)
	{
		FLinearColor Result;
		if (bPrimary)
		{
			const auto Low = FLinearColor::FromSRGBColor(FColor(195, 68, 35));
			const auto High = FLinearColor::FromSRGBColor(FColor(255, 150, 95));
			const float Light = .16f + .48f * UV.X + .22f * FMath::Square(1.f - UV.Y);
			Result = FMath::Lerp(Low, High, Light);
		}
		else
		{
			const auto Body = FLinearColor::FromSRGBColor(FColor(3, 43, 67));
			const float TopLight = FMath::Exp(-FMath::Square((UV.X - .88f) / .62f) - FMath::Square((UV.Y - .03f) / .34f));
			const float BottomLight = FMath::Exp(-FMath::Square((UV.X - .12f) / .66f) - FMath::Square((UV.Y - 1.f) / .40f));
			Result = FMath::Lerp(Midnight, Body, .25f + .4f * UV.Y);
			Result = FMath::Lerp(Result, Blue, .70f * TopLight + .46f * BottomLight);
		}
		Result.A = Fill.A;
		return Result;
	}
}
