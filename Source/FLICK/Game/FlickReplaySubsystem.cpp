#include "Game/FlickReplaySubsystem.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Engine/DemoNetDriver.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "NetworkReplayStreaming.h"
#include "Player/FlickPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "ReplaySubsystem.h"
#include "Engine/StreamableManager.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "Online/FlickSessionSubsystem.h"
#include "Online/FlickPartySubsystem.h"
#include "Game/FlickGameInstance.h"
#if !UE_BUILD_SHIPPING
#include "Tests/FlickReplaySmoke.h"
#endif

namespace
{
	const TArray<FString> ReplayOptions={TEXT("ReplayStreamerOverride=LocalFileNetworkReplayStreaming")};
	FString MatchFile(const FString& Id) { return UFlickReplaySubsystem::Root()/TEXT("Matches")/(Id+TEXT(".flick.replay")); }
}

void UFlickReplaySubsystem::StartCapture(AFlickGameState& State)
{
	if (FinalizingRecording.IsValid()) return;
	if (GetWorld()->GetDemoNetDriver()) return;
	if (!TemporaryFile.IsEmpty()) IFileManager::Get().Delete(*TemporaryFile,false,true);
	Current=FFlickSavedReplay(); Current.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);
	Current.RecordedAt=FDateTime::UtcNow().ToIso8601(); Current.GameVersion=FApp::GetBuildVersion();
	Current.Variant=State.ActiveMatchVariant; Current.TeamSize=State.PlayersPerTeam;
	if(const auto* Rate=IConsoleManager::Get().FindConsoleVariable(TEXT("demo.RecordHz"))) Current.PoseSampleRate=Rate->GetFloat();
	UE_LOG(LogTemp,Display,TEXT("FLICK replay capture: %.1f Hz"),Current.PoseSampleRate);
	// Store asset references, never transient per-puck material instances.
	TSet<UObject*> SeenAssets;
	const auto RememberAsset=[this, &SeenAssets](UObject* Asset)
	{
		if (!Asset || !Asset->IsAsset()) return;
		bool bAlreadySeen = false;
		SeenAssets.Add(Asset, &bAlreadySeen);
		if (!bAlreadySeen) Current.Assets.Emplace(Asset);
	};
	for(TActorIterator<AActor> It(GetWorld());It;++It)
	{
		for(const auto* Mesh:TInlineComponentArray<UStaticMeshComponent*>(*It))
		{
			RememberAsset(Mesh->GetStaticMesh());
			for(int32 I=0;I<Mesh->GetNumMaterials();++I)
			{
				UMaterialInterface* Material=Mesh->GetMaterial(I);
				while(Material&&!Material->IsAsset())
				{
					const auto* Instance=Cast<UMaterialInstance>(Material);
					Material=Instance?Instance->Parent.Get():nullptr;
				}
				RememberAsset(Material);
			}
		}
	}
	Current.Mode=State.bPrivateMatchActive ? TEXT("Private") : State.bRankedMatch ? TEXT("Competitive") : TEXT("Casual");
	for(int32 Seat=0;Seat<State.PlayersPerTeam*2;++Seat)
		Current.Players.Add(FString::Printf(TEXT("%s player %d"),Seat<State.PlayersPerTeam?TEXT("Blue"):TEXT("Orange"),Seat%State.PlayersPerTeam+1));
	Current.Name=FString::Printf(TEXT("%s %dv%d — %s"),Current.Variant==EFlickMatchVariant::Bob ? TEXT("BOB") : TEXT("Knockout"),Current.TeamSize,Current.TeamSize,*FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M")));
	TemporaryFile=Root()/TEXT("Temp")/(Current.Id+TEXT(".flick.replay"));
	IFileManager::Get().MakeDirectory(*(Root()/TEXT("Temp")),true);
	CaptureMatchId=State.MatchId; CaptureWorld=GetWorld(); bCurrentSaved=false; EndDelay=LastCheckpoint=0;
	LastRound=LastTurn=LastSwitches=0; LastCount=-1;
	GetGameInstance()->StartRecordingReplay(TemporaryFile,Current.Name,ReplayOptions);
	bCapturing=GetWorld()->GetDemoNetDriver() && GetWorld()->GetDemoNetDriver()->IsRecording();
	Status=bCapturing ? TEXT("Recording match") : TEXT("Replay recording could not start");
	if (bCapturing) if (auto* R=GetGameInstance()->GetSubsystem<UReplaySubsystem>()) R->RequestCheckpoint();
}

void UFlickReplaySubsystem::AddMarker(const FString& Label)
{
	const auto* Driver=GetWorld()->GetDemoNetDriver();
	if (Driver && Current.Markers.Num()<20000) Current.Markers.Add({Driver->GetDemoCurrentTime(),Label});
}

void UFlickReplaySubsystem::FinishCapture()
{
	if (!bCapturing) return;
	if (GetWorld() && CaptureWorld.Get()==GetWorld())
	{
		if (auto* Driver=GetWorld()->GetDemoNetDriver()) { Driver->PauseRecording(false); Current.Duration=Driver->GetDemoCurrentTime(); FinalizingRecording=Driver->GetReplayStreamer(); }
		GetGameInstance()->StopRecordingReplay();
		// The local streamer drains its queued writes on subsequent ticks. A global
		// Flush here blocks the game thread during the winner-camera transition.
	}
	bCapturing=false; CaptureWorld.Reset();
	Status=FinalizingRecording.IsValid() ? TEXT("Finishing replay recording") : TEXT("Match ready to save");
}

void UFlickReplaySubsystem::Tick(float DeltaTime)
{
	UWorld* World=GetWorld();
	if (!World || !World->IsGameWorld() || IsRunningCommandlet()) return;
	if (FinalizingRecording.IsValid()
		&& FinalizingRecording->GetReplayStreamerState()==EReplayStreamerState::Idle)
	{
		const bool bFailed=FinalizingRecording->HasError();
		FinalizingRecording.Reset();
		if (bFailed) Current.Duration=0;
		Status=bFailed ? TEXT("Replay recording could not finish") : TEXT("Match ready to save");
	}
#if !UE_BUILD_SHIPPING
	if (FlickReplaySmoke::Tick(*this,DeltaTime)) return;
#endif
	if(bPreloading || bWarmingTheatre) TickLoading();
	if (IsTheatre())
	{
		if(!IsLoadingTheatre()&&!bPaused&&!bSeeking&&PendingSeek<0&&!bScrubbing)
		{
			const float End=bPreviewingClip?GetViewStart()+ClipOut:GetViewEnd();
			if(GetTime()>=End-.002f)
			{
				if(bPreviewingClip) Seek(GetViewStart()+ClipIn);
				else {SetPaused(true);Seek(End);}
			}
		}
		// Local streaming normally throttles chunk reads for several seconds.
		// Request a bounded window ahead before playback reaches the disk edge.
		if(!bSeeking && PendingSeek<0 && FPlatformTime::Seconds()>=NextBufferRequest)
		{
			NextBufferRequest=FPlatformTime::Seconds()+.25;
			if(auto* Driver=World->GetDemoNetDriver()) if(auto Stream=Driver->GetReplayStreamer())
				Stream->SetHighPriorityTimeRange(uint32(GetTime()*1000),uint32(FMath::Min(GetDuration(),GetTime()+15.f)*1000));
		}
		if (PendingSeek>=0 && !bSeeking && (bBoundaryPreview?FPlatformTime::Seconds()-LastBoundarySeek>.12:FPlatformTime::Seconds()-LastSeekRequest>.06))
		{
			if (auto* Driver=World->GetDemoNetDriver())
			{
				const float Target=PendingSeek; ActiveSeek=Target; PendingSeek=-1; bSeeking=true;
				LastBoundarySeek=FPlatformTime::Seconds();
				World->GetWorldSettings()->SetPauserPlayerState(nullptr);
				Driver->GotoTimeInSeconds(Target,FOnGotoTimeDelegate::CreateWeakLambda(this,[this,Target](bool bSuccess)
				{ if(bSuccess&&GetWorld()&&GetWorld()->GetDemoNetDriver()) GetWorld()->GetDemoNetDriver()->SetDemoCurrentTime(Target); ++PlaybackRevision; if(!bSuccess) Status=TEXT("Could not seek to that point"); SetPaused(bPaused); bSeeking=false; }));
			}
		}
		return;
	}
	if (bPlaybackRequested) return;
	if (!PendingWatchId.IsEmpty())
	{
		const auto* Sessions=GetGameInstance()->GetSubsystem<UFlickSessionSubsystem>();
		if(World->GetNetMode()!=NM_Standalone&&Sessions&&!Sessions->HasActiveSession()&&Sessions->GetState()==EFlickSessionState::Idle)
		{UGameplayStatics::OpenLevel(this,FName(TEXT("/Engine/Maps/Entry")));return;}
	}
	if (!PendingWatchId.IsEmpty() && !NeedsSessionExit())
	{
		const FString Id=PendingWatchId; PendingWatchId.Reset(); Watch(Id); return;
	}
	auto* State=World->GetGameState<AFlickGameState>();
	const auto* Mode=World->GetAuthGameMode<AFlickGameMode>();
	const bool bEligible=World->GetNetMode()!=NM_DedicatedServer && State && State->IsGameplayActive() && !State->bNetworkLobbyActive
		&& (!Mode || (!Mode->IsPracticeMode() && !Mode->IsFreePlayTraining() && !Mode->IsTutorialMode()));
	if (bCapturing && (!bEligible || CaptureWorld.Get()!=World)) { FinishCapture(); Status=TEXT("Recording interrupted"); Current.Duration=0; return; }
	if (bCapturing && State && !State->MatchId.IsEmpty() && !CaptureMatchId.IsEmpty() && CaptureMatchId!=State->MatchId) FinishCapture();
	if (!bCapturing && bEligible && !State->bSeriesComplete && (!Mode||Mode->GetFrontendScreen()==EFlickFrontendScreen::Playing)) StartCapture(*State);
	if (!bCapturing || !State) return;
	if (CaptureMatchId.IsEmpty()) CaptureMatchId=State->MatchId;
	for (const APlayerState* Base:State->PlayerArray)
		if(const auto* P=Cast<AFlickPlayerState>(Base);P&&P->GetTeam()!=EFlickTeam::None&&P->GetTeamPlayerSlot()>=0)
		{
			const int32 Seat=(P->GetTeam()==EFlickTeam::Player2?State->PlayersPerTeam:0)+P->GetTeamPlayerSlot();
			if(Current.Players.IsValidIndex(Seat)) Current.Players[Seat]=P->GetPlayerName().Left(100);
		}
	auto* Driver=World->GetDemoNetDriver();
	if (!Driver) { bCapturing=false; Current.Duration=0; Status=TEXT("Replay recording stopped unexpectedly"); return; }
	const bool bHighlight=Mode && Mode->IsCinematicReplayActive();
	Driver->PauseRecording(bHighlight);
	if (bHighlight) return;
	if (Driver->GetDemoCurrentTime()-LastCheckpoint>5)
	{
		if(auto* Replays=GetGameInstance()->GetSubsystem<UReplaySubsystem>();Replays&&!Replays->IsSavingCheckpoint())
		{Replays->RequestCheckpoint();LastCheckpoint=Driver->GetDemoCurrentTime();}
	}
	if (LastRound!=State->RoundNumber) { LastCount=-1; AddMarker(FString::Printf(TEXT("Round %d"),State->RoundNumber)); LastRound=State->RoundNumber; }
	const int32 ShotCount=State->Player1ShotsTaken+State->Player2ShotsTaken;
	if (LastTurn!=ShotCount) { AddMarker(FString::Printf(TEXT("Shot %d"),State->TurnNumber)); LastTurn=ShotCount; }
	const int32 Count=State->Player1ActivePieces+State->Player2ActivePieces;
	if (LastCount>=0 && Count<LastCount && State->MatchPhase==EFlickMatchPhase::ResolvingPhysics) AddMarker(State->ActiveMatchVariant==EFlickMatchVariant::Bob ? TEXT("Pocket") : TEXT("Knockout"));
	LastCount=Count;
	int32 Switches=0; for (const auto& P:State->PlayerMatchStats) Switches+=P.SwitchActivations;
	if (Switches>LastSwitches) AddMarker(TEXT("Switch")); LastSwitches=Switches;
	if (State->bSeriesComplete && State->MatchPhase==EFlickMatchPhase::RoundOver)
	{
		EndDelay+=DeltaTime;
		if (EndDelay>.25f)
		{
			Current.Result=FString::Printf(TEXT("BLUE %d — %d ORANGE"),State->Player1RoundsWon,State->Player2RoundsWon);
			AddMarker(TEXT("Match end")); FinishCapture();
		}
	}
	else EndDelay=0;
}

bool UFlickReplaySubsystem::CanSave() const { return !bCapturing && !FinalizingRecording.IsValid() && !bCurrentSaved && Current.Duration>0 && IFileManager::Get().FileSize(*TemporaryFile)>0; }
bool UFlickReplaySubsystem::SaveCurrent()
{
	if (!CanSave()) return false;
	Current.Name=Current.Name.TrimStartAndEnd();
	if(Current.Name.IsEmpty()) { Status=TEXT("Enter a replay name before saving."); return false; }
	IFileManager::Get().MakeDirectory(*(Root()/TEXT("Matches")),true);
	if (!IFileManager::Get().Move(*MatchFile(Current.Id),*TemporaryFile,false,true)) { Status=TEXT("Could not save replay. Check available disk space."); return false; }
	if (!WriteIndex(Current)) { IFileManager::Get().Move(*TemporaryFile,*MatchFile(Current.Id),false,true); Status=TEXT("Could not save replay details"); return false; }
	bCurrentSaved=true; ++LibraryRevision; TemporaryFile.Reset(); Status=TEXT("Replay saved"); return true;
}

bool UFlickReplaySubsystem::Watch(const FString& Id)
{
	if (FinalizingRecording.IsValid()) { Status=TEXT("Finishing replay recording"); return false; }
	if(IsLoadingTheatre()) return false;
	if (bCapturing) { Status=TEXT("Finish or leave the current match before watching a replay."); return false; }
	if (NeedsSessionExit()) { Status=TEXT("Leave the online room before watching a saved replay."); return false; }
	if (!ReadIndex(Id,Playing)) { Status=TEXT("Replay is missing or unreadable"); return false; }
	++PlaybackRevision; bPlaybackRequested=true; bPaused=bSeeking=false; Speed=1; PendingSeek=-1;
	bEditingClip=bPreviewingClip=bScrubbing=bBoundaryPreview=false;
	ShowLoading();
	return true;
}
bool UFlickReplaySubsystem::NeedsSessionExit() const
{
	const auto* Sessions=GetGameInstance()->GetSubsystem<UFlickSessionSubsystem>();
	return (GetWorld()&&GetWorld()->GetNetMode()!=NM_Standalone)||(Sessions&&(Sessions->HasActiveSession()||Sessions->GetState()!=EFlickSessionState::Idle));
}
bool UFlickReplaySubsystem::WatchAfterLeaving(const FString& Id)
{
	FFlickSavedReplay Check;
	if (bCapturing||!ReadIndex(Id,Check)) {Status=TEXT("Finish the current match before watching a replay.");return false;}
	PendingWatchId=Id;
	if(auto* I=Cast<UFlickGameInstance>(GetGameInstance())) I->bPostMatchQueuePending=false;
	if(auto* Party=GetGameInstance()->GetSubsystem<UFlickPartySubsystem>()) Party->Clear();
	if(auto* Sessions=GetGameInstance()->GetSubsystem<UFlickSessionSubsystem>())
	{
		Sessions->ClearPersistentPartyIdentity(); Sessions->CancelMatchmaking(false); Sessions->LeaveSession(true);
	}
	Status=TEXT("Leaving online room to open replay..."); return true;
}
bool UFlickReplaySubsystem::IsTheatre() const { return GetWorld() && GetWorld()->IsPlayingReplay(); }
float UFlickReplaySubsystem::GetTime() const { const auto* D=GetWorld()?GetWorld()->GetDemoNetDriver():nullptr; return D?D->GetDemoCurrentTime():0; }
float UFlickReplaySubsystem::GetDuration() const { const auto* D=GetWorld()?GetWorld()->GetDemoNetDriver():nullptr; return D?D->GetDemoTotalTime():Playing.Duration; }
float UFlickReplaySubsystem::GetViewEnd() const
{
	const float Duration=GetDuration()>0?GetDuration():Playing.Duration;
	return Playing.IsClip()?FMath::Min(Playing.ClipEnd,Duration):Duration;
}
float UFlickReplaySubsystem::GetDisplayTime() const
{
	const float Raw=bScrubbing?ScrubTime:PendingSeek>=0?PendingSeek:bSeeking?ActiveSeek:GetTime();
	const float Relative=FMath::Clamp(Raw-GetViewStart(),0.f,GetViewDuration());
	return GetViewDuration()-Relative<.01f?GetViewDuration():Relative;
}
void UFlickReplaySubsystem::BeginClip()
{
	if(!IsTheatre()||IsLoadingTheatre()||GetViewDuration()<.1f) return;
	++ClipEditorRevision;bEditingClip=true;bPreviewingClip=bBoundaryPreview=false;SetPaused(true);
	ClipIn=FMath::Clamp(GetTime()-GetViewStart()-5.f,0.f,FMath::Max(0.f,GetViewDuration()-.1f));
	ClipOut=FMath::Min(GetViewDuration(),FMath::Max(ClipIn+.1f,GetTime()-GetViewStart()+5.f));
	Status=TEXT("Choose a range, preview it, then save a named clip.");
}
void UFlickReplaySubsystem::CloseClip() { bEditingClip=bPreviewingClip=bBoundaryPreview=false;SetPaused(true); }
bool UFlickReplaySubsystem::SetClipRange(float Start,float End)
{
	if(!bEditingClip||!FMath::IsFinite(Start)||!FMath::IsFinite(End)||Start<0||End>GetViewDuration()||End-Start<.1f) return false;
	bPreviewingClip=false;SetPaused(true);ClipIn=Start;ClipOut=End;return true;
}
void UFlickReplaySubsystem::PreviewClip()
{
	if(!bEditingClip) return;
	bBoundaryPreview=false;
	if(bPreviewingClip) {bPreviewingClip=false;SetPaused(true);return;}
	bPreviewingClip=true;Seek(GetViewStart()+ClipIn);SetPaused(false);
}
void UFlickReplaySubsystem::PreviewClipBoundary(bool bStart,bool bFinal)
{
	if(!bEditingClip) return;
	bBoundaryPreview=!bFinal;bPreviewingClip=false;SetPaused(true);
	const float Target=FMath::Clamp(GetViewStart()+(bStart?ClipIn:ClipOut),GetViewStart(),FMath::Max(GetViewStart(),GetViewEnd()-.001f));
	// Releasing a handle or holding it against a limit must not repeat a seek
	// already queued, in flight, or displayed. Returning to an active/displayed
	// time also cancels an obsolete queued target from earlier in the drag.
	if((bSeeking&&FMath::IsNearlyEqual(ActiveSeek,Target,.0001f))
		||(!bSeeking&&FMath::IsNearlyEqual(GetTime(),Target,.0001f))) {PendingSeek=-1;return;}
	if(PendingSeek>=0&&FMath::IsNearlyEqual(PendingSeek,Target,.0001f)) return;
	Seek(Target);
}
void UFlickReplaySubsystem::BeginScrub()
{
	bPreviewingClip=bBoundaryPreview=false;
	bScrubWasPaused=bPaused;ScrubTime=GetTime();bScrubbing=true;SetPaused(true);
}
void UFlickReplaySubsystem::PreviewScrub(float Seconds)
{
	if(!FMath::IsFinite(Seconds)) return;
	if(bScrubbing) ScrubTime=FMath::Clamp(Seconds,GetViewStart(),GetViewEnd());else Seek(Seconds);
}
void UFlickReplaySubsystem::EndScrub()
{
	if(!bScrubbing) return;
	bScrubbing=false;Seek(ScrubTime);SetPaused(bScrubWasPaused);
}
void UFlickReplaySubsystem::Seek(float Seconds)
{
	if (!IsTheatre()||!FMath::IsFinite(Seconds)) return;
	const float Start=IsLoadingTheatre()?0.f:GetViewStart();
	const float End=IsLoadingTheatre()?GetDuration():GetViewEnd();
	PendingSeek=FMath::Clamp(Seconds,Start,FMath::Max(Start,End-.001f)); LastSeekRequest=FPlatformTime::Seconds();
}
void UFlickReplaySubsystem::JumpMarker(int32 Direction,bool bRoundsOnly)
{
	const float Now=GetTime(); float Target=Direction>0?GetViewEnd():GetViewStart();
	for (const auto& M:Playing.Markers)
	{
		if(M.Time<GetViewStart()||M.Time>GetViewEnd()) continue;
		if (bRoundsOnly&&!M.Label.StartsWith(TEXT("Round "))) continue;
		if (!bRoundsOnly&&!M.Label.StartsWith(TEXT("Shot "))&&M.Label!=TEXT("Kickoff")) continue;
		if (Direction>0 && M.Time>Now+.1f) { Target=M.Time; break; }
		if (Direction<0 && M.Time<Now-.1f) Target=M.Time;
	}
	Seek(Target);
}
void UFlickReplaySubsystem::SetPaused(bool bValue)
{
	if(!bValue&&!IsLoadingTheatre()&&!bSeeking&&PendingSeek<0&&GetTime()>=GetViewEnd()-.01f)
		Seek(bPreviewingClip?GetViewStart()+ClipIn:GetViewStart());
	bPaused=bValue;
	if (IsTheatre()) if (auto* W=GetWorld()->GetWorldSettings()) W->SetPauserPlayerState(bValue && GetWorld()->GetFirstPlayerController()?GetWorld()->GetFirstPlayerController()->PlayerState:nullptr);
}
void UFlickReplaySubsystem::SetSpeed(float Value)
{
	if (!FMath::IsFinite(Value)) return; Speed=FMath::Clamp(Value,.25f,2.f);
	if (IsTheatre()) GetWorld()->GetWorldSettings()->DemoPlayTimeDilation=Speed;
}
void UFlickReplaySubsystem::PlaybackFailed(const FString& Error) { HideLoading(); PreloadHandle.Reset(); bPlaybackRequested=false; bSeeking=false; PendingSeek=-1; Status=Error; }
void UFlickReplaySubsystem::ExitTheatre()
{
	HideLoading();
	if(PreloadHandle) PreloadHandle->CancelHandle();
	PreloadHandle.Reset();
	bReturnToLibrary=true;
	Status=TEXT("Replay library");
	bScrubbing=false;bPlaybackRequested=false; bPaused=bSeeking=false; PendingSeek=-1;
	bEditingClip=bPreviewingClip=bBoundaryPreview=false;
	if (GetWorld()) { if(auto* W=GetWorld()->GetWorldSettings()) W->SetPauserPlayerState(nullptr); UGameplayStatics::OpenLevel(this,FName(TEXT("/Engine/Maps/Entry"))); }
}
void UFlickReplaySubsystem::Deinitialize()
{
	HideLoading();
	FinishCapture();
	// Shutdown may wait, but never delete a temporary file with queued writes.
	if (FinalizingRecording.IsValid()) { FNetworkReplayStreaming::Get().Flush(); FinalizingRecording.Reset(); }
	if(!TemporaryFile.IsEmpty()) IFileManager::Get().Delete(*TemporaryFile,false,true);
	Super::Deinitialize();
}
