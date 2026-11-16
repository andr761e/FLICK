#include "Core/FlickRenderStability.h"
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> StableLighting(TEXT("flick.Visual.StableArenaLighting"),1,
		TEXT("Use stable arena/replay lighting. 0 restores the previous rig refresh and quality."));
	TAutoConsoleVariable<int32> PausedSampling(TEXT("flick.Visual.PauseStableSampling"),1,
		TEXT("Let a settled paused replay use Unreal's paused temporal sampling. Moving cameras and seeks remain live."));
	TAutoConsoleVariable<float> ReflectionQuality(TEXT("flick.Visual.ArenaReflectionQuality"),1.f,
		TEXT("Lumen reconstruction quality for Knockout (1-2). Does not change light intensities."));
	TAutoConsoleVariable<float> GatherQuality(TEXT("flick.Visual.ArenaFinalGatherQuality"),1.f,
		TEXT("Lumen final gather quality for Knockout (1-2)."));
}
bool FlickRenderStability::IsEnabled() {return StableLighting.GetValueOnGameThread()!=0;}
bool FlickRenderStability::PauseStableSampling() {return IsEnabled()&&PausedSampling.GetValueOnGameThread()!=0;}
float FlickRenderStability::ReflectionQuality() {return FMath::Clamp(::ReflectionQuality.GetValueOnGameThread(),1.f,2.f);}
float FlickRenderStability::FinalGatherQuality() {return FMath::Clamp(GatherQuality.GetValueOnGameThread(),1.f,2.f);}
