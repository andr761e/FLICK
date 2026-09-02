#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlickWorldFeedback.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

UENUM()
enum class EFlickFeedbackKind : uint8
{
	Launch,
	Impact,
	Elimination
};

UCLASS(NotBlueprintable)
class FLICK_API AFlickWorldFeedback : public AActor
{
	GENERATED_BODY()

public:
	AFlickWorldFeedback();

	virtual void Tick(float DeltaSeconds) override;
	void InitializeFeedback(
		EFlickFeedbackKind InKind,
		const FLinearColor& InColor,
		float Strength,
		const FVector& InBiasDirection = FVector::ZeroVector);

private:
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
	FVector BiasDirection = FVector::ZeroVector;
	float Age = 0.0f;
	float Duration = 0.4f;
	float MaxDistance = 70.0f;
	float BaseShardScale = 0.08f;
};
