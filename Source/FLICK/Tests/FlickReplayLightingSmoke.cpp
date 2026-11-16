#include "Tests/FlickReplayLightingSmoke.h"
#if !UE_BUILD_SHIPPING
#include "Game/FlickReplaySubsystem.h"
#include "Player/FlickReplayController.h"
#include "Player/FlickCameraPawn.h"
#include "Arena/FlickArenaLighting.h"
#include "Core/FlickLightingSettings.h"
#include "Camera/CameraComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/RectLight.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "UnrealClient.h"

namespace FlickReplayLightingSmoke
{
	bool Tick(UFlickReplaySubsystem& R)
	{
		static FString Id;
		static const bool bEnabled=FParse::Value(FCommandLine::Get(),TEXT("FlickLightingAuditReplay="),Id);
		if(!bEnabled) return false;
		static int32 Stage=0,Sample=0,Mode=0;
		static int32 SeekRevision=0;
		static double Started=FPlatformTime::Seconds(),At=Started,Next=0,LastFrame=Started;
		static FString Output,Audit=TEXT("stage,time,key_id,key_intensity,key_transform,fill_id,fill_intensity\n");
		static float SourceTime=0;
		static uint32 KeyId=0,FillId=0;
		static FString Performance;
		static TArray<double> Frames;
		const double Now=FPlatformTime::Seconds();
		const auto Advance=[&](int32 S) {Stage=S;At=Now;Sample=0;Next=Now;};
		const auto Set=[](const TCHAR* Name,float Value) {if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name)) V->Set(Value,ECVF_SetByConsole);};
		if(Output.IsEmpty()) {FParse::Value(FCommandLine::Get(),TEXT("FlickReplaySmokeOutput="),Output);FParse::Value(FCommandLine::Get(),TEXT("FlickLightingAuditMode="),Mode);IFileManager::Get().MakeDirectory(*Output,true);}
		if(Now-Started>110&&Stage!=99) {FFileHelper::SaveStringToFile(TEXT("FAIL: lighting audit timed out: ")+R.GetStatus(),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
		auto* C=R.IsTheatre()?Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController()):nullptr;
		const auto Capture=[&](const TCHAR* Prefix)
		{
			const auto Rig=FlickArenaLighting::FindRig(R.GetWorld());
			if(!Rig.PuckFinishLight||!Rig.PuckFillLight||!Rig.SkyLightActor)
			{FFileHelper::SaveStringToFile(TEXT("FAIL: a replay light disappeared during the audit"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return false;}
			if(Mode!=0&&KeyId&&(KeyId!=Rig.PuckFinishLight->GetUniqueID()||FillId!=Rig.PuckFillLight->GetUniqueID()))
			{FFileHelper::SaveStringToFile(TEXT("FAIL: local replay lights were recreated during a seek"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return false;}
			KeyId=Rig.PuckFinishLight->GetUniqueID();FillId=Rig.PuckFillLight->GetUniqueID();
			FScreenshotRequest::RequestScreenshot(Output/FString::Printf(TEXT("%s-%02d.png"),Prefix,Sample),false,false);
			if(Rig.PuckFinishLight&&Rig.PuckFillLight) Audit+=FString::Printf(TEXT("%s,%.5f,%u,%.3f,%s,%u,%.3f\n"),Prefix,R.GetTime(),Rig.PuckFinishLight->GetUniqueID(),Rig.PuckFinishLight->RectLightComponent->Intensity,
				*Rig.PuckFinishLight->GetActorTransform().ToString(),Rig.PuckFillLight->GetUniqueID(),Rig.PuckFillLight->RectLightComponent->Intensity);
			++Sample;Next=Now+.15;return true;
		};
		if(Stage==0)
		{
			Set(TEXT("flick.Visual.StableArenaLighting"),Mode==0?0:1);
			Set(TEXT("flick.Visual.PauseStableSampling"),Mode==5?1:0);
			Set(TEXT("flick.Visual.ArenaReflectionQuality"),Mode>=2&&Mode<=4?2:1);
			Set(TEXT("flick.Visual.ArenaFinalGatherQuality"),Mode==4?2:1);
			if(Mode==3||Mode==4) {Set(TEXT("r.Lumen.Reflections.DownsampleFactor"),1);Set(TEXT("r.Lumen.Reflections.DownsampleCheckerboard"),0);}
			if(!R.Watch(Id)) {FFileHelper::SaveStringToFile(TEXT("FAIL: ")+R.GetStatus(),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
			Advance(1);return true;
		}
		else if(Stage==1&&C&&!R.IsLoadingTheatre())
		{
			C->ResetView();if(C->ShowsControls()) C->ToggleControls();R.SetPaused(true);
			SourceTime=FMath::Min(24.f,R.GetDuration()-.5f);R.Seek(SourceTime);Advance(2);
		}
		else if(Stage==2&&!R.IsSeeking()&&Now-At>3)
		{
			if(Mode==5&&(!C||!C->WantsPausedSampling())) {FFileHelper::SaveStringToFile(TEXT("FAIL: paused view did not settle"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
			FString Settings=FString::Printf(TEXT("Mode %d, source %.5f\n"),Mode,R.GetTime());
			for(const TCHAR* Name:{TEXT("r.Lumen.Reflections.DownsampleFactor"),TEXT("r.Lumen.Reflections.DownsampleCheckerboard"),TEXT("r.Lumen.Reflections.Temporal.MaxFramesAccumulated"),TEXT("r.ScreenPercentage"),TEXT("r.Lumen.HardwareRayTracing")})
				if(auto* V=IConsoleManager::Get().FindConsoleVariable(Name)) Settings+=FString::Printf(TEXT("%s = %s\n"),Name,*V->GetString());
			if(C) if(auto* Camera=Cast<AFlickCameraPawn>(C->GetViewTarget())) if(auto* Lens=Camera->FindComponentByClass<UCameraComponent>())
				Settings+=FString::Printf(TEXT("ReflectionQuality %.2f GatherQuality %.2f Camera %s\n"),Lens->PostProcessSettings.LumenReflectionQuality,Lens->PostProcessSettings.LumenFinalGatherQuality,*Camera->GetActorTransform().ToString());
			for(int32 I=0;I<int32(FlickLightingSettings::EControl::Count);++I) Settings+=FString::Printf(TEXT("Local setting %d = %.5f\n"),I,FlickLightingSettings::GetValue(FlickLightingSettings::EScene::Gameplay,FlickLightingSettings::EControl(I)));
			FFileHelper::SaveStringToFile(Settings,*(Output/TEXT("settings.txt")));Advance(3);
		}
		else if(Stage==3&&Now>=Next)
		{
			if(!Capture(TEXT("paused"))) return true;if(Sample>=12) Advance(4);
		}
		else if(Stage==4)
		{
			if(Now-At>.3) {SeekRevision=R.GetPlaybackRevision();R.Seek(SourceTime);Advance(6);}
		}
		else if(Stage==5)
		{
			static int32 Seeks=0;
			Sample=Seeks;
			if(!Capture(TEXT("seek"))) return true; // Snapshot immediately after the same-time seek.
			++Seeks;
			if(Seeks<10) {SeekRevision=R.GetPlaybackRevision();R.Seek(SourceTime);Advance(6);}
			else {Advance(7);}
		}
		else if(Stage==6&&!R.IsSeeking()&&R.GetPlaybackRevision()>SeekRevision) {Stage=5;}
		else if(Stage==7&&Now-At>2)
		{
			R.SetPaused(false);Frames.Reset();LastFrame=Now;Advance(8);
		}
		else if(Stage==8)
		{
			Frames.Add(Now-LastFrame);LastFrame=Now;
			if(Now-At>6)
			{
				Frames.Sort();double Sum=0;for(double F:Frames) Sum+=F;
				Performance=FString::Printf(TEXT("PASS: mode %d, %d frames, mean %.3f ms, p95 %.3f ms, source time %.3f\n"),Mode,Frames.Num(),1000*Sum/Frames.Num(),1000*Frames[FMath::Min(Frames.Num()-1,int32(Frames.Num()*.95))],R.GetTime());
				R.SetPaused(true);Advance(9);
			}
		}
		else if(Stage==9&&C&&Now-At>.8)
		{
			if(Mode==5&&!C->WantsPausedSampling()) {FFileHelper::SaveStringToFile(TEXT("FAIL: view did not settle after playback"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
			C->ToggleFly();
			if(auto* Camera=Cast<AFlickCameraPawn>(C->GetViewTarget())) Camera->AddFreeCameraInput(1,0,0,FVector2D(5,0),false,.1f);
			Advance(10);
		}
		else if(Stage==10&&C&&Now-At>.3)
		{
			if(C->WantsPausedSampling()) {FFileHelper::SaveStringToFile(TEXT("FAIL: moving free camera kept frozen temporal sampling"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("free-camera.png"),false,false);C->ToggleFly();Advance(11);
		}
		else if(Stage==11&&C&&Now-At>.8)
		{
			if(Mode==5&&!C->WantsPausedSampling()) {FFileHelper::SaveStringToFile(TEXT("FAIL: released free camera did not settle"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
			C->SelectPlayer(FMath::Max(0,C->PlayerCount()-1));Advance(12);
		}
		else if(Stage==12&&C&&Now-At>.8)
		{
			if(Mode==5&&!C->WantsPausedSampling()) {FFileHelper::SaveStringToFile(TEXT("FAIL: opposite player view did not settle"),*(Output/TEXT("result.txt")));FPlatformMisc::RequestExit(false);Stage=99;return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("opposite-player.png"),false,false);Advance(13);
		}
		else if(Stage==13&&Now-At>.3)
		{
			FFileHelper::SaveStringToFile(Performance+TEXT("PASS: stable light identities across seeks; camera input wakes rendering; free and opposite-player views settle correctly.\n"),*(Output/TEXT("result.txt")));
			FFileHelper::SaveStringToFile(Audit,*(Output/TEXT("lights.csv")));FPlatformMisc::RequestExit(false);Stage=99;return true;
		}
		return false;
	}
}
#endif
