#include "Core/FlickSpawnStyle.h"

const FlickSpawnStyle::FStyle& FlickSpawnStyle::Get(const int32 Index)
{
	static const FStyle Styles[Count] = {
		{EPattern::Drop, {0.3f,0.4f,0.5f}, {0.6f,0.7f,0.8f}, 0,0,0,0,1,0,
			TEXT("The original clean puck drop, without additional light or particles.")},
		{EPattern::Pulse, {0.01f,0.65f,1}, {0.08f,1,1}, 64,110,20,.55f,1,24,
			TEXT("Three descending cyan scanner rings meet a soft landing pulse and fine ion dust.")},
		{EPattern::Sparks, {1,.16f,.008f}, {1,.65f,.035f}, 65,110,22,.65f,5,32,
			TEXT("A golden ignition ring draws in sparks before releasing an amber shower on landing.")},
		{EPattern::Ice, {.002f,.18f,1}, {.015f,.7f,1}, 68,130,19,.75f,2,32,
			TEXT("A faceted ice crown grows around the arrival, then dissolves into drifting frost diamonds.")},
		{EPattern::Fire, {1,.085f,.002f}, {1,.55f,.01f}, 70,150,23,.7f,5,36,
			TEXT("Twisting flame ribbons climb from a molten seal; the landing scatters glowing embers.")},
		{EPattern::Portal, {.42f,.015f,1}, {.015f,.65f,1}, 66,115,22,.65f,2,28,
			TEXT("Counter-rotating violet and cyan portals descend and fold into an expanding floor seal.")},
		{EPattern::Beam, {.005f,.55f,1}, {.025f,.95f,1}, 64,180,24,.6f,1,32,
			TEXT("A tall energy column, fine vertical filaments and travelling rings assemble the puck.")},
		{EPattern::Lightning, {.48f,.015f,1}, {.15f,.48f,1}, 72,175,24,.6f,5,32,
			TEXT("Branching electric bolts connect an overhead strike to a crackling violet landing seal.")},
		{EPattern::Prism, {1,.015f,.2f}, {.01f,.8f,1}, 65,155,20,.65f,5,32,
			TEXT("A spectrum of coloured light shafts and rainbow rings showers the arrival with stardust.")},
		{EPattern::Gold, {1,.5f,.008f}, {1,.82f,.16f}, 65,150,20,.7f,5,40,
			TEXT("A gilded light curtain, concentric gold seals and delicate ascending star sparks.")},
		{EPattern::Hearts, {1,.035f,.25f}, {1,.18f,.55f}, 68,125,18,.8f,4,24,
			TEXT("Pink hearts spiral gently upwards through a rose-coloured arrival halo and soft sparkles.")},
		{EPattern::Pixel, {.01f,.24f,1}, {.01f,.8f,1}, 65,145,20,.65f,3,40,
			TEXT("Blue voxel fragments assemble downwards around the puck and disintegrate into digital dust.")},
		{EPattern::Galaxy, {.32f,.01f,1}, {.015f,.45f,1}, 76,135,22,.8f,5,40,
			TEXT("A tilted spiral galaxy gathers above the puck with orbiting stars and a violet-blue floor nebula.")},
		{EPattern::Spirit, {.005f,.85f,.62f}, {.015f,.45f,1}, 68,150,20,.75f,1,28,
			TEXT("Translucent turquoise spirit ribbons curl upwards and settle into a lingering spectral halo.")}
	};
	return Styles[Index >= 0 && Index < Count ? Index : 0];
}

FLinearColor FlickSpawnStyle::Spectrum(const float Phase)
{
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Frac(Phase) * 255), 245, 255);
}
