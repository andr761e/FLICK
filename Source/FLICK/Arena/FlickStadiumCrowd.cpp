#include "Arena/FlickStadiumCrowd.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UFlickStadiumCrowd::UFlickStadiumCrowd()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetVisibility(false);
	SetHiddenInGame(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Head(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for (int32 Index = 0; Index < 5; ++Index)
	{
		auto* Batch = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
			*FString::Printf(TEXT("Spectators_%d"), Index));
		Batch->SetupAttachment(this);
		Batch->SetStaticMesh(Index == 4 ? Head.Object : Cube.Object);
		Batch->SetMaterial(0, Material.Object);
		Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Batch->SetGenerateOverlapEvents(false);
		Batch->SetCanEverAffectNavigation(false);
		Batch->SetCastShadow(false);
		Batch->SetVisibility(false);
		Batch->SetHiddenInGame(true);
		Batch->SetComponentTickEnabled(false);
		Batches.Add(Batch);
	}
}

void UFlickStadiumCrowd::AddSpectator(const FVector& Seat, float OutwardAngle, int32 Index, bool bBob)
{
	// Local X points outwards. Bent thighs, hanging shins and resting forearms
	// point towards the arena. Small deterministic variations avoid a cloned grid.
	const float Size = SpectatorScale * (0.94f + (Index % 5) * 0.025f);
	const FQuat Facing(FVector::UpVector, OutwardAngle);
	const auto Part = [&](int32 Batch, FVector Offset, FVector Dimensions)
	{
		Batches[Batch]->AddInstance(FTransform(Facing,
			Seat + Facing.RotateVector(Offset * Size), Dimensions * Size / 100.0f));
	};
	const int32 Shirt = Index % 3 == 0 ? 0 : Index % 3 == 1 ? 1 : 2;
	Part(Shirt, FVector(0, 0, 24), FVector(19, 28, 35));
	Part(4, FVector(-1, 0, 49), FVector(18, 19, 20));
	for (float Side : {-1.0f, 1.0f})
		Part(2, FVector(-9.5f, Side * 3.5f, 51), FVector(2, 3, 3));
	for (float Side : {-1.0f, 1.0f})
	{
		Part(2, FVector(-10, Side * 8, 3), FVector(31, 10, 10));
		Part(2, FVector(-23, Side * 8, bBob ? -20 : -13), FVector(10, 10, bBob ? 40 : 27));
		Part(2, FVector(-28, Side * 8, bBob ? -40 : -26), FVector(20, 11, 7));
		Part(Shirt, FVector(0, Side * 18, 23), FVector(10, 9, 25));
		Part(3, FVector(-10, Side * 18, 12), FVector(25, 8, 8));
	}
}

void UFlickStadiumCrowd::Configure(bool bBob, float StadiumScale, float SurfaceZ, bool bCrowdVisible)
{
	SetRelativeLocation(FVector(0, 0, SurfaceZ));
	SetRelativeScale3D(FVector(StadiumScale));
	SetVisibility(bCrowdVisible, true);
	SetHiddenInGame(!bCrowdVisible, true);
	if (!bCrowdVisible || GetNetMode() == NM_DedicatedServer) return;
	if (bBuilt && bBob == bLastBob && Occupancy == LastOccupancy && SpectatorScale == LastSpectatorScale) return;
	const FLinearColor Colours[] = {
		FLinearColor(0.035f, 0.42f, 0.58f), FLinearColor(0.8f, 0.24f, 0.055f),
		FLinearColor(0.055f, 0.075f, 0.10f), FLinearColor(0.62f, 0.48f, 0.34f)};
	for (int32 Index = 0; Index < Batches.Num(); ++Index)
	{
		Batches[Index]->ClearInstances();
		auto* Material = Cast<UMaterialInstanceDynamic>(Batches[Index]->GetMaterial(0));
		if (!Material) Material = Batches[Index]->CreateAndSetMaterialInstanceDynamic(0);
		if (Material)
		{
			Material->SetVectorParameterValue(TEXT("Color"), Colours[FMath::Min(Index, 3)]);
			Material->SetVectorParameterValue(TEXT("BaseColor"), Colours[FMath::Min(Index, 3)]);
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
		}
	}
	FRandomStream Random(7319);
	int32 Person = 0;
	for (int32 Gallery = 0; Gallery < 4; ++Gallery)
	{
		for (int32 Tier = 0; Tier < (bBob ? 4 : 3); ++Tier)
		{
			const int32 Seats = bBob ? 7 : 18;
			for (int32 Seat = 0; Seat < Seats; ++Seat, ++Person)
			{
				if (Random.FRand() > FMath::Clamp(Occupancy, 0.0f, 1.0f)) continue;
				const float Angle = FMath::DegreesToRadians(bBob ? 225.0f + Gallery * 90.0f
					: 20.0f + Gallery * 90.0f + Seat * (48.0f / (Seats - 1)));
				const FVector Outward(FMath::Cos(Angle), FMath::Sin(Angle), 0);
				const FVector Tangent(-Outward.Y, Outward.X, 0);
				// Match the source generators' metre-to-centimetre seat coordinates.
				const float Radius = bBob ? 1904.0f + Tier * 62.0f : 1200.0f + Tier * 52.0f;
				const float Z = bBob ? -54.0f + Tier * 34.0f + 44.0f
					: -50.0f + Tier * 25.0f + 31.0f;
				AddSpectator(Outward * Radius + Tangent * (bBob ? (Seat - 3) * 54.0f : 0.0f)
					+ FVector(0, 0, Z), Angle, Person, bBob);
			}
		}
	}
	bBuilt = true;
	bLastBob = bBob;
	LastOccupancy = Occupancy;
	LastSpectatorScale = SpectatorScale;
}
