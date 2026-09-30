#include "Feedback/FlickWorldFeedback.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AFlickWorldFeedback::AFlickWorldFeedback()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
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
	if (EmissiveMaterial.Succeeded())
	{
		CoreFlash->SetMaterial(0, EmissiveMaterial.Object);
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
		if (EmissiveMaterial.Succeeded())
		{
			Shard->SetMaterial(0, EmissiveMaterial.Object);
		}
		Shards.Add(Shard);
	}
}

void AFlickWorldFeedback::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickWorldFeedback, Appearance);
}

void AFlickWorldFeedback::InitializeFeedback(
	const EFlickFeedbackKind InKind,
	const FLinearColor& InColor,
	const float Strength,
	const FVector& InBiasDirection,
	const int32 InStyle)
{
	Appearance.Kind = InKind;
	Appearance.Color = InColor;
	Appearance.Strength = Strength;
	Appearance.BiasDirection = InBiasDirection;
	Appearance.Style = FMath::Clamp(InStyle, 0, 2);
	// Impacts are numerous and remain local presentation; the selected launch,
	// arrival and knockout looks must be visible to every match participant.
	SetReplicates(InKind != EFlickFeedbackKind::Impact);
	ApplyFeedbackAppearance();
}

void AFlickWorldFeedback::OnRep_Appearance()
{
	ApplyFeedbackAppearance();
}

void AFlickWorldFeedback::ApplyFeedbackAppearance()
{
	const EFlickFeedbackKind InKind = Appearance.Kind;
	const FLinearColor InColor = Appearance.Color;
	const float Strength = Appearance.Strength;
	const FVector InBiasDirection = Appearance.BiasDirection;
	Kind = InKind;
	Style = Appearance.Style;
	Age = 0.0f;
	BiasDirection = FVector(InBiasDirection.X, InBiasDirection.Y, 0.0f).GetSafeNormal();
	const float SafeStrength = FMath::Clamp(Strength, 0.0f, 1.0f);

	switch (Kind)
	{
	case EFlickFeedbackKind::Spawn:
		Duration = Style == 1 ? 0.7f : 0.58f;
		MaxDistance = Style == 1 ? 105.0f : 78.0f;
		BaseShardScale = Style == 1 ? 0.075f : 0.09f;
		break;
	case EFlickFeedbackKind::Launch:
		Duration = 0.38f;
		MaxDistance = FMath::Lerp(45.0f, Style == 0 ? 95.0f : 125.0f, SafeStrength);
		BaseShardScale = FMath::Lerp(0.055f, 0.09f, SafeStrength);
		break;
	case EFlickFeedbackKind::Elimination:
		Duration = 0.9f;
		MaxDistance = Style == 1 ? 205.0f : Style == 2 ? 175.0f : 155.0f;
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
			const float EmissiveStrength = Kind == EFlickFeedbackKind::Elimination || Kind == EFlickFeedbackKind::Spawn ? 6.0f : 4.2f;
			const FLinearColor StyleColor = Style == 2 && Kind != EFlickFeedbackKind::Impact
				? FLinearColor(1.0f, 0.32f, 0.025f) : InColor;
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(
				StyleColor.R * EmissiveStrength,
				StyleColor.G * EmissiveStrength,
				StyleColor.B * EmissiveStrength,
				StyleColor.A));
			ShardMaterials.Add(Material);
		}
		Shard->SetVisibility(true);
	}
	CoreMaterial = CoreFlash->CreateAndSetMaterialInstanceDynamic(0);
	if (CoreMaterial)
	{
		const FLinearColor CoreColor = FMath::Lerp(InColor, FLinearColor::White, 0.58f);
		const float CoreStrength = Kind == EFlickFeedbackKind::Elimination ? 8.0f : 5.5f;
		CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(
			CoreColor.R * CoreStrength,
			CoreColor.G * CoreStrength,
			CoreColor.B * CoreStrength,
			CoreColor.A));
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
		Direction.Z = Kind == EFlickFeedbackKind::Elimination || Kind == EFlickFeedbackKind::Spawn
			? FMath::Lerp(0.45f, 1.1f, static_cast<float>(Index % 3) / 2.0f)
			: 0.12f + 0.16f * static_cast<float>(Index % 2);
		if (Style == 1 && (Kind == EFlickFeedbackKind::Elimination || Kind == EFlickFeedbackKind::Spawn))
		{
			Direction = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.06f);
		}
		else if (Style == 2 && (Kind == EFlickFeedbackKind::Elimination || Kind == EFlickFeedbackKind::Spawn))
		{
			Direction.Z += 0.8f;
		}
		Direction.Normalize();

		const float DistanceVariation = 0.78f + 0.22f * static_cast<float>((Index * 7) % 5) / 4.0f;
		Shard->SetRelativeLocation(Direction * MaxDistance * DistanceVariation * TravelAlpha);
		const float Scale = BaseShardScale * ScaleAlpha * (Index % 2 == 0 ? 1.0f : 0.72f);
		Shard->SetRelativeRotation(Direction.Rotation());
		Shard->SetRelativeScale3D(FVector(Scale * 2.8f, Scale * 0.5f, Scale * 0.5f));
	}
}
