#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FlickStadiumCrowd.generated.h"

class UInstancedStaticMeshComponent;

/** Static, non-colliding spectators batched by colour, not one actor per person. */
UCLASS(ClassGroup = (FLICK))
class FLICK_API UFlickStadiumCrowd : public USceneComponent
{
	GENERATED_BODY()
public:
	UFlickStadiumCrowd();
	void Configure(bool bBob, float StadiumScale, float SurfaceZ, bool bCrowdVisible);

	UPROPERTY(EditAnywhere, Category = "Crowd", meta = (ClampMin = "0", ClampMax = "1"))
	float Occupancy = 0.82f;

	UPROPERTY(EditAnywhere, Category = "Crowd", meta = (ClampMin = "0.5", ClampMax = "1.5"))
	float SpectatorScale = 1.0f;

private:
	void AddSpectator(const FVector& Seat, float OutwardAngle, int32 Index, bool bBob);
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;
	bool bBuilt = false;
	bool bLastBob = false;
	float LastOccupancy = -1.0f;
	float LastSpectatorScale = -1.0f;
};
