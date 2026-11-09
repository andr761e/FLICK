// This EditorContext asset test uses editor-only mesh inspection APIs, which
// are not linked into the packaged Development game target.
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerState.h"
#include "Core/FlickCosmeticCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRetroArcadeTest, "FLICK.Cosmetics.RetroArcade",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRetroArcadeTest::RunTest(const FString& Parameters)
{
    using namespace FlickCosmeticCatalog;
    const auto Settings=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Settings);
    if (!TestNotNull(TEXT("Retro Arcade test world"),World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    UStaticMesh* StandardArt=nullptr;
    TSet<UStaticMesh*> UniqueArt;
    for (int32 Type=0;Type<FlickPieceArchetypeRules::ArchetypeCount;++Type)
    {
        const auto Archetype=static_cast<EFlickPieceArchetype>(Type);
        FString Name=GetPieceArchetypeName(Archetype).ToLower();
        Name[0]=FChar::ToUpper(Name[0]);
        const auto& Rules=FlickPieceArchetypeRules::Get(Archetype);
        const float Radius=45.f*Rules.RadiusMultiplier, Thickness=20.f*Rules.ThicknessMultiplier;
        TestEqual(TEXT("Every class appends Retro Arcade at the stable index"),GetItems(PuckCategoryStart+Type)[RetroArcadeSkin],FString(TEXT("RETRO ARCADE")));
        TestEqual(TEXT("Original six finishes remain available"),GetItems(PuckCategoryStart+Type).Num(),7);
        AFlickPiece* Piece=World->SpawnActor<AFlickPiece>();
        Piece->InitializePiece(EFlickTeam::Player1,Type+1,Radius,Thickness,Archetype);
        Piece->EnableTestArenaVisuals();
        auto* Visual=Piece->GetPresentationMesh();
        UStaticMesh* Classic=Visual->GetStaticMesh();
        auto* Physics=CastChecked<UStaticMeshComponent>(Piece->GetRootComponent());
        UStaticMesh* Collision=Physics->GetStaticMesh();
        const float Mass=Physics->GetMass();
        const auto* PhysicalMaterial=Physics->GetBodyInstance()->GetSimplePhysicalMaterial();
        const float Friction=PhysicalMaterial->Friction, Restitution=PhysicalMaterial->Restitution;
        const FVector PhysicsScale=Physics->GetComponentScale();
        Piece->SetPuckSkin(RetroArcadeSkin);
        UStaticMesh* Art=Visual->GetStaticMesh();
        if (TestNotNull(*FString::Printf(TEXT("%s Retro Arcade mesh loads"),*Name),Art))
        {
            UniqueArt.Add(Art);
            if (Type==0) StandardArt=Art;
            TestTrue(TEXT("Each type uses its own authored geometry"),Art!=Classic && Art->GetName()==FString::Printf(TEXT("SM_Puck_%s_RetroArcade"),*Name));
            TestTrue(TEXT("Class diameter and thickness match existing rules"),FMath::IsNearlyEqual(Art->GetBounds().BoxExtent.X,Radius,.3f) && FMath::IsNearlyEqual(Art->GetBounds().BoxExtent.Z,Thickness*.5f,.15f));
            TestTrue(TEXT("Pivot stays centered"),Art->GetBounds().Origin.Size()<.2f);
            TestEqual(TEXT("All types share ten material roles"),Art->GetStaticMaterials().Num(),10);
            TestTrue(TEXT("Packed canonical coordinates keep scaled class finishes consistent"),Art->GetNumUVChannels(0)>=3);
            TArray<UMaterialInterface*> Authored;
            for(int32 Slot=0;Slot<Visual->GetNumMaterials();++Slot) Authored.Add(Visual->GetMaterial(Slot));
            Piece->SetHovered(true);Piece->SetSelected(true);Piece->Tick(.016f);
            for(int32 Slot=0;Slot<Authored.Num();++Slot)
                TestTrue(TEXT("Selection preserves the authored multi-colour materials"),Visual->GetMaterial(Slot)==Authored[Slot]);
            TestTrue(TEXT("Every type retains the fixed puck lighting channel"),Visual->LightingChannels.bChannel1 && !Visual->LightingChannels.bChannel0);
        }
        TestTrue(TEXT("Skin never replaces the physics mesh"),Physics->GetStaticMesh()==Collision);
        TestEqual(TEXT("Skin preserves each class mass"),Physics->GetMass(),Mass);
        TestTrue(TEXT("Skin preserves class collision scale"),Physics->GetComponentScale().Equals(PhysicsScale));
        TestTrue(TEXT("Skin preserves physical material"),Physics->GetBodyInstance()->GetSimplePhysicalMaterial()==PhysicalMaterial);
        TestEqual(TEXT("Skin preserves friction"),PhysicalMaterial->Friction,Friction);
        TestEqual(TEXT("Skin preserves restitution"),PhysicalMaterial->Restitution,Restitution);
        TestEqual(TEXT("Cosmetic mesh never has collision"),Visual->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
        Piece->SetPuckSkin(0);
        TestTrue(TEXT("Unequipping restores each class original art"),Visual->GetStaticMesh()==Classic);
        for(const TCHAR* View:{TEXT(""),TEXT("Locker/"),TEXT("Presentation/"),TEXT("Hero/"),TEXT("ProfileCutout/")})
            TestNotNull(*FString::Printf(TEXT("%s %s portrait loads"),*Name,View),LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/UI/PuckThumbnails/%sT_Puck_%s_6.T_Puck_%s_6"),View,*Name,*Name)));
    }
    TestEqual(TEXT("Nine genuinely separate class meshes"),UniqueArt.Num(),9);
    AFlickPiece* Striker=World->SpawnActor<AFlickPiece>();
    Striker->InitializePiece(EFlickTeam::Player1,20,42.f,16.f,EFlickPieceArchetype::Standard,true);
    Striker->EnableTestArenaVisuals();Striker->SetPuckSkin(RetroArcadeSkin);
    TestTrue(TEXT("BOB striker retains the same Standard set skin"),Striker->GetPresentationMesh()->GetStaticMesh()==StandardArt);
	for (int32 Slot=0; Slot<Striker->GetPresentationMesh()->GetNumMaterials(); ++Slot)
	{
		const auto& MaterialSlot=StandardArt->GetStaticMaterials()[Slot];
		if (MaterialSlot.MaterialSlotName.ToString().Contains(TEXT("RetroArcade_Face")))
			TestTrue(TEXT("BOB striker has its target face instead of the scoring puck face"),
				Striker->GetPresentationMesh()->GetMaterial(Slot)->GetMaterial()->GetName()==TEXT("M_BobStrikerFace"));
		else
			TestTrue(TEXT("BOB striker preserves all other authored arcade material roles"),
				Striker->GetPresentationMesh()->GetMaterial(Slot)==MaterialSlot.MaterialInterface);
	}
    auto* Player=World->SpawnActor<AFlickPlayerState>();
    TArray<int32> Loadout;Loadout.Init(RetroArcadeSkin,FlickPieceArchetypeRules::ArchetypeCount);
    TestTrue(TEXT("Network loadout accepts Retro Arcade for all nine types"),Player->SetPuckSkins(Loadout));
    Loadout[8]=RetroArcadeSkin+1;
    TestFalse(TEXT("Network loadout still rejects invalid skin indices"),Player->SetPuckSkins(Loadout));
    World->DestroyWorld(false);GEngine->DestroyWorldContext(World);
    return true;
}
#endif
