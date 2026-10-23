#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Arena/FlickTestArena.h"
#include "Player/FlickCameraPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ScopeExit.h"
#include "UI/FlickLockerStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLockerArtworkTest, "FLICK.UI.LockerArtwork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickLockerArtworkTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : {TEXT("Standard"), TEXT("Heavy"), TEXT("Striker"), TEXT("Grippy"),
		TEXT("Slider"), TEXT("Blocker"), TEXT("Compact"), TEXT("Bouncer"), TEXT("Toppler")})
		for (int32 Finish = 0; Finish < 6; ++Finish)
		{
			const FString Asset = FString::Printf(TEXT("T_Puck_%s_%d"), Name, Finish);
			const auto* Texture = LoadObject<UTexture2D>(nullptr, *(TEXT("/Game/UI/PuckThumbnails/Locker/") + Asset + TEXT(".") + Asset));
			if (!TestNotNull(Asset, Texture)) continue;
			TestTrue(TEXT("Locker art is sRGB"), Texture->SRGB);
			TestEqual(TEXT("Locker art uses compressed BC7"), Texture->CompressionSettings, static_cast<uint8>(TC_BC7));
			TestEqual(TEXT("Locker UI texture group"), Texture->LODGroup, static_cast<uint8>(TEXTUREGROUP_UI));
#if WITH_EDITORONLY_DATA
			TestEqual(TEXT("Locker texture budget"), Texture->MaxTextureSize, 1024);
			TestEqual(TEXT("Square source width"), Texture->Source.GetSizeX(), int64(768));
			TestEqual(TEXT("Square source height"), Texture->Source.GetSizeY(), int64(768));
#endif
		}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLockerIsolationTest, "FLICK.UI.LockerIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickLockerIsolationTest::RunTest(const FString& Parameters)
{
	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Locker isolation world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto* Arena = World->SpawnActor<AFlickTestArena>();
	if (!TestNotNull(TEXT("Actual arena"), Arena)) return false;
	Arena->InitializeTestArena(650, 50, 250, 1);
	TInlineComponentArray<UStaticMeshComponent*> Components(Arena);
	UStaticMeshComponent* Deck = nullptr;
	TArray<UStaticMeshComponent*> Colliders;
	TArray<FTransform> ColliderTransforms;
	for (auto* Component : Components)
	{
		if (Component->GetFName() == TEXT("WorkshopArenaMesh")) Deck = Component;
		if (Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{ Colliders.Add(Component); ColliderTransforms.Add(Component->GetComponentTransform()); }
	}
	if (!TestNotNull(TEXT("Imported visual deck"), Deck)) return false;
	UStaticMesh* OriginalMesh = Deck->GetStaticMesh().Get();
	TArray<UMaterialInterface*> Original;
	for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot) Original.Add(Deck->GetMaterial(Slot));
	Arena->SetMenuPresentationEnabled(true);
	TArray<UMaterialInterface*> Menu;
	for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot) Menu.Add(Deck->GetMaterial(Slot));
	Arena->SetLockerPresentationEnabled(true);
	int32 DeckFinishes = 0;
	for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot)
	{
		if (Deck->GetMaterial(Slot) == Menu[Slot]) continue;
		++DeckFinishes;
		float Roughness = 0;
		Deck->GetMaterial(Slot)->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Roughness")), Roughness);
		TestEqual(TEXT("Locker deck has its own controlled reflective finish"), Roughness, .24f);
	}
	TestTrue(TEXT("Locker finish actually replaces the broad deck materials"), DeckFinishes > 0);
	TestTrue(TEXT("The actual mesh is never replaced"), Deck->GetStaticMesh().Get() == OriginalMesh);
	for (int32 Index = 0; Index < Colliders.Num(); ++Index)
		TestTrue(TEXT("All collision component transforms are unchanged"), Colliders[Index]->GetComponentTransform().Equals(ColliderTransforms[Index]));
	Arena->SetLockerPresentationEnabled(false);
	for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot)
		TestTrue(TEXT("Leaving locker restores the exact active Home/menu material"), Deck->GetMaterial(Slot) == Menu[Slot]);
	Arena->SetMenuPresentationEnabled(false);
	for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot)
		TestTrue(TEXT("Gameplay materials are restored by identity, not approximate parameters"), Deck->GetMaterial(Slot) == Original[Slot]);
	Arena->SetMenuPresentationEnabled(true);
	Arena->SetLockerPresentationEnabled(true);
	Arena->SetMenuPresentationEnabled(false);
	for (int32 Slot = 0; Slot < Deck->GetNumMaterials(); ++Slot)
		TestTrue(TEXT("Direct locker-to-game exit also restores originals"), Deck->GetMaterial(Slot) == Original[Slot]);
	auto* Camera = World->SpawnActor<AFlickCameraPawn>();
	if (!TestNotNull(TEXT("Actual presentation camera"), Camera)) return false;
	auto* Lens = Camera->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Camera lens"), Lens)) return false;
	Camera->SetMenuOrbitEnabled(true);
	Camera->SetMenuPresentation(true);
	const float HomeAperture = Lens->PostProcessSettings.DepthOfFieldFstop;
	Camera->SetLockerPreviewMode(1);
	Camera->SetMenuPresentation(true);
	TestEqual(TEXT("Live puck is sharp across its full face"), Lens->PostProcessSettings.DepthOfFieldFstop, 2.4f);
	Camera->SetLockerPreviewMode(0);
	Camera->SetMenuPresentation(true);
	TestEqual(TEXT("Home aperture is restored"), Lens->PostProcessSettings.DepthOfFieldFstop, HomeAperture);
	Camera->SetMenuPresentation(false);
	TestFalse(TEXT("Gameplay does not retain locker depth-of-field overrides"), Lens->PostProcessSettings.bOverride_DepthOfFieldFocalDistance);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickLockerLayoutTest, "FLICK.UI.LockerLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickLockerLayoutTest::RunTest(const FString& Parameters)
{
	using namespace FlickLockerStyle;
	TestEqual(TEXT("Three cards and two gaps fit the contents rail exactly"), 3 * CardWidth + 2 * CardGap, ContentsWidth);
	TestTrue(TEXT("Both rows fit below the locker header"), 2 * CardHeight + CardGap < 520);
	TestTrue(TEXT("All fifteen categories fit without shrinking text"), 15 * CategoryHeight + 100 < 600);
	for (const float Aspect : {16.f / 9.f, 21.f / 9.f, 2.4f})
	{
		const float VerticalTangent = FMath::Tan(FMath::DegreesToRadians(PuckFieldOfView(Aspect) * .5f)) / Aspect;
		TestTrue(TEXT("Ultrawide keeps the same vertical product framing"), FMath::IsNearlyEqual(VerticalTangent,
			FMath::Tan(FMath::DegreesToRadians(PuckFieldOfView(16.f / 9.f) * .5f)) / (16.f / 9.f), .0001f));
		FVector Location;
		FRotator Rotation;
		PuckView(250, 1.f, Aspect, Location, Rotation);
		const FVector ToPuck = FVector(0, 0, 250 + PuckFocusHeight) - Location;
		const FVector Local = Rotation.UnrotateVector(ToPuck);
		const float ProjectedX = .5f + .5f * Local.Y / (Local.X * FMath::Tan(FMath::DegreesToRadians(PuckFieldOfView(Aspect) * .5f)));
		TestTrue(TEXT("Live puck projects to the centre of its UI column"), FMath::IsNearlyEqual(ProjectedX, PreviewCenterX(Aspect), .0001f));
		TestTrue(TEXT("Product is vertically centred, not clipped at an edge"), FMath::IsNearlyZero(Local.Z, .001));
	}
	return true;
}
#endif
