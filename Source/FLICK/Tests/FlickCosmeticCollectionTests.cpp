#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickCosmeticCollectionTest, "FLICK.Cosmetics.CollectionPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickCosmeticCollectionTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Cosmetic test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	for (int32 Style = 3; Style <= 5; ++Style)
	{
		for (EFlickFeedbackKind Kind : {EFlickFeedbackKind::Spawn, EFlickFeedbackKind::Elimination, EFlickFeedbackKind::Launch})
		{
			AFlickWorldFeedback* Effect = World->SpawnActor<AFlickWorldFeedback>();
			if (!TestNotNull(TEXT("Collection effect actor"), Effect)) continue;
			Effect->InitializeFeedback(Kind, FLinearColor::White, 0.8f, FVector::ForwardVector, Style);
			TestTrue(TEXT("Chosen effects replicate to match participants"), Effect->GetIsReplicated());
			TestEqual(TEXT("Spawn and knockout are batched; launches retain their three halos"), TInlineComponentArray<UProceduralMeshComponent*>(Effect).Num(), Kind == EFlickFeedbackKind::Launch ? 3 : 1);
			for (int32 Frame = 0; Frame < 260; ++Frame) Effect->Tick(1.0f / 120.0f);
			for (UPrimitiveComponent* Component : TInlineComponentArray<UPrimitiveComponent*>(Effect))
			{
				TestEqual(TEXT("All cosmetic geometry has no collision"), Component->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
				TestFalse(TEXT("Effects cannot add physics bodies"), Component->IsSimulatingPhysics());
				TestFalse(TEXT("Finite effect transforms throughout lifetime"), Component->GetComponentTransform().ContainsNaN());
				TestTrue(TEXT("Effect finishes with zero-scale particles or hidden rings"),
					!Component->IsVisible() || Component->GetComponentScale().SizeSquared() < 0.001f);
			}
			Effect->Destroy();
		}
		AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
		if (!TestNotNull(TEXT("Trail test puck"), Piece)) continue;
		Piece->InitializePiece(EFlickTeam::Player1, Style, 45.0f, 20.0f, EFlickPieceArchetype::Standard);
		Piece->SetPuckEffects({Style, Style, Style});
		TestEqual(TEXT("New selection is applied to the puck"), Piece->GetPuckEffect(0), Style);
		for (int32 Frame = 0; Frame < 35; ++Frame)
		{
			Piece->SetActorLocation(FVector(100.0f + Frame * 9.0f, 100.0f, 20.0f));
			Piece->Tick(1.0f / 45.0f);
		}
		const auto Ribbons = TInlineComponentArray<UProceduralMeshComponent*>(Piece);
		TestEqual(TEXT("One bounded ribbon component per puck"), Ribbons.Num(), 1);
		if (!Ribbons.IsEmpty())
		{
			UProceduralMeshComponent* Ribbon = Ribbons[0];
			TestTrue(TEXT("Moving puck displays its ribbon"), Ribbon->IsVisible());
			TestEqual(TEXT("Trail never changes collisions"), Ribbon->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			TestTrue(TEXT("Ribbon lanes have independent colour instances"), Ribbon->GetMaterial(0) != Ribbon->GetMaterial(1));
			const FProcMeshSection* Section = Ribbon->GetProcMeshSection(0);
			TestTrue(TEXT("Trail geometry is bounded at 48 samples per strand"), Section && Section->ProcVertexBuffer.Num() <= 672 && Section->ProcVertexBuffer.Num() >= 4);
			for (int32 Frame = 0; Frame < 40; ++Frame) Piece->Tick(1.0f / 45.0f);
			TestFalse(TEXT("Stationary trail expires rather than piling up"), Ribbon->IsVisible());
			Piece->SetPuckEffects({0, 0, 0});
			Piece->Tick(0.025f);
			TestFalse(TEXT("None removes the collection ribbon"), Ribbon->IsVisible());
		}
		Piece->Destroy();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
