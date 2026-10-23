#pragma once

#include "CoreMinimal.h"

// Profile overview only, in the shared 1600 x 900 frontend design frame.
namespace FlickProfileStyle
{
	constexpr float ContentWidth = 1468;
	constexpr float ContentHeight = 818;
	constexpr float IdentityWidth = 424;
	constexpr float IdentityGap = 28;
	constexpr float CardGap = 16;
	constexpr float CardPadding = 26;
	constexpr float CardWidth = (ContentWidth - IdentityWidth - IdentityGap - CardGap) / 2;
	inline const FLinearColor Surface = FLinearColor::FromSRGBColor(FColor(3, 14, 22, 244));
	inline const FLinearColor SurfaceHover = FLinearColor::FromSRGBColor(FColor(6, 21, 29, 248));

	// Newest records are stored first; charts read left-to-right chronologically.
	inline TArray<float> NormalizePoints(const TArray<int32>& NewestFirst)
	{
		const int32 Count = FMath::Min(NewestFirst.Num(), 8);
		int32 Peak = 1;
		for (int32 I = 0; I < Count; ++I) Peak = FMath::Max(Peak, NewestFirst[I]);
		TArray<float> Result;
		for (int32 I = Count - 1; I >= 0; --I) Result.Add(FMath::Clamp(NewestFirst[I] / float(Peak), 0.f, 1.f));
		return Result;
	}
}
