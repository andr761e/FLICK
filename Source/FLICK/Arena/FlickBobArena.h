#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/Actor.h"
#include "FlickBobArena.generated.h"

class UMaterialInstanceDynamic;
class UPhysicalMaterial;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class FLICK_API AFlickBobArena : public AActor
{
	GENERATED_BODY()

public:
	AFlickBobArena();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeArena(float InHalfExtent, float InThickness, float InSurfaceZ);
	bool IsInsidePocket(const FVector& WorldLocation) const;
	bool IsCapturedByPocket(const FVector& WorldLocation, float PieceRadius) const;
	bool IsSafeForTabletopSelfRighting(const FVector& WorldLocation, float PieceRadius) const;
	FVector GetPocketWorldLocation(int32 PocketIndex) const;
	FVector GetStrikerStart(EFlickTeam Team, float PieceThickness) const;
	float GetSurfaceZ() const { return SurfaceZ; }
	float GetHalfExtent() const { return BoardHalfExtent; }
	float GetPocketRadius() const { return PocketRadius; }

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float BoardHalfExtent = 620.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float BoardThickness = 54.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float SurfaceZ = 250.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float PocketRadius = 66.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float PocketInset = 150.0f;

	/** Minimum vertical tolerance above the flat tabletop for pocket-opening capture. */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB|Pocket Physics", meta = (ClampMin = "2.0", ClampMax = "40.0"))
	float PocketCaptureDepth = 8.0f;

	/** Fraction of puck radius that must pass inside the visual pocket lip before capture. */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB|Pocket Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PocketCaptureRadiusScale = 0.35f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB|Pocket Visuals", meta = (ClampMin = "4.0", ClampMax = "60.0"))
	float PocketVisualDepth = 24.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float RailHeight = 42.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|BOB")
	float RailThickness = 34.0f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_ArenaConfiguration();

	void ApplyArenaShape();
	void ApplyPhysicsMaterials();

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> BoardBase;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> PlayingSurface;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CenterRingOuter;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CenterRingInner;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> Pedestal;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> Backdrop;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> StageBase;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> VenueBackWall;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Rails;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> PocketTrims;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Pockets;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> PocketDepths;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> PocketBottoms;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> PocketRimSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> GuideLines;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> StartLines;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> RailAccentStrips;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SurfacePanelLines;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> CornerCaps;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> VenuePylons;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> VenueBannerPanels;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> VenueLightBars;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FloorGridSegments;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> BoardPhysicalMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> RailPhysicalMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RuntimeMaterials;

	UPROPERTY(ReplicatedUsing = OnRep_ArenaConfiguration)
	int32 ConfigurationRevision = 0;
};
