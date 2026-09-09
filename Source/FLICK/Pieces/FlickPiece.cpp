#include "Pieces/FlickPiece.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Core/FlickLog.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "Game/FlickGameMode.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodyInstance.h"
#include "UObject/ConstructorHelpers.h"

AFlickPiece::AFlickPiece()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	// The server owns every puck outcome. Legacy/default correction favors an
	// exact authoritative resting state over a separately predicted client result.
	SetPhysicsReplicationMode(EPhysicsReplicationMode::Default);
	SetNetUpdateFrequency(60.0f);
	SetMinNetUpdateFrequency(30.0f);

	PieceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PieceMesh"));
	SetRootComponent(PieceMesh);
	PieceMesh->SetIsReplicated(true);
	WorkshopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorkshopMesh"));
	WorkshopMesh->SetupAttachment(PieceMesh);
	WorkshopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WorkshopMesh->SetGenerateOverlapEvents(false);
	WorkshopMesh->SetCanEverAffectNavigation(false);
	WorkshopMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		PieceMesh->SetStaticMesh(CylinderMesh.Object);
	}

	TopDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopDisc"));
	TopDisc->SetupAttachment(PieceMesh);
	OuterTrim = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OuterTrim"));
	OuterTrim->SetupAttachment(PieceMesh);
	SideBand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SideBand"));
	SideBand->SetupAttachment(PieceMesh);
	Underglow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Underglow"));
	Underglow->SetupAttachment(PieceMesh);
	SelectionHalo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionHalo"));
	SelectionHalo->SetupAttachment(PieceMesh);
	CenterPip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CenterPip"));
	CenterPip->SetupAttachment(PieceMesh);
	InnerRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerRing"));
	InnerRing->SetupAttachment(PieceMesh);
	CorePlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CorePlate"));
	CorePlate->SetupAttachment(PieceMesh);
	LowerTrim = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LowerTrim"));
	LowerTrim->SetupAttachment(PieceMesh);
	UpperShoulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("UpperShoulder"));
	UpperShoulder->SetupAttachment(PieceMesh);
	LowerShoulder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LowerShoulder"));
	LowerShoulder->SetupAttachment(PieceMesh);
	TopBezel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopBezel"));
	TopBezel->SetupAttachment(PieceMesh);
	CoreBezel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreBezel"));
	CoreBezel->SetupAttachment(PieceMesh);
	SignatureRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignatureRing"));
	SignatureRing->SetupAttachment(PieceMesh);
	SignatureInset = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SignatureInset"));
	SignatureInset->SetupAttachment(PieceMesh);
	if (CylinderMesh.Succeeded())
	{
		TopDisc->SetStaticMesh(CylinderMesh.Object);
		OuterTrim->SetStaticMesh(CylinderMesh.Object);
		SideBand->SetStaticMesh(CylinderMesh.Object);
		Underglow->SetStaticMesh(CylinderMesh.Object);
		SelectionHalo->SetStaticMesh(CylinderMesh.Object);
		CenterPip->SetStaticMesh(CylinderMesh.Object);
		InnerRing->SetStaticMesh(CylinderMesh.Object);
		CorePlate->SetStaticMesh(CylinderMesh.Object);
		LowerTrim->SetStaticMesh(CylinderMesh.Object);
		UpperShoulder->SetStaticMesh(CylinderMesh.Object);
		LowerShoulder->SetStaticMesh(CylinderMesh.Object);
		TopBezel->SetStaticMesh(CylinderMesh.Object);
		CoreBezel->SetStaticMesh(CylinderMesh.Object);
		SignatureRing->SetStaticMesh(CylinderMesh.Object);
		SignatureInset->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveMaterial(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (BasicMaterial.Succeeded())
	{
		PieceMesh->SetMaterial(0, BasicMaterial.Object);
		TopDisc->SetMaterial(0, BasicMaterial.Object);
		OuterTrim->SetMaterial(0, BasicMaterial.Object);
		SideBand->SetMaterial(0, BasicMaterial.Object);
		Underglow->SetMaterial(0, BasicMaterial.Object);
		SelectionHalo->SetMaterial(0, BasicMaterial.Object);
		CenterPip->SetMaterial(0, BasicMaterial.Object);
		InnerRing->SetMaterial(0, BasicMaterial.Object);
		CorePlate->SetMaterial(0, BasicMaterial.Object);
		LowerTrim->SetMaterial(0, BasicMaterial.Object);
		UpperShoulder->SetMaterial(0, BasicMaterial.Object);
		LowerShoulder->SetMaterial(0, BasicMaterial.Object);
		TopBezel->SetMaterial(0, BasicMaterial.Object);
		CoreBezel->SetMaterial(0, BasicMaterial.Object);
		SignatureRing->SetMaterial(0, BasicMaterial.Object);
		SignatureInset->SetMaterial(0, BasicMaterial.Object);
	}
	if (EmissiveMaterial.Succeeded())
	{
		// Top-facing hardware is colored metal, not a collection of light sources.
		// Emission is reserved for the subtle underglow and explicit selection halo.
		Underglow->SetMaterial(0, EmissiveMaterial.Object);
		SelectionHalo->SetMaterial(0, EmissiveMaterial.Object);
	}

	for (UStaticMeshComponent* VisualMesh : {
		TopDisc.Get(), OuterTrim.Get(), SideBand.Get(), Underglow.Get(), SelectionHalo.Get(), CenterPip.Get(),
		InnerRing.Get(), CorePlate.Get(), LowerTrim.Get(), UpperShoulder.Get(), LowerShoulder.Get(),
		TopBezel.Get(), CoreBezel.Get(), SignatureRing.Get(), SignatureInset.Get()})
	{
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualMesh->SetGenerateOverlapEvents(false);
		VisualMesh->SetCanEverAffectNavigation(false);
	}
	SelectionHalo->SetCastShadow(false);
	SelectionHalo->SetVisibility(false);
	CenterPip->SetCastShadow(false);
	InnerRing->SetCastShadow(false);
	CorePlate->SetCastShadow(false);
	LowerTrim->SetCastShadow(false);
	UpperShoulder->SetCastShadow(false);
	LowerShoulder->SetCastShadow(false);
	TopBezel->SetCastShadow(false);
	CoreBezel->SetCastShadow(false);
	SignatureRing->SetCastShadow(false);
	SignatureInset->SetCastShadow(false);
	SideBand->SetCastShadow(false);
	Underglow->SetCastShadow(false);
	OuterTrim->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	constexpr int32 DetailCount = 16;
	TopTicks.Reserve(DetailCount);
	SideLugs.Reserve(DetailCount);
	for (int32 Index = 0; Index < DetailCount; ++Index)
	{
		UStaticMeshComponent* TopTick = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("TopTick_%02d"), Index));
		TopTick->SetupAttachment(PieceMesh);
		UStaticMeshComponent* SideLug = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("SideLug_%02d"), Index));
		SideLug->SetupAttachment(PieceMesh);
		for (UStaticMeshComponent* Detail : {TopTick, SideLug})
		{
			if (CubeMesh.Succeeded())
			{
				Detail->SetStaticMesh(CubeMesh.Object);
			}
			if (BasicMaterial.Succeeded())
			{
				Detail->SetMaterial(0, BasicMaterial.Object);
			}
			Detail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Detail->SetGenerateOverlapEvents(false);
			Detail->SetCastShadow(false);
			Detail->SetCanEverAffectNavigation(false);
		}
		TopTicks.Add(TopTick);
		SideLugs.Add(SideLug);
	}

	// Small non-colliding bars build the top-face archetype emblems. Keeping
	// these as geometry makes the symbols sharp, emissive, and team-tintable
	// without introducing texture assets or affecting the physics body.
	constexpr int32 EmblemPartCount = 5;
	EmblemParts.Reserve(EmblemPartCount);
	for (int32 Index = 0; Index < EmblemPartCount; ++Index)
	{
		UStaticMeshComponent* EmblemPart = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("EmblemPart_%02d"), Index));
		EmblemPart->SetupAttachment(PieceMesh);
		if (CubeMesh.Succeeded())
		{
			EmblemPart->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicMaterial.Succeeded())
		{
			EmblemPart->SetMaterial(0, BasicMaterial.Object);
		}
		EmblemPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EmblemPart->SetGenerateOverlapEvents(false);
		EmblemPart->SetCastShadow(false);
		EmblemPart->SetCanEverAffectNavigation(false);
		EmblemPart->SetVisibility(false);
		EmblemParts.Add(EmblemPart);
	}

	AccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight"));
	AccentLight->SetupAttachment(PieceMesh);
	AccentLight->SetCastShadows(false);
	AccentLight->SetAttenuationRadius(145.0f);
	AccentLight->SetIntensity(0.0f);
	AccentLight->SetSourceRadius(10.0f);
	AccentLight->SetSpecularScale(0.0f);

	PieceMesh->BodyInstance.bLockXRotation = false;
	PieceMesh->BodyInstance.bLockYRotation = false;
	PieceMesh->BodyInstance.bLockZRotation = false;
	PieceMesh->BodyInstance.DOFMode = EDOFMode::SixDOF;
	PieceMesh->BodyInstance.bUseCCD = true;
	PieceMesh->SetEnableGravity(true);
	PieceMesh->SetSimulatePhysics(true);
	PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PieceMesh->SetCollisionObjectType(ECC_PhysicsBody);
	PieceMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PieceMesh->SetNotifyRigidBodyCollision(true);
	PieceMesh->SetGenerateOverlapEvents(false);
	PieceMesh->OnComponentHit.AddDynamic(this, &AFlickPiece::HandleMeshHit);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(PieceMesh);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(28.0f);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	Label->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCastShadow(false);
	Label->SetAbsolute(false, false, true);
	Label->SetVisibility(false);
	PlayerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PlayerLabel"));
	PlayerLabel->SetupAttachment(PieceMesh);
	PlayerLabel->SetHorizontalAlignment(EHTA_Center);
	PlayerLabel->SetVerticalAlignment(EVRTA_TextCenter);
	PlayerLabel->SetWorldSize(24.0f);
	PlayerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 19.0f));
	PlayerLabel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	PlayerLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlayerLabel->SetCastShadow(false);
	PlayerLabel->SetAbsolute(false, false, true);
	PlayerLabel->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UFont> LabelFont(TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
	if (LabelFont.Succeeded())
	{
		Label->SetFont(LabelFont.Object);
		PlayerLabel->SetFont(LabelFont.Object);
	}
}

void AFlickPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickPiece, Team);
	DOREPLIFETIME(AFlickPiece, Archetype);
	DOREPLIFETIME(AFlickPiece, PieceId);
	DOREPLIFETIME(AFlickPiece, OwningPlayerSlot);
	DOREPLIFETIME(AFlickPiece, bShowPlayerIdentity);
	DOREPLIFETIME(AFlickPiece, bEliminated);
	DOREPLIFETIME(AFlickPiece, bKickoffLocked);
	DOREPLIFETIME(AFlickPiece, bBobStriker);
	DOREPLIFETIME(AFlickPiece, bHighDetailVisualsEnabled);
	DOREPLIFETIME(AFlickPiece, PieceRadius);
	DOREPLIFETIME(AFlickPiece, PieceThickness);
	DOREPLIFETIME(AFlickPiece, PieceMassKg);
	DOREPLIFETIME(AFlickPiece, PieceFriction);
	DOREPLIFETIME(AFlickPiece, PieceRestitution);
	DOREPLIFETIME(AFlickPiece, LinearDamping);
	DOREPLIFETIME(AFlickPiece, AngularDamping);
	DOREPLIFETIME(AFlickPiece, LaunchSpeedMultiplier);
	DOREPLIFETIME(AFlickPiece, CenterOfMassOffsetZ);
	DOREPLIFETIME(AFlickPiece, bAllowEdgeTipping);
	DOREPLIFETIME(AFlickPiece, bUseContinuousCollisionDetection);
	DOREPLIFETIME(AFlickPiece, PositionSolverIterations);
	DOREPLIFETIME(AFlickPiece, VelocitySolverIterations);
}

void AFlickPiece::BeginPlay()
{
	Super::BeginPlay();
	ApplyPhysicsSettings();
	ApplyVisuals();
}

void AFlickPiece::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	VisualTime += DeltaSeconds;
	const bool bWasFlashing = HitFlashRemaining > 0.0f;
	HitFlashRemaining = FMath::Max(0.0f, HitFlashRemaining - DeltaSeconds);
	if (bSelected || bHovered || bKickoffLocked || bWasFlashing)
	{
		ApplyVisuals();
	}
	if (bWasFlashing && HitFlashRemaining <= 0.0f)
	{
		HitFlashStrength = 0.0f;
	}
}

void AFlickPiece::InitializePiece(
	const EFlickTeam InTeam,
	const int32 InPieceId,
	const float InRadius,
	const float InThickness,
	const EFlickPieceArchetype InArchetype,
	const bool bInBobStriker,
	const int32 InOwningPlayerSlot,
	const bool bInShowPlayerIdentity)
{
	Team = InTeam;
	PieceId = InPieceId;
	Archetype = InArchetype;
	bBobStriker = bInBobStriker;
	OwningPlayerSlot = FMath::Max(0, InOwningPlayerSlot);
	bShowPlayerIdentity = bInShowPlayerIdentity && !bInBobStriker;
	PieceRadius = InRadius;
	PieceThickness = InThickness;
	bEliminated = false;
	bSelected = false;
	bHovered = false;
	bKickoffLocked = false;
	HitFlashRemaining = 0.0f;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	PieceMesh->SetSimulatePhysics(true);
	PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Label->SetVisibility(true);

	const FVector NewScale(PieceRadius / 50.0f, PieceRadius / 50.0f, PieceThickness / 100.0f);
	PieceMesh->SetWorldScale3D(NewScale);
	UpdateVisualTransforms();

	ApplyPhysicsSettings();
	ApplyVisuals();
}

void AFlickPiece::ApplyPhysicsSettings()
{
	if (!PieceMesh)
	{
		return;
	}

	if (!RuntimePhysicalMaterial)
	{
		RuntimePhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("FlickPiecePhysicalMaterial"));
	}

	RuntimePhysicalMaterial->Friction = PieceFriction;
	RuntimePhysicalMaterial->Restitution = PieceRestitution;
	RuntimePhysicalMaterial->bOverrideFrictionCombineMode = true;
	RuntimePhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
	RuntimePhysicalMaterial->bOverrideRestitutionCombineMode = true;
	RuntimePhysicalMaterial->RestitutionCombineMode = EFrictionCombineMode::Average;

	PieceMesh->SetPhysMaterialOverride(RuntimePhysicalMaterial);
	PieceMesh->SetMassOverrideInKg(NAME_None, PieceMassKg, true);
	PieceMesh->SetCenterOfMass(FVector(0.0f, 0.0f, CenterOfMassOffsetZ));
	PieceMesh->SetLinearDamping(LinearDamping);
	PieceMesh->SetAngularDamping(AngularDamping);
	PieceMesh->SetEnableGravity(true);
	PieceMesh->BodyInstance.bLockXRotation = !bAllowEdgeTipping;
	PieceMesh->BodyInstance.bLockYRotation = !bAllowEdgeTipping;
	PieceMesh->BodyInstance.bLockZRotation = false;
	PieceMesh->BodyInstance.DOFMode = EDOFMode::SixDOF;
	PieceMesh->SetUseCCD(bUseContinuousCollisionDetection);
	PieceMesh->BodyInstance.SetPositionSolverIterationCount(static_cast<uint8>(FMath::Clamp(PositionSolverIterations, 1, 255)));
	PieceMesh->BodyInstance.SetVelocitySolverIterationCount(static_cast<uint8>(FMath::Clamp(VelocitySolverIterations, 1, 255)));
}

void AFlickPiece::Launch(const FVector& Direction, const float NormalizedPower, const float MaxLaunchSpeed)
{
	if (bEliminated || !PieceMesh || !PieceMesh->IsSimulatingPhysics())
	{
		return;
	}

	FVector LaunchDirection = Direction;
	LaunchDirection.Z = 0.0f;
	LaunchDirection.Normalize();

	const float LaunchSpeed = FMath::Max(0.0f, MaxLaunchSpeed) * FMath::Clamp(NormalizedPower, 0.0f, 1.0f) * LaunchSpeedMultiplier;
	PieceMesh->WakeRigidBody();
	PieceMesh->AddImpulse(LaunchDirection * LaunchSpeed, NAME_None, true);
	ForceNetUpdate();
}

void AFlickPiece::ApplyTabletopSelfRighting(
	const float TorqueStrength,
	const float DampingStrength,
	const float MinimumTiltDegrees)
{
	if (bEliminated || !PieceMesh || !PieceMesh->IsSimulatingPhysics())
	{
		return;
	}

	const FVector PieceUp = PieceMesh->GetUpVector().GetSafeNormal();
	const FVector TargetNormal = PieceUp.Z >= 0.0f ? FVector::UpVector : FVector::DownVector;
	const float Alignment = FMath::Clamp(FVector::DotProduct(PieceUp, TargetNormal), 0.0f, 1.0f);
	const float TiltRadians = FMath::Acos(Alignment);
	if (TiltRadians < FMath::DegreesToRadians(FMath::Max(0.0f, MinimumTiltDegrees)))
	{
		return;
	}

	const FVector TiltAxis = FVector::CrossProduct(PieceUp, TargetNormal).GetSafeNormal();
	if (TiltAxis.IsNearlyZero())
	{
		return;
	}

	const FVector AngularVelocity = PieceMesh->GetPhysicsAngularVelocityInRadians();
	const FVector TiltingAngularVelocity = AngularVelocity
		- TargetNormal * FVector::DotProduct(AngularVelocity, TargetNormal);
	const FVector CorrectiveAcceleration = TiltAxis * FMath::Max(0.0f, TorqueStrength) * TiltRadians
		- TiltingAngularVelocity * FMath::Max(0.0f, DampingStrength);
	PieceMesh->WakeRigidBody();
	PieceMesh->AddTorqueInRadians(CorrectiveAcceleration, NAME_None, true);
}

void AFlickPiece::ApplyTabletopFlightContainment(
	const float SurfaceZ,
	const float MaximumUpwardSpeed,
	const float DownwardAcceleration)
{
	if (bEliminated || !PieceMesh || !PieceMesh->IsSimulatingPhysics())
	{
		return;
	}

	FVector Velocity = PieceMesh->GetPhysicsLinearVelocity();
	const float SafeMaximumUpwardSpeed = FMath::Max(0.0f, MaximumUpwardSpeed);
	if (Velocity.Z > SafeMaximumUpwardSpeed)
	{
		Velocity.Z = SafeMaximumUpwardSpeed;
		PieceMesh->SetPhysicsLinearVelocity(Velocity);
	}

	const float RestingCenterZ = SurfaceZ + PieceThickness * 0.5f;
	if (PieceMesh->GetComponentLocation().Z > RestingCenterZ + PieceThickness * 0.35f)
	{
		PieceMesh->AddForce(
			FVector(0.0f, 0.0f, -FMath::Max(0.0f, DownwardAcceleration)),
			NAME_None,
			true);
	}
}

void AFlickPiece::SettleFlatOnTabletop(const float SurfaceZ)
{
	if (bEliminated || !PieceMesh || !PieceMesh->IsSimulatingPhysics())
	{
		return;
	}

	const FVector CurrentLocation = PieceMesh->GetComponentLocation();
	const FRotator CurrentRotation = PieceMesh->GetComponentRotation();
	PieceMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PieceMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PieceMesh->SetWorldLocationAndRotation(
		FVector(CurrentLocation.X, CurrentLocation.Y, SurfaceZ + PieceThickness * 0.5f),
		FRotator(0.0f, CurrentRotation.Yaw, 0.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	PieceMesh->PutRigidBodyToSleep();
	ForceNetUpdate();
}

void AFlickPiece::BeginReplayPresentation()
{
	if (bReplayPresentationActive || !PieceMesh)
	{
		return;
	}

	PreReplayTransform = GetActorTransform();
	bReplayPresentationActive = true;
	PieceMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PieceMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PieceMesh->SetSimulatePhysics(false);
	PieceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetActorEnableCollision(false);
}

void AFlickPiece::ApplyReplayPresentation(const FTransform& Transform, const bool bVisible)
{
	if (!bReplayPresentationActive || !PieceMesh)
	{
		return;
	}

	SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(!bVisible);
}

void AFlickPiece::EndReplayPresentation()
{
	if (!bReplayPresentationActive || !PieceMesh)
	{
		return;
	}

	bReplayPresentationActive = false;
	SetActorTransform(PreReplayTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (bEliminated)
	{
		ApplyEliminatedState();
		return;
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PieceMesh->SetSimulatePhysics(true);
	PieceMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PieceMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PieceMesh->PutRigidBodyToSleep();
}

void AFlickPiece::Eliminate()
{
	if (bEliminated)
	{
		return;
	}

	bEliminated = true;
	bSelected = false;
	bHovered = false;
	bKickoffLocked = false;
	ApplyEliminatedState();
	ForceNetUpdate();
	UE_LOG(LogFlick, Log, TEXT("Piece %d eliminated"), PieceId);
}

void AFlickPiece::OnRep_PieceConfiguration()
{
	const FVector NewScale(PieceRadius / 50.0f, PieceRadius / 50.0f, PieceThickness / 100.0f);
	PieceMesh->SetWorldScale3D(NewScale);
	UpdateVisualTransforms();
	ApplyPhysicsSettings();
	ApplyVisuals();
}

void AFlickPiece::OnRep_Eliminated()
{
	if (bEliminated)
	{
		ApplyEliminatedState();
	}
	else
	{
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		PieceMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PieceMesh->SetSimulatePhysics(true);
		ApplyVisuals();
	}
}

void AFlickPiece::ApplyEliminatedState()
{
	if (PieceMesh)
	{
		PieceMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		PieceMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		PieceMesh->SetSimulatePhysics(false);
		PieceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void AFlickPiece::SetSelected(const bool bInSelected)
{
	if (bEliminated)
	{
		bSelected = false;
	}
	else
	{
		bSelected = bInSelected;
	}

	ApplyVisuals();
}

void AFlickPiece::SetHovered(const bool bInHovered)
{
	bHovered = !bEliminated && bInHovered;
	ApplyVisuals();
}

void AFlickPiece::SetKickoffLocked(const bool bInKickoffLocked)
{
	bKickoffLocked = !bEliminated && bInKickoffLocked;
	ApplyVisuals();
	ForceNetUpdate();
}

void AFlickPiece::OnRep_KickoffLocked()
{
	ApplyVisuals();
}

void AFlickPiece::OnRep_HighDetailVisuals()
{
	if (bHighDetailVisualsEnabled)
	{
		EnableTestArenaVisuals();
	}
}

void AFlickPiece::PlayImpactFlash(const float Strength)
{
	if (bEliminated)
	{
		return;
	}

	HitFlashStrength = FMath::Clamp(FMath::Max(HitFlashStrength, Strength), 0.0f, 1.0f);
	HitFlashRemaining = FMath::Lerp(0.08f, 0.2f, HitFlashStrength);
	ApplyVisuals();
}

bool AFlickPiece::IsSelectableBy(const EFlickTeam InTeam) const
{
	return !bEliminated && Team == InTeam;
}

FVector AFlickPiece::GetLinearVelocity() const
{
	return PieceMesh ? PieceMesh->GetPhysicsLinearVelocity() : FVector::ZeroVector;
}

FVector AFlickPiece::GetAngularVelocityDegrees() const
{
	return PieceMesh ? PieceMesh->GetPhysicsAngularVelocityInDegrees() : FVector::ZeroVector;
}

void AFlickPiece::HandleMeshHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const FVector NormalImpulse,
	const FHitResult& Hit)
{
	AFlickPiece* OtherPiece = Cast<AFlickPiece>(OtherActor);
	if (bEliminated)
	{
		return;
	}

	if (!OtherPiece)
	{
		const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
		// The tabletop continuously supports every puck. Only lateral contacts
		// belong to a rail/rim and should produce an impact voice.
		if (FMath::Abs(ImpactNormal.Z) > 0.65f)
		{
			return;
		}
		const float ImpulseVelocityChange = NormalImpulse.Size() / FMath::Max(PieceMassKg, 0.1f);
		const float ClosingSpeed = FMath::Abs(FVector::DotProduct(GetLinearVelocity(), ImpactNormal));
		const float ImpactVelocityChange = FMath::Max(ImpulseVelocityChange, ClosingSpeed);
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		if (ImpactVelocityChange < 45.0f || Now - LastArenaImpactNotificationTime < 0.09f)
		{
			return;
		}
		LastArenaImpactNotificationTime = Now;
		if (AFlickGameMode* FlickGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr)
		{
			const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
			FlickGameMode->NotifyArenaImpact(this, OtherComponent, ImpactLocation, ImpactVelocityChange);
		}
		return;
	}

	if (OtherPiece == this || OtherPiece->IsEliminated())
	{
		return;
	}

	const float VelocityChange = NormalImpulse.Size() / FMath::Max(PieceMassKg, 0.1f);
	if (VelocityChange < 35.0f)
	{
		return;
	}

	const float VisualStrength = FMath::Clamp((VelocityChange - 35.0f) / 900.0f, 0.05f, 1.0f);
	PlayImpactFlash(VisualStrength);
	OtherPiece->PlayImpactFlash(VisualStrength);

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (PieceId >= OtherPiece->GetPieceId() || Now - LastImpactNotificationTime < 0.08f)
	{
		return;
	}
	LastImpactNotificationTime = Now;

	if (AFlickGameMode* FlickGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr)
	{
		const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero()
			? (GetActorLocation() + OtherPiece->GetActorLocation()) * 0.5f
			: FVector(Hit.ImpactPoint);
		FlickGameMode->NotifyPieceImpact(this, OtherPiece, ImpactLocation, VelocityChange);
	}
}

void AFlickPiece::EnsureVisualMaterials()
{
	if (!BodyMaterial && PieceMesh)
	{
		BodyMaterial = PieceMesh->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!TopMaterial && TopDisc)
	{
		TopMaterial = TopDisc->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!OuterTrimMaterial && OuterTrim)
	{
		OuterTrimMaterial = OuterTrim->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!SideBandMaterial && SideBand)
	{
		SideBandMaterial = SideBand->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!UnderglowMaterial && Underglow)
	{
		UnderglowMaterial = Underglow->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!HaloMaterial && SelectionHalo)
	{
		HaloMaterial = SelectionHalo->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!PipMaterial && CenterPip)
	{
		PipMaterial = CenterPip->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!InnerRingMaterial && InnerRing)
	{
		InnerRingMaterial = InnerRing->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!CorePlateMaterial && CorePlate)
	{
		CorePlateMaterial = CorePlate->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!LowerTrimMaterial && LowerTrim)
	{
		LowerTrimMaterial = LowerTrim->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!UpperShoulderMaterial && UpperShoulder)
	{
		UpperShoulderMaterial = UpperShoulder->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!LowerShoulderMaterial && LowerShoulder)
	{
		LowerShoulderMaterial = LowerShoulder->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!TopBezelMaterial && TopBezel)
	{
		TopBezelMaterial = TopBezel->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!CoreBezelMaterial && CoreBezel)
	{
		CoreBezelMaterial = CoreBezel->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!SignatureRingMaterial && SignatureRing)
	{
		SignatureRingMaterial = SignatureRing->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (!SignatureInsetMaterial && SignatureInset)
	{
		SignatureInsetMaterial = SignatureInset->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (TopTickMaterials.Num() != TopTicks.Num())
	{
		TopTickMaterials.Reset();
		for (UStaticMeshComponent* Detail : TopTicks)
		{
			TopTickMaterials.Add(Detail ? Detail->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	if (SideLugMaterials.Num() != SideLugs.Num())
	{
		SideLugMaterials.Reset();
		for (UStaticMeshComponent* Detail : SideLugs)
		{
			SideLugMaterials.Add(Detail ? Detail->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
	if (EmblemPartMaterials.Num() != EmblemParts.Num())
	{
		EmblemPartMaterials.Reset();
		for (UStaticMeshComponent* EmblemPart : EmblemParts)
		{
			EmblemPartMaterials.Add(EmblemPart ? EmblemPart->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
		}
	}
}

void AFlickPiece::UpdateVisualTransforms()
{
	const float SafeThickness = FMath::Max(PieceThickness, 1.0f);
	const float ParentZScale = SafeThickness / 100.0f;
	const float ParentXYScale = FMath::Max(PieceRadius / 50.0f, 0.01f);
	float TopScale = 0.8f;
	float PipScale = 0.14f;
	float SignatureScale = 0.66f;
	float SignatureInsetScale = 0.57f;
	float DetailRadiusFactor = 0.82f;
	float OuterTrimScale = 0.95f;
	float SideBandScale = 1.018f;
	float SideBandHeight = 0.44f;
	float ShoulderScale = 1.045f;
	float TickLength = 9.0f;
	float TickWidth = 3.0f;
	float LugLength = 7.0f;
	float LugWidth = 3.2f;
	float LugHeight = 0.4f;
	int32 TopDetailStride = 1;
	int32 SideDetailStride = 2;
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy:
		TopScale = 0.79f;
		PipScale = 0.2f;
		SignatureScale = 0.63f;
		SignatureInsetScale = 0.54f;
		DetailRadiusFactor = 0.81f;
		SideBandHeight = 0.54f;
		TickLength = 14.0f;
		TickWidth = 5.0f;
		LugLength = 12.0f;
		LugWidth = 5.0f;
		LugHeight = 0.54f;
		TopDetailStride = 2;
		break;
	case EFlickPieceArchetype::Striker:
		TopScale = 0.84f;
		PipScale = 0.055f;
		SignatureScale = 0.69f;
		SignatureInsetScale = 0.59f;
		DetailRadiusFactor = 0.84f;
		TickLength = 7.0f;
		TickWidth = 2.2f;
		LugLength = 4.5f;
		LugWidth = 2.4f;
		LugHeight = 0.38f;
		SideDetailStride = 1;
		break;
	case EFlickPieceArchetype::Grippy:
		TopScale = 0.8f;
		PipScale = 0.06f;
		SignatureScale = 0.67f;
		SignatureInsetScale = 0.57f;
		DetailRadiusFactor = 0.8f;
		TickLength = 7.0f;
		TickWidth = 3.8f;
		LugLength = 5.0f;
		LugWidth = 4.2f;
		LugHeight = 0.55f;
		SideDetailStride = 1;
		break;
	case EFlickPieceArchetype::Slider:
		TopScale = 0.83f;
		PipScale = 0.055f;
		SignatureScale = 0.69f;
		SignatureInsetScale = 0.59f;
		DetailRadiusFactor = 0.85f;
		SideBandHeight = 0.36f;
		TickLength = 11.0f;
		TickWidth = 2.2f;
		LugLength = 8.0f;
		LugWidth = 2.8f;
		LugHeight = 0.3f;
		break;
	case EFlickPieceArchetype::Blocker:
		TopScale = 0.87f;
		PipScale = 0.21f;
		SignatureScale = 0.73f;
		SignatureInsetScale = 0.63f;
		DetailRadiusFactor = 0.82f;
		OuterTrimScale = 0.965f;
		SideBandHeight = 0.34f;
		TickLength = 16.0f;
		TickWidth = 5.0f;
		LugLength = 14.0f;
		LugWidth = 4.8f;
		LugHeight = 0.32f;
		TopDetailStride = 2;
		SideDetailStride = 4;
		break;
	case EFlickPieceArchetype::Compact:
		TopScale = 0.84f;
		PipScale = 0.12f;
		SignatureScale = 0.65f;
		SignatureInsetScale = 0.55f;
		DetailRadiusFactor = 0.75f;
		SideBandHeight = 0.52f;
		TickLength = 6.0f;
		TickWidth = 2.5f;
		LugLength = 5.0f;
		LugWidth = 2.6f;
		LugHeight = 0.56f;
		TopDetailStride = 2;
		SideDetailStride = 1;
		break;
	case EFlickPieceArchetype::Bouncer:
		TopScale = 0.81f;
		PipScale = 0.12f;
		SignatureScale = 0.68f;
		SignatureInsetScale = 0.58f;
		DetailRadiusFactor = 0.8f;
		TickLength = 13.0f;
		TickWidth = 4.0f;
		LugLength = 7.0f;
		LugWidth = 4.0f;
		LugHeight = 0.42f;
		TopDetailStride = 2;
		break;
	case EFlickPieceArchetype::Toppler:
		TopScale = 0.82f;
		PipScale = 0.18f;
		SignatureScale = 0.69f;
		SignatureInsetScale = 0.59f;
		DetailRadiusFactor = 0.82f;
		SideBandHeight = 0.58f;
		TickLength = 11.0f;
		TickWidth = 4.5f;
		LugLength = 9.0f;
		LugWidth = 3.6f;
		LugHeight = 0.68f;
		break;
	case EFlickPieceArchetype::Standard:
	default:
		break;
	}
	if (bBobStriker)
	{
		TopScale = 0.64f;
		PipScale = 0.25f;
		SignatureScale = 0.56f;
		SignatureInsetScale = 0.48f;
	}

	OuterTrim->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 0.62f) / ParentZScale));
	OuterTrim->SetRelativeScale3D(FVector(OuterTrimScale, OuterTrimScale, 1.25f / SafeThickness));
	TopDisc->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 1.9f) / ParentZScale));
	TopDisc->SetRelativeScale3D(FVector(TopScale, TopScale, 1.6f / SafeThickness));
	SideBand->SetRelativeLocation(FVector(0.0f, 0.0f, -SafeThickness * 0.08f / ParentZScale));
	SideBand->SetRelativeScale3D(FVector(SideBandScale, SideBandScale, SideBandHeight));
	const float ShoulderOffset = SafeThickness * 0.31f / ParentZScale;
	UpperShoulder->SetRelativeLocation(FVector(0.0f, 0.0f, ShoulderOffset));
	UpperShoulder->SetRelativeScale3D(FVector(ShoulderScale, ShoulderScale, 0.14f));
	LowerShoulder->SetRelativeLocation(FVector(0.0f, 0.0f, -ShoulderOffset));
	LowerShoulder->SetRelativeScale3D(FVector(ShoulderScale, ShoulderScale, 0.14f));
	Underglow->SetRelativeLocation(FVector(0.0f, 0.0f, (-SafeThickness * 0.5f + 0.9f) / ParentZScale));
	Underglow->SetRelativeScale3D(FVector(1.11f, 1.11f, 1.2f / SafeThickness));
	LowerTrim->SetRelativeLocation(FVector(0.0f, 0.0f, (-SafeThickness * 0.5f + 2.2f) / ParentZScale));
	LowerTrim->SetRelativeScale3D(FVector(1.045f, 1.045f, 1.5f / SafeThickness));
	SelectionHalo->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 0.7f) / ParentZScale));
	BaseHaloRelativeScale = FVector(1.16f, 1.16f, 1.0f / SafeThickness);
	SelectionHalo->SetRelativeScale3D(BaseHaloRelativeScale);
	TopBezel->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 1.28f) / ParentZScale));
	const float TopBezelScale = FMath::Clamp(TopScale + 0.095f, 0.74f, 0.97f);
	TopBezel->SetRelativeScale3D(FVector(TopBezelScale, TopBezelScale, 1.35f / SafeThickness));
	SignatureRing->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 2.75f) / ParentZScale));
	SignatureRing->SetRelativeScale3D(FVector(SignatureScale, SignatureScale, 0.9f / SafeThickness));
	SignatureInset->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 3.08f) / ParentZScale));
	SignatureInset->SetRelativeScale3D(FVector(SignatureInsetScale, SignatureInsetScale, 0.9f / SafeThickness));
	InnerRing->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 3.56f) / ParentZScale));
	const float InnerRingScale = bShowPlayerIdentity ? 0.52f : FMath::Max(0.36f, SignatureInsetScale - 0.085f);
	InnerRing->SetRelativeScale3D(FVector(InnerRingScale, InnerRingScale, 0.72f / SafeThickness));
	CoreBezel->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 3.84f) / ParentZScale));
	const float CoreBezelScale = bShowPlayerIdentity ? 0.455f : FMath::Max(0.31f, InnerRingScale - 0.062f);
	CoreBezel->SetRelativeScale3D(FVector(CoreBezelScale, CoreBezelScale, 0.72f / SafeThickness));
	CorePlate->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 4.12f) / ParentZScale));
	const float CorePlateScale = bShowPlayerIdentity ? 0.39f : FMath::Max(0.26f, CoreBezelScale - 0.07f);
	CorePlate->SetRelativeScale3D(FVector(CorePlateScale, CorePlateScale, 0.76f / SafeThickness));
	CenterPip->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 4.52f) / ParentZScale));
	CenterPip->SetRelativeScale3D(FVector(PipScale, PipScale, 1.05f / SafeThickness));
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 5.5f) / ParentZScale));
	if (bShowPlayerIdentity)
	{
		Label->SetRelativeLocation(FVector(
			0.0f,
			-PieceRadius * 0.43f / ParentXYScale,
			(SafeThickness * 0.5f + 5.2f) / ParentZScale));
	}
	PlayerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 5.55f) / ParentZScale));

	for (int32 Index = 0; Index < TopTicks.Num(); ++Index)
	{
		const bool bPlayerPatternTick = OwningPlayerSlot == 0
			? Index == 0
			: OwningPlayerSlot == 1
				? Index == 0 || Index == TopTicks.Num() / 2
				: Index % 4 == 0;
		const bool bTopDetailVisible = Index % FMath::Max(1, TopDetailStride) == 0;
		const bool bSideDetailVisible = Index % FMath::Max(1, SideDetailStride) == 0;
		TopTicks[Index]->SetVisibility(bShowPlayerIdentity ? bPlayerPatternTick : bTopDetailVisible);
		SideLugs[Index]->SetVisibility(bSideDetailVisible);
		const float Angle = 2.0f * PI * static_cast<float>(Index) / FMath::Max(1, TopTicks.Num());
		const float DetailRadius = PieceRadius * DetailRadiusFactor;
		TopTicks[Index]->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * DetailRadius / ParentXYScale,
			FMath::Sin(Angle) * DetailRadius / ParentXYScale,
			(SafeThickness * 0.5f + 3.08f) / ParentZScale));
		TopTicks[Index]->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		const float EffectiveTickLength = bShowPlayerIdentity ? 19.0f : TickLength * (Index % 2 == 0 ? 1.0f : 0.82f);
		const float EffectiveTickWidth = bShowPlayerIdentity ? 6.5f : TickWidth;
		TopTicks[Index]->SetRelativeScale3D(FVector(
			EffectiveTickLength / (100.0f * ParentXYScale),
			EffectiveTickWidth / (100.0f * ParentXYScale),
			0.78f / (100.0f * ParentZScale)));

		const float LugRadius = PieceRadius * 0.99f;
		SideLugs[Index]->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * LugRadius / ParentXYScale,
			FMath::Sin(Angle) * LugRadius / ParentXYScale,
			-SafeThickness * 0.05f / ParentZScale));
		SideLugs[Index]->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		SideLugs[Index]->SetRelativeScale3D(FVector(
			LugLength / (100.0f * ParentXYScale),
			LugWidth / (100.0f * ParentXYScale),
			SafeThickness * LugHeight / (100.0f * ParentZScale)));
	}

	for (UStaticMeshComponent* EmblemPart : EmblemParts)
	{
		if (EmblemPart)
		{
			EmblemPart->SetVisibility(false);
		}
	}
	const float SymbolScale = FMath::Clamp(PieceRadius / 45.0f, 0.76f, 1.24f);
	const auto ConfigureEmblemPart = [this, ParentXYScale, ParentZScale, SafeThickness, SymbolScale](
		const int32 Index,
		const FVector2D& Offset,
		const float RotationDegrees,
		const float Length,
		const float Width)
	{
		if (!EmblemParts.IsValidIndex(Index) || !EmblemParts[Index])
		{
			return;
		}
		UStaticMeshComponent* Part = EmblemParts[Index];
		Part->SetVisibility(true);
		Part->SetRelativeLocation(FVector(
			Offset.X * SymbolScale / ParentXYScale,
			Offset.Y * SymbolScale / ParentXYScale,
			(SafeThickness * 0.5f + 5.18f) / ParentZScale));
		Part->SetRelativeRotation(FRotator(0.0f, RotationDegrees, 0.0f));
		Part->SetRelativeScale3D(FVector(
			Length * SymbolScale / (100.0f * ParentXYScale),
			Width * SymbolScale / (100.0f * ParentXYScale),
			0.72f / (100.0f * ParentZScale)));
	};

	if (!bShowPlayerIdentity && !bBobStriker)
	{
		switch (Archetype)
		{
		case EFlickPieceArchetype::Slider:
		{
			const TArray<FVector2D> ShieldPoints = {
				FVector2D(-8.0f, -6.0f), FVector2D(8.0f, -6.0f), FVector2D(7.0f, 2.0f),
				FVector2D(0.0f, 10.0f), FVector2D(-7.0f, 2.0f)};
			for (int32 Index = 0; Index < ShieldPoints.Num(); ++Index)
			{
				const FVector2D Start = ShieldPoints[Index];
				const FVector2D End = ShieldPoints[(Index + 1) % ShieldPoints.Num()];
				const FVector2D Delta = End - Start;
				ConfigureEmblemPart(Index, (Start + End) * 0.5f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), Delta.Size(), 2.4f);
			}
			break;
		}
		case EFlickPieceArchetype::Grippy:
			ConfigureEmblemPart(0, FVector2D(-4.0f, 1.5f), 45.0f, 12.0f, 3.1f);
			ConfigureEmblemPart(1, FVector2D(4.0f, 1.5f), 135.0f, 12.0f, 3.1f);
			ConfigureEmblemPart(2, FVector2D(-4.0f, -5.0f), 45.0f, 12.0f, 3.1f);
			ConfigureEmblemPart(3, FVector2D(4.0f, -5.0f), 135.0f, 12.0f, 3.1f);
			break;
		case EFlickPieceArchetype::Striker:
		{
			TArray<FVector2D> StarPoints;
			for (int32 PointIndex = 0; PointIndex < 5; ++PointIndex)
			{
				const float Angle = -PI * 0.5f + 2.0f * PI * static_cast<float>(PointIndex) / 5.0f;
				StarPoints.Add(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 10.5f);
			}
			const int32 StarOrder[5] = {0, 2, 4, 1, 3};
			for (int32 Index = 0; Index < 5; ++Index)
			{
				const FVector2D Start = StarPoints[StarOrder[Index]];
				const FVector2D End = StarPoints[StarOrder[(Index + 1) % 5]];
				const FVector2D Delta = End - Start;
				ConfigureEmblemPart(Index, (Start + End) * 0.5f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), Delta.Size(), 2.5f);
			}
			break;
		}
		case EFlickPieceArchetype::Bouncer:
		{
			const TArray<FVector2D> DiamondPoints = {
				FVector2D(0.0f, -9.0f), FVector2D(9.0f, 0.0f),
				FVector2D(0.0f, 9.0f), FVector2D(-9.0f, 0.0f)};
			for (int32 Index = 0; Index < DiamondPoints.Num(); ++Index)
			{
				const FVector2D Start = DiamondPoints[Index];
				const FVector2D End = DiamondPoints[(Index + 1) % DiamondPoints.Num()];
				const FVector2D Delta = End - Start;
				ConfigureEmblemPart(Index, (Start + End) * 0.5f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), Delta.Size(), 2.8f);
			}
			break;
		}
		default:
			break;
		}
	}
	AccentLight->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 22.0f) / ParentZScale));
}

bool AFlickPiece::HasTestArenaVisuals() const
{
	return WorkshopMesh && WorkshopMesh->GetStaticMesh() != nullptr;
}

void AFlickPiece::EnableTestArenaVisuals()
{
	if (HasTestArenaVisuals()) return;
	bHighDetailVisualsEnabled = true;
	if (HasAuthority())
	{
		ForceNetUpdate();
	}
	if (GetNetMode() == NM_DedicatedServer) return;
	static const TCHAR* Names[] = {TEXT("Standard"), TEXT("Heavy"), TEXT("Striker"),
		TEXT("Grippy"), TEXT("Slider"), TEXT("Blocker"), TEXT("Compact"), TEXT("Bouncer"), TEXT("Toppler")};
	const int32 Index = static_cast<int32>(Archetype);
	if (Index < 0 || Index >= UE_ARRAY_COUNT(Names)) return;
	bUsingHighDetailPlayerIdentity = false;
	FString HighDetailPath;
	if (bShowPlayerIdentity && OwningPlayerSlot >= 0 && OwningPlayerSlot < 3)
	{
		const FString Identity = FString::Printf(TEXT("P%d"), OwningPlayerSlot + 1);
		HighDetailPath = FString::Printf(
			TEXT("/Game/TestArena/Pucks/HighDetail/PlayerIdentity/%s/SM_Puck_%s_%s_HighDetail.SM_Puck_%s_%s_HighDetail"),
			*Identity, Names[Index], *Identity, Names[Index], *Identity);
	}
	else
	{
		HighDetailPath = Archetype == EFlickPieceArchetype::Standard
		? TEXT("/Game/TestArena/Pucks/PrototypeStandard/SM_Puck_Standard_Blue_Prototype.SM_Puck_Standard_Blue_Prototype")
		: FString::Printf(
			TEXT("/Game/TestArena/Pucks/HighDetail/SM_Puck_%s_HighDetail.SM_Puck_%s_HighDetail"),
			Names[Index], Names[Index]);
	}
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *HighDetailPath);
	bUsingHighDetailPlayerIdentity = Mesh != nullptr && bShowPlayerIdentity;
	bUsingHighDetailPuck = Mesh != nullptr;
	if (!Mesh && bShowPlayerIdentity)
	{
		HighDetailPath = Archetype == EFlickPieceArchetype::Standard
			? TEXT("/Game/TestArena/Pucks/PrototypeStandard/SM_Puck_Standard_Blue_Prototype.SM_Puck_Standard_Blue_Prototype")
			: FString::Printf(
				TEXT("/Game/TestArena/Pucks/HighDetail/SM_Puck_%s_HighDetail.SM_Puck_%s_HighDetail"),
				Names[Index], Names[Index]);
		Mesh = LoadObject<UStaticMesh>(nullptr, *HighDetailPath);
		bUsingHighDetailPuck = Mesh != nullptr;
	}
	if (!Mesh)
	{
		const FString FallbackPath = FString::Printf(
			TEXT("/Game/TestArena/Pucks/SM_Puck_%s.SM_Puck_%s"), Names[Index], Names[Index]);
		Mesh = LoadObject<UStaticMesh>(nullptr, *FallbackPath);
	}
	if (!Mesh)
	{
		UE_LOG(LogFlick, Warning, TEXT("Test arena puck mesh missing: %s; keeping procedural visuals"), *HighDetailPath);
		return;
	}
	WorkshopMesh->SetStaticMesh(Mesh);
	WorkshopTeamMaterials.Reset();
	for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
	{
		const FString SlotName = Mesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString();
		const bool bTeamLightSlot = SlotName.Contains(TEXT("05_Team"))
			|| SlotName.Contains(TEXT("Cyan"));
		if (bTeamLightSlot)
		{
			if (bUsingHighDetailPuck && Team == EFlickTeam::Player2)
			{
				const TCHAR* OrangeMaterialPath = SlotName.Contains(TEXT("center"), ESearchCase::IgnoreCase)
					? TEXT("/Game/TestArena/Pucks/HighDetail/MI_Orange_Center_Emblem.MI_Orange_Center_Emblem")
					: TEXT("/Game/TestArena/Pucks/HighDetail/MI_Orange_Light_Diffuser.MI_Orange_Light_Diffuser");
				if (UMaterialInterface* OrangeMaterial = LoadObject<UMaterialInterface>(nullptr, OrangeMaterialPath))
				{
					WorkshopMesh->SetMaterial(Slot, OrangeMaterial);
				}
			}
			if (UMaterialInstanceDynamic* TeamMaterial = WorkshopMesh->CreateDynamicMaterialInstance(Slot))
			{
				WorkshopTeamMaterials.Add(TeamMaterial);
			}
		}
		else if (bBobStriker)
		{
			// BOB's controllable strikers retain the Standard silhouette and
			// physics, but receive a brighter team-tinted metal crown. The rack
			// pucks keep the regular Standard treatment, so the shot pieces are
			// recognizable without adding another broad light source to the board.
			const bool bSilverCrown = SlotName.Contains(TEXT("brushed_silver"), ESearchCase::IgnoreCase)
				|| SlotName.Contains(TEXT("brushed silver"), ESearchCase::IgnoreCase)
				|| SlotName.Contains(TEXT("Machined_edge"), ESearchCase::IgnoreCase)
				|| SlotName.Contains(TEXT("Machined edge"), ESearchCase::IgnoreCase);
			const bool bGraphiteHousing = SlotName.Contains(TEXT("Graphite"), ESearchCase::IgnoreCase);
			if (bSilverCrown || bGraphiteHousing)
			{
				if (UMaterialInstanceDynamic* StrikerMaterial = WorkshopMesh->CreateDynamicMaterialInstance(Slot))
				{
					const FLinearColor CrownColor = FMath::Lerp(
						FLinearColor(0.78f, 0.84f, 0.91f, 1.0f), GetTeamColor(Team), 0.20f);
					StrikerMaterial->SetVectorParameterValue(
						TEXT("BaseColor"),
						bSilverCrown ? CrownColor : FLinearColor(0.065f, 0.085f, 0.115f, 1.0f));
					StrikerMaterial->SetScalarParameterValue(TEXT("Metallic"), bSilverCrown ? 0.98f : 0.72f);
					StrikerMaterial->SetScalarParameterValue(TEXT("Roughness"), bSilverCrown ? 0.16f : 0.24f);
					StrikerMaterial->SetScalarParameterValue(TEXT("SurfaceLift"), bSilverCrown ? 0.40f : 0.10f);
				}
			}
		}
	}
	// Convert from the root cylinder's gameplay scale to the high-detail mesh's authored
	// Classic dimensions. This is normally an identity transform, but it also scales the
	// Standard art down correctly for BOB's smaller physics pucks.
	const FFlickModeRules& ClassicRules = FlickModeRules::Get(EFlickMatchVariant::Classic);
	const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(Archetype);
	const float AuthoredRadius = ClassicRules.PieceRadius * ArchetypeRules.RadiusMultiplier;
	const float AuthoredThickness = ClassicRules.PieceThickness * ArchetypeRules.ThicknessMultiplier;
	WorkshopMesh->SetRelativeScale3D(FVector(
		50.0f / FMath::Max(AuthoredRadius, 1.0f),
		50.0f / FMath::Max(AuthoredRadius, 1.0f),
		100.0f / FMath::Max(AuthoredThickness, 1.0f)));
	// A compact, low-energy local light gives the machined rings a controlled
	// highlight and confines team colour to the deck immediately below the puck.
	AccentLight->SetAttenuationRadius(64.0f);
	AccentLight->SetSourceRadius(8.0f);
	AccentLight->SetSpecularScale(bUsingHighDetailPuck ? 0.36f : 0.28f);
	AccentLight->SetIndirectLightingIntensity(0.03f);
	AccentLight->SetVolumetricScatteringIntensity(0.0f);
	PieceMesh->SetCastShadow(false);
	ApplyVisuals();
	UE_LOG(LogFlick, Log, TEXT("Test arena Blender puck ready: %s, team %d"), Names[Index], static_cast<int32>(Team));
}

void AFlickPiece::UpdateTestArenaVisuals()
{
	if (!HasTestArenaVisuals()) return;
	PieceMesh->SetVisibility(false);
	for (USceneComponent* Child : PieceMesh->GetAttachChildren())
	{
		if (Child != WorkshopMesh && Child != SelectionHalo && Child != PlayerLabel && Child != AccentLight)
		{
			Child->SetVisibility(false);
		}
	}
	AccentLight->SetVisibility(!bEliminated);
	WorkshopMesh->SetVisibility(!bEliminated);
	if (!WorkshopTeamMaterials.IsEmpty())
	{
		const FLinearColor Color = Team == EFlickTeam::Player2
			? FLinearColor(1.0f, 0.18f, 0.003f) : FLinearColor(0.0f, 0.5f, 1.0f);
		const FLinearColor DiffuserBaseColor = Team == EFlickTeam::Player2
			? FLinearColor(0.125f, 0.022f, 0.002f) : FLinearColor(0.004f, 0.080f, 0.125f);
		const float Flash = FMath::Clamp(HitFlashRemaining / 0.2f, 0.0f, 1.0f) * HitFlashStrength;
		// The game camera runs below neutral exposure. HDR values in this range
		// retain saturated cores while crossing the restrained bloom threshold,
		// giving the authored strips a narrow LED halo rather than a blurry blob.
		for (UMaterialInstanceDynamic* TeamMaterial : WorkshopTeamMaterials)
		{
			if (!TeamMaterial) continue;
			TeamMaterial->SetVectorParameterValue(TEXT("TeamColor"), Color);
			if (bUsingHighDetailPuck)
			{
				TeamMaterial->SetVectorParameterValue(TEXT("BaseColor"), DiffuserBaseColor);
			}
			const float IdleEmission = bUsingHighDetailPuck ? (bBobStriker ? 7.5f : 6.0f) : 10.0f;
			const float HighlightEmission = bUsingHighDetailPuck ? (bBobStriker ? 9.5f : 8.0f) : 14.0f;
			TeamMaterial->SetScalarParameterValue(
				TEXT("Emission"), (bSelected || bHovered ? HighlightEmission : IdleEmission) + Flash);
		}
	}
	const float IdleLight = bUsingHighDetailPuck ? 8.0f : 11.0f;
	const float HighlightLight = bUsingHighDetailPuck ? 14.0f : 18.0f;
	AccentLight->SetIntensity(
		bEliminated ? 0.0f : (bSelected || bHovered || bKickoffLocked ? HighlightLight : IdleLight));
}

void AFlickPiece::ApplyVisuals()
{
	if (!PieceMesh || !TopDisc || !OuterTrim || !SideBand || !Underglow || !SelectionHalo || !CenterPip
		|| !InnerRing || !CorePlate || !LowerTrim || !UpperShoulder || !LowerShoulder
		|| !TopBezel || !CoreBezel || !SignatureRing || !SignatureInset || !Label || !PlayerLabel)
	{
		return;
	}

	EnsureVisualMaterials();
	const FLinearColor TeamColor = GetTeamColor(Team);
	const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(Archetype);
	const FLinearColor ArchetypeColor = ArchetypeRules.AccentColor;
	const FLinearColor VisualAccent = FlickPieceArchetypeRules::GetVisualAccent(Archetype, TeamColor);
	const FLinearColor PlayerIdentityColor = OwningPlayerSlot == 0
		? FLinearColor(0.92f, 0.96f, 1.0f, 1.0f)
		: OwningPlayerSlot == 1
			? FLinearColor(1.0f, 0.06f, 0.72f, 1.0f)
			: FLinearColor(0.58f, 1.0f, 0.02f, 1.0f);
	const float FlashAlpha = HitFlashRemaining > 0.0f
		? FMath::Clamp(HitFlashRemaining / 0.2f, 0.0f, 1.0f) * HitFlashStrength
		: 0.0f;
	const FLinearColor DarkMetal(0.034f, 0.046f, 0.064f, 1.0f);
	FLinearColor BodyColor = bBobStriker
		? FLinearColor(0.04f, 0.052f, 0.066f, 1.0f)
		: FMath::Lerp(DarkMetal, TeamColor, 0.09f);
	if (bShowPlayerIdentity)
	{
		BodyColor = FMath::Lerp(DarkMetal, TeamColor, 0.62f);
	}
	if (bHovered)
	{
		BodyColor = FMath::Lerp(BodyColor, FLinearColor::White, 0.16f);
	}
	BodyColor = FMath::Lerp(BodyColor, FLinearColor::White, FlashAlpha * 0.8f);
	const FLinearColor CoolMetal(0.13f, 0.18f, 0.23f, 1.0f);
	const float ArchetypeMix = Archetype == EFlickPieceArchetype::Standard ? 0.04f : 0.18f;
	FLinearColor TopColor = FMath::Lerp(
		FMath::Lerp(CoolMetal, TeamColor, 0.12f),
		VisualAccent,
		ArchetypeMix);
	const FLinearColor HaloColor = bKickoffLocked
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.7f)
		: bSelected
		? FLinearColor(1.0f, 0.78f, 0.05f, 1.0f)
		: FMath::Lerp(TeamColor, FLinearColor::White, 0.38f);
	FLinearColor PipColor = FMath::Lerp(
		FLinearColor(0.025f, 0.03f, 0.04f, 1.0f),
		VisualAccent,
		Archetype == EFlickPieceArchetype::Standard ? 0.18f : 0.72f);
	if (bBobStriker)
	{
		TopColor = FMath::Lerp(FLinearColor(0.055f, 0.067f, 0.078f, 1.0f), TeamColor, 0.08f);
		PipColor = FLinearColor(0.48f, 0.56f, 0.62f, 1.0f);
	}
	else if (bShowPlayerIdentity)
	{
		// Team modes reserve the broad puck construction for team identity and
		// use only the enlarged center plate for the owning player identity.
		TopColor = FMath::Lerp(CoolMetal, TeamColor, 0.3f);
		PipColor = FLinearColor(0.012f, 0.018f, 0.028f, 1.0f);
	}

	const auto SetMaterialColor = [](UMaterialInstanceDynamic* Material, const FLinearColor& Color, const float Roughness = 0.88f)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Material->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		}
	};
	const auto SetAccentPaint = [&SetMaterialColor](UMaterialInstanceDynamic* Material, const FLinearColor& Color, const float Brightness)
	{
		SetMaterialColor(Material, FLinearColor(
			Color.R * Brightness,
			Color.G * Brightness,
			Color.B * Brightness,
			Color.A), 0.92f);
	};
	const auto SetGlowColor = [](UMaterialInstanceDynamic* Material, const FLinearColor& Color, const float Intensity)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(
				Color.R * Intensity,
				Color.G * Intensity,
				Color.B * Intensity,
				Color.A));
		}
	};
	// Keep idle puck lighting below the arena bloom threshold. Selection can
	// still lift the accents, but the pieces should read as illuminated metal
	// rather than independent light sources.
	constexpr float IdleOuterBrightness = 0.16f;
	constexpr float SelectedOuterBrightness = 0.32f;
	constexpr float IdleCoreBrightness = 0.18f;
	constexpr float SelectedCoreBrightness = 0.36f;
	constexpr float IdleSignatureBrightness = 0.2f;
	constexpr float SelectedSignatureBrightness = 0.38f;
	constexpr float IdleDetailBrightness = 0.14f;
	constexpr float SelectedDetailBrightness = 0.32f;
	SetMaterialColor(BodyMaterial, BodyColor);
	SetMaterialColor(TopMaterial, TopColor);
	SetAccentPaint(OuterTrimMaterial, bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.18f)
		: FMath::Lerp(TeamColor, FLinearColor::White, bShowPlayerIdentity ? 0.03f : 0.08f),
		bSelected ? SelectedOuterBrightness : IdleOuterBrightness);
	SetMaterialColor(SideBandMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.008f, 0.012f, 0.02f, 1.0f), TeamColor, 0.92f)
		: bBobStriker
		? FLinearColor(0.026f, 0.033f, 0.042f, 1.0f)
		: FMath::Lerp(FLinearColor(0.01f, 0.015f, 0.024f, 1.0f), FMath::Lerp(TeamColor, ArchetypeColor, 0.5f), 0.2f));
	SetGlowColor(UnderglowMaterial, FMath::Lerp(TeamColor, FLinearColor::White, bSelected ? 0.12f : 0.0f), bSelected ? 0.1f : 0.018f);
	SetGlowColor(HaloMaterial, HaloColor, bKickoffLocked ? 0.3f : bSelected ? 0.22f : 0.055f);
	SetAccentPaint(PipMaterial, PipColor, bSelected ? 0.34f : 0.16f);
	SetAccentPaint(InnerRingMaterial, bShowPlayerIdentity
		? PlayerIdentityColor
		: bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.16f)
		: FMath::Lerp(VisualAccent, FLinearColor::White, 0.08f),
		bSelected ? SelectedCoreBrightness : IdleCoreBrightness);
	SetAccentPaint(SignatureRingMaterial, bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.12f)
		: FMath::Lerp(VisualAccent, FLinearColor::White, 0.1f),
		bSelected ? SelectedSignatureBrightness : IdleSignatureBrightness);
	SetMaterialColor(SignatureInsetMaterial, FMath::Lerp(
		FLinearColor(0.008f, 0.014f, 0.024f, 1.0f), VisualAccent, 0.055f));
	SetMaterialColor(CorePlateMaterial, bShowPlayerIdentity
		? PlayerIdentityColor
		: bBobStriker
		? FLinearColor(0.025f, 0.035f, 0.045f, 1.0f)
		: FMath::Lerp(FLinearColor(0.014f, 0.022f, 0.034f, 1.0f), VisualAccent, 0.19f));
	SetMaterialColor(LowerTrimMaterial, FMath::Lerp(TeamColor, FLinearColor(0.004f, 0.008f, 0.014f, 1.0f), 0.52f));
	SetMaterialColor(UpperShoulderMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.08f, 0.12f, 0.16f, 1.0f), TeamColor, 0.42f)
		: FMath::Lerp(FLinearColor(0.09f, 0.13f, 0.17f, 1.0f), TeamColor, 0.2f));
	SetMaterialColor(LowerShoulderMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.004f, 0.008f, 0.014f, 1.0f), TeamColor, 0.58f)
		: FMath::Lerp(FLinearColor(0.004f, 0.008f, 0.014f, 1.0f), TeamColor, 0.22f));
	SetMaterialColor(TopBezelMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.12f, 0.17f, 0.22f, 1.0f), TeamColor, 0.3f)
		: FMath::Lerp(FLinearColor(0.14f, 0.19f, 0.24f, 1.0f), TeamColor, 0.14f));
	SetMaterialColor(CoreBezelMaterial, FLinearColor(0.006f, 0.011f, 0.02f, 1.0f));
	for (int32 Index = 0; Index < TopTickMaterials.Num(); ++Index)
	{
		const FLinearColor DetailColor = bShowPlayerIdentity
			? FMath::Lerp(TeamColor, FLinearColor::White, 0.12f)
			: Index % 4 == 0
				? FMath::Lerp(VisualAccent, FLinearColor::White, 0.18f)
				: Index % 2 == 0
					? VisualAccent
					: FMath::Lerp(TeamColor, VisualAccent, 0.35f);
		SetAccentPaint(TopTickMaterials[Index], DetailColor,
			bSelected ? SelectedDetailBrightness : IdleDetailBrightness);
	}
	for (int32 Index = 0; Index < SideLugMaterials.Num(); ++Index)
	{
		const FLinearColor LugColor = Index % 4 == 0
			? FMath::Lerp(VisualAccent, FLinearColor::White, 0.25f)
			: FMath::Lerp(TeamColor, VisualAccent, 0.2f);
		SetAccentPaint(SideLugMaterials[Index], LugColor, bSelected ? 0.3f : 0.12f);
	}
	for (UMaterialInstanceDynamic* Material : EmblemPartMaterials)
	{
		SetAccentPaint(Material, FMath::Lerp(VisualAccent, FLinearColor::White, 0.06f), bSelected ? 0.36f : 0.18f);
	}
	AccentLight->SetLightColor(TeamColor);
	AccentLight->SetIntensity(0.0f);

	SelectionHalo->SetVisibility(bSelected || bHovered || bKickoffLocked);
	const float Pulse = bSelected || bKickoffLocked
		? 1.0f + 0.055f * FMath::Sin(VisualTime * 7.0f)
		: 1.0f + 0.025f * FMath::Sin(VisualTime * 5.0f);
	SelectionHalo->SetRelativeScale3D(FVector(
		BaseHaloRelativeScale.X * Pulse,
		BaseHaloRelativeScale.Y * Pulse,
		BaseHaloRelativeScale.Z));

	Label->SetText(FText::FromString(bBobStriker ? TEXT("B") : GetPieceArchetypeMark(Archetype)));
	Label->SetTextRenderColor((bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.72f)
		: FMath::Lerp(VisualAccent, FLinearColor::White, 0.45f)).ToFColor(true));
	Label->SetWorldSize(bSelected ? 21.0f : 18.0f);
	Label->SetVisibility(bBobStriker && !bShowPlayerIdentity && !bEliminated);
	PlayerLabel->SetText(FText::FromString(FString::Printf(TEXT("P%d"), OwningPlayerSlot + 1)));
	PlayerLabel->SetTextRenderColor((bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.78f)
		: OwningPlayerSlot == 1
			? FLinearColor::White
			: FLinearColor(0.005f, 0.01f, 0.018f, 1.0f)).ToFColor(true));
	PlayerLabel->SetWorldSize(bBobStriker
		? (bSelected ? 31.0f : 29.0f)
		: (bSelected ? 29.0f : 26.0f));
	PlayerLabel->SetVisibility(
		(bBobStriker || (bShowPlayerIdentity && !bUsingHighDetailPlayerIdentity)) && !bEliminated);
	UpdateTestArenaVisuals();
}
