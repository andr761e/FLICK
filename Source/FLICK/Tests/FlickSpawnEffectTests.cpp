#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickSpawnStyle.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionEyeAdaptationInverse.h"
#include "Materials/MaterialExpressionPower.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickSpawnEffectTest, "FLICK.Cosmetics.SpawnEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickSpawnEffectTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Spawn catalog and renderer stay in sync"),
		FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::SpawnCategory).Num(), FlickSpawnStyle::Count);
	const TCHAR* Original[] = {TEXT("BASIC DROP"),TEXT("PULSE ARRIVAL"),TEXT("SPARK ARRIVAL"),
		TEXT("CRYO LOCK"),TEXT("SOLAR FLARE"),TEXT("PHASE GATE")};
	for (int32 I = 0; I < 6; ++I) TestEqual(TEXT("Saved spawn selections keep their meaning"),
		FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::SpawnCategory)[I], FString(Original[I]));
	for (int32 I = 6; I < FlickSpawnStyle::Count; ++I)
	{
		TestEqual(TEXT("New spawns do not index collection palettes"),
			FlickCosmeticCatalog::GetCollection(FlickCosmeticCatalog::SpawnCategory,I),INDEX_NONE);
		TestTrue(TEXT("New styles have useful descriptions"),
			FlickCosmeticCatalog::GetDescription(FlickCosmeticCatalog::SpawnCategory,I).Len() > 30);
	}
	UMaterial* Material = LoadObject<UMaterial>(nullptr,TEXT("/Game/Cosmetics/Spawns/M_PuckSpawn_SoftV1.M_PuckSpawn_SoftV1"));
	if (TestNotNull(TEXT("Spawn shader exists and is included in packaging"),Material))
	{
		TestEqual(TEXT("Spawn surfaces are additive, not opaque"),Material->GetBlendMode(),BLEND_Additive);
		TestTrue(TEXT("Spawn shader needs no per-effect lights"),Material->GetShadingModels().HasShadingModel(MSM_Unlit));
#if WITH_EDITOR
		const auto* E = Material->GetExpressionInputForProperty(MP_EmissiveColor);
		const auto* Inverse = E ? Cast<UMaterialExpressionEyeAdaptationInverse>(E->Expression) : nullptr;
		const auto* Glow = Inverse ? Cast<UMaterialExpressionMultiply>(Inverse->LightValueInput.Expression) : nullptr;
		const auto* Saturation = Glow ? Cast<UMaterialExpressionPower>(Glow->A.Expression) : nullptr;
		TestTrue(TEXT("Real vertex RGB drives saturated, exposure-compensated emission"),
			Saturation && Cast<UMaterialExpressionVertexColor>(Saturation->Base.Expression));
		const auto* O = Material->GetExpressionInputForProperty(MP_Opacity);
		const auto* Fade = O ? Cast<UMaterialExpressionMultiply>(O->Expression) : nullptr;
		TestTrue(TEXT("Vertex alpha drives transparency"),Fade && Fade->A.OutputIndex == 4);
		const auto* Mask = Fade ? Cast<UMaterialExpressionCustom>(Fade->B.Expression) : nullptr;
		bool bConnected = Mask && Mask->Inputs.Num() == 3;
		if (Mask) for (const FCustomInput& Input : Mask->Inputs) bConnected &= Input.Input.Expression != nullptr;
		TestTrue(TEXT("Every analytic mask input is connected"),bConnected);
#endif
	}
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Settings);
	if (!TestNotNull(TEXT("Spawn test world"),World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	for (int32 I = 0; I < FlickSpawnStyle::Count; ++I)
	{
		TArray<FVector> Reference;
		for (int32 FPS : {30,60,120})
		{
			const FString Case = FString::Printf(TEXT("Spawn %d at %d FPS: "),I,FPS);
			AFlickWorldFeedback* Effect = World->SpawnActor<AFlickWorldFeedback>();
			if (!TestNotNull(Case+TEXT("actor"),Effect)) continue;
			Effect->InitializeFeedback(EFlickFeedbackKind::Spawn,FLinearColor::White,.8f,FVector::ZeroVector,I,1.2f);
			TestTrue(Case+TEXT("replicated selection"),Effect->GetIsReplicated());
			for (int32 Frame = 0; Frame < FPS/2; ++Frame) Effect->Tick(1.f/FPS);
			const auto Meshes = TInlineComponentArray<UProceduralMeshComponent*>(Effect);
			TestEqual(Case+TEXT("one bounded component; Basic Drop has none"),Meshes.Num(),I == 0 ? 0 : 1);
			for (UStaticMeshComponent* Old : TInlineComponentArray<UStaticMeshComponent*>(Effect))
				TestFalse(Case+TEXT("old cubes and core cannot show through"),Old->IsVisible());
			if (!Meshes.IsEmpty())
			{
				UProceduralMeshComponent* Mesh = Meshes[0];
				TestTrue(Case+TEXT("visible arrival"),Mesh->IsVisible());
				TestFalse(Case+TEXT("no physics"),Mesh->IsSimulatingPhysics());
				TestFalse(Case+TEXT("no shadow"),Mesh->CastShadow);
				TestFalse(Case+TEXT("no overlaps"),Mesh->GetGenerateOverlapEvents());
				TestEqual(Case+TEXT("no collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
				int32 Count = 0; bool bFinite = true, bBounds = true, bIndices = true;
				TArray<FVector> Positions;
				for (int32 Section = 0; Section < Mesh->GetNumSections(); ++Section)
				{
					if (const FProcMeshSection* Batch = Mesh->GetProcMeshSection(Section))
					{
						Count += Batch->ProcVertexBuffer.Num();
						for (const FProcMeshVertex& V : Batch->ProcVertexBuffer)
						{
							bFinite &= !V.Position.ContainsNaN() && !V.UV0.ContainsNaN();
							bBounds &= V.Position.Size() < 260 && V.Position.Z >= -1;
							Positions.Add(V.Position);
						}
						for (uint32 Index : Batch->ProcIndexBuffer) bIndices &= Index < static_cast<uint32>(Batch->ProcVertexBuffer.Num());
						TestFalse(Case+TEXT("no cooked collision geometry"),Batch->bEnableCollision);
					}
				}
				TestTrue(Case+TEXT("geometry bounded below 1800 vertices"),Count > 300 && Count <= 1800);
				TestTrue(Case+TEXT("finite vertices and UVs"),bFinite);
				TestTrue(Case+TEXT("effect remains local and above floor"),bBounds);
				TestTrue(Case+TEXT("triangle indices stay valid"),bIndices);
				if (FPS == 30) Reference = Positions;
				else
				{
					bool bEqual = Reference.Num() == Positions.Num();
					for (int32 V = 0; bEqual && V < Positions.Num(); ++V) bEqual &= Reference[V].Equals(Positions[V],.05f);
					TestTrue(Case+TEXT("age-driven animation matches across frame rates"),bEqual);
				}
				for (int32 Frame = 0; Frame < FPS*2; ++Frame) Effect->Tick(1.f/FPS);
				TestFalse(Case+TEXT("finished effect is invisible"),Mesh->IsVisible());
				TestEqual(Case+TEXT("finished geometry is released"),Mesh->GetNumSections(),0);
			}
			Effect->Destroy();
		}
	}
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
