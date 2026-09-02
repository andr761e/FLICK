#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/Actor.h"
#include "FlickAudioDirector.generated.h"

class USoundWaveProcedural;

enum class EFlickGeneratedSoundKind : uint8
{
	UiNavigate,
	UiConfirm,
	Launch,
	Impact,
	Scrape,
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
	void PlayImpact(float Strength, float CombinedMassKg, const FVector& Location);
	void PlayRingOut(const FVector& Location);
	void PlayTurn(EFlickTeam Team);
	void PlayRoundResult(EFlickTeam Winner, bool bDraw, bool bSeriesComplete);
	void UpdateScrape(float NormalizedSpeed, const FVector& Location, float DeltaSeconds);

private:
	USoundWaveProcedural* CreateSound(
		EFlickGeneratedSoundKind Kind,
		float Duration,
		float BaseFrequency,
		int32 Seed);
	void PlayGenerated(
		EFlickGeneratedSoundKind Kind,
		float Duration,
		float BaseFrequency,
		float Volume,
		float Pitch,
		const FVector* Location = nullptr);
	float GetEffectsVolume() const;
	float GetInterfaceVolume() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundWaveProcedural>> GeneratedSounds;

	float ScrapeCooldown = 0.0f;
	int32 SoundSeed = 173;
};
