#pragma once

#include "CoreMinimal.h"

// Append-only saved indices. All values affect presentation, never puck physics.
namespace FlickSpawnStyle
{
	constexpr int32 Count = 14;
	enum class EPattern : uint8 { Drop, Pulse, Sparks, Ice, Fire, Portal, Beam, Lightning, Prism, Gold, Hearts, Pixel, Galaxy, Spirit };
	struct FStyle
	{
		EPattern Pattern;
		FLinearColor Primary, Secondary;
		float Radius, Height, Glow, Afterglow, ParticleShape;
		int32 Particles;
		const TCHAR* Description;
	};
	const FStyle& Get(int32 Index);
	FLinearColor Spectrum(float Phase);
}
