#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickStarterSkinTest, "FLICK.Cosmetics.StarterSkins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickStarterSkinTest::RunTest(const FString& Parameters)
{
	const FString Key = FlickCosmeticCatalog::GetConfigKey(FlickCosmeticCatalog::PuckCategoryStart);
	int32 Previous = 0;
	const bool HadPrevious = GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), *Key, Previous, GGameUserSettingsIni);
	for (int32 Retired = 2; Retired <= 5; ++Retired)
	{
		GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), *Key, Retired, GGameUserSettingsIni);
		TestEqual(TEXT("Saved retired skins fall back to Classic Blue"), FlickCosmeticCatalog::LoadPuckSkins()[0], 0);
	}
	if (HadPrevious) GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), *Key, Previous, GGameUserSettingsIni);
	else GConfig->RemoveKey(TEXT("FLICK.ProfileCosmetics"), *Key, GGameUserSettingsIni);

	const auto Settings = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Settings);
	if (!TestNotNull(TEXT("Starter skin fixture"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AFlickPiece* Piece = World->SpawnActor<AFlickPiece>();
	Piece->InitializePiece(EFlickTeam::Player1, 1, 45.f, 20.f, EFlickPieceArchetype::Standard);
	Piece->EnableTestArenaVisuals();
	auto* Visual = Piece->GetPresentationMesh();
	UStaticMesh* Mesh = Visual->GetStaticMesh();
	auto* Physics = CastChecked<UStaticMeshComponent>(Piece->GetRootComponent());
	const auto* BodyMaterial = Physics->GetBodyInstance()->GetSimplePhysicalMaterial();
	const float Mass = Physics->GetMass();
	for (int32 Skin = 0; Skin < 2; ++Skin)
	{
		Piece->SetPuckSkin(Skin);
		Piece->SetHovered(true);
		Piece->Tick(.016f);
		TestEqual(TEXT("Starter skin is equipped"), Piece->GetPuckSkin(), Skin);
		TestTrue(TEXT("Starter skins share the original mesh"), Visual->GetStaticMesh() == Mesh);
		TestEqual(TEXT("Starter skin preserves mass"), Physics->GetMass(), Mass);
		TestTrue(TEXT("Starter skin preserves friction"), Physics->GetBodyInstance()->GetSimplePhysicalMaterial() == BodyMaterial);
		TestEqual(TEXT("Starter visuals add no collision"), Visual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}
	if (Mesh) AddInfo(FString::Printf(TEXT("Standard visual dimensions (cm): %s"), *(Mesh->GetBounds().BoxExtent * 2.f).ToString()));
	AFlickPlayerState* State = World->SpawnActor<AFlickPlayerState>();
	TArray<int32> Skins;
	Skins.Init(1, FlickPieceArchetypeRules::ArchetypeCount);
	TestTrue(TEXT("Classic Orange remains valid for all puck types"), State->SetPuckSkins(Skins));
	for (int32 Retired = 2; Retired <= 5; ++Retired)
	{
		Skins[0] = Retired;
		TestFalse(TEXT("Server rejects retired Standard skins"), State->SetPuckSkins(Skins));
		TestEqual(TEXT("Rejected updates preserve Classic Orange"), State->GetPuckSkin(EFlickPieceArchetype::Standard), 1);
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
