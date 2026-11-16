#include "Tests/FlickReplaySmoke.h"
#include "Tests/FlickReplayClipSmoke.h"
#include "Tests/FlickReplayLightingSmoke.h"
#if !UE_BUILD_SHIPPING
#include "Game/FlickReplaySubsystem.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameState.h"
#include "Game/FlickGameMode.h"
#include "Player/FlickReplayController.h"
#include "Player/FlickPlayerState.h"
#include "Player/FlickPostMatchPresentationComponent.h"
#include "Player/FlickCameraPawn.h"
#include "Pieces/FlickPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/LightComponent.h"
#include "Camera/CameraComponent.h"
#include "Arena/FlickArenaLighting.h"
#include "Arena/FlickTestArena.h"
#include "Core/FlickLightingSettings.h"
#include "Engine/RectLight.h"
#include "Engine/DemoNetDriver.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/Application/SlateApplication.h"

namespace FlickReplaySmoke
{
	bool Tick(UFlickReplaySubsystem& R,float DeltaTime)
	{
		if(FlickReplayLightingSmoke::Tick(R)) return true;
		if(FlickReplayClipSmoke::Tick(R)) return true;
		static FString InspectId;
		static bool bInspect=FParse::Value(FCommandLine::Get(),TEXT("FlickReplayInspect="),InspectId);
		if(bInspect)
		{
			static int32 Step=0;static double Began=FPlatformTime::Seconds(),Last=Began;
			static TArray<double> Frames;static FString Out;
			static FString Motion=TEXT("time,id,raw_x,raw_y,raw_z,shown_x,shown_y,shown_z,simulating,source_time\n");
			if(Out.IsEmpty()) {FParse::Value(FCommandLine::Get(),TEXT("FlickReplaySmokeOutput="),Out);IFileManager::Get().MakeDirectory(*Out,true);}
			const double Now=FPlatformTime::Seconds();
			static const bool bTransition=FParse::Param(FCommandLine::Get(),TEXT("FlickReplayTransitionInspect"));
			static const bool bAngles=FParse::Param(FCommandLine::Get(),TEXT("FlickReplayLightingAngles"));
			static const bool bUi=FParse::Param(FCommandLine::Get(),TEXT("FlickReplayUIInspect"));
			static float OriginalKey=0;
			if(Step==0) {R.Watch(InspectId);Step=1;}
			else if(Step==1&&R.IsTheatre()&&!R.IsLoadingTheatre())
			{
				R.SetPaused(bTransition||bAngles||bUi);Began=Now;Step=bUi?30:bAngles?20:bTransition?10:2;
				float InspectSpeed=1.f;FParse::Value(FCommandLine::Get(),TEXT("FlickReplayInspectSpeed="),InspectSpeed);R.SetSpeed(InspectSpeed);
				if(bAngles) {R.Seek(FMath::Min(24.f,R.GetDuration()-.1f));OriginalKey=FlickLightingSettings::GetValue(FlickLightingSettings::EScene::Gameplay,FlickLightingSettings::EControl::Key);}
				if(bTransition) if(auto* C=Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController())) C->SelectPlayer(0);
			}
			else if(Step==30)
			{
				static int32 Action=0;static TArray<TSharedRef<SComboButton>> Combos;
				if(Now-Began>1+Action)
				{
					auto* C=Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController());
					if(Action==0 && GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWindow().IsValid())
					{
						TFunction<void(TSharedRef<SWidget>)> Visit;
						Visit=[&](TSharedRef<SWidget> W)
						{
							if(!W->GetVisibility().IsVisible()) return;
							if(W->GetType()==FName(TEXT("SComboButton"))) Combos.Add(StaticCastSharedRef<SComboButton>(W));
							if(auto* Children=W->GetChildren()) for(int32 I=0;I<Children->Num();++I) Visit(Children->GetChildAt(I));
						};
						Visit(GEngine->GameViewport->GetWindow()->GetContent());
						FString Sizes;
						for(const auto& Combo:Combos) Sizes+=FString::Printf(TEXT("Combo %s size %s\n"),*Combo->GetCachedGeometry().GetAbsolutePosition().ToString(),*Combo->GetCachedGeometry().GetAbsoluteSize().ToString());
						FFileHelper::SaveStringToFile(Sizes,*(Out/TEXT("ui-geometry.txt")));
					}
					if(Action==1||Action==3||Action==5)
					{
						FSlateApplication::Get().DismissAllMenus();
						const int32 Index=(Action-1)/2;
						if(Combos.IsValidIndex(Index)) Combos[Index]->SetIsOpen(true,false);
					}
					else if(Action==7) {FSlateApplication::Get().DismissAllMenus();if(C) C->ToggleControls();}
					else if(Action==9) {if(C) {C->ToggleControls();C->SelectCamera(EFlickReplayCamera::Puck);}}
					else if(Action==11) {FPlatformMisc::RequestExit(false);Step=31;}
					else FScreenshotRequest::RequestScreenshot(Out/FString::Printf(TEXT("ui-%d.png"),Action),true,false);
					++Action;
				}
			}
			else if(Step==20&&!R.IsSeeking()&&FMath::Abs(R.GetTime()-FMath::Min(24.f,R.GetDuration()-.1f))<.2f&&Now-Began>1)
			{
				if(auto* C=Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController())) C->SelectPlayer(0);
				Began=Now;Step=10;
			}
			else if(Step==10)
			{
				static int32 Action=0;
				if(Now-Began>1+Action)
				{
					auto* C=Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController());
					auto* Camera=C?Cast<AFlickCameraPawn>(C->GetViewTarget()):nullptr;
					if(!C||!Camera) return false;
					const auto* State=R.GetWorld()->GetGameState<AFlickGameState>();
					const FVector Focus(0,0,(State?State->ArenaSurfaceZ:250.f)+20);
					if(bAngles&&Action==1) C->SelectPlayer(1);
					else if(Action==1) C->CycleTarget(-1);
					else if(bAngles&&Action==3) C->CycleTarget(1);
					else if(Action==3) Camera->AddFreeCameraInput(0,0,0,FVector2D(600,0),false,.01f);
					else if(bAngles&&(Action==5||Action==7))
					{
						const FVector Eye(Action==5?1490.f:-1490.f,0,Focus.Z+800);
						Camera->SetActorLocationAndRotation(Eye,(Focus-Eye).Rotation());
					}
					else if(bAngles&&Action==9) Camera->AddFreeCameraInput(0,0,0,FVector2D(600,0),false,.01f);
					else if(bAngles&&Action==11) Camera->SetActorRotation((Focus-Camera->GetActorLocation()).Rotation());
					else if(bAngles&&(Action==13||Action==15))
						FlickLightingSettings::SetValue(FlickLightingSettings::EScene::Gameplay,FlickLightingSettings::EControl::Key,
							Action==13?FlickLightingSettings::GetControl(FlickLightingSettings::EControl::Key).Minimum:OriginalKey);
					else if((!bAngles&&Action==6)||(bAngles&&Action==17)) {FPlatformMisc::RequestExit(false);Step=11;}
					else
					{
						const auto* Lens=Camera->FindComponentByClass<UCameraComponent>();
						FString Audit=FString::Printf(TEXT("Mode %d Target %s Camera %s Bloom %.3f Exposure %.3f Weight %.3f\n"),
							int32(C->GetCameraMode()),*C->TargetLabel(),*Camera->GetActorTransform().ToString(),
							Lens->PostProcessSettings.BloomIntensity,Lens->PostProcessSettings.AutoExposureBias,Lens->PostProcessBlendWeight);
						for(int32 I=0;I<int32(FlickLightingSettings::EControl::Count);++I)
							Audit+=FString::Printf(TEXT("Local setting %d = %.3f\n"),I,FlickLightingSettings::GetValue(FlickLightingSettings::EScene::Gameplay,FlickLightingSettings::EControl(I)));
						for(TActorIterator<AActor> It(R.GetWorld());It;++It)
							if(const auto* Light=It->FindComponentByClass<ULightComponent>())
								Audit+=FString::Printf(TEXT("%s intensity %.3f visible %d hidden %d transform %s\n"),*It->GetName(),Light->Intensity,
									Light->IsVisible(),Light->bHiddenInGame,*It->GetActorTransform().ToString());
						FFileHelper::SaveStringToFile(Audit,*(Out/FString::Printf(TEXT("transition-%d.txt"),Action)));
						FScreenshotRequest::RequestScreenshot(Out/FString::Printf(TEXT("transition-%d.png"),Action),true,false);
					}
					++Action;
				}
			}
			else if(Step==2)
			{
				Frames.Add(Now-Last);
				for(TActorIterator<AFlickPiece> It(R.GetWorld());It;++It)
				{
					const FVector Raw=It->GetReplaySourceLocation(), Shown=It->GetActorLocation();
					static const auto* SampleTime=FindFProperty<FFloatProperty>(AFlickPiece::StaticClass(),TEXT("SavedReplaySampleTime"));
					Motion+=FString::Printf(TEXT("%.5f,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.5f\n"),R.GetTime(),It->GetPieceId(),
						Raw.X,Raw.Y,Raw.Z,Shown.X,Shown.Y,Shown.Z,It->FindComponentByClass<UStaticMeshComponent>()->IsSimulatingPhysics(),SampleTime?SampleTime->GetPropertyValue_InContainer(*It):-1.f);
				}
				static int32 Capture = 0;
				if(Now-Began>1+Capture*4 && Capture<5)
				{
					FScreenshotRequest::RequestScreenshot(Out/FString::Printf(TEXT("camera-%d.png"),Capture),true,false);
					++Capture;
				}
				if(auto* C=Cast<AFlickReplayController>(R.GetWorld()->GetFirstPlayerController()))
				{
					const int32 Mode=FMath::Min(4,int32((Now-Began)/4));
					if(int32(C->GetCameraMode())!=Mode)
					{
						C->SelectCamera(EFlickReplayCamera(Mode));
						for(TActorIterator<AActor> It(R.GetWorld());It;++It) if(It->GetClass()->GetName().Contains(TEXT("Arena"))||It->IsA<AFlickCameraPawn>()) UE_LOG(LogTemp,Display,TEXT("SCENE_AUDIT mode=%d actor=%s class=%s position=%s tick=%d"),Mode,*It->GetName(),*It->GetClass()->GetName(),*It->GetActorLocation().ToString(),It->IsActorTickEnabled());
					}
				}
				if(Now-Began>20)
				{
					Frames.Sort();double Sum=0;int32 Hitches=0;for(double F:Frames) {Sum+=F;if(F>.05) ++Hitches;}
					const FString Report=FString::Printf(TEXT("Frames %d Mean %.2f ms P95 %.2f ms >50ms %d Demo time %.2f / %.2f after 20s"),Frames.Num(),1000*Sum/Frames.Num(),1000*Frames[int32(Frames.Num()*.95)],Hitches,R.GetTime(),R.GetDuration());
					FFileHelper::SaveStringToFile(Report,*(Out/TEXT("inspect.txt")));FScreenshotRequest::RequestScreenshot(Out/TEXT("inspect.png"),true,false);Step=3;Began=Now;
					FFileHelper::SaveStringToFile(Motion,*(Out/TEXT("motion.csv")));
				}
			}
			else if(Step==3&&Now-Began>1) {FPlatformMisc::RequestExit(false);Step=4;}
			if(Now-Last>.2) UE_LOG(LogTemp,Warning,TEXT("REPLAY_INSPECT hitch %.3f at %.3f"),Now-Last,R.GetTime());
			Last=Now;return false;
		}
		static const bool bEnabled=FParse::Param(FCommandLine::Get(),TEXT("FlickReplaySmoke"));
		if (!bEnabled) return false;
		static bool BobShot=false, LoadingCaptured=false, SummaryCaptured=false;
		static int32 CheckedSeat=0;
		static bool ScrubTested=false;
		static TArray<double> LiveFrames, ReplayFrames;
		static double LastFrameTime=FPlatformTime::Seconds();
		static int32 Stage=0,LiveCount=0,ProfileMatches=0;
		static double LastHeartbeat=0;
		static double Started=FPlatformTime::Seconds(),StageTime=Started;
		static FString Output,Id;
		static TMap<int32,FTransform> Reference;
		static FTransform KeyBeforeFree, FillBeforeFree;
		static float BloomBeforeFree=0.f, ExposureBeforeFree=0.f;
		if (Output.IsEmpty()) { FParse::Value(FCommandLine::Get(),TEXT("FlickReplaySmokeOutput="),Output); if(Output.IsEmpty()) Output=FPaths::ProjectSavedDir()/TEXT("ReplaySmoke"); IFileManager::Get().MakeDirectory(*Output,true); }
		const auto Fail=[&](const FString& Error) {FFileHelper::SaveStringToFile(TEXT("FAIL: ")+Error+TEXT("\n")+R.GetStatus(),*(Output/TEXT("result.txt")));UE_LOG(LogTemp,Error,TEXT("REPLAY_SMOKE FAIL: %s"),*Error);Stage=99;FPlatformMisc::RequestExit(false);};
		const auto Advance=[&](int32 Next) {Stage=Next;StageTime=FPlatformTime::Seconds();};
		if (Stage==99) return true;
		if (FPlatformTime::Seconds()-Started>100) {Fail(FString::Printf(TEXT("Timed out at stage %d"),Stage));return true;}
		if(FPlatformTime::Seconds()-LastHeartbeat>2) { LastHeartbeat=FPlatformTime::Seconds(); UE_LOG(LogTemp,Display,TEXT("REPLAY_SMOKE stage %d, time %.3f, seeking %d, paused %d"),Stage,R.GetTime(),R.IsSeeking(),R.IsPaused()); }
		UWorld* World=R.GetWorld(); auto* State=World?World->GetGameState<AFlickGameState>():nullptr;
		auto* Instance=Cast<UFlickGameInstance>(R.GetGameInstance());
		const double FrameNow=FPlatformTime::Seconds(),FrameSeconds=FrameNow-LastFrameTime;LastFrameTime=FrameNow;
		if(Stage==0&&R.GetTime()>2&&R.GetTime()<6) LiveFrames.Add(FrameSeconds);
		if(Stage==30&&R.GetTime()>.5) ReplayFrames.Add(FrameSeconds);
		if(Stage==3&&R.IsLoadingTheatre()&&!LoadingCaptured) {LoadingCaptured=true;FScreenshotRequest::RequestScreenshot(Output/TEXT("loading.png"),true,false);}
		AFlickReplayController* Controller=nullptr;
		if(World) for(TActorIterator<AFlickReplayController> It(World);It;++It) if(It->IsLocalController()) Controller=*It;
		const auto Pose=[&]() {TMap<int32,FTransform> Result; if(World) for(TActorIterator<AFlickPiece> It(World);It;++It) Result.Add(It->GetPieceId(),It->GetActorTransform()); return Result;};
		const auto ActorCount=[&]() {int32 Count=0;if(World) for(TActorIterator<AFlickPiece> It(World);It;++It) ++Count;return Count;};
		if(Stage==0&&!BobShot&&State&&State->ActiveMatchVariant==EFlickMatchVariant::Bob&&R.GetTime()>2)
			if(auto* Mode=World->GetAuthGameMode<AFlickGameMode>()) for(TActorIterator<AFlickPiece> It(World);It;++It)
				if(It->IsBobStriker()&&It->GetTeam()==State->CurrentTeam) {FVector Direction=-It->GetActorLocation();Direction.Z=0;BobShot=Mode->TryLaunchPiece(*It,Direction.GetSafeNormal(),.7f);break;}
		if (Stage==0 && R.IsRecordingMatch() && World->GetDemoNetDriver() && World->GetDemoNetDriver()->GetDemoCurrentTime()>6 && State)
		{
			LiveCount=Pose().Num();
			if(State->HasAuthority())
			{
				if(World->GetNetMode()!=NM_Standalone&&R.GetTime()<10) return false;
				State->RoundsToWin=1; State->CompleteRound(EFlickMatchOutcome::Player1Wins); State->FinalizeAuthoritativeMatch(EFlickMatchOutcome::Player1Wins); Advance(1);
			}
			else if(State->bSeriesComplete) Advance(1);
		}
		else if (Stage==1 && R.CanSave())
		{
			R.SetSaveName(TEXT("  Smoke test - named replay  "));
			if(!R.SaveCurrent()) {Fail(TEXT("SaveCurrent failed"));return true;}
			const auto Files=R.List(); if(Files.IsEmpty()) {Fail(TEXT("Saved file absent from library"));return true;}
			if(Files[0].Name!=TEXT("Smoke test - named replay")) {Fail(TEXT("Custom replay title did not persist"));return true;}
			if(FParse::Param(FCommandLine::Get(),TEXT("FlickReplaySmokeLegacy"))) {auto Legacy=Files[0];Legacy.Assets.Reset();UFlickReplaySubsystem::WriteIndex(Legacy);}
			Id=Files[0].Id; ProfileMatches=Instance?Instance->GetProfileStats().MatchesPlayed:0;
			if(Files[0].PoseSampleRate<119.f) {Fail(TEXT("Recording did not use the configured 120 Hz capture rate"));return true;}
			if(auto* Player=World->GetFirstPlayerController()) if(auto* Presentation=Player->FindComponentByClass<UFlickPostMatchPresentationComponent>()) Presentation->ContinueToSummary();
			FScreenshotRequest::RequestScreenshot(Output/TEXT("post-match.png"),true,false); Advance(2);
		}
		else if(Stage==2&&!SummaryCaptured&&FPlatformTime::Seconds()-StageTime>8) {SummaryCaptured=true;FScreenshotRequest::RequestScreenshot(Output/TEXT("save-name-summary.png"),true,false);}
		else if (Stage==2 && FPlatformTime::Seconds()-StageTime>9)
		{
			if(!(R.NeedsSessionExit()?R.WatchAfterLeaving(Id):R.Watch(Id))) {Fail(TEXT("Watch failed"));return true;} Advance(3); return true;
		}
		else if(Stage==3&&R.IsTheatre()&&!R.IsLoadingTheatre()) {if(!R.IsPaused()){Fail(TEXT("Replay did not start paused"));return true;}R.SetPaused(false);Advance(30);}
		else if (Stage==30 && R.IsTheatre() && Controller && State && R.GetTime()>1 && FPlatformTime::Seconds()-StageTime>2)
		{
			for(TActorIterator<AFlickPiece> It(World);It;++It)
				if(It->FindComponentByClass<UStaticMeshComponent>()->IsSimulatingPhysics()) {Fail(TEXT("Saved playback is resimulating puck physics"));return true;}
			int32 ArenaCount=0;for(TActorIterator<AFlickTestArena> It(World);It;++It) ++ArenaCount;
			if(State->ActiveMatchVariant==EFlickMatchVariant::Classic&&ArenaCount!=1) {Fail(TEXT("Replay contains overlapping arenas"));return true;}
			if(Pose().Num()!=LiveCount||ActorCount()!=LiveCount) {Fail(TEXT("Playback puck count differs from recording"));return true;}
			R.SetPaused(true); FScreenshotRequest::RequestScreenshot(Output/TEXT("theatre.png"),true,false);
			R.Seek(1); Advance(4);
		}
		else if (Stage==4 && !R.IsSeeking() && FMath::Abs(R.GetTime()-1)<.12f && FPlatformTime::Seconds()-StageTime>.5)
		{
			if(!World->IsPaused()) {Fail(TEXT("Pause did not pause the replay world"));return true;}
			Reference=Pose(); UE_LOG(LogTemp,Display,TEXT("REPLAY_SMOKE reference %d unique / %d actors"),Reference.Num(),ActorCount()); FScreenshotRequest::RequestScreenshot(Output/TEXT("paused-seek.png"),true,false); R.Seek(4); Advance(5);
		}
		else if (Stage==5 && !R.IsSeeking() && FMath::Abs(R.GetTime()-4)<.12f && FPlatformTime::Seconds()-StageTime>.5)
		{ R.Seek(FMath::Max(0.f,R.GetDuration()-.02f)); Advance(52); }
		else if(Stage==52&&!R.IsSeeking()&&FMath::Abs(R.GetTime()-(R.GetDuration()-.02f))<.12f&&FPlatformTime::Seconds()-StageTime>.5)
		{
			int32 Visible=0;for(TActorIterator<AFlickPiece> It(World);It;++It) if(!It->IsHidden()&&It->GetPresentationMesh()&&!It->GetPresentationMesh()->bHiddenInGame) ++Visible;
			if(!Visible) {Fail(TEXT("End-of-match presentation hid the replay arena pucks"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("end-of-replay.png"),true,false);R.Seek(1);Advance(6);
		}
		else if (Stage==6 && Controller && !R.IsSeeking() && FMath::Abs(R.GetTime()-1)<.12f && FPlatformTime::Seconds()-StageTime>.5)
		{
			const auto Again=Pose();
			if(Again.Num()!=Reference.Num()) {Fail(FString::Printf(TEXT("Seeking changed puck count: %d -> %d, actors %d"),Reference.Num(),Again.Num(),ActorCount()));return true;}
			for(const auto& Pair:Reference) {const auto* T=Again.Find(Pair.Key); if(!T||!T->GetLocation().Equals(Pair.Value.GetLocation(),2)) {Fail(TEXT("Seeking backward changed recorded positions"));return true;}}
			if(Instance&&Instance->GetProfileStats().MatchesPlayed!=ProfileMatches) {Fail(TEXT("Playback changed profile match totals"));return true;}
			if(!ScrubTested) {ScrubTested=true;R.BeginScrub();R.PreviewScrub(4);Advance(62);}
			else {CheckedSeat=0;Controller->SelectPlayer(CheckedSeat);Advance(61);}
		}
		else if(Stage==62&&FPlatformTime::Seconds()-StageTime>.3)
		{
			if(R.IsSeeking()||FMath::Abs(R.GetTime()-1)>.12f||FMath::Abs(R.GetDisplayTime()-4)>.01f) {Fail(TEXT("Dragging timeline performed a seek before release"));return true;}
			R.EndScrub();Advance(63);
		}
		else if(Stage==63&&!R.IsSeeking()&&FMath::Abs(R.GetTime()-4)<.12f&&FPlatformTime::Seconds()-StageTime>.5) {R.Seek(1);Advance(6);}
		else if(Stage==61&&Controller&&State&&FPlatformTime::Seconds()-StageTime>.25)
		{
			auto* Camera=Cast<AFlickCameraPawn>(Controller->GetViewTarget());
			if(!Camera||Controller->GetCameraMode()!=EFlickReplayCamera::Player||!Camera->CaptureSpectatorView().IsSafe()) {Fail(TEXT("Player camera unavailable"));return true;}
			const int32 Size=FMath::Max(1,State->PlayersPerTeam);
			const auto Team=CheckedSeat<Size?EFlickTeam::Player1:EFlickTeam::Player2;
			for(const APlayerState* Base:State->PlayerArray)
				if(const auto* P=Cast<AFlickPlayerState>(Base);P&&P->GetTeam()==Team&&P->GetTeamPlayerSlot()==CheckedSeat%Size)
				{
					if(P->GetPrivateCameraView().bValid&&!Camera->GetActorLocation().Equals(P->GetPrivateCameraView().Location,2)) {Fail(TEXT("Selected player POV differs from recorded camera"));return true;}
					UE_LOG(LogTemp,Display,TEXT("REPLAY_SMOKE POV %d: recorded=%d"),CheckedSeat,P->GetPrivateCameraView().bValid);
				}
			if(++CheckedSeat<Controller->PlayerCount()) {Controller->SelectPlayer(CheckedSeat);Advance(61);}
			else
			{
				const auto Rig=FlickArenaLighting::FindRig(World);
				if(!Rig.PuckFinishLight||!Rig.PuckFillLight) {Fail(TEXT("Replay studio lights missing"));return true;}
				KeyBeforeFree=Rig.PuckFinishLight->GetActorTransform();FillBeforeFree=Rig.PuckFillLight->GetActorTransform();
				const auto* Lens=Camera->FindComponentByClass<UCameraComponent>();
				BloomBeforeFree=Lens->PostProcessSettings.BloomIntensity;ExposureBeforeFree=Lens->PostProcessSettings.AutoExposureBias;
				FScreenshotRequest::RequestScreenshot(Output/TEXT("before-free.png"),true,false);
				Controller->CycleTarget(1);
				if(Controller->GetCameraMode()!=EFlickReplayCamera::Free) {Fail(TEXT("Target carousel did not enter free camera"));return true;}
				Advance(7);
			}
		}
		else if (Stage==7 && Controller && FPlatformTime::Seconds()-StageTime>.3)
		{
			Controller->ToggleFly();Controller->Exit();
			if(Controller->IsFlying()||!R.IsTheatre()) {Fail(TEXT("Escape did not release fly camera safely"));return true;}
			auto* Camera=Cast<AFlickCameraPawn>(Controller->GetViewTarget());
			if(!Camera) {Fail(TEXT("Free camera missing after seek"));return true;}
			const auto Rig=FlickArenaLighting::FindRig(World);
			const auto* Lens=Camera->FindComponentByClass<UCameraComponent>();
			if(!Rig.PuckFinishLight||!Rig.PuckFillLight||!Rig.PuckFinishLight->GetActorTransform().Equals(KeyBeforeFree,.001f)
				||!Rig.PuckFillLight->GetActorTransform().Equals(FillBeforeFree,.001f)
				||!FMath::IsNearlyEqual(Lens->PostProcessSettings.BloomIntensity,BloomBeforeFree)
				||!FMath::IsNearlyEqual(Lens->PostProcessSettings.AutoExposureBias,ExposureBeforeFree))
				{Fail(TEXT("Free camera changed the arena lights or exposure"));return true;}
			const auto Entry=Camera->GetActorTransform();Camera->AddFreeCameraInput(0,0,0,FVector2D::ZeroVector,false,.1f);
			if(!Camera->GetActorTransform().Equals(Entry,.01f)) {Fail(TEXT("Taking over free camera snapped the view"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("after-free.png"),true,false);Advance(71);
		}
		else if(Stage==71&&Controller&&State&&FPlatformTime::Seconds()-StageTime>.3)
		{
			auto* Camera=Cast<AFlickCameraPawn>(Controller->GetViewTarget());
			const auto Before=Camera->GetActorLocation(); Camera->AddFreeCameraInput(1,0,0,FVector2D(4,0),false,.1f);
			if(Camera->GetActorLocation().Equals(Before,1)) {Fail(TEXT("Free camera could not move while paused"));return true;}
			Camera->AddFreeCameraInput(0,0,-1,FVector2D::ZeroVector,true,10.f);
			if(Camera->GetActorLocation().Z<State->ArenaSurfaceZ+49.f) {Fail(TEXT("Free camera went below the arena deck"));return true;}
			Controller->ResetView();Advance(72);
		}
		else if(Stage==72&&Controller&&FPlatformTime::Seconds()-StageTime>.3)
		{
			auto* Camera=Cast<AFlickCameraPawn>(Controller->GetViewTarget());
			const auto Before=Camera->GetActorTransform();Controller->SelectCamera(EFlickReplayCamera::Free);
			Camera->AddFreeCameraInput(0,0,0,FVector2D::ZeroVector,false,.1f);
			if(!Camera->GetActorTransform().Equals(Before,.01f)) {Fail(TEXT("Overview-to-free camera snapped at movement bounds"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("free-camera.png"),true,false); Advance(8);
		}
		else if (Stage==8 && FPlatformTime::Seconds()-StageTime>.5)
		{R.ExitTheatre();Advance(9);return true;}
		else if (Stage==9 && !R.IsTheatre() && FPlatformTime::Seconds()-StageTime>2)
		{
			if(R.List().IsEmpty()) {Fail(TEXT("Replay did not survive returning to frontend"));return true;}
			FScreenshotRequest::RequestScreenshot(Output/TEXT("library.png"),true,false); Advance(10);
		}
		else if (Stage==10 && FPlatformTime::Seconds()-StageTime>1)
		{
			FFileHelper::SaveStringToFile(TEXT("PASS: record, save, load, matching puck count, pause, forward/backward seek, stable positions, free camera while paused, profile isolation, library persistence.\n")+Id,*(Output/TEXT("result.txt")));
			const auto Stats=[](TArray<double> Samples) {if(Samples.IsEmpty()) return FString(TEXT("No samples"));Samples.Sort();double Sum=0;for(double V:Samples) Sum+=V;return FString::Printf(TEXT("%d frames, mean %.2f ms, p95 %.2f ms"),Samples.Num(),1000*Sum/Samples.Num(),1000*Samples[FMath::Clamp(int32(Samples.Num()*.95),0,Samples.Num()-1)]);};
			FFileHelper::SaveStringToFile(TEXT("Live: ")+Stats(LiveFrames)+TEXT("\nReplay after loading: ")+Stats(ReplayFrames),*(Output/TEXT("performance.txt")));
			UE_LOG(LogTemp,Display,TEXT("REPLAY_SMOKE PASS")); Stage=99; FPlatformMisc::RequestExit(false);return true;
		}
		return false;
	}
}
#endif
