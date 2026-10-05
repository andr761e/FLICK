#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickKnockoutStyle.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Pieces/FlickPiece.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickKnockoutEffectTest,"FLICK.Cosmetics.KnockoutEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickKnockoutEffectTest::RunTest(const FString& Parameters)
{
	const auto& Items = FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::KnockoutCategory);
	TestEqual(TEXT("Knockout catalog and renderer stay in sync"),Items.Num(),FlickKnockoutStyle::Count);
	TestEqual(TEXT("Existing profile key remains stable"),FlickCosmeticCatalog::GetConfigKey(FlickCosmeticCatalog::KnockoutCategory),FString(TEXT("KnockoutEffect")));
	const TCHAR* Original[] = {TEXT("BASIC BURST"),TEXT("SHOCKWAVE"),TEXT("SPARK SHOWER"),
		TEXT("CRYO SHATTER"),TEXT("SOLAR NOVA"),TEXT("PHASE COLLAPSE")};
	for (int32 I = 0; I < 6; ++I) TestEqual(TEXT("Saved selections preserve their meaning"),Items[I],FString(Original[I]));
	for (int32 I = 0; I < FlickKnockoutStyle::Count; ++I)
	{
		TestTrue(TEXT("Every knockout has a helpful description"),FlickCosmeticCatalog::GetDescription(FlickCosmeticCatalog::KnockoutCategory,I).Len() > 30);
		if (I >= 6) TestEqual(TEXT("Standalone knockout does not index collection palettes"),
			FlickCosmeticCatalog::GetCollection(FlickCosmeticCatalog::KnockoutCategory,I),INDEX_NONE);
		const auto& Look = FlickKnockoutStyle::Get(I);
		TestTrue(TEXT("All effects are brief and local"),Look.Duration > 0 && Look.Duration <= 1.8f && Look.Radius <= 180);
	}
	UMaterial* Material = LoadObject<UMaterial>(nullptr,TEXT("/Game/Cosmetics/Knockouts/M_PuckKnockout_SoftV1.M_PuckKnockout_SoftV1"));
	if (TestNotNull(TEXT("Knockout shader asset exists"),Material))
	{
		TestEqual(TEXT("Knockout surfaces are not opaque"),Material->GetBlendMode(),BLEND_Additive);
		TestTrue(TEXT("No scene lights are needed"),Material->GetShadingModels().HasShadingModel(MSM_Unlit));
#if WITH_EDITOR
		const auto* E = Material->GetExpressionInputForProperty(MP_EmissiveColor);
		const auto* Inverse = E ? Cast<UMaterialExpressionEyeAdaptationInverse>(E->Expression) : nullptr;
		const auto* Glow = Inverse ? Cast<UMaterialExpressionMultiply>(Inverse->LightValueInput.Expression) : nullptr;
		const auto* Power = Glow ? Cast<UMaterialExpressionPower>(Glow->A.Expression) : nullptr;
		TestTrue(TEXT("Saturated real RGB is exposure compensated"),Power && Cast<UMaterialExpressionVertexColor>(Power->Base.Expression));
		const auto* O = Material->GetExpressionInputForProperty(MP_Opacity);
		const auto* Fade = O ? Cast<UMaterialExpressionMultiply>(O->Expression) : nullptr;
		TestTrue(TEXT("Vertex alpha controls transparency"),Fade && Fade->A.OutputIndex == 4);
		const auto* Mask = Fade ? Cast<UMaterialExpressionCustom>(Fade->B.Expression) : nullptr;
		bool bConnected = Mask && Mask->Inputs.Num() == 3;
		if (Mask) for (const FCustomInput& Input : Mask->Inputs) bConnected &= Input.Input.Expression != nullptr;
		TestTrue(TEXT("Every mask input is wired"),bConnected);
#endif
	}
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Settings);
	if (!TestNotNull(TEXT("Knockout test world"),World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	for (int32 I = 1; I < FlickKnockoutStyle::Count; ++I)
	{
		TArray<FVector> Reference;
		for (int32 FPS : {30,60,120})
		{
			const FString Case = FString::Printf(TEXT("Knockout %d at %d FPS: "),I,FPS);
			AFlickWorldFeedback* Effect = World->SpawnActor<AFlickWorldFeedback>();
			if (!TestNotNull(Case+TEXT("actor"),Effect)) continue;
			Effect->InitializeFeedback(EFlickFeedbackKind::Elimination,FLinearColor::White,.8f,FVector::ForwardVector,I);
			TestTrue(Case+TEXT("selection replicates"),Effect->GetIsReplicated());
			for (int32 Frame = 0; Frame < FPS/2; ++Frame) Effect->Tick(1.f/FPS);
			const auto Meshes = TInlineComponentArray<UProceduralMeshComponent*>(Effect);
			TestEqual(Case+TEXT("one bounded component"),Meshes.Num(),1);
			for (UStaticMeshComponent* Old : TInlineComponentArray<UStaticMeshComponent*>(Effect))
				TestFalse(Case+TEXT("old cubes and sphere are hidden"),Old->IsVisible());
			if (!Meshes.IsEmpty())
			{
				UProceduralMeshComponent* Mesh = Meshes[0];
				TestTrue(Case+TEXT("visible knockout"),Mesh->IsVisible());
				TestEqual(Case+TEXT("no collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
				TestFalse(Case+TEXT("no physics"),Mesh->IsSimulatingPhysics());
				TestFalse(Case+TEXT("no shadows"),Mesh->CastShadow);
				TestFalse(Case+TEXT("no overlaps"),Mesh->GetGenerateOverlapEvents());
				bool bFinite = true, bBounds = true, bIndices = true, bColor = false;
				int32 Count = 0;
				TArray<FVector> Positions;
				for (int32 Section = 0; Section < Mesh->GetNumSections(); ++Section)
					if (const FProcMeshSection* Batch = Mesh->GetProcMeshSection(Section))
					{
						Count += Batch->ProcVertexBuffer.Num();
						for (const FProcMeshVertex& V : Batch->ProcVertexBuffer)
						{
							bFinite &= !V.Position.ContainsNaN() && !V.UV0.ContainsNaN();
							bBounds &= V.Position.Size() < 280;
							bColor |= V.Color.A > 40 && (V.Color.R > 30 || V.Color.G > 30 || V.Color.B > 30);
							Positions.Add(V.Position);
						}
						for (uint32 Index : Batch->ProcIndexBuffer) bIndices &= Index < static_cast<uint32>(Batch->ProcVertexBuffer.Num());
						TestFalse(Case+TEXT("no collision sections"),Batch->bEnableCollision);
					}
				TestTrue(Case+TEXT("geometry budget below 2400 vertices"),Count > 300 && Count <= 2400);
				TestTrue(Case+TEXT("finite geometry"),bFinite);
				TestTrue(Case+TEXT("local effect bounds"),bBounds);
				TestTrue(Case+TEXT("valid triangles"),bIndices);
				TestTrue(Case+TEXT("visible vertex colour and opacity"),bColor);
				if (FPS == 30) Reference = Positions;
				else
				{
					bool bEqual = Reference.Num() == Positions.Num();
					for (int32 V = 0; bEqual && V < Positions.Num(); ++V) bEqual &= Reference[V].Equals(Positions[V],.05f);
					TestTrue(Case+TEXT("frame-rate independent motion"),bEqual);
				}
				for (int32 Frame = 0; Frame < FPS*2; ++Frame) Effect->Tick(1.f/FPS);
				TestFalse(Case+TEXT("finished effect is invisible"),Mesh->IsVisible());
				TestEqual(Case+TEXT("finished geometry is freed"),Mesh->GetNumSections(),0);
			}
			Effect->Destroy();
		}
		AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
		Piece->InitializePiece(EFlickTeam::Player1,I,45,20,EFlickPieceArchetype::Standard);
		Piece->SetPuckEffects({0,0,I});
		TestEqual(TEXT("Every new knockout can be equipped"),Piece->GetPuckEffect(2),I);
		Piece->Destroy();
	}
	for (int32 I : {0,-1,999})
	{
		AFlickWorldFeedback* Basic = World->SpawnActor<AFlickWorldFeedback>();
		Basic->InitializeFeedback(EFlickFeedbackKind::Elimination,FLinearColor::White,1,FVector::ZeroVector,I);
		TestEqual(TEXT("Basic and invalid selections retain the legacy burst"),TInlineComponentArray<UProceduralMeshComponent*>(Basic).Num(),0);
		Basic->Destroy();
	}
	World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	return true;
}
#endif
