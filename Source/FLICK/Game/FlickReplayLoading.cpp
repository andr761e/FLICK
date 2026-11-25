#include "Game/FlickReplaySubsystem.h"
#include "Game/FlickGameState.h"
#include "Player/FlickReplayController.h"
#include "GameFramework/WorldSettings.h"
#include "Pieces/FlickPiece.h"
#include "UI/FlickReplayTheatre.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/DemoNetDriver.h"
#include "NetworkReplayStreaming.h"

void UFlickReplaySubsystem::ShowLoading()
{
	bPreloading=true; bWarmingTheatre=false; LoadingStarted=FPlatformTime::Seconds();
	StableSince=0; WarmFrames=0; LastWarmActorCount=-1;
	bWarmPrerolled=bWarmRewinding=false;PrerollStarted=0;
	Status=TEXT("Loading arena, pucks and cosmetics...");
	if(GEngine&&GEngine->GameViewport)
	{
		LoadingWidget=MakeFlickReplayLoading(this);
		GEngine->GameViewport->AddViewportWidgetContent(LoadingWidget.ToSharedRef(),200);
	}
	TArray<FSoftObjectPath> Assets=Playing.Assets;
	// Legacy files have no manifest. Let the replay stream load its actual scene
	// behind the loading overlay; loading the entire cosmetic catalogue here
	// creates thousands of unrelated pipeline compilations and starves playback.

	PreloadHandle=Assets.IsEmpty()?nullptr:UAssetManager::GetStreamableManager().RequestAsyncLoad(Assets);
}

void UFlickReplaySubsystem::BeginPlayback()
{
	bPreloading=false; bWarmingTheatre=true; Status=TEXT("Preparing replay and warming up the arena...");
	const FString File=Root()/TEXT("Matches")/(Playing.PayloadId()+TEXT(".flick.replay"));
	const TArray<FString> Options={TEXT("ReplayStreamerOverride=LocalFileNetworkReplayStreaming")};
	if(!GetGameInstance()->PlayReplay(File,nullptr,Options))
		PlaybackFailed(TEXT("This replay could not be opened; it may require a different game version."));
}

void UFlickReplaySubsystem::TickLoading()
{
	if(FPlatformTime::Seconds()-LoadingStarted>60)
	{
		PlaybackFailed(TEXT("Replay loading timed out. Return to the library and try again."));
		bReturnToLibrary=true; return;
	}
	if(bPreloading)
	{
		if(!PreloadHandle||PreloadHandle->HasLoadCompleted()) BeginPlayback();
		return;
	}
	if(!bWarmingTheatre||!IsTheatre()) return;
	if(bWarmRewinding)
	{
		if(IsSeeking()||PendingSeek>=0) return;
		bWarmRewinding=false;bWarmPrerolled=true;WarmFrames=0;StableSince=0;LastWarmActorCount=-1;
	}
	const auto* State=GetWorld()->GetGameState<AFlickGameState>();
	int32 Count=0; for(TActorIterator<AFlickPiece> It(GetWorld());It;++It) ++Count;
	if(!State||!Count||(!bWarmPrerolled&&GetTime()<.15f)||IsAsyncLoading()) {StableSince=0; WarmFrames=0;return;}
	// Paused frames do not exercise motion/impact render pipelines. Run a bounded
	// hidden playback pass, then restore the initial checkpoint before opening UI.
	if(!bWarmPrerolled)
	{
		if(PrerollStarted==0) {PrerollStarted=FPlatformTime::Seconds();SetPaused(false);GetWorld()->GetWorldSettings()->DemoPlayTimeDilation=1.f;}
		Status=TEXT("Warming up replay motion and camera effects...");
		if(auto* C=Cast<AFlickReplayController>(GetWorld()->GetFirstPlayerController()))
		{
			const int32 Mode=FMath::Min(4,int32((FPlatformTime::Seconds()-PrerollStarted)*2));
			if(int32(C->GetCameraMode())!=Mode) C->SelectCamera(EFlickReplayCamera(Mode));
		}
		if(FPlatformTime::Seconds()-PrerollStarted>=20||GetTime()>=FMath::Min(10.f,GetDuration()-.05f))
		{
			SetSpeed(1);SetPaused(true);Seek(GetViewStart());bWarmRewinding=true;
		}
		return;
	}
	// Keep the first playable moment still while streaming/render resources settle.
	SetPaused(true);
	if(Count!=LastWarmActorCount) {LastWarmActorCount=Count;StableSince=0;WarmFrames=0;}
	if(StableSince==0) StableSince=FPlatformTime::Seconds();
	++WarmFrames;
	if(auto* Camera=Cast<AFlickReplayController>(GetWorld()->GetFirstPlayerController()))
	{
		if(WarmFrames<=150&&WarmFrames%30==1) {Camera->SelectCamera(EFlickReplayCamera((WarmFrames-1)/30));StableSince=FPlatformTime::Seconds();}
		if(WarmFrames==151) {Camera->ResetView();StableSince=FPlatformTime::Seconds();}
	}
	// Readiness depends on the arena, camera passes and buffered data. A slow
	// renderer (or a frame cap) can stay below 20 FPS even after all are ready.
	Status=TEXT("Preparing camera views and rendering resources...");
	if(WarmFrames>=180&&FPlatformTime::Seconds()-StableSince>=1)
	{
		if(auto* Driver=GetWorld()->GetDemoNetDriver()) if(auto Stream=Driver->GetReplayStreamer())
			if(!Stream->IsDataAvailableForTimeRange(uint32(GetTime()*1000),uint32(FMath::Min(GetDuration(),GetTime()+3.f)*1000)))
			{Status=TEXT("Buffering replay data...");return;}
		bPlaybackRequested=false; Status=TEXT("Ready - choose a camera and press Play");
		HideLoading();
	}
}

void UFlickReplaySubsystem::HideLoading()
{
	bPreloading=bWarmingTheatre=false;
	if(LoadingWidget.IsValid()&&GEngine&&GEngine->GameViewport)
		GEngine->GameViewport->RemoveViewportWidgetContent(LoadingWidget.ToSharedRef());
	LoadingWidget.Reset();
}
