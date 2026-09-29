#include "Arena/FlickTestArena.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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
