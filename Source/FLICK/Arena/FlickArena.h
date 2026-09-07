#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FlickArena.generated.h"

class UPhysicalMaterial;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class FLICK_API AFlickArena : public AActor
{
	GENERATED_BODY()

public:
	AFlickArena();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeArena(float InRadius, float InThickness, float InSurfaceZ, int32 InPlayersPerTeam = 1);
	float GetSurfaceZ() const { return SurfaceZ; }
	float GetRadius() const { return ArenaRadius; }

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaRadius = 650.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float ArenaThickness = 50.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float SurfaceZ = 250.0f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float SurfaceFriction = 0.1f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	float SurfaceRestitution = 0.25f;

	UPROPERTY(ReplicatedUsing = OnRep_ArenaConfiguration, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Arena")
	int32 PlayersPerTeam = 1;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	// Allows specialized arenas to replace the generated presentation without
	// disabling the authoritative circular floor collision.
	void SetArenaPresentationVisible(bool bVisible);

private:
	UFUNCTION()
	void OnRep_ArenaConfiguration();

	void ApplyArenaShape();
	void ApplyPhysicsMaterial();

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> ArenaMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> TopSurfaceMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> InnerFieldMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> RimAccentMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> OuterBezelMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> LowerDeckMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CenterPlateMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> BackdropMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> StageBaseMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> VenueBackWallMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CenterLineMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CrossLineMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> CenterMarkMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> Player1HomeMarkMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TObjectPtr<UStaticMeshComponent> Player2HomeMarkMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> RimSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FieldRingSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SideLightSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> DirectionMarkerSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SurfaceSeamSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> VenuePylons;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> VenueBannerPanels;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> VenueLightBars;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FloorGridSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> FloorLightStuds;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> MultiplayerTeamArcSegments;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> MultiplayerPlayerZoneOutlines;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> MultiplayerPlayerZoneInsets;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> RuntimePhysicalMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TopMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InnerFieldMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RimAccentMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OuterBezelMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LowerDeckMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CenterPlateMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PedestalMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BackdropMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> StageBaseMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VenueBackWallMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> NeutralMarkMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Player1MarkMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Player2MarkMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RimSegmentMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> FieldRingMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SideLightMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DirectionMarkerMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SurfaceSeamMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VenuePylonMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FloorGridMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FloorLightStudMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> VenueBannerMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> VenueLightMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MultiplayerTeamArcMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MultiplayerPlayerZoneOutlineMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MultiplayerPlayerZoneInsetMaterials;

	UPROPERTY(ReplicatedUsing = OnRep_ArenaConfiguration)
	int32 ConfigurationRevision = 0;
};
