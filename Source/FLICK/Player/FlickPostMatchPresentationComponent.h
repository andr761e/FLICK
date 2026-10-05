#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlickPostMatchPresentationComponent.generated.h"

// Local presentation only: never spawns gameplay pieces or changes their physics.
UCLASS()
class FLICK_API UFlickPostMatchPresentationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlickPostMatchPresentationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	void ContinueToSummary() { bContinueRequested = true; }
	bool ShowsDetails() const { return bActive && GetElapsed() >= ShowcaseSeconds + PullbackSeconds; }
	FVector GetWinnerPosition(int32 Slot) const;
	float GetElapsed() const;
	static constexpr float ShowcaseSeconds = 5.0f;
	static constexpr float PullbackSeconds = 1.4f;
private:
	void ClearPresentation();
	void BuildWinner(int32 Slot);
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> DisplayPucks;
	TMap<TWeakObjectPtr<class UPrimitiveComponent>, bool> HiddenComponents;
	FString MatchId;
	float MatchStart = -1.0f;
	float ContinuedAt = -1.0f;
	bool bContinueRequested = false;
	bool bActive = false;
};
