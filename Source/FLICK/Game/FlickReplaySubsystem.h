#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Core/FlickTypes.h"
#include "FlickReplaySubsystem.generated.h"

class INetworkReplayStreamer;

struct FFlickReplayMarker
{
	float Time = 0;
	FString Label;
};

// The .flick.replay payload uses Unreal's versioned replay stream. This small
// separate index keeps library operations independent of loading a replay world.
struct FFlickSavedReplay
{
	FString Id, Name, RecordedAt, GameVersion, Mode, Result;
	// Clips reference a full recording directly, even when made from another clip.
	FString SourceId, SourceName;
	float ClipStart = 0, ClipEnd = 0;
	bool IsClip() const { return !SourceId.IsEmpty(); }
	const FString& PayloadId() const { return IsClip()?SourceId:Id; }
	float ViewDuration() const { return IsClip()?ClipEnd-ClipStart:Duration; }
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;
	int32 TeamSize = 1;
	float Duration = 0;
	// Missing in legacy indexes, which were recorded with Unreal's 8 Hz default.
	float PoseSampleRate = 8.f;
	bool bFavourite = false;
	TArray<FString> Players;
	TArray<FSoftObjectPath> Assets;
	TArray<FFlickReplayMarker> Markers;
	int64 Bytes = 0;
};

UCLASS()
class FLICK_API UFlickReplaySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UFlickReplaySubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	virtual void Deinitialize() override;
	bool CanSave() const;
	bool IsRecordingMatch() const { return bCapturing; }
	bool IsCurrentSaved() const { return bCurrentSaved; }
	bool SaveCurrent();
	const FString& GetSaveName() const { return Current.Name; }
	void SetSaveName(const FString& Name) { if(!bCurrentSaved) Current.Name=Name.Left(100); }
	TArray<FFlickSavedReplay> List() const;
	bool RenameReplay(const FString& Id, const FString& Name);
	bool SetFavourite(const FString& Id, bool bValue);
	bool Delete(const FString& Id);
	// Times are relative to the selected library entry. Returns the new entry ID.
	FString SaveClip(const FString& Id, const FString& Name, float Start, float End);
	bool Watch(const FString& Id);
	bool NeedsSessionExit() const;
	bool WatchAfterLeaving(const FString& Id);
	void ExitTheatre();
	bool IsTheatre() const;
	bool IsLoadingTheatre() const { return bPlaybackRequested || bWarmingTheatre; }
	const FFlickSavedReplay& GetPlaying() const { return Playing; }
	const FString& GetStatus() const { return Status; }
	float GetTime() const;
	float GetDuration() const;
	// Native playback/pose clocks stay in source seconds; the UI is clip-relative.
	float GetViewStart() const { return Playing.IsClip()?Playing.ClipStart:0.f; }
	float GetViewEnd() const;
	float GetViewDuration() const { return FMath::Max(0.f,GetViewEnd()-GetViewStart()); }
	float GetDisplayTime() const;
	void BeginClip();
	void CloseClip();
	bool IsEditingClip() const { return bEditingClip; }
	int32 GetClipEditorRevision() const { return ClipEditorRevision; }
	bool SetClipRange(float Start, float End);
	float GetClipIn() const { return ClipIn; }
	float GetClipOut() const { return ClipOut; }
	void PreviewClip();
	void PreviewClipBoundary(bool bStart, bool bFinal=false);
	bool IsPreviewingClip() const { return bPreviewingClip; }
	void Seek(float Seconds);
	void BeginScrub();
	void PreviewScrub(float Seconds);
	void EndScrub();
	void JumpMarker(int32 Direction, bool bRoundsOnly = false);
	void SetPaused(bool bValue);
	bool IsPaused() const { return bPaused; }
	void SetSpeed(float Value);
	float GetSpeed() const { return Speed; }
	float GetPoseSampleRate() const { return Playing.PoseSampleRate; }
	bool IsSeeking() const { return bSeeking; }
	int32 GetPlaybackRevision() const { return PlaybackRevision; }
	int32 GetLibraryRevision() const { return LibraryRevision; }
	bool bReturnToLibrary = false;
	void PlaybackFailed(const FString& Error);
	static FString Root();
	static bool IsValidId(const FString& Id);
	static bool WriteIndex(const FFlickSavedReplay& Replay);
	static bool ReadIndex(const FString& Id, FFlickSavedReplay& Out);
private:
	void BeginPlayback();
	void TickLoading();
	void ShowLoading();
	void HideLoading();
	void StartCapture(class AFlickGameState& State);
	void FinishCapture();
	void AddMarker(const FString& Label);
	FString CaptureMatchId, TemporaryFile, Status, PendingWatchId;
	FFlickSavedReplay Current, Playing;
	bool bCapturing = false, bCurrentSaved = false, bPlaybackRequested = false;
	TSharedPtr<INetworkReplayStreamer> FinalizingRecording;
	bool bPaused = false, bSeeking = false;
	bool bScrubbing = false, bScrubWasPaused = false;
	bool bEditingClip = false, bPreviewingClip = false;
	bool bBoundaryPreview = false;
	double LastBoundarySeek = 0;
	float ClipIn = 0, ClipOut = 0;
	int32 ClipEditorRevision = 0;
	float ScrubTime = 0;
	bool bPreloading = false, bWarmingTheatre = false;
	bool bWarmPrerolled = false, bWarmRewinding = false;
	double PrerollStarted = 0;
	double LoadingStarted = 0, StableSince = 0;
	int32 WarmFrames = 0, LastWarmActorCount = -1;
	TSharedPtr<struct FStreamableHandle> PreloadHandle;
	TSharedPtr<class SWidget> LoadingWidget;
	float Speed = 1, PendingSeek = -1, ActiveSeek = 0, EndDelay = 0, LastCheckpoint = 0;
	int32 LastRound = 0, LastTurn = 0, LastCount = -1, LastSwitches = 0;
	int32 LibraryRevision = 0, PlaybackRevision = 0;
	double LastSeekRequest = 0;
	double NextBufferRequest = 0;
	TWeakObjectPtr<UWorld> CaptureWorld;
};
