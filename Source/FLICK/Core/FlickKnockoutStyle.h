#pragma once
#include "CoreMinimal.h"

// Append-only profile indices. Presentation parameters never alter puck physics.
namespace FlickKnockoutStyle
{
	constexpr int32 Count = 20;
	enum class EPattern : uint8 { Basic, Shock, Sparks, Ice, Fire, Vortex, Lightning, Toxic, Gold,
		Pixel, Hearts, Water, Spirit, Galaxy, Comic, Confetti, Hologram, Arcane, Prism, Smoke };
	struct FStyle
	{
		EPattern Pattern;
		FLinearColor Primary, Secondary;
		float Duration, Radius, Height, Glow, ParticleMask;
		int32 Particles;
		const TCHAR* Description;
	};
	const FStyle& Get(int32 Index);
	FLinearColor Spectrum(float Phase);
}
