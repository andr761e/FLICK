#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "Audio/FlickSoundSynthesis.h"
#include "GameFramework/Actor.h"
#include "FlickAudioDirector.generated.h"

class USoundWaveProcedural;
class USoundAttenuation;
class UAudioComponent;

// Replicate the small sound description, never generated PCM or gameplay state.
USTRUCT()
struct FFlickAudioCue
{
	GENERATED_BODY()
	UPROPERTY() uint8 Kind = 0;
	UPROPERTY() float Duration = 0;
	UPROPERTY() float Frequency = 0;
	UPROPERTY() float Volume = 0;
	UPROPERTY() float Pitch = 1;
	UPROPERTY() FVector Location = FVector::ZeroVector;
	UPROPERTY() bool bSpatial = false;
	UPROPERTY() float Timbre = 0.5f;
	UPROPERTY() float Intensity = 1;
};

UCLASS(NotBlueprintable)
class FLICK_API AFlickAudioDirector : public AActor
{
	GENERATED_BODY()

public:
	AFlickAudioDirector();
	virtual void Tick(float DeltaSeconds) override;

	void PlayUi(bool bConfirm);
	void PlayLaunch(EFlickPieceArchetype Archetype, float Power, const FVector& Location);
	void PlayImpact(
		float Strength,
		float CombinedMassKg,
		EFlickPieceArchetype FirstArchetype,
		EFlickPieceArchetype SecondArchetype,
		const FVector& Location);
	void PlayRimImpact(float Strength, float MassKg, EFlickPieceArchetype Archetype, const FVector& Location);
	void PlayRingOut(const FVector& Location);
	void PlayTurn(EFlickTeam Team);
	void PlayRoundResult(EFlickTeam Winner, bool bDraw, bool bSeriesComplete);
	void PlayReplayMusic(float Duration, EFlickTeam WinningTeam);
	void StopReplayMusic();
	void PlaySwitch(const FVector& Location);
	void PlayDivider(const FVector& Location);
	void PlayPocket(const FVector& Location);
	// Local-only: team ping audio must never leak to opponents or spectators.
	void PlayLocalNotification(bool bOwnTurn);

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpactCue(const FFlickAudioCue& Cue);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastImportantCue(const FFlickAudioCue& Cue);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReplayMusic(float Duration, EFlickTeam WinningTeam, bool bStop);
	void PlayLocalCue(const FFlickAudioCue& Cue);
	void PlayReplayMusicLocal(float Duration, EFlickTeam WinningTeam);
	USoundWaveProcedural* CreateSound(
		EFlickGeneratedSoundKind Kind,
		float Duration,
		float BaseFrequency,
		int32 Seed,
		float Timbre,
		float Intensity);
	void PlayGenerated(
		EFlickGeneratedSoundKind Kind,
		float Duration,
		float BaseFrequency,
		float Volume,
		float Pitch,
		const FVector* Location = nullptr,
		float Timbre = 0.5f,
		float Intensity = 1.0f);
	bool CanPlay(EFlickGeneratedSoundKind Kind, float MinimumInterval);
	float GetArchetypeTimbre(EFlickPieceArchetype Archetype) const;
	float GetEffectsVolume() const;
	float GetInterfaceVolume() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundWaveProcedural>> GeneratedSounds;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> SpatialAttenuation;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ReplayMusicComponent;

	// Kept separately from the rolling one-shot cache so a long replay bed
	// cannot be collected while short launch/impact cues are being generated.
	UPROPERTY(Transient)
	TObjectPtr<USoundWaveProcedural> ReplayMusicSound;

	float LastImpactTime = -100.0f;
	float LastRimImpactTime = -100.0f;
	float LastUiTime = -100.0f;
	int32 SoundSeed = 173;
	bool bReplayMusicActive = false;
};
