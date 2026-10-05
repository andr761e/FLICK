#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/FlickTypes.h"
#include "Core/FlickSpectatorView.h"
#include "FlickPrivateSpectatorComponent.generated.h"

class AFlickPlayerState;

// Publishes private players' cameras and follows one occupied seat or bot seat.
// Seats, rather than player IDs, also handle one human controlling several slots.
UCLASS()
class FLICK_API UFlickPrivateSpectatorComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlickPrivateSpectatorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void CycleTarget(int32 Direction);
	void StopFollowing();
	bool IsSpectator() const;
	bool IsFollowing() const { return bFollowing && IsSpectator(); }
	EFlickTeam GetTargetTeam() const;
	int32 GetTargetSlot() const { return TargetSeat == INDEX_NONE ? 0 : TargetSeat % 3; }
	FString GetTargetName() const;
	const AFlickPlayerState* GetTargetPlayer() const;

	UPROPERTY(EditAnywhere, Category = "FLICK|Spectator", meta = (ClampMin = "0.05", ClampMax = "0.5"))
	float CameraPublishInterval = 0.1f;

private:
	UFUNCTION(Server, Unreliable)
	void ServerPublishCamera(FFlickSpectatorView View);
	int32 TargetSeat = INDEX_NONE;
	bool bFollowing = false;
	float PublishElapsed = 0.0f;
	int32 AppliedSeat = INDEX_NONE;
	TWeakObjectPtr<const AFlickPlayerState> AppliedPlayer;
};
