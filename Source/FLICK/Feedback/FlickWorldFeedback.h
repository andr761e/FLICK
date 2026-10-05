#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlickWorldFeedback.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;

UENUM()
enum class EFlickFeedbackKind : uint8
{
	Launch,
	Impact,
	Elimination,
	Spawn
};

USTRUCT()
struct FFlickFeedbackAppearance
{
	GENERATED_BODY()

	UPROPERTY() EFlickFeedbackKind Kind = EFlickFeedbackKind::Impact;
	UPROPERTY() FLinearColor Color = FLinearColor::White;
	UPROPERTY() float Strength = 0.0f;
	UPROPERTY() FVector BiasDirection = FVector::ZeroVector;
	UPROPERTY() int32 Style = 0;
	UPROPERTY() float ArrivalDuration = 1.2f;
};

UCLASS(NotBlueprintable)
class FLICK_API AFlickWorldFeedback : public AActor
{
	GENERATED_BODY()

public:
	AFlickWorldFeedback();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void InitializeFeedback(
		EFlickFeedbackKind InKind,
		const FLinearColor& InColor,
		float Strength,
		const FVector& InBiasDirection = FVector::ZeroVector,
		int32 InStyle = 0,
		float InArrivalDuration = 1.2f);

private:
	UFUNCTION()
	void OnRep_Appearance();
	void ApplyFeedbackAppearance();
	void UpdateCollectionEffect(float Alpha);
	void InitializeSpawnEffect();
	void UpdateSpawnEffect();
	void InitializeKnockoutEffect();
	void UpdateKnockoutEffect();
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> KnockoutGeometry;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> KnockoutMaterials;
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> SpawnGeometry;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SpawnMaterials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> HaloRings;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> HaloMaterials;
	UPROPERTY(ReplicatedUsing = OnRep_Appearance)
	FFlickFeedbackAppearance Appearance;
	UPROPERTY(VisibleAnywhere, Category = "FLICK|Feedback")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Feedback")
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Feedback")
	TObjectPtr<UStaticMeshComponent> CoreFlash;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ShardMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;

	EFlickFeedbackKind Kind = EFlickFeedbackKind::Impact;
	int32 Style = 0;
	FVector BiasDirection = FVector::ZeroVector;
	float Age = 0.0f;
	float Duration = 0.4f;
	float MaxDistance = 70.0f;
	float BaseShardScale = 0.08f;
};
