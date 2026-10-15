#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickPuckPalette.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/CameraComponent.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickStarterSkinTest, "FLICK.Cosmetics.StarterSkins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickPaletteDefinitionTest, "FLICK.Cosmetics.PaletteDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPaletteDefinitionTest::RunTest(const FString& Parameters)
{
	const TCHAR* Names[] = {TEXT("Emerald"), TEXT("Amethyst"), TEXT("Crimson"), TEXT("Amber")};
	const TCHAR* Slots[] = {TEXT("Cyan light diffuser"), TEXT("Cyan center emblem"),
		TEXT("Graphite anodized housing"), TEXT("Recessed dark titanium"),
		TEXT("Circular brushed silver"), TEXT("Machined edge highlights")};
	const TCHAR* Roles[] = {TEXT("Diffuser"), TEXT("Emblem"), TEXT("Housing"), TEXT("Core"), TEXT("Crown"), TEXT("Edges")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		FString Source;
		const FString Path = FPaths::ProjectDir() / TEXT("AssetDevelopment/Pucks") / Names[Index] / TEXT("set.json");
		if (!TestTrue(TEXT("Palette source exists"), FFileHelper::LoadFileToString(Source, *Path))) continue;
		TSharedPtr<FJsonObject> Definition;
		if (!TestTrue(TEXT("Palette source is valid JSON"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source), Definition)
			&& Definition.IsValid())) continue;
		const auto ReadColor = [](const TArray<TSharedPtr<FJsonValue>>& Values)
		{
			return FLinearColor(Values[0]->AsNumber(), Values[1]->AsNumber(), Values[2]->AsNumber());
		};
		const FLinearColor Accent = ReadColor(Definition->GetArrayField(TEXT("accent_color")));
		TestTrue(TEXT("Runtime light and UI hue match the authored palette"),
			FlickCosmeticCatalog::GetPuckSkinColor(Index + 2).Equals(Accent, .00001f));
		const auto& BaseColors = Definition->GetObjectField(TEXT("base_colors"));
		for (int32 Role = 0; Role < UE_ARRAY_COUNT(Roles); ++Role)
		{
			auto* Material = LoadObject<UMaterialInstanceConstant>(nullptr, *FlickPuckPalette::MaterialPath(Index + 2, Slots[Role]));
			if (!TestNotNull(TEXT("Authored palette role has a cooked material"), Material)) continue;
			FLinearColor Actual;
			Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), Actual);
			TestTrue(TEXT("Imported base colour matches its source"), Actual.Equals(ReadColor(BaseColors->GetArrayField(Roles[Role])), .00001f));
			if (Role < 2)
			{
				Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("TeamColor")), Actual);
				TestTrue(TEXT("Imported LED hue matches runtime"), Actual.Equals(Accent, .00001f));
			}
		}
	}
	return true;
}

bool FFlickStarterSkinTest::RunTest(const FString& Parameters)
{
	const FString Key = FlickCosmeticCatalog::GetConfigKey(FlickCosmeticCatalog::PuckCategoryStart);
	int32 Previous = 0;
	const bool HadPrevious = GConfig->GetInt(TEXT("FLICK.ProfileCosmetics"), *Key, Previous, GGameUserSettingsIni);
	for (int32 Retired = 6; Retired <= 9; ++Retired)
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
	auto* CameraPawn = World->SpawnActor<AFlickCameraPawn>();
	auto* Camera = CameraPawn->FindComponentByClass<UCameraComponent>();
	auto* Accent = Piece->FindComponentByClass<UPointLightComponent>();
	CameraPawn->SetTestArenaPresentation(true);
	TestEqual(TEXT("Original camera film grain"), Camera->PostProcessSettings.FilmGrainIntensity, .07f);
	TestEqual(TEXT("Original bloom kernel"), Camera->PostProcessSettings.Bloom1Size, .20f);
	const float BloomBeforeSkins = Camera->PostProcessSettings.BloomIntensity;
	TestEqual(TEXT("Original LED light intensity"), Accent->Intensity, 8.f);
	TestEqual(TEXT("Original LED light radius"), Accent->AttenuationRadius, 64.f);
	for (int32 Skin = 0; Skin < 6; ++Skin)
	{
		Piece->SetPuckSkin(Skin);
		Piece->SetHovered(true);
		Piece->Tick(.016f);
		TestEqual(TEXT("Starter skin is equipped"), Piece->GetPuckSkin(), Skin);
		TestEqual(TEXT("Skin switching preserves existing bloom"), Camera->PostProcessSettings.BloomIntensity, BloomBeforeSkins);
		TestTrue(TEXT("Starter skins share the original mesh"), Visual->GetStaticMesh() == Mesh);
		TestEqual(TEXT("Starter skin preserves mass"), Physics->GetMass(), Mass);
		TestTrue(TEXT("Starter skin preserves friction"), Physics->GetBodyInstance()->GetSimplePhysicalMaterial() == BodyMaterial);
		TestEqual(TEXT("Starter visuals add no collision"), Visual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		if (Skin >= 2 && Mesh)
		{
			for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
			{
				const FString Path = FlickPuckPalette::MaterialPath(Skin, Mesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString());
				if (Path.IsEmpty()) continue;
				auto* Expected = LoadObject<UMaterialInstanceConstant>(nullptr, *Path);
				TestNotNull(TEXT("Palette material exists"), Expected);
				UMaterialInterface* Actual = Visual->GetMaterial(Slot);
				auto* Dynamic = Cast<UMaterialInstanceDynamic>(Actual);
				TestTrue(TEXT("Hover preserves the selected palette"), Expected && (Dynamic ? Dynamic->Parent == Expected : Actual == Expected));
				if (!Expected) continue;
				UMaterialInterface* Original = Mesh->GetStaticMaterials()[Slot].MaterialInterface;
				TestTrue(TEXT("Palette inherits the exact original material"), Expected->Parent == Original);
				for (const TCHAR* Parameter : {TEXT("Metallic"), TEXT("Roughness"), TEXT("Anisotropy"), TEXT("SurfaceLift")})
				{
					float Current = 0.f, Baseline = 0.f;
					Actual->GetScalarParameterValue(FMaterialParameterInfo(Parameter), Current);
					Original->GetScalarParameterValue(FMaterialParameterInfo(Parameter), Baseline);
					TestEqual(TEXT("Palette keeps the original finish parameters"), Current, Baseline);
				}
				FLinearColor CurrentBase, ExpectedBase;
				Actual->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), CurrentBase);
				Expected->GetVectorParameterValue(FMaterialParameterInfo(TEXT("BaseColor")), ExpectedBase);
				TestTrue(TEXT("Hover does not reset the authored palette to Blue"), CurrentBase.Equals(ExpectedBase));
			}
		}
	}
	Piece->SetPuckSkin(0);
	if (Mesh)
	{
		for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
		{
			UMaterialInterface* Actual = Visual->GetMaterial(Slot);
			auto* Dynamic = Cast<UMaterialInstanceDynamic>(Actual);
			UMaterialInterface* Original = Mesh->GetStaticMaterials()[Slot].MaterialInterface;
			TestTrue(TEXT("Returning to Blue clears all palette overrides"), Dynamic ? Dynamic->Parent == Original : Actual == Original);
		}
		AddInfo(FString::Printf(TEXT("Standard visual dimensions (cm): %s"), *(Mesh->GetBounds().BoxExtent * 2.f).ToString()));
	}

	AFlickPlayerState* State = World->SpawnActor<AFlickPlayerState>();
	for (int32 Type = 1; Type < FlickPieceArchetypeRules::ArchetypeCount; ++Type)
	{
		auto* Variant = World->SpawnActor<AFlickPiece>();
		Variant->InitializePiece(EFlickTeam::Player2, Type + 10, 45.f, 20.f, static_cast<EFlickPieceArchetype>(Type));
		Variant->EnableTestArenaVisuals();
		auto* VariantVisual = Variant->GetPresentationMesh();
		UStaticMesh* VariantMesh = VariantVisual->GetStaticMesh();
		auto* VariantBody = CastChecked<UStaticMeshComponent>(Variant->GetRootComponent());
		const float VariantMass = VariantBody->GetMass();
		const auto* VariantPhysics = VariantBody->GetBodyInstance()->GetSimplePhysicalMaterial();
		for (int32 Palette = 2; Palette <= 5; ++Palette)
		{
			Variant->SetPuckSkin(Palette);
			Variant->SetHovered(true);
			Variant->Tick(.016f);
			TestEqual(TEXT("Every archetype equips each palette"), Variant->GetPuckSkin(), Palette);
			TestTrue(TEXT("Palettes reuse the optimized archetype mesh"), VariantVisual->GetStaticMesh() == VariantMesh);
			TestEqual(TEXT("Palette preserves archetype mass"), VariantBody->GetMass(), VariantMass);
			TestTrue(TEXT("Palette preserves physical material"), VariantBody->GetBodyInstance()->GetSimplePhysicalMaterial() == VariantPhysics);
			TestEqual(TEXT("Palette visual remains non-colliding"), VariantVisual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			if (!VariantMesh) continue;
			int32 PaletteRoleCount = 0;
			for (int32 Slot = 0; Slot < VariantMesh->GetStaticMaterials().Num(); ++Slot)
			{
				const FString Path = FlickPuckPalette::MaterialPath(Palette, VariantMesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString());
				if (Path.IsEmpty()) continue;
				++PaletteRoleCount;
				auto* Expected = LoadObject<UMaterialInstanceConstant>(nullptr, *Path);
				auto* Actual = VariantVisual->GetMaterial(Slot);
				auto* Dynamic = Cast<UMaterialInstanceDynamic>(Actual);
				TestTrue(TEXT("Every archetype uses the shared palette even while hovered"),
					Expected && (Dynamic ? Dynamic->Parent == Expected : Actual == Expected));
			}
			TestEqual(TEXT("Every archetype recolours all six palette regions"), PaletteRoleCount, 6);
		}
		Variant->SetPuckSkin(0);
		if (VariantMesh)
		for (int32 Slot = 0; Slot < VariantMesh->GetStaticMaterials().Num(); ++Slot)
		{
			auto* Actual = VariantVisual->GetMaterial(Slot);
			auto* Dynamic = Cast<UMaterialInstanceDynamic>(Actual);
			auto* Original = VariantMesh->GetStaticMaterials()[Slot].MaterialInterface.Get();
			TestTrue(TEXT("Every archetype clears palette overrides when switching back"), Dynamic ? Dynamic->Parent == Original : Actual == Original);
		}
	}
	TArray<int32> Skins;
	Skins.Init(1, FlickPieceArchetypeRules::ArchetypeCount);
	TestTrue(TEXT("Classic Orange remains valid for all puck types"), State->SetPuckSkins(Skins));
	for (int32 Palette = 2; Palette <= 5; ++Palette)
	{
		Skins.Init(Palette, FlickPieceArchetypeRules::ArchetypeCount);
		TestTrue(TEXT("Server accepts each palette for every archetype"), State->SetPuckSkins(Skins));
		for (int32 Type = 0; Type < Skins.Num(); ++Type)
			TestEqual(TEXT("Player state retains each archetype's palette"), State->GetPuckSkin(static_cast<EFlickPieceArchetype>(Type)), Palette);
	}
	Skins.Init(1, FlickPieceArchetypeRules::ArchetypeCount);
	State->SetPuckSkins(Skins);
	for (int32 Retired = 6; Retired <= 9; ++Retired)
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
