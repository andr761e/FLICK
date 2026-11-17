#include "Player/FlickReplayController.h"
#include "Player/FlickReplayViewExtension.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPlayerState.h"
#include "Game/FlickReplaySubsystem.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickLightingSettings.h"
#include "Core/FlickRenderStability.h"
#include "Arena/FlickArenaLighting.h"
#include "UI/FlickReplayTheatre.h"
#include "UI/FlickLockerStyle.h"
#include "Engine/Engine.h"
#include "Engine/RectLight.h"
#include "Engine/Light.h"
#include "Engine/SkyLight.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Misc/App.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/KismetMathLibrary.h"
#include "Framework/Application/SlateApplication.h"

AFlickReplayController::AFlickReplayController()
{
	// Present this frame's replay puck poses before framing its camera.
	PrimaryActorTick.TickGroup=TG_PostPhysics;
	bShouldPerformFullTickWhenPaused=true;
	bAutoManageActiveCameraTarget=false;
}
UFlickReplaySubsystem* AFlickReplayController::Replay() const { return GetGameInstance()?GetGameInstance()->GetSubsystem<UFlickReplaySubsystem>():nullptr; }
void AFlickReplayController::BeginPlay()
{
	Super::BeginPlay();
	if (!GetWorld()->IsPlayingReplay()||!IsLocalController()) return;
	if (!PlayerState) { PlayerState=GetWorld()->SpawnActor<APlayerState>(); if(PlayerState) PlayerState->SetOwner(this); }
	bShowMouseCursor=true;
	SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
	if (GEngine&&GEngine->GameViewport) { TheatreUI=MakeFlickReplayTheatre(this); GEngine->GameViewport->AddViewportWidgetContent(TheatreUI.ToSharedRef(),50); }
}
void AFlickReplayController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (TheatreUI.IsValid()&&GEngine&&GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(TheatreUI.ToSharedRef());
	TheatreUI.Reset();PauseViewExtension.Reset();Super::EndPlay(Reason);
}
void AFlickReplayController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::SpaceBar,IE_Pressed,this,&AFlickReplayController::TogglePause).bExecuteWhenPaused=true;
	InputComponent->BindKey(EKeys::F,IE_Pressed,this,&AFlickReplayController::ToggleControls).bExecuteWhenPaused=true;
	InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&AFlickReplayController::Exit).bExecuteWhenPaused=true;
	InputComponent->BindKey(EKeys::RightMouseButton,IE_Pressed,this,&AFlickReplayController::LookPressed).bExecuteWhenPaused=true;
	InputComponent->BindKey(EKeys::RightMouseButton,IE_Released,this,&AFlickReplayController::LookReleased).bExecuteWhenPaused=true;
	InputComponent->BindKey(EKeys::MouseScrollUp,IE_Pressed,this,&AFlickReplayController::WheelUp).bExecuteWhenPaused=true;
	InputComponent->BindKey(EKeys::MouseScrollDown,IE_Pressed,this,&AFlickReplayController::WheelDown).bExecuteWhenPaused=true;
}
void AFlickReplayController::TogglePause() { if(auto* R=Replay()) R->SetPaused(!R->IsPaused()); }
void AFlickReplayController::ToggleControls() { bControls=!bControls; }
void AFlickReplayController::Exit() { if(bLooking||bFlyLatched) {bFlyLatched=false;LookReleased();return;} if(auto* R=Replay()) {if(R->IsEditingClip()) R->CloseClip();else R->ExitTheatre();} }
void AFlickReplayController::ResetView() {OverviewZoom=1;Orbit=0;SelectCamera(EFlickReplayCamera::Overview);}
void AFlickReplayController::ToggleFly() {if(bFlyLatched) {bFlyLatched=false;LookReleased();}else {SelectCamera(EFlickReplayCamera::Free);bFlyLatched=true;LookPressed();}}
void AFlickReplayController::LookPressed() { bLooking=true; bShowMouseCursor=false; SetInputMode(FInputModeGameOnly()); }
void AFlickReplayController::LookReleased() { if(bFlyLatched) return; bLooking=false; bShowMouseCursor=true; SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false)); }
void AFlickReplayController::WheelUp() { OverviewZoom=FMath::Clamp(OverviewZoom*.9f,.45f,1.8f); }
void AFlickReplayController::WheelDown() { OverviewZoom=FMath::Clamp(OverviewZoom/ .9f,.45f,1.8f); }
void AFlickReplayController::SelectCamera(EFlickReplayCamera Value)
{
	bFlyLatched=false;LookReleased();
	bSnapNextCamera=true;
	CameraMode=Value;
	// Restore the complete local presentation when taking over from a recorded
	// target. Never inherit a stale rig/camera history after a replay transition.
	if(!FlickRenderStability::IsEnabled()) LightingRevision=INDEX_NONE;
	bViewLightingReady=false;
	if(PlayerCameraManager) PlayerCameraManager->SetGameCameraCutThisFrame();
	if (TheatreCamera) { TheatreCamera->ClearSpectatorView(); TheatreCamera->SetFreeCameraEnabled(Value==EFlickReplayCamera::Free); }
}

int32 AFlickReplayController::PlayerCount() const
{
	const auto* State=GetWorld()->GetGameState<AFlickGameState>();
	return State?FMath::Max(1,State->PlayersPerTeam*2):0;
}
void AFlickReplayController::SelectPlayer(int32 Seat)
{
	Target=FMath::Clamp(Seat,0,FMath::Max(0,PlayerCount()-1));
	SelectCamera(EFlickReplayCamera::Player);
}
FString AFlickReplayController::PlayerLabel(int32 Seat) const
{
	const auto* State=GetWorld()->GetGameState<AFlickGameState>();
	if(!State) return TEXT("Loading players...");
	const int32 Size=FMath::Max(1,State->PlayersPerTeam);
	const auto Team=Seat<Size?EFlickTeam::Player1:EFlickTeam::Player2;
	const FString Prefix=Team==EFlickTeam::Player1?TEXT("BLUE"):TEXT("ORANGE");
	for(const APlayerState* Base:State->PlayerArray)
		if(const auto* P=Cast<AFlickPlayerState>(Base);P&&P->GetTeam()==Team&&P->GetTeamPlayerSlot()==Seat%Size)
			return Prefix+TEXT(" / ")+P->GetPlayerName()+(P->GetPrivateCameraView().bValid?TEXT(""):TEXT(" (arena view)"));
	return FString::Printf(TEXT("%s / PLAYER %d (arena view)"),*Prefix,Seat%Size+1);
}

FString AFlickReplayController::TargetLabel() const
{
	const auto* State=GetWorld()->GetGameState<AFlickGameState>();
	if (!State) return TEXT("Loading arena");
	if (CameraMode==EFlickReplayCamera::Player)
	{
		return PlayerLabel(Target%FMath::Max(1,PlayerCount()));
	}
	if (CameraMode==EFlickReplayCamera::Puck||CameraMode==EFlickReplayCamera::Cinematic) return FString::Printf(TEXT("Puck target %d"),Target+1);
	return CameraMode==EFlickReplayCamera::Free?TEXT("Free camera — hold RMB to fly"):TEXT("Arena overview — RMB orbit / wheel zoom");
}

void AFlickReplayController::CycleTarget(int32 Direction)
{
	if(CameraMode==EFlickReplayCamera::Player||CameraMode==EFlickReplayCamera::Free)
	{
		// Free camera is a real target in the player carousel, sharing the same
		// camera actor, exposure and arena rig as the player being followed.
		const int32 Players=FMath::Max(1,PlayerCount());
		const int32 Seat=CameraMode==EFlickReplayCamera::Free?Players:Target%Players;
		const int32 Next=(Seat+Direction+Players+1)%(Players+1);
		if(Next==Players) SelectCamera(EFlickReplayCamera::Free);
		else SelectPlayer(Next);
		return;
	}
	int32 Count=1;
	if (CameraMode==EFlickReplayCamera::Player)
	{
		if (const auto* State=GetWorld()->GetGameState<AFlickGameState>()) Count=FMath::Max(1,State->PlayersPerTeam*2);
	}
	else
	{
		Count=0; for(TActorIterator<AFlickPiece> It(GetWorld());It;++It) if(!It->IsHidden()) ++Count;
		Count=FMath::Max(1,Count);
	}
	Target=(Target+Direction+Count)%Count;
	bSnapNextCamera=true;
}

void AFlickReplayController::UpdateViewLighting()
{
	if(!TheatreCamera || !ViewPuckKey.IsValid() || !ViewPuckFill.IsValid()) return;
#if !UE_BUILD_SHIPPING
	// Controlled screenshot reference for the previous fixed-view replay rig.
	static const bool bFixedReference=FParse::Param(FCommandLine::Get(),TEXT("FlickReplayFixedLightingReference"));
	if(bFixedReference) return;
#endif
	const FVector Eye=TheatreCamera->GetActorLocation();
	if(bViewLightingReady && Eye.Equals(LastLightingEye,.01f)) return;
	const FVector Centre(0,0,LightingSurfaceZ);
	// The authored deck rig faces the default south-side gameplay view.
	// Rotate that same rig around the board for the actual eye position. Keep
	// all intensities, colours, reflection strength and local slider values.
	const FVector2D Planar(Eye.X,Eye.Y);
	if(!Planar.IsNearlyZero())
	{
		const float Azimuth=FMath::RadiansToDegrees(FMath::Atan2(Eye.Y,Eye.X));
		const FQuat Turn=FRotator(0,Azimuth+90.f,0).Quaternion();
		for(int32 I=0;I<ViewDeckLights.Num();++I)
			if(auto* Light=ViewDeckLights[I].Get(); Light && DeckReferenceTransforms.IsValidIndex(I))
			{
				const auto& Base=DeckReferenceTransforms[I];
				Light->SetActorLocationAndRotation(Centre+Turn.RotateVector(Base.GetLocation()-Centre),Turn*Base.GetRotation());
			}
	}
	FVector ReferenceLocation; FRotator ReferenceRotation;
	FlickLockerStyle::PuckView(250.f,1.f,16.f/9.f,ReferenceLocation,ReferenceRotation);
	ReferenceRotation=(FVector(0,0,250.f+FlickLockerStyle::PuckFocusHeight)-ReferenceLocation).Rotation();
	const float ReferenceAzimuth=FMath::RadiansToDegrees(FMath::Atan2(ReferenceLocation.Y,ReferenceLocation.X));
	// Use the ray to the board in every mode. Looking around in free camera
	// must not swing the lighting below the surface or change its exposure.
	FRotator View=(Centre+FVector(0,0,20)-Eye).Rotation();
	View.Pitch=FMath::Clamp(View.Pitch,-80.f,-15.f);
	const FQuat Frame=FlickArenaLighting::MakePuckStudioFrame(View,ReferenceRotation,ReferenceAzimuth,12.f,true);
	const auto* State=GetWorld()->GetGameState<AFlickGameState>();
	const float Scale=LightingArenaRadius/(State && State->ActiveMatchVariant==EFlickMatchVariant::Bob?620.f:650.f);
	FlickArenaLighting::UpdatePuckStudioLights(ViewPuckKey.Get(),ViewPuckFill.Get(),Eye,Scale,LightingSurfaceZ,&Frame,true);
	for(auto* Light:{ViewPuckKey.Get(),ViewPuckFill.Get()})
	{
		FVector Position=Light->GetActorLocation();
		Position.Z=FMath::Max(Position.Z,LightingSurfaceZ+200.f*Scale);
		Light->SetActorLocationAndRotation(Position,(Centre-Position).Rotation());
	}
	LastLightingEye=Eye;bViewLightingReady=true;
}

bool AFlickReplayController::WantsPausedSampling() const
{
	const auto* R=Replay();
	return R&&R->IsPaused()&&!R->IsSeeking()&&FlickRenderStability::PauseStableSampling()&&SettledPausedFrames>=32;
}
void AFlickReplayController::UpdatePausedRendering()
{
	const auto* R=Replay();if(!R||!TheatreCamera) return;
	const auto View=TheatreCamera->CaptureSpectatorView();
	const FTransform Transform(View.Rotation,View.Location);
	const bool bViewChanged=!Transform.Equals(LastPausedView,.001f)||!FMath::IsNearlyEqual(View.FieldOfView,LastPausedFov,.001f);
	if(!R->IsPaused()||R->IsSeeking()||bLooking||bFlyLatched||bViewChanged||LastPausedRevision!=R->GetPlaybackRevision()) SettledPausedFrames=0;
	else SettledPausedFrames=FMath::Min(SettledPausedFrames+1,32);
	LastPausedView=Transform;LastPausedFov=View.FieldOfView;LastPausedRevision=R->GetPlaybackRevision();
	// Keep temporal rendering active during movement and convergence. Once
	// settled, use the engine's normal paused history/sample-index behaviour.
	// UI and camera input still tick; no cached screenshot replaces the view.
}

void AFlickReplayController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!GetWorld()->IsPlayingReplay()||!IsLocalController()) return;
	GetWorld()->bIsCameraMoveableWhenPaused=true;
	if(!PauseViewExtension.IsValid()) PauseViewExtension=FSceneViewExtensions::NewExtension<FFlickReplayViewExtension>(this);
	if (!TheatreUI.IsValid()&&GEngine&&GEngine->GameViewport)
	{
		bShowMouseCursor=true; SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
		TheatreUI=MakeFlickReplayTheatre(this); GEngine->GameViewport->AddViewportWidgetContent(TheatreUI.ToSharedRef(),50);
	}
	if (!IsValid(TheatreCamera))
	{
		TheatreCamera=GetWorld()->SpawnActor<AFlickCameraPawn>();
		if (!IsValid(TheatreCamera)) return;
		TheatreCamera->SetMenuPresentation(false); TheatreCamera->SetMenuOrbitEnabled(false);
		TheatreCamera->SetReplayLightingLocked(true);
		TheatreCamera->SetOwner(this);
		TheatreCamera->PrimaryActorTick.bTickEvenWhenPaused=true;
		TheatreCamera->SetActorLocation(FVector(0,-1400,1100));
		TheatreCamera->SetActorRotation(FRotator(-40,90,0));
		LightingRevision=INDEX_NONE;
		if(auto* Lens=TheatreCamera->FindComponentByClass<UCameraComponent>()) {Lens->PostProcessSettings.bOverride_MotionBlurAmount=true;Lens->PostProcessSettings.MotionBlurAmount=0;}
		SelectCamera(CameraMode);
	}
	SetViewTarget(TheatreCamera);
	auto* R=Replay(); const auto* State=GetWorld()->GetGameState<AFlickGameState>();
	if (!R||!State) return;
	if (!R->IsSeeking())
	{
		const int32 Context=static_cast<int32>(State->ActiveMatchVariant)|(State->PlayersPerTeam<<4);
		const bool bStable=FlickRenderStability::IsEnabled();
		const bool bDiscontinuousTime=FMath::Abs(R->GetTime()-LastReplayTime)>1;
		const bool bPlaybackChanged=LightingRevision!=R->GetPlaybackRevision();
		const bool bCheckRig=FPlatformTime::Seconds()>=NextRigCheck;
		const bool bResolveRig=bCheckRig||bPlaybackChanged||!ViewPuckKey.IsValid()||!ViewPuckFill.IsValid();
		FlickArenaLighting::FRig Rig;
		if(bResolveRig) {Rig=FlickArenaLighting::FindRig(GetWorld());NextRigCheck=FPlatformTime::Seconds()+.5;}
		uint32 SettingsHash=LightingSettingsHash;
		if(bCheckRig)
		{
			SettingsHash=0;
			for(int32 I=0;I<int32(FlickLightingSettings::EControl::Count);++I)
				SettingsHash=HashCombineFast(SettingsHash,GetTypeHash(FlickLightingSettings::GetValue(FlickLightingSettings::EScene::Gameplay,FlickLightingSettings::EControl(I))));
		}
		const float Radius=FlickModeRules::GetArenaRadius(State->ActiveMatchVariant,State->PlayersPerTeam)
			*(State->bPrivateMatchActive&&State->ActiveMatchVariant==EFlickMatchVariant::Classic?FMath::Clamp(State->PrivateMatchSettings.ArenaScale,.85f,1.3f):1.f);
		bool bRigChanged=bResolveRig&&(Rig.PuckFinishLight.Get()!=ViewPuckKey.Get()||Rig.PuckFillLight.Get()!=ViewPuckFill.Get());
		if(bResolveRig&&ViewDeckLights.Num()==3)
			bRigChanged|=ViewDeckLights[0].Get()!=Rig.TestPuckKeyLight.Get()||ViewDeckLights[1].Get()!=Rig.TestPuckRimLight.Get()||ViewDeckLights[2].Get()!=Rig.DeckReflectionLight.Get();
		const bool bMissingRig=!ViewPuckKey.IsValid()||!ViewPuckFill.IsValid()
			||(bResolveRig&&(!IsValid(Rig.PuckFinishLight)||!IsValid(Rig.PuckFillLight)));
		const bool bPresentationChanged=SettingsHash!=LightingSettingsHash||FMath::Abs(LightingSurfaceZ-State->ArenaSurfaceZ)>.1f
			||LightingContext!=Context||FMath::Abs(LightingArenaRadius-Radius)>.1f;
		if(bMissingRig||bRigChanged||bPresentationChanged||LightingRevision==INDEX_NONE||(!bStable&&(bPlaybackChanged||bDiscontinuousTime)))
		{
			if(!bResolveRig) Rig=FlickArenaLighting::FindRig(GetWorld());
			bSnapNextCamera=true;SettledPausedFrames=0;
			FlickArenaLighting::FParameters Parameters;
			Parameters.bClassicArenaLighting=State->ActiveMatchVariant==EFlickMatchVariant::Classic;
			Parameters.bBobArenaLighting=State->ActiveMatchVariant==EFlickMatchVariant::Bob;
			Parameters.bTestArenaMode=FlickModeRules::Get(State->ActiveMatchVariant).bUseSwitchyardArena;
			Parameters.bPremiumArena=Parameters.bTestArenaMode;
			Parameters.ArenaRadius=FlickModeRules::GetArenaRadius(State->ActiveMatchVariant,State->PlayersPerTeam);
			if(State->bPrivateMatchActive && Parameters.bClassicArenaLighting)
				Parameters.ArenaRadius*=FMath::Clamp(State->PrivateMatchSettings.ArenaScale,.85f,1.3f);
			Parameters.ArenaSurfaceZ=State->ArenaSurfaceZ;
			FlickArenaLighting::Configure(GetWorld(),Rig,Parameters);
			// These are viewer-local fixtures, not recorded match actors. Native
			// checkpoint rewinds otherwise delete them and invalidate Lumen's
			// lighting caches on every scrub. Preserve only our non-networked rig.
			for(TActorIterator<ALight> It(GetWorld());It;++It)
				if(!It->GetIsReplicated()) for(const FName Tag:It->Tags)
					if(Tag.ToString().StartsWith(TEXT("FLICK."))) {It->bReplayRewindable=bStable;break;}
			if(Rig.SkyLightActor&&!Rig.SkyLightActor->GetIsReplicated()) Rig.SkyLightActor->bReplayRewindable=bStable;
			// Cache an authored south-side reference, rather than keeping its
			// highlights frozen on one side when the replay viewer changes sides.
			if(Parameters.bTestArenaMode)
				FlickArenaLighting::UpdateMenuSoftboxes(Rig.TestPuckKeyLight,Rig.TestPuckRimLight,FVector(0,-1400,1100),Parameters.ArenaRadius,Parameters.ArenaSurfaceZ,Rig.DeckReflectionLight);
			ViewDeckLights={Rig.TestPuckKeyLight.Get(),Rig.TestPuckRimLight.Get(),Rig.DeckReflectionLight.Get()};
			DeckReferenceTransforms.Reset();
			for(const auto& Light:ViewDeckLights) DeckReferenceTransforms.Add(Light.IsValid()?Light->GetActorTransform():FTransform::Identity);
			ViewPuckKey=Rig.PuckFinishLight;ViewPuckFill=Rig.PuckFillLight;
			LightingArenaRadius=Parameters.ArenaRadius;bViewLightingReady=false;
			LightingSettingsHash=SettingsHash;
			FVector ReferenceLocation; FRotator ReferenceRotation;
			FlickLockerStyle::PuckView(250.f,1.f,16.f/9.f,ReferenceLocation,ReferenceRotation);
			ReferenceRotation=(FVector(0,0,250.f+FlickLockerStyle::PuckFocusHeight)-ReferenceLocation).Rotation();
			const float ReferenceAzimuth=FMath::RadiansToDegrees(FMath::Atan2(ReferenceLocation.Y,ReferenceLocation.X));
			const FQuat Frame=FlickArenaLighting::MakePuckStudioFrame(FRotator(-45,90,0),ReferenceRotation,ReferenceAzimuth,12.f,true);
			FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight,Rig.PuckFillLight,FVector(0,-1400,1100),
				Parameters.ArenaRadius/(Parameters.bBobArenaLighting?620.f:650.f),Parameters.ArenaSurfaceZ,&Frame,true);
			TheatreCamera->SetBobGameplayFraming(Parameters.bBobArenaLighting);
			TheatreCamera->SetTestArenaPresentation(Parameters.bTestArenaMode);
			TheatreCamera->SetOneVsOneArenaPresentation(Parameters.bTestArenaMode&&State->PlayersPerTeam==1);
			LightingSurfaceZ=State->ArenaSurfaceZ;
			if(PlayerCameraManager) PlayerCameraManager->SetGameCameraCutThisFrame();
			LightingContext=Context; LightingRevision=R->GetPlaybackRevision();
		}
		if(bPlaybackChanged)
		{
			// A seek may replace actors, but unchanged lights/camera do not need
			// another reset. Large time jumps still invalidate temporal history.
			bSnapNextCamera=true;
			if(bDiscontinuousTime&&PlayerCameraManager) PlayerCameraManager->SetGameCameraCutThisFrame();
			LightingRevision=R->GetPlaybackRevision();
		}
		LastReplayTime=R->GetTime();
		if(bCheckRig)
			for(TActorIterator<AFlickCameraPawn> It(GetWorld());It;++It)
				if(*It!=TheatreCamera) It->SetActorTickEnabled(false);

	}
	const float RealDelta=FMath::Clamp(static_cast<float>(FApp::GetDeltaTime()),0.f,.1f);
	// Camera response follows wall time, even when replay playback is slowed down.
	// Apply one interpolation pass here rather than the pawn's game-time smoothing.
	const auto ApplyView=[&](const FFlickSpectatorView& TargetView)
	{
		FFlickSpectatorView View=TargetView;
		if(bSnapNextCamera&&PlayerCameraManager
			&&(FVector::DistSquared(TheatreCamera->GetActorLocation(),View.Location)>FMath::Square(250.f)
				||TheatreCamera->GetActorQuat().AngularDistance(View.Rotation.Quaternion())>FMath::DegreesToRadians(20.f)))
			PlayerCameraManager->SetGameCameraCutThisFrame();
		if(!bSnapNextCamera&&!R->IsPaused())
		{
			const float Blend=1.f-FMath::Exp(-12.f*RealDelta);
			View.Location=FMath::Lerp(TheatreCamera->GetActorLocation(),View.Location,Blend);
			View.Rotation=FQuat::Slerp(TheatreCamera->GetActorQuat(),View.Rotation.Quaternion(),Blend).Rotator();
			View.FieldOfView=FMath::Lerp(TheatreCamera->CaptureSpectatorView().FieldOfView,View.FieldOfView,Blend);
		}
		TheatreCamera->SnapSpectatorView(View);bSnapNextCamera=false;
		UpdateViewLighting();
		UpdatePausedRendering();
	};
	const auto Focused=FSlateApplication::Get().GetKeyboardFocusedWidget();
	const bool bTyping=Focused.IsValid()&&Focused->GetTypeAsString().Contains(TEXT("EditableText"));
	if(!bTyping)
	{
	if (WasInputKeyJustPressed(EKeys::Right)) R->Seek(R->GetTime()+5);
	if (WasInputKeyJustPressed(EKeys::Left)) R->Seek(R->GetTime()-5);
	if (WasInputKeyJustPressed(EKeys::Period)) { R->SetPaused(true); R->Seek(R->GetTime()+1.f/30); }
	if (WasInputKeyJustPressed(EKeys::Comma)) { R->SetPaused(true); R->Seek(R->GetTime()-1.f/30); }
	if (WasInputKeyJustPressed(EKeys::Home)) R->Seek(R->GetViewStart());
	if (WasInputKeyJustPressed(EKeys::End)) R->Seek(R->GetViewEnd());
	if (WasInputKeyJustPressed(EKeys::C)) SelectCamera(static_cast<EFlickReplayCamera>((static_cast<int32>(CameraMode)+1)%5));
	if (WasInputKeyJustPressed(EKeys::LeftBracket)) CycleTarget(-1);
	if (WasInputKeyJustPressed(EKeys::RightBracket)) CycleTarget(1);
	}
	float MX=0,MY=0; GetInputMouseDelta(MX,MY);
	if (CameraMode==EFlickReplayCamera::Free)
	{
		if (bLooking) TheatreCamera->AddFreeCameraInput((IsInputKeyDown(EKeys::W)?1.f:0)-(IsInputKeyDown(EKeys::S)?1.f:0),
			(IsInputKeyDown(EKeys::D)?1.f:0)-(IsInputKeyDown(EKeys::A)?1.f:0),(IsInputKeyDown(EKeys::E)?1.f:0)-(IsInputKeyDown(EKeys::Q)?1.f:0),
			FVector2D(MX,MY),IsInputKeyDown(EKeys::LeftShift),RealDelta);
		UpdateViewLighting();
		UpdatePausedRendering();
		return;
	}
	TheatreCamera->SetFreeCameraEnabled(false);
	if (bLooking) Orbit+=MX*.25f;
	const float Radius=FlickModeRules::GetArenaRadius(State->ActiveMatchVariant,State->PlayersPerTeam)
		*(State->bPrivateMatchActive && State->ActiveMatchVariant==EFlickMatchVariant::Classic
			? FMath::Clamp(State->PrivateMatchSettings.ArenaScale,.85f,1.3f):1.f);
	FFlickSpectatorView View; View.bValid=true; View.FieldOfView=45;
	FVector Focus(0,0,State->ArenaSurfaceZ+20);
	FVector Offset(0,-Radius*3.25f,Radius*2.9f);
	if (CameraMode==EFlickReplayCamera::Player)
	{
		const int32 Seat=Target%FMath::Max(1,State->PlayersPerTeam*2);
		const auto Team=Seat<State->PlayersPerTeam?EFlickTeam::Player1:EFlickTeam::Player2;
		for (const APlayerState* Base:State->PlayerArray)
			if(const auto* P=Cast<AFlickPlayerState>(Base);P&&P->GetTeam()==Team&&P->GetTeamPlayerSlot()==Seat%State->PlayersPerTeam&&P->GetPrivateCameraView().bValid)
			{
				ApplyView(P->GetPrivateCameraView()); return;
			}
		if (Team==EFlickTeam::Player2) Offset.Y*=-1;
	}
	if (CameraMode==EFlickReplayCamera::Puck||CameraMode==EFlickReplayCamera::Cinematic)
	{
		TArray<AFlickPiece*, TInlineAllocator<32>> Pieces;
		for(TActorIterator<AFlickPiece> It(GetWorld());It;++It) if(!It->IsHidden()) Pieces.Add(*It);
		Pieces.Sort([](const AFlickPiece& A,const AFlickPiece& B) { return A.GetPieceId()<B.GetPieceId(); });
		if (!Pieces.IsEmpty()) Focus=Pieces[Target%Pieces.Num()]->GetActorLocation();
		Offset=FVector(0,-520,310); View.FieldOfView=35;
		if (CameraMode==EFlickReplayCamera::Cinematic) Offset=Offset.RotateAngleAxis(R->GetTime()*5.f,FVector::UpVector);
	}
	View.Location=Focus+Offset.RotateAngleAxis(Orbit,FVector::UpVector)*OverviewZoom;
	View.Rotation=UKismetMathLibrary::FindLookAtRotation(View.Location,Focus);
	ApplyView(View);
}
