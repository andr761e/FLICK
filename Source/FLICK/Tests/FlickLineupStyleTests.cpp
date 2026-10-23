#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "UI/FlickLineupStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLineupPresentationTest, "FLICK.UI.LineupPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickLineupPresentationTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : {TEXT("Standard"), TEXT("Heavy"), TEXT("Striker"), TEXT("Grippy"),
		TEXT("Slider"), TEXT("Blocker"), TEXT("Compact"), TEXT("Bouncer"), TEXT("Toppler")})
	{
		for (int32 Finish = 0; Finish < 6; ++Finish)
		{
			const FString Asset = FString::Printf(TEXT("T_Puck_%s_%d"), Name, Finish);
			const auto* Texture = LoadObject<UTexture2D>(nullptr, *(TEXT("/Game/UI/PuckThumbnails/Presentation/") + Asset + TEXT(".") + Asset));
			if (!TestNotNull(Asset, Texture)) continue;
			TestTrue(TEXT("Presentation art is sRGB"), Texture->SRGB);
			TestEqual(TEXT("Compressed, bounded presentation budget"), Texture->CompressionSettings, static_cast<uint8>(TC_BC7));
			TestEqual(TEXT("Presentation texture group"), Texture->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
#if WITH_EDITORONLY_DATA
			TestEqual(TEXT("Maximum texture size"), Texture->MaxTextureSize, 1024);
			TestEqual(TEXT("Product-stage authored width"), Texture->Source.GetSizeX(), int64(1024));
			TestEqual(TEXT("Product-stage authored height"), Texture->Source.GetSizeY(), int64(768));
#endif
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLineupGeometryTest, "FLICK.UI.LineupGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickLineupGeometryTest::RunTest(const FString& Parameters)
{
	// The global 900-unit design grid is shared across supported display sizes.
	const FVector2D FormationArea(465, 280);
	for (const bool bThreePucks : {false, true})
	{
		const int32 Count = bThreePucks ? 3 : 4;
		for (int32 Slot = 0; Slot < Count; ++Slot)
		{
			const FVector2D Center = FlickLineupStyle::SlotPosition(Slot, bThreePucks) * FormationArea;
			const float Half = FlickLineupStyle::SlotSize * .5f;
			TestTrue(TEXT("Selectable slot stays within its formation"), Center.X >= Half && Center.Y >= Half
				&& Center.X <= FormationArea.X - Half && Center.Y <= FormationArea.Y - Half);
			for (int32 Other = Slot + 1; Other < Count; ++Other)
			{
				const FVector2D Separation = (FlickLineupStyle::SlotPosition(Other, bThreePucks) * FormationArea - Center).GetAbs();
				TestTrue(TEXT("Slot hit targets do not overlap"), Separation.X >= FlickLineupStyle::SlotSize || Separation.Y >= FlickLineupStyle::SlotSize);
			}
		}
	}
	return true;
}
#endif
