#include "Debug/FlickPhysicsDiagnosticsComponent.h"
#include "Arena/FlickBobArena.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "ProceduralMeshComponent.h"

#if !UE_BUILD_SHIPPING
namespace
{
	TAutoConsoleVariable<int32> Enabled(TEXT("flick.Diagnostics"), 0, TEXT("Local physics diagnostics: 0 off, 1 on."), ECVF_Cheat);
	TAutoConsoleVariable<int32> Collision(TEXT("flick.Diagnostics.Collision"), 1, TEXT("Draw active collision geometry."), ECVF_Cheat);
	TAutoConsoleVariable<int32> Velocity(TEXT("flick.Diagnostics.Velocity"), 1, TEXT("Draw puck velocity arrows (0.15 seconds of travel)."), ECVF_Cheat);
	TAutoConsoleVariable<int32> Normals(TEXT("flick.Diagnostics.Contacts"), 1, TEXT("Draw recent actual hit normals."), ECVF_Cheat);
	TAutoConsoleVariable<int32> Pockets(TEXT("flick.Diagnostics.Pockets"), 1, TEXT("Draw BOB openings and capture depth."), ECVF_Cheat);

	void DrawCollision(UWorld* World, UPrimitiveComponent* Component)
	{
		if (!Component->IsCollisionEnabled()) return;
		const FTransform Transform = Component->GetComponentTransform();
		const FColor Colour = FColor::Cyan;
		if (UProceduralMeshComponent* Floor = Cast<UProceduralMeshComponent>(Component))
		{
			for (int32 SectionIndex = 0; SectionIndex < Floor->GetNumSections(); ++SectionIndex)
			{
				const FProcMeshSection* Section = Floor->GetProcMeshSection(SectionIndex);
				if (!Section || !Section->bEnableCollision) continue;
				for (int32 I = 0; I + 2 < Section->ProcIndexBuffer.Num(); I += 3)
				for (int32 Edge = 0; Edge < 3; ++Edge)
					DrawDebugLine(World, Transform.TransformPosition(Section->ProcVertexBuffer[Section->ProcIndexBuffer[I + Edge]].Position),
						Transform.TransformPosition(Section->ProcVertexBuffer[Section->ProcIndexBuffer[I + (Edge + 1) % 3]].Position), Colour);
			}
			return;
		}
		UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
		UBodySetup* Body = Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetBodySetup() : nullptr;
		if (!Body) return;
		for (const FKBoxElem& Box : Body->AggGeom.BoxElems)
		{
			const FTransform BoxWorld = Box.GetTransform() * Transform;
			DrawDebugBox(World, BoxWorld.GetLocation(), FVector(Box.X, Box.Y, Box.Z) * .5 * BoxWorld.GetScale3D().GetAbs(), BoxWorld.GetRotation(), Colour);
		}
		for (const FKConvexElem& Hull : Body->AggGeom.ConvexElems)
		{
			const FTransform HullWorld = Hull.GetTransform() * Transform;
			for (int32 I = 0; I + 2 < Hull.IndexData.Num(); I += 3)
			for (int32 Edge = 0; Edge < 3; ++Edge)
				DrawDebugLine(World, HullWorld.TransformPosition(Hull.VertexData[Hull.IndexData[I + Edge]]),
					HullWorld.TransformPosition(Hull.VertexData[Hull.IndexData[I + (Edge + 1) % 3]]), Colour);
		}
	}
}
#endif

UFlickPhysicsDiagnosticsComponent::UFlickPhysicsDiagnosticsComponent()
{
	PrimaryComponentTick.bCanEverTick = !UE_BUILD_SHIPPING;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UFlickPhysicsDiagnosticsComponent::IsEnabled()
{
#if !UE_BUILD_SHIPPING
	return Enabled.GetValueOnGameThread() != 0;
#else
	return false;
#endif
}

void UFlickPhysicsDiagnosticsComponent::ClearBindings()
{
	for (auto& Component : Observed)
		if (Component.IsValid()) Component->OnComponentHit.RemoveDynamic(this, &UFlickPhysicsDiagnosticsComponent::RecordContact);
	Observed.Reset();
	Contacts.Reset();
	Summary.Reset();
}

void UFlickPhysicsDiagnosticsComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearBindings();
	Super::EndPlay(Reason);
}

void UFlickPhysicsDiagnosticsComponent::RecordContact(UPrimitiveComponent* Component, AActor* Other,
	UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Hit)
{
	if (!IsEnabled()) return;
	if (Contacts.Num() >= 32) Contacts.RemoveAt(0);
	Contacts.Add({Hit.ImpactPoint, Hit.ImpactNormal, GetWorld()->GetTimeSeconds(),
		FString::Printf(TEXT("%s -> %s / %s | normal %s"), *GetNameSafe(Component->GetOwner()),
			*GetNameSafe(Other), *GetNameSafe(OtherComponent), *Hit.ImpactNormal.ToCompactString())});
}

void UFlickPhysicsDiagnosticsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
#if !UE_BUILD_SHIPPING
	AFlickPlayerController* Viewer = Cast<AFlickPlayerController>(GetOwner());
	if (!Viewer || !Viewer->IsLocalController() || !IsEnabled())
	{
		if (!Observed.IsEmpty()) ClearBindings();
		return;
	}
	UWorld* World = GetWorld();
	Observed.RemoveAll([](const auto& Component) { return !Component.IsValid(); });
	Contacts.RemoveAll([World](const FContact& Contact) { return World->GetTimeSeconds() - Contact.Time > 2.0f; });
	int32 ActivePucks = 0;
	for (TActorIterator<AFlickPiece> It(World); It; ++It)
	{
		AFlickPiece* Piece = *It;
		if (!Piece->IsActive()) continue;
		++ActivePucks;
		UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Piece->GetRootComponent());
		if (Root && !Observed.Contains(Root))
		{
			Root->OnComponentHit.AddUniqueDynamic(this, &UFlickPhysicsDiagnosticsComponent::RecordContact);
			Observed.Add(Root);
		}
		if (Velocity.GetValueOnGameThread())
			DrawDebugDirectionalArrow(World, Piece->GetActorLocation(), Piece->GetActorLocation() + Piece->GetLinearVelocity() * .15f, 12, FColor::Yellow, false, 0, 0, 1.5f);
	}
	if (Collision.GetValueOnGameThread())
		for (TActorIterator<AActor> It(World); It; ++It)
			for (UPrimitiveComponent* Component : TInlineComponentArray<UPrimitiveComponent*>(*It)) DrawCollision(World, Component);
	if (Normals.GetValueOnGameThread())
		for (const FContact& Contact : Contacts)
			DrawDebugDirectionalArrow(World, Contact.Point, Contact.Point + Contact.Normal * 45, 10, FColor::Green, false, 0, 0, 2);
	AFlickBobArena* Bob = nullptr;
	for (TActorIterator<AFlickBobArena> It(World); It; ++It) { Bob = *It; break; }
	if (Bob && Pockets.GetValueOnGameThread())
		for (int32 I = 0; I < 4; ++I)
		{
			const FVector Center = Bob->GetPocketWorldLocation(I) - FVector(0, 0, 2);
			DrawDebugCylinder(World, Center, Center - FVector(0, 0, Bob->PocketCaptureDepth), Bob->GetPocketRadius(), 32, FColor::Magenta);
		}
	Summary = FString::Printf(TEXT("PHYSICS DIAGNOSTICS | %d active pucks\nCyan collision | yellow velocity | green hit normals\nMagenta BOB pockets / capture depth"), ActivePucks);
	const AFlickPiece* Piece = Viewer->GetInspectedPiece();
	if (!Piece) Piece = Viewer->GetSelectedPiece();
	if (Piece)
	{
		Summary += FString::Printf(TEXT("\nPuck %d | speed %.1f cm/s | spin %.1f deg/s\nPosition %s"), Piece->GetPieceId(),
			Piece->GetLinearVelocity().Size(), Piece->GetAngularVelocityDegrees().Size(), *Piece->GetActorLocation().ToCompactString());
		FHitResult Support;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(FlickDiagnosticsSupport), false, Piece);
		if (World->LineTraceSingleByObjectType(Support, Piece->GetActorLocation() + FVector(0, 0, 25),
			Piece->GetActorLocation() - FVector(0, 0, 200), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
			Summary += FString::Printf(TEXT("\nSupport: %s | Z %.3f | normal %s"), *GetNameSafe(Support.GetComponent()),
				Support.ImpactPoint.Z, *Support.ImpactNormal.ToCompactString());
		else Summary += TEXT("\nNo static support below this puck.");
		if (Bob)
			Summary += FString::Printf(TEXT("\nBOB depth %.1f / %.1f cm | inside %s | captured %s"),
				Bob->GetSurfaceZ() - Piece->GetActorLocation().Z, Bob->PocketCaptureDepth,
				Bob->IsInsidePocket(Piece->GetActorLocation()) ? TEXT("yes") : TEXT("no"),
				Bob->IsCapturedByPocket(Piece->GetActorLocation(), Piece->GetPieceRadius()) ? TEXT("yes") : TEXT("no"));
	}
	else Summary += TEXT("\nHover or select a puck to inspect it.");
	if (!Contacts.IsEmpty()) Summary += TEXT("\nLast hit: ") + Contacts.Last().Description;
#endif
}
