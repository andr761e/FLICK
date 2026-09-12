#include "Arena/FlickTestArena.h"

#include "Components/StaticMeshComponent.h"
#include "Core/FlickArenaControlRules.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Pieces/FlickPiece.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void ConfigureVisualComponent(UStaticMeshComponent* Component)
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCastShadow(false);
		Component->SetCanEverAffectNavigation(false);
	}

	void SetMaterialColor(UMaterialInstanceDynamic* Material, const FLinearColor& Color, const float Roughness)
	{
		if (!Material)
		{
			return;
		}
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
	}

	void PlaceBarBetween(
		UStaticMeshComponent* Bar,
		const FVector2D& Start,
		const FVector2D& End,
		const float Z,
		const float Width,
		const float Height)
	{
		const FVector2D Delta = End - Start;
		Bar->SetRelativeLocation(FVector((Start + End) * 0.5f, Z));
		Bar->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.0f));
		Bar->SetRelativeScale3D(FVector(Delta.Size() / 100.0f, Width / 100.0f, Height / 100.0f));
	}

	void PlaceWorkshopTraceBetween(
		UStaticMeshComponent* Trace,
		const FVector2D& Start,
		const FVector2D& End,
		const float Z)
	{
		const FVector2D Delta = End - Start;
		Trace->SetRelativeLocation(FVector(Start, Z));
		Trace->SetRelativeRotation(FRotator(
			0.0f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.0f));
		// The authored trace is one metre long and has its pivot at its start.
		Trace->SetRelativeScale3D(FVector(Delta.Size() / 100.0f, 1.0f, 1.0f));
	}

	int32 GetAccentMaterialIndex(const UStaticMeshComponent* Component)
	{
		if (!Component)
		{
			return INDEX_NONE;
		}
		const int32 Index = Component->GetMaterialIndex(TEXT("09_Switch_Accent"));
		return Index != INDEX_NONE ? Index : 0;
	}

	UMaterialInstanceDynamic* CreateAccentMaterial(UStaticMeshComponent* Component)
	{
		const int32 MaterialIndex = GetAccentMaterialIndex(Component);
		return Component && MaterialIndex != INDEX_NONE
			? Component->CreateAndSetMaterialInstanceDynamic(MaterialIndex)
			: nullptr;
	}

	void SetAccentMaterial(UStaticMeshComponent* Component, UMaterialInterface* Material)
	{
		const int32 MaterialIndex = GetAccentMaterialIndex(Component);
		if (Component && Material && MaterialIndex != INDEX_NONE)
		{
			Component->SetMaterial(MaterialIndex, Material);
		}
	}
}

AFlickTestArena::AFlickTestArena()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkshopArenaAsset(
		TEXT("/Game/TestArena/Arena/SM_TestArena_Static.SM_TestArena_Static"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkshopDividerAsset(
		TEXT("/Game/TestArena/Arena/SM_TestArena_Divider.SM_TestArena_Divider"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkshopSocketAsset(
		TEXT("/Game/TestArena/Arena/SM_TestArena_DividerSocket.SM_TestArena_DividerSocket"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkshopSwitchAsset(
		TEXT("/Game/TestArena/Arena/SM_TestArena_SwitchHousing.SM_TestArena_SwitchHousing"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkshopDotAsset(
		TEXT("/Game/TestArena/Arena/SM_TestArena_SwitchDot.SM_TestArena_SwitchDot"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WorkshopTraceAsset(
		TEXT("/Game/TestArena/Arena/SM_TestArena_SignalTrace.SM_TestArena_SignalTrace"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StadiumStructureAsset(
		TEXT("/Game/TestArena/Stadium/SM_TestStadium_Structure.SM_TestStadium_Structure"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StadiumLightsAsset(
		TEXT("/Game/TestArena/Stadium/SM_TestStadium_Lights.SM_TestStadium_Lights"));
	bUsingWorkshopAssets = WorkshopArenaAsset.Succeeded()
		&& WorkshopDividerAsset.Succeeded() && WorkshopSocketAsset.Succeeded()
		&& WorkshopSwitchAsset.Succeeded() && WorkshopDotAsset.Succeeded()
		&& WorkshopTraceAsset.Succeeded();
	bUsingStadiumAssets = StadiumStructureAsset.Succeeded() && StadiumLightsAsset.Succeeded();

	auto CreateBar = [this](const FString& Name, const bool bCollision)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
		Component->SetupAttachment(GetRootComponent());
		if (CubeMesh.Succeeded())
		{
			Component->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			Component->SetMaterial(0, BasicMaterial.Object);
		}
		if (bCollision)
		{
			Component->SetSimulatePhysics(false);
			Component->SetCollisionObjectType(ECC_WorldStatic);
			Component->SetCollisionResponseToAllChannels(ECR_Block);
			Component->SetGenerateOverlapEvents(false);
			Component->SetCanEverAffectNavigation(false);
		}
		else
		{
			ConfigureVisualComponent(Component);
		}
		return Component;
	};
	auto CreateDisc = [&CreateBar](const FString& Name)
	{
		UStaticMeshComponent* Component = CreateBar(Name, false);
		if (CylinderMesh.Succeeded())
		{
			Component->SetStaticMesh(CylinderMesh.Object);
		}
		return Component;
	};

	ZoneOuterMeshes.Reserve(MaxMechanismCount);
	InstrumentDeckMesh = CreateDisc(TEXT("SwitchyardInstrumentDeck"));
	WorkshopArenaMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkshopArenaMesh"));
	WorkshopArenaMesh->SetupAttachment(GetRootComponent());
	ConfigureVisualComponent(WorkshopArenaMesh);
	WorkshopArenaMesh->SetCastShadow(true);
	if (bUsingWorkshopAssets)
	{
		WorkshopArenaMesh->SetStaticMesh(WorkshopArenaAsset.Object);
	}
	StadiumStructureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StadiumStructureMesh"));
	StadiumStructureMesh->SetupAttachment(GetRootComponent());
	ConfigureVisualComponent(StadiumStructureMesh);
	// The enclosing wall is presentation geometry, not an actual roof. Let the
	// established arena key light reach gameplay instead of casting the entire
	// board and every puck into one large interior shadow.
	StadiumStructureMesh->SetCastShadow(false);
	StadiumLightsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StadiumLightsMesh"));
	StadiumLightsMesh->SetupAttachment(GetRootComponent());
	ConfigureVisualComponent(StadiumLightsMesh);
	StadiumLightsMesh->SetCastShadow(false);
	if (bUsingStadiumAssets)
	{
		StadiumStructureMesh->SetStaticMesh(StadiumStructureAsset.Object);
		StadiumLightsMesh->SetStaticMesh(StadiumLightsAsset.Object);
	}
	else
	{
		StadiumStructureMesh->SetVisibility(false, true);
		StadiumStructureMesh->SetHiddenInGame(true, true);
		StadiumLightsMesh->SetVisibility(false, true);
		StadiumLightsMesh->SetHiddenInGame(true, true);
	}
	DividerCapMeshes.Reserve(MaxMechanismCount);
	ZoneInnerMeshes.Reserve(MaxMechanismCount);
	ZoneDotMeshes.Reserve(MaxMechanismCount);
	SignalTraceMeshes.Reserve(MaxMechanismCount);
	DividerMeshes.Reserve(MaxMechanismCount);
	DividerVisualMeshes.Reserve(MaxMechanismCount);
	for (int32 Index = 0; Index < MaxMechanismCount; ++Index)
	{
		ZoneOuterMeshes.Add(CreateDisc(FString::Printf(TEXT("ControlSwitchOuter_%02d"), Index)));
		ZoneInnerMeshes.Add(CreateDisc(FString::Printf(TEXT("ControlSwitchInner_%02d"), Index)));
		ZoneDotMeshes.Add(CreateDisc(FString::Printf(TEXT("ControlSwitchDot_%02d"), Index)));
		SignalTraceMeshes.Add(CreateBar(FString::Printf(TEXT("ControlSignalTrace_%02d"), Index), false));
		DividerMeshes.Add(CreateBar(FString::Printf(TEXT("EdgeDivider_%02d"), Index), true));
		DividerCapMeshes.Add(CreateBar(FString::Printf(TEXT("DividerCap_%02d"), Index), false));
		UStaticMeshComponent* DividerVisual = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("WorkshopDivider_%02d"), Index));
		DividerVisual->SetupAttachment(GetRootComponent());
		ConfigureVisualComponent(DividerVisual);
		DividerVisual->SetCastShadow(true);
		if (bUsingWorkshopAssets)
		{
			DividerVisual->SetStaticMesh(WorkshopDividerAsset.Object);
		}
		DividerVisualMeshes.Add(DividerVisual);
	}
	DividerBaseMeshes.Reserve(MaxPossibleLocationCount);
	for (int32 LocationIndex = 0; LocationIndex < MaxPossibleLocationCount; ++LocationIndex)
	{
		DividerBaseMeshes.Add(CreateBar(FString::Printf(TEXT("DividerSocket_%02d"), LocationIndex), false));
	}

	if (bUsingWorkshopAssets)
	{
		InstrumentDeckMesh->SetVisibility(false, true);
		InstrumentDeckMesh->SetHiddenInGame(true, true);
		for (int32 Index = 0; Index < MaxMechanismCount; ++Index)
		{
			ZoneOuterMeshes[Index]->SetStaticMesh(WorkshopSwitchAsset.Object);
			ZoneInnerMeshes[Index]->SetVisibility(false, true);
			ZoneInnerMeshes[Index]->SetHiddenInGame(true, true);
			ZoneDotMeshes[Index]->SetStaticMesh(WorkshopDotAsset.Object);
			SignalTraceMeshes[Index]->SetStaticMesh(WorkshopTraceAsset.Object);
			DividerCapMeshes[Index]->SetVisibility(false, true);
			DividerCapMeshes[Index]->SetHiddenInGame(true, true);
			// The decorative divider never participates in collision.
			DividerMeshes[Index]->SetVisibility(false, true);
			DividerMeshes[Index]->SetHiddenInGame(true, true);
		}
		for (UStaticMeshComponent* Socket : DividerBaseMeshes)
		{
			Socket->SetStaticMesh(WorkshopSocketAsset.Object);
		}
	}
	else
	{
		WorkshopArenaMesh->SetVisibility(false, true);
		WorkshopArenaMesh->SetHiddenInGame(true, true);
	}
}

void AFlickTestArena::BeginPlay()
{
	Super::BeginPlay();
	CreateRuntimeMaterials();
	BuildLayoutFromSeed();
	ApplyTestLayout();
	ApplyMechanismState();
}

void AFlickTestArena::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickTestArena, RaisedDividerMask);
	DOREPLIFETIME(AFlickTestArena, PendingToggleMask);
	DOREPLIFETIME(AFlickTestArena, ArenaLayoutSeed);
	DOREPLIFETIME(AFlickTestArena, ActiveMechanismCount);
	DOREPLIFETIME(AFlickTestArena, DesignedLocationCount);
}

void AFlickTestArena::InitializeTestArena(
	const float InRadius,
	const float InThickness,
	const float InSurfaceZ,
	const int32 InPlayersPerTeam)
{
	const int32 TeamSize = FMath::Clamp(InPlayersPerTeam, 1, 3);
	InitializeArena(InRadius, InThickness, InSurfaceZ, TeamSize);
	DesignedLocationCount = TeamSize == 1 ? 20 : TeamSize == 2 ? 28 : 36;
	ActiveMechanismCount = TeamSize == 1 ? 8 : TeamSize == 2 ? 12 : 14;
	if (HasAuthority())
	{
		ArenaLayoutSeed = FMath::Rand();
#if !UE_BUILD_SHIPPING
		FParse::Value(FCommandLine::Get(), TEXT("FlickTestArenaSeed="), ArenaLayoutSeed);
#endif
	}
	CreateRuntimeMaterials();
	BuildLayoutFromSeed();
	ApplyTestLayout();
	ResetMechanisms();
#if !UE_BUILD_SHIPPING
	// Explicit visual fixture for reviewing raised/queued/open states together.
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickTestArenaStatePreview")))
	{
		RaisedDividerMask = 0x1555 & static_cast<uint16>((1 << GetMechanismCount()) - 1);
		PendingToggleMask = 0x02;
		ApplyMechanismState();
	}
#endif
}

void AFlickTestArena::BeginControlZoneTracking(const TArray<TObjectPtr<AFlickPiece>>& Pieces)
{
	if (!HasAuthority())
	{
		return;
	}

	bTrackingShot = true;
	PendingToggleMask = 0;
	ArmedZoneMask = 0;
	TriggeredThisShotMask = 0;
	DeploymentTimers.Init(0.0f, GetMechanismCount());
	PreviousPieceLocations.Reset();
	for (const AFlickPiece* Piece : Pieces)
	{
		if (Piece && IsValid(Piece) && Piece->IsActive())
		{
			const FVector LocalLocation = GetActorTransform().InverseTransformPosition(Piece->GetActorLocation());
			PreviousPieceLocations.Add(Piece->GetPieceId(), FVector2D(LocalLocation.X, LocalLocation.Y));
		}
	}
	for (int32 ZoneIndex = 0; ZoneIndex < GetMechanismCount(); ++ZoneIndex)
	{
		if (!IsZoneCurrentlyOverlapped(ZoneIndex, Pieces))
		{
			ArmedZoneMask |= static_cast<uint16>(1 << ZoneIndex);
		}
	}
	ApplyMechanismState();
}

uint16 AFlickTestArena::TrackControlZoneCrossings(
	const TArray<TObjectPtr<AFlickPiece>>& Pieces,
	const float DeltaSeconds,
	uint16& OutDeployedMechanisms)
{
	OutDeployedMechanisms = 0;
	if (!HasAuthority() || !bTrackingShot)
	{
		return 0;
	}

	struct FTrackedPiece
	{
		int32 PieceId = 0;
		float Radius = 0.0f;
		FVector2D Current = FVector2D::ZeroVector;
		FVector2D Previous = FVector2D::ZeroVector;
	};
	TArray<FTrackedPiece, TInlineAllocator<24>> TrackedPieces;
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}
		const FVector LocalLocation = GetActorTransform().InverseTransformPosition(Piece->GetActorLocation());
		FTrackedPiece& Tracked = TrackedPieces.AddDefaulted_GetRef();
		Tracked.PieceId = Piece->GetPieceId();
		Tracked.Radius = Piece->GetPieceRadius();
		Tracked.Current = FVector2D(LocalLocation.X, LocalLocation.Y);
		if (const FVector2D* PreviousLocation = PreviousPieceLocations.Find(Tracked.PieceId))
		{
			Tracked.Previous = *PreviousLocation;
		}
		else
		{
			Tracked.Previous = Tracked.Current;
		}
	}

	const uint16 PendingMaskBeforeTracking = PendingToggleMask;
	for (int32 ZoneIndex = 0; ZoneIndex < GetMechanismCount(); ++ZoneIndex)
	{
		const uint16 ZoneBit = static_cast<uint16>(1 << ZoneIndex);
		if ((TriggeredThisShotMask & ZoneBit) != 0)
		{
			continue;
		}

		bool bCurrentlyOverlapped = false;
		bool bCrossedThisFrame = false;
		const FVector2D ZoneCenter = GetZoneLocalCenter(ZoneIndex);
		for (const FTrackedPiece& Piece : TrackedPieces)
		{
			const float DetectionRadius = Piece.Radius + SwitchDetectionPadding;
			bCurrentlyOverlapped |= FlickArenaControlRules::DoCirclesOverlap(
				Piece.Current, DetectionRadius, ZoneCenter, SwitchActivationDotRadius);
			bCrossedThisFrame |= FlickArenaControlRules::DoesSweptCircleCrossCircle(
				Piece.Previous, Piece.Current, DetectionRadius, ZoneCenter, SwitchActivationDotRadius);
		}

		if ((ArmedZoneMask & ZoneBit) == 0)
		{
			if (!bCurrentlyOverlapped)
			{
				ArmedZoneMask |= ZoneBit;
			}
			continue;
		}
		if (bCrossedThisFrame)
		{
			PendingToggleMask |= ZoneBit;
			TriggeredThisShotMask |= ZoneBit;
			ArmedZoneMask &= ~ZoneBit;
			if (DeploymentTimers.IsValidIndex(ZoneIndex))
			{
				DeploymentTimers[ZoneIndex] = DividerDeploymentDelay;
			}
		}
	}

	PreviousPieceLocations.Reset();
	for (const FTrackedPiece& Piece : TrackedPieces)
	{
		PreviousPieceLocations.Add(Piece.PieceId, Piece.Current);
	}
	const uint16 NewlyTriggeredMask = PendingToggleMask & ~PendingMaskBeforeTracking;
	if (NewlyTriggeredMask != 0)
	{
		ApplyMechanismState();
		ForceNetUpdate();
	}
	OutDeployedMechanisms = DeployReadyDividers(Pieces, DeltaSeconds);
	return NewlyTriggeredMask;
}

uint16 AFlickTestArena::CommitPendingControlZoneToggles(const TArray<TObjectPtr<AFlickPiece>>& Pieces)
{
	if (!HasAuthority())
	{
		return 0;
	}

	uint16 ToggledMechanisms = 0;
	TrackControlZoneCrossings(Pieces, 0.0f, ToggledMechanisms);
	const uint16 PendingBeforeCommit = PendingToggleMask;
	// A shot normally lasts far longer than the deployment delay. At resolution,
	// safely deploy any remaining unobstructed mechanisms and cancel only a wall
	// whose footprint is still occupied by a settled puck.
	for (int32 Index = 0; Index < GetMechanismCount(); ++Index)
	{
		const uint16 MechanismBit = static_cast<uint16>(1 << Index);
		if ((PendingToggleMask & MechanismBit) == 0)
		{
			continue;
		}
		const bool bWillRaise = (RaisedDividerMask & MechanismBit) == 0;
		if (!bWillRaise || !IsDividerCurrentlyOverlapped(Index, Pieces))
		{
			RaisedDividerMask ^= MechanismBit;
			ToggledMechanisms |= MechanismBit;
		}
	}
	PendingToggleMask = 0;
	ArmedZoneMask = 0;
	TriggeredThisShotMask = 0;
	bTrackingShot = false;
	PreviousPieceLocations.Reset();
	DeploymentTimers.Reset();
	if (ToggledMechanisms != 0 || PendingBeforeCommit != 0)
	{
		ApplyMechanismState();
		ForceNetUpdate();
	}
	return ToggledMechanisms;
}

void AFlickTestArena::ResetMechanisms()
{
	RaisedDividerMask = 0;
	PendingToggleMask = 0;
	ArmedZoneMask = 0;
	TriggeredThisShotMask = 0;
	bTrackingShot = false;
	PreviousPieceLocations.Reset();
	DeploymentTimers.Reset();
	ApplyMechanismState();
	ForceNetUpdate();
}

void AFlickTestArena::SetTrainingBoardEditMode(const bool bEnabled)
{
	bTrainingBoardEditMode = bEnabled;
	ApplyTestLayout();
	ApplyMechanismState();
}

bool AFlickTestArena::ToggleTrainingMechanismAtWorldLocation(
	const FVector& WorldLocation,
	bool& bOutEnabled,
	bool& bOutChanged)
{
	bOutEnabled = false;
	bOutChanged = false;
	if (!bTrainingBoardEditMode || !HasAuthority())
	{
		return false;
	}

	const FVector LocalLocation3D = GetActorTransform().InverseTransformPosition(WorldLocation);
	const FVector2D LocalLocation(LocalLocation3D.X, LocalLocation3D.Y);
	int32 ClosestLocationIndex = INDEX_NONE;
	float ClosestScore = TNumericLimits<float>::Max();
	for (int32 LocationIndex = 0; LocationIndex < GetPossibleLocationCount(); ++LocationIndex)
	{
		const FVector2D Offset = LocalLocation - PossibleDividerCenters[LocationIndex];
		const FVector2D Tangent(
			FMath::Cos(PossibleDividerAngles[LocationIndex]),
			FMath::Sin(PossibleDividerAngles[LocationIndex]));
		const FVector2D Normal(-Tangent.Y, Tangent.X);
		const float Along = FMath::Abs(FVector2D::DotProduct(Offset, Tangent));
		const float Across = FMath::Abs(FVector2D::DotProduct(Offset, Normal));
		const float HalfLength = PossibleDividerLengths[LocationIndex] * 0.5f + 14.0f;
		const float HalfWidth = DividerThickness * 0.5f + 22.0f;
		if (Along <= HalfLength && Across <= HalfWidth)
		{
			const float Score = FMath::Square(Along / HalfLength) + FMath::Square(Across / HalfWidth);
			if (Score < ClosestScore)
			{
				ClosestScore = Score;
				ClosestLocationIndex = LocationIndex;
			}
		}
	}

	// A live mechanism can also be selected by its switch, which is normally
	// much easier to click than the narrow flush divider socket.
	for (int32 MechanismIndex = 0; MechanismIndex < ActiveLocationIndices.Num(); ++MechanismIndex)
	{
		if (!ZoneCenters.IsValidIndex(MechanismIndex))
		{
			continue;
		}
		const float DistanceSquared = FVector2D::DistSquared(LocalLocation, ZoneCenters[MechanismIndex]);
		if (DistanceSquared <= FMath::Square(ControlZoneRadius + 18.0f))
		{
			ClosestLocationIndex = ActiveLocationIndices[MechanismIndex];
			break;
		}
	}

	if (ClosestLocationIndex == INDEX_NONE)
	{
		return false;
	}

	const int32 ExistingIndex = ActiveLocationIndices.Find(ClosestLocationIndex);
	if (ExistingIndex != INDEX_NONE)
	{
		// Keep one usable switch/divider pair so the mechanic cannot be left in
		// an invalid zero-component state.
		if (ActiveLocationIndices.Num() <= 1)
		{
			return true;
		}
		ActiveLocationIndices.RemoveAt(ExistingIndex);
		bOutEnabled = false;
		bOutChanged = true;
	}
	else
	{
		if (ActiveLocationIndices.Num() >= MaxMechanismCount)
		{
			return true;
		}
		ActiveLocationIndices.Add(ClosestLocationIndex);
		bOutEnabled = true;
		bOutChanged = true;
	}

	ActiveMechanismCount = ActiveLocationIndices.Num();
	PopulateActiveLayoutFromLocations();
	ResetMechanisms();
	ApplyTestLayout();
	ApplyMechanismState();
	ForceNetUpdate();
	return true;
}

void AFlickTestArena::BeginReplayPresentation()
{
	if (bReplayPresentationActive)
	{
		return;
	}
	PreReplayRaisedDividerMask = RaisedDividerMask;
	bReplayPresentationActive = true;
}

void AFlickTestArena::ApplyReplayDividerState(const uint16 DividerMask)
{
	if (!bReplayPresentationActive)
	{
		return;
	}
	RaisedDividerMask = DividerMask;
	PendingToggleMask = 0;
	ApplyMechanismState();
}

void AFlickTestArena::EndReplayPresentation()
{
	if (!bReplayPresentationActive)
	{
		return;
	}
	bReplayPresentationActive = false;
	RaisedDividerMask = PreReplayRaisedDividerMask;
	PendingToggleMask = 0;
	ApplyMechanismState();
}

bool AFlickTestArena::IsDividerRaised(const int32 DividerIndex) const
{
	return DividerIndex >= 0
		&& DividerIndex < GetMechanismCount()
		&& (RaisedDividerMask & (1 << DividerIndex)) != 0;
}

bool AFlickTestArena::FindDividerIndex(
	const UPrimitiveComponent* Component,
	int32& OutDividerIndex) const
{
	OutDividerIndex = INDEX_NONE;
	for (int32 Index = 0; Index < DividerMeshes.Num(); ++Index)
	{
		if (DividerMeshes[Index] == Component)
		{
			OutDividerIndex = Index;
			return true;
		}
	}
	return false;
}

bool AFlickTestArena::IsDividerPending(const int32 DividerIndex) const
{
	return DividerIndex >= 0 && DividerIndex < GetMechanismCount()
		&& (PendingToggleMask & (1 << DividerIndex)) != 0;
}

FVector AFlickTestArena::GetSwitchLabelLocation(const int32 Index) const
{
	const FVector2D Center = GetZoneLocalCenter(Index);
	return GetActorTransform().TransformPosition(FVector(
		Center - Center.GetSafeNormal() * (ControlZoneRadius + 24.0f), SurfaceZ + 5.0f));
}

FVector AFlickTestArena::GetDividerLabelLocation(const int32 Index) const
{
	if (!DividerCenters.IsValidIndex(Index)) return GetActorLocation();
	const FVector2D Center = DividerCenters[Index];
	return GetActorTransform().TransformPosition(FVector(
		Center - Center.GetSafeNormal() * (DividerThickness + 30.0f), SurfaceZ + 5.0f));
}

FVector AFlickTestArena::GetDividerWorldCenter(const int32 Index) const
{
	if (!DividerCenters.IsValidIndex(Index))
	{
		return GetActorLocation();
	}
	return GetActorTransform().TransformPosition(FVector(DividerCenters[Index], SurfaceZ));
}

FVector AFlickTestArena::GetSwitchWorldCenter(const int32 Index) const
{
	const FVector2D Center = GetZoneLocalCenter(Index);
	return GetActorTransform().TransformPosition(FVector(Center, SurfaceZ));
}

FVector2D AFlickTestArena::GetDividerWorldTangent(const int32 Index) const
{
	if (!DividerAngles.IsValidIndex(Index))
	{
		return FVector2D(1.0f, 0.0f);
	}
	const FVector LocalTangent(FMath::Cos(DividerAngles[Index]), FMath::Sin(DividerAngles[Index]), 0.0f);
	const FVector WorldTangent = GetActorTransform().TransformVectorNoScale(LocalTangent).GetSafeNormal();
	return FVector2D(WorldTangent.X, WorldTangent.Y);
}

float AFlickTestArena::GetDividerLength(const int32 Index) const
{
	return RandomizedDividerLengths.IsValidIndex(Index)
		? RandomizedDividerLengths[Index]
		: DividerLength;
}

FString AFlickTestArena::GetDividerLabel(const int32 DividerIndex) const
{
	static const TCHAR* Labels[8] =
	{
		TEXT("EAST"), TEXT("NORTH-EAST"), TEXT("NORTH"), TEXT("NORTH-WEST"),
		TEXT("WEST"), TEXT("SOUTH-WEST"), TEXT("SOUTH"), TEXT("SOUTH-EAST")
	};
	if (!DividerCenters.IsValidIndex(DividerIndex))
	{
		return TEXT("EDGE");
	}
	const float AngleDegrees = FMath::Fmod(
		FMath::RadiansToDegrees(FMath::Atan2(DividerCenters[DividerIndex].Y, DividerCenters[DividerIndex].X)) + 360.0f,
		360.0f);
	const int32 DirectionIndex = FMath::RoundToInt(AngleDegrees / 45.0f) % 8;
	return FString::Printf(TEXT("%02d / %s"), DividerIndex + 1, Labels[DirectionIndex]);
}

FLinearColor AFlickTestArena::GetMechanismColor(const int32 MechanismIndex) const
{
	static const FLinearColor Colors[MaxMechanismCount] =
	{
		FLinearColor(0.0f, 0.72f, 0.92f, 1.0f),
		FLinearColor(0.52f, 0.34f, 0.96f, 1.0f),
		FLinearColor(0.95f, 0.58f, 0.08f, 1.0f),
		FLinearColor(0.98f, 0.24f, 0.045f, 1.0f),
		FLinearColor(0.84f, 0.2f, 0.62f, 1.0f),
		FLinearColor(0.08f, 0.72f, 0.5f, 1.0f),
		FLinearColor(0.34f, 0.78f, 0.18f, 1.0f),
		FLinearColor(0.94f, 0.82f, 0.12f, 1.0f),
		FLinearColor(0.10f, 0.64f, 0.96f, 1.0f),
		FLinearColor(0.72f, 0.30f, 0.92f, 1.0f),
		FLinearColor(0.98f, 0.46f, 0.10f, 1.0f),
		FLinearColor(0.96f, 0.18f, 0.32f, 1.0f),
		FLinearColor(0.20f, 0.82f, 0.64f, 1.0f),
		FLinearColor(0.68f, 0.88f, 0.16f, 1.0f)
	};
	return Colors[FMath::Clamp(MechanismIndex, 0, MaxMechanismCount - 1)];
}

void AFlickTestArena::OnRep_DividerState()
{
	ApplyMechanismState();
}

void AFlickTestArena::OnRep_TestLayout()
{
	BuildLayoutFromSeed();
	ApplyTestLayout();
	ApplyMechanismState();
}

FVector2D AFlickTestArena::GetZoneLocalCenter(const int32 ZoneIndex) const
{
	return ZoneCenters.IsValidIndex(ZoneIndex) ? ZoneCenters[ZoneIndex] : FVector2D::ZeroVector;
}

bool AFlickTestArena::IsZoneCurrentlyOverlapped(
	const int32 ZoneIndex,
	const TArray<TObjectPtr<AFlickPiece>>& Pieces) const
{
	const FVector2D ZoneCenter = GetZoneLocalCenter(ZoneIndex);
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}
		const FVector LocalLocation = GetActorTransform().InverseTransformPosition(Piece->GetActorLocation());
		if (FlickArenaControlRules::DoCirclesOverlap(
			FVector2D(LocalLocation.X, LocalLocation.Y),
			Piece->GetPieceRadius() + SwitchDetectionPadding,
			ZoneCenter,
			SwitchActivationDotRadius))
		{
			return true;
		}
	}
	return false;
}

bool AFlickTestArena::IsDividerCurrentlyOverlapped(
	const int32 DividerIndex,
	const TArray<TObjectPtr<AFlickPiece>>& Pieces) const
{
	if (!DividerCenters.IsValidIndex(DividerIndex)
		|| !DividerAngles.IsValidIndex(DividerIndex)
		|| !RandomizedDividerLengths.IsValidIndex(DividerIndex))
	{
		return false;
	}

	const FVector2D HalfExtents(
		RandomizedDividerLengths[DividerIndex] * 0.5f,
		DividerThickness * 0.5f + DividerDeploymentClearance);
	for (const AFlickPiece* Piece : Pieces)
	{
		if (!Piece || !IsValid(Piece) || !Piece->IsActive())
		{
			continue;
		}
		const FVector LocalLocation = GetActorTransform().InverseTransformPosition(Piece->GetActorLocation());
		if (FlickArenaControlRules::DoesCircleOverlapOrientedBox(
			FVector2D(LocalLocation.X, LocalLocation.Y),
			Piece->GetPieceRadius(),
			DividerCenters[DividerIndex],
			HalfExtents,
			DividerAngles[DividerIndex]))
		{
			return true;
		}
	}
	return false;
}

uint16 AFlickTestArena::DeployReadyDividers(
	const TArray<TObjectPtr<AFlickPiece>>& Pieces,
	const float DeltaSeconds)
{
	uint16 DeployedMask = 0;
	for (int32 Index = 0; Index < GetMechanismCount(); ++Index)
	{
		const uint16 MechanismBit = static_cast<uint16>(1 << Index);
		if ((PendingToggleMask & MechanismBit) == 0)
		{
			continue;
		}
		if (DeploymentTimers.IsValidIndex(Index))
		{
			DeploymentTimers[Index] = FMath::Max(0.0f, DeploymentTimers[Index] - DeltaSeconds);
			if (DeploymentTimers[Index] > 0.0f)
			{
				continue;
			}
		}

		const bool bWillRaise = (RaisedDividerMask & MechanismBit) == 0;
		if (bWillRaise && IsDividerCurrentlyOverlapped(Index, Pieces))
		{
			continue;
		}

		RaisedDividerMask ^= MechanismBit;
		PendingToggleMask &= ~MechanismBit;
		DeployedMask |= MechanismBit;
	}

	if (DeployedMask != 0)
	{
		ApplyMechanismState();
		ForceNetUpdate();
	}
	return DeployedMask;
}

void AFlickTestArena::BuildLayoutFromSeed()
{
	const int32 ActiveCount = GetMechanismCount();
	const int32 LocationCount = GetPossibleLocationCount();
	FRandomStream LayoutRandom(ArenaLayoutSeed);

	// Each format uses a rotationally symmetric authored pool split evenly
	// between tangent rim sockets and staggered inset sockets. Randomness only
	// chooses which locations are wired to live switches for this match.
	PossibleDividerCenters.SetNum(LocationCount);
	PossibleDividerAngles.SetNum(LocationCount);
	PossibleDividerLengths.SetNum(LocationCount);
	TArray<int32, TInlineAllocator<18>> OuterLocations;
	TArray<int32, TInlineAllocator<18>> InsetLocations;
	for (int32 LocationIndex = 0; LocationIndex < LocationCount; ++LocationIndex)
	{
		const bool bOuter = (LocationIndex % 2) == 0;
		const float RadialAngle = FMath::DegreesToRadians(LocationIndex * (360.0f / LocationCount));
		const FVector2D Radial(FMath::Cos(RadialAngle), FMath::Sin(RadialAngle));
		PossibleDividerCenters[LocationIndex] = Radial * ArenaRadius * (bOuter ? OuterDividerRadiusFraction : 0.82f);
		PossibleDividerAngles[LocationIndex] = RadialAngle + PI * 0.5f
			+ (bOuter ? 0.0f : FMath::DegreesToRadians((LocationIndex % 4) == 1 ? 11.0f : -11.0f));
		PossibleDividerLengths[LocationIndex] = DividerLength * (bOuter ? 0.92f : 1.08f);
		(bOuter ? OuterLocations : InsetLocations).Add(LocationIndex);
	}

	const auto ShuffleLocations = [&LayoutRandom](auto& Locations)
	{
		for (int32 Index = Locations.Num() - 1; Index > 0; --Index)
		{
			Locations.Swap(Index, LayoutRandom.RandRange(0, Index));
		}
	};
	ShuffleLocations(OuterLocations);
	ShuffleLocations(InsetLocations);

	ActiveLocationIndices.Reset(ActiveCount);
	const int32 OuterCount = FMath::Clamp(
		FMath::Max(GuaranteedOuterEdgeMechanisms, (ActiveCount + 1) / 2), 1, ActiveCount);
	const int32 InsetCount = ActiveCount - OuterCount;
	for (int32 Index = 0; Index < OuterCount && Index < OuterLocations.Num(); ++Index)
	{
		ActiveLocationIndices.Add(OuterLocations[Index]);
	}
	for (int32 Index = 0; Index < InsetCount && Index < InsetLocations.Num(); ++Index)
	{
		ActiveLocationIndices.Add(InsetLocations[Index]);
	}
	ShuffleLocations(ActiveLocationIndices);

	PopulateActiveLayoutFromLocations();
}

void AFlickTestArena::PopulateActiveLayoutFromLocations()
{
	const int32 ActiveCount = FMath::Min(ActiveLocationIndices.Num(), MaxMechanismCount);
	const int32 LocationCount = GetPossibleLocationCount();
	ZoneCenters.SetNum(ActiveCount);
	DividerCenters.SetNum(ActiveCount);
	DividerAngles.SetNum(ActiveCount);
	RandomizedDividerLengths.SetNum(ActiveCount);
	for (int32 Index = 0; Index < ActiveCount; ++Index)
	{
		const int32 LocationIndex = FMath::Clamp(
			ActiveLocationIndices[Index], 0, LocationCount - 1);
		DividerCenters[Index] = PossibleDividerCenters[LocationIndex];
		DividerAngles[Index] = PossibleDividerAngles[LocationIndex];
		RandomizedDividerLengths[Index] = PossibleDividerLengths[LocationIndex];

		const float DividerAngle = FMath::DegreesToRadians(LocationIndex * (360.0f / LocationCount));
		const float Direction = (LocationIndex % 4) < 2 ? 1.0f : -1.0f;
		const float SwitchAngle = DividerAngle + FMath::DegreesToRadians(Direction * 3.0f);
		const float SwitchRadius = FMath::Max(
			ArenaRadius * 0.35f,
			DividerCenters[Index].Size() - SwitchDistanceFromDivider);
		ZoneCenters[Index] = FVector2D(FMath::Cos(SwitchAngle), FMath::Sin(SwitchAngle))
			* SwitchRadius;
	}
}

void AFlickTestArena::ApplyTestLayout()
{
	if (ZoneCenters.Num() != GetMechanismCount())
	{
		BuildLayoutFromSeed();
	}
	// In both paths the inherited cylinder remains the sole play-surface collider.
	// Workshop art is visual-only and is authored around Z=0 (the play surface).
	SetArenaPresentationVisible(!bUsingWorkshopAssets);
	WorkshopArenaMesh->SetVisibility(bUsingWorkshopAssets, true);
	WorkshopArenaMesh->SetHiddenInGame(!bUsingWorkshopAssets, true);
	WorkshopArenaMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ));
	WorkshopArenaMesh->SetRelativeScale3D(FVector(
		ArenaRadius / 650.0f,
		ArenaRadius / 650.0f,
		ArenaThickness / 50.0f));
	// Both stadium exports use the same origin and 650 cm arena opening as the
	// workshop arena. Uniform scaling keeps every surrounding detail aligned if
	// the Test arena radius is tuned later, while its components remain visual-only.
	const float StadiumScale = ArenaRadius / 650.0f;
	for (UStaticMeshComponent* StadiumComponent : {
		StadiumStructureMesh.Get(), StadiumLightsMesh.Get()})
	{
		StadiumComponent->SetVisibility(bUsingStadiumAssets, true);
		StadiumComponent->SetHiddenInGame(!bUsingStadiumAssets, true);
		StadiumComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ));
		StadiumComponent->SetRelativeScale3D(FVector(StadiumScale));
	}
	InstrumentDeckMesh->SetVisibility(!bUsingWorkshopAssets, true);
	InstrumentDeckMesh->SetHiddenInGame(bUsingWorkshopAssets, true);
	InstrumentDeckMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SurfaceZ + 2.1f));
	InstrumentDeckMesh->SetRelativeScale3D(FVector(ArenaRadius * 1.98f / 100.0f, ArenaRadius * 1.98f / 100.0f, 0.004f));
	// Workshop switch, trace and dormant-socket meshes are authored downward
	// from a shared Z=0 top face. Place that face exactly on the authoritative
	// arena surface so pucks cannot visually enter non-colliding presentation.
	// Keep the authored switch graphics visually flush while giving their top
	// faces a sub-centimetre depth separation from the arena's curved linework.
	// These components never collide, so gameplay geometry remains exactly flat.
	// Keep the complete switch assembly above both the deck and its etched line
	// layer. A one-centimetre visual separation is imperceptible at puck scale,
	// but prevents depth-buffer contention where switches cross curved markings.
	const float SwitchSurfaceZ = SurfaceZ + 1.15f;
	for (int32 LocationIndex = 0; LocationIndex < MaxPossibleLocationCount; ++LocationIndex)
	{
		UStaticMeshComponent* Socket = DividerBaseMeshes[LocationIndex];
		const bool bDesignedForFormat = LocationIndex < GetPossibleLocationCount();
		Socket->SetVisibility(bDesignedForFormat, true);
		Socket->SetHiddenInGame(!bDesignedForFormat, true);
		if (!bDesignedForFormat)
		{
			continue;
		}
		// Socket collision/presentation stays flush with the authoritative deck;
		// only the non-colliding switch graphics need the anti-z-fighting lift.
		Socket->SetRelativeLocation(FVector(PossibleDividerCenters[LocationIndex], SurfaceZ));
		Socket->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(PossibleDividerAngles[LocationIndex]), 0.0f));
		Socket->SetRelativeScale3D(bUsingWorkshopAssets
			? FVector(PossibleDividerLengths[LocationIndex] / DividerLength, 1.0f, 1.0f)
			: FVector(
				PossibleDividerLengths[LocationIndex] / 100.0f,
				(DividerThickness + 8.0f) / 100.0f,
				0.008f));
		if (DormantSocketMaterial)
		{
			SetAccentMaterial(Socket, DormantSocketMaterial);
		}
	}

	for (int32 Index = 0; Index < MaxMechanismCount; ++Index)
	{
		const bool bActive = Index < GetMechanismCount();
		for (UStaticMeshComponent* Component : {
			ZoneOuterMeshes[Index].Get(), ZoneDotMeshes[Index].Get(),
			SignalTraceMeshes[Index].Get()})
		{
			Component->SetVisibility(bActive, true);
			Component->SetHiddenInGame(!bActive, true);
		}
		ZoneInnerMeshes[Index]->SetVisibility(bActive && !bUsingWorkshopAssets, true);
		ZoneInnerMeshes[Index]->SetHiddenInGame(!bActive || bUsingWorkshopAssets, true);
		if (!bActive)
		{
			DividerCapMeshes[Index]->SetVisibility(false);
			DividerMeshes[Index]->SetVisibility(false, true);
			DividerMeshes[Index]->SetHiddenInGame(true, true);
			DividerMeshes[Index]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			DividerVisualMeshes[Index]->SetVisibility(false, true);
			DividerVisualMeshes[Index]->SetHiddenInGame(true, true);
			continue;
		}

		const float DividerYaw = FMath::RadiansToDegrees(DividerAngles[Index]);
		const FVector2D ZoneCenter = GetZoneLocalCenter(Index);
		const FVector2D DividerCenter = DividerCenters[Index];
		const FVector2D ToDivider = (DividerCenter - ZoneCenter).GetSafeNormal();

		ZoneOuterMeshes[Index]->SetRelativeLocation(FVector(ZoneCenter, SwitchSurfaceZ));
		ZoneOuterMeshes[Index]->SetRelativeScale3D(bUsingWorkshopAssets
			? FVector(ControlZoneRadius / 40.0f)
			: FVector(ControlZoneRadius * 2.0f / 100.0f, ControlZoneRadius * 2.0f / 100.0f, 0.008f));
		ZoneInnerMeshes[Index]->SetRelativeLocation(FVector(ZoneCenter, SwitchSurfaceZ + 0.5f));
		const float InnerRadius = FMath::Max(SwitchActivationDotRadius + 5.0f, ControlZoneRadius - 7.0f);
		ZoneInnerMeshes[Index]->SetRelativeScale3D(FVector(InnerRadius * 2.0f / 100.0f, InnerRadius * 2.0f / 100.0f, 0.008f));
		ZoneDotMeshes[Index]->SetRelativeLocation(FVector(ZoneCenter, SwitchSurfaceZ));
		ZoneDotMeshes[Index]->SetRelativeScale3D(bUsingWorkshopAssets
			? FVector(SwitchActivationDotRadius / 8.0f)
			: FVector(
				SwitchActivationDotRadius * 2.0f / 100.0f,
				SwitchActivationDotRadius * 2.0f / 100.0f,
				0.009f));
		const FVector2D TraceStart = ZoneCenter + ToDivider * (ControlZoneRadius + 7.0f);
		const FVector2D TraceEnd = DividerCenter - ToDivider * (DividerThickness * 0.5f + 8.0f);
		if (bUsingWorkshopAssets)
		{
			PlaceWorkshopTraceBetween(SignalTraceMeshes[Index], TraceStart, TraceEnd, SwitchSurfaceZ);
		}
		else
		{
			PlaceBarBetween(SignalTraceMeshes[Index], TraceStart, TraceEnd,
				SwitchSurfaceZ + 0.2f, 2.5f, 0.6f);
		}

		if (ActiveLocationIndices.IsValidIndex(Index)
			&& DividerBaseMeshes.IsValidIndex(ActiveLocationIndices[Index]))
		{
			SetAccentMaterial(DividerBaseMeshes[ActiveLocationIndices[Index]], AccentMaterials[Index]);
			DividerBaseMeshes[ActiveLocationIndices[Index]]->SetRelativeScale3D(bUsingWorkshopAssets
				? FVector(RandomizedDividerLengths[Index] / DividerLength, 1.0f, 1.0f)
				: FVector(
					RandomizedDividerLengths[Index] / 100.0f,
					(DividerThickness + 8.0f) / 100.0f,
					0.012f));
		}
		DividerMeshes[Index]->SetRelativeLocation(FVector(DividerCenter, SurfaceZ + DividerHeight * 0.5f + 1.0f));
		DividerMeshes[Index]->SetRelativeRotation(FRotator(0.0f, DividerYaw, 0.0f));
		DividerMeshes[Index]->SetRelativeScale3D(FVector(RandomizedDividerLengths[Index] / 100.0f, DividerThickness / 100.0f, DividerHeight / 100.0f));
		DividerCapMeshes[Index]->SetRelativeLocation(FVector(DividerCenter, SurfaceZ + DividerHeight + 1.6f));
		DividerCapMeshes[Index]->SetRelativeRotation(FRotator(0.0f, DividerYaw, 0.0f));
		DividerCapMeshes[Index]->SetRelativeScale3D(FVector(
			(RandomizedDividerLengths[Index] - 8.0f) / 100.0f, DividerThickness * 0.62f / 100.0f, 0.012f));
		DividerVisualMeshes[Index]->SetRelativeLocation(FVector(DividerCenter, SurfaceZ));
		DividerVisualMeshes[Index]->SetRelativeRotation(FRotator(0.0f, DividerYaw, 0.0f));
		DividerVisualMeshes[Index]->SetRelativeScale3D(FVector(
			RandomizedDividerLengths[Index] / DividerLength,
			DividerThickness / 18.0f,
			DividerHeight / 56.0f));
	}
}

void AFlickTestArena::CreateRuntimeMaterials()
{
	if (!InstrumentDeckMaterial)
	{
		InstrumentDeckMaterial = InstrumentDeckMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	SetMaterialColor(InstrumentDeckMaterial, FLinearColor(0.026f, 0.034f, 0.032f, 1.0f), 0.94f);
	if (AccentMaterials.IsEmpty())
	{
		AccentMaterials.Reserve(MaxMechanismCount);
		DividerMaterials.Reserve(MaxMechanismCount);
		for (int32 Index = 0; Index < MaxMechanismCount; ++Index)
		{
			UMaterialInstanceDynamic* Accent = bUsingWorkshopAssets
				? CreateAccentMaterial(ZoneOuterMeshes[Index])
				: ZoneOuterMeshes[Index]->CreateAndSetMaterialInstanceDynamic(0);
			AccentMaterials.Add(Accent);
			DotMaterials.Add(bUsingWorkshopAssets
				? CreateAccentMaterial(ZoneDotMeshes[Index])
				: ZoneDotMeshes[Index]->CreateAndSetMaterialInstanceDynamic(0));
			TraceMaterials.Add(bUsingWorkshopAssets
				? CreateAccentMaterial(SignalTraceMeshes[Index])
				: SignalTraceMeshes[Index]->CreateAndSetMaterialInstanceDynamic(0));
			if (!bUsingWorkshopAssets)
			{
				DividerCapMeshes[Index]->SetMaterial(0, Accent);
			}
			DividerMaterials.Add(bUsingWorkshopAssets
				? CreateAccentMaterial(DividerVisualMeshes[Index])
				: DividerMeshes[Index]->CreateAndSetMaterialInstanceDynamic(0));
		}
	}
	if (!ZoneInsetMaterial)
	{
		ZoneInsetMaterial = ZoneInnerMeshes[0]->CreateAndSetMaterialInstanceDynamic(0);
		for (int32 Index = 1; Index < MaxMechanismCount; ++Index)
		{
			ZoneInnerMeshes[Index]->SetMaterial(0, ZoneInsetMaterial);
		}
	}
	if (!DormantSocketMaterial)
	{
		DormantSocketMaterial = bUsingWorkshopAssets
			? CreateAccentMaterial(DividerBaseMeshes[0])
			: DividerBaseMeshes[0]->CreateAndSetMaterialInstanceDynamic(0);
		for (int32 Index = 1; Index < DividerBaseMeshes.Num(); ++Index)
		{
			SetAccentMaterial(DividerBaseMeshes[Index], DormantSocketMaterial);
		}
	}
	// Matte, non-emissive surfaces preserve puck readability and avoid adding bloom.
	SetMaterialColor(ZoneInsetMaterial, FLinearColor(0.012f, 0.019f, 0.017f, 1.0f), 0.88f);
	SetMaterialColor(DormantSocketMaterial, FLinearColor(0.11f, 0.14f, 0.18f, 1.0f), 0.72f);
	for (int32 Index = 0; Index < MaxMechanismCount; ++Index)
	{
		const FLinearColor Accent = GetMechanismColor(Index);
		SetMaterialColor(
			DividerMaterials[Index],
			FLinearColor(
				Accent.R * 0.42f + 0.07f,
				Accent.G * 0.42f + 0.085f,
				Accent.B * 0.42f + 0.10f,
				1.0f),
			0.48f);
	}

	if (!RuntimeDividerPhysicalMaterial)
	{
		RuntimeDividerPhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("TestDividerPhysicalMaterial"));
	}
	if (RuntimeDividerPhysicalMaterial)
	{
		RuntimeDividerPhysicalMaterial->Friction = DividerFriction;
		RuntimeDividerPhysicalMaterial->Restitution = DividerRestitution;
		RuntimeDividerPhysicalMaterial->bOverrideFrictionCombineMode = true;
		RuntimeDividerPhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Min;
		RuntimeDividerPhysicalMaterial->bOverrideRestitutionCombineMode = true;
		RuntimeDividerPhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Max;
		for (UStaticMeshComponent* Divider : DividerMeshes)
		{
			Divider->SetPhysMaterialOverride(RuntimeDividerPhysicalMaterial);
		}
	}
}

void AFlickTestArena::ApplyMechanismState()
{
	for (int32 Index = 0; Index < MaxMechanismCount; ++Index)
	{
		const uint16 MechanismBit = static_cast<uint16>(1 << Index);
		const bool bActive = Index < GetMechanismCount();
		const bool bRaised = bActive && (RaisedDividerMask & MechanismBit) != 0;
		const bool bPending = bActive && (PendingToggleMask & MechanismBit) != 0;
		const FLinearColor BaseColor = GetMechanismColor(Index);
		const float Intensity = bPending ? 1.0f : bRaised ? 0.92f : 0.56f;
		SetMaterialColor(
			AccentMaterials.IsValidIndex(Index) ? AccentMaterials[Index] : nullptr,
			FLinearColor(BaseColor.R * Intensity, BaseColor.G * Intensity, BaseColor.B * Intensity, 1.0f),
			0.74f);
		SetMaterialColor(DotMaterials.IsValidIndex(Index) ? DotMaterials[Index] : nullptr,
			bPending ? FLinearColor(0.95f, 0.96f, 0.87f, 1.0f) : BaseColor, 0.72f);
		SetMaterialColor(TraceMaterials.IsValidIndex(Index) ? TraceMaterials[Index] : nullptr,
			FLinearColor(BaseColor.R, BaseColor.G, BaseColor.B, 1.0f) * (bPending ? 0.65f : bRaised ? 0.4f : 0.18f), 0.9f);
		DividerCapMeshes[Index]->SetVisibility(bRaised && !bUsingWorkshopAssets);
		DividerCapMeshes[Index]->SetHiddenInGame(!bRaised || bUsingWorkshopAssets);
		if (DividerVisualMeshes.IsValidIndex(Index) && DividerVisualMeshes[Index])
		{
			DividerVisualMeshes[Index]->SetVisibility(bRaised && bUsingWorkshopAssets, true);
			DividerVisualMeshes[Index]->SetHiddenInGame(!bRaised || !bUsingWorkshopAssets, true);
		}

		if (DividerMeshes.IsValidIndex(Index) && DividerMeshes[Index])
		{
			DividerMeshes[Index]->SetVisibility(bRaised && !bUsingWorkshopAssets, true);
			DividerMeshes[Index]->SetHiddenInGame(!bRaised || bUsingWorkshopAssets, true);
			DividerMeshes[Index]->SetCollisionEnabled(
				bRaised ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		}
	}
}
