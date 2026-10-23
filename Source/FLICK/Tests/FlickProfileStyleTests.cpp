#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/FlickProfileStyle.h"
#include "UI/FlickArtworkGeometry.h"
#include "UI/FlickUITheme.h"
#include "Engine/Texture2D.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickProfilePortraitTest, "FLICK.UI.ProfileShowcasePortraits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickProfilePortraitTest::RunTest(const FString&)
{
	for (const TCHAR* Name : {TEXT("Standard"),TEXT("Heavy"),TEXT("Striker"),TEXT("Grippy"),TEXT("Slider"),
		TEXT("Blocker"),TEXT("Compact"),TEXT("Bouncer"),TEXT("Toppler")})
		for (int32 Skin=0;Skin<6;++Skin)
		{
			const FString Asset=FString::Printf(TEXT("T_Puck_%s_%d"),Name,Skin);
			auto* T=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/UI/PuckThumbnails/ProfileCutout/")+Asset+TEXT(".")+Asset));
			if (!TestNotNull(Asset,T)) continue;
			TestTrue(TEXT("Profile portrait colour space"),T->SRGB);
			TestEqual(TEXT("Profile portrait compression"),T->CompressionSettings,static_cast<uint8>(TC_BC7));
#if WITH_EDITORONLY_DATA
			// NullRHI has no GPU texture format to query. Inspect actual imported
			// pixels instead: a transparent stage and fully opaque puck centre.
			TArray64<uint8> Pixels;
			TestEqual(TEXT("Profile portrait alpha-capable source format"),T->Source.GetFormat(),TSF_BGRA8);
			if (TestTrue(TEXT("Profile portrait source pixels are readable"),T->Source.GetMipData(Pixels,0))
				&& Pixels.Num()==1024*768*4)
			{
				TestEqual(TEXT("Profile portrait background is transparent"),Pixels[3],uint8(0));
				TestEqual(TEXT("Actual puck remains fully opaque"),Pixels[(384*1024+512)*4+3],uint8(255));
			}
			else AddError(TEXT("Unexpected profile portrait pixel buffer size"));
			TestEqual(TEXT("Profile portrait texture budget"),T->MaxTextureSize,1024);
			TestEqual(TEXT("Profile portrait authored width"),T->Source.GetSizeX(),int64(1024));
			TestEqual(TEXT("Profile portrait authored height"),T->Source.GetSizeY(),int64(768));
#endif
		}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickProfileLayoutTest, "FLICK.UI.ProfileLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickProfileLayoutTest::RunTest(const FString& Parameters)
{
	using namespace FlickProfileStyle;
	TestTrue(TEXT("Overview retains safe margins in the shared design frame"), ContentWidth <= 1480 && ContentHeight <= 840);
	TestTrue(TEXT("Rank heading has room for icon and arrow without wrapping"), CardWidth - 2*CardPadding - 74 - 28 >= 330);
	TestTrue(TEXT("Identity retains a generous product illustration"), IdentityWidth - 44 >= 360);
	TestTrue(TEXT("Two card columns and identity fill the declared layout"), FMath::IsNearlyEqual(IdentityWidth + IdentityGap + CardWidth*2 + CardGap, ContentWidth));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickProfileChartTest, "FLICK.UI.ProfileActivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickProfileChartTest::RunTest(const FString& Parameters)
{
	using namespace FlickProfileStyle;
	TestTrue(TEXT("Empty profile never invents chart results"), NormalizePoints({}).IsEmpty());
	const auto Zero = NormalizePoints({0,0,0});
	TestEqual(TEXT("Zero points remain zero, no divide by zero"),Zero[0],0.f);
	const auto Chronological = NormalizePoints({30,10,20});
	TestEqual(TEXT("Oldest match appears at the left"),Chronological[0],2.f/3.f);
	TestEqual(TEXT("Newest match appears at the right"),Chronological[2],1.f);
	const auto Capped = NormalizePoints({1,2,3,4,5,6,7,8,100});
	TestEqual(TEXT("Only the latest eight matches fit the card"),Capped.Num(),8);
	TestEqual(TEXT("Older outlier cannot change the chart scale"),Capped[0],1.f);
	for (float V : NormalizePoints({-2,0,4})) TestTrue(TEXT("Corrupt negative points cannot escape graph bounds"), V >= 0 && V <= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickProfileImageBlendTest, "FLICK.UI.ProfileImageBlend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickProfileImageBlendTest::RunTest(const FString& Parameters)
{
	const FVector2D Size(340,192);
	const auto Outline = FlickUITheme::GetPanelOutline(Size,12);
	const FMargin Fade(112,76,8,8);
	const auto Polygons = FlickArtworkGeometry::FadePolygons(Outline,Size,Fade);
	double TotalArea = 0;
	for (const auto& Polygon : Polygons)
	{
		double Area = 0;
		for (int32 I=0; I<Polygon.Num(); ++I)
		{
			const auto A=Polygon[I], B=Polygon[(I+1)%Polygon.Num()];
			Area += A.X*B.Y-B.X*A.Y;
			TestTrue(TEXT("Faded images remain inside the cut-corner frame"),
				A.X>=0 && A.Y>=0 && A.X<=Size.X && A.Y<=Size.Y && A.X+A.Y>=12-1e-4
				&& Size.X-A.X+A.Y>=12-1e-4 && A.X+Size.Y-A.Y>=12-1e-4 && Size.X-A.X+Size.Y-A.Y>=12-1e-4);
		}
		TotalArea += FMath::Abs(Area)*.5;
	}
	TestTrue(TEXT("Image blending introduces neither holes nor overlapping polygons"),FMath::IsNearlyEqual(TotalArea,Size.X*Size.Y-2*12*12,1e-4));
	TestEqual(TEXT("Puck and podium stay fully opaque away from the edges"),FlickArtworkGeometry::Opacity(FVector2D(220,130),Size,Fade),1.f);
	TestEqual(TEXT("Top seam disappears into its panel"),FlickArtworkGeometry::Opacity(FVector2D(200,0),Size,Fade),0.f);
	TestEqual(TEXT("Left seam disappears into its panel"),FlickArtworkGeometry::Opacity(FVector2D(0,100),Size,Fade),0.f);
	TestEqual(TEXT("Existing illustrations without fade retain a single polygon"),FlickArtworkGeometry::FadePolygons(Outline,Size,FMargin(0)).Num(),1);
	return true;
}
#endif
