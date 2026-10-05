#include "Feedback/FlickWorldFeedback.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Feedback/FlickCosmeticGeometry.h"
#include "Feedback/FlickCosmeticMaterial.h"

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
	const int32 InStyle,
	const float InArrivalDuration)
{
	Appearance.Kind = InKind;
	Appearance.Color = InColor;
	Appearance.Strength = Strength;
	Appearance.ArrivalDuration = FMath::Clamp(InArrivalDuration, 0.1f, 5.0f);
	Appearance.BiasDirection = InBiasDirection;
	const int32 Category = InKind == EFlickFeedbackKind::Spawn ? FlickCosmeticCatalog::SpawnCategory
		: InKind == EFlickFeedbackKind::Elimination ? FlickCosmeticCatalog::KnockoutCategory : FlickCosmeticCatalog::TrailCategory;
	Appearance.Style = InKind != EFlickFeedbackKind::Impact && FlickCosmeticCatalog::GetItems(Category).IsValidIndex(InStyle) ? InStyle : 0;
	// Newly added trails do not implicitly select a new launch burst.
	if (InKind == EFlickFeedbackKind::Launch && InStyle > 5) Appearance.Style = 0;
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
	if (KnockoutGeometry) KnockoutGeometry->SetVisibility(false);
	if (Kind == EFlickFeedbackKind::Spawn)
	{
		InitializeSpawnEffect();
		return;
	}
	if (SpawnGeometry) SpawnGeometry->SetVisibility(false);
	BiasDirection = FVector(InBiasDirection.X, InBiasDirection.Y, 0.0f).GetSafeNormal();
	if (Kind == EFlickFeedbackKind::Elimination && Style > 0)
	{
		InitializeKnockoutEffect();
		return;
	}
	const float SafeStrength = FMath::Clamp(Strength, 0.0f, 1.0f);
	const bool bCollection = Style >= 3 && Kind != EFlickFeedbackKind::Impact;
	const FLinearColor EffectColor = bCollection ? FlickCosmeticCatalog::GetCollectionColor(Style - 3) : InColor;

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
	if (bCollection)
	{
		Duration = Kind == EFlickFeedbackKind::Launch ? 0.42f : Kind == EFlickFeedbackKind::Spawn ? 1.05f : 1.25f;
		MaxDistance = Kind == EFlickFeedbackKind::Spawn ? 95.0f : Kind == EFlickFeedbackKind::Elimination ? 175.0f : 65.0f;
		// Existing ten shards plus fourteen finer flecks, allocated only for collection effects.
		while (Shards.Num() < 24)
		{
			UStaticMeshComponent* Shard = NewObject<UStaticMeshComponent>(this);
			Shard->SetupAttachment(SceneRoot);
			Shard->SetStaticMesh(Shards[0]->GetStaticMesh());
			Shard->SetMaterial(0, Shards[0]->GetMaterial(0));
			Shard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Shard->SetGenerateOverlapEvents(false);
			Shard->SetCastShadow(false);
			Shard->SetCanEverAffectNavigation(false);
			Shard->RegisterComponent();
			Shards.Add(Shard);
		}
		if (HaloRings.IsEmpty())
		{
			for (int32 Index = 0; Index < 3; ++Index)
			{
				UProceduralMeshComponent* Ring = NewObject<UProceduralMeshComponent>(this);
				Ring->SetupAttachment(SceneRoot);
				Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Ring->SetGenerateOverlapEvents(false);
				Ring->SetCastShadow(false);
				Ring->SetCanEverAffectNavigation(false);
				FlickCosmeticGeometry::BuildRing(*Ring, 0, Style == 3 && Index == 1 ? 6 : 96);
				Ring->SetMaterial(0, CoreFlash->GetMaterial(0));
				Ring->RegisterComponent();
				HaloRings.Add(Ring);
				UMaterialInstanceDynamic* Material = FlickCosmeticMaterial::CreateGlow(this);
				Ring->SetMaterial(0, Material);
				HaloMaterials.Add(Material);
			}
		}
	}
	for (UProceduralMeshComponent* Ring : HaloRings) Ring->SetVisibility(bCollection);

	ShardMaterials.Reset();
	ShardMaterials.Reserve(Shards.Num());
	for (UStaticMeshComponent* Shard : Shards)
	{
		if (!Shard)
		{
			continue;
		}

		UMaterialInstanceDynamic* Material = bCollection ? FlickCosmeticMaterial::CreateGlow(this) : Shard->CreateAndSetMaterialInstanceDynamic(0);
		if (bCollection) Shard->SetMaterial(0, Material);
		if (Material)
		{
			const float EmissiveStrength = Kind == EFlickFeedbackKind::Elimination || Kind == EFlickFeedbackKind::Spawn ? 6.0f : 4.2f;
			const FLinearColor StyleColor = Style == 2 && Kind != EFlickFeedbackKind::Impact
				? FLinearColor(1.0f, 0.32f, 0.025f) : EffectColor;
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(
				StyleColor.R * EmissiveStrength,
				StyleColor.G * EmissiveStrength,
				StyleColor.B * EmissiveStrength,
				StyleColor.A));
			ShardMaterials.Add(Material);
		}
		Shard->SetVisibility(true);
	}
	CoreMaterial = bCollection ? FlickCosmeticMaterial::CreateGlow(this) : CoreFlash->CreateAndSetMaterialInstanceDynamic(0);
	if (bCollection) CoreFlash->SetMaterial(0, CoreMaterial);
	if (CoreMaterial)
	{
		const FLinearColor CoreColor = FMath::Lerp(EffectColor, FLinearColor::White, 0.58f);
		const float CoreStrength = Kind == EFlickFeedbackKind::Elimination ? 8.0f : 5.5f;
		CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(
			CoreColor.R * CoreStrength,
			CoreColor.G * CoreStrength,
			CoreColor.B * CoreStrength,
			CoreColor.A));
	}
	CoreFlash->SetVisibility(true);
	// Place new effects before the first render; no origin flash or full-size cubes.
	if (bCollection) UpdateCollectionEffect(0.0f);

	SetLifeSpan(Duration + 0.1f);
}

void AFlickWorldFeedback::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;
	if (Kind == EFlickFeedbackKind::Spawn)
	{
		UpdateSpawnEffect();
		return;
	}
	if (Kind == EFlickFeedbackKind::Elimination && Style > 0)
	{
		UpdateKnockoutEffect();
		return;
	}
	const float Alpha = FMath::Clamp(Age / FMath::Max(Duration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	if (Style >= 3 && Kind != EFlickFeedbackKind::Impact)
	{
		UpdateCollectionEffect(Alpha);
		return;
	}
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

void AFlickWorldFeedback::UpdateCollectionEffect(const float Alpha)
{
	const bool bArrival = Kind == EFlickFeedbackKind::Spawn;
	const bool bLaunch = Kind == EFlickFeedbackKind::Launch;
	const bool bRift = Style == 5;
	const bool bSolar = Style == 4;
	const FLinearColor Color = FlickCosmeticCatalog::GetCollectionColor(Style - 3);
	const FLinearColor Secondary = bRift ? FLinearColor(0.05f, 0.85f, 1.0f)
		: FMath::Lerp(Color, FLinearColor::White, bSolar ? 0.55f : 0.7f);
	const float Fade = FMath::Square(1.0f - Alpha);
	const float Ease = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);
	for (int32 Index = 0; Index < HaloRings.Num(); ++Index)
	{
		UProceduralMeshComponent* Ring = HaloRings[Index];
		const float Local = FMath::Clamp((Alpha - Index * 0.1f) / (1.0f - Index * 0.1f), 0.0f, 1.0f);
		const float Envelope = FMath::Sin(Local * PI);
		float Radius = FMath::Lerp(18.0f, MaxDistance * (0.6f + Index * 0.2f), 1.0f - FMath::Square(1.0f - Local));
		if (bArrival) Radius = bRift ? FMath::Lerp(105.0f - Index * 16.0f, 25.0f, Ease)
			: bSolar ? FMath::Lerp(20.0f, 80.0f + Index * 15.0f, Ease)
			: 38.0f + Index * 22.0f + 8.0f * FMath::Sin(Local * PI);
		else if (bRift && Alpha < 0.55f) Radius = FMath::Lerp(115.0f - Index * 12.0f, 9.0f, Alpha / 0.55f);
		Ring->SetVisibility(Envelope > 0.015f && !bLaunch);
		Ring->SetRelativeScale3D(FVector(Radius, Radius, 1.0f));
		Ring->SetRelativeLocation(FVector(0.0f, 0.0f, bArrival ? 4.0f + Index * 5.0f + (bRift ? 45.0f * (1.0f - Ease) : 0.0f) : 5.0f + Index * 3.0f));
		Ring->SetRelativeRotation(FRotator(bArrival && bRift ? 16.0f * (1.0f - Ease) : 0.0f, Age * (Index % 2 ? -75.0f : 55.0f), 0.0f));
		if (HaloMaterials.IsValidIndex(Index) && HaloMaterials[Index])
			FlickCosmeticMaterial::SetGlow(HaloMaterials[Index], (Index == 1 ? Secondary : Color) * (Envelope * 4.5f));
	}
	const float CoreEnvelope = bRift && !bArrival && !bLaunch
		? FMath::Sin(FMath::Clamp((Alpha - 0.42f) * 4.0f, 0.0f, 1.0f) * PI) : FMath::Sin(FMath::Clamp(Alpha * 2.3f, 0.0f, 1.0f) * PI);
	const float CoreSize = CoreEnvelope * (bSolar ? 0.34f : 0.2f);
	CoreFlash->SetRelativeScale3D(FVector(CoreSize, CoreSize, CoreSize * 0.25f));
	FlickCosmeticMaterial::SetGlow(CoreMaterial, Secondary * (5.0f * CoreEnvelope));
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		UStaticMeshComponent* Shard = Shards[Index];
		const float Seed = static_cast<float>((Index * 13) % 23) / 23.0f;
		float Angle = 2.0f * PI * Index / Shards.Num();
		float Distance = MaxDistance * Ease * (0.45f + Seed * 0.55f);
		float Height = 5.0f;
		if (bArrival)
		{
			Angle += Age * (bRift ? -3.8f : 2.4f);
			Distance = bSolar ? 28.0f + 40.0f * Ease : FMath::Lerp(90.0f, 32.0f, Ease);
			Height = bSolar ? 110.0f * FMath::Sin(Alpha * PI) * (0.4f + Seed) : (1.0f - Ease) * (30.0f + Seed * 65.0f);
		}
		else if (bRift && !bLaunch)
		{
			Angle += Age * 5.0f;
			Distance = Alpha < 0.55f ? FMath::Lerp(120.0f, 3.0f, Alpha / 0.55f)
				: 150.0f * (Alpha - 0.55f) / 0.45f;
			Height = 15.0f + 35.0f * FMath::Sin(Alpha * PI);
		}
		else Height = bSolar ? 10.0f + 130.0f * Alpha * (1.0f - Alpha) * (0.7f + Seed) : 5.0f + Seed * 25.0f * Ease;
		FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		const FVector Location = Direction * Distance + FVector(0.0f, 0.0f, Height) + BiasDirection * (bLaunch ? 65.0f * Ease : 0.0f);
		Shard->SetRelativeLocation(Location);
		Shard->SetRelativeRotation(FRotator(bSolar ? 60.0f + Seed * 50.0f : 0.0f, FMath::RadiansToDegrees(Angle) + (bArrival || bRift ? 90.0f : 0.0f), Age * 100.0f));
		const float Envelope = FMath::Sin(Alpha * PI) * (0.65f + Seed * 0.35f);
		const float Size = Envelope * (bLaunch ? 0.028f : 0.045f);
		Shard->SetRelativeScale3D(FVector(Size * (bSolar ? 1.0f : 3.4f), Size * 0.35f, Size * (bSolar ? 0.7f : 0.2f)));
		if (ShardMaterials.IsValidIndex(Index) && ShardMaterials[Index])
			FlickCosmeticMaterial::SetGlow(ShardMaterials[Index], (Index % 4 == 0 ? Secondary : Color) * (4.0f * Fade));
	}
}
