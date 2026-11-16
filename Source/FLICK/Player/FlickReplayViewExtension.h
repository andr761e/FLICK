#pragma once
#include "SceneViewExtension.h"

class AFlickReplayController;

// Unreal treats replay worlds as camera-movable even when paused. Override
// only this viewer's settled view, leaving input and other worlds unaffected.
class FFlickReplayViewExtension : public FSceneViewExtensionBase
{
public:
	FFlickReplayViewExtension(const FAutoRegister& AutoRegister,AFlickReplayController* Owner);
	virtual void SetupViewFamily(FSceneViewFamily& Family) override;
	virtual void SetupView(FSceneViewFamily&,FSceneView&) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}
private:
	TWeakObjectPtr<AFlickReplayController> Controller;
	FIntPoint LastTargetSize=FIntPoint::ZeroValue;
	int32 SettledSizeFrames=0;
};
