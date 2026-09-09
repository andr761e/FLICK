#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/Actor.h"
#include "FlickPiece.generated.h"

class UPhysicalMaterial;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class FLICK_API AFlickPiece : public AActor
{
	GENERATED_BODY()

public:
	AFlickPiece();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializePiece(
		EFlickTeam InTeam,
		int32 InPieceId,
		float InRadius,
		float InThickness,
		EFlickPieceArchetype InArchetype,
		bool bInBobStriker = false,
		int32 InOwningPlayerSlot = 0,
		bool bInShowPlayerIdentity = false);
	void ApplyPhysicsSettings();
	/** Cosmetic mesh for the divider test arena; the existing root remains the physics body. */
	void EnableTestArenaVisuals();
	bool HasTestArenaVisuals() const;
	void Launch(const FVector& Direction, float NormalizedPower, float MaxLaunchSpeed);
	void Eliminate();
	void SetSelected(bool bInSelected);
	void SetHovered(bool bInHovered);
	void SetKickoffLocked(bool bInKickoffLocked);
	void PlayImpactFlash(float Strength);
	void ApplyTabletopSelfRighting(float TorqueStrength, float DampingStrength, float MinimumTiltDegrees);
	void ApplyTabletopFlightContainment(float SurfaceZ, float MaximumUpwardSpeed, float DownwardAcceleration);
	void SettleFlatOnTabletop(float SurfaceZ);
	void BeginReplayPresentation();
	void ApplyReplayPresentation(const FTransform& Transform, bool bVisible);
	void EndReplayPresentation();

	bool IsSelectableBy(EFlickTeam Team) const;
	bool IsActive() const { return !bEliminated; }
	bool IsEliminated() const { return bEliminated; }
	EFlickTeam GetTeam() const { return Team; }
	EFlickPieceArchetype GetArchetype() const { return Archetype; }
	bool IsBobStriker() const { return bBobStriker; }
	int32 GetPieceId() const { return PieceId; }
	int32 GetOwningPlayerSlot() const { return OwningPlayerSlot; }
	bool ShowsPlayerIdentity() const { return bShowPlayerIdentity; }
	FVector GetLinearVelocity() const;
	FVector GetAngularVelocityDegrees() const;
	float GetPieceRadius() const { return PieceRadius; }
	float GetPieceThickness() const { return PieceThickness; }
	float GetLaunchSpeedMultiplier() const { return LaunchSpeedMultiplier; }

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceMassKg = 4.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceFriction = 0.12f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float PieceRestitution = 0.32f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float LinearDamping = 0.55f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float AngularDamping = 1.8f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float LaunchSpeedMultiplier = 1.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Piece")
	float CenterOfMassOffsetZ = 0.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics")
	bool bAllowEdgeTipping = true;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics")
	bool bUseContinuousCollisionDetection = true;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics", meta = (ClampMin = "1", ClampMax = "255"))
	int32 PositionSolverIterations = 12;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Physics", meta = (ClampMin = "1", ClampMax = "255"))
	int32 VelocitySolverIterations = 4;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_PieceConfiguration();

	UFUNCTION()
	void OnRep_Eliminated();

	UFUNCTION()
	void OnRep_KickoffLocked();

	void ApplyEliminatedState();

	UFUNCTION()
	void HandleMeshHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void EnsureVisualMaterials();
	void UpdateVisualTransforms();
	void ApplyVisuals();
	void UpdateTestArenaVisuals();

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> WorkshopMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WorkshopTeamMaterials;
	bool bUsingHighDetailPuck = false;
	bool bUsingHighDetailPlayerIdentity = false;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> PieceMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> TopDisc;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> OuterTrim;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> SideBand;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> Underglow;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> SelectionHalo;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CenterPip;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> InnerRing;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CorePlate;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> LowerTrim;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> UpperShoulder;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> LowerShoulder;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> TopBezel;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CoreBezel;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> SignatureRing;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> SignatureInset;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> TopTicks;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SideLugs;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> EmblemParts;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UPointLightComponent> AccentLight;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UTextRenderComponent> PlayerLabel;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> RuntimePhysicalMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TopMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OuterTrimMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SideBandMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> UnderglowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HaloMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PipMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InnerRingMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CorePlateMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LowerTrimMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> UpperShoulderMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LowerShoulderMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TopBezelMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoreBezelMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SignatureRingMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SignatureInsetMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TopTickMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SideLugMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> EmblemPartMaterials;

	UPROPERTY(ReplicatedUsing = OnRep_PieceConfiguration, VisibleAnywhere, Category = "FLICK|Piece")
	EFlickTeam Team = EFlickTeam::None;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Piece")
	EFlickPieceArchetype Archetype = EFlickPieceArchetype::Standard;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Piece")
	int32 PieceId = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Piece")
	int32 OwningPlayerSlot = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PieceConfiguration, VisibleAnywhere, Category = "FLICK|Piece")
	bool bShowPlayerIdentity = false;

	UPROPERTY(ReplicatedUsing = OnRep_Eliminated, VisibleAnywhere, Category = "FLICK|Piece")
	bool bEliminated = false;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Piece")
	bool bSelected = false;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Piece")
	bool bHovered = false;

	UPROPERTY(ReplicatedUsing = OnRep_KickoffLocked, VisibleAnywhere, Category = "FLICK|Piece")
	bool bKickoffLocked = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Piece")
	bool bBobStriker = false;

	UPROPERTY(Replicated)
	float PieceRadius = 45.0f;

	UPROPERTY(Replicated)
	float PieceThickness = 20.0f;
	float HitFlashRemaining = 0.0f;
	float HitFlashStrength = 0.0f;
	float VisualTime = 0.0f;
	float LastImpactNotificationTime = -100.0f;
	float LastArenaImpactNotificationTime = -100.0f;
	FVector BaseHaloRelativeScale = FVector::OneVector;
	FTransform PreReplayTransform = FTransform::Identity;
	bool bReplayPresentationActive = false;
};
