#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlickPracticeComponent.generated.h"

class UProceduralMeshComponent;

// Offline one-shot practice packs. Uses normal launches, collisions and resolution.
UCLASS()
class FLICK_API UFlickPracticeComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFlickPracticeComponent();
	void Start(int32 InCategory, int32 InDifficulty);
	void Stop();
	void Restart();
	void ResolveShot();
	void Update(float DeltaSeconds);
	bool IsRunning() const { return Category >= 0; }
	bool IsComplete() const { return bComplete; }
	int32 GetCategory() const { return Category; }
	int32 GetDifficulty() const { return Difficulty; }
	int32 GetChallengeNumber() const { return FMath::Min(Challenge + 1, 5); }
	int32 GetScore() const { return Score; }
	int32 GetShooterId() const { return ShooterId; }
	int32 GetResult(int32 Index) const { return Results.IsValidIndex(Index) ? Results[Index] : -1; }
	FString GetObjective() const;
	FString GetFeedback() const;
	UPROPERTY(EditAnywhere, Category="FLICK|Practice", meta=(ClampMin="0.5", ClampMax="5"))
	float ResultDisplaySeconds = 2.5f;
	UPROPERTY(EditAnywhere, Category="FLICK|Practice", meta=(ClampMin="45", ClampMax="200"))
	float FoundationZoneRadius = 140.f;
	UPROPERTY(EditAnywhere, Category="FLICK|Practice", meta=(ClampMin="40", ClampMax="140"))
	float ExpertZoneRadius = 65.f;
	UPROPERTY(EditAnywhere, Category="FLICK|Practice")
	int32 LayoutSeed = 1337;
private:
	friend class FFlickPracticeTest;
	void SetupChallenge();
	void ShowMarker(const FVector& Location, float Radius, bool bTargetMarker = false);
	int32 Category = INDEX_NONE;
	int32 Difficulty = 0;
	int32 Challenge = 0;
	int32 Score = 0;
	int32 ShooterId = INDEX_NONE;
	int32 TargetId = INDEX_NONE;
	int32 GuardId = INDEX_NONE;
	int32 SwitchIndex = INDEX_NONE;
	bool bComplete = false;
	float TransitionRemaining = 0;
	FVector2D ZoneCenter = FVector2D::ZeroVector;
	float ZoneRadius = 140.f;
	TArray<int32> Results;
	FString Feedback;
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Marker;
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> TargetMarker;
	UPROPERTY()
	TObjectPtr<AActor> MarkerActor;
};
