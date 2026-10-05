#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickTrailStyle.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Pieces/FlickPiece.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPuckTrailTest, "FLICK.Cosmetics.PuckTrails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPuckTrailTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Catalog and renderer agree on saved indices"), FlickCosmeticCatalog::GetItems(FlickCosmeticCatalog::TrailCategory).Num(), FlickTrailStyle::Count);
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Cosmetics/Trails/M_PuckTrail_SoftV1.M_PuckTrail_SoftV1"));
	if (TestNotNull(TEXT("Packaged trail shader exists"), Material))
	{
		TestEqual(TEXT("Trails use transparency, not opaque arena strips"), Material->GetBlendMode(), BLEND_Additive);
		TestTrue(TEXT("Trail shader is unlit"), Material->GetShadingModels().HasShadingModel(MSM_Unlit));
#if WITH_EDITOR
		const FExpressionInput* Emissive = Material->GetMaterial()->GetExpressionInputForProperty(MP_EmissiveColor);
		const UMaterialExpressionMultiply* Glow = Emissive ? Cast<UMaterialExpressionMultiply>(Emissive->Expression) : nullptr;
		TestTrue(TEXT("Vertex palette actually drives emission (RGB output is unnamed)"),
			Glow && Cast<UMaterialExpressionVertexColor>(Glow->A.Expression) != nullptr);
		const FExpressionInput* Opacity = Material->GetMaterial()->GetExpressionInputForProperty(MP_Opacity);
		const UMaterialExpressionMultiply* Fade = Opacity ? Cast<UMaterialExpressionMultiply>(Opacity->Expression) : nullptr;
		const UMaterialExpressionCustom* Mask = Fade ? Cast<UMaterialExpressionCustom>(Fade->B.Expression) : nullptr;
		bool bConnectedMask = Mask && Mask->Inputs.Num() == 3;
		if (Mask) for (const FCustomInput& Input : Mask->Inputs) bConnectedMask &= Input.Input.Expression != nullptr;
		TestTrue(TEXT("Analytic masks have UV, shape and animation inputs connected"), bConnectedMask);
		TestTrue(TEXT("Lifetime uses vertex alpha, not vertex RGB"), Fade && Fade->A.OutputIndex == 4);
#endif
	}
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Trail test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	for (int32 Style = 1; Style < FlickTrailStyle::Count; ++Style)
	{
		for (int32 FPS : {30, 60, 120})
		{
			const FString Case = FString::Printf(TEXT("Style %d at %d FPS: "), Style, FPS);
			AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
			if (!TestNotNull(Case + TEXT("puck"), Piece)) continue;
			Piece->InitializePiece(EFlickTeam::Player1, 1, 45, 20, EFlickPieceArchetype::Standard);
			Piece->SetPregamePreview(true);
			Piece->SetPuckEffects({Style, 0, 0});
			for (int32 Frame = 0; Frame <= FPS; ++Frame)
			{
				Piece->SetActorLocation(FVector(100 + Frame * 360.0f / FPS, 100, 20));
				Piece->Tick(1.0f / FPS);
			}
			const auto Components = TInlineComponentArray<UProceduralMeshComponent*>(Piece);
			TestEqual(Case + TEXT("one render component"), Components.Num(), 1);
			if (!Components.IsEmpty())
			{
				UProceduralMeshComponent* Mesh = Components[0];
				TestTrue(Case + TEXT("visible moving trail"), Mesh->IsVisible());
				TestEqual(Case + TEXT("no collision"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
				TestFalse(Case + TEXT("no physics"), Mesh->IsSimulatingPhysics());
				TestEqual(Case + TEXT("two batched layers"), Mesh->GetNumSections(), 2);
				const FProcMeshSection* Ribbon = Mesh->GetProcMeshSection(0);
				const FProcMeshSection* Particles = Mesh->GetProcMeshSection(1);
				TestTrue(Case + TEXT("bounded ribbon geometry"), Ribbon && Ribbon->ProcVertexBuffer.Num() >= 4 && Ribbon->ProcVertexBuffer.Num() <= 672);
				TestTrue(Case + TEXT("bounded particle geometry"), Particles && Particles->ProcVertexBuffer.Num() > 0 && Particles->ProcVertexBuffer.Num() <= 192);
				if (Ribbon)
				{
					uint8 MaximumAlpha = 0, MinimumAlpha = 255;
					bool bFinite = true;
					for (const FProcMeshVertex& Vertex : Ribbon->ProcVertexBuffer)
					{
						MaximumAlpha = FMath::Max(MaximumAlpha, Vertex.Color.A);
						MinimumAlpha = FMath::Min(MinimumAlpha, Vertex.Color.A);
						bFinite &= !Vertex.Position.ContainsNaN() && !Vertex.UV0.ContainsNaN();
					}
					TestTrue(Case + TEXT("finite vertices and UVs"), bFinite);
					TestTrue(Case + TEXT("real per-point fade and transparent tail cap"), MaximumAlpha > 0 && MaximumAlpha < 255 && MinimumAlpha == 0);
				}
				for (int32 Frame = 0; Frame < FPS * 1.1f; ++Frame) Piece->Tick(1.0f / FPS);
				TestFalse(Case + TEXT("stationary trail finishes fading"), Mesh->IsVisible());
				Piece->SetActorLocation(FVector(5000,100,20)); Piece->Tick(1.0f / FPS);
				TestFalse(Case + TEXT("teleport cannot streak across arena"), Mesh->IsVisible());
				Piece->SetPuckEffects({0,0,0}); Piece->Tick(1.0f / FPS);
				TestFalse(Case + TEXT("None remains clean"), Mesh->IsVisible());
			}
			TestEqual(Case + TEXT("spawn selection untouched"), Piece->GetPuckEffect(1), 0);
			TestEqual(Case + TEXT("knockout selection untouched"), Piece->GetPuckEffect(2), 0);
			Piece->Destroy();
		}
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
