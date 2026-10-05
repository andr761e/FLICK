#include "Feedback/FlickWorldFeedback.h"
#include "Core/FlickKnockoutStyle.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace
{
	using EPattern = FlickKnockoutStyle::EPattern;
	// Four draw sections, no particle actors or physics bodies. Analytic trajectories
	// make the knockout read identically at different frame rates and in the locker.
	struct FKnockoutBatch
	{
		TArray<FVector> Vertices, Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<int32> Indices;
		void Vertex(const FVector& P, FVector2D UV, FLinearColor C)
		{
			Vertices.Add(P); Normals.Add(FVector::UpVector); UVs.Add(UV); Colors.Add(C.ToFColor(false));
		}
		void Pair(const FVector& P, const FVector& Side, float W, FLinearColor C, float U)
		{
			Vertex(P-Side*W,{U,0},C); Vertex(P+Side*W,{U,1},C);
		}
		void Join(int32 Start, int32 Pairs)
		{
			for (int32 I = 0; I < Pairs-1; ++I)
			{
				const int32 A = Start + I*2;
				Indices.Append({A,A+1,A+2,A+1,A+3,A+2});
			}
		}
		void Quad(const FVector& P, const FVector& Right, const FVector& Up, float W, float H, FLinearColor C)
		{
			const int32 A = Vertices.Num();
			Vertex(P-Right*W-Up*H,{0,1},C); Vertex(P+Right*W-Up*H,{1,1},C);
			Vertex(P-Right*W+Up*H,{0,0},C); Vertex(P+Right*W+Up*H,{1,0},C);
			Indices.Append({A,A+1,A+2,A+1,A+3,A+2});
		}
		void Ring(float R, float W, float Z, float Phase, FLinearColor C, float Span = 2*PI)
		{
			const int32 Start = Vertices.Num();
			for (int32 I = 0; I <= 48; ++I)
			{
				const float T = I/48.f, A = Phase+T*Span;
				const FVector D(FMath::Cos(A),FMath::Sin(A),0);
				Pair(D*R+FVector(0,0,Z),D,W,C,T);
			}
			Join(Start,49);
		}
		void Shard(const FVector& P, const FVector& Axis, const FVector& Side, float W, float H, FLinearColor C)
		{
			// Actual three-dimensional asymmetric bipyramid, with contrasting facets.
			const FVector Normal = FVector::CrossProduct(Axis,Side).GetSafeNormal();
			for (int32 I = 0; I < 4; ++I)
			{
				const float A = I*PI/2;
				const FVector V = Side*FMath::Cos(A)*W+Normal*FMath::Sin(A)*W;
				const FVector Next = Side*FMath::Cos(A+PI/2)*W+Normal*FMath::Sin(A+PI/2)*W;
				const int32 Start = Vertices.Num();
				const FLinearColor Facet = (C*(I%2 ? .35f : 1.f)).CopyWithNewOpacity(C.A);
				Vertex(P+V,{.5f,.5f},Facet); Vertex(P+Next,{.5f,.5f},Facet);
				Vertex(P+Axis*H,{.5f,.5f},Facet); Vertex(P-Axis*H*.6f,{.5f,.5f},Facet);
				Indices.Append({Start,Start+1,Start+2,Start+1,Start,Start+3});
			}
		}
		void Upload(UProceduralMeshComponent& Mesh, int32 Section) const
		{
			if (Vertices.IsEmpty()) { Mesh.ClearMeshSection(Section); return; }
			const FProcMeshSection* Old = Mesh.GetProcMeshSection(Section);
			if (Old && Old->ProcVertexBuffer.Num() == Vertices.Num() && Old->ProcIndexBuffer.Num() == Indices.Num())
				Mesh.UpdateMeshSection(Section,Vertices,Normals,UVs,Colors,TArray<FProcMeshTangent>());
			else Mesh.CreateMeshSection(Section,Vertices,Indices,Normals,UVs,Colors,TArray<FProcMeshTangent>(),false);
		}
	};
	float Seed(float I) { return FMath::Frac(FMath::Abs(FMath::Sin(I*127.1f+17.7f)*43758.5453f)); }
}

void AFlickWorldFeedback::InitializeKnockoutEffect()
{
	CoreFlash->SetVisibility(false);
	for (UStaticMeshComponent* Shard : Shards) Shard->SetVisibility(false);
	for (UProceduralMeshComponent* Ring : HaloRings) Ring->SetVisibility(false);
	const auto& Look = FlickKnockoutStyle::Get(Style);
	Duration = Look.Duration;
	SetLifeSpan(Duration+.1f);
	if (GetNetMode() == NM_DedicatedServer) return;
	if (!KnockoutGeometry)
	{
		UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Cosmetics/Knockouts/M_PuckKnockout_SoftV1.M_PuckKnockout_SoftV1"));
		if (!Source) return; // Never fall back to opaque cards covering the board.
		KnockoutGeometry = NewObject<UProceduralMeshComponent>(this,TEXT("KnockoutPresentation"));
		KnockoutGeometry->SetupAttachment(SceneRoot);
		KnockoutGeometry->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		KnockoutGeometry->SetGenerateOverlapEvents(false);
		KnockoutGeometry->SetCastShadow(false);
		KnockoutGeometry->SetCanEverAffectNavigation(false);
		KnockoutGeometry->SetReceivesDecals(false);
		KnockoutGeometry->RegisterComponent();
		for (int32 Section = 0; Section < 4; ++Section)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Source,this);
			KnockoutMaterials.Add(Material); KnockoutGeometry->SetMaterial(Section,Material);
		}
	}
	const bool bOrganic = Look.Pattern == EPattern::Toxic || Look.Pattern == EPattern::Spirit || Look.Pattern == EPattern::Smoke;
	for (int32 I = 0; I < KnockoutMaterials.Num(); ++I)
	{
		KnockoutMaterials[I]->SetScalarParameterValue(TEXT("GlowStrength"),Look.Glow);
		KnockoutMaterials[I]->SetScalarParameterValue(TEXT("MaskShape"),I == 2 ? Look.ParticleMask
			: I == 3 ? (bOrganic || Look.Pattern == EPattern::Galaxy ? 8 : Look.Pattern == EPattern::Fire ? 7 : 3)
			: I == 1 && (bOrganic || Look.Pattern == EPattern::Galaxy || Look.Pattern == EPattern::Vortex) ? 6 : 0);
	}
	UpdateKnockoutEffect();
}

void AFlickWorldFeedback::UpdateKnockoutEffect()
{
	if (!KnockoutGeometry || GetNetMode() == NM_DedicatedServer) return;
	if (Age >= Duration) { KnockoutGeometry->SetVisibility(false); KnockoutGeometry->ClearAllMeshSections(); return; }
	const auto& Look = FlickKnockoutStyle::Get(Style);
	const EPattern Pattern = Look.Pattern;
	const float Alpha = FMath::Clamp(Age/Duration,0.f,1.f);
	const float Fade = 1-FMath::SmoothStep(.35f,1.f,Alpha);
	const float Envelope = FMath::SmoothStep(0.f,.045f,Age)*Fade;
	const float Burst = 1-FMath::Exp(-Age*5.5f);
	const float Strength = FMath::Lerp(.7f,1.f,FMath::Clamp(Appearance.Strength,0.f,1.f));
	const float R = Look.Radius, H = Look.Height;
	const bool bSpectrum = Pattern == EPattern::Prism || Pattern == EPattern::Confetti;
	const auto Color = [&](float T, float Opacity)
	{
		const FLinearColor C = bSpectrum ? FlickKnockoutStyle::Spectrum(T)
			: FMath::Lerp(Look.Primary,Look.Secondary,FMath::Clamp(T,0.f,1.f));
		return C.CopyWithNewOpacity(FMath::Clamp(Opacity*Envelope*Strength,0.f,1.f));
	};
	FVector Right = FVector::RightVector, Up = FVector::UpVector;
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
		if (PC->PlayerCameraManager)
		{
			const FRotationMatrix View(PC->PlayerCameraManager->GetCameraRotation());
			Right = GetActorTransform().InverseTransformVectorNoScale(View.GetUnitAxis(EAxis::Y));
			Up = GetActorTransform().InverseTransformVectorNoScale(View.GetUnitAxis(EAxis::Z));
		}
	for (UMaterialInstanceDynamic* Material : KnockoutMaterials)
		Material->SetScalarParameterValue(TEXT("EffectAge"),Age);
	FKnockoutBatch Rings, Ribbons, Motes, Details;
	// Staged travelling waves, not a persistent spawn seal or an opaque sphere.
	for (int32 I = 0; I < 3; ++I)
	{
		const float Local = FMath::Max(0.f,Age-I*.085f);
		const float Travel = 1-FMath::Exp(-Local*5);
		float Radius = 10+R*Travel*(.75f+.12f*I);
		if (Pattern == EPattern::Vortex)
		{
			const float Collapse = FMath::Lerp(R,8.f,FMath::SmoothStep(0.f,.45f,Age));
			const float Release = 8+R*(1-FMath::Exp(-FMath::Max(0.f,Age-.45f)*5));
			Radius = FMath::Lerp(Collapse,Release,FMath::SmoothStep(.45f,.65f,Age));
		}
		const bool bPressure = Pattern == EPattern::Shock || Pattern == EPattern::Arcane;
		const float Opacity = FMath::Exp(-Local*2.3f)*(Local > 0 ? (bPressure ? .65f : .4f) : 0);
		Rings.Ring(Radius,I == 0 ? (bPressure ? 5 : 3) : 1.4f,I*2,Age*(I%2 ? -1 : 1),Color(I/2.f,Opacity),
			Pattern == EPattern::Arcane ? PI*1.75f : 2*PI);
	}
	const bool bVortex = Pattern == EPattern::Vortex || Pattern == EPattern::Galaxy;
	const bool bOrganic = Pattern == EPattern::Toxic || Pattern == EPattern::Spirit || Pattern == EPattern::Smoke;
	if (bVortex || Pattern == EPattern::Arcane)
	{
		const FVector X = Right, Y = (Up*.8f+FVector::UpVector*.2f).GetSafeNormal();
		const FVector Centre(0,0,Pattern == EPattern::Galaxy ? H*.55f : 20);
		const float Collapse = Pattern == EPattern::Vortex ? 1-FMath::SmoothStep(.05f,.5f,Age)*.9f : Burst;
		if (Pattern == EPattern::Galaxy)
		{
			// A star core and soft nebula sit underneath the finer spiral arms.
			Motes.Quad(Centre,Right,Up,12*Burst,12*Burst,Color(.65f,.95f));
			for (int32 I = 0; I < 4; ++I)
			{
				const float A = I*PI/2+Age;
				Details.Quad(Centre+(X*FMath::Cos(A)+Y*FMath::Sin(A))*R*.3f*Burst,
					Right,Up,R*.6f*Burst,R*.45f*Burst,Color(I*.12f,.14f));
			}
		}
		for (int32 Arm = 0; Arm < 4; ++Arm)
		{
			const int32 Start = Ribbons.Vertices.Num();
			for (int32 I = 0; I <= 40; ++I)
			{
				const float T = I/40.f, A = T*7+Arm*PI/2-Age*2.8f;
				const FVector D = X*FMath::Cos(A)+Y*FMath::Sin(A);
				Ribbons.Pair(Centre+D*(8+T*R*Collapse),D,2+T*(Pattern == EPattern::Galaxy ? 5 : 9),
					Color(Pattern == EPattern::Galaxy ? T*.6f : T,.65f*FMath::Sin(T*PI)),T);
			}
			Ribbons.Join(Start,41);
		}
		if (Pattern == EPattern::Arcane)
			for (int32 I = 0; I < 12; ++I)
			{
				const float A = I*PI/6-Age, C = FMath::Cos(A), S = FMath::Sin(A);
				const FVector D(C,S,0), Side(-S,C,0), P = D*(R*.8f*Burst)+FVector(0,0,4);
				Rings.Quad(P,D,Side,5,1,Color(.5f,.8f));
				Rings.Quad(P+D*3,D,Side,1,5,Color(.8f,.8f));
			}
	}
	if (Pattern == EPattern::Lightning)
	{
		const float Frame = FMath::FloorToFloat(Age*14+.001f);
		const float Flicker = .65f+.35f*FMath::Square(FMath::Sin(Age*27));
		for (int32 Bolt = 0; Bolt < 8; ++Bolt)
		{
			const float A = Bolt*PI/4;
			const FVector D(FMath::Cos(A),FMath::Sin(A),0), Side(-D.Y,D.X,0);
			// Soft outer arc and sharp inner core share the same jagged path.
			for (int32 Core = 0; Core < 2; ++Core)
			{
				const int32 Start = Ribbons.Vertices.Num();
				for (int32 I = 0; I <= 16; ++I)
				{
					const float T = I/16.f, Jitter = (Seed(I+Bolt*17+Frame*31)*2-1)*19*FMath::Sin(T*PI);
					const FVector P = D*(T*R*Burst)+Side*Jitter+FVector(0,0,FMath::Sin(T*PI)*H*.6f);
					Ribbons.Pair(P,Right,Core ? .7f : 4,Color(Core ? .85f : .15f,Flicker*(Core ? .9f : .25f)),T);
				}
				Ribbons.Join(Start,17);
			}
			const FVector Branch = D*R*.5f*Burst+FVector(0,0,H*.6f);
			const int32 Start = Ribbons.Vertices.Num();
			for (int32 I = 0; I <= 6; ++I)
			{
				const float T = I/6.f;
				Ribbons.Pair(Branch+Side*T*35+D*T*25+Up*FMath::Sin(I*29+Frame)*4,
					Right,.7f,Color(.7f,Flicker*.7f),T);
			}
			Ribbons.Join(Start,7);
		}
	}
	if (Pattern == EPattern::Water || Pattern == EPattern::Fire || bOrganic)
	{
		for (int32 Strand = 0; Strand < 10; ++Strand)
		{
			const int32 Start = Ribbons.Vertices.Num();
			for (int32 I = 0; I <= 24; ++I)
			{
				const float T = I/24.f, A = Strand*2*PI/10 + (bOrganic ? T*4-Age*1.5f : .2f*FMath::Sin(T*9));
				const FVector D(FMath::Cos(A),FMath::Sin(A),0);
				const float Radius = bOrganic ? R*(.4f+.2f*FMath::Sin(T*9+Age*3)) : R*T*Burst;
				const float Z = bOrganic ? T*H*Burst : FMath::Sin(T*PI)*H*Burst*(.65f+.3f*Seed(Strand));
				const FVector Side(-D.Y,D.X,0);
				Ribbons.Pair(D*Radius+FVector(0,0,Z),Pattern == EPattern::Water ? Side : Right,
					(bOrganic ? 11 : Pattern == EPattern::Water ? 15 : 6)*(1-T*.85f),
					Color(T,FMath::Sin(T*PI)*(Pattern == EPattern::Water ? .5f : .65f)),T);
			}
			Ribbons.Join(Start,25);
		}
		if (bOrganic || Pattern == EPattern::Fire)
			for (int32 I = 0; I < 12; ++I)
			{
				const float S = Seed(I+5), A = I*2.39996f+Age*.7f;
				const float Z = H*(.1f+.5f*S)*Burst, W = (16+S*18)*(.4f+.6f*Burst);
				Details.Quad(FVector(FMath::Cos(A),FMath::Sin(A),0)*R*.4f*Burst+FVector(0,0,Z),
					Right,Up,W,Pattern == EPattern::Fire ? W*1.7f : W,
					Color(S,Pattern == EPattern::Smoke ? .45f : .65f));
			}
	}
	for (int32 I = 0; I < Look.Particles; ++I)
	{
		const float S = Seed(I+4), T = Seed(I+17), A = I*2.39996f;
		const FVector D(FMath::Cos(A),FMath::Sin(A),0), Side(-D.Y,D.X,0);
		const float Local = FMath::Max(0.f,Age-T*.12f), Travel = 1-FMath::Exp(-Local*3.7f);
		float Distance = R*(.35f+.65f*S)*Travel;
		float Z = H*(.3f+.7f*T)*FMath::Sin(FMath::Min(Local/Duration,1.f)*PI)-Local*15;
		if (Pattern == EPattern::Shock) Z *= .2f;
		if (bOrganic || Pattern == EPattern::Hearts) Z = Local*H*.7f;
		if (bVortex)
		{
			if (Pattern == EPattern::Vortex)
			{
				const float Collapse = R*(1-FMath::SmoothStep(0.f,.5f,Age)*.95f)*(.5f+.5f*S);
				Distance = FMath::Lerp(Collapse,Distance,FMath::SmoothStep(.48f,.72f,Age));
			}
			Z = 25+H*.5f*FMath::Sin(Age*2+T*PI);
		}
		FVector P = D*Distance+BiasDirection*Travel*22+FVector(0,0,Z);
		if (Pattern == EPattern::Pixel) P = FVector(FMath::GridSnap(P.X,8.f),FMath::GridSnap(P.Y,8.f),FMath::GridSnap(P.Z,8.f));
		const float Size = Pattern == EPattern::Hearts ? 5+S*6 : Pattern == EPattern::Pixel ? 2+S*5 : 1.2f+S*2.4f;
		float Opacity = .8f*(1-FMath::SmoothStep(.4f+.25f*T,1.f,Alpha));
		if (Pattern == EPattern::Pixel) Opacity *= .55f+.45f*FMath::Square(FMath::Sin(Age*13+I));
		const float Rotate = Age*(I%2 ? -3 : 3)+A;
		const FVector SpinRight = Right*FMath::Cos(Rotate)+Up*FMath::Sin(Rotate);
		const FVector SpinUp = Up*FMath::Cos(Rotate)-Right*FMath::Sin(Rotate);
		Motes.Quad(P,SpinRight,SpinUp,Size,Size,Color(I/float(Look.Particles),Opacity));
		if (Pattern == EPattern::Ice || Pattern == EPattern::Hologram || Pattern == EPattern::Prism)
			if (I < 16)
			{
				const FVector Axis = (D*.5f+Up*.5f+Right*FMath::Sin(Rotate)).GetSafeNormal();
				const FVector ShardSide = FVector::CrossProduct(Axis,Up).GetSafeNormal();
				const FLinearColor C = Pattern == EPattern::Hologram ? FlickKnockoutStyle::Spectrum(I/16.f)
					: Color(I/16.f,.8f);
				Details.Shard(P,Axis,ShardSide,(4+S*6)*Fade,(14+T*19)*Fade,
					C.CopyWithNewOpacity(Opacity*Envelope*.7f));
				// Fine edge glints make the rotating facets read as crystal, not flat confetti.
				const float Length = (14+T*19)*Fade;
				const int32 Start = Ribbons.Vertices.Num();
				Ribbons.Pair(P-Axis*Length*.6f,Right,.45f,C.CopyWithNewOpacity(0),0);
				Ribbons.Pair(P,Right,.55f,C.CopyWithNewOpacity(Opacity*Envelope),.4f);
				Ribbons.Pair(P+Axis*Length,Right,.4f,C.CopyWithNewOpacity(0),1);
				Ribbons.Join(Start,3);
			}
		if (Pattern == EPattern::Confetti)
			Details.Quad(P,SpinRight,SpinUp,2+S*2,4+T*4,Color(I/float(Look.Particles),Opacity));
		if (Pattern == EPattern::Sparks || Pattern == EPattern::Gold || Pattern == EPattern::Shock || Pattern == EPattern::Prism)
		{
			const FVector Tail = D*(9+T*24)*Fade+Up*(Z*.12f);
			const int32 Start = Ribbons.Vertices.Num();
			Ribbons.Pair(P-Tail,Right,.7f,Color(T,0),0);
			Ribbons.Pair(P-Tail*.65f,Right,.9f,Color(T,Opacity*.45f),.35f);
			Ribbons.Pair(P,Right,.6f,Color(T,Opacity),1); Ribbons.Join(Start,3);
		}
		if (Pattern == EPattern::Comic && I < 16)
		{
			const int32 Start = Details.Vertices.Num();
			const FVector Base = P*.25f;
			const FVector Tip = P + D*(15+S*20)*Burst;
			Details.Vertex(Base-Side*6,{.5f,.5f},Color(.7f,.85f));
			Details.Vertex(Base+Side*6,{.5f,.5f},Color(.7f,.85f));
			Details.Vertex(Tip,{.5f,.5f},Color(.7f,0));
			Details.Indices.Append({Start,Start+1,Start+2});
		}
	}
	Rings.Upload(*KnockoutGeometry,0); Ribbons.Upload(*KnockoutGeometry,1);
	Motes.Upload(*KnockoutGeometry,2); Details.Upload(*KnockoutGeometry,3);
	KnockoutGeometry->SetVisibility(true);
}
