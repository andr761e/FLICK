#pragma once
#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Components/ActorComponent.h"
#include "FlickMenuRadioComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

// Local presentation only. No music during gameplay, including paused-match settings.
UCLASS()
class FLICK_API UFlickMenuRadioComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlickMenuRadioComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void Skip(int32 Direction);
	void TogglePlayback();
	FString GetTrackTitle() const;
	FString GetStatus() const;
	bool IsEnabled() const;
	float GetProgress() const;
private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FFlickRadioContextTest;
	friend class FFlickRadioResumeTest;
#endif
	void RequestTrack();
	void StartTrack(const TArray<int16>& Samples);
	void SuspendPlayback();
	bool IsMenuContext() const;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Music;
	UPROPERTY(Transient) TObjectPtr<USoundWaveProcedural> Source;
	TFuture<TArray<int16>> PendingSamples;
	TMap<int32, TArray<int16>> Cache;
	int32 Track = 0;
	int32 PendingTrack = INDEX_NONE;
	double PreviousTime = 0;
	double PauseAt = 0;
	double FadeStartedAt = 0;
	float FadeStartVolume = 0;
	float AppliedVolume = 0;
	float PlayedSeconds = 0;
	float LastVolume = -1;
	bool bAudible = false;
};
