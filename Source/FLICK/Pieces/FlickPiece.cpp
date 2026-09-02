#include "Pieces/FlickPiece.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Core/FlickLog.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Engine/Font.h"
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
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
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
	}

	for (UStaticMeshComponent* VisualMesh : {
		TopDisc.Get(), OuterTrim.Get(), SideBand.Get(), Underglow.Get(), SelectionHalo.Get(), CenterPip.Get(),
		InnerRing.Get(), CorePlate.Get(), LowerTrim.Get(), UpperShoulder.Get(), LowerShoulder.Get(),
		TopBezel.Get(), CoreBezel.Get()})
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

	AccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight"));
	AccentLight->SetupAttachment(PieceMesh);
	AccentLight->SetCastShadows(false);
	AccentLight->SetAttenuationRadius(205.0f);
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
	DOREPLIFETIME(AFlickPiece, bBobStriker);
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
	if (bEliminated || !OtherPiece || OtherPiece == this || OtherPiece->IsEliminated())
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
}

void AFlickPiece::UpdateVisualTransforms()
{
	const float SafeThickness = FMath::Max(PieceThickness, 1.0f);
	const float ParentZScale = SafeThickness / 100.0f;
	const float ParentXYScale = FMath::Max(PieceRadius / 50.0f, 0.01f);
	float TopScale = 0.78f;
	float PipScale = 0.14f;
	switch (Archetype)
	{
	case EFlickPieceArchetype::Heavy:
		TopScale = 0.68f;
		PipScale = 0.22f;
		break;
	case EFlickPieceArchetype::Striker:
		TopScale = 0.86f;
		PipScale = 0.1f;
		break;
	case EFlickPieceArchetype::Grippy:
		TopScale = 0.74f;
		PipScale = 0.17f;
		break;
	case EFlickPieceArchetype::Slider:
		TopScale = 0.84f;
		PipScale = 0.1f;
		break;
	case EFlickPieceArchetype::Blocker:
		TopScale = 0.63f;
		PipScale = 0.24f;
		break;
	case EFlickPieceArchetype::Compact:
		TopScale = 0.9f;
		PipScale = 0.11f;
		break;
	case EFlickPieceArchetype::Bouncer:
		TopScale = 0.77f;
		PipScale = 0.19f;
		break;
	case EFlickPieceArchetype::Toppler:
		TopScale = 0.7f;
		PipScale = 0.21f;
		break;
	case EFlickPieceArchetype::Standard:
	default:
		break;
	}
	if (bBobStriker)
	{
		TopScale = 0.64f;
		PipScale = 0.25f;
	}

	OuterTrim->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 0.8f) / ParentZScale));
	OuterTrim->SetRelativeScale3D(FVector(0.94f, 0.94f, 1.8f / SafeThickness));
	TopDisc->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 1.7f) / ParentZScale));
	TopDisc->SetRelativeScale3D(FVector(TopScale, TopScale, 1.8f / SafeThickness));
	SideBand->SetRelativeLocation(FVector(0.0f, 0.0f, -SafeThickness * 0.08f / ParentZScale));
	SideBand->SetRelativeScale3D(FVector(1.025f, 1.025f, 0.46f));
	const float ShoulderOffset = SafeThickness * 0.31f / ParentZScale;
	UpperShoulder->SetRelativeLocation(FVector(0.0f, 0.0f, ShoulderOffset));
	UpperShoulder->SetRelativeScale3D(FVector(1.055f, 1.055f, 0.14f));
	LowerShoulder->SetRelativeLocation(FVector(0.0f, 0.0f, -ShoulderOffset));
	LowerShoulder->SetRelativeScale3D(FVector(1.055f, 1.055f, 0.14f));
	Underglow->SetRelativeLocation(FVector(0.0f, 0.0f, (-SafeThickness * 0.5f + 0.9f) / ParentZScale));
	Underglow->SetRelativeScale3D(FVector(1.11f, 1.11f, 1.2f / SafeThickness));
	LowerTrim->SetRelativeLocation(FVector(0.0f, 0.0f, (-SafeThickness * 0.5f + 2.2f) / ParentZScale));
	LowerTrim->SetRelativeScale3D(FVector(1.045f, 1.045f, 1.5f / SafeThickness));
	SelectionHalo->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 0.7f) / ParentZScale));
	BaseHaloRelativeScale = FVector(1.16f, 1.16f, 1.0f / SafeThickness);
	SelectionHalo->SetRelativeScale3D(BaseHaloRelativeScale);
	TopBezel->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 1.28f) / ParentZScale));
	const float TopBezelScale = FMath::Clamp(TopScale + 0.095f, 0.72f, 0.965f);
	TopBezel->SetRelativeScale3D(FVector(TopBezelScale, TopBezelScale, 1.2f / SafeThickness));
	InnerRing->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 2.45f) / ParentZScale));
	const float InnerRingScale = bShowPlayerIdentity ? 0.7f : 0.58f;
	InnerRing->SetRelativeScale3D(FVector(InnerRingScale, InnerRingScale, 1.25f / SafeThickness));
	CoreBezel->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 2.82f) / ParentZScale));
	const float CoreBezelScale = bShowPlayerIdentity ? 0.63f : 0.515f;
	CoreBezel->SetRelativeScale3D(FVector(CoreBezelScale, CoreBezelScale, 1.2f / SafeThickness));
	CorePlate->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 3.05f) / ParentZScale));
	const float CorePlateScale = bShowPlayerIdentity ? 0.56f : 0.45f;
	CorePlate->SetRelativeScale3D(FVector(CorePlateScale, CorePlateScale, 1.25f / SafeThickness));
	CenterPip->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 3.75f) / ParentZScale));
	CenterPip->SetRelativeScale3D(FVector(PipScale, PipScale, 2.4f / SafeThickness));
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 5.0f) / ParentZScale));
	if (bShowPlayerIdentity)
	{
		Label->SetRelativeLocation(FVector(
			0.0f,
			-PieceRadius * 0.43f / ParentXYScale,
			(SafeThickness * 0.5f + 5.2f) / ParentZScale));
	}
	PlayerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 5.4f) / ParentZScale));

	for (int32 Index = 0; Index < TopTicks.Num(); ++Index)
	{
		const bool bSparseDetails = Archetype == EFlickPieceArchetype::Blocker
			|| Archetype == EFlickPieceArchetype::Bouncer;
		const bool bArchetypeDetailVisible = !bSparseDetails || Index % 2 == 0;
		const bool bPlayerPatternTick = OwningPlayerSlot == 0
			? Index == 0
			: OwningPlayerSlot == 1
				? Index == 0 || Index == TopTicks.Num() / 2
				: Index % 4 == 0;
		TopTicks[Index]->SetVisibility(bShowPlayerIdentity ? bPlayerPatternTick : bArchetypeDetailVisible);
		SideLugs[Index]->SetVisibility(bArchetypeDetailVisible);
		const float Angle = 2.0f * PI * static_cast<float>(Index) / FMath::Max(1, TopTicks.Num());
		const float DetailRadiusFactor = Archetype == EFlickPieceArchetype::Compact
			? 0.7f
			: Archetype == EFlickPieceArchetype::Slider
				? 0.84f
				: 0.79f;
		const float DetailRadius = PieceRadius * DetailRadiusFactor;
		TopTicks[Index]->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * DetailRadius / ParentXYScale,
			FMath::Sin(Angle) * DetailRadius / ParentXYScale,
			(SafeThickness * 0.5f + 3.0f) / ParentZScale));
		TopTicks[Index]->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		const float TickLength = bShowPlayerIdentity
			? 19.0f
			: Archetype == EFlickPieceArchetype::Slider
			? 14.0f
			: Archetype == EFlickPieceArchetype::Blocker
				? 12.0f
				: 9.0f;
		const float TickWidth = bShowPlayerIdentity
			? 6.5f
			: Archetype == EFlickPieceArchetype::Toppler ? 4.5f : 3.0f;
		TopTicks[Index]->SetRelativeScale3D(FVector(
			TickLength / (100.0f * ParentXYScale),
			TickWidth / (100.0f * ParentXYScale),
			1.2f / (100.0f * ParentZScale)));

		const float LugRadius = PieceRadius * 0.98f;
		SideLugs[Index]->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * LugRadius / ParentXYScale,
			FMath::Sin(Angle) * LugRadius / ParentXYScale,
			-SafeThickness * 0.05f / ParentZScale));
		SideLugs[Index]->SetRelativeRotation(FRotator(0.0f, FMath::RadiansToDegrees(Angle) + 90.0f, 0.0f));
		const float LugLength = Archetype == EFlickPieceArchetype::Blocker ? 11.0f : 7.0f;
		const float LugHeight = Archetype == EFlickPieceArchetype::Toppler ? 0.62f : 0.42f;
		SideLugs[Index]->SetRelativeScale3D(FVector(
			LugLength / (100.0f * ParentXYScale),
			3.2f / (100.0f * ParentXYScale),
			SafeThickness * LugHeight / (100.0f * ParentZScale)));
	}
	AccentLight->SetRelativeLocation(FVector(0.0f, 0.0f, (SafeThickness * 0.5f + 22.0f) / ParentZScale));
}

void AFlickPiece::ApplyVisuals()
{
	if (!PieceMesh || !TopDisc || !OuterTrim || !SideBand || !Underglow || !SelectionHalo || !CenterPip
		|| !InnerRing || !CorePlate || !LowerTrim || !UpperShoulder || !LowerShoulder
		|| !TopBezel || !CoreBezel || !Label || !PlayerLabel)
	{
		return;
	}

	EnsureVisualMaterials();
	const FLinearColor TeamColor = GetTeamColor(Team);
	const FFlickPieceArchetypeRules& ArchetypeRules = FlickPieceArchetypeRules::Get(Archetype);
	const FLinearColor ArchetypeColor = ArchetypeRules.AccentColor;
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
	const float ArchetypeMix = Archetype == EFlickPieceArchetype::Standard ? 0.0f : 0.26f;
	FLinearColor TopColor = FMath::Lerp(
		FMath::Lerp(BodyColor, FLinearColor(0.16f, 0.19f, 0.23f, 1.0f), 0.24f),
		ArchetypeColor,
		ArchetypeMix * 0.55f);
	const FLinearColor HaloColor = bKickoffLocked
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.7f)
		: bSelected
		? FLinearColor(1.0f, 0.78f, 0.05f, 1.0f)
		: FMath::Lerp(TeamColor, FLinearColor::White, 0.38f);
	FLinearColor PipColor = FMath::Lerp(
		FLinearColor(0.025f, 0.03f, 0.04f, 1.0f),
		ArchetypeColor,
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
		TopColor = FMath::Lerp(FLinearColor(0.018f, 0.026f, 0.04f, 1.0f), TeamColor, 0.88f);
		PipColor = FLinearColor(0.012f, 0.018f, 0.028f, 1.0f);
	}

	const auto SetMaterialColor = [](UMaterialInstanceDynamic* Material, const FLinearColor& Color)
	{
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.82f);
		}
	};
	SetMaterialColor(BodyMaterial, BodyColor);
	SetMaterialColor(TopMaterial, TopColor);
	SetMaterialColor(OuterTrimMaterial, bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.18f)
		: FMath::Lerp(TeamColor, FLinearColor::White, bShowPlayerIdentity ? 0.03f : 0.08f));
	SetMaterialColor(SideBandMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.008f, 0.012f, 0.02f, 1.0f), TeamColor, 0.92f)
		: bBobStriker
		? FLinearColor(0.026f, 0.033f, 0.042f, 1.0f)
		: FMath::Lerp(FLinearColor(0.01f, 0.015f, 0.024f, 1.0f), FMath::Lerp(TeamColor, ArchetypeColor, 0.5f), 0.2f));
	SetMaterialColor(UnderglowMaterial, FMath::Lerp(TeamColor, FLinearColor::White, bSelected ? 0.18f : 0.02f));
	SetMaterialColor(HaloMaterial, HaloColor);
	SetMaterialColor(PipMaterial, PipColor);
	SetMaterialColor(InnerRingMaterial, bShowPlayerIdentity
		? PlayerIdentityColor
		: bBobStriker
		? FMath::Lerp(TeamColor, FLinearColor::White, 0.16f)
		: FMath::Lerp(TeamColor, FLinearColor::White, 0.08f));
	SetMaterialColor(CorePlateMaterial, bShowPlayerIdentity
		? PlayerIdentityColor
		: bBobStriker
		? FLinearColor(0.025f, 0.035f, 0.045f, 1.0f)
		: FMath::Lerp(FLinearColor(0.014f, 0.022f, 0.034f, 1.0f), TeamColor, 0.22f));
	SetMaterialColor(LowerTrimMaterial, FMath::Lerp(TeamColor, FLinearColor(0.004f, 0.008f, 0.014f, 1.0f), 0.52f));
	SetMaterialColor(UpperShoulderMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.012f, 0.018f, 0.028f, 1.0f), TeamColor, 0.86f)
		: FMath::Lerp(FLinearColor(0.012f, 0.018f, 0.028f, 1.0f), TeamColor, 0.34f));
	SetMaterialColor(LowerShoulderMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.004f, 0.008f, 0.014f, 1.0f), TeamColor, 0.58f)
		: FMath::Lerp(FLinearColor(0.004f, 0.008f, 0.014f, 1.0f), TeamColor, 0.22f));
	SetMaterialColor(TopBezelMaterial, bShowPlayerIdentity
		? FMath::Lerp(FLinearColor(0.016f, 0.024f, 0.038f, 1.0f), TeamColor, 0.46f)
		: FMath::Lerp(FLinearColor(0.016f, 0.024f, 0.038f, 1.0f), TeamColor, 0.18f));
	SetMaterialColor(CoreBezelMaterial, FLinearColor(0.006f, 0.011f, 0.02f, 1.0f));
	for (UMaterialInstanceDynamic* Material : TopTickMaterials)
	{
		SetMaterialColor(Material, bShowPlayerIdentity
			? FMath::Lerp(TeamColor, FLinearColor::White, 0.12f)
			: FMath::Lerp(TeamColor, FLinearColor::White, 0.06f));
	}
	for (UMaterialInstanceDynamic* Material : SideLugMaterials)
	{
		SetMaterialColor(Material, bShowPlayerIdentity
			? FMath::Lerp(TeamColor, FLinearColor(0.008f, 0.01f, 0.016f, 1.0f), 0.34f)
			: FMath::Lerp(TeamColor, FLinearColor(0.008f, 0.01f, 0.016f, 1.0f), 0.48f));
	}
	AccentLight->SetLightColor(TeamColor);
	AccentLight->SetIntensity(bSelected ? 45.0f : bHovered ? 18.0f : 0.0f);

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
		: FMath::Lerp(ArchetypeColor, FLinearColor::White, 0.45f)).ToFColor(true));
	Label->SetWorldSize(bSelected ? 21.0f : 18.0f);
	Label->SetVisibility(!bShowPlayerIdentity && !bEliminated);
	PlayerLabel->SetText(FText::FromString(FString::Printf(TEXT("P%d"), OwningPlayerSlot + 1)));
	PlayerLabel->SetTextRenderColor((OwningPlayerSlot == 1
		? FLinearColor::White
		: FLinearColor(0.005f, 0.01f, 0.018f, 1.0f)).ToFColor(true));
	PlayerLabel->SetWorldSize(bSelected ? 29.0f : 26.0f);
	PlayerLabel->SetVisibility(bShowPlayerIdentity && !bEliminated);
}
