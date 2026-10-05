#include "Pieces/FlickPiece.h"
#include "Core/FlickTrailStyle.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	constexpr float SampleInterval = 1.0f / 60.0f;
	constexpr int32 MaxSamples = 48;
	constexpr float MaxTrailLength = 420.0f;
	constexpr float TeleportDistance = 450.0f;
	constexpr float MinimumSpeed = 12.0f;

	// Two batched sections: feathered ribbons and camera-facing particles.
	// No particle actors, collision bodies or lights.
	struct FTrailBatch
	{
		TArray<FVector> Vertices, Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<int32> Indices;
		void Pair(const FVector& Center, const FVector& Side, float Width, FLinearColor Color, float U)
		{
			Vertices.Append({Center - Side * Width, Center + Side * Width});
			Normals.Append({FVector::UpVector, FVector::UpVector});
			UVs.Append({FVector2D(U, 0), FVector2D(U, 1)});
			Colors.Append({Color.ToFColor(false), Color.ToFColor(false)});
		}
		void Quad(const FVector& Center, const FVector& Right, const FVector& Up, float Size, float Height, FLinearColor Color)
		{
			const int32 A = Vertices.Num();
			Vertices.Append({Center - Right * Size - Up * Height, Center + Right * Size - Up * Height,
				Center - Right * Size + Up * Height, Center + Right * Size + Up * Height});
			const FVector Normal = FVector::CrossProduct(Right, Up).GetSafeNormal();
			Normals.Append({Normal, Normal, Normal, Normal});
			UVs.Append({{0,1}, {1,1}, {0,0}, {1,0}});
			for (int32 I = 0; I < 4; ++I) Colors.Add(Color.ToFColor(false));
			Indices.Append({A, A+1, A+2, A+1, A+3, A+2});
		}
		void Upload(UProceduralMeshComponent* Mesh, int32 Section) const
		{
			if (Vertices.IsEmpty()) { Mesh->ClearMeshSection(Section); return; }
			const FProcMeshSection* Existing = Mesh->GetProcMeshSection(Section);
			if (Existing && Existing->ProcVertexBuffer.Num() == Vertices.Num() && Existing->ProcIndexBuffer.Num() == Indices.Num())
				Mesh->UpdateMeshSection(Section, Vertices, Normals, UVs, Colors, TArray<FProcMeshTangent>());
			else Mesh->CreateMeshSection(Section, Vertices, Indices, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
		}
	};
	float Noise(float Seed)
	{
		const float Value = FMath::Sin(Seed * 127.1f + 31.7f) * 43758.5453f;
		return Value - FMath::FloorToFloat(Value);
	}
}

void AFlickPiece::UpdateCollectionTrail(const float DeltaSeconds)
{
	if (GetNetMode() == NM_DedicatedServer) return;
	const float Dt = FMath::Max(0.0f, DeltaSeconds);
	TrailClock += Dt;
	const FlickTrailStyle::FStyle& Style = FlickTrailStyle::Get(PuckTrail);
	if (!CosmeticRibbon)
	{
		UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Cosmetics/Trails/M_PuckTrail_SoftV1.M_PuckTrail_SoftV1"));
		if (!Source) return; // Never fall back to an opaque strip on a missing shader.
		CosmeticRibbon = NewObject<UProceduralMeshComponent>(this);
		CosmeticRibbon->SetupAttachment(PieceMesh);
		CosmeticRibbon->SetAbsolute(true, true, true);
		CosmeticRibbon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CosmeticRibbon->SetGenerateOverlapEvents(false);
		CosmeticRibbon->SetCastShadow(false);
		CosmeticRibbon->SetCanEverAffectNavigation(false);
		CosmeticRibbon->SetReceivesDecals(false);
		CosmeticRibbon->RegisterComponent();
		for (int32 Section = 0; Section < 2; ++Section)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Source, this);
			CosmeticRibbon->SetMaterial(Section, Material);
			RibbonMaterials.Add(Material);
		}
	}
	CosmeticRibbon->SetWorldTransform(FTransform::Identity);
	RibbonMaterials[0]->SetScalarParameterValue(TEXT("MaskShape"), PuckTrail == 2 || PuckTrail == 4 ? 6
		: PuckTrail == 9 || PuckTrail == 12 ? 7 : 0);
	RibbonMaterials[1]->SetScalarParameterValue(TEXT("MaskShape"), Style.ParticleShape);
	for (UMaterialInstanceDynamic* Material : RibbonMaterials)
		// Arena exposure is deliberately low; preserve visible HDR colour without
		// changing global lighting or introducing expensive per-particle lights.
		Material->SetScalarParameterValue(TEXT("GlowStrength"), Style.Glow * 5.0f);
	if (TrailPointAges.Num() != TrailPoints.Num()) { TrailPoints.Reset(); TrailPointAges.Reset(); }
	for (float& Age : TrailPointAges) Age += Dt;
	const FVector Position = GetActorLocation() - FVector(0, 0, PieceThickness * 0.38f);
	if (LastTrailPosition.IsZero()) LastTrailPosition = Position;
	const float Distance = FVector::Dist2D(Position, LastTrailPosition);
	const bool bTeleported = Distance > TeleportDistance || Dt > 0.2f;
	if (bTeleported)
	{
		TrailPoints.Reset(); TrailPointAges.Reset(); TrailSampleElapsed = 0;
		LastTrailPosition = Position; // Never connect two previews or respawn positions.
	}
	const bool bArriving = VisualRoot && VisualRoot->GetRelativeLocation().Z > 1.0f;
	const bool bMoving = !bTeleported && !bEliminated && !bArriving && Dt > SMALL_NUMBER && Distance / Dt > MinimumSpeed;
	if (bMoving)
	{
		// Interpolate at fixed intervals even at 30 FPS; do not emit once per frame.
		float SampleTime = SampleInterval - TrailSampleElapsed;
		for (int32 Samples = 0; SampleTime <= Dt && Samples < 12; ++Samples, SampleTime += SampleInterval)
		{
			TrailPoints.Insert(FMath::Lerp(LastTrailPosition, Position, SampleTime / Dt), 0);
			TrailPointAges.Insert(Dt - SampleTime, 0);
		}
		TrailSampleElapsed = FMath::Fmod(TrailSampleElapsed + Dt, SampleInterval);
	}
	else TrailSampleElapsed = 0;
	LastTrailPosition = Position;
	while (!TrailPoints.IsEmpty() && (TrailPoints.Num() > MaxSamples || TrailPointAges.Last() >= Style.Lifetime
		|| FVector::DistSquared2D(Position, TrailPoints.Last()) > FMath::Square(MaxTrailLength)))
	{
		TrailPoints.Pop(EAllowShrinking::No); TrailPointAges.Pop(EAllowShrinking::No);
	}
	CosmeticRibbon->SetVisibility(TrailPoints.Num() >= 2);
	if (TrailPoints.Num() < 2) return;
	FVector CameraRight = FVector::ForwardVector, CameraUp = FVector::RightVector;
	if (const APlayerController* Viewer = GetWorld()->GetFirstPlayerController())
		if (Viewer->PlayerCameraManager)
		{
			const FRotationMatrix Basis(Viewer->PlayerCameraManager->GetCameraRotation());
			CameraRight = Basis.GetUnitAxis(EAxis::Y); CameraUp = Basis.GetUnitAxis(EAxis::Z);
		}
	const int32 Count = TrailPoints.Num();
	FTrailBatch Ribbons, Particles;
	for (int32 Lane = 0; Lane < Style.Lanes; ++Lane)
	{
		const int32 Start = Ribbons.Vertices.Num();
		for (int32 I = 0; I < Count; ++I)
		{
			const FVector Tangent = (TrailPoints[FMath::Max(0, I-1)] - TrailPoints[FMath::Min(Count-1, I+1)]).GetSafeNormal2D();
			const FVector Side(-Tangent.Y, Tangent.X, 0);
			const float Age = TrailPointAges[I], Life = FMath::Clamp(1 - Age / Style.Lifetime, 0.0f, 1.0f);
			const float Cap = FMath::Clamp((Count - 1 - I) / 4.0f, 0.0f, 1.0f);
			const float Birth = TrailClock - Age;
			const float Strand = Lane - (Style.Lanes - 1) * 0.5f;
			float Width = Lane == 0 ? Style.Width : 1.25f;
			float Offset = Lane == 0 ? 0 : Strand * 6;
			float Opacity = Lane == 0 ? .24f : .8f;
			FLinearColor Color = Lane == 0 ? Style.Primary : FMath::Lerp(Style.Primary, Style.Secondary, Lane / float(Style.Lanes - 1));
			const float Wave = FMath::Sin(Birth * 17 + Lane * 2.1f + TrailClock * 3);
			switch (PuckTrail)
			{
			case 1: Offset *= 0.65f; if (Lane == 1) { Offset = 0; Width = 1.8f; Color = Style.Secondary; } break;
			case 2: case 4:
				Offset = Lane == 0 ? 0 : Strand * 5 + Wave * 6 * (1-Life);
				Width = Lane == 0 ? Style.Width : (Lane == 1 ? 3.2f : 5.2f) * (.65f + .35f * Wave);
				Opacity = Lane == 0 ? .14f : .45f; break;
			case 3: Offset = Lane == 0 ? 0 : Strand * 6 + Wave * 1.8f; break;
			case 5: case 12:
				Offset = Lane == 0 ? 0 : Strand * 5 + Wave * (PuckTrail == 12 ? 8 : 6);
				Width = Lane == 0 ? Style.Width : (PuckTrail == 12 ? 5.0f : 2.8f);
				Opacity = Lane == 0 ? .12f : .32f; break;
			case 6:
				Offset = Lane == 0 ? 0 : Strand * 5 + (Noise(Birth * 19 + Lane * 7 + FMath::FloorToFloat(TrailClock * 12)) - .5f) * 18;
				Width = Lane == 0 ? Style.Width : (Lane == 1 ? .85f : .5f); Opacity = Lane == 0 ? .1f : .85f; break;
			case 7: Color = FlickTrailStyle::Rainbow(Lane / 7.0f); Offset = Strand * 4; Width = 3.0f; Opacity = .52f; break;
			case 8: Width = Lane == 0 ? Style.Width : .85f; Offset += Wave * 1.3f; break;
			case 9:
				Offset = Strand * 8 + Wave * 5; Width = Lane == 0 ? Style.Width : 9;
				Color = FMath::Lerp(Style.Primary, Style.Secondary, .5f + Wave * .5f); Opacity = .16f; break;
			case 10: Width = Lane == 0 ? Style.Width : .75f; Offset = 0; Opacity = Lane == 0 ? .08f : .35f; break;
			case 11: Offset = Strand * 4 + Wave * 3; Width = Lane == 0 ? Style.Width : 2; Opacity = .17f; break;
			default: break;
			}
			Color.A = FMath::Min(0.95f, Opacity * 1.15f) * FMath::Pow(Life, 1.1f) * Cap;
			const FVector Center = TrailPoints[I] + Side * Offset * FMath::Sqrt(Life) + FVector(0,0,1.5f + Lane*.18f);
			Ribbons.Pair(Center, Side, Width * FMath::Sqrt(Life) * Cap, Color, Age / Style.Lifetime);
			if (I < Count - 1)
			{
				const int32 A = Start + I * 2;
				Ribbons.Indices.Append({A, A+1, A+2, A+1, A+3, A+2});
			}
		}
	}
	for (int32 I = 1; I < Count; ++I)
	{
		const float Age = TrailPointAges[I], Life = FMath::Clamp(1 - Age / Style.Lifetime, 0.0f, 1.0f);
		// Birth IDs remain stable as history shifts; no flickering random particles.
		const float Seed = FMath::RoundToFloat((TrailClock - Age) / SampleInterval);
		if (FMath::RoundToInt(Seed) % Style.ParticleStride != 0) continue;
		const float N = Noise(Seed), N2 = Noise(Seed + 67);
		const FVector Tangent = (TrailPoints[I-1] - TrailPoints[FMath::Min(Count-1,I+1)]).GetSafeNormal2D();
		const FVector Side(-Tangent.Y, Tangent.X, 0);
		float Size = FMath::Lerp(1.3f, 3.8f, N), Spread = 26, Rise = 14, Opacity = .8f;
		if (PuckTrail == 2) { Size = FMath::Lerp(2.5f, 5.5f, N); Rise = 20; Spread = 26; }
		if (PuckTrail == 4) { Size *= .7f; Rise = 27; Spread = 35; }
		if (PuckTrail == 3) { Size *= 1.2f; Rise = 10; Spread = 33; }
		if (PuckTrail == 9) { Size *= .9f; Rise = 12; Spread = 50; }
		if (PuckTrail == 10) { Size = FMath::Lerp(2.5f, 5.5f, N); Rise = 8; Spread = 33; }
		if (PuckTrail == 11) { Size = FMath::Lerp(5.f, 8.f, N); Rise = 24; Spread = 36; Opacity = .6f; }
		if (PuckTrail == 12) { Size = FMath::Lerp(7.f, 14.f, N); Rise = 28; Spread = 38; Opacity = .16f; }
		FLinearColor Color = PuckTrail == 7 ? FlickTrailStyle::Rainbow(N) : FMath::Lerp(Style.Primary, Style.Secondary, N2);
		Color.A = FMath::Min(0.95f, Opacity * 1.3f) * FMath::Pow(Life, 1.2f) * FMath::Clamp(Age / .06f, 0.0f, 1.0f);
		const FVector Center = TrailPoints[I] + Side * (N-.5f) * (6 + Spread * (1-Life)) + FVector(0,0,3 + Age * Rise);
		const float Rotation = PuckTrail == 10 ? 0 : (N2-.5f) * 1.1f + Age * (PuckTrail == 11 ? .5f : 2.5f);
		const FVector Right = CameraRight * FMath::Cos(Rotation) + CameraUp * FMath::Sin(Rotation);
		const FVector Up = CameraUp * FMath::Cos(Rotation) - CameraRight * FMath::Sin(Rotation);
		Size *= FMath::Sqrt(Life);
		Particles.Quad(Center, Right, Up, Size, Size * (PuckTrail == 2 ? 1.8f : 1.f), Color);
	}
	Ribbons.Upload(CosmeticRibbon, 0);
	Particles.Upload(CosmeticRibbon, 1);
}
