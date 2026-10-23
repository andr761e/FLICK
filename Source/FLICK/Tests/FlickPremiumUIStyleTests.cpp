#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "UI/FlickLineupStyle.h"
#include "UI/FlickPlaylistStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPremiumUIStyleTest, "FLICK.UI.PremiumPolish",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPremiumUIStyleTest::RunTest(const FString&)
{
	TestTrue(TEXT("Selected preset remains opaque rather than exposing the background"), FlickLineupStyle::PresetSurface(true).A >= .98f);
	TestTrue(TEXT("Unselected preset remains readable"), FlickLineupStyle::PresetSurface(false).A >= .95f);
	for (const FVector2D Size : {FVector2D(16,16),FVector2D(26,26),FVector2D(42,42)})
	{
		const auto Arrow=FlickPlaylistStyle::ArrowOutline(Size);
		TestEqual(TEXT("One consistent native silhouette at every scale"),Arrow.Num(),9);
		for (const auto P : Arrow) TestTrue(TEXT("Arrow stays within its hit target"),P.X>=0 && P.Y>=0 && P.X<=Size.X && P.Y<=Size.Y);
		TestTrue(TEXT("Arrow head is symmetric"),FMath::IsNearlyEqual(Arrow[3].Y+Arrow[5].Y,Size.Y));
	}
	for (const TCHAR* Name : {TEXT("Standard"),TEXT("Heavy"),TEXT("Striker"),TEXT("Grippy"),TEXT("Slider"),
		TEXT("Blocker"),TEXT("Compact"),TEXT("Bouncer"),TEXT("Toppler")})
		for (int32 Skin=0; Skin<6; ++Skin)
		{
			const FString Asset=FString::Printf(TEXT("T_Puck_%s_%d"),Name,Skin);
			const auto* Texture=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/UI/PuckThumbnails/Hero/")+Asset+TEXT(".")+Asset));
			if (!TestNotNull(Asset,Texture)) continue;
			TestTrue(TEXT("Hero portraits use sRGB"),Texture->SRGB);
			TestEqual(TEXT("Hero portrait compression"),Texture->CompressionSettings,static_cast<uint8>(TC_BC7));
			TestEqual(TEXT("Hero portrait texture group"),Texture->LODGroup,static_cast<uint8>(TEXTUREGROUP_UI));
#if WITH_EDITORONLY_DATA
			TestEqual(TEXT("Bounded hero memory budget"),Texture->MaxTextureSize,1024);
			TestEqual(TEXT("Hero portrait authored width"),Texture->Source.GetSizeX(),int64(1024));
			TestEqual(TEXT("Hero portrait authored height"),Texture->Source.GetSizeY(),int64(768));
#endif
		}
	return true;
}
#endif
