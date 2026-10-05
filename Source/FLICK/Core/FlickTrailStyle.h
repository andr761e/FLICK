#pragma once

#include "CoreMinimal.h"

// Saved trail indices are append-only. These values tune presentation, never physics.
namespace FlickTrailStyle
{
	constexpr int32 Count = 13;
	struct FStyle
	{
		FLinearColor Primary, Secondary;
		float Lifetime, Width, Glow, ParticleShape;
		int32 Lanes, ParticleStride;
		const TCHAR* Description;
	};
	const FStyle& Get(int32 Index);
	FLinearColor Rainbow(float Phase);
}
