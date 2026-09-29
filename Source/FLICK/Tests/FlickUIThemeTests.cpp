#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/FlickUITheme.h"
#include "UI/FlickMainMenuStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickUIThemeTest, "FLICK.UI.SharedPresentationTheme",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickUIThemeTest::RunTest(const FString& Parameters)
{
	using namespace FlickUITheme;
	TestEqual(TEXT("Home and other screens use the same action lime"), FlickMainMenuStyle::Lime, Brand);
	TestEqual(TEXT("Home and other screens use the same highlight cyan"), FlickMainMenuStyle::Ice, Ice);
	for (const FVector2D Size : {FVector2D(26,26), FVector2D(160,44), FVector2D(520,350), FVector2D(1320,820)})
	{
		const TArray<FVector2D> Points = GetPanelOutline(Size, 14.0f);
		TestEqual(TEXT("Controls and page containers share eight cut corners"), Points.Num(), 8);
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2D Point = Points[Index];
			TestTrue(TEXT("Chamfers stay inside the existing widget bounds"),
				Point.X >= 0 && Point.Y >= 0 && Point.X <= Size.X && Point.Y <= Size.Y);
			TestTrue(TEXT("Panel remains convex for Slate triangle-fan rendering"),
				FVector2D::CrossProduct(Points[(Index + 1) % Points.Num()] - Point,
					Points[(Index + 2) % Points.Num()] - Points[(Index + 1) % Points.Num()]) > 0);
		}
	}
	for (const FLinearColor Color : {Brand, Cyan, Orange, FLinearColor(0.026f, 0.009f, 0.008f, 0.72f),
		FLinearColor(0.015f, 0.002f, 0.025f, 0.85f)})
	{
		TestEqual(TEXT("Shared sheen does not recolour CTA, team, danger or cosmetic colours"), GetSurfaceHighlight(Color), Color);
	}
	TestTrue(TEXT("Raised and base panels retain distinct hierarchy"), PanelRaised.GetLuminance() > Panel.GetLuminance());
	TestTrue(TEXT("Graphite panels receive a subtle directional highlight"), GetSurfaceHighlight(Panel).GetLuminance() > Panel.GetLuminance());
	TestEqual(TEXT("Sheen preserves translucent panel opacity"), GetSurfaceHighlight(Panel).A, Panel.A);
	const auto Contrast = [](const FLinearColor A, const FLinearColor B)
	{
		return (FMath::Max(A.GetLuminance(), B.GetLuminance()) + 0.05f)
			/ (FMath::Min(A.GetLuminance(), B.GetLuminance()) + 0.05f);
	};
	TestTrue(TEXT("Body text remains readable against raised graphite panels"), Contrast(Paper, PanelRaised) >= 4.5f);
	TestTrue(TEXT("Secondary text remains readable against raised graphite panels"), Contrast(Muted, PanelRaised) >= 4.5f);
	TestTrue(TEXT("Primary actions retain dark-on-lime contrast"), Contrast(Ink, Brand) >= 7.0f);
	return true;
}
#endif
