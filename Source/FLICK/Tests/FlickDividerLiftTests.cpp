#if WITH_DEV_AUTOMATION_TESTS
#include "Arena/FlickTestArena.h"
#include "Components/StaticMeshComponent.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Pieces/FlickPiece.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickDividerLiftTest, "FLICK.Arena.DividerLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickDividerLiftTest::RunTest(const FString& Parameters)
{
	for (const float StepSeconds : {1.f / 30.f, 1.f / 60.f, 1.f / 120.f})
	for (const EFlickPieceArchetype Type : {EFlickPieceArchetype::Standard, EFlickPieceArchetype::Heavy})
	for (const float OffsetFraction : {0.f, .65f})
	{
		const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
			.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(true);
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
		if (!TestNotNull(TEXT("Physics test world"), World)) return false;
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		AFlickTestArena* Arena = World->SpawnActor<AFlickTestArena>();
		Arena->InitializeTestArena(650.f, 50.f, 250.f);
		Arena->SetTrainingLayoutSeed(1337);
		World->BeginPlay();
		UStaticMeshComponent* Divider = nullptr;
		TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
		for (UStaticMeshComponent* Component : Components)
			if (Component->GetName() == TEXT("EdgeDivider_00")) Divider = Component;
		if (!TestNotNull(TEXT("Divider collider"), Divider)) return false;
		AFlickPiece* Rider = World->SpawnActor<AFlickPiece>();
		const auto& Rules = FlickPieceArchetypeRules::Get(Type);
		Rider->PieceMassKg *= Rules.MassMultiplier;
		Rider->PieceFriction *= Rules.FrictionMultiplier;
		Rider->PieceRestitution *= Rules.RestitutionMultiplier;
		Rider->LinearDamping *= Rules.LinearDampingMultiplier;
		Rider->AngularDamping *= Rules.AngularDampingMultiplier;
		Rider->CenterOfMassOffsetZ = 20.f * Rules.ThicknessMultiplier * Rules.CenterOfMassHeightFraction;
		Rider->InitializePiece(EFlickTeam::Player1, 1, 45.f * Rules.RadiusMultiplier, 20.f * Rules.ThicknessMultiplier, Type);
		const FVector Center = Arena->GetDividerWorldCenter(0);
		const FVector2D Tangent = Arena->GetDividerWorldTangent(0);
		FVector2D Inward(-Tangent.Y, Tangent.X);
		if (FVector2D::DotProduct(Inward, FVector2D(Center)) > 0.f) Inward *= -1.f;
		const FVector2D RiderXY = FVector2D(Center) + Inward * Rider->GetPieceRadius() * OffsetFraction;
		Rider->SetActorLocation(FVector(RiderXY, 250.f + Rider->GetPieceThickness() * .5f + 3.f), false, nullptr, ETeleportType::TeleportPhysics);
		AFlickPiece* Activator = World->SpawnActor<AFlickPiece>();
		Activator->InitializePiece(EFlickTeam::Player2, 2, 45.f, 20.f, EFlickPieceArchetype::Standard);
		const FVector Switch = Arena->GetSwitchWorldCenter(0);
		Activator->SetActorLocation(FVector(Switch.X - 110.f, Switch.Y, 263.f), false, nullptr, ETeleportType::TeleportPhysics);
		TArray<TObjectPtr<AFlickPiece>> Pieces{Rider, Activator};
		for (int32 Frame = 0; Frame < FMath::CeilToInt(.5f / StepSeconds); ++Frame)
		{
			++GFrameCounter; World->Tick(LEVELTICK_All, StepSeconds);
		}
		const float RestZ = Rider->GetActorLocation().Z;
		Cast<UPrimitiveComponent>(Rider->GetRootComponent())->PutRigidBodyToSleep();
		Arena->BeginControlZoneTracking(Pieces);
		Activator->SetActorLocation(FVector(Switch.X, Switch.Y, 263.f), false, nullptr, ETeleportType::TeleportPhysics);
		uint16 Deployed = 0;
		const uint16 Triggered = Arena->TrackControlZoneCrossings(Pieces, 0.f, Deployed);
		TestTrue(TEXT("Switch activates while its dormant divider is occupied"), (Triggered & 1) != 0);
		TestTrue(TEXT("Activation cue precedes lift"), Arena->IsDividerPending(0) && !Arena->IsDividerRaised(0));
		Activator->Eliminate();
		float PeakZ = RestZ;
		float PeakSpeed = 0.f;
		float PreviousDividerZ = Divider->GetComponentLocation().Z;
		for (int32 Frame = 0; Frame < FMath::CeilToInt(3.f / StepSeconds); ++Frame)
		{
			Arena->TrackControlZoneCrossings(Pieces, StepSeconds, Deployed);
			const float Z = Divider->GetComponentLocation().Z;
			TestTrue(TEXT("Collider rises in bounded steps instead of appearing inside the puck"),
				Z - PreviousDividerZ <= (Arena->DividerHeight + 2.f) * StepSeconds / Arena->DividerRiseDuration + .01f);
			PreviousDividerZ = Z;
			++GFrameCounter; World->Tick(LEVELTICK_All, StepSeconds);
			PeakZ = FMath::Max(PeakZ, Rider->GetActorLocation().Z);
			PeakSpeed = FMath::Max(PeakSpeed, Rider->GetLinearVelocity().Size());
			TestFalse(TEXT("Lift produces finite physics"), Rider->GetActorLocation().ContainsNaN());
		}
		AddInfo(FString::Printf(TEXT("%s %s at %.0f Hz: lift %.1f cm, peak speed %.1f cm/s"),
			Type == EFlickPieceArchetype::Standard ? TEXT("Standard") : TEXT("Heavy"),
			OffsetFraction == 0.f ? TEXT("centred") : TEXT("partial overlap"), 1.f / StepSeconds, PeakZ - RestZ, PeakSpeed));
		TestTrue(TEXT("Collider contact lifts or tips a sleeping puck; glancing overlaps may slip off"),
			OffsetFraction == 0.f ? PeakZ > RestZ + 25.f :
			(PeakZ > RestZ + 1.f || FVector2D::DotProduct(FVector2D(Rider->GetActorLocation()) - RiderXY, Inward) > 10.f));
		TestTrue(TEXT("No explosive launch"), PeakSpeed < 1200.f);
		TestTrue(TEXT("Occupied divider completes deployment"), Arena->IsDividerRaised(0) && !Arena->HasMovingDividers());
		TestFalse(TEXT("Launched puck does not balance on the crown"), Arena->HasPiecesOnDividerCrowns(Pieces));
		Arena->CommitPendingControlZoneToggles(Pieces);
		const float RaisedZ = Divider->GetComponentLocation().Z;
		Arena->BeginReplayPresentation();
		TArray<float> ReplayLifts = Arena->GetDividerLiftFractions();
		ReplayLifts[0] = .5f;
		Arena->ApplyReplayDividerState(Arena->GetRaisedDividerMask(), ReplayLifts);
		TestTrue(TEXT("Replay renders recorded partial lift rather than snapping to full height"),
			FMath::IsNearlyEqual(Divider->GetComponentLocation().Z, RaisedZ - (Arena->DividerHeight + 2.f) * .5f, .01f));
		Arena->EndReplayPresentation();
		TestTrue(TEXT("Replay restores the live raised collider"), FMath::IsNearlyEqual(Divider->GetComponentLocation().Z, RaisedZ, .01f));
		// A fresh crossing retracts the divider through the same bounded movement.
		// Remove the tossed rider: it may now rest on this or another switch and disarm that zone.
		Rider->Eliminate();
		Activator = World->SpawnActor<AFlickPiece>();
		Activator->InitializePiece(EFlickTeam::Player2, 3, 45.f, 20.f, EFlickPieceArchetype::Standard);
		Activator->SetActorLocation(FVector(Switch.X - 110.f, Switch.Y, 263.f), false, nullptr, ETeleportType::TeleportPhysics);
		Pieces.Add(Activator);
		Arena->BeginControlZoneTracking(Pieces);
		Activator->SetActorLocation(FVector(Switch.X, Switch.Y, 263.f), false, nullptr, ETeleportType::TeleportPhysics);
		Arena->TrackControlZoneCrossings(Pieces, 0.f, Deployed);
		Activator->Eliminate();
		PreviousDividerZ = RaisedZ;
		for (int32 Frame = 0; Frame < FMath::CeilToInt(.6f / StepSeconds); ++Frame)
		{
			Arena->TrackControlZoneCrossings(Pieces, StepSeconds, Deployed);
			const float Z = Divider->GetComponentLocation().Z;
			TestTrue(TEXT("Retraction is bounded and downward"), Z <= PreviousDividerZ + .01f
				&& PreviousDividerZ - Z <= (Arena->DividerHeight + 2.f) * StepSeconds / Arena->DividerRetractionDuration + .01f);
			PreviousDividerZ = Z;
			++GFrameCounter; World->Tick(LEVELTICK_All, StepSeconds);
		}
		TestFalse(TEXT("Retraction completes for the tested divider"), Arena->IsDividerRaised(0));
		TestTrue(TEXT("Tested divider fully retracts"), FMath::IsNearlyZero(Arena->GetDividerLiftFractions()[0]));
		// Drop onto an already raised crown, independently of switch crossings.
		for (const float LandingOffset : {-.65f, 0.f, .65f})
		{
			Arena->RestoreTrainingMechanisms(1);
			AFlickPiece* Landing = World->SpawnActor<AFlickPiece>();
			Landing->InitializePiece(EFlickTeam::Player1, 4, 45.f, 20.f, EFlickPieceArchetype::Standard);
			const FVector2D StartXY = FVector2D(Center) + Inward * 45.f * LandingOffset;
			Landing->SetActorLocation(FVector(StartXY, 250.f + Arena->DividerHeight + 90.f), false, nullptr, ETeleportType::TeleportPhysics);
			TArray<TObjectPtr<AFlickPiece>> LandingPieces{Landing};
			for (int32 Frame = 0; Frame < FMath::CeilToInt(3.f / StepSeconds); ++Frame)
			{
				Arena->DeflectPiecesFromDividerCrowns(LandingPieces);
				++GFrameCounter; World->Tick(LEVELTICK_All, StepSeconds);
			}
			TestFalse(TEXT("Landing puck cannot remain on a divider crown"), Arena->HasPiecesOnDividerCrowns(LandingPieces));
			const float Displacement = FVector2D::DotProduct(FVector2D(Landing->GetActorLocation()) - StartXY, Inward);
			TestTrue(TEXT("Landing tips toward its greater overhang (centred tie goes inward)"),
				LandingOffset < 0.f ? Displacement < -10.f : Displacement > 10.f);
			Landing->Eliminate();
		}
		Arena->ResetMechanisms();
		TestFalse(TEXT("Reset clears lift and pending state"), Arena->HasMovingDividers());
		TestEqual(TEXT("Reset disables retracted collision"), Divider->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
	}
	return true;
}
#endif
