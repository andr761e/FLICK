#include "Feedback/FlickWorldFeedback.h"
#include "Core/FlickSpawnStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	using ESpawnPattern = FlickSpawnStyle::EPattern;
	// Four batched layers: feathered rings, ribbons, shaped motes, crystal facets.
	// Positions are analytic functions of age, not frame-rate-dependent particles.
	struct FSpawnBatch
	{
		TArray<FVector> Vertices, Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<int32> Indices;
		void Vertex(const FVector& P, FVector2D UV, const FLinearColor& C)
		{
			Vertices.Add(P); Normals.Add(FVector::UpVector); UVs.Add(UV); Colors.Add(C.ToFColor(false));
		}
		void Pair(const FVector& P, const FVector& Side, float Width, const FLinearColor& C, float U)
		{
			Vertex(P - Side * Width, {U,0}, C); Vertex(P + Side * Width, {U,1}, C);
		}
		void Join(int32 Start, int32 Pairs)
		{
			for (int32 I = 0; I < Pairs - 1; ++I)
			{
				const int32 A = Start + I * 2;
				Indices.Append({A,A+1,A+2, A+1,A+3,A+2});
			}
		}
		void Quad(const FVector& P, const FVector& Right, const FVector& Up, float W, float H, const FLinearColor& C)
		{
			const int32 A = Vertices.Num();
			Vertex(P-Right*W-Up*H,{0,1},C); Vertex(P+Right*W-Up*H,{1,1},C);
			Vertex(P-Right*W+Up*H,{0,0},C); Vertex(P+Right*W+Up*H,{1,0},C);
			Indices.Append({A,A+1,A+2,A+1,A+3,A+2});
		}
		void Beam(const FVector& P, const FVector& Side, float W, float H, const FLinearColor& C)
		{
			const int32 Start = Vertices.Num();
			Pair(P-FVector(0,0,H),Side,W,C.CopyWithNewOpacity(0),0);
			Pair(P-FVector(0,0,H*.85f),Side,W,C,.075f);
			Pair(P+FVector(0,0,H*.65f),Side,W,C,.825f);
			Pair(P+FVector(0,0,H),Side,W,C.CopyWithNewOpacity(0),1);
			Join(Start,4);
		}
		void Ring(float R, float W, float Z, float Phase, const FLinearColor& C, float Span = 2*PI,
			const FVector& X = FVector::ForwardVector, const FVector& Y = FVector::RightVector)
		{
			const int32 A = Vertices.Num();
			for (int32 I = 0; I <= 48; ++I)
			{
				const float T = I/48.f, Angle = Phase + T*Span;
				const FVector D = X*FMath::Cos(Angle) + Y*FMath::Sin(Angle);
				Pair(D*R + FVector(0,0,Z), D, W, C, T);
			}
			Join(A,49);
		}
		void Crystal(const FVector& Base, float Angle, float Width, float Height, const FLinearColor& C)
		{
			const FVector Tip = Base + FVector(FMath::Cos(Angle)*Height*.18f,FMath::Sin(Angle)*Height*.18f,Height);
			for (int32 Side = 0; Side < 4; ++Side)
			{
				const int32 A = Vertices.Num();
				const float T = Angle + Side*PI/2;
				const FLinearColor Facet = (C * (Side % 2 ? .3f : 1.f)).CopyWithNewOpacity(C.A);
				Vertex(Base + FVector(FMath::Cos(T)*Width,FMath::Sin(T)*Width,Height*.25f),{.5f,.5f},Facet);
				Vertex(Base + FVector(FMath::Cos(T+PI/2)*Width,FMath::Sin(T+PI/2)*Width,Height*.25f),{.5f,.5f},Facet);
				Vertex(Tip,{.5f,.5f},Facet); Vertex(Base,{.5f,.5f},Facet);
				Indices.Append({A,A+1,A+2,A+1,A,A+3});
			}
		}
		void Upload(UProceduralMeshComponent& Mesh, int32 Section) const
		{
			if (Vertices.IsEmpty()) { Mesh.ClearMeshSection(Section); return; }
			const FProcMeshSection* Old = Mesh.GetProcMeshSection(Section);
			if (Old && Old->ProcVertexBuffer.Num() == Vertices.Num() && Old->ProcIndexBuffer.Num() == Indices.Num())
				Mesh.UpdateMeshSection(Section, Vertices, Normals, UVs, Colors, TArray<FProcMeshTangent>());
			else Mesh.CreateMeshSection(Section, Vertices, Indices, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
		}
	};
	float SpawnSeed(float I)
	{
		return FMath::Frac(FMath::Abs(FMath::Sin(I*127.1f + 17.7f)*43758.5453f));
	}
	float Smooth(float A, float B, float T) { return FMath::SmoothStep(A,B,T); }
}

void AFlickWorldFeedback::InitializeSpawnEffect()
{
	// The old static cubes/sphere must never render underneath the new arrival.
	CoreFlash->SetVisibility(false);
	for (UStaticMeshComponent* Shard : Shards) Shard->SetVisibility(false);
	for (UProceduralMeshComponent* Ring : HaloRings) Ring->SetVisibility(false);
	const auto& Look = FlickSpawnStyle::Get(Style);
	Duration = Appearance.ArrivalDuration + Look.Afterglow;
	SetLifeSpan(Duration + .1f);
	if (SpawnGeometry) SpawnGeometry->SetVisibility(false);
	if (Style == 0 || GetNetMode() == NM_DedicatedServer) return;
	if (!SpawnGeometry)
	{
		UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Cosmetics/Spawns/M_PuckSpawn_SoftV1.M_PuckSpawn_SoftV1"));
		if (!Source) return; // An opaque fallback would obscure the puck.
		SpawnGeometry = NewObject<UProceduralMeshComponent>(this, TEXT("SpawnPresentation"));
		SpawnGeometry->SetupAttachment(SceneRoot);
		SpawnGeometry->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SpawnGeometry->SetGenerateOverlapEvents(false);
		SpawnGeometry->SetCastShadow(false);
		SpawnGeometry->SetCanEverAffectNavigation(false);
		SpawnGeometry->SetReceivesDecals(false);
		SpawnGeometry->RegisterComponent();
		for (int32 Section = 0; Section < 4; ++Section)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Source, this);
			SpawnMaterials.Add(Material); SpawnGeometry->SetMaterial(Section, Material);
		}
	}
	for (int32 Section = 0; Section < SpawnMaterials.Num(); ++Section)
	{
		// The spawn shader compensates exposure, so use restrained scene-linear energy.
		SpawnMaterials[Section]->SetScalarParameterValue(TEXT("GlowStrength"), Look.Glow);
		SpawnMaterials[Section]->SetScalarParameterValue(TEXT("MaskShape"), Section == 2 ? Look.ParticleShape
			: Section == 3 ? (Look.Pattern == ESpawnPattern::Fire || Look.Pattern == ESpawnPattern::Spirit ? 7 : 3)
			: Section == 1 && (Look.Pattern == ESpawnPattern::Fire || Look.Pattern == ESpawnPattern::Spirit) ? 6 : 0);
	}
	UpdateSpawnEffect();
}

void AFlickWorldFeedback::UpdateSpawnEffect()
{
	if (!SpawnGeometry || Style == 0 || GetNetMode() == NM_DedicatedServer) return;
	if (Age >= Duration) { SpawnGeometry->SetVisibility(false); SpawnGeometry->ClearAllMeshSections(); return; }
	const auto& Look = FlickSpawnStyle::Get(Style);
	const ESpawnPattern Pattern = Look.Pattern;
	const float Landing = FMath::Max(.1f, Appearance.ArrivalDuration);
	const float Progress = FMath::Clamp(Age/Landing,0.f,1.f);
	const float Fade = 1-Smooth(Landing,Duration,Age);
	const float Envelope = Smooth(0,.12f,Age)*Fade;
	const float Pulse = FMath::Exp(-FMath::Square((Age-Landing)/.13f));
	const float Strength = FMath::Lerp(.7f,1.f,FMath::Clamp(Appearance.Strength,0.f,1.f));
	const float R = Look.Radius, H = Look.Height;
	const auto Color = [&](float T, float Opacity)
	{
		const FLinearColor C = Pattern == ESpawnPattern::Prism ? FlickSpawnStyle::Spectrum(T)
			: FMath::Lerp(Look.Primary,Look.Secondary,FMath::Clamp(T,0.f,1.f));
		return C.CopyWithNewOpacity(FMath::Clamp(Opacity*Envelope*Strength,0.f,1.f));
	};
	FVector Right = FVector::RightVector, Up = FVector::UpVector;
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			const FRotationMatrix View(PC->PlayerCameraManager->GetCameraRotation());
			Right = GetActorTransform().InverseTransformVectorNoScale(View.GetUnitAxis(EAxis::Y));
			Up = GetActorTransform().InverseTransformVectorNoScale(View.GetUnitAxis(EAxis::Z));
		}
	}
	FSpawnBatch Rings, Ribbons, Motes, Facets;
	// Ground seals stay outside the puck footprint. No oversized white core flash.
	Rings.Ring(R,2.5f,1,Age*.65f,Color(.15f,.6f));
	Rings.Ring(R+12+Progress*8,1.4f,1.5f,-Age*.8f,Color(.8f,.28f),PI*1.65f);
	Rings.Ring(R+12+Smooth(Landing,Duration,Age)*34,4.5f,2,0,Color(.5f,Pulse*.85f));
	for (int32 I = 0; I < 12; ++I)
	{
		const float A = I*PI/6 + Age*.1f;
		const FVector D(FMath::Cos(A),FMath::Sin(A),0);
		Rings.Quad(D*(R+8)+FVector(0,0,2),D,FVector(-D.Y,D.X,0),3.5f,.7f,Color(I/12.f,.35f));
	}
	const bool bBeam = Pattern == ESpawnPattern::Pulse || Pattern == ESpawnPattern::Beam || Pattern == ESpawnPattern::Prism || Pattern == ESpawnPattern::Gold;
	if (bBeam)
	{
		const int32 Shafts = Pattern == ESpawnPattern::Prism ? 7 : Pattern == ESpawnPattern::Beam ? 10 : 6;
		for (int32 I = 0; I < Shafts; ++I)
		{
			const float A = I*2*PI/Shafts;
			const FVector P(FMath::Cos(A)*R*.78f,FMath::Sin(A)*R*.78f,H*.5f+2);
			Ribbons.Beam(P,Right,Pattern == ESpawnPattern::Prism ? 7 : 3,H*.5f,
				Color(I/float(Shafts),(.18f+.10f*FMath::Sin(Age*4+I))*(1-Smooth(.7f,1.f,Progress))));
			Ribbons.Beam(P,Right,.6f,H*.5f,Color(I/float(Shafts),.48f*(1-Smooth(.85f,1.f,Progress))));
		}
		for (int32 I = 0; I < 3; ++I)
			Rings.Ring(R*(.8f+I*.07f),1.6f,(1-Progress)*H*(.35f+I*.25f)+4,Age*.5f,
				Color(I/3.f,.65f*(1-Smooth(.85f,1.f,Progress))));
	}
	if (Pattern == ESpawnPattern::Portal)
	{
		for (int32 I = 0; I < 3; ++I)
		{
			const float Z = (1-Smooth(0,1,Progress))*(40+I*35)+3;
			Rings.Ring(R*(.8f+I*.12f),2.8f,Z,Age*(I%2 ? -2 : 2),Color(I/2.f,.7f),PI*1.75f);
		}
	}
	if (Pattern == ESpawnPattern::Fire || Pattern == ESpawnPattern::Spirit || Pattern == ESpawnPattern::Sparks)
	{
		const int32 Strands = Pattern == ESpawnPattern::Sparks ? 2 : 4;
		for (int32 Strand = 0; Strand < Strands; ++Strand)
		{
			const int32 Start = Ribbons.Vertices.Num();
			for (int32 I = 0; I <= 40; ++I)
			{
				const float T = I/40.f, A = Strand*2*PI/Strands + T*7 - Age*2.8f;
				const float Radius = R*(.75f-.5f*T)*(1+.1f*FMath::Sin(T*21+Age*4));
				const FVector D(FMath::Cos(A),FMath::Sin(A),0);
				const FVector P = D*Radius + FVector(0,0,3+T*H);
				const float Opacity = FMath::Sin(T*PI)*.48f*(1-Smooth(.7f,1,Progress)*.7f);
				Ribbons.Pair(P,D,Pattern == ESpawnPattern::Sparks ? 1.1f : (12-8*T),Color(T,Opacity),T);
			}
			Ribbons.Join(Start,41);
		}
		if (Pattern != ESpawnPattern::Sparks)
		{
			// Broad, tapered turbulent tongues beneath the finer twisting ribbons.
			// Camera-facing cards are depth-tested against the puck, not drawn over it.
			for (int32 I = 0; I < 12; ++I)
			{
				const float A = I*2.39996f + Age*.4f;
				const float Height = H*(.35f+.5f*SpawnSeed(I+9));
				const float Radius = R*(.45f+.4f*SpawnSeed(I+3));
				Facets.Quad(FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,Height*.5f+3),
					Right,FVector::UpVector,10+SpawnSeed(I)*9,Height*.5f,
					Color(Pattern == ESpawnPattern::Fire ? .2f+SpawnSeed(I)*.3f : SpawnSeed(I),
						.72f*(1-Smooth(.9f,1.f,Progress)*.6f)));
			}
		}
	}
	if (Pattern == ESpawnPattern::Ice)
	{
		for (int32 I = 0; I < 10; ++I)
		{
			const float A = I*2*PI/10, Scale = Smooth(0,.4f,Progress)*(1-Smooth(Landing+.08f,Duration,Age));
			Facets.Crystal(FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,2),A,7*Scale,
				H*(.35f+.6f*SpawnSeed(I))*Scale,Color(SpawnSeed(I+11),.65f));
		}
	}
	if (Pattern == ESpawnPattern::Lightning)
	{
		const float Flicker = .55f+.45f*FMath::Square(FMath::Sin(Age*16));
		for (int32 Bolt = 0; Bolt < 5; ++Bolt)
		{
			const int32 Start = Ribbons.Vertices.Num();
			const float A = Bolt*2*PI/5 + .25f*FMath::Sin(Age*5);
			const FVector D(FMath::Cos(A),FMath::Sin(A),0), Side(-D.Y,D.X,0);
			TArray<FVector> Branches;
			for (int32 I = 0; I <= 18; ++I)
			{
				const float T = I/18.f;
				const float Jitter = FMath::Sin(I*39+Bolt*11+FMath::FloorToFloat(Age*12+.001f))*12*FMath::Sin(T*PI);
				const FVector P = D*(R*(1-T)) + Side*Jitter + FVector(0,0,3+T*H);
				Ribbons.Pair(P,Right,1.1f,Color(Bolt/5.f,Flicker*.75f*(1-Smooth(.93f,1.f,Progress))),T);
				if (I == 6 || I == 12)
					Branches.Add(P + Side*10);
			}
			Ribbons.Join(Start,19);
			for (const FVector& Branch : Branches)
				Ribbons.Quad(Branch,Side,Up,12,.7f,Color(.5f,Flicker*.55f));
		}
	}
	if (Pattern == ESpawnPattern::Galaxy)
	{
		Motes.Quad(FVector(0,0,H*.65f),Right,Up,11,11,Color(.4f,.85f));
		const FVector X(1,0,0), Y(0,.75f,.66f);
		for (int32 Arm = 0; Arm < 3; ++Arm)
		{
			const int32 Start = Ribbons.Vertices.Num();
			for (int32 I = 0; I <= 48; ++I)
			{
				const float T = I/48.f, A = Arm*2*PI/3+T*7+Age;
				const FVector D = X*FMath::Cos(A)+Y*FMath::Sin(A);
				Ribbons.Pair(D*(8+T*R) + FVector(0,0,H*.65f),D,3+8*T,
					Color(T,.5f*FMath::Sin(T*PI)*(1-Smooth(.8f,1,Progress))),T);
			}
			Ribbons.Join(Start,49);
		}
	}
	// Motifs are not generic cubes: masks provide diamonds, hearts, stars or voxels.
	for (int32 I = 0; I < Look.Particles; ++I)
	{
		const float S = SpawnSeed(I+4), T = SpawnSeed(I+17), Local = FMath::Frac(Age*.65f + T);
		const float A = I*2.39996f+Age*(Pattern == ESpawnPattern::Hearts ? .6f : .9f);
		float Radius = R*(.55f+.6f*S), Z = 6+Local*H;
		if (Pattern == ESpawnPattern::Pixel) Z = 6+(1-Local)*H;
		if (Pattern == ESpawnPattern::Sparks || Pattern == ESpawnPattern::Fire)
		{
			const float Burst = FMath::Clamp((Age-Landing)/Look.Afterglow,0.f,1.f);
			Radius += Burst*40; Z = 8+Local*H*.65f+FMath::Sin(Burst*PI)*25;
		}
		const float Opacity = FMath::Sin(Local*PI)*.7f;
		const float Size = Pattern == ESpawnPattern::Hearts ? 5+S*4 : Pattern == ESpawnPattern::Pixel ? 2+S*4 : 1.5f+S*2;
		Motes.Quad(FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,Z),Right,Up,Size,Size,
			Color(I/float(Look.Particles),Opacity));
	}
	Rings.Upload(*SpawnGeometry,0); Ribbons.Upload(*SpawnGeometry,1);
	Motes.Upload(*SpawnGeometry,2); Facets.Upload(*SpawnGeometry,3);
	SpawnGeometry->SetVisibility(true);
}
