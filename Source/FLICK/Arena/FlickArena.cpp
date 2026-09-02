#include "Arena/FlickArena.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 ArenaRimSegmentCount = 44;
	constexpr int32 ArenaFieldRingCount = 5;
	constexpr int32 ArenaFieldRingSegmentCount = 56;
	constexpr int32 ArenaSideLightSegmentCount = 48;
	constexpr int32 ArenaSurfaceSeamCount = 20;
	constexpr int32 ArenaVenuePylonCount = 11;
	constexpr int32 ArenaVenueBannerCount = 10;
	constexpr int32 ArenaFloorGridCount = 20;
	constexpr int32 MultiplayerTeamArcSegmentsPerSide = 24;
	constexpr int32 MaximumMultiplayerPlayersPerTeam = 3;
}

AFlickArena::AFlickArena()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ArenaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArenaMesh"));
	ArenaMesh->SetupAttachment(SceneRoot);
	TopSurfaceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopSurfaceMesh"));
	TopSurfaceMesh->SetupAttachment(SceneRoot);
	InnerFieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerFieldMesh"));
	InnerFieldMesh->SetupAttachment(SceneRoot);
	RimAccentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RimAccentMesh"));
	RimAccentMesh->SetupAttachment(SceneRoot);
	CenterPlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CenterPlateMesh"));
	CenterPlateMesh->SetupAttachment(SceneRoot);
	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(SceneRoot);
	BackdropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackdropMesh"));
	BackdropMesh->SetupAttachment(SceneRoot);
	StageBaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageBaseMesh"));
	StageBaseMesh->SetupAttachment(SceneRoot);
	VenueBackWallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VenueBackWallMesh"));
	VenueBackWallMesh->SetupAttachment(SceneRoot);
	CenterLineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CenterLineMesh"));
	CenterLineMesh->SetupAttachment(SceneRoot);
	CrossLineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrossLineMesh"));
	CrossLineMesh->SetupAttachment(SceneRoot);
	CenterMarkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CenterMarkMesh"));
	CenterMarkMesh->SetupAttachment(SceneRoot);
	Player1HomeMarkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Player1HomeMarkMesh"));
	Player1HomeMarkMesh->SetupAttachment(SceneRoot);
	Player2HomeMarkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Player2HomeMarkMesh"));
	Player2HomeMarkMesh->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CylinderMesh.Succeeded())
	{
		ArenaMesh->SetStaticMesh(CylinderMesh.Object);
		TopSurfaceMesh->SetStaticMesh(CylinderMesh.Object);
		InnerFieldMesh->SetStaticMesh(CylinderMesh.Object);
		RimAccentMesh->SetStaticMesh(CylinderMesh.Object);
		CenterPlateMesh->SetStaticMesh(CylinderMesh.Object);
		PedestalMesh->SetStaticMesh(CylinderMesh.Object);
		StageBaseMesh->SetStaticMesh(CylinderMesh.Object);
		CenterMarkMesh->SetStaticMesh(CylinderMesh.Object);
	}

	RimSegments.Reserve(ArenaRimSegmentCount);
	for (int32 Index = 0; Index < ArenaRimSegmentCount; ++Index)
	{
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("RimSegment_%02d"), Index));
		Segment->SetupAttachment(SceneRoot);
		if (CubeMesh.Succeeded())
		{
			Segment->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Segment->SetMaterial(0, BasicMaterial.Object);
		}
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetGenerateOverlapEvents(false);
		Segment->SetCastShadow(false);
		Segment->SetCanEverAffectNavigation(false);
		RimSegments.Add(Segment);
	}

	FieldRingSegments.Reserve(ArenaFieldRingCount * ArenaFieldRingSegmentCount);
	for (int32 RingIndex = 0; RingIndex < ArenaFieldRingCount; ++RingIndex)
	{
		for (int32 SegmentIndex = 0; SegmentIndex < ArenaFieldRingSegmentCount; ++SegmentIndex)
		{
			UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(
				*FString::Printf(TEXT("FieldRing_%d_%02d"), RingIndex, SegmentIndex));
			Segment->SetupAttachment(SceneRoot);
			if (CubeMesh.Succeeded())
			{
				Segment->SetStaticMesh(CubeMesh.Object);
			}
			if (BasicMaterial.Succeeded())
			{
				Segment->SetMaterial(0, BasicMaterial.Object);
			}
			Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Segment->SetGenerateOverlapEvents(false);
			Segment->SetCastShadow(false);
			Segment->SetCanEverAffectNavigation(false);
			FieldRingSegments.Add(Segment);
		}
	}

	SideLightSegments.Reserve(ArenaSideLightSegmentCount);
	for (int32 SegmentIndex = 0; SegmentIndex < ArenaSideLightSegmentCount; ++SegmentIndex)
	{
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("SideLight_%02d"), SegmentIndex));
		Segment->SetupAttachment(SceneRoot);
		if (CubeMesh.Succeeded())
		{
			Segment->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Segment->SetMaterial(0, BasicMaterial.Object);
		}
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetGenerateOverlapEvents(false);
		Segment->SetCastShadow(false);
		Segment->SetCanEverAffectNavigation(false);
		SideLightSegments.Add(Segment);
	}

	constexpr int32 DirectionMarkerCount = 8;
	DirectionMarkerSegments.Reserve(DirectionMarkerCount);
	for (int32 SegmentIndex = 0; SegmentIndex < DirectionMarkerCount; ++SegmentIndex)
	{
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("DirectionMarker_%02d"), SegmentIndex));
		Segment->SetupAttachment(SceneRoot);
		if (CubeMesh.Succeeded())
		{
			Segment->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Segment->SetMaterial(0, BasicMaterial.Object);
		}
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetGenerateOverlapEvents(false);
		Segment->SetCastShadow(false);
		Segment->SetCanEverAffectNavigation(false);
		DirectionMarkerSegments.Add(Segment);
	}

	const auto CreateVisualCube = [this](const FString& Name)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Component->SetupAttachment(SceneRoot);
		if (CubeMesh.Succeeded())
		{
			Component->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Component->SetMaterial(0, BasicMaterial.Object);
		}
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		return Component;
	};

	for (int32 Index = 0; Index < ArenaSurfaceSeamCount; ++Index)
	{
		UStaticMeshComponent* Component = CreateVisualCube(FString::Printf(TEXT("SurfaceSeam_%02d"), Index));
		Component->SetCastShadow(false);
		SurfaceSeamSegments.Add(Component);
	}
	for (int32 Index = 0; Index < ArenaVenuePylonCount; ++Index)
	{
		VenuePylons.Add(CreateVisualCube(FString::Printf(TEXT("VenuePylon_%02d"), Index)));
		UStaticMeshComponent* LightBar = CreateVisualCube(FString::Printf(TEXT("VenueLightBar_%02d"), Index));
		LightBar->SetCastShadow(false);
		VenueLightBars.Add(LightBar);
	}
	for (int32 Index = 0; Index < ArenaVenueBannerCount; ++Index)
	{
		VenueBannerPanels.Add(CreateVisualCube(FString::Printf(TEXT("VenueBanner_%02d"), Index)));
	}
	for (int32 Index = 0; Index < ArenaFloorGridCount; ++Index)
	{
		UStaticMeshComponent* Component = CreateVisualCube(FString::Printf(TEXT("FloorGrid_%02d"), Index));
		Component->SetCastShadow(false);
		FloorGridSegments.Add(Component);
	}
	for (int32 Index = 0; Index < MultiplayerTeamArcSegmentsPerSide * 2; ++Index)
	{
		UStaticMeshComponent* Component = CreateVisualCube(FString::Printf(TEXT("MultiplayerTeamArc_%02d"), Index));
		Component->SetCastShadow(false);
		MultiplayerTeamArcSegments.Add(Component);
	}
	const auto CreatePlayerZoneDisc = [this](const FString& Name)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Component->SetupAttachment(SceneRoot);
		if (CylinderMesh.Succeeded())
		{
			Component->SetStaticMesh(CylinderMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Component->SetMaterial(0, BasicMaterial.Object);
		}
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCastShadow(false);
		Component->SetCanEverAffectNavigation(false);
		return Component;
	};
	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		for (int32 PlayerSlot = 0; PlayerSlot < MaximumMultiplayerPlayersPerTeam; ++PlayerSlot)
		{
			const int32 ZoneIndex = TeamIndex * MaximumMultiplayerPlayersPerTeam + PlayerSlot;
			MultiplayerPlayerZoneOutlines.Add(CreatePlayerZoneDisc(
				FString::Printf(TEXT("MultiplayerPlayerZoneOutline_%02d"), ZoneIndex)));
			MultiplayerPlayerZoneInsets.Add(CreatePlayerZoneDisc(
				FString::Printf(TEXT("MultiplayerPlayerZoneInset_%02d"), ZoneIndex)));
		}
	}
	if (CubeMesh.Succeeded())
	{
		BackdropMesh->SetStaticMesh(CubeMesh.Object);
		VenueBackWallMesh->SetStaticMesh(CubeMesh.Object);
		CenterLineMesh->SetStaticMesh(CubeMesh.Object);
		CrossLineMesh->SetStaticMesh(CubeMesh.Object);
		Player1HomeMarkMesh->SetStaticMesh(CubeMesh.Object);
		Player2HomeMarkMesh->SetStaticMesh(CubeMesh.Object);
	}

	const TArray<UStaticMeshComponent*> VisualMeshes = {
		ArenaMesh,
		TopSurfaceMesh,
		InnerFieldMesh,
		RimAccentMesh,
		CenterPlateMesh,
		PedestalMesh,
		BackdropMesh,
		StageBaseMesh,
		VenueBackWallMesh,
		CenterLineMesh,
		CrossLineMesh,
		CenterMarkMesh,
		Player1HomeMarkMesh,
		Player2HomeMarkMesh
	};
	for (UStaticMeshComponent* Mesh : VisualMeshes)
	{
		if (BasicMaterial.Succeeded())
		{
			Mesh->SetMaterial(0, BasicMaterial.Object);
		}
		Mesh->SetCanEverAffectNavigation(false);
	}

	ArenaMesh->SetSimulatePhysics(false);
	ArenaMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ArenaMesh->SetCollisionObjectType(ECC_WorldStatic);
	ArenaMesh->SetCollisionResponseToAllChannels(ECR_Block);

	for (UStaticMeshComponent* Mesh : {
		TopSurfaceMesh.Get(),
		InnerFieldMesh.Get(),
		RimAccentMesh.Get(),
		CenterPlateMesh.Get(),
		PedestalMesh.Get(),
		BackdropMesh.Get(),
		StageBaseMesh.Get(),
		VenueBackWallMesh.Get(),
		CenterLineMesh.Get(),
		CrossLineMesh.Get(),
		CenterMarkMesh.Get(),
		Player1HomeMarkMesh.Get(),
		Player2HomeMarkMesh.Get()})
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(false);
	}

	BackdropMesh->SetCastShadow(false);
	StageBaseMesh->SetCastShadow(true);
	VenueBackWallMesh->SetCastShadow(false);
	CenterLineMesh->SetCastShadow(false);
	CrossLineMesh->SetCastShadow(false);
	CenterMarkMesh->SetCastShadow(false);
	Player1HomeMarkMesh->SetCastShadow(false);
	Player2HomeMarkMesh->SetCastShadow(false);
}

void AFlickArena::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickArena, ArenaRadius);
	DOREPLIFETIME(AFlickArena, ArenaThickness);
	DOREPLIFETIME(AFlickArena, SurfaceZ);
	DOREPLIFETIME(AFlickArena, SurfaceFriction);
	DOREPLIFETIME(AFlickArena, SurfaceRestitution);
	DOREPLIFETIME(AFlickArena, PlayersPerTeam);
	DOREPLIFETIME(AFlickArena, ConfigurationRevision);
}

void AFlickArena::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyArenaShape();
}

void AFlickArena::BeginPlay()
{
	Super::BeginPlay();
	ApplyArenaShape();
	ApplyPhysicsMaterial();
}

void AFlickArena::InitializeArena(
	const float InRadius,
	const float InThickness,
	const float InSurfaceZ,
	const int32 InPlayersPerTeam)
{
	ArenaRadius = InRadius;
	ArenaThickness = InThickness;
	SurfaceZ = InSurfaceZ;
	PlayersPerTeam = FMath::Clamp(InPlayersPerTeam, 1, MaximumMultiplayerPlayersPerTeam);
	++ConfigurationRevision;
	ApplyArenaShape();
	ApplyPhysicsMaterial();
	ForceNetUpdate();
}

void AFlickArena::OnRep_ArenaConfiguration()
{
	ApplyArenaShape();
	ApplyPhysicsMaterial();
}

void AFlickArena::ApplyArenaShape()
{
	if (!ArenaMesh)
	{
		return;
	}

	const float ArenaBottomZ = SurfaceZ - ArenaThickness;
	const float PedestalHeight = FMath::Max(80.0f, ArenaBottomZ);
	const float ArenaScale = ArenaRadius / 650.0f;
	const float VenueScale = FMath::Lerp(1.0f, FMath::Max(1.0f, ArenaScale), 0.68f);

	ArenaMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ - ArenaThickness * 0.5f));
	ArenaMesh->SetWorldScale3D(FVector(ArenaRadius / 50.0f, ArenaRadius / 50.0f, ArenaThickness / 100.0f));
	TopSurfaceMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ - 1.0f));
	TopSurfaceMesh->SetWorldScale3D(FVector(ArenaRadius * 0.985f / 50.0f, ArenaRadius * 0.985f / 50.0f, 0.03f));
	RimAccentMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ - 3.0f));
	RimAccentMesh->SetWorldScale3D(FVector(ArenaRadius * 1.012f / 50.0f, ArenaRadius * 1.012f / 50.0f, 0.065f));
	InnerFieldMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 0.05f));
	InnerFieldMesh->SetWorldScale3D(FVector(ArenaRadius * 0.73f / 50.0f, ArenaRadius * 0.73f / 50.0f, 0.012f));
	CenterPlateMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 0.25f));
	CenterPlateMesh->SetWorldScale3D(FVector(ArenaRadius * 0.14f / 50.0f, ArenaRadius * 0.14f / 50.0f, 0.012f));
	PedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, ArenaBottomZ - PedestalHeight * 0.5f));
	PedestalMesh->SetWorldScale3D(FVector(ArenaRadius * 0.42f / 50.0f, ArenaRadius * 0.42f / 50.0f, PedestalHeight / 100.0f));
	StageBaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, ArenaBottomZ - 18.0f));
	StageBaseMesh->SetWorldScale3D(FVector(ArenaRadius * 0.88f / 50.0f, ArenaRadius * 0.88f / 50.0f, 0.36f));
	BackdropMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
	BackdropMesh->SetWorldScale3D(FVector(120.0f, 120.0f, 0.2f));
	VenueBackWallMesh->SetRelativeLocation(FVector(0.0f, 1280.0f * VenueScale, 290.0f * VenueScale));
	VenueBackWallMesh->SetWorldScale3D(FVector(28.0f * VenueScale, 0.36f, 4.3f * VenueScale));

	for (int32 Index = 0; Index < VenuePylons.Num(); ++Index)
	{
		const float Normalized = VenuePylons.Num() > 1
			? static_cast<float>(Index) / static_cast<float>(VenuePylons.Num() - 1)
			: 0.5f;
		const float X = FMath::Lerp(-1120.0f, 1120.0f, Normalized) * VenueScale;
		const float Y = 1238.0f * VenueScale + FMath::Abs(X) * 0.035f;
		VenuePylons[Index]->SetRelativeLocation(FVector(X, Y, 335.0f * VenueScale));
		VenuePylons[Index]->SetWorldScale3D(FVector(0.2f, 0.38f, 6.6f * VenueScale));
		if (VenueLightBars.IsValidIndex(Index) && VenueLightBars[Index])
		{
			VenueLightBars[Index]->SetRelativeLocation(FVector(X, Y - 28.0f, 650.0f * VenueScale));
			VenueLightBars[Index]->SetWorldScale3D(FVector(1.38f, 0.12f, 0.07f));
		}
	}

	for (int32 Index = 0; Index < VenueBannerPanels.Num(); ++Index)
	{
		const float Normalized = VenueBannerPanels.Num() > 1
			? static_cast<float>(Index) / static_cast<float>(VenueBannerPanels.Num() - 1)
			: 0.5f;
		const float X = FMath::Lerp(-980.0f, 980.0f, Normalized) * VenueScale;
		VenueBannerPanels[Index]->SetRelativeLocation(FVector(X, 1190.0f * VenueScale, 350.0f * VenueScale));
		VenueBannerPanels[Index]->SetWorldScale3D(FVector(2.35f, 0.08f, 0.62f));
	}

	for (int32 Index = 0; Index < FloorGridSegments.Num(); ++Index)
	{
		const float Normalized = FloorGridSegments.Num() > 1
			? static_cast<float>(Index) / static_cast<float>(FloorGridSegments.Num() - 1)
			: 0.5f;
		FloorGridSegments[Index]->SetRelativeLocation(FVector(FMath::Lerp(-1450.0f, 1450.0f, Normalized) * VenueScale, 120.0f, 21.0f));
		FloorGridSegments[Index]->SetWorldScale3D(FVector(0.018f, 27.0f * VenueScale, 0.012f));
	}

	CenterLineMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 1.0f));
	CenterLineMesh->SetWorldScale3D(FVector(ArenaRadius * 1.54f / 100.0f, 0.022f, 0.009f));
	CrossLineMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 1.0f));
	CrossLineMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	CrossLineMesh->SetWorldScale3D(FVector(ArenaRadius * 1.54f / 100.0f, 0.022f, 0.009f));
	CenterMarkMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 1.2f));
	CenterMarkMesh->SetWorldScale3D(FVector(0.56f, 0.56f, 0.015f));
	const float HomeMarkY = ArenaRadius * 0.78f;
	const float HomeMarkHalfLength = FMath::Clamp(ArenaRadius * 0.17f, 85.0f, 115.0f);
	Player1HomeMarkMesh->SetRelativeLocation(FVector(0.0f, -HomeMarkY, SurfaceZ + 1.15f));
	Player1HomeMarkMesh->SetWorldScale3D(FVector(HomeMarkHalfLength * 2.0f / 100.0f, 0.075f, 0.012f));
	Player2HomeMarkMesh->SetRelativeLocation(FVector(0.0f, HomeMarkY, SurfaceZ + 1.15f));
	Player2HomeMarkMesh->SetWorldScale3D(FVector(HomeMarkHalfLength * 2.0f / 100.0f, 0.075f, 0.012f));
	CenterLineMesh->SetVisibility(false);
	CrossLineMesh->SetVisibility(false);
	Player1HomeMarkMesh->SetVisibility(false);
	Player2HomeMarkMesh->SetVisibility(false);

	const float SegmentRadius = ArenaRadius * 1.015f;
	const float SegmentLength = 2.0f * PI * SegmentRadius / FMath::Max(1, RimSegments.Num()) * 0.72f;
	for (int32 Index = 0; Index < RimSegments.Num(); ++Index)
	{
		UStaticMeshComponent* Segment = RimSegments[Index];
		if (!Segment)
		{
			continue;
		}
		const float Angle = 2.0f * PI * static_cast<float>(Index) / FMath::Max(1, RimSegments.Num());
		Segment->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * SegmentRadius,
			FMath::Sin(Angle) * SegmentRadius,
			SurfaceZ + 1.7f));
		Segment->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		Segment->SetWorldScale3D(FVector(SegmentLength / 100.0f, 0.045f, 0.035f));
	}

	const float RingFractions[] = {0.2f, 0.35f, 0.5f, 0.65f, 0.8f};
	for (int32 Index = 0; Index < FieldRingSegments.Num(); ++Index)
	{
		UStaticMeshComponent* Segment = FieldRingSegments[Index];
		if (!Segment)
		{
			continue;
		}
		const int32 RingIndex = Index / ArenaFieldRingSegmentCount;
		const int32 SegmentIndex = Index % ArenaFieldRingSegmentCount;
		const float Radius = ArenaRadius * RingFractions[FMath::Clamp(RingIndex, 0, UE_ARRAY_COUNT(RingFractions) - 1)];
		const float Angle = 2.0f * PI * static_cast<float>(SegmentIndex) / ArenaFieldRingSegmentCount;
		const float Length = 2.0f * PI * Radius / ArenaFieldRingSegmentCount * 0.76f;
		Segment->SetRelativeLocation(FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SurfaceZ + 1.05f));
		Segment->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		Segment->SetWorldScale3D(FVector(Length / 100.0f, 0.018f, 0.007f));
	}

	const bool bMultiplayerArena = PlayersPerTeam > 1;
	const float TeamArcRadius = ArenaRadius * 0.735f;
	const float TeamArcSpanDegrees = 138.0f;
	const float TeamArcLength = TeamArcRadius
		* FMath::DegreesToRadians(TeamArcSpanDegrees)
		/ static_cast<float>(MultiplayerTeamArcSegmentsPerSide - 1)
		* 0.76f;
	for (int32 Index = 0; Index < MultiplayerTeamArcSegments.Num(); ++Index)
	{
		UStaticMeshComponent* Segment = MultiplayerTeamArcSegments[Index];
		if (!Segment)
		{
			continue;
		}
		const int32 TeamIndex = Index / MultiplayerTeamArcSegmentsPerSide;
		const int32 SegmentIndex = Index % MultiplayerTeamArcSegmentsPerSide;
		const float SegmentAlpha = static_cast<float>(SegmentIndex)
			/ static_cast<float>(MultiplayerTeamArcSegmentsPerSide - 1);
		const float CenterAngle = TeamIndex == 0 ? -90.0f : 90.0f;
		const float AngleDegrees = CenterAngle
			+ FMath::Lerp(-TeamArcSpanDegrees * 0.5f, TeamArcSpanDegrees * 0.5f, SegmentAlpha);
		const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
		Segment->SetRelativeLocation(FVector(
			FMath::Cos(AngleRadians) * TeamArcRadius,
			FMath::Sin(AngleRadians) * TeamArcRadius,
			SurfaceZ + 0.92f));
		Segment->SetRelativeRotation(FRotator(0.0f, AngleDegrees + 90.0f, 0.0f));
		Segment->SetWorldScale3D(FVector(TeamArcLength / 100.0f, 0.072f, 0.008f));
		Segment->SetVisibility(bMultiplayerArena);
	}

	const int32 ColumnsPerRow = PlayersPerTeam * 2;
	const float PlayerZoneRadius = ArenaRadius * (PlayersPerTeam >= 3 ? 0.15f : 0.16f);
	const float PlayerZoneDistance = ArenaRadius * 0.45f;
	for (int32 TeamIndex = 0; TeamIndex < 2; ++TeamIndex)
	{
		for (int32 PlayerSlot = 0; PlayerSlot < MaximumMultiplayerPlayersPerTeam; ++PlayerSlot)
		{
			const int32 ZoneIndex = TeamIndex * MaximumMultiplayerPlayersPerTeam + PlayerSlot;
			if (!MultiplayerPlayerZoneOutlines.IsValidIndex(ZoneIndex)
				|| !MultiplayerPlayerZoneInsets.IsValidIndex(ZoneIndex))
			{
				continue;
			}
			const bool bZoneVisible = bMultiplayerArena && PlayerSlot < PlayersPerTeam;
			const int32 FirstColumn = PlayerSlot * 2;
			const float FirstColumnAlpha = ColumnsPerRow > 1
				? static_cast<float>(FirstColumn) / static_cast<float>(ColumnsPerRow - 1)
				: 0.5f;
			const float SecondColumnAlpha = ColumnsPerRow > 1
				? static_cast<float>(FirstColumn + 1) / static_cast<float>(ColumnsPerRow - 1)
				: 0.5f;
			const float ZoneAngle = FMath::DegreesToRadians((
				FMath::Lerp(-60.0f, 60.0f, FirstColumnAlpha)
				+ FMath::Lerp(-60.0f, 60.0f, SecondColumnAlpha)) * 0.5f);
			const float TeamDirection = TeamIndex == 0 ? -1.0f : 1.0f;
			const FVector ZoneLocation(
				FMath::Sin(ZoneAngle) * PlayerZoneDistance,
				FMath::Cos(ZoneAngle) * PlayerZoneDistance * TeamDirection,
				SurfaceZ + 0.54f);
			UStaticMeshComponent* Outline = MultiplayerPlayerZoneOutlines[ZoneIndex];
			UStaticMeshComponent* Inset = MultiplayerPlayerZoneInsets[ZoneIndex];
			Outline->SetRelativeLocation(ZoneLocation);
			Outline->SetWorldScale3D(FVector(PlayerZoneRadius / 50.0f, PlayerZoneRadius / 50.0f, 0.008f));
			Outline->SetVisibility(bZoneVisible);
			Inset->SetRelativeLocation(ZoneLocation + FVector(0.0f, 0.0f, 0.36f));
			Inset->SetWorldScale3D(FVector(PlayerZoneRadius * 0.87f / 50.0f, PlayerZoneRadius * 0.87f / 50.0f, 0.008f));
			Inset->SetVisibility(bZoneVisible);
		}
	}

	for (int32 Index = 0; Index < SurfaceSeamSegments.Num(); ++Index)
	{
		UStaticMeshComponent* Segment = SurfaceSeamSegments[Index];
		if (!Segment)
		{
			continue;
		}
		const float Angle = 2.0f * PI * static_cast<float>(Index) / FMath::Max(1, SurfaceSeamSegments.Num());
		const float InnerRadius = ArenaRadius * 0.16f;
		const float OuterRadius = ArenaRadius * 0.965f;
		const float MidRadius = (InnerRadius + OuterRadius) * 0.5f;
		Segment->SetRelativeLocation(FVector(FMath::Cos(Angle) * MidRadius, FMath::Sin(Angle) * MidRadius, SurfaceZ + 0.72f));
		Segment->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle), 0.0f));
		Segment->SetWorldScale3D(FVector((OuterRadius - InnerRadius) / 100.0f, 0.012f, 0.005f));
	}

	const float SideRadius = ArenaRadius * 1.016f;
	const float SideLength = 2.0f * PI * SideRadius / FMath::Max(1, SideLightSegments.Num()) * 0.62f;
	for (int32 Index = 0; Index < SideLightSegments.Num(); ++Index)
	{
		UStaticMeshComponent* Segment = SideLightSegments[Index];
		if (!Segment)
		{
			continue;
		}
		const float Angle = 2.0f * PI * static_cast<float>(Index) / FMath::Max(1, SideLightSegments.Num());
		Segment->SetRelativeLocation(FVector(FMath::Cos(Angle) * SideRadius, FMath::Sin(Angle) * SideRadius, SurfaceZ - ArenaThickness * 0.48f));
		Segment->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		Segment->SetWorldScale3D(FVector(SideLength / 100.0f, 0.04f, ArenaThickness * 0.22f / 100.0f));
	}

	for (int32 DirectionIndex = 0; DirectionIndex < 4; ++DirectionIndex)
	{
		const float Angle = PI * 0.5f * static_cast<float>(DirectionIndex);
		const FVector2D Radial(FMath::Cos(Angle), FMath::Sin(Angle));
		const FVector2D Tangent(-Radial.Y, Radial.X);
		const FVector2D Tip = Radial * ArenaRadius * 0.77f;
		for (int32 SideIndex = 0; SideIndex < 2; ++SideIndex)
		{
			const int32 SegmentIndex = DirectionIndex * 2 + SideIndex;
			if (!DirectionMarkerSegments.IsValidIndex(SegmentIndex) || !DirectionMarkerSegments[SegmentIndex])
			{
				continue;
			}
			const float TangentSide = SideIndex == 0 ? -1.0f : 1.0f;
			const FVector2D Tail = Radial * ArenaRadius * 0.855f + Tangent * 24.0f * TangentSide;
			const FVector2D Delta = Tip - Tail;
			UStaticMeshComponent* Segment = DirectionMarkerSegments[SegmentIndex];
			Segment->SetRelativeLocation(FVector((Tip + Tail) * 0.5f, SurfaceZ + 1.35f));
			Segment->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.0f));
		Segment->SetWorldScale3D(FVector(Delta.Size() / 100.0f, 0.05f, 0.008f));
		}
	}

	if (!BodyMaterial)
	{
		BodyMaterial = ArenaMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!TopMaterial)
	{
		TopMaterial = TopSurfaceMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!InnerFieldMaterial)
	{
		InnerFieldMaterial = InnerFieldMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!RimAccentMaterial)
	{
		RimAccentMaterial = RimAccentMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!CenterPlateMaterial)
	{
		CenterPlateMaterial = CenterPlateMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!PedestalMaterial)
	{
		PedestalMaterial = PedestalMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!BackdropMaterial)
	{
		BackdropMaterial = BackdropMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!StageBaseMaterial)
	{
		StageBaseMaterial = StageBaseMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!VenueBackWallMaterial)
	{
		VenueBackWallMaterial = VenueBackWallMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!NeutralMarkMaterial)
	{
		NeutralMarkMaterial = CenterLineMesh->CreateAndSetMaterialInstanceDynamic(0);
		CrossLineMesh->SetMaterial(0, NeutralMarkMaterial);
		CenterMarkMesh->SetMaterial(0, NeutralMarkMaterial);
	}
	if (!Player1MarkMaterial)
	{
		Player1MarkMaterial = Player1HomeMarkMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!Player2MarkMaterial)
	{
		Player2MarkMaterial = Player2HomeMarkMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!DirectionMarkerMaterial && !DirectionMarkerSegments.IsEmpty())
	{
		DirectionMarkerMaterial = DirectionMarkerSegments[0]->CreateAndSetMaterialInstanceDynamic(0);
		for (int32 Index = 1; Index < DirectionMarkerSegments.Num(); ++Index)
		{
			DirectionMarkerSegments[Index]->SetMaterial(0, DirectionMarkerMaterial);
		}
	}
	if (!SurfaceSeamMaterial && !SurfaceSeamSegments.IsEmpty())
	{
		SurfaceSeamMaterial = SurfaceSeamSegments[0]->CreateAndSetMaterialInstanceDynamic(0);
		for (int32 Index = 1; Index < SurfaceSeamSegments.Num(); ++Index)
		{
			SurfaceSeamSegments[Index]->SetMaterial(0, SurfaceSeamMaterial);
		}
	}
	if (!VenuePylonMaterial && !VenuePylons.IsEmpty())
	{
		VenuePylonMaterial = VenuePylons[0]->CreateAndSetMaterialInstanceDynamic(0);
		for (int32 Index = 1; Index < VenuePylons.Num(); ++Index)
		{
			VenuePylons[Index]->SetMaterial(0, VenuePylonMaterial);
		}
	}
	if (!FloorGridMaterial && !FloorGridSegments.IsEmpty())
	{
		FloorGridMaterial = FloorGridSegments[0]->CreateAndSetMaterialInstanceDynamic(0);
		for (int32 Index = 1; Index < FloorGridSegments.Num(); ++Index)
		{
			FloorGridSegments[Index]->SetMaterial(0, FloorGridMaterial);
		}
	}

	const auto SetMaterialColor = [](UMaterialInstanceDynamic* Material, const FLinearColor& Color)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.88f);
		}
	};

	SetMaterialColor(BodyMaterial, FLinearColor(0.004f, 0.006f, 0.012f, 1.0f));
	SetMaterialColor(RimAccentMaterial, FLinearColor(0.0f, 0.2f, 0.34f, 1.0f));
	SetMaterialColor(TopMaterial, FLinearColor(0.065f, 0.085f, 0.12f, 1.0f));
	SetMaterialColor(InnerFieldMaterial, FLinearColor(0.078f, 0.104f, 0.145f, 1.0f));
	SetMaterialColor(CenterPlateMaterial, FLinearColor(0.105f, 0.128f, 0.16f, 1.0f));
	SetMaterialColor(PedestalMaterial, FLinearColor(0.012f, 0.016f, 0.026f, 1.0f));
	SetMaterialColor(BackdropMaterial, FLinearColor(0.002f, 0.004f, 0.008f, 1.0f));
	SetMaterialColor(StageBaseMaterial, FLinearColor(0.008f, 0.016f, 0.026f, 1.0f));
	SetMaterialColor(VenueBackWallMaterial, FLinearColor(0.001f, 0.003f, 0.006f, 1.0f));
	SetMaterialColor(NeutralMarkMaterial, FLinearColor(0.14f, 0.17f, 0.2f, 1.0f));
	SetMaterialColor(Player1MarkMaterial, FLinearColor(0.0f, 0.62f, 0.9f, 1.0f));
	SetMaterialColor(Player2MarkMaterial, FLinearColor(1.0f, 0.22f, 0.08f, 1.0f));
	SetMaterialColor(DirectionMarkerMaterial, FLinearColor(0.42f, 0.52f, 0.62f, 1.0f));
	SetMaterialColor(SurfaceSeamMaterial, FLinearColor(0.025f, 0.036f, 0.052f, 1.0f));
	SetMaterialColor(VenuePylonMaterial, FLinearColor(0.008f, 0.018f, 0.028f, 1.0f));
	SetMaterialColor(FloorGridMaterial, FLinearColor(0.004f, 0.026f, 0.042f, 1.0f));

	if (VenueBannerMaterials.Num() != VenueBannerPanels.Num())
	{
		VenueBannerMaterials.Reset();
		for (UStaticMeshComponent* PanelMesh : VenueBannerPanels)
		{
			VenueBannerMaterials.Add(PanelMesh ? PanelMesh->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	for (int32 Index = 0; Index < VenueBannerMaterials.Num(); ++Index)
	{
		const FLinearColor TeamTint = Index % 2 == 0
			? FLinearColor(0.0f, 0.075f, 0.14f, 1.0f)
			: FLinearColor(0.14f, 0.022f, 0.004f, 1.0f);
		SetMaterialColor(VenueBannerMaterials[Index], TeamTint);
	}

	if (VenueLightMaterials.Num() != VenueLightBars.Num())
	{
		VenueLightMaterials.Reset();
		for (UStaticMeshComponent* LightBar : VenueLightBars)
		{
			VenueLightMaterials.Add(LightBar ? LightBar->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	for (int32 Index = 0; Index < VenueLightMaterials.Num(); ++Index)
	{
		const FLinearColor LightColor = Index < VenueLightMaterials.Num() / 2
			? FLinearColor(0.0f, 0.62f, 0.96f, 1.0f)
			: Index == VenueLightMaterials.Num() / 2
				? FLinearColor(0.72f, 0.84f, 0.94f, 1.0f)
				: FLinearColor(1.0f, 0.2f, 0.025f, 1.0f);
		SetMaterialColor(VenueLightMaterials[Index], LightColor);
	}

	if (RimSegmentMaterials.Num() != RimSegments.Num())
	{
		RimSegmentMaterials.Reset();
		for (UStaticMeshComponent* Segment : RimSegments)
		{
			RimSegmentMaterials.Add(Segment ? Segment->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	for (int32 Index = 0; Index < RimSegmentMaterials.Num(); ++Index)
	{
		const FLinearColor SegmentColor = Index % 2 == 0
			? FLinearColor(0.0f, 0.78f, 1.0f, 1.0f)
			: FLinearColor(0.0f, 0.54f, 0.84f, 1.0f);
		SetMaterialColor(RimSegmentMaterials[Index], SegmentColor);
	}

	if (FieldRingMaterials.Num() != FieldRingSegments.Num())
	{
		FieldRingMaterials.Reset();
		for (UStaticMeshComponent* Segment : FieldRingSegments)
		{
			FieldRingMaterials.Add(Segment ? Segment->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	for (int32 Index = 0; Index < FieldRingMaterials.Num(); ++Index)
	{
		const int32 RingIndex = Index / ArenaFieldRingSegmentCount;
		const float Shade = RingIndex % 2 == 0 ? 1.0f : 0.72f;
		SetMaterialColor(FieldRingMaterials[Index], FLinearColor(0.085f, 0.105f, 0.13f, 1.0f) * Shade);
	}

	if (SideLightMaterials.Num() != SideLightSegments.Num())
	{
		SideLightMaterials.Reset();
		for (UStaticMeshComponent* Segment : SideLightSegments)
		{
			SideLightMaterials.Add(Segment ? Segment->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	for (int32 Index = 0; Index < SideLightMaterials.Num(); ++Index)
	{
		const float Angle = 2.0f * PI * static_cast<float>(Index) / FMath::Max(1, SideLightMaterials.Num());
		SetMaterialColor(SideLightMaterials[Index], FMath::Sin(Angle) < 0.0f
			? FLinearColor(0.0f, 0.42f, 0.72f, 1.0f)
			: FLinearColor(0.78f, 0.09f, 0.015f, 1.0f));
	}

	if (MultiplayerTeamArcMaterials.Num() != MultiplayerTeamArcSegments.Num())
	{
		MultiplayerTeamArcMaterials.Reset();
		for (UStaticMeshComponent* Segment : MultiplayerTeamArcSegments)
		{
			MultiplayerTeamArcMaterials.Add(Segment ? Segment->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	for (int32 Index = 0; Index < MultiplayerTeamArcMaterials.Num(); ++Index)
	{
		const bool bBlueTeam = Index < MultiplayerTeamArcSegmentsPerSide;
		SetMaterialColor(MultiplayerTeamArcMaterials[Index], bBlueTeam
			? FLinearColor(0.0f, 0.31f, 0.53f, 1.0f)
			: FLinearColor(0.58f, 0.075f, 0.008f, 1.0f));
	}

	if (MultiplayerPlayerZoneOutlineMaterials.Num() != MultiplayerPlayerZoneOutlines.Num())
	{
		MultiplayerPlayerZoneOutlineMaterials.Reset();
		for (UStaticMeshComponent* Zone : MultiplayerPlayerZoneOutlines)
		{
			MultiplayerPlayerZoneOutlineMaterials.Add(Zone ? Zone->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	if (MultiplayerPlayerZoneInsetMaterials.Num() != MultiplayerPlayerZoneInsets.Num())
	{
		MultiplayerPlayerZoneInsetMaterials.Reset();
		for (UStaticMeshComponent* Zone : MultiplayerPlayerZoneInsets)
		{
			MultiplayerPlayerZoneInsetMaterials.Add(Zone ? Zone->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	const FLinearColor PlayerZoneColors[] = {
		FLinearColor(0.84f, 0.92f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.04f, 0.66f, 1.0f),
		FLinearColor(0.5f, 0.94f, 0.025f, 1.0f)};
	for (int32 Index = 0; Index < MultiplayerPlayerZoneOutlineMaterials.Num(); ++Index)
	{
		const int32 TeamIndex = Index / MaximumMultiplayerPlayersPerTeam;
		const int32 PlayerSlot = Index % MaximumMultiplayerPlayersPerTeam;
		const FLinearColor TeamColor = TeamIndex == 0
			? FLinearColor(0.0f, 0.52f, 0.82f, 1.0f)
			: FLinearColor(0.92f, 0.12f, 0.015f, 1.0f);
		const FLinearColor PlayerColor = PlayerZoneColors[PlayerSlot];
		SetMaterialColor(
			MultiplayerPlayerZoneOutlineMaterials[Index],
			FMath::Lerp(FMath::Lerp(FLinearColor(0.018f, 0.028f, 0.045f, 1.0f), TeamColor, 0.18f), PlayerColor, 0.32f));
		if (MultiplayerPlayerZoneInsetMaterials.IsValidIndex(Index))
		{
			SetMaterialColor(
				MultiplayerPlayerZoneInsetMaterials[Index],
				FMath::Lerp(FMath::Lerp(FLinearColor(0.055f, 0.073f, 0.1f, 1.0f), TeamColor, 0.12f), PlayerColor, 0.045f));
		}
	}
}

void AFlickArena::ApplyPhysicsMaterial()
{
	if (!ArenaMesh)
	{
		return;
	}

	if (!RuntimePhysicalMaterial)
	{
		RuntimePhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("FlickArenaPhysicalMaterial"));
	}

	RuntimePhysicalMaterial->Friction = SurfaceFriction;
	RuntimePhysicalMaterial->Restitution = SurfaceRestitution;
	RuntimePhysicalMaterial->bOverrideFrictionCombineMode = true;
	RuntimePhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
	RuntimePhysicalMaterial->bOverrideRestitutionCombineMode = true;
	RuntimePhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Average;
	ArenaMesh->SetPhysMaterialOverride(RuntimePhysicalMaterial);
}
