#pragma once
#if WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionDepthFade.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionEyeAdaptationInverse.h"

namespace FlickCosmeticShaderChecks
{
	inline void Verify(FAutomationTestBase& Test, UMaterial& Material)
	{
		const auto* E = Material.GetExpressionInputForProperty(MP_EmissiveColor);
		const auto* Inverse = E ? Cast<UMaterialExpressionEyeAdaptationInverse>(E->Expression) : nullptr;
		const auto* Glow = Inverse ? Cast<UMaterialExpressionMultiply>(Inverse->LightValueInput.Expression) : nullptr;
		const auto* Shaded = Glow ? Cast<UMaterialExpressionMultiply>(Glow->A.Expression) : nullptr;
		const auto* Palette = Shaded ? Cast<UMaterialExpressionPower>(Shaded->A.Expression) : nullptr;
		Test.TestTrue(TEXT("Shaped vertex RGB drives exposure-compensated HDR light"),
			Palette && Cast<UMaterialExpressionVertexColor>(Palette->Base.Expression));
		const auto* O = Material.GetExpressionInputForProperty(MP_Opacity);
		const auto* Depth = O ? Cast<UMaterialExpressionDepthFade>(O->Expression) : nullptr;
		const auto* Fade = Depth ? Cast<UMaterialExpressionMultiply>(Depth->InOpacity.Expression) : nullptr;
		Test.TestTrue(TEXT("Soft scene intersections preserve vertex lifetime alpha"), Fade && Fade->A.OutputIndex == 4);
		const auto* Coverage = Fade ? Cast<UMaterialExpressionComponentMask>(Fade->B.Expression) : nullptr;
		const auto* Mask = Coverage ? Cast<UMaterialExpressionCustom>(Coverage->Input.Expression) : nullptr;
		bool bConnected = Mask && Mask->Inputs.Num() == 3 && Coverage->A && !Coverage->R && !Coverage->G && !Coverage->B;
		if (Mask) for (const auto& Input : Mask->Inputs) bConnected &= Input.Input.Expression != nullptr;
		Test.TestTrue(TEXT("Coverage uses the connected analytic mask alpha"),bConnected);
		Test.TestTrue(TEXT("RGB shading field shares the same mask evaluation"), Shaded
			&& Cast<UMaterialExpressionComponentMask>(Shaded->B.Expression)
			&& Cast<UMaterialExpressionComponentMask>(Shaded->B.Expression)->Input.Expression == Mask);
	}
}
#endif
