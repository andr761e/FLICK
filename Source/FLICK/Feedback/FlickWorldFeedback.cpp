#include "Feedback/FlickWorldFeedback.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFlickWorldFeedback::AFlickWorldFeedback()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CoreFlash = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreFlash"));
	CoreFlash->SetupAttachment(SceneRoot);
	CoreFlash->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoreFlash->SetGenerateOverlapEvents(false);
	CoreFlash->SetCastShadow(false);
	CoreFlash->SetCanEverAffectNavigation(false);
	if (SphereMesh.Succeeded())
	{
		CoreFlash->SetStaticMesh(SphereMesh.Object);
	}
	if (BasicMaterial.Succeeded())
	{
		CoreFlash->SetMaterial(0, BasicMaterial.Object);
	}

	constexpr int32 ShardCount = 10;
	Shards.Reserve(ShardCount);
	for (int32 Index = 0; Index < ShardCount; ++Index)
	{
		UStaticMeshComponent* Shard = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Shard_%02d"), Index));
		Shard->SetupAttachment(SceneRoot);
		Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shard->SetGenerateOverlapEvents(false);
		Shard->SetCastShadow(false);
		Shard->SetCanEverAffectNavigation(false);
		if (CubeMesh.Succeeded())
		{
			Shard->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Shard->SetMaterial(0, BasicMaterial.Object);
		}
		Shards.Add(Shard);
	}
}

void AFlickWorldFeedback::InitializeFeedback(
	const EFlickFeedbackKind InKind,
	const FLinearColor& InColor,
	const float Strength,
	const FVector& InBiasDirection)
{
	Kind = InKind;
	Age = 0.0f;
	BiasDirection = FVector(InBiasDirection.X, InBiasDirection.Y, 0.0f).GetSafeNormal();
	const float SafeStrength = FMath::Clamp(Strength, 0.0f, 1.0f);

	switch (Kind)
	{
	case EFlickFeedbackKind::Launch:
		Duration = 0.38f;
		MaxDistance = FMath::Lerp(45.0f, 95.0f, SafeStrength);
		BaseShardScale = FMath::Lerp(0.055f, 0.09f, SafeStrength);
		break;
	case EFlickFeedbackKind::Elimination:
		Duration = 0.9f;
		MaxDistance = 155.0f;
		BaseShardScale = 0.12f;
		break;
	case EFlickFeedbackKind::Impact:
	default:
		Duration = FMath::Lerp(0.25f, 0.42f, SafeStrength);
		MaxDistance = FMath::Lerp(30.0f, 105.0f, SafeStrength);
		BaseShardScale = FMath::Lerp(0.045f, 0.105f, SafeStrength);
		break;
	}

	ShardMaterials.Reset();
	ShardMaterials.Reserve(Shards.Num());
	for (UStaticMeshComponent* Shard : Shards)
	{
		if (!Shard)
		{
			continue;
		}

		UMaterialInstanceDynamic* Material = Shard->CreateAndSetMaterialInstanceDynamic(0);
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), InColor);
			Material->SetVectorParameterValue(TEXT("BaseColor"), InColor);
			ShardMaterials.Add(Material);
		}
		Shard->SetVisibility(true);
	}
	CoreMaterial = CoreFlash->CreateAndSetMaterialInstanceDynamic(0);
	if (CoreMaterial)
	{
		CoreMaterial->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(InColor, FLinearColor::White, 0.58f));
		CoreMaterial->SetVectorParameterValue(TEXT("BaseColor"), FMath::Lerp(InColor, FLinearColor::White, 0.58f));
	}
	CoreFlash->SetVisibility(true);

	SetLifeSpan(Duration + 0.1f);
}

void AFlickWorldFeedback::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	const float Alpha = FMath::Clamp(Age / FMath::Max(Duration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const float TravelAlpha = 1.0f - FMath::Square(1.0f - Alpha);
	const float ScaleAlpha = FMath::Sin(Alpha * PI);
	const float CoreScale = BaseShardScale
		* (Kind == EFlickFeedbackKind::Elimination ? 2.1f : 1.55f)
		* FMath::Sin(FMath::Clamp(Alpha * 1.45f, 0.0f, 1.0f) * PI);
	CoreFlash->SetRelativeScale3D(FVector(CoreScale, CoreScale, CoreScale * 0.46f));

	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		UStaticMeshComponent* Shard = Shards[Index];
		if (!Shard)
		{
			continue;
		}

		const float Angle = (2.0f * PI * Index) / FMath::Max(1, Shards.Num());
		FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		Direction += BiasDirection * (Kind == EFlickFeedbackKind::Launch ? 1.25f : 0.35f);
		Direction.Z = Kind == EFlickFeedbackKind::Elimination
			? FMath::Lerp(0.45f, 1.1f, static_cast<float>(Index % 3) / 2.0f)
			: 0.12f + 0.16f * static_cast<float>(Index % 2);
		Direction.Normalize();

		const float DistanceVariation = 0.78f + 0.22f * static_cast<float>((Index * 7) % 5) / 4.0f;
		Shard->SetRelativeLocation(Direction * MaxDistance * DistanceVariation * TravelAlpha);
		const float Scale = BaseShardScale * ScaleAlpha * (Index % 2 == 0 ? 1.0f : 0.72f);
		Shard->SetRelativeRotation(Direction.Rotation());
		Shard->SetRelativeScale3D(FVector(Scale * 2.8f, Scale * 0.5f, Scale * 0.5f));
	}
}
