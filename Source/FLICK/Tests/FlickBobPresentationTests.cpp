#if WITH_DEV_AUTOMATION_TESTS

#include "Arena/FlickBobArena.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FlickModeRules.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/BodyInstance.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickBobConceptPresentationTest, "FLICK.BOB.ConceptPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickBobConceptPresentationTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
		ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("BOB presentation test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Arena = World->SpawnActor<AFlickBobArena>();
	if (!TestNotNull(TEXT("BOB presentation test arena"), Arena))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		return false;
	}
	Arena->InitializeArena(620.f, 54.f, 250.f);

	const auto FindMesh = [Arena](const FName Name)
	{
		TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
		for (UStaticMeshComponent* Component : Components)
			if (Component->GetFName() == Name) return Component;
		return static_cast<UStaticMeshComponent*>(nullptr);
	};
	const auto CheckPhysicalMaterial = [this](UPrimitiveComponent* Component,
		const float Friction, const float Restitution)
	{
		if (!TestNotNull(TEXT("Physical component"), Component)) return;
		const FBodyInstance* Body = Component->GetBodyInstance();
		const UPhysicalMaterial* Physical = Body ? Body->GetSimplePhysicalMaterial() : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s physical material"), *Component->GetName()), Physical)) return;
		TestEqual(TEXT("Visual finish cannot change physical friction"), Physical->Friction, Friction);
		TestEqual(TEXT("Visual finish cannot change physical restitution"), Physical->Restitution, Restitution);
		TestTrue(TEXT("Friction retains an explicit combine override"), Physical->bOverrideFrictionCombineMode);
		TestTrue(TEXT("Restitution retains an explicit combine override"), Physical->bOverrideRestitutionCombineMode);
		TestEqual(TEXT("Friction retains Average combining"), Physical->FrictionCombineMode.GetValue(), EFrictionCombineMode::Average);
		TestEqual(TEXT("Restitution retains Average combining"), Physical->RestitutionCombineMode.GetValue(), EFrictionCombineMode::Average);
	};

	struct FPresentationSnapshot
	{
		UStaticMeshComponent* Component;
		UStaticMesh* Mesh;
		FTransform Transform;
		TArray<UMaterialInterface*> Materials;
	};
	TArray<FPresentationSnapshot> Presentation;
	for (const FName Name : {FName(TEXT("HighDetailArenaMesh")),
		FName(TEXT("HighDetailStadiumStructure")), FName(TEXT("HighDetailStadiumLights"))})
	{
		UStaticMeshComponent* Component = FindMesh(Name);
		if (!TestNotNull(*Name.ToString(), Component)) continue;
		UStaticMesh* Mesh = Component->GetStaticMesh();
		if (!TestNotNull(TEXT("Original imported BOB mesh remains loaded"), Mesh)) continue;
		const bool bBoard = Name == TEXT("HighDetailArenaMesh");
		const FString ExpectedMesh = bBoard ? TEXT("SM_BobArena_HighDetail")
			: Name == TEXT("HighDetailStadiumStructure") ? TEXT("SM_BobStadium_Structure") : TEXT("SM_BobStadium_Lights");
		TestEqual(TEXT("Visual revision retains the authored mesh asset"), Mesh->GetName(), ExpectedMesh);
		TestTrue(TEXT("High-detail art remains visible"), Component->IsVisible());
		TestEqual(TEXT("High-detail art cannot affect puck collisions"),
			Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestFalse(TEXT("High-detail art cannot generate physics overlaps"), Component->GetGenerateOverlapEvents());
		TestTrue(TEXT("BOB presentation keeps its unit scale"), Component->GetRelativeScale3D().Equals(FVector::OneVector));
		TestTrue(TEXT("BOB presentation keeps its authored placement"), Component->GetRelativeLocation().Equals(
			bBoard ? FVector::ZeroVector : FVector(0, 0, 250)));
		TestTrue(TEXT("BOB presentation keeps its authored rotation"), Component->GetRelativeRotation().IsNearlyZero());
		const FString ExpectedParent = bBoard
			? TEXT("/Game/BOB/Arena/M_BobArena_Premium.M_BobArena_Premium")
			: TEXT("/Game/BOB/Stadium/M_BobStadiumSurface.M_BobStadiumSurface");
		FPresentationSnapshot Snapshot{Component, Mesh, Component->GetRelativeTransform(), {}};
		for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Material = Component->GetMaterial(Slot);
			Snapshot.Materials.Add(Material);
			if (!TestNotNull(TEXT("Authored BOB material remains assigned"), Material)) continue;
			TestTrue(TEXT("High-detail BOB artwork shares the authored material instance"), Material == Mesh->GetMaterial(Slot));
			TestFalse(TEXT("Repeated setup does not wrap high-detail BOB art in runtime MIDs"), Material->IsA<UMaterialInstanceDynamic>());
			UMaterial* Parent = Material->GetMaterial();
			if (!TestNotNull(TEXT("BOB material parent"), Parent)) continue;
			TestEqual(TEXT("Presentation upgrades the existing BOB material parent"), Parent->GetPathName(), ExpectedParent);
			FLinearColor Color;
			TestTrue(TEXT("BOB materials retain editable reflectance color"),
				Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Color));
			TestTrue(TEXT("BOB reflectance color is finite"), FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G)
				&& FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A));
			for (const FName Parameter : {FName(TEXT("Metallic")), FName(TEXT("Roughness")), FName(TEXT("Emission")),
				FName(TEXT("FinishType")), FName(TEXT("FinishDetail")), FName(TEXT("NormalStrength")),
				FName(TEXT("GrainScale")), FName(TEXT("IndirectEmissionScale"))})
			{
				float Value = 0;
				TestTrue(*FString::Printf(TEXT("BOB finish exposes %s on %s"), *Parameter.ToString(), *Material->GetName()),
					Material->GetScalarParameterValue(FMaterialParameterInfo(Parameter), Value));
				TestTrue(TEXT("BOB material controls are finite and non-negative"), FMath::IsFinite(Value) && Value >= 0);
			}
			float IndirectEmissionScale = 0;
			Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("IndirectEmissionScale")), IndirectEmissionScale);
			TestEqual(TEXT("BOB emissive fixtures retain restrained indirect lighting"), IndirectEmissionScale, .25f);
			if (Material->GetFName() == TEXT("MI_01_BOB_Surface"))
			{
				float FinishType = 0, Roughness = 0;
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FinishType")), FinishType);
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
				TestEqual(TEXT("BOB board uses the upgraded ivory surface finish"), FinishType, 1.f);
				TestEqual(TEXT("BOB board retains its polished material roughness"), Roughness, .26f);
			}
			if (Material->GetFName() == TEXT("MI_09_Light_Composite"))
			{
				float FinishType = 0;
				Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FinishType")), FinishType);
				TestEqual(TEXT("BOB stadium uses the upgraded architectural composite finish"), FinishType, 3.f);
			}
		}
		Presentation.Add(MoveTemp(Snapshot));
	}

	auto* Floor = Arena->FindComponentByClass<UProceduralMeshComponent>();
	TArray<FVector> FloorVertices;
	TArray<uint32> FloorIndices;
	if (TestNotNull(TEXT("Dedicated welded BOB floor collision"), Floor))
	{
		TestTrue(TEXT("The physical BOB floor stays on its original height"), Floor->GetRelativeLocation().Equals(FVector(0, 0, 250)));
		TestEqual(TEXT("The physical floor retains collision"), Floor->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		CheckPhysicalMaterial(Floor, .035f, .18f);
		if (const FProcMeshSection* Section = Floor->GetProcMeshSection(0))
		{
			for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer) FloorVertices.Add(Vertex.Position);
			FloorIndices = Section->ProcIndexBuffer;
		}
		TestTrue(TEXT("Planar collision has actual vertices"), !FloorVertices.IsEmpty());
	}
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UStaticMeshComponent* Rail = FindMesh(*FString::Printf(TEXT("Rail_%d"), Index));
		if (!TestNotNull(TEXT("Separate physical BOB rail"), Rail)) continue;
		const bool bVertical = Index >= 2;
		const float Side = Index % 2 == 0 ? -1.f : 1.f;
		TestTrue(TEXT("Rail collision keeps its original location"), Rail->GetRelativeLocation().Equals(
			bVertical ? FVector(Side * 632.f, 0, 271.f) : FVector(0, Side * 632.f, 271.f)));
		TestTrue(TEXT("Rail collision keeps its original dimensions"), Rail->GetRelativeScale3D().Equals(
			bVertical ? FVector(.34f, 13.08f, .42f) : FVector(13.08f, .34f, .42f)));
		TestEqual(TEXT("Separate rails retain collision"), Rail->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		CheckPhysicalMaterial(Rail, .04f, .48f);
	}

	// Arena configuration is applied on construction, BeginPlay and replication.
	// Those paths must not grow material chains or rebuild/change the physics mesh.
	Arena->InitializeArena(620.f, 54.f, 250.f);
	World->BeginPlay();
	for (const FPresentationSnapshot& Snapshot : Presentation)
	{
		TestTrue(TEXT("Repeated configuration keeps the same presentation mesh"), Snapshot.Component->GetStaticMesh() == Snapshot.Mesh);
		TestTrue(TEXT("Repeated configuration preserves presentation transforms"),
			Snapshot.Component->GetRelativeTransform().Equals(Snapshot.Transform));
		for (int32 Slot = 0; Slot < Snapshot.Materials.Num(); ++Slot)
			TestTrue(TEXT("Repeated configuration preserves shared BOB materials"), Snapshot.Component->GetMaterial(Slot) == Snapshot.Materials[Slot]);
	}
	if (Floor)
	{
		CheckPhysicalMaterial(Floor, .035f, .18f);
		const FProcMeshSection* Section = Floor->GetProcMeshSection(0);
		if (TestNotNull(TEXT("The same welded floor survives visual configuration"), Section))
		{
			TestEqual(TEXT("Visual setup does not change floor collision vertex count"), Section->ProcVertexBuffer.Num(), FloorVertices.Num());
			TestTrue(TEXT("Visual setup does not change floor collision indices"), Section->ProcIndexBuffer == FloorIndices);
			for (int32 Index = 0; Index < FMath::Min(Section->ProcVertexBuffer.Num(), FloorVertices.Num()); ++Index)
				TestTrue(TEXT("Visual setup does not move any floor collision vertex"), Section->ProcVertexBuffer[Index].Position.Equals(FloorVertices[Index]));
		}
	}
	const FFlickModeRules& Rules = FlickModeRules::Get(EFlickMatchVariant::Bob);
	TestEqual(TEXT("BOB piece friction is unchanged by presentation"), Rules.PieceFriction, .05f);
	TestEqual(TEXT("BOB piece restitution is unchanged by presentation"), Rules.PieceRestitution, .28f);
	TestEqual(TEXT("BOB linear damping is unchanged by presentation"), Rules.LinearDamping, .18f);
	TestEqual(TEXT("BOB angular damping is unchanged by presentation"), Rules.AngularDamping, 1.35f);
	TestEqual(TEXT("BOB shot strength is unchanged by presentation"), Rules.MaxLaunchSpeed, 1320.f);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif
