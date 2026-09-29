#if WITH_DEV_AUTOMATION_TESTS
#include "Core/FlickPresentationFrame.h"
#include "Misc/AutomationTest.h"
#include "UI/FlickMainMenuStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickMainMenuStyleTest, "FLICK.UI.MainMenuPresentationLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMainMenuStyleTest::RunTest(const FString& Parameters)
{
	using namespace FlickMainMenuStyle;
	for (const FVector2D Viewport : {FVector2D(1280, 720), FVector2D(1920, 1080), FVector2D(3440, 1440),
		FVector2D(1280, 960), FVector2D(1080, 1920), FVector2D(5120, 1440)})
	{
		const FVector2D Frame = FlickPresentationFrame::GetContainedSize(Viewport);
		const float CanvasScale = FMath::Min(Frame.X / 1600.0, Frame.Y / 900.0);
		const FVector2D Canvas = Frame / CanvasScale;
		const float ColumnScale = FMath::Clamp((16.0 / 9.0) / (Frame.X / Frame.Y), 0.78, 1.0);
		const float Top = GetDiagonalTopEdgeX(Canvas, ColumnScale);
		const float Bottom = GetDiagonalBottomEdgeX(Canvas, ColumnScale);
		TestTrue(TEXT("Diagonal slopes forward and stays within the contained frame"), Top > 0.0f && Top < Bottom && Bottom < Canvas.X);
		float PreviousBottom = NavigationTop;
		for (int32 Row = 0; Row < 6; ++Row)
		{
			const FVector2D Size(Navigation::GetRowWidth(Row), Navigation::GetRowHeight(Row));
			const TArray<FVector2D> Points = GetCardOutline(Size);
			TestEqual(TEXT("Navigation retains eight clean cut corners"), Points.Num(), 8);
			const float RowTop = NavigationTop + Navigation::GetRowTop(Row) * ColumnScale;
			TestTrue(TEXT("Cards retain their spacing at all supported display ratios"), RowTop >= PreviousBottom);
			PreviousBottom = RowTop + Size.Y * ColumnScale;
			for (int32 Index = 0; Index < Points.Num(); ++Index)
			{
				const FVector2D Point(LeftPadding + Points[Index].X * ColumnScale, RowTop + Points[Index].Y * ColumnScale);
				const float Edge = FMath::Lerp(Top, Bottom, static_cast<float>(Point.Y / Canvas.Y));
				TestTrue(TEXT("All navigation corners remain inside the diagonal overlay with room for edge glow"), Point.X + 8.0f <= Edge);
				TestTrue(TEXT("Cut-corner polygon is convex for the Slate triangle fan"),
					FVector2D::CrossProduct(Points[(Index + 1) % Points.Num()] - Points[Index],
						Points[(Index + 2) % Points.Num()] - Points[(Index + 1) % Points.Num()]) > 0.0);
			}
		}
		// The existing bottom-anchored profile and dynamically centered puck
		// selector have enough room, including on 720p and compressed ultrawide.
		const float ProfileTop = Canvas.Y - 48.0f - 82.0f * ColumnScale;
		TestTrue(TEXT("QUIT leaves enough room for the display puck selector above the profile"),
			ProfileTop - PreviousBottom >= 32.0f * ColumnScale + 12.0f);
	}
	TestTrue(TEXT("PLAY is the dominant action by height"), Navigation::PrimaryHeight > Navigation::SecondaryHeight);
	TestEqual(TEXT("Navigation container accommodates the widest card without squeezing the swept edges"),
		Navigation::StackWidth, Navigation::GetRowWidth(5));
	TestTrue(TEXT("Card widths step outward to follow the panel sweep"), Navigation::GetRowWidth(5) > Navigation::GetRowWidth(1));
	return true;
}
#endif
