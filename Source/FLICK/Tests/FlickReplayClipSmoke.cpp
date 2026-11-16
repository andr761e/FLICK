#include "Tests/FlickReplayClipSmoke.h"
#if !UE_BUILD_SHIPPING
#include "Game/FlickReplaySubsystem.h"
#include "Player/FlickReplayController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	bool ClickClipButton(const FString& Label)
	{
		if(!GEngine||!GEngine->GameViewport||!GEngine->GameViewport->GetWindow()) return false;
		TSharedPtr<SButton> Found;
		TFunction<bool(TSharedRef<SWidget>)> HasLabel=[&](TSharedRef<SWidget> W)
		{
			if(W->GetType()==FName(TEXT("STextBlock"))&&StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Label) return true;
			if(auto* Kids=W->GetChildren()) for(int32 I=0;I<Kids->Num();++I) if(HasLabel(Kids->GetChildAt(I))) return true;
			return false;
		};
		TFunction<void(TSharedRef<SWidget>)> Visit=[&](TSharedRef<SWidget> W)
		{
			if(!W->GetVisibility().IsVisible()||Found.IsValid()) return;
			if(W->GetType()==FName(TEXT("SButton"))&&HasLabel(W)) {Found=StaticCastSharedRef<SButton>(W);return;}
			if(auto* Kids=W->GetChildren()) for(int32 I=0;I<Kids->Num();++I) Visit(Kids->GetChildAt(I));
		};
		Visit(GEngine->GameViewport->GetWindow()->GetContent());
		if(Found.IsValid()) Found->SimulateClick();return Found.IsValid();
	}
}

namespace FlickReplayClipSmoke
{
	bool Tick(UFlickReplaySubsystem& R)
	{
		static FString SourceId;
		static const bool bEnabled=FParse::Value(FCommandLine::Get(),TEXT("FlickReplayClipSource="),SourceId);
		if(!bEnabled) return false;
		static int32 Step=0,Camera=0;
		static double Started=FPlatformTime::Seconds(),StageTime=Started;
		static FString Output,ClipId,NestedId;
		static float Start=0,End=0,LastTime=0;
		static bool bLooped=false;
		static int32 PreviewChanges=0,EntriesBeforeSave=0;
		static int32 LimitRevision=0;
		if(Output.IsEmpty()) {FParse::Value(FCommandLine::Get(),TEXT("FlickReplaySmokeOutput="),Output);IFileManager::Get().MakeDirectory(*Output,true);}
		const auto Fail=[&](const FString& Why)
		{
			FFileHelper::SaveStringToFile(FString::Printf(TEXT("FAIL stage %d: %s\n%s"),Step,*Why,*R.GetStatus()),*(Output/TEXT("result.txt")));
			UE_LOG(LogTemp,Error,TEXT("REPLAY_CLIP_SMOKE %s"),*Why);Step=99;FPlatformMisc::RequestExit(false);
		};
		const double Now=FPlatformTime::Seconds();
		const auto Advance=[&](int32 Next) {Step=Next;StageTime=Now;};
		if(Step==99) return true;
		if(Now-Started>100) {Fail(TEXT("Timed out"));return true;}
		if(Step==0)
		{
			FFlickSavedReplay Source;
			if(!UFlickReplaySubsystem::ReadIndex(SourceId,Source)) {Fail(TEXT("Missing fixture recording"));return true;}
			Start=FMath::Min(12.f,Source.Duration*.25f);End=FMath::Min(Start+6.f,Source.Duration-.1f);
			ClipId=R.SaveClip(SourceId,TEXT("The winning moment"),Start,End);
			if(ClipId.IsEmpty()||!R.Delete(SourceId)||!R.Watch(ClipId)) {Fail(TEXT("Cannot open clip after removing original entry"));return true;}
			Advance(1);return true;
		}
		if(Step==1&&R.IsTheatre()&&!R.IsLoadingTheatre())
		{
			if(!R.IsPaused()||FMath::Abs(R.GetTime()-Start)>.05f||R.GetDisplayTime()>.05f) {Fail(TEXT("Clip did not load paused at relative zero"));return true;}
			R.BeginClip();
			if(!R.SetClipRange(.5f,1.5f)) {Fail(TEXT("Cannot edit a range inside a clip"));return true;}
			LastTime=R.GetTime();Advance(20);
		}
		else if(Step==20)
		{
			if(FMath::Abs(R.GetTime()-LastTime)>.03f) {++PreviewChanges;LastTime=R.GetTime();}
			const float In=FMath::Lerp(.2f,.8f,FMath::Clamp(float((Now-StageTime)/2),0.f,1.f));
			R.SetClipRange(In,1.5f);R.PreviewClipBoundary(true);
			if(Now-StageTime>2) {R.PreviewClipBoundary(true,true);Advance(21);}
		}
		else if(Step==21&&!R.IsSeeking()&&Now-StageTime>.5)
		{
			if(PreviewChanges<4||FMath::Abs(R.GetTime()-(Start+.8f))>.05f) {Fail(TEXT("START drag did not preview footage during dragging / settle on release"));return true;}
			PreviewChanges=0;LastTime=R.GetTime();Advance(22);
		}
		else if(Step==22)
		{
			if(FMath::Abs(R.GetTime()-LastTime)>.03f) {++PreviewChanges;LastTime=R.GetTime();}
			R.SetClipRange(.8f,FMath::Lerp(2.f,3.f,FMath::Clamp(float((Now-StageTime)/2),0.f,1.f)));R.PreviewClipBoundary(false);
			if(Now-StageTime>2) {R.PreviewClipBoundary(false,true);Advance(23);}
		}
		else if(Step==23&&!R.IsSeeking()&&Now-StageTime>.5)
		{
			if(PreviewChanges<4||FMath::Abs(R.GetTime()-(Start+3.f))>.05f) {Fail(TEXT("END drag did not preview footage during dragging / settle on release"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("boundary-preview.png"),true,false);
			R.SetClipRange(1.399f,1.5f);R.PreviewClipBoundary(true,true);Advance(40);
		}
		else if(Step==40&&!R.IsSeeking()&&Now-StageTime>.4)
		{
			LimitRevision=R.GetPlaybackRevision();Advance(41);
		}
		else if(Step==41)
		{
			// Repeated drag events have the same clamped START time at END.
			R.SetClipRange(1.399f,1.5f);R.PreviewClipBoundary(true);
			if(R.IsSeeking()||R.GetPlaybackRevision()!=LimitRevision) {Fail(TEXT("Holding START against END repeated the preview seek"));return true;}
			if(Now-StageTime>.7) {R.PreviewClipBoundary(true,true);Advance(42);}
		}
		else if(Step==42&&Now-StageTime>.4)
		{
			if(R.IsSeeking()||R.GetPlaybackRevision()!=LimitRevision) {Fail(TEXT("Releasing unchanged START repeated the preview seek"));return true;}
			R.SetClipRange(1.f,1.5f);R.PreviewClipBoundary(true,true);Advance(43);
		}
		else if(Step==43&&!R.IsSeeking()&&Now-StageTime>.4)
		{
			if(R.GetPlaybackRevision()<=LimitRevision||FMath::Abs(R.GetTime()-(Start+1.f))>.03f) {Fail(TEXT("START preview did not resume when moving away from END"));return true;}
			R.SetClipRange(.5f,.601f);R.PreviewClipBoundary(false,true);Advance(44);
		}
		else if(Step==44&&!R.IsSeeking()&&Now-StageTime>.4)
		{
			LimitRevision=R.GetPlaybackRevision();Advance(45);
		}
		else if(Step==45)
		{
			// Repeated drag events have the same clamped END time at START.
			R.SetClipRange(.5f,.601f);R.PreviewClipBoundary(false);
			if(R.IsSeeking()||R.GetPlaybackRevision()!=LimitRevision) {Fail(TEXT("Holding END against START repeated the preview seek"));return true;}
			if(Now-StageTime>.7) {R.PreviewClipBoundary(false,true);Advance(46);}
		}
		else if(Step==46&&Now-StageTime>.4)
		{
			if(R.IsSeeking()||R.GetPlaybackRevision()!=LimitRevision) {Fail(TEXT("Releasing unchanged END repeated the preview seek"));return true;}
			R.SetClipRange(.5f,1.1f);R.PreviewClipBoundary(false,true);Advance(47);
		}
		else if(Step==47&&!R.IsSeeking()&&Now-StageTime>.4)
		{
			if(R.GetPlaybackRevision()<=LimitRevision||FMath::Abs(R.GetTime()-(Start+1.1f))>.03f) {Fail(TEXT("END preview did not resume when moving away from START"));return true;}
			R.SetClipRange(.5f,1.5f);Advance(2);
		}
		else if(Step==2&&Now-StageTime>.5)
		{
			FScreenshotRequest::RequestScreenshot(Output/TEXT("clip-editor.png"),true,false);
			R.PreviewClip();LastTime=R.GetTime();Advance(3);
		}
		else if(Step==3)
		{
			if(R.GetTime()<LastTime-.4f) bLooped=true;
			LastTime=R.GetTime();
			if(auto* C=Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController()))
			{
				const int32 Desired=FMath::Min(6,int32((Now-StageTime)*2));
				if(Desired!=Camera)
				{
					Camera=Desired;if(Camera<5) C->SelectCamera(EFlickReplayCamera(Camera));else C->SelectPlayer(Camera-5);
				}
			}
			if(Now-StageTime>4)
			{
				if(!bLooped||!R.IsPreviewingClip()) {Fail(TEXT("Range preview did not loop"));return true;}
				EntriesBeforeSave=R.List().Num();
				if(!ClickClipButton(TEXT("REVIEW & SAVE"))) {Fail(TEXT("Review button missing"));return true;}
				Advance(31);
			}
		}
		else if(Step==31&&Now-StageTime>.4)
		{
			if(R.List().Num()!=EntriesBeforeSave) {Fail(TEXT("Review step saved before confirmation"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("review-clip.png"),true,false);
			Advance(33);
		}
		else if(Step==33&&Now-StageTime>.4)
		{
			if(!ClickClipButton(TEXT("CONFIRM & SAVE"))) {Fail(TEXT("Confirmation button missing"));return true;}
			Advance(32);
		}
		else if(Step==32&&Now-StageTime>.4)
		{
			if(R.List().Num()!=EntriesBeforeSave+1||!R.IsEditingClip()) {Fail(TEXT("Saved confirmation not persistent or duplicate clip created"));return true;}
			for(const auto& E:R.List()) if(E.Id!=ClipId) NestedId=E.Id;
			FScreenshotRequest::RequestScreenshot(Output/TEXT("saved-clip.png"),true,false);
			Advance(34);
		}
		else if(Step==34&&Now-StageTime>.4)
		{
			if(!ClickClipButton(TEXT("DONE"))) {Fail(TEXT("Saved confirmation missing Done"));return true;}
			R.Seek(-100);Advance(4);
		}
		else if(Step==4&&!R.IsSeeking()&&Now-StageTime>.5)
		{
			if(FMath::Abs(R.GetTime()-Start)>.05f) {Fail(TEXT("Seeking escaped clip start"));return true;}
			R.JumpMarker(-1,true);R.JumpMarker(1);R.Seek(10000);Advance(5);
		}
		else if(Step==5&&!R.IsSeeking()&&Now-StageTime>.5)
		{
			if(FMath::Abs(R.GetTime()-End)>.05f||FMath::Abs(R.GetDisplayTime()-R.GetViewDuration())>.01f) {Fail(TEXT("Seeking escaped clip end or relative clock is wrong"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("clip-end.png"),true,false);
			R.SetSpeed(2);R.SetPaused(false);Advance(6);
		}
		else if(Step==6&&Now-StageTime>(End-Start)/2.f+2)
		{
			if(!R.IsPaused()||FMath::Abs(R.GetTime()-End)>.05f) {Fail(TEXT("Play from end did not restart and stop within clip"));return true;}
			R.ExitTheatre();Advance(7);return true;
		}
		else if(Step==7&&!R.IsTheatre()&&Now-StageTime>2)
		{
			FFlickSavedReplay Clip,Nested;
			if(!UFlickReplaySubsystem::ReadIndex(ClipId,Clip)||!UFlickReplaySubsystem::ReadIndex(NestedId,Nested)
				||Clip.PayloadId()!=SourceId||Nested.PayloadId()!=SourceId) {Fail(TEXT("Clips lost shared source"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("clip-library.png"),true,false);Advance(8);
		}
		else if(Step==8&&Now-StageTime>1)
		{
			FFileHelper::SaveStringToFile(TEXT("PASS: clip loads after original deletion; relative zero; START and END live drag previews and exact release; no repeated seeks while held against either limit or on unchanged release; preview resumes moving away; review without writing; explicit UI confirmation; persistent success with Done; looping preview; all cameras/player targets; nested save; bounded seeks; end restart and automatic stop at 2x; library persistence.\n"),*(Output/TEXT("result.txt")));
			Step=99;FPlatformMisc::RequestExit(false);
		}
		return false;
	}
}
#endif
