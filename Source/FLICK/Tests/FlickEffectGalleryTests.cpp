#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Player/FlickCameraPawn.h"
#include "Pieces/FlickPiece.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Arena/FlickArenaLighting.h"
#include "UnrealClient.h"

namespace
{
	struct FEffectReview
	{
		TWeakObjectPtr<UWorld> World;
		TArray<TWeakObjectPtr<AActor>> Actors;
		FString Directory, Manifest = TEXT("image,slot,style,name,x,y\n");
		void Clear()
		{
			for (auto Actor : Actors) if (Actor.IsValid()) Actor->Destroy();
			Actors.Reset();
		}
	};
}

// Explicit visual-review utility: run in a disposable -game viewport, not NullRHI.
// It never writes player preferences or changes production preview behaviour.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickEffectGalleryTest,"FLICK.Visuals.EffectGallery",
	EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FFlickEffectGalleryTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(),TEXT("FlickEffectReview"))) return true;
	auto Review = MakeShared<FEffectReview>();
	Review->Directory = FPaths::ProjectSavedDir()/TEXT("EffectReview");
	FParse::Value(FCommandLine::Get(),TEXT("FlickEffectReviewDir="),Review->Directory);
	IFileManager::Get().MakeDirectory(*Review->Directory,true);
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this,Review]()
	{
		for (const auto& Context : GEngine->GetWorldContexts())
			if (Context.WorldType == EWorldType::Game) { Review->World = Context.World(); break; }
		UWorld* World = Review->World.Get();
		if (!TestNotNull(TEXT("Real RHI review world"),World)) return;
		if (World->GetAuthGameMode()) World->GetAuthGameMode()->SetActorTickEnabled(false);
		FPostProcessSettings Finish;
		for (TActorIterator<AFlickCameraPawn> It(World); It; ++It)
		{
			if (const auto* Component = It->FindComponentByClass<UCameraComponent>()) Finish = Component->PostProcessSettings;
			It->SetActorTickEnabled(false);
		}
		for (TActorIterator<AFlickPiece> It(World); It; ++It) { It->SetActorHiddenInGame(true); It->SetActorTickEnabled(false); }
		auto* Camera = World->SpawnActor<ACameraActor>();
		const FVector Location(0,-1050,1600), Focus(0,0,250);
		Camera->SetActorLocation(Location);
		Camera->SetActorRotation((Focus-Location).Rotation());
		Camera->GetCameraComponent()->SetFieldOfView(53);
		Camera->GetCameraComponent()->PostProcessSettings = Finish;
		World->GetFirstPlayerController()->SetViewTarget(Camera);
		// The Home close-up's focus distance is unsuitable for the review grid.
		World->GetFirstPlayerController()->ConsoleCommand(TEXT("r.DepthOfFieldQuality 0"));
		const auto Rig = FlickArenaLighting::FindRig(World);
		const auto Frame = FlickArenaLighting::MakePuckStudioFrame((Focus-Location).Rotation(),FRotator(-30,90,0),-90,12,true);
		FlickArenaLighting::UpdatePuckStudioLights(Rig.PuckFinishLight,Rig.PuckFillLight,Location,1,250,&Frame,true);
	},4.f));
	for (int32 Category = 12; Category <= 14; ++Category)
	{
		const int32 First = Category == 14 ? 0 : 1;
		const int32 Count = FlickCosmeticCatalog::GetItems(Category).Num();
		for (int32 Page = 0; First+Page*6 < Count; ++Page)
			for (int32 Phase = 0; Phase < 2; ++Phase)
			{
				const FString Image = FString::Printf(TEXT("family-%d-page-%d-phase-%d.png"),Category,Page,Phase);
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this,Review,Category,Page,Phase,First,Count,Image]()
				{
					Review->Clear();
					UWorld* World = Review->World.Get();
					if (!World) return;
					for (int32 Slot = 0; Slot < 6; ++Slot)
					{
						const int32 Style = First+Page*6+Slot;
						if (Style >= Count) break;
						const FVector Cell((Slot%3-1)*330.f,(Slot/3 ? -185.f : 190.f),252);
						if (Category != 14)
						{
							auto* Puck = World->SpawnActor<AFlickPiece>();
							Puck->InitializePiece(EFlickTeam::Player1,Slot+1,29,13,EFlickPieceArchetype::Standard);
							Puck->EnableTestArenaVisuals(); Puck->SetCosmeticPreview(true); Puck->BeginReplayPresentation();
							Puck->SetPuckEffects({Category == 12 ? Style : 0,0,0});
							const int32 Frames = 60+Phase*20;
							for (int32 Frame = 0; Frame <= Frames; ++Frame)
							{
								const float T = Frame/float(Frames);
								Puck->SetActorLocation(Cell+FVector(Category == 12 ? (T-.5f)*190.f : 0,
									Category == 12 ? FMath::Sin(T*PI)*35 : 0,10));
								Puck->Tick(1.f/60.f);
							}
							Puck->SetActorTickEnabled(false); Review->Actors.Add(Puck);
						}
						if (Category != 12)
						{
							auto* Effect = World->SpawnActor<AFlickWorldFeedback>(Cell,FRotator::ZeroRotator);
							Effect->InitializeFeedback(Category == 13 ? EFlickFeedbackKind::Spawn : EFlickFeedbackKind::Elimination,
								FLinearColor(0,.6f,1),1,FVector::ForwardVector,Style,1.2f);
							Effect->SetActorScale3D(FVector(.68f));
							Effect->Tick(Category == 13 ? (Phase ? 1.24f : .55f) : (Phase ? .7f : .23f));
							Effect->SetActorTickEnabled(false); Effect->SetLifeSpan(0); Review->Actors.Add(Effect);
						}
						FVector2D Pixel;
						World->GetFirstPlayerController()->ProjectWorldLocationToScreen(Cell,Pixel);
						Review->Manifest += FString::Printf(TEXT("%s,%d,%d,%s,%.0f,%.0f\n"),*Image,Slot,Style,
							*FlickCosmeticCatalog::GetItems(Category)[Style],Pixel.X,Pixel.Y);
					}
				},.1f));
				ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Review,Image]()
				{
					FScreenshotRequest::RequestScreenshot(Review->Directory/Image,false,false);
				},.8f));
			}
	}
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([Review]()
	{
		FFileHelper::SaveStringToFile(Review->Manifest,*(Review->Directory/TEXT("manifest.csv")));
		Review->Clear();
	},1.f));
	return true;
}
#endif
