#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/Actor.h"
#include "FlickAudioDirector.generated.h"

class USoundWaveProcedural;
class USoundAttenuation;

enum class EFlickGeneratedSoundKind : uint8
{
	UiNavigate,
	UiConfirm,
	Launch,
	Impact,
	RimImpact,
	RingOut,
	Turn,
	RoundWin,
	MatchWin
};

UCLASS(NotBlueprintable)
class FLICK_API AFlickAudioDirector : public AActor
{
	GENERATED_BODY()

public:
	AFlickAudioDirector();

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

private:
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

	float LastImpactTime = -100.0f;
	float LastRimImpactTime = -100.0f;
	float LastUiTime = -100.0f;
	int32 SoundSeed = 173;
};
