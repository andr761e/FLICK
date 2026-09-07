#pragma once

#include "Arena/FlickArena.h"
#include "FlickTestArena.generated.h"

class AFlickPiece;
class UMaterialInstanceDynamic;
class UPhysicalMaterial;
class UPrimitiveComponent;
class UStaticMeshComponent;

/**
 * Isolated arena-mechanic prototype used only by the Test playlist.
 * Switch graphics are non-colliding; their edge dividers are the only added physics.
 */
UCLASS()
class FLICK_API AFlickTestArena : public AFlickArena
{
	GENERATED_BODY()

public:
	// Twenty authored sockets remain fixed and readable. Eight of them receive
	// a live switch/divider mechanism for each match.
	static constexpr int32 PossibleLocationCount = 20;
	static constexpr int32 MechanismCount = 8;

	AFlickTestArena();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeTestArena(float InRadius, float InThickness, float InSurfaceZ);
	void BeginControlZoneTracking(const TArray<TObjectPtr<AFlickPiece>>& Pieces);
	uint8 TrackControlZoneCrossings(
		const TArray<TObjectPtr<AFlickPiece>>& Pieces,
		float DeltaSeconds,
		uint8& OutDeployedMechanisms);
	uint8 CommitPendingControlZoneToggles(const TArray<TObjectPtr<AFlickPiece>>& Pieces);
	void ResetMechanisms();
	bool IsDividerRaised(int32 DividerIndex) const;
	bool IsDividerPending(int32 DividerIndex) const;
	bool FindDividerIndex(const UPrimitiveComponent* Component, int32& OutDividerIndex) const;
	FVector GetSwitchLabelLocation(int32 Index) const;
	FVector GetDividerLabelLocation(int32 Index) const;
	FVector GetDividerWorldCenter(int32 Index) const;
	FVector GetSwitchWorldCenter(int32 Index) const;
	FVector2D GetDividerWorldTangent(int32 Index) const;
	float GetDividerLength(int32 Index) const;
	float GetDividerCollisionThickness() const { return DividerThickness; }
	int32 GetMechanismCount() const { return FMath::Clamp(ActiveMechanismCount, 1, MechanismCount); }
	FString GetDividerLabel(int32 DividerIndex) const;
	FLinearColor GetMechanismColor(int32 MechanismIndex) const;
	uint8 GetRaisedDividerMask() const { return RaisedDividerMask; }
	void BeginReplayPresentation();
	void ApplyReplayDividerState(uint8 DividerMask);
	void EndReplayPresentation();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "24.0", ClampMax = "70.0"))
	float ControlZoneRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "6.0", ClampMax = "30.0"))
	float SwitchActivationDotRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float SwitchDetectionPadding = 2.0f;

	UPROPERTY(ReplicatedUsing = OnRep_TestLayout, EditAnywhere, BlueprintReadOnly, Category = "FLICK|Test Arena", meta = (ClampMin = "4", ClampMax = "8"))
	int32 ActiveMechanismCount = 8;

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
	// All presentation additions are non-colliding and read the authoritative state.
	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TObjectPtr<UStaticMeshComponent> InstrumentDeckMesh;

	UPROPERTY(VisibleAnywhere, Category = "FLICK|Test Arena|Components")
	TObjectPtr<UStaticMeshComponent> WorkshopArenaMesh;

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
	void ApplyTestLayout();
	void ApplyMechanismState();
	void CreateRuntimeMaterials();
	bool IsZoneCurrentlyOverlapped(int32 ZoneIndex, const TArray<TObjectPtr<AFlickPiece>>& Pieces) const;
	bool IsDividerCurrentlyOverlapped(int32 DividerIndex, const TArray<TObjectPtr<AFlickPiece>>& Pieces) const;
	FVector2D GetZoneLocalCenter(int32 ZoneIndex) const;
	uint8 DeployReadyDividers(const TArray<TObjectPtr<AFlickPiece>>& Pieces, float DeltaSeconds);

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
	uint8 RaisedDividerMask = 0;

	UPROPERTY(ReplicatedUsing = OnRep_DividerState)
	uint8 PendingToggleMask = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TestLayout)
	int32 ArenaLayoutSeed = 1337;

	uint8 ArmedZoneMask = 0;
	uint8 TriggeredThisShotMask = 0;
	bool bTrackingShot = false;
	TMap<int32, FVector2D> PreviousPieceLocations;
	TArray<float> DeploymentTimers;
	uint8 PreReplayRaisedDividerMask = 0;
	bool bReplayPresentationActive = false;
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
};
