#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Texture2D.h"
#include "Misc/AutomationTest.h"
#include "UI/FlickMenuArtwork.h"
#include "UI/FlickPlaylistStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickMenuArtworkTest, "FLICK.UI.MenuArtwork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickMenuArtworkTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : {TEXT("PlayCasual"), TEXT("PlayCompetitive"), TEXT("PlayTraining"),
		TEXT("PlayPrivate"), TEXT("PracticeSwitch"), TEXT("PracticeBob")})
	{
		const FString Path = FString::Printf(TEXT("/Game/UI/MenuArtwork/T_Menu_%s.T_Menu_%s"), Name, Name);
		const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (!TestNotNull(FString::Printf(TEXT("Menu illustration packaged asset: %s"), Name), Texture)) continue;
		TestTrue(TEXT("Card illustrations use sRGB"), Texture->SRGB);
		TestEqual(TEXT("Compressed card art, not raw UI RGBA"), Texture->CompressionSettings, static_cast<uint8>(TC_BC7));
		TestEqual(TEXT("Shared UI texture group"), Texture->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
#if WITH_EDITORONLY_DATA
		TestEqual(TEXT("Bounded cooked card texture budget"), Texture->MaxTextureSize, 1024);
		TestEqual(TEXT("Original wide illustration is retained for future reimports"), Texture->Source.GetSizeX(), int64(1672));
		TestTrue(TEXT("Original wide illustration height (generator can round by one pixel)"),
			Texture->Source.GetSizeY() >= 940 && Texture->Source.GetSizeY() <= 941);
#endif
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPlaylistArtworkTest, "FLICK.UI.PlaylistArtwork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPlaylistArtworkTest::RunTest(const FString& Parameters)
{
	for (uint8 Index = 0; Index < static_cast<uint8>(EFlickMenuArtwork::Count); ++Index)
	{
		const auto Artwork = static_cast<EFlickMenuArtwork>(Index);
		const FString Name = FlickMenuArtwork::AssetName(Artwork);
		const FString Path = FString::Printf(TEXT("/Game/UI/MenuArtwork/T_Menu_%s.T_Menu_%s"), *Name, *Name);
		const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (!TestNotNull(Name, Texture)) continue;
		TestTrue(TEXT("Playlist art is sRGB"), Texture->SRGB);
		TestEqual(TEXT("Playlist art uses compressed BC7"), Texture->CompressionSettings, static_cast<uint8>(TC_BC7));
		TestEqual(TEXT("Playlist art uses UI texture group"), Texture->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
#if WITH_EDITORONLY_DATA
		const FVector2f Source = FlickMenuArtwork::AuthoredSize(Artwork);
		TestEqual(Name + TEXT(" authored width matches cover-crop UVs"), Texture->Source.GetSizeX(), int64(Source.X));
		TestTrue(Name + TEXT(" authored height matches cover-crop UVs"), FMath::Abs(Texture->Source.GetSizeY() - int64(Source.Y)) <= 1);
		TestEqual(TEXT("No unbounded runtime texture sizes"), Texture->MaxTextureSize,
			Artwork == EFlickMenuArtwork::Backdrop || Artwork == EFlickMenuArtwork::SettingsBackdrop
				|| Artwork == EFlickMenuArtwork::ProfileBackdrop || Artwork == EFlickMenuArtwork::TrainingPracticeWide ? 2048 : 1024);
#endif
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPlaylistGeometryTest, "FLICK.UI.PlaylistGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPlaylistGeometryTest::RunTest(const FString& Parameters)
{
	for (const FVector2D Size : {FVector2D(606, 272), FVector2D(606, 184), FVector2D(1248, 128), FVector2D(96, 48)})
	{
		const auto Outline = FlickPlaylistStyle::GetClosedOutline(Size, 10.5f);
		TestEqual(TEXT("Eight cut-corner vertices plus one closing point"), Outline.Num(), 9);
		TestTrue(TEXT("Selection outline closes without aliasing its array storage"), Outline[0].Equals(Outline.Last()));
		for (const FVector2D Point : Outline)
			TestTrue(TEXT("Every selected-frame vertex stays inside its card"), Point.X >= 0 && Point.Y >= 0 && Point.X <= Size.X && Point.Y <= Size.Y);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPuckThumbnailTest, "FLICK.UI.PuckThumbnails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPuckThumbnailTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : {TEXT("Standard"), TEXT("Heavy"), TEXT("Striker"), TEXT("Grippy"),
		TEXT("Slider"), TEXT("Blocker"), TEXT("Compact"), TEXT("Bouncer"), TEXT("Toppler")})
	{
		for (int32 Finish = 0; Finish < 6; ++Finish)
		{
			const FString Asset = FString::Printf(TEXT("T_Puck_%s_%d"), Name, Finish);
			const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *(TEXT("/Game/UI/PuckThumbnails/") + Asset + TEXT(".") + Asset));
			if (!TestNotNull(Asset, Texture)) continue;
			TestTrue(TEXT("Thumbnails use sRGB"), Texture->SRGB);
			TestEqual(TEXT("Compressed thumbnail budget"), Texture->CompressionSettings, static_cast<uint8>(TC_BC7));
			TestEqual(TEXT("UI texture group"), Texture->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
#if WITH_EDITORONLY_DATA
			TestEqual(TEXT("Maximum size is bounded"), Texture->MaxTextureSize, 512);
			TestEqual(TEXT("Retains original model render width"), Texture->Source.GetSizeX(), int64(512));
			TestEqual(TEXT("Retains original model render height"), Texture->Source.GetSizeY(), int64(384));
#endif
		}
	}
	return true;
}
#endif
