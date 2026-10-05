#pragma once

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace FlickCosmeticMaterial
{
	inline UMaterialInstanceDynamic* CreateGlow(UObject* Outer)
	{
		// Reuse the game's existing HDR rim-light shader, already always cooked.
		// The engine debug material's texture modulation washes out fine filaments.
		UMaterialInterface* Source = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/TestArena/Arena/MI_06_Team_Cyan.MI_06_Team_Cyan"));
		if (!Source) Source = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
		UMaterialInstanceDynamic* Material = Source ? UMaterialInstanceDynamic::Create(Source, Outer) : nullptr;
		if (Material)
		{
			Material->SetScalarParameterValue(TEXT("Emission"), 2.5f);
			Material->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
			Material->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
			Material->SetScalarParameterValue(TEXT("Anisotropy"), 0.0f);
			Material->SetScalarParameterValue(TEXT("SurfaceLift"), 0.0f);
			Material->SetScalarParameterValue(TEXT("RenderLayerOffset"), 0.0f);
		}
		return Material;
	}
	inline void SetGlow(UMaterialInstanceDynamic* Material, const FLinearColor& Color)
	{
		if (!Material) return;
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
		Material->SetVectorParameterValue(TEXT("Color"), Color); // Engine fallback.
	}
}
