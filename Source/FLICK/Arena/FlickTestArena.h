#pragma once

#include "Arena/FlickArena.h"
#include "FlickTestArena.generated.h"

class AFlickPiece;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPhysicalMaterial;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Switchyard arena presentation and divider mechanics used by Knockout playlists.
 * Switch graphics are non-colliding; their edge dividers are the only added physics.
 */
UCLASS()
class FLICK_API AFlickTestArena : public AFlickArena
{
	GENERATED_BODY()

public:
	// Allocate once for the largest experimental format. Runtime layout counts
	// remain 20/8 in 1v1, 28/12 in 2v2, and 36/14 in 3v3.
	static constexpr int32 MaxPossibleLocationCount = 36;
	static constexpr int32 MaxMechanismCount = 14;

	AFlickTestArena();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeTestArena(float InRadius, float InThickness, float InSurfaceZ, int32 InPlayersPerTeam = 1);
	void BeginControlZoneTracking(const TArray<TObjectPtr<AFlickPiece>>& Pieces);
	uint16 TrackControlZoneCrossings(
		const TArray<TObjectPtr<AFlickPiece>>& Pieces,
		float DeltaSeconds,
		uint16& OutDeployedMechanisms,
		TFunction<void(int32)> OnSwitchActivated = nullptr);
	uint16 CommitPendingControlZoneToggles(const TArray<TObjectPtr<AFlickPiece>>& Pieces);
	void ResetMechanisms();
	void SetTrainingBoardEditMode(bool bEnabled);
	bool ToggleTrainingMechanismAtWorldLocation(const FVector& WorldLocation, bool& bOutEnabled, bool& bOutChanged);
	bool IsDividerRaised(int32 DividerIndex) const;
	bool IsDividerPending(int32 DividerIndex) const;
	bool FindDividerIndex(const UPrimitiveComponent* Component, int32& OutDividerIndex) const;
	FVector GetSwitchLabelLocation(int32 Index) const;
	FVector GetDividerLabelLocation(int32 Index) const;
	FVector GetDividerWorldCenter(int32 Index) const;
	FVector GetSwitchWorldCenter(int32 Index) const;
	int32 FindSwitchAtWorldLocation(const FVector& WorldLocation) const;
	FVector2D GetDividerWorldTangent(int32 Index) const;
	float GetDividerLength(int32 Index) const;
	float GetDividerCollisionThickness() const { return DividerThickness; }
	int32 GetMechanismCount() const { return FMath::Clamp(ActiveMechanismCount, 1, MaxMechanismCount); }
	int32 GetPossibleLocationCount() const { return FMath::Clamp(DesignedLocationCount, 1, MaxPossibleLocationCount); }
	FString GetDividerLabel(int32 DividerIndex) const;
	FLinearColor GetMechanismColor(int32 MechanismIndex) const;
	uint16 GetRaisedDividerMask() const { return RaisedDividerMask; }
	void BeginReplayPresentation();
	void ApplyReplayDividerState(uint16 DividerMask);
	void EndReplayPresentation();
	// Local presentation overrides; imported assets and gameplay materials remain untouched.
	void SetMenuPresentationEnabled(bool bEnabled);

	// Historical OneVsOne property/subobject names are retained for saved asset
	// compatibility; these presentation settings now serve 1v1, 2v2 and 3v3.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena Presentation", meta = (ClampMin = "12", ClampMax = "40"))
	int32 RimLensCount = 28;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena Presentation", meta = (ClampMin = "10.0", ClampMax = "120.0"))
	float RimLensEmission = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena Presentation", meta = (DisplayName = "Deck Metallic", ClampMin = "0.0", ClampMax = "1.0"))
	float OneVsOneDeckMetallic = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena Presentation", meta = (DisplayName = "Deck Roughness", ClampMin = "0.12", ClampMax = "0.45"))
	float OneVsOneDeckRoughness = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Arena Presentation", meta = (DisplayName = "Menu Deck Roughness", ClampMin = "0.12", ClampMax = "0.45"))
	float OneVsOneMenuDeckRoughness = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "24.0", ClampMax = "70.0"))
	float ControlZoneRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "6.0", ClampMax = "30.0"))
	float SwitchActivationDotRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float SwitchDetectionPadding = 2.0f;

	UPROPERTY(ReplicatedUsing = OnRep_TestLayout, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "4", ClampMax = "14"))
	int32 ActiveMechanismCount = 8;

	UPROPERTY(ReplicatedUsing = OnRep_TestLayout, VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena")
	int32 DesignedLocationCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "1", ClampMax = "8"))
	int32 GuaranteedOuterEdgeMechanisms = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "0.03", ClampMax = "0.75"))
	float DividerDeploymentDelay = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "90.0", ClampMax = "260.0"))
	float SwitchDistanceFromDivider = 145.0f;

	// Leaves a readable strip of playing surface between edge mechanisms and
	// the authored rim while retaining their edge-control role.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "0.88", ClampMax = "0.95"))
	float OuterDividerRadiusFraction = 0.935f;

	// Move every modular divider and socket inward together. A radius-relative
	// inset preserves rim clearance across 1v1, 2v2 and 3v3 without resizing them.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "0.0", ClampMax = "0.10"))
	float DividerRadialInsetFraction = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float DividerDeploymentClearance = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "80.0", ClampMax = "240.0"))
	float DividerLength = 146.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "8.0", ClampMax = "40.0"))
	float DividerThickness = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "20.0", ClampMax = "100.0"))
	float DividerHeight = 56.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DividerFriction = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DividerRestitution = 0.36f;

protected:
	virtual void BeginPlay() override;

private:
	void CreateOneVsOnePresentationComponents(UStaticMesh* Box, UStaticMesh* Cylinder);
	void UpdateOneVsOnePresentation();
	void BuildOneVsOneRim();

	UPROPERTY(VisibleAnywhere, Category = "FLICK|1v1 Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> RimLensBodies;
	UPROPERTY(VisibleAnywhere, Category = "FLICK|1v1 Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> RimLensCaps;
	UPROPERTY(VisibleAnywhere, Category = "FLICK|1v1 Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> RimHousingBodies;
	UPROPERTY(VisibleAnywhere, Category = "FLICK|1v1 Presentation")
	TObjectPtr<UInstancedStaticMeshComponent> RimHousingCaps;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OneVsOneOriginalMaterials;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OneVsOneMaterials;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RimLensMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RimHousingMaterial;
	bool bOneVsOnePresentationEnabled = false;

	// All presentation additions are non-colliding and read the authoritative state.
	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TObjectPtr<UStaticMeshComponent> InstrumentDeckMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TObjectPtr<UStaticMeshComponent> WorkshopArenaMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> MenuOriginalMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> MenuPresentationMaterials;

	bool bMenuMaterialsEnabled = false;

	// The surrounding stadium is presentation-only. Keeping its shell and
	// emissive details separate lets us tune visibility and lighting without
	// ever adding another collision surface around the gameplay arena.
	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TObjectPtr<UStaticMeshComponent> StadiumStructureMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TObjectPtr<UStaticMeshComponent> StadiumLightsMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> DividerCapMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> InstrumentDeckMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DotMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TraceMaterials;

	UFUNCTION()
	void OnRep_DividerState();

	UFUNCTION()
	void OnRep_TestLayout();

	void BuildLayoutFromSeed();
	void PopulateActiveLayoutFromLocations();
	void ApplyTestLayout();
	void ApplyMechanismState();
	void CreateRuntimeMaterials();
	bool IsZoneCurrentlyOverlapped(int32 ZoneIndex, const TArray<TObjectPtr<AFlickPiece>>& Pieces) const;
	bool IsDividerCurrentlyOverlapped(int32 DividerIndex, const TArray<TObjectPtr<AFlickPiece>>& Pieces) const;
	FVector2D GetZoneLocalCenter(int32 ZoneIndex) const;
	uint16 DeployReadyDividers(const TArray<TObjectPtr<AFlickPiece>>& Pieces, float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> ZoneOuterMeshes;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> ZoneInnerMeshes;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> ZoneDotMeshes;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> SignalTraceMeshes;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> DividerBaseMeshes;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> DividerMeshes;

	// Authored presentation follows DividerMeshes, which remain the simple and
	// predictable authoritative collision bodies.
	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TArray<TObjectPtr<UStaticMeshComponent>> DividerVisualMeshes;

	UPROPERTY(ReplicatedUsing = OnRep_DividerState)
	uint16 RaisedDividerMask = 0;

	UPROPERTY(ReplicatedUsing = OnRep_DividerState)
	uint16 PendingToggleMask = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TestLayout)
	int32 ArenaLayoutSeed = 1337;

	uint16 ArmedZoneMask = 0;
	uint16 TriggeredThisShotMask = 0;
	bool bTrackingShot = false;
	TMap<int32, FVector2D> PreviousPieceLocations;
	TArray<float> DeploymentTimers;
	uint16 PreReplayRaisedDividerMask = 0;
	bool bReplayPresentationActive = false;
	bool bTrainingBoardEditMode = false;
	TArray<int32> ActiveLocationIndices;
	TArray<FVector2D> ZoneCenters;
	TArray<FVector2D> DividerCenters;
	TArray<float> DividerAngles;
	TArray<float> RandomizedDividerLengths;
	TArray<FVector2D> PossibleDividerCenters;
	TArray<float> PossibleDividerAngles;
	TArray<float> PossibleDividerLengths;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> AccentMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DividerMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ZoneInsetMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DormantSocketMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPhysicalMaterial> RuntimeDividerPhysicalMaterial;

	bool bUsingWorkshopAssets = false;
	bool bUsingStadiumAssets = false;
};
