#include "Arena/FlickBobArena.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FlickTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 PocketRimSegmentCount = 32;

	void SetMaterialColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		}
	}
}

AFlickBobArena::AFlickBobArena()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	const auto CreateMesh = [this](const FName Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(SceneRoot);
		Component->SetStaticMesh(Mesh);
		if (BasicMaterial.Succeeded())
		{
			Component->SetMaterial(0, BasicMaterial.Object);
		}
		Component->SetCanEverAffectNavigation(false);
		return Component;
	};

	BoardBase = CreateMesh(TEXT("BoardBase"), CubeMesh.Object);
	PlayingSurface = CreateMesh(TEXT("PlayingSurface"), CubeMesh.Object);
	CenterRingOuter = CreateMesh(TEXT("CenterRingOuter"), CylinderMesh.Object);
	CenterRingInner = CreateMesh(TEXT("CenterRingInner"), CylinderMesh.Object);
	Pedestal = CreateMesh(TEXT("Pedestal"), CubeMesh.Object);
	Backdrop = CreateMesh(TEXT("Backdrop"), CubeMesh.Object);
	StageBase = CreateMesh(TEXT("StageBase"), CubeMesh.Object);
	VenueBackWall = CreateMesh(TEXT("VenueBackWall"), CubeMesh.Object);

	for (int32 Index = 0; Index < 4; ++Index)
	{
		Rails.Add(CreateMesh(*FString::Printf(TEXT("Rail_%d"), Index), CubeMesh.Object));
		RailAccentStrips.Add(CreateMesh(*FString::Printf(TEXT("RailAccent_%d"), Index), CubeMesh.Object));
		PocketTrims.Add(CreateMesh(*FString::Printf(TEXT("PocketTrim_%d"), Index), CylinderMesh.Object));
		Pockets.Add(CreateMesh(*FString::Printf(TEXT("Pocket_%d"), Index), CylinderMesh.Object));
		PocketDepths.Add(CreateMesh(*FString::Printf(TEXT("PocketDepth_%d"), Index), CylinderMesh.Object));
		PocketBottoms.Add(CreateMesh(*FString::Printf(TEXT("PocketBottom_%d"), Index), CylinderMesh.Object));
		for (int32 SegmentIndex = 0; SegmentIndex < PocketRimSegmentCount; ++SegmentIndex)
		{
			PocketRimSegments.Add(CreateMesh(
				*FString::Printf(TEXT("PocketRim_%d_%02d"), Index, SegmentIndex),
				CubeMesh.Object));
		}
		CornerCaps.Add(CreateMesh(*FString::Printf(TEXT("CornerCap_%d"), Index), CylinderMesh.Object));
		GuideLines.Add(CreateMesh(*FString::Printf(TEXT("GuideLine_%d"), Index), CubeMesh.Object));
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		StartLines.Add(CreateMesh(*FString::Printf(TEXT("StartLine_%d"), Index), CubeMesh.Object));
	}
	for (int32 Index = 0; Index < 10; ++Index)
	{
		SurfacePanelLines.Add(CreateMesh(*FString::Printf(TEXT("SurfacePanelLine_%02d"), Index), CubeMesh.Object));
	}
	for (int32 Index = 0; Index < 9; ++Index)
	{
		VenuePylons.Add(CreateMesh(*FString::Printf(TEXT("VenuePylon_%02d"), Index), CubeMesh.Object));
		VenueLightBars.Add(CreateMesh(*FString::Printf(TEXT("VenueLightBar_%02d"), Index), CubeMesh.Object));
	}
	for (int32 Index = 0; Index < 8; ++Index)
	{
		VenueBannerPanels.Add(CreateMesh(*FString::Printf(TEXT("VenueBanner_%02d"), Index), CubeMesh.Object));
	}
	for (int32 Index = 0; Index < 14; ++Index)
	{
		FloorGridSegments.Add(CreateMesh(*FString::Printf(TEXT("FloorGrid_%02d"), Index), CubeMesh.Object));
	}

	// A single simple box is the authoritative tabletop. Decorative markings and
	// pocket geometry never participate in collision, so there are no hidden triangle
	// seams or raised visual strips capable of steering a moving puck.
	BoardBase->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoardBase->SetCollisionObjectType(ECC_WorldStatic);
	BoardBase->SetCollisionResponseToAllChannels(ECR_Block);
	BoardBase->SetGenerateOverlapEvents(false);
	for (UStaticMeshComponent* Rail : Rails)
	{
		Rail->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Rail->SetCollisionObjectType(ECC_WorldStatic);
		Rail->SetCollisionResponseToAllChannels(ECR_Block);
	}

	TArray<UStaticMeshComponent*> VisualOnly = {
		PlayingSurface.Get(), CenterRingOuter.Get(), CenterRingInner.Get(), Pedestal.Get(), Backdrop.Get(),
		StageBase.Get(), VenueBackWall.Get()
	};
	for (UStaticMeshComponent* Component : PocketTrims) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : Pockets) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : PocketDepths) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : PocketBottoms) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : PocketRimSegments) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : RailAccentStrips) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : CornerCaps) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : GuideLines) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : StartLines) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : SurfacePanelLines) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : VenuePylons) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : VenueBannerPanels) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : VenueLightBars) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : FloorGridSegments) VisualOnly.Add(Component);
	for (UStaticMeshComponent* Component : VisualOnly)
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
	}
	Backdrop->SetCastShadow(false);
	VenueBackWall->SetCastShadow(false);
	for (UStaticMeshComponent* Component : RailAccentStrips) Component->SetCastShadow(false);
	for (UStaticMeshComponent* Component : SurfacePanelLines) Component->SetCastShadow(false);
	for (UStaticMeshComponent* Component : VenueLightBars) Component->SetCastShadow(false);
	for (UStaticMeshComponent* Component : FloorGridSegments) Component->SetCastShadow(false);
	for (UStaticMeshComponent* Component : GuideLines) Component->SetCastShadow(false);
	for (UStaticMeshComponent* Component : StartLines) Component->SetCastShadow(false);
}

void AFlickBobArena::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickBobArena, BoardHalfExtent);
	DOREPLIFETIME(AFlickBobArena, BoardThickness);
	DOREPLIFETIME(AFlickBobArena, SurfaceZ);
	DOREPLIFETIME(AFlickBobArena, PocketRadius);
	DOREPLIFETIME(AFlickBobArena, PocketInset);
	DOREPLIFETIME(AFlickBobArena, PocketCaptureDepth);
	DOREPLIFETIME(AFlickBobArena, PocketCaptureRadiusScale);
	DOREPLIFETIME(AFlickBobArena, PocketVisualDepth);
	DOREPLIFETIME(AFlickBobArena, RailHeight);
	DOREPLIFETIME(AFlickBobArena, RailThickness);
	DOREPLIFETIME(AFlickBobArena, ConfigurationRevision);
}

void AFlickBobArena::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyArenaShape();
}

void AFlickBobArena::BeginPlay()
{
	Super::BeginPlay();
	ApplyArenaShape();
	ApplyPhysicsMaterials();
}

void AFlickBobArena::InitializeArena(
	const float InHalfExtent,
	const float InThickness,
	const float InSurfaceZ)
{
	BoardHalfExtent = FMath::Max(300.0f, InHalfExtent);
	BoardThickness = FMath::Max(20.0f, InThickness);
	SurfaceZ = InSurfaceZ;
	++ConfigurationRevision;
	ApplyArenaShape();
	ApplyPhysicsMaterials();
	ForceNetUpdate();
}

void AFlickBobArena::OnRep_ArenaConfiguration()
{
	ApplyArenaShape();
	ApplyPhysicsMaterials();
}

bool AFlickBobArena::IsInsidePocket(const FVector& WorldLocation) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const float PocketOffset = BoardHalfExtent - PocketInset;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector2D Center(
			(Index % 2 == 0 ? -1.0f : 1.0f) * PocketOffset,
			(Index < 2 ? -1.0f : 1.0f) * PocketOffset);
		if (FVector2D::Distance(FVector2D(LocalLocation.X, LocalLocation.Y), Center) <= PocketRadius)
		{
			return true;
		}
	}
	return false;
}

bool AFlickBobArena::IsCapturedByPocket(const FVector& WorldLocation, const float PieceRadius) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const float SafePieceRadius = FMath::Max(0.0f, PieceRadius);
	const float VerticalTolerance = FMath::Max(PocketCaptureDepth, SafePieceRadius * 0.4f);
	if (LocalLocation.Z > SurfaceZ + VerticalTolerance
		|| LocalLocation.Z < SurfaceZ - BoardThickness)
	{
		return false;
	}

	const float PocketOffset = BoardHalfExtent - PocketInset;
	const float CaptureRadius = FMath::Max(
		4.0f,
		PocketRadius - SafePieceRadius * PocketCaptureRadiusScale);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector2D Center(
			(Index % 2 == 0 ? -1.0f : 1.0f) * PocketOffset,
			(Index < 2 ? -1.0f : 1.0f) * PocketOffset);
		if (FVector2D::Distance(FVector2D(LocalLocation.X, LocalLocation.Y), Center) <= CaptureRadius)
		{
			return true;
		}
	}
	return false;
}

bool AFlickBobArena::IsSafeForTabletopSelfRighting(
	const FVector& WorldLocation,
	const float PieceRadius) const
{
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const float SafeRadius = FMath::Max(0.0f, PieceRadius);
	if (LocalLocation.Z < SurfaceZ - PocketCaptureDepth
		|| FMath::Abs(LocalLocation.X) > BoardHalfExtent - SafeRadius
		|| FMath::Abs(LocalLocation.Y) > BoardHalfExtent - SafeRadius)
	{
		return false;
	}

	const float PocketOffset = BoardHalfExtent - PocketInset;
	const float PocketInfluenceRadius = PocketRadius + SafeRadius + 6.0f;
	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector2D PocketCenter(
			(Index % 2 == 0 ? -1.0f : 1.0f) * PocketOffset,
			(Index < 2 ? -1.0f : 1.0f) * PocketOffset);
		if (FVector2D::Distance(FVector2D(LocalLocation.X, LocalLocation.Y), PocketCenter)
			<= PocketInfluenceRadius)
		{
			return false;
		}
	}
	return true;
}

FVector AFlickBobArena::GetPocketWorldLocation(const int32 PocketIndex) const
{
	const int32 SafeIndex = FMath::Clamp(PocketIndex, 0, 3);
	const float PocketOffset = BoardHalfExtent - PocketInset;
	return GetActorTransform().TransformPosition(FVector(
		(SafeIndex % 2 == 0 ? -1.0f : 1.0f) * PocketOffset,
		(SafeIndex < 2 ? -1.0f : 1.0f) * PocketOffset,
		SurfaceZ + 2.0f));
}

FVector AFlickBobArena::GetStrikerStart(const EFlickTeam Team, const float PieceThickness) const
{
	const float Side = Team == EFlickTeam::Player2 ? 1.0f : -1.0f;
	const float Baseline = BoardHalfExtent - 158.0f;
	return GetActorTransform().TransformPosition(FVector(
		0.0f,
		Side * Baseline,
		SurfaceZ + PieceThickness * 0.5f + 3.0f));
}

void AFlickBobArena::ApplyArenaShape()
{
	if (!BoardBase || Rails.Num() != 4 || Pockets.Num() != 4)
	{
		return;
	}

	const float BottomZ = SurfaceZ - BoardThickness;
	const float PedestalHeight = FMath::Max(80.0f, BottomZ);
	BoardBase->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ - BoardThickness * 0.5f));
	BoardBase->SetRelativeScale3D(FVector(BoardHalfExtent / 50.0f, BoardHalfExtent / 50.0f, BoardThickness / 100.0f));
	PlayingSurface->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 0.8f));
	PlayingSurface->SetRelativeScale3D(FVector((BoardHalfExtent - 18.0f) / 50.0f, (BoardHalfExtent - 18.0f) / 50.0f, 0.018f));
	const float VisibleSurfaceTop = SurfaceZ + 1.7f;
	Pedestal->SetRelativeLocation(FVector(0.0f, 0.0f, BottomZ - PedestalHeight * 0.5f));
	Pedestal->SetRelativeScale3D(FVector(BoardHalfExtent * 0.42f / 50.0f, BoardHalfExtent * 0.42f / 50.0f, PedestalHeight / 100.0f));
	StageBase->SetRelativeLocation(FVector(0.0f, 0.0f, BottomZ - 18.0f));
	StageBase->SetRelativeScale3D(FVector(BoardHalfExtent * 1.08f / 50.0f, BoardHalfExtent * 1.08f / 50.0f, 0.36f));
	Backdrop->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
	Backdrop->SetRelativeScale3D(FVector(120.0f, 120.0f, 0.2f));
	VenueBackWall->SetRelativeLocation(FVector(0.0f, 1280.0f, 290.0f));
	VenueBackWall->SetRelativeScale3D(FVector(28.0f, 0.36f, 4.3f));

	for (int32 Index = 0; Index < VenuePylons.Num(); ++Index)
	{
		const float Normalized = VenuePylons.Num() > 1
			? static_cast<float>(Index) / static_cast<float>(VenuePylons.Num() - 1)
			: 0.5f;
		const float X = FMath::Lerp(-1120.0f, 1120.0f, Normalized);
		const float Y = 1238.0f + FMath::Abs(X) * 0.035f;
		VenuePylons[Index]->SetRelativeLocation(FVector(X, Y, 335.0f));
		VenuePylons[Index]->SetRelativeScale3D(FVector(0.2f, 0.38f, 6.6f));
		if (VenueLightBars.IsValidIndex(Index) && VenueLightBars[Index])
		{
			VenueLightBars[Index]->SetRelativeLocation(FVector(X, Y - 28.0f, 650.0f));
			VenueLightBars[Index]->SetRelativeScale3D(FVector(1.38f, 0.12f, 0.07f));
		}
	}
	for (int32 Index = 0; Index < VenueBannerPanels.Num(); ++Index)
	{
		const float Normalized = VenueBannerPanels.Num() > 1
			? static_cast<float>(Index) / static_cast<float>(VenueBannerPanels.Num() - 1)
			: 0.5f;
		VenueBannerPanels[Index]->SetRelativeLocation(FVector(FMath::Lerp(-980.0f, 980.0f, Normalized), 1190.0f, 350.0f));
		VenueBannerPanels[Index]->SetRelativeScale3D(FVector(2.35f, 0.08f, 0.62f));
	}
	for (int32 Index = 0; Index < FloorGridSegments.Num(); ++Index)
	{
		const float Normalized = FloorGridSegments.Num() > 1
			? static_cast<float>(Index) / static_cast<float>(FloorGridSegments.Num() - 1)
			: 0.5f;
		FloorGridSegments[Index]->SetRelativeLocation(FVector(FMath::Lerp(-1450.0f, 1450.0f, Normalized), 120.0f, 21.0f));
		FloorGridSegments[Index]->SetRelativeScale3D(FVector(0.018f, 27.0f, 0.012f));
	}

	const float RailOffset = BoardHalfExtent + RailThickness * 0.5f - 5.0f;
	for (int32 Index = 0; Index < Rails.Num(); ++Index)
	{
		const bool bVertical = Index >= 2;
		const float Side = Index % 2 == 0 ? -1.0f : 1.0f;
		Rails[Index]->SetRelativeLocation(bVertical
			? FVector(Side * RailOffset, 0.0f, SurfaceZ + RailHeight * 0.5f)
			: FVector(0.0f, Side * RailOffset, SurfaceZ + RailHeight * 0.5f));
		Rails[Index]->SetRelativeScale3D(bVertical
			? FVector(RailThickness / 100.0f, (BoardHalfExtent * 2.0f + RailThickness * 2.0f) / 100.0f, RailHeight / 100.0f)
			: FVector((BoardHalfExtent * 2.0f + RailThickness * 2.0f) / 100.0f, RailThickness / 100.0f, RailHeight / 100.0f));
		if (RailAccentStrips.IsValidIndex(Index) && RailAccentStrips[Index])
		{
			RailAccentStrips[Index]->SetRelativeLocation(bVertical
				? FVector(Side * (RailOffset - RailThickness * 0.46f), 0.0f, SurfaceZ + RailHeight - 3.0f)
				: FVector(0.0f, Side * (RailOffset - RailThickness * 0.46f), SurfaceZ + RailHeight - 3.0f));
			RailAccentStrips[Index]->SetRelativeScale3D(bVertical
				? FVector(0.055f, BoardHalfExtent * 1.94f / 50.0f, 0.045f)
				: FVector(BoardHalfExtent * 1.94f / 50.0f, 0.055f, 0.045f));
		}
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		const FVector PocketLocation = GetActorTransform().InverseTransformPosition(GetPocketWorldLocation(Index));
		PocketTrims[Index]->SetRelativeLocation(PocketLocation + FVector(0.0f, 0.0f, 0.2f));
		PocketTrims[Index]->SetRelativeScale3D(FVector((PocketRadius + 12.0f) / 50.0f, (PocketRadius + 12.0f) / 50.0f, 0.018f));
		Pockets[Index]->SetRelativeLocation(PocketLocation + FVector(0.0f, 0.0f, 0.55f));
		Pockets[Index]->SetRelativeScale3D(FVector((PocketRadius + 6.0f) / 50.0f, (PocketRadius + 6.0f) / 50.0f, 0.028f));
		PocketDepths[Index]->SetRelativeLocation(PocketLocation + FVector(0.0f, 0.0f, 1.1f));
		PocketDepths[Index]->SetRelativeScale3D(FVector(
			(PocketRadius - 4.0f) / 50.0f,
			(PocketRadius - 4.0f) / 50.0f,
			0.024f));
		const float DepthAlpha = FMath::GetMappedRangeValueClamped(
			FVector2D(4.0f, 60.0f),
			FVector2D(0.0f, 1.0f),
			PocketVisualDepth);
		const float BottomScale = FMath::Lerp(0.68f, 0.5f, DepthAlpha);
		PocketBottoms[Index]->SetRelativeLocation(PocketLocation + FVector(0.0f, FMath::Lerp(-4.0f, -10.0f, DepthAlpha), 1.45f));
		PocketBottoms[Index]->SetRelativeScale3D(FVector(
			PocketRadius * BottomScale / 50.0f,
			PocketRadius * BottomScale * 0.78f / 50.0f,
			0.012f));

		const float RimRadius = PocketRadius + 8.5f;
		const float RimSegmentLength = 2.0f * PI * RimRadius / PocketRimSegmentCount * 0.96f;
		for (int32 SegmentIndex = 0; SegmentIndex < PocketRimSegmentCount; ++SegmentIndex)
		{
			const int32 FlatIndex = Index * PocketRimSegmentCount + SegmentIndex;
			if (!PocketRimSegments.IsValidIndex(FlatIndex) || !PocketRimSegments[FlatIndex])
			{
				continue;
			}
			const float Angle = 2.0f * PI * static_cast<float>(SegmentIndex) / PocketRimSegmentCount;
			const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			UStaticMeshComponent* RimSegment = PocketRimSegments[FlatIndex];
			RimSegment->SetRelativeLocation(
				PocketLocation + Radial * RimRadius + FVector(0.0f, 0.0f, 2.05f));
			RimSegment->SetRelativeRotation(FRotator(
				0.0f,
				FMath::RadiansToDegrees(Angle) + 90.0f,
				0.0f));
			RimSegment->SetRelativeScale3D(FVector(RimSegmentLength / 100.0f, 0.024f, 0.014f));
		}
		const float CapX = (Index % 2 == 0 ? -1.0f : 1.0f) * (BoardHalfExtent + RailThickness * 0.48f);
		const float CapY = (Index < 2 ? -1.0f : 1.0f) * (BoardHalfExtent + RailThickness * 0.48f);
		CornerCaps[Index]->SetRelativeLocation(FVector(CapX, CapY, SurfaceZ + RailHeight + 1.0f));
		CornerCaps[Index]->SetRelativeScale3D(FVector(0.56f, 0.56f, 0.08f));
	}

	for (int32 Index = 0; Index < SurfacePanelLines.Num(); ++Index)
	{
		const bool bVertical = Index < 5;
		const int32 LaneIndex = Index % 5;
		const float Offset = FMath::Lerp(-BoardHalfExtent * 0.68f, BoardHalfExtent * 0.68f, static_cast<float>(LaneIndex) / 4.0f);
		SurfacePanelLines[Index]->SetRelativeLocation(bVertical
			? FVector(Offset, 0.0f, VisibleSurfaceTop - 0.18f)
			: FVector(0.0f, Offset, VisibleSurfaceTop - 0.18f));
		SurfacePanelLines[Index]->SetRelativeScale3D(bVertical
			? FVector(0.009f, BoardHalfExtent * 1.76f / 50.0f, 0.004f)
			: FVector(BoardHalfExtent * 1.76f / 50.0f, 0.009f, 0.004f));
	}

	const float GuideOffset = BoardHalfExtent - 108.0f;
	const float GuideLength = (BoardHalfExtent - 126.0f) * 2.0f;
	for (int32 Index = 0; Index < GuideLines.Num(); ++Index)
	{
		const bool bVertical = Index >= 2;
		const float Side = Index % 2 == 0 ? -1.0f : 1.0f;
		GuideLines[Index]->SetRelativeLocation(bVertical
			? FVector(Side * GuideOffset, 0.0f, VisibleSurfaceTop - 0.58f)
			: FVector(0.0f, Side * GuideOffset, VisibleSurfaceTop - 0.58f));
		GuideLines[Index]->SetRelativeScale3D(bVertical
			? FVector(0.025f, GuideLength / 100.0f, 0.012f)
			: FVector(GuideLength / 100.0f, 0.025f, 0.012f));
	}

	const float StartY = BoardHalfExtent - 158.0f;
	for (int32 Index = 0; Index < StartLines.Num(); ++Index)
	{
		StartLines[Index]->SetRelativeLocation(FVector(
			0.0f,
			Index == 0 ? -StartY : StartY,
			VisibleSurfaceTop - 0.68f));
		StartLines[Index]->SetRelativeScale3D(FVector(3.5f, 0.045f, 0.014f));
	}

	CenterRingOuter->SetRelativeLocation(FVector(0.0f, 0.0f, VisibleSurfaceTop - 0.88f));
	CenterRingOuter->SetRelativeScale3D(FVector(3.9f, 3.9f, 0.018f));
	CenterRingInner->SetRelativeLocation(FVector(0.0f, 0.0f, VisibleSurfaceTop - 0.93f));
	CenterRingInner->SetRelativeScale3D(FVector(3.72f, 3.72f, 0.019f));

	RuntimeMaterials.Reset();
	const auto ColorComponent = [this](UStaticMeshComponent* Component, const FLinearColor& Color)
	{
		UMaterialInstanceDynamic* Material = Component ? Component->CreateAndSetMaterialInstanceDynamic(0) : nullptr;
		RuntimeMaterials.Add(Material);
		SetMaterialColor(Material, Color);
	};
	const FLinearColor RailMetal(0.055f, 0.07f, 0.088f, 1.0f);
	const FLinearColor BoardEdge(0.012f, 0.024f, 0.036f, 1.0f);
	const FLinearColor Surface(0.062f, 0.084f, 0.122f, 1.0f);
	const FLinearColor Marking(0.42f, 0.085f, 0.012f, 1.0f);
	ColorComponent(BoardBase, BoardEdge);
	ColorComponent(PlayingSurface, Surface);
	ColorComponent(Pedestal, FLinearColor(0.018f, 0.024f, 0.028f, 1.0f));
	ColorComponent(Backdrop, FLinearColor(0.002f, 0.004f, 0.008f, 1.0f));
	ColorComponent(StageBase, FLinearColor(0.006f, 0.014f, 0.023f, 1.0f));
	ColorComponent(VenueBackWall, FLinearColor(0.001f, 0.003f, 0.006f, 1.0f));
	for (UStaticMeshComponent* Rail : Rails) ColorComponent(Rail, RailMetal);
	for (int32 Index = 0; Index < RailAccentStrips.Num(); ++Index)
	{
		const FLinearColor Accent = Index == 0
			? GetTeamColor(EFlickTeam::Player1)
			: Index == 1 ? GetTeamColor(EFlickTeam::Player2) : FLinearColor(0.14f, 0.2f, 0.23f, 1.0f);
		ColorComponent(RailAccentStrips[Index], Accent);
	}
	for (int32 Index = 0; Index < PocketTrims.Num(); ++Index)
	{
		ColorComponent(PocketTrims[Index], Index < 2
			? GetTeamColor(EFlickTeam::Player1)
			: GetTeamColor(EFlickTeam::Player2));
	}
	for (UStaticMeshComponent* Pocket : Pockets) ColorComponent(Pocket, FLinearColor(0.075f, 0.09f, 0.11f, 1.0f));
	for (UStaticMeshComponent* PocketDepth : PocketDepths) ColorComponent(PocketDepth, FLinearColor(0.001f, 0.002f, 0.004f, 1.0f));
	for (UStaticMeshComponent* PocketBottom : PocketBottoms) ColorComponent(PocketBottom, FLinearColor(0.012f, 0.022f, 0.032f, 1.0f));
	for (int32 FlatIndex = 0; FlatIndex < PocketRimSegments.Num(); ++FlatIndex)
	{
		const int32 PocketIndex = FlatIndex / PocketRimSegmentCount;
		const FLinearColor TeamAccent = PocketIndex < 2
			? GetTeamColor(EFlickTeam::Player1)
			: GetTeamColor(EFlickTeam::Player2);
		ColorComponent(PocketRimSegments[FlatIndex], FMath::Lerp(
			FLinearColor(0.18f, 0.23f, 0.28f, 1.0f),
			TeamAccent,
			0.68f));
	}
	for (int32 Index = 0; Index < CornerCaps.Num(); ++Index)
	{
		ColorComponent(CornerCaps[Index], Index < 2
			? GetTeamColor(EFlickTeam::Player1)
			: GetTeamColor(EFlickTeam::Player2));
	}
	for (UStaticMeshComponent* Guide : GuideLines) ColorComponent(Guide, FLinearColor(0.4f, 0.47f, 0.54f, 1.0f));
	for (UStaticMeshComponent* PanelLine : SurfacePanelLines) ColorComponent(PanelLine, FLinearColor(0.035f, 0.055f, 0.078f, 1.0f));
	ColorComponent(StartLines[0], GetTeamColor(EFlickTeam::Player1));
	ColorComponent(StartLines[1], GetTeamColor(EFlickTeam::Player2));
	ColorComponent(CenterRingOuter, Marking);
	ColorComponent(CenterRingInner, Surface);
	for (UStaticMeshComponent* Pylon : VenuePylons) ColorComponent(Pylon, FLinearColor(0.008f, 0.018f, 0.028f, 1.0f));
	for (int32 Index = 0; Index < VenueBannerPanels.Num(); ++Index)
	{
		ColorComponent(VenueBannerPanels[Index], Index % 2 == 0
			? FLinearColor(0.0f, 0.075f, 0.14f, 1.0f)
			: FLinearColor(0.14f, 0.022f, 0.004f, 1.0f));
	}
	for (int32 Index = 0; Index < VenueLightBars.Num(); ++Index)
	{
		ColorComponent(VenueLightBars[Index], Index < VenueLightBars.Num() / 2
			? FLinearColor(0.0f, 0.62f, 0.96f, 1.0f)
			: Index == VenueLightBars.Num() / 2
				? FLinearColor(0.72f, 0.84f, 0.94f, 1.0f)
				: FLinearColor(1.0f, 0.2f, 0.025f, 1.0f));
	}
	for (UStaticMeshComponent* GridLine : FloorGridSegments) ColorComponent(GridLine, FLinearColor(0.004f, 0.026f, 0.042f, 1.0f));
}

void AFlickBobArena::ApplyPhysicsMaterials()
{
	if (!BoardBase)
	{
		return;
	}
	if (!BoardPhysicalMaterial)
	{
		BoardPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("BobBoardPhysicalMaterial"));
	}
	if (!RailPhysicalMaterial)
	{
		RailPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("BobRailPhysicalMaterial"));
	}
	BoardPhysicalMaterial->Friction = 0.035f;
	BoardPhysicalMaterial->Restitution = 0.18f;
	BoardPhysicalMaterial->bOverrideFrictionCombineMode = true;
	BoardPhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
	BoardPhysicalMaterial->bOverrideRestitutionCombineMode = true;
	BoardPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Average;
	RailPhysicalMaterial->Friction = 0.04f;
	RailPhysicalMaterial->Restitution = 0.48f;
	RailPhysicalMaterial->bOverrideFrictionCombineMode = true;
	RailPhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
	RailPhysicalMaterial->bOverrideRestitutionCombineMode = true;
	RailPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Average;
	BoardBase->SetPhysMaterialOverride(BoardPhysicalMaterial);
	for (UStaticMeshComponent* Rail : Rails)
	{
		Rail->SetPhysMaterialOverride(RailPhysicalMaterial);
	}
}
