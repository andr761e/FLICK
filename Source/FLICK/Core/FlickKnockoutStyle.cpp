#include "Core/FlickKnockoutStyle.h"

const FlickKnockoutStyle::FStyle& FlickKnockoutStyle::Get(const int32 Index)
{
	static const FStyle Styles[Count] = {
		{EPattern::Basic,{.3f,.4f,.5f},{.6f,.7f,.8f},.9f,155,80,0,1,10,
			TEXT("The original compact team-coloured burst, without additional cosmetic layers.")},
		{EPattern::Shock,{.005f,.6f,1},{.02f,.95f,1},1.25f,170,45,22,1,32,
			TEXT("Three staggered cyan pressure rings sweep outwards with fine radial ion streaks.")},
		{EPattern::Sparks,{1,.18f,.002f},{1,.65f,.025f},1.45f,160,120,23,5,52,
			TEXT("An amber ignition scatters ballistic sparks, bright comet heads and fine falling embers.")},
		{EPattern::Ice,{.005f,.24f,1},{.04f,.8f,1},1.55f,150,110,20,2,36,
			TEXT("Rotating crystalline shards explode through a double frost shockwave and cold diamond dust.")},
		{EPattern::Fire,{1,.055f,.001f},{1,.45f,.008f},1.55f,160,140,24,5,44,
			TEXT("A molten blast unfolds into turbulent flame tongues, a hot pressure wave and descending cinders.")},
		{EPattern::Vortex,{.38f,.005f,1},{.01f,.4f,1},1.65f,145,115,23,2,36,
			TEXT("Violet spiral arms pull fragments into a dark centre before releasing a cyan-violet aftershock.")},
		{EPattern::Lightning,{.55f,.008f,1},{.85f,.5f,1},1.3f,165,120,25,5,32,
			TEXT("Branching violet arcs jump out from the knockout, with sharp electric cores and a crackling halo.")},
		{EPattern::Toxic,{.15f,1,.002f},{.5f,1,.015f},1.65f,135,140,19,9,30,
			TEXT("Lime vapour billows from a rippling toxic seal while glowing bubbles rise and disperse.")},
		{EPattern::Gold,{1,.48f,.005f},{1,.8f,.12f},1.5f,165,115,23,5,48,
			TEXT("A gilded starburst sends long gold filaments and sparkling flecks through concentric golden rings.")},
		{EPattern::Pixel,{.002f,.2f,1},{.005f,.75f,1},1.4f,135,120,22,3,48,
			TEXT("Blue voxel fragments burst into stepped digital streaks and dissolve in staggered pulses.")},
		{EPattern::Hearts,{1,.012f,.25f},{1,.12f,.5f},1.65f,130,135,19,4,30,
			TEXT("A rose-coloured pop releases tumbling pink hearts, soft sparkles and a delicate expanding halo.")},
		{EPattern::Water,{.002f,.3f,1},{.005f,.85f,1},1.55f,155,120,22,9,40,
			TEXT("An arcing blue splash crown breaks into luminous droplets and bubbles above two fading ripples.")},
		{EPattern::Spirit,{.002f,.8f,.55f},{.005f,.5f,1},1.7f,120,150,20,1,28,
			TEXT("Turquoise spectral wisps spiral away from the puck, trailing a feathered mist and drifting light.")},
		{EPattern::Galaxy,{.55f,.005f,1},{.005f,.4f,1},1.7f,150,125,23,5,48,
			TEXT("A tilted spiral rift opens in violet-blue nebula ribbons, then disperses into orbiting star sparks.")},
		{EPattern::Comic,{.8f,.85f,1},{1,1,1},1.2f,165,110,12,3,28,
			TEXT("A crisp white ink-style starburst snaps open with asymmetric spikes and scattered graphic fragments.")},
		{EPattern::Confetti,{1,.02f,.22f},{.005f,.65f,1},1.7f,145,135,18,3,56,
			TEXT("A colourful celebration bursts into tumbling rainbow strips and a sparkling spectrum halo.")},
		{EPattern::Hologram,{.005f,.7f,1},{.65f,.025f,1},1.6f,150,120,22,2,36,
			TEXT("Iridescent faceted shards fracture outwards with cyan-violet edge glints and prismatic dust.")},
		{EPattern::Arcane,{.5f,.005f,1},{.65f,.15f,1},1.65f,140,110,23,5,32,
			TEXT("Counter-rotating violet seals, radial sigils and orbiting star motes release an arcane aftershock.")},
		{EPattern::Prism,{1,.01f,.15f},{.005f,.7f,1},1.4f,165,130,23,2,40,
			TEXT("A vivid spectrum flash fans into rainbow light rays, crystalline fragments and fine chromatic dust.")},
		{EPattern::Smoke,{.2f,.26f,.35f},{.65f,.7f,.8f},1.7f,130,140,10,1,32,
			TEXT("A soft graphite-grey smoke bloom rolls upwards, shedding pale dust as the cloud dissipates.")}
	};
	return Styles[Index >= 0 && Index < Count ? Index : 0];
}

FLinearColor FlickKnockoutStyle::Spectrum(const float Phase)
{
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Frac(Phase)*255),245,255);
}
