#include "Arena/FlickTestArena.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	UMaterialInstanceDynamic* ClonePresentationMaterial(UMaterialInterface* Source, UObject* Owner)
	{
		if (!Source) return nullptr;
		auto* Dynamic = Cast<UMaterialInstanceDynamic>(Source);
		auto* Result = UMaterialInstanceDynamic::Create(Dynamic ? Dynamic->Parent.Get() : Source, Owner);
		if (Dynamic) Result->CopyInterpParameters(Dynamic);
		return Result;
	}
}

// Visual-only dressing shared by every Knockout format. Imported art remains the
// source for every format; no source asset, switch, divider or collider moves.
void AFlickTestArena::CreateOneVsOnePresentationComponents(UStaticMesh* Box, UStaticMesh* Cylinder)
{
	const auto CreateInstances = [this](const TCHAR* Name, UStaticMesh* Mesh)
	{
		auto* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(GetRootComponent());
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
		Component->SetVisibility(false);
		return Component;
	};
	RimLensBodies = CreateInstances(TEXT("OneVsOneRimLensBodies"), Box);
	RimLensCaps = CreateInstances(TEXT("OneVsOneRimLensCaps"), Cylinder);
	RimHousingBodies = CreateInstances(TEXT("OneVsOneRimHousingBodies"), Box);
	RimHousingCaps = CreateInstances(TEXT("OneVsOneRimHousingCaps"), Cylinder);
	ConceptGuides = CreateInstances(TEXT("OneVsOneConceptGuides"), Box);
	ConceptSocketMetal = CreateInstances(TEXT("OneVsOneConceptSocketMetal"), Box);
	ConceptSocketDark = CreateInstances(TEXT("OneVsOneConceptSocketDark"), Box);
	ConceptWarmRim = CreateInstances(TEXT("OneVsOneConceptWarmRim"), Box);
}

void AFlickTestArena::UpdateOneVsOnePresentation()
{
	const bool bEnabled = bUsingWorkshopAssets;
	TArray<UStaticMeshComponent*> Components = {WorkshopArenaMesh, StadiumStructureMesh, StadiumLightsMesh};
	for (UStaticMeshComponent* Component : ZoneOuterMeshes) Components.Add(Component);
	for (UStaticMeshComponent* Component : DividerBaseMeshes) Components.Add(Component);
	if (bEnabled != bOneVsOnePresentationEnabled)
	{
		// Menu materials inherit the shared finish. Invalidate their cache when
		// switching between workshop art and the primitive fallback.
		const bool bWasMenu = bMenuMaterialsEnabled;
		SetMenuPresentationEnabled(false);
		MenuOriginalMaterials.Reset();
		MenuPresentationMaterials.Reset();
		if (bEnabled && OneVsOneMaterials.IsEmpty())
		{
			for (UStaticMeshComponent* Component : Components)
			{
				for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
				{
					// State-owned accent MIDs are shared between each switch and its
					// active socket. Never cache or replace them with cosmetic MIDs.
					if (Slot == Component->GetMaterialIndex(TEXT("09_Switch_Accent"))) continue;
					UMaterialInterface* Original = Component->GetMaterial(Slot);
					OneVsOneOriginalMaterials.Add(Original);
					const FString Name = FString::Printf(TEXT("OneVsOne_%s_%d_%s"), *Component->GetName(), Slot,
						Original ? *Original->GetName() : TEXT("Empty"));
					UMaterialInstanceDynamic* Material = Original ? UMaterialInstanceDynamic::Create(Original, this, FName(*Name)) : nullptr;
					OneVsOneMaterials.Add(Material);
					if (!Material) continue;
					const FString SourceName = Original->GetName();
					float Emission = 0.0f;
					Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), Emission);
					if (Component == WorkshopArenaMesh)
					{
						if (SourceName.Contains(TEXT("Arena_Surface")) || SourceName.Contains(TEXT("Inner_Field")) || SourceName.Contains(TEXT("Center_Inset")))
						{
							// Satin silver: let the existing softboxes and environment
							// shape the metal instead of flattening it with emissive fill.
							Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.60f, 0.65f, 0.69f));
							Material->SetScalarParameterValue(TEXT("Metallic"), OneVsOneDeckMetallic);
							Material->SetScalarParameterValue(TEXT("Roughness"), OneVsOneDeckRoughness);
							Material->SetScalarParameterValue(TEXT("Specular"), 0.65f);
							Material->SetScalarParameterValue(TEXT("Anisotropy"), 0.35f);
							Material->SetScalarParameterValue(TEXT("SurfaceLift"), 0.025f);
						}
						else if (SourceName.Contains(TEXT("Team_Cyan")) || SourceName.Contains(TEXT("Team_Orange")))
						{
							// The recessed lenses replace the flat two-color ribbon.
							Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.008f, 0.035f, 0.048f));
							Material->SetScalarParameterValue(TEXT("Emission"), 0.35f);
						}
					}
					else if (Component != StadiumStructureMesh && Component != StadiumLightsMesh)
					{
						// Dark gunmetal bezels frame the existing coloured lenses.
						// Preserve all authored render offsets and recessed geometry.
						if (SourceName.Contains(TEXT("Brushed_Titanium")) || SourceName.Contains(TEXT("Accent_Metal")))
						{
							Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.09f, 0.135f, 0.17f));
							Material->SetScalarParameterValue(TEXT("Metallic"), 0.72f);
							Material->SetScalarParameterValue(TEXT("Roughness"), 0.30f);
							Material->SetScalarParameterValue(TEXT("SurfaceLift"), 0.012f);
						}
						else if (SourceName.Contains(TEXT("Arena_Graphite")))
						{
							Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.045f, 0.073f, 0.095f));
							Material->SetScalarParameterValue(TEXT("SurfaceLift"), 0.015f);
						}
					}
					else if (Emission > 0.1f)
					{
						const bool bWarmFixture = SourceName.Contains(TEXT("Warm"));
						Material->SetScalarParameterValue(TEXT("Emission"), Emission * (bWarmFixture ? 0.55f : 2.0f));
						if (bWarmFixture)
							Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.56f, 0.25f));
					}
					else if (SourceName.Contains(TEXT("Porcelain")) || SourceName.Contains(TEXT("Concrete")))
					{
						Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.025f, 0.039f, 0.050f));
						Material->SetScalarParameterValue(TEXT("Metallic"), 0.38f);
						Material->SetScalarParameterValue(TEXT("Roughness"), 0.34f);
					}
				}
			}
		}
		int32 MaterialIndex = 0;
		for (UStaticMeshComponent* Component : Components)
		{
			for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
			{
				if (Slot == Component->GetMaterialIndex(TEXT("09_Switch_Accent"))) continue;
				if (OneVsOneOriginalMaterials.IsValidIndex(MaterialIndex))
					Component->SetMaterial(Slot, bEnabled && OneVsOneMaterials[MaterialIndex]
						? static_cast<UMaterialInterface*>(OneVsOneMaterials[MaterialIndex].Get()) : OneVsOneOriginalMaterials[MaterialIndex].Get());
				++MaterialIndex;
			}
		}
		bOneVsOnePresentationEnabled = bEnabled;
		if (bWasMenu) SetMenuPresentationEnabled(true);
	}
	for (auto* Component : {RimLensBodies.Get(), RimLensCaps.Get(), RimHousingBodies.Get(), RimHousingCaps.Get()})
	{
		Component->SetVisibility(bEnabled);
		Component->SetHiddenInGame(!bEnabled);
	}
	if (bEnabled) BuildOneVsOneRim();
	UpdateOneVsOneConcept(true);
}

void AFlickTestArena::UpdateOneVsOneConcept(const bool bRebuildGuides)
{
	if (GetNetMode() == NM_DedicatedServer || !bUsingWorkshopAssets) return;
	if (!bConceptApplied)
	{
		const bool bWasMenu = bMenuMaterialsEnabled;
		SetMenuPresentationEnabled(false);
		MenuOriginalMaterials.Reset();
		MenuPresentationMaterials.Reset();
		if (ConceptMaterials.IsEmpty())
		{
			TArray<UStaticMeshComponent*> Components{WorkshopArenaMesh};
			for (UStaticMeshComponent* Component : ZoneOuterMeshes) Components.Add(Component);
			for (UStaticMeshComponent* Component : DividerBaseMeshes) Components.Add(Component);
			for (UStaticMeshComponent* Component : DividerVisualMeshes) Components.Add(Component);
			for (auto* Component : Components)
			for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
			{
				// Switch/socket accent MIDs remain owned by mechanism state.
				if (Slot == Component->GetMaterialIndex(TEXT("09_Switch_Accent"))) continue;
				auto* Original = Component->GetMaterial(Slot);
				auto* Material = ClonePresentationMaterial(Original, this);
				if (!Material) continue;
				ConceptMaterials.Add(Material);
				Component->SetMaterial(Slot, Material);
				const FString Name = Original->GetName();
				if (Component == WorkshopArenaMesh)
				{
					float Emission = 0.f;
					Original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), Emission);
					if (Emission > .1f) Material->SetScalarParameterValue(TEXT("Emission"), Emission * 4.f);
					if (Name.Contains(TEXT("Arena_Surface")) || Name.Contains(TEXT("Inner_Field")) || Name.Contains(TEXT("Center_Inset")))
					{
						Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(.21f, .27f, .33f));
						Material->SetScalarParameterValue(TEXT("Metallic"), OneVsOneDeckMetallic);
						Material->SetScalarParameterValue(TEXT("Roughness"), OneVsOneMenuDeckRoughness);
						Material->SetScalarParameterValue(TEXT("SurfaceLift"), .025f);
					}
					else if (Name.Contains(TEXT("Dark_Marking")) || Name.Contains(TEXT("Floor_Lines")))
					{
						// Bury the authored dense art beneath the rendered deck only.
						// WPO never changes collision or the original source asset.
						Material->SetScalarParameterValue(TEXT("RenderLayerOffset"), -2.f);
					}
				}
				else if (ZoneOuterMeshes.Contains(Component)
					&& Slot == Component->GetMaterialIndex(TEXT("05_Deep_Recess")))
				{
					// The authored backing cylinder shares its outer radius with the
					// metal rim, causing coplanar side faces. Hide only that backing
					// below the deck; retain the housing transform and activation area.
					Material->SetScalarParameterValue(TEXT("RenderLayerOffset"), -3.f);
				}
				else if (DividerBaseMeshes.Contains(Component)
					&& (Name.Contains(TEXT("Accent_Metal")) || Name.Contains(TEXT("Deep_Recess"))))
				{
					// Replace only the flat frame/channel art; retain the live status rail.
					Material->SetScalarParameterValue(TEXT("RenderLayerOffset"), -2.f);
				}
				else if (Name.Contains(TEXT("Brushed_Titanium")) || Name.Contains(TEXT("Accent_Metal")))
				{
					Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(.48f, .54f, .58f));
					Material->SetScalarParameterValue(TEXT("Metallic"), .9f);
					Material->SetScalarParameterValue(TEXT("Roughness"), .23f);
					Material->SetScalarParameterValue(TEXT("SurfaceLift"), .015f);
				}
			}
		}
		bConceptApplied = true;
		ConceptGuides->SetVisibility(true);
		ConceptGuides->SetHiddenInGame(false);
		for (auto* Component : {ConceptSocketMetal.Get(), ConceptSocketDark.Get(), ConceptWarmRim.Get()})
		{
			Component->SetVisibility(true);
			Component->SetHiddenInGame(false);
		}
		BuildConceptGuides();
		BuildConceptSockets();
		if (bWasMenu) SetMenuPresentationEnabled(true);
	}
	else if (bRebuildGuides) { BuildConceptGuides(); BuildConceptSockets(); }
}

void AFlickTestArena::BuildConceptSockets()
{
	if (!ConceptSocketMetalMaterial)
	{
		ConceptSocketMetalMaterial = ClonePresentationMaterial(LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/TestArena/Arena/MI_04_Brushed_Titanium.MI_04_Brushed_Titanium")), this);
		ConceptSocketDarkMaterial = ClonePresentationMaterial(LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/TestArena/Arena/MI_05_Deep_Recess.MI_05_Deep_Recess")), this);
		ConceptWarmRimMaterial = ClonePresentationMaterial(LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/TestArena/Arena/MI_07_Team_Orange.MI_07_Team_Orange")), this);
		if (ConceptSocketMetalMaterial)
		{
			ConceptSocketMetalMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(.53f, .58f, .63f));
			ConceptSocketMetalMaterial->SetScalarParameterValue(TEXT("Metallic"), .92f);
			ConceptSocketMetalMaterial->SetScalarParameterValue(TEXT("Roughness"), .19f);
			// The existing arena's dark reflection environment otherwise makes
			// horizontal metal read black. Keep the same restrained ambient fill
			// convention as the authored metal, without changing global lighting.
			ConceptSocketMetalMaterial->SetScalarParameterValue(TEXT("SurfaceLift"), .16f);
			ConceptSocketMetalMaterial->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.f);
		}
		if (ConceptSocketDarkMaterial)
		{
			ConceptSocketDarkMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(.009f, .018f, .027f));
			ConceptSocketDarkMaterial->SetScalarParameterValue(TEXT("Metallic"), .6f);
			ConceptSocketDarkMaterial->SetScalarParameterValue(TEXT("Roughness"), .25f);
			ConceptSocketDarkMaterial->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.f);
		}
		if (ConceptWarmRimMaterial)
		{
			ConceptWarmRimMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.f, .55f, .18f));
			ConceptWarmRimMaterial->SetScalarParameterValue(TEXT("Emission"), 4.f);
			ConceptWarmRimMaterial->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.f);
		}
		ConceptSocketMetal->SetMaterial(0, ConceptSocketMetalMaterial);
		ConceptSocketDark->SetMaterial(0, ConceptSocketDarkMaterial);
		ConceptWarmRim->SetMaterial(0, ConceptWarmRimMaterial);
	}
	ConceptSocketMetal->ClearInstances();
	ConceptSocketDark->ClearInstances();
	ConceptWarmRim->ClearInstances();
	for (int32 Index = 0; Index < GetPossibleLocationCount(); ++Index)
	{
		const auto* Socket = DividerBaseMeshes[Index].Get();
		const float Length = Socket->GetRelativeScale3D().X * DividerLength;
		const float Angle = PossibleDividerAngles[Index];
		const FVector Tangent(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		const FVector Normal(-Tangent.Y, Tangent.X, 0.f);
		const FQuat Rotation = FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f).Quaternion();
		const FVector Center(PossibleDividerCenters[Index], SurfaceZ);
		// Narrow layered bezels suggest a recessed machined slot. No geometry
		// projects above 0.5 cm; all instances have collision permanently disabled.
		ConceptSocketDark->AddInstance(FTransform(Rotation, Center + FVector(0.f, 0.f, .10f), FVector(Length + 4.f, 44.f, .15f) / 100.f));
		for (const float Side : {-1.f, 1.f})
		{
			ConceptSocketMetal->AddInstance(FTransform(Rotation, Center + Normal * (17.5f * Side) + FVector(0.f, 0.f, .30f),
				FVector(Length - 3.f, 5.f, .35f) / 100.f));
			ConceptSocketMetal->AddInstance(FTransform(Rotation, Center + Tangent * ((Length * .5f - 3.f) * Side) + FVector(0.f, 0.f, .30f),
				FVector(5.f, 35.f, .35f) / 100.f));
		}
	}
	for (int32 Index = 0; Index < 192; ++Index)
	{
		const float Angle = UE_TWO_PI * Index / 192.f;
		ConceptWarmRim->AddInstance(FTransform(FRotator(0.f, FMath::RadiansToDegrees(Angle) + 90.f, 0.f),
			FVector(FMath::Cos(Angle) * ArenaRadius * .998f, FMath::Sin(Angle) * ArenaRadius * .998f,
				SurfaceZ - ArenaThickness + 4.f), FVector(UE_TWO_PI * ArenaRadius / 192.f + .1f, .3f, 1.2f) / 100.f));
	}
}

void AFlickTestArena::BuildConceptGuides()
{
	if (!ConceptGuideMaterial)
	{
		auto* Source = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TestArena/Arena/MI_08_Floor_Lines.MI_08_Floor_Lines"));
		ConceptGuideMaterial = ClonePresentationMaterial(Source, this);
		if (ConceptGuideMaterial)
		{
			ConceptGuideMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(.48f, .57f, .62f));
			ConceptGuideMaterial->SetScalarParameterValue(TEXT("Roughness"), .48f);
			ConceptGuideMaterial->SetScalarParameterValue(TEXT("Metallic"), .15f);
			ConceptGuideMaterial->SetScalarParameterValue(TEXT("Emission"), 0.f);
			ConceptGuideMaterial->SetScalarParameterValue(TEXT("SurfaceLift"), .035f);
			ConceptGuideMaterial->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.f);
		}
		ConceptGuides->SetMaterial(0, ConceptGuideMaterial);
	}
	ConceptGuides->ClearInstances();
	const float Scale = ArenaRadius / 650.f;
	const auto Line = [this, Scale](FVector2D Start, FVector2D End, float Width, float Height = .8f)
	{
		const FVector2D Delta = End - Start;
		ConceptGuides->AddInstance(FTransform(FRotator(0.f, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0.f),
			FVector((Start + End) * .5f, SurfaceZ + Height), FVector(Delta.Size(), Width * Scale, .02f) / 100.f));
	};
	// One restrained axis and concentric guides replace spokes/ticks/arrows.
	// Separate visual layers at axis/axis and axis/ring intersections. Coplanar
	// boxes otherwise fight for depth as the camera moves. These never collide.
	Line(FVector2D(-ArenaRadius * .94f, 0.f), FVector2D(ArenaRadius * .94f, 0.f), .55f, .9f);
	Line(FVector2D(0.f, -ArenaRadius * .94f), FVector2D(0.f, ArenaRadius * .94f), .55f, 1.f);
	for (const float Fraction : {.151f, .36f, .52f, .705f, .855f})
	for (int32 Segment = 0; Segment < 192; ++Segment)
	{
		if (Fraction > .2f && Segment % 48 < 3) continue; // Small quadrant breaks.
		const float A = UE_TWO_PI * Segment / 192.f;
		const float B = UE_TWO_PI * (Segment + 1) / 192.f;
		Line(FVector2D(FMath::Cos(A), FMath::Sin(A)) * ArenaRadius * Fraction,
			FVector2D(FMath::Cos(B), FMath::Sin(B)) * ArenaRadius * Fraction, Fraction < .2f ? .9f : .55f);
	}
}

void AFlickTestArena::BuildOneVsOneRim()
{
	if (!RimLensMaterial)
	{
		UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TestArena/Arena/MI_06_Team_Cyan.MI_06_Team_Cyan"));
		RimLensMaterial = Source ? UMaterialInstanceDynamic::Create(Source, this) : nullptr;
		Source = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TestArena/Arena/MI_04_Brushed_Titanium.MI_04_Brushed_Titanium"));
		RimHousingMaterial = Source ? UMaterialInstanceDynamic::Create(Source, this) : nullptr;
	}
	if (RimLensMaterial)
	{
		RimLensMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.005f, 0.70f, 1.0f));
		RimLensMaterial->SetScalarParameterValue(TEXT("Emission"), RimLensEmission);
		RimLensMaterial->SetScalarParameterValue(TEXT("SurfaceLift"), 0.0f);
		RimLensMaterial->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.0f);
	}
	if (RimHousingMaterial)
	{
		RimHousingMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.015f, 0.035f, 0.046f));
		RimHousingMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.20f);
		RimHousingMaterial->SetScalarParameterValue(TEXT("SurfaceLift"), 0.0f);
		RimHousingMaterial->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.0f);
	}
	RimLensBodies->SetMaterial(0, RimLensMaterial);
	RimLensCaps->SetMaterial(0, RimLensMaterial);
	RimHousingBodies->SetMaterial(0, RimHousingMaterial);
	RimHousingCaps->SetMaterial(0, RimHousingMaterial);
	for (auto* Component : {RimLensBodies.Get(), RimLensCaps.Get(), RimHousingBodies.Get(), RimHousingCaps.Get()}) Component->ClearInstances();
	const float Scale = ArenaRadius / 650.0f;
	const int32 Count = FMath::Clamp(RimLensCount, 12, 40);
	// The authored rim top is SurfaceZ. The playing-field material's negative
	// render offset applies only inside the rim; using it here buries the inserts
	// beneath the layered rim armor and graphite trim.
	const float RimZ = SurfaceZ;
	const float LensLength = 72.0f * Scale;
	const float LensWidth = 3.2f * Scale;
	const float BezelWidth = 5.2f * Scale;
	const float InsertThickness = 0.04f * Scale;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = 2.0f * PI * (Index + 0.5f) / Count;
		const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		const FVector Tangent(-Radial.Y, Radial.X, 0.0f);
		const FQuat Rotation = FRotationMatrix::MakeFromX(Tangent).ToQuat();
		const FVector Center = Radial * (ArenaRadius - 16.0f * Scale) + FVector(0.0f, 0.0f, RimZ);
		// Flat boxes with vertical-axis disk ends form rounded, recessed slots.
		// Bodies extend below the rim surface; just 0.15/0.30 mm of separation
		// keeps their planar top faces from z-fighting, with no raised lamp sides.
		const FVector BezelCenter = Center - FVector(0.0f, 0.0f, 0.005f * Scale);
		const FVector LensCenter = Center + FVector(0.0f, 0.0f, 0.01f * Scale);
		RimHousingBodies->AddInstance(FTransform(Rotation, BezelCenter, FVector(LensLength, BezelWidth, InsertThickness) / 100.0f));
		RimLensBodies->AddInstance(FTransform(Rotation, LensCenter, FVector(LensLength, LensWidth, InsertThickness) / 100.0f));
		for (const float Sign : {-1.0f, 1.0f})
		{
			const FVector EndOffset = Tangent * (LensLength * 0.5f * Sign);
			RimHousingCaps->AddInstance(FTransform(FQuat::Identity, BezelCenter + EndOffset,
				FVector(BezelWidth, BezelWidth, InsertThickness) / 100.0f));
			RimLensCaps->AddInstance(FTransform(FQuat::Identity, LensCenter + EndOffset,
				FVector(LensWidth, LensWidth, InsertThickness) / 100.0f));
		}
	}
}
