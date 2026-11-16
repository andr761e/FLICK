#include "Player/FlickReplayViewExtension.h"
#include "Player/FlickReplayController.h"
#include "SceneInterface.h"
#include "UnrealClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"

FFlickReplayViewExtension::FFlickReplayViewExtension(const FAutoRegister& AutoRegister,AFlickReplayController* Owner)
	:FSceneViewExtensionBase(AutoRegister),Controller(Owner) {}

void FFlickReplayViewExtension::SetupViewFamily(FSceneViewFamily& Family)
{
	if(!Controller.IsValid()||!Family.Scene||Family.Scene->GetWorld()!=Controller->GetWorld()) return;
	const auto* Player=Controller->GetLocalPlayer();
	if(!Player||!Player->ViewportClient||Family.RenderTarget!=Player->ViewportClient->Viewport) return;
	const auto Size=Family.RenderTarget?Family.RenderTarget->GetSizeXY():FIntPoint::ZeroValue;
	if(Size!=LastTargetSize) {LastTargetSize=Size;SettledSizeFrames=0;}
	else SettledSizeFrames=FMath::Min(SettledSizeFrames+1,32);
	if(Controller->WantsPausedSampling()&&SettledSizeFrames>=32) Family.bWorldIsPaused=true;
}
