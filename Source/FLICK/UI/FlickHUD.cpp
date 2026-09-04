#include "UI/FlickHUD.h"

#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "UI/FlickGameLayer.h"
#include "Widgets/SWeakWidget.h"

namespace
{
	constexpr float TopBarHeight = 205.0f;
	const FLinearColor PanelColor(0.008f, 0.012f, 0.02f, 0.93f);
	const FLinearColor MutedTextColor(0.62f, 0.68f, 0.72f, 1.0f);

	FString GetDramaticFeedLabel(const AFlickGameState& GameState)
	{
		switch (GameState.DramaticEvent)
		{
		case EFlickDramaticEvent::Trade:
			return TEXT("DOUBLE KNOCKOUT  //  TRADE");
		case EFlickDramaticEvent::DoubleKnockout:
			return TEXT("DOUBLE KNOCKOUT");
		case EFlickDramaticEvent::MultiKnockout:
			return FString::Printf(TEXT("MULTI KNOCKOUT  //  %d PUCKS"), GameState.DramaticEventValue);
		case EFlickDramaticEvent::SelfKnockout:
			return TEXT("SELF KNOCKOUT");
		case EFlickDramaticEvent::LastPuckStanding:
			return TEXT("LAST PUCK STANDING");
		case EFlickDramaticEvent::ChainReaction:
			return FString::Printf(TEXT("CHAIN REACTION  //  %d IMPACTS"), GameState.DramaticEventImpactCount);
		case EFlickDramaticEvent::None:
		default:
			return FString();
		}
	}

	FLinearColor GetDramaticFeedColor(const AFlickGameState& GameState)
	{
		if (GameState.DramaticEvent == EFlickDramaticEvent::SelfKnockout)
		{
			return FLinearColor(1.0f, 0.3f, 0.08f, 1.0f);
		}
		if (GameState.DramaticEvent == EFlickDramaticEvent::Trade
			|| GameState.DramaticEventTeam == EFlickTeam::None)
		{
			return FLinearColor(1.0f, 0.76f, 0.16f, 1.0f);
		}
		return GetTeamColor(GameState.DramaticEventTeam);
	}
}

void AFlickHUD::BeginPlay()
{
	Super::BeginPlay();
	AFlickGameMode* FlickGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr;
	AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(GetOwningPlayerController());
	if (!GEngine || !GEngine->GameViewport || !FlickController)
	{
		return;
	}
	SAssignNew(GameLayer, SFlickGameLayer)
		.OwnerHud(this)
		.GameMode(FlickGameMode)
		.PlayerController(FlickController);
	GameLayerContainer = SNew(SWeakWidget).PossiblyNullContent(GameLayer.ToSharedRef());
	GEngine->GameViewport->AddViewportWidgetContent(GameLayerContainer.ToSharedRef(), 20);
	if (UFlickGameInstance* FlickGameInstance = Cast<UFlickGameInstance>(GetGameInstance()))
	{
		FlickGameInstance->NotifyFrontendReady();
		if (!FlickGameMode)
		{
			FlickGameInstance->CompleteStartupPresentation();
		}
	}
}

void AFlickHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GEngine && GEngine->GameViewport && GameLayerContainer.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GameLayerContainer.ToSharedRef());
	}
	GameLayerContainer.Reset();
	GameLayer.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFlickHUD::DrawHUD()
{
	Super::DrawHUD();

	const AFlickGameMode* FlickGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr;
	const AFlickGameState* FlickGameState = GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
	const AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(GetOwningPlayerController());
	if (!Canvas || !FlickGameState)
	{
		return;
	}

	const float Width = Canvas->ClipX;
	const float Height = Canvas->ClipY;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	MenuHitRegions.Reset();
	float CursorX = -1000.0f;
	float CursorY = -1000.0f;
	if (GetOwningPlayerController() && GetOwningPlayerController()->GetMousePosition(CursorX, CursorY))
	{
		MenuCursorPosition = FVector2D(CursorX, CursorY);
	}

	if (FlickGameMode && FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu)
	{
		return;
	}
	if (FlickGameMode && FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::ModeSelect)
	{
		return;
	}

	if (FlickGameMode && FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Loadout)
	{
		return;
	}

	if (FlickGameMode && FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Settings)
	{
		return;
	}
	if (FlickGameMode && FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::ItemShop)
	{
		return;
	}
	if (FlickGameMode && FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::PrivateMatch)
	{
		return;
	}
	if (!FlickGameMode && FlickGameState->bPrivateMatchLobbyActive && FlickController)
	{
		DrawPrivateMatchFrontend(*FlickGameState, *FlickController, Width, Height);
		return;
	}
	if (!FlickGameMode && FlickGameState->bNetworkLobbyActive && FlickController)
	{
		DrawNetworkLobby(*FlickGameState, *FlickController, Width, Height);
		return;
	}
	if (!FlickGameMode && FlickGameState->bPartyActive && FlickController)
	{
		DrawPartyFrontend(*FlickGameState, *FlickController, Width, Height);
		return;
	}

	ObserveMatchState(*FlickGameState, Now);
	if (FlickGameMode && FlickGameMode->HasLockedKickoffShot())
	{
		DrawLockedKickoffPresentation(*FlickGameMode);
	}

	if (FlickController && FlickController->IsAimingShot())
	{
		if (!FlickGameMode || FlickGameMode->IsAimGuideEnabled())
		{
			DrawAimPresentation(*FlickController);
		}
	}

	if (!FlickGameMode && !GameLayer.IsValid())
	{
		DrawTopBar(*FlickGameState, Width, Now);
		DrawEventFeed(Width, Now);
		DrawTurnBanner(*FlickGameState, Width, Height, Now);
		if (FlickController && FlickController->IsAimingShot())
		{
			DrawPowerMeter(*FlickController, Width, Height);
		}
		if (FlickGameState->MatchPhase == EFlickMatchPhase::RoundOver)
		{
			DrawRoundOver(*FlickGameState, Width, Height);
		}
		else
		{
			DrawControls(Height);
		}
	}
}

void AFlickHUD::DrawLockedKickoffPresentation(const AFlickGameMode& GameMode)
{
	const AFlickPiece* Piece = GameMode.GetLockedKickoffPiece();
	const APlayerController* Controller = GetOwningPlayerController();
	if (!Piece || !Controller)
	{
		return;
	}

	const FVector StartWorld = Piece->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
	const float GuideLength = FMath::Lerp(260.0f, 920.0f, GameMode.GetLockedKickoffPower())
		* Piece->GetLaunchSpeedMultiplier();
	const FVector EndWorld = StartWorld + GameMode.GetLockedKickoffDirection() * GuideLength;
	FVector2D StartScreen;
	FVector2D EndScreen;
	if (!Controller->ProjectWorldLocationToScreen(StartWorld, StartScreen)
		|| !Controller->ProjectWorldLocationToScreen(EndWorld, EndScreen))
	{
		return;
	}

	DrawTechnicalAimArrow(
		StartScreen,
		EndScreen,
		FMath::Lerp(GetTeamColor(Piece->GetTeam()), FLinearColor::White, 0.12f));
}

void AFlickHUD::DrawTechnicalAimArrow(
	const FVector2D& Start,
	const FVector2D& End,
	const FLinearColor& Accent)
{
	const FVector2D ArrowVector = End - Start;
	const float ArrowLength = ArrowVector.Size();
	if (ArrowLength < 18.0f)
	{
		return;
	}

	const FVector2D Direction = ArrowVector / ArrowLength;
	const FVector2D Side(-Direction.Y, Direction.X);
	const float TailInset = FMath::Min(25.0f, ArrowLength * 0.16f);
	const float HeadLength = FMath::Clamp(ArrowLength * 0.14f, 22.0f, 36.0f);
	const FVector2D RailStart = Start + Direction * TailInset;
	const FVector2D HeadBase = End - Direction * FMath::Min(HeadLength, ArrowLength * 0.4f);
	const float RailLength = FVector2D::Distance(RailStart, HeadBase);
	const FLinearColor BrightAccent = FMath::Lerp(Accent, FLinearColor::White, 0.22f);

	// Thin parallel rails and separated power blocks mirror the angular HUD
	// borders without covering the arena or the puck underneath the guide.
	for (const float RailSide : {-1.0f, 1.0f})
	{
		const FVector2D RailOffset = Side * 6.0f * RailSide;
		DrawLine(
			RailStart.X + RailOffset.X,
			RailStart.Y + RailOffset.Y,
			HeadBase.X + RailOffset.X,
			HeadBase.Y + RailOffset.Y,
			FLinearColor(Accent.R, Accent.G, Accent.B, 0.24f),
			1.0f);
	}

	constexpr int32 SegmentCount = 6;
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const float SegmentStartAlpha = static_cast<float>(SegmentIndex) / SegmentCount;
		const float SegmentEndAlpha = FMath::Min(
			SegmentStartAlpha + 0.72f / SegmentCount,
			1.0f);
		const FVector2D SegmentStart = RailStart + Direction * RailLength * SegmentStartAlpha;
		const FVector2D SegmentEnd = RailStart + Direction * RailLength * SegmentEndAlpha;
		const float SegmentOpacity = FMath::Lerp(0.52f, 0.96f, SegmentEndAlpha);
		DrawLine(SegmentStart.X, SegmentStart.Y, SegmentEnd.X, SegmentEnd.Y,
			FLinearColor(0.0f, 0.006f, 0.012f, 0.9f), 8.0f);
		DrawLine(SegmentStart.X, SegmentStart.Y, SegmentEnd.X, SegmentEnd.Y,
			FLinearColor(Accent.R, Accent.G, Accent.B, SegmentOpacity), 3.5f);
		DrawLine(SegmentStart.X, SegmentStart.Y, SegmentEnd.X, SegmentEnd.Y,
			FLinearColor(BrightAccent.R, BrightAccent.G, BrightAccent.B, SegmentOpacity * 0.72f), 1.0f);
	}

	const FVector2D OuterLeft = HeadBase + Side * 17.0f;
	const FVector2D OuterRight = HeadBase - Side * 17.0f;
	const FVector2D InnerBase = HeadBase + Direction * 5.0f;
	const FVector2D InnerLeft = InnerBase + Side * 10.5f;
	const FVector2D InnerRight = InnerBase - Side * 10.5f;
	DrawFilledTriangle(End + Direction * 3.0f, OuterLeft, OuterRight, FLinearColor(0.0f, 0.006f, 0.012f, 0.92f));
	DrawLine(End.X, End.Y, OuterLeft.X, OuterLeft.Y, Accent, 3.0f);
	DrawLine(End.X, End.Y, OuterRight.X, OuterRight.Y, Accent, 3.0f);
	DrawLine(OuterLeft.X, OuterLeft.Y, InnerLeft.X, InnerLeft.Y, Accent.CopyWithNewOpacity(0.58f), 1.5f);
	DrawLine(OuterRight.X, OuterRight.Y, InnerRight.X, InnerRight.Y, Accent.CopyWithNewOpacity(0.58f), 1.5f);
	DrawLine(End.X, End.Y, InnerLeft.X, InnerLeft.Y, BrightAccent, 1.25f);
	DrawLine(End.X, End.Y, InnerRight.X, InnerRight.Y, BrightAccent, 1.25f);

	TArray<FVector2D, TInlineAllocator<9>> OriginPoints;
	for (int32 PointIndex = 0; PointIndex <= 8; ++PointIndex)
	{
		const float Angle = PI * 0.125f + 2.0f * PI * static_cast<float>(PointIndex % 8) / 8.0f;
		OriginPoints.Add(Start + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * 18.0f);
	}
	for (int32 PointIndex = 0; PointIndex < 8; ++PointIndex)
	{
		const FVector2D& A = OriginPoints[PointIndex];
		const FVector2D& B = OriginPoints[PointIndex + 1];
		DrawLine(A.X, A.Y, B.X, B.Y, FLinearColor(0.0f, 0.006f, 0.012f, 0.88f), 5.5f);
		DrawLine(A.X, A.Y, B.X, B.Y, Accent.CopyWithNewOpacity(0.9f), 2.0f);
	}
	DrawLine(Start.X - Side.X * 7.0f, Start.Y - Side.Y * 7.0f,
		Start.X + Side.X * 7.0f, Start.Y + Side.Y * 7.0f, BrightAccent, 1.5f);
}

void AFlickHUD::PushEventMessage(
	const FString& Message,
	const FLinearColor& Color,
	const float Duration,
	const FString& PointsText)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	FFlickHudEventMessage& Event = EventMessages.AddDefaulted_GetRef();
	Event.Message = Message;
	Event.PointsText = PointsText;
	Event.Color = Color;
	Event.CreatedAt = Now;
	Event.ExpiresAt = Now + FMath::Max(Duration, 0.1f);

	while (EventMessages.Num() > 4)
	{
		EventMessages.RemoveAt(0);
	}
}

void AFlickHUD::ResetPresentation()
{
	EventMessages.Reset();
	bHasObservedState = false;
	LastObservedTeam = EFlickTeam::None;
	LastObservedDramaticEventSerial = 0;
	LastObservedPhase = EFlickMatchPhase::WaitingToStart;
	StateChangedAt = -100.0f;
}

void AFlickHUD::ObserveMatchState(const AFlickGameState& GameState, const float Now)
{
	if (LastObservedDramaticEventSerial != GameState.DramaticEventSerial)
	{
		LastObservedDramaticEventSerial = GameState.DramaticEventSerial;
		if (GameState.DramaticEvent != EFlickDramaticEvent::None)
		{
			FString PointsText;
			if (GameState.DramaticEventBonusPoints > 0)
			{
				PointsText = GameState.DramaticEvent == EFlickDramaticEvent::Trade
					? FString::Printf(TEXT("+%d EACH"), GameState.DramaticEventBonusPoints)
					: FString::Printf(TEXT("+%d"), GameState.DramaticEventBonusPoints);
			}
			PushEventMessage(
				GetDramaticFeedLabel(GameState),
				GetDramaticFeedColor(GameState),
				GameState.DramaticEventDuration,
				PointsText);
		}
	}

	if (!bHasObservedState
		|| LastObservedPhase != GameState.MatchPhase
		|| LastObservedTeam != GameState.CurrentTeam)
	{
		LastObservedPhase = GameState.MatchPhase;
		LastObservedTeam = GameState.CurrentTeam;
		StateChangedAt = Now;
		bHasObservedState = true;
	}
}

void AFlickHUD::DrawTopBar(const AFlickGameState& GameState, const float Width, const float Now)
{
	const float SideWidth = FMath::Clamp(Width * 0.25f, 300.0f, 400.0f);
	DrawTeamBlock(
		EFlickTeam::Player1,
		GameState.Player1ActivePieces,
		GameState.StartingPiecesPerTeam,
		GameState.Player1ShotsTaken,
		GameState.Player1RoundsWon,
		GameState.RoundsToWin,
		20.0f,
		22.0f,
		SideWidth,
		GameState.MatchPhase == EFlickMatchPhase::Aiming && GameState.CurrentTeam == EFlickTeam::Player1);
	DrawTeamBlock(
		EFlickTeam::Player2,
		GameState.Player2ActivePieces,
		GameState.StartingPiecesPerTeam,
		GameState.Player2ShotsTaken,
		GameState.Player2RoundsWon,
		GameState.RoundsToWin,
		Width - SideWidth - 20.0f,
		22.0f,
		SideWidth,
		GameState.MatchPhase == EFlickMatchPhase::Aiming && GameState.CurrentTeam == EFlickTeam::Player2);

	FString Status;
	FLinearColor StatusColor = FLinearColor::White;
	if (GameState.MatchPhase == EFlickMatchPhase::RoundOver)
	{
		Status = GameState.bDraw
			? TEXT("ROUND DRAW")
			: (GameState.bSeriesComplete
				? FString::Printf(TEXT("PLAYER %d MATCH WIN"), GetTeamNumber(GameState.WinnerTeam))
				: FString::Printf(TEXT("PLAYER %d TAKES ROUND"), GetTeamNumber(GameState.WinnerTeam)));
		StatusColor = GameState.bDraw
			? FLinearColor(1.0f, 0.8f, 0.15f, 1.0f)
			: GetTeamColor(GameState.WinnerTeam);
	}
	else if (GameState.MatchPhase == EFlickMatchPhase::ResolvingPhysics)
	{
		const int32 DotCount = 1 + static_cast<int32>(Now * 2.5f) % 3;
		Status = TEXT("PHYSICS IN MOTION") + FString::ChrN(DotCount, TEXT('.'));
		StatusColor = FLinearColor(1.0f, 0.78f, 0.16f, 1.0f);
	}
	else
	{
		Status = FString::Printf(TEXT("PLAYER %d TO FLICK"), GetTeamNumber(GameState.CurrentTeam));
		StatusColor = GetTeamColor(GameState.CurrentTeam);
	}

	const AFlickGameMode* CurrentGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr;
	const EFlickMatchVariant Variant = CurrentGameMode ? CurrentGameMode->GetSelectedMatchVariant() : GameState.ActiveMatchVariant;
	const float BannerWidth = FMath::Clamp(Width * 0.45f, 520.0f, 720.0f);
	const float BannerX = (Width - BannerWidth) * 0.5f;
	DrawShowcasePanel(BannerX, 12.0f, BannerWidth, 145.0f, PanelColor, FLinearColor(0.0f, 0.76f, 1.0f, 0.72f), 16.0f);
	DrawCenteredText(Variant == EFlickMatchVariant::Bob ? TEXT("BOB") : TEXT("FLICK"), Width * 0.5f, 37.0f, FLinearColor(0.95f, 0.98f, 1.0f, 1.0f), 1.55f, true);
	DrawCenteredText(Status, Width * 0.5f, 83.0f, StatusColor, 0.92f, true);
	DrawCenteredText(
		Variant == EFlickMatchVariant::Bob
			? FString::Printf(TEXT("STANDARD PUCKS  /  SHOT %d"), GameState.TurnNumber)
			: FString::Printf(TEXT("ROUND %d  /  SHOT %d"), GameState.RoundNumber, GameState.TurnNumber),
		Width * 0.5f,
		116.0f,
		MutedTextColor,
		0.68f);
}

void AFlickHUD::DrawAimPresentation(const AFlickPlayerController& Controller)
{
	const AFlickPiece* SelectedPiece = Controller.GetSelectedPiece();
	if (!SelectedPiece || !Controller.HasAimCursorPoint())
	{
		return;
	}

	FVector2D PieceScreen;
	FVector2D CursorScreen;
	const FVector PieceWorld = SelectedPiece->GetActorLocation() + FVector(0.0f, 0.0f, 28.0f);
	if (!Controller.ProjectWorldLocationToScreen(PieceWorld, PieceScreen)
		|| !Controller.ProjectWorldLocationToScreen(Controller.GetAimCursorWorldPoint(), CursorScreen))
	{
		return;
	}

	const FFlickLaunchResult& Aim = Controller.GetAimResult();
	const FLinearColor PowerColor = Aim.bValidShot ? GetPowerColor(Aim.NormalizedPower) : MutedTextColor;
	const FLinearColor TeamGuideColor = FMath::Lerp(GetTeamColor(SelectedPiece->GetTeam()), FLinearColor::White, 0.18f);
	const FLinearColor GuideColor = FMath::Lerp(TeamGuideColor, PowerColor, 0.24f);

	// The pull line is intentionally subordinate to the launch direction, but the
	// layered treatment keeps it crisp over both bright pucks and the dark arena.
	DrawLine(PieceScreen.X, PieceScreen.Y, CursorScreen.X, CursorScreen.Y, FLinearColor(0.0f, 0.0f, 0.0f, 0.78f), 8.0f);
	DrawLine(PieceScreen.X, PieceScreen.Y, CursorScreen.X, CursorScreen.Y, FLinearColor(PowerColor.R, PowerColor.G, PowerColor.B, 0.36f), 5.0f);
	DrawLine(PieceScreen.X, PieceScreen.Y, CursorScreen.X, CursorScreen.Y, FLinearColor(PowerColor.R, PowerColor.G, PowerColor.B, 0.94f), 2.0f);
	DrawCircle(CursorScreen, 15.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.86f), 40, 6.0f);
	DrawCircle(CursorScreen, 12.0f, PowerColor, 40, 2.5f);
	DrawCircle(CursorScreen, 5.0f, FMath::Lerp(PowerColor, FLinearColor::White, 0.65f), 32, 2.0f);
	DrawLine(CursorScreen.X - 19.0f, CursorScreen.Y, CursorScreen.X - 8.0f, CursorScreen.Y, PowerColor, 1.8f);
	DrawLine(CursorScreen.X + 8.0f, CursorScreen.Y, CursorScreen.X + 19.0f, CursorScreen.Y, PowerColor, 1.8f);
	DrawLine(CursorScreen.X, CursorScreen.Y - 19.0f, CursorScreen.X, CursorScreen.Y - 8.0f, PowerColor, 1.8f);
	DrawLine(CursorScreen.X, CursorScreen.Y + 8.0f, CursorScreen.X, CursorScreen.Y + 19.0f, PowerColor, 1.8f);

	if (!Aim.bValidShot)
	{
		return;
	}

	const float GuideLength = FMath::Max(Controller.GetAimGuideDistance(), 1.0f);
	FVector2D EndScreen;
	if (Controller.ProjectWorldLocationToScreen(PieceWorld + Aim.Direction * GuideLength, EndScreen))
	{
		DrawTechnicalAimArrow(PieceScreen, EndScreen, GuideColor);
	}

	if (Controller.HasPredictedContact())
	{
		FVector2D ContactScreen;
		if (Controller.ProjectWorldLocationToScreen(
			Controller.GetPredictedContactWorldPoint() + FVector(0.0f, 0.0f, 24.0f),
			ContactScreen))
		{
			DrawCircle(ContactScreen, 24.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.88f), 48, 7.0f);
			DrawCircle(ContactScreen, 20.0f, FLinearColor::White, 48, 2.2f);
			DrawCircle(ContactScreen, 14.0f, GuideColor, 48, 3.0f);
			DrawCircle(ContactScreen, 6.0f, FMath::Lerp(GuideColor, FLinearColor::White, 0.65f), 36, 2.0f);
			DrawLine(ContactScreen.X - 28.0f, ContactScreen.Y, ContactScreen.X - 17.0f, ContactScreen.Y, GuideColor, 2.0f);
			DrawLine(ContactScreen.X + 17.0f, ContactScreen.Y, ContactScreen.X + 28.0f, ContactScreen.Y, GuideColor, 2.0f);
			DrawLine(ContactScreen.X, ContactScreen.Y - 28.0f, ContactScreen.X, ContactScreen.Y - 17.0f, GuideColor, 2.0f);
			DrawLine(ContactScreen.X, ContactScreen.Y + 17.0f, ContactScreen.X, ContactScreen.Y + 28.0f, GuideColor, 2.0f);
		}
	}
}

void AFlickHUD::DrawPowerMeter(const AFlickPlayerController& Controller, const float Width, const float Height)
{
	const float Power = FMath::Clamp(Controller.GetAimResult().NormalizedPower, 0.0f, 1.0f);
	const float MeterWidth = FMath::Min(380.0f, Width - 80.0f);
	const float MeterX = (Width - MeterWidth) * 0.5f;
	const float MeterY = Height - 82.0f;
	const FLinearColor PowerColor = GetPowerColor(Power);
	const AFlickPiece* SelectedPiece = Controller.GetSelectedPiece();
	const FString PowerLabel = Power >= 0.995f
		? TEXT("MAX POWER")
		: FString::Printf(TEXT("POWER  %d%%"), FMath::RoundToInt(Power * 100.0f));
	const AFlickGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr;
	const AFlickGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
	const float MaxSpeed = GameMode
		? GameMode->GetMaxLaunchSpeed()
		: (GameState ? GameState->MaxLaunchSpeed : 0.0f);
	const int32 EffectiveSpeed = SelectedPiece
		? FMath::RoundToInt(MaxSpeed * SelectedPiece->GetLaunchSpeedMultiplier() * Power)
		: 0;
	const FString MeterLabel = SelectedPiece
		? FString::Printf(TEXT("%s  |  %d CM/S  |  %s"), *GetPieceArchetypeName(SelectedPiece->GetArchetype()), EffectiveSpeed, *PowerLabel)
		: PowerLabel;

	DrawShowcasePanel(MeterX - 12.0f, MeterY - 24.0f, MeterWidth + 24.0f, 58.0f, PanelColor, PowerColor, 8.0f);
	DrawCenteredText(
		MeterLabel,
		Width * 0.5f,
		MeterY - 19.0f,
		PowerColor,
		0.72f);
	DrawPanel(MeterX, MeterY + 8.0f, MeterWidth, 8.0f, FLinearColor(0.13f, 0.15f, 0.18f, 1.0f));
	DrawPanel(MeterX, MeterY + 8.0f, MeterWidth * Power, 8.0f, PowerColor);
}

void AFlickHUD::DrawEventFeed(const float Width, const float Now)
{
	EventMessages.RemoveAll([Now](const FFlickHudEventMessage& Event)
	{
		return Event.ExpiresAt <= Now;
	});

	float Y = TopBarHeight + 18.0f;
	for (int32 Index = EventMessages.Num() - 1; Index >= 0; --Index)
	{
		const FFlickHudEventMessage& Event = EventMessages[Index];
		const float Duration = FMath::Max(Event.ExpiresAt - Event.CreatedAt, 0.1f);
		const float Remaining = FMath::Clamp((Event.ExpiresAt - Now) / Duration, 0.0f, 1.0f);
		const float Fade = FMath::Clamp(Remaining * 3.0f, 0.0f, 1.0f);
		const float EventWidth = 280.0f;
		const float X = Width - EventWidth - 20.0f;
		DrawShowcasePanel(
			X,
			Y,
			EventWidth,
			34.0f,
			FLinearColor(0.01f, 0.015f, 0.02f, 0.76f * Fade),
			FLinearColor(Event.Color.R, Event.Color.G, Event.Color.B, Fade),
			6.0f);
		DrawText(Event.Message, FLinearColor(Event.Color.R, Event.Color.G, Event.Color.B, Fade), X + 14.0f, Y + 8.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.78f);
		if (!Event.PointsText.IsEmpty())
		{
			DrawText(Event.PointsText, FLinearColor(0.3f, 1.0f, 0.58f, Fade), X + EventWidth - 66.0f, Y + 8.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.78f);
		}
		Y += 41.0f;
	}
}

void AFlickHUD::DrawTurnBanner(
	const AFlickGameState& GameState,
	const float Width,
	const float Height,
	const float Now)
{
	if (GameState.MatchPhase != EFlickMatchPhase::Aiming)
	{
		return;
	}

	const float Age = Now - StateChangedAt;
	if (Age < 0.0f || Age > 1.15f)
	{
		return;
	}

	const float Fade = Age < 0.15f ? Age / 0.15f : FMath::Clamp((1.15f - Age) / 0.35f, 0.0f, 1.0f);
	const FLinearColor TeamColor = GetTeamColor(GameState.CurrentTeam);
	const float BannerWidth = FMath::Min(440.0f, Width - 60.0f);
	const float BannerX = (Width - BannerWidth) * 0.5f;
	const float BannerY = Height * 0.35f;
	DrawShowcasePanel(
		BannerX,
		BannerY,
		BannerWidth,
		88.0f,
		FLinearColor(0.006f, 0.012f, 0.018f, 0.82f * Fade),
		FLinearColor(TeamColor.R, TeamColor.G, TeamColor.B, Fade),
		12.0f);
	DrawCenteredText(
		FString::Printf(TEXT("ROUND %d"), GameState.RoundNumber),
		Width * 0.5f,
		BannerY + 10.0f,
		FLinearColor(MutedTextColor.R, MutedTextColor.G, MutedTextColor.B, Fade),
		0.62f);
	DrawCenteredText(
		GameState.PlayersPerTeam > 1
			? FString::Printf(
				TEXT("TEAM %d  /  PLAYER %d"),
				GetTeamNumber(GameState.CurrentTeam),
				GameState.CurrentTeamPlayerSlot + 1)
			: FString::Printf(TEXT("PLAYER %d"), GetTeamNumber(GameState.CurrentTeam)),
		Width * 0.5f,
		BannerY + 30.0f,
		FLinearColor(TeamColor.R, TeamColor.G, TeamColor.B, Fade),
		1.22f,
		true);
	DrawCenteredText(
		GameState.TurnNumber == 1 ? TEXT("OPENS THE ROUND") : TEXT("YOUR TURN"),
		Width * 0.5f,
		BannerY + 66.0f,
		FLinearColor(1.0f, 1.0f, 1.0f, Fade),
		0.64f);
}

void AFlickHUD::DrawRoundOver(const AFlickGameState& GameState, const float Width, const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.005f, 0.008f, 0.012f, 0.66f));
	const FLinearColor ResultColor = GameState.bDraw
		? FLinearColor(1.0f, 0.8f, 0.15f, 1.0f)
		: GetTeamColor(GameState.WinnerTeam);
	const float ResultPanelWidth = FMath::Min(560.0f, Width - 48.0f);
	DrawShowcasePanel(
		(Width - ResultPanelWidth) * 0.5f,
		Height * 0.25f,
		ResultPanelWidth,
		Height * 0.5f,
		FLinearColor(0.002f, 0.008f, 0.016f, 0.94f),
		ResultColor,
		18.0f);
	const FString Result = GameState.bDraw
		? GameState.bSeriesComplete ? TEXT("MATCH DRAW") : TEXT("ROUND DRAW")
		: (GameState.bSeriesComplete
			? FString::Printf(TEXT("PLAYER %d CHAMPION"), GetTeamNumber(GameState.WinnerTeam))
			: FString::Printf(TEXT("PLAYER %d TAKES ROUND"), GetTeamNumber(GameState.WinnerTeam)));

	DrawCenteredText(
		GameState.bSeriesComplete ? TEXT("MATCH COMPLETE") : FString::Printf(TEXT("ROUND %d COMPLETE"), GameState.RoundNumber),
		Width * 0.5f,
		Height * 0.31f,
		MutedTextColor,
		0.76f);
	DrawCenteredText(Result, Width * 0.5f, Height * 0.38f, ResultColor, 1.85f, true);
	const FString SeriesScore = GameState.bSeriesComplete && GameState.Player1RoundsWon == GameState.Player2RoundsWon
		? FString::Printf(
			TEXT("SERIES  %d - %d   |   POINTS  %d - %d"),
			GameState.Player1RoundsWon,
			GameState.Player2RoundsWon,
			GameState.GetTeamScore(EFlickTeam::Player1),
			GameState.GetTeamScore(EFlickTeam::Player2))
		: FString::Printf(TEXT("SERIES  %d  -  %d"), GameState.Player1RoundsWon, GameState.Player2RoundsWon);
	DrawCenteredText(
		SeriesScore,
		Width * 0.5f,
		Height * 0.5f,
		FLinearColor::White,
		1.05f,
		true);
	DrawCenteredText(
		FString::Printf(TEXT("%d SHOTS THIS ROUND"), GameState.Player1ShotsTaken + GameState.Player2ShotsTaken),
		Width * 0.5f,
		Height * 0.56f,
		MutedTextColor,
		0.72f);
	const float ButtonWidth = FMath::Min(230.0f, Width * 0.22f);
	const float ButtonY = Height * 0.66f;
	DrawMenuButton(
		GameState.bSeriesComplete ? EFlickMenuAction::Restart : EFlickMenuAction::NextRound,
		GameState.bSeriesComplete ? TEXT("REMATCH") : TEXT("NEXT ROUND"),
		Width * 0.5f - ButtonWidth - 8.0f,
		ButtonY,
		ButtonWidth,
		48.0f,
		ResultColor,
		true);
	const bool bNetworkMatch = GameState.PlayerArray.Num() > 1;
	DrawMenuButton(
		bNetworkMatch ? EFlickMenuAction::MainMenu : GameState.bSeriesComplete ? EFlickMenuAction::EditLoadout : EFlickMenuAction::MainMenu,
		bNetworkMatch ? TEXT("LEAVE SESSION") : GameState.bSeriesComplete ? TEXT("EDIT LINEUPS") : TEXT("MAIN MENU"),
		Width * 0.5f + 8.0f,
		ButtonY,
		ButtonWidth,
		48.0f,
		FLinearColor(0.4f, 0.44f, 0.48f, 1.0f));
}

void AFlickHUD::DrawNetworkLobby(
	const AFlickGameState& GameState,
	const AFlickPlayerController& Controller,
	const float Width,
	const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.002f, 0.006f, 0.012f, 0.86f));
	DrawCenteredText(TEXT("LOCAL LOBBY"), Width * 0.5f, Height * 0.1f, FLinearColor::White, 1.8f, true);
	DrawCenteredText(TEXT("CONNECTED TO HOST"), Width * 0.5f, Height * 0.17f, FLinearColor(0.0f, 0.76f, 1.0f, 1.0f), 0.72f);

	const FString ModeName = GetMatchVariantName(GameState.LobbySelectedVariant);
	const FLinearColor ModeColor = GetModeColor(GameState.LobbySelectedVariant);
	DrawCenteredText(ModeName, Width * 0.5f, Height * 0.25f, ModeColor, 1.35f, true);
	DrawCenteredText(GetMatchVariantSummary(GameState.LobbySelectedVariant), Width * 0.5f, Height * 0.31f, MutedTextColor, 0.68f);

	auto FindLobbyPlayer = [&GameState](const EFlickTeam Team, const int32 PlayerSlot) -> const AFlickPlayerState*
	{
		for (const APlayerState* PlayerState : GameState.PlayerArray)
		{
			const AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(PlayerState);
			if (FlickPlayerState
				&& FlickPlayerState->GetTeam() == Team
				&& FlickPlayerState->GetTeamPlayerSlot() == PlayerSlot)
			{
				return FlickPlayerState;
			}
		}
		return nullptr;
	};

	const float SlotWidth = FMath::Min(350.0f, Width * 0.38f);
	const float SlotHeight = GameState.PlayersPerTeam > 1 ? 72.0f : 116.0f;
	const float SlotStartY = Height * 0.37f;
	for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
	{
		const bool bPlayer1 = Team == EFlickTeam::Player1;
		const float SlotX = bPlayer1 ? Width * 0.5f - SlotWidth - 12.0f : Width * 0.5f + 12.0f;
		const FLinearColor Accent = GetTeamColor(Team);
		for (int32 PlayerSlot = 0; PlayerSlot < GameState.PlayersPerTeam; ++PlayerSlot)
		{
			const float SlotY = SlotStartY + PlayerSlot * (SlotHeight + 8.0f);
			const AFlickPlayerState* Player = FindLobbyPlayer(Team, PlayerSlot);
			DrawShowcasePanel(
				SlotX,
				SlotY,
				SlotWidth,
				SlotHeight,
				FLinearColor(0.006f, 0.016f, 0.028f, 0.94f),
				Accent,
				10.0f);
			DrawText(
				FString::Printf(TEXT("TEAM %d  /  PLAYER %d%s"), GetTeamNumber(Team), PlayerSlot + 1, bPlayer1 && PlayerSlot == 0 ? TEXT("  /  HOST") : TEXT("")),
				Accent,
				SlotX + 18.0f,
				SlotY + 10.0f,
				GEngine ? GEngine->GetSmallFont() : nullptr,
				0.68f);
			DrawText(Player ? Player->GetPlayerName() : TEXT("WAITING FOR PLAYER"), FLinearColor::White, SlotX + 18.0f, SlotY + 31.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 0.66f);
			DrawText(!Player ? TEXT("OPEN SLOT") : Player->IsLobbyReady() ? TEXT("READY") : TEXT("NOT READY"), Player && Player->IsLobbyReady() ? Accent : MutedTextColor, SlotX + SlotWidth - 92.0f, SlotY + 14.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.62f);
		}
	}

	const AFlickPlayerState* LocalState = Controller.GetPlayerState<AFlickPlayerState>();
	const bool bReady = LocalState && LocalState->IsLobbyReady();
	const float ButtonWidth = FMath::Min(260.0f, Width * 0.3f);
	const float ButtonY = Height * 0.76f;
	DrawMenuButton(EFlickMenuAction::ToggleLobbyReady, bReady ? TEXT("SET NOT READY") : TEXT("READY UP"), Width * 0.5f - ButtonWidth - 8.0f, ButtonY, ButtonWidth, 54.0f, GetTeamColor(Controller.GetLocalTeam()), true);
	DrawMenuButton(EFlickMenuAction::LeaveLobby, TEXT("LEAVE LOBBY"), Width * 0.5f + 8.0f, ButtonY, ButtonWidth, 54.0f, FLinearColor(0.65f, 0.24f, 0.12f, 1.0f));
	DrawCenteredText(
		FString::Printf(TEXT("THE HOST STARTS WHEN ALL %d PLAYERS ARE READY"), GameState.PlayersPerTeam * 2),
		Width * 0.5f,
		ButtonY + 82.0f,
		MutedTextColor,
		0.64f);
}

void AFlickHUD::DrawPartyFrontend(
	const AFlickGameState& GameState,
	const AFlickPlayerController& Controller,
	const float Width,
	const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.002f, 0.006f, 0.012f, 0.9f));
	DrawCenteredText(TEXT("FLICK PARTY"), Width * 0.5f, Height * 0.1f, FLinearColor::White, 1.8f, true);
	DrawCenteredText(TEXT("CONNECTED TO PARTY LEADER"), Width * 0.5f, Height * 0.17f, FLinearColor(0.0f, 0.76f, 1.0f, 1.0f), 0.72f);

	const float SlotWidth = FMath::Min(620.0f, Width * 0.62f);
	const float SlotGap = 8.0f;
	const float SlotHeight = FMath::Min(
		78.0f,
		(Height * 0.5f - SlotGap * (GameState.PartyMaximumMembers - 1))
			/ FMath::Max(GameState.PartyMaximumMembers, 1));
	const float SlotX = Width * 0.5f - SlotWidth * 0.5f;
	const float SlotStartY = Height * 0.23f;
	for (int32 PartySlot = 0; PartySlot < GameState.PartyMaximumMembers; ++PartySlot)
	{
		const AFlickPlayerState* Member = nullptr;
		for (const APlayerState* PlayerState : GameState.PlayerArray)
		{
			const AFlickPlayerState* Candidate = Cast<AFlickPlayerState>(PlayerState);
			if (Candidate && Candidate->GetPartySlot() == PartySlot)
			{
				Member = Candidate;
				break;
			}
		}
		const float SlotY = SlotStartY + PartySlot * (SlotHeight + SlotGap);
		DrawShowcasePanel(
			SlotX,
			SlotY,
			SlotWidth,
			SlotHeight,
			FLinearColor(0.006f, 0.016f, 0.028f, 0.94f),
			Member && Member->IsPartyLeader()
				? FLinearColor(1.0f, 0.45f, 0.08f, 1.0f)
				: FLinearColor(0.0f, 0.76f, 1.0f, 1.0f),
			10.0f);
		DrawText(
			Member ? Member->GetPlayerName() : TEXT("OPEN PARTY SLOT"),
			Member ? FLinearColor::White : MutedTextColor,
			SlotX + 22.0f,
			SlotY + 13.0f,
			GEngine ? GEngine->GetLargeFont() : nullptr,
			0.72f);
		DrawText(
			Member && Member->IsPartyLeader() ? TEXT("PARTY LEADER") : Member ? TEXT("IN PARTY") : TEXT("WAITING FOR INVITE"),
			MutedTextColor,
			SlotX + 22.0f,
			SlotY + 49.0f,
			GEngine ? GEngine->GetSmallFont() : nullptr,
			0.66f);
	}

	const float ButtonWidth = FMath::Min(270.0f, Width * 0.3f);
	const float ButtonY = Height * 0.79f;
	DrawMenuButton(EFlickMenuAction::LeaveLobby, TEXT("LEAVE PARTY"), Width * 0.5f - ButtonWidth * 0.5f, ButtonY, ButtonWidth, 54.0f, FLinearColor(0.65f, 0.24f, 0.12f, 1.0f));
	DrawCenteredText(TEXT("THE PARTY LEADER IS PREPARING THE NEXT MATCH"), Width * 0.5f, ButtonY + 82.0f, MutedTextColor, 0.64f);
}

void AFlickHUD::DrawPrivateMatchFrontend(
	const AFlickGameState& GameState,
	const AFlickPlayerController& Controller,
	const float Width,
	const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.002f, 0.006f, 0.012f, 0.92f));
	DrawCenteredText(TEXT("PRIVATE MATCH"), Width * 0.5f, Height * 0.06f, FLinearColor::White, 1.7f, true);
	DrawCenteredText(
		FString::Printf(
			TEXT("%dV%d  /  FIRST TO %d  /  ARENA %.0f%%  /  FRICTION %.0f%%  /  POWER %.0f%%"),
			GameState.PrivateMatchSettings.PlayersPerTeam,
			GameState.PrivateMatchSettings.PlayersPerTeam,
			GameState.PrivateMatchSettings.RoundsToWin,
			GameState.PrivateMatchSettings.ArenaScale * 100.0f,
			GameState.PrivateMatchSettings.FrictionScale * 100.0f,
			GameState.PrivateMatchSettings.LaunchSpeedScale * 100.0f),
		Width * 0.5f,
		Height * 0.13f,
		FLinearColor(0.0f, 0.76f, 1.0f, 1.0f),
		0.68f);
	DrawCenteredText(TEXT("CLAIM ANY NUMBER OF PLAYER SLOTS, OR SPECTATE"), Width * 0.5f, Height * 0.18f, MutedTextColor, 0.62f);

	const AFlickPlayerState* LocalState = Controller.GetPlayerState<AFlickPlayerState>();
	auto FindOwner = [&GameState](const EFlickTeam Team, const int32 PlayerSlot) -> const AFlickPlayerState*
	{
		for (const APlayerState* PlayerState : GameState.PlayerArray)
		{
			const AFlickPlayerState* Player = Cast<AFlickPlayerState>(PlayerState);
			if (Player && Player->ControlsPrivateSlot(Team, PlayerSlot))
			{
				return Player;
			}
		}
		return nullptr;
	};

	const float SlotWidth = FMath::Min(410.0f, Width * 0.4f);
	const float SlotHeight = 82.0f;
	const float SlotStartY = Height * 0.25f;
	for (const EFlickTeam Team : {EFlickTeam::Player1, EFlickTeam::Player2})
	{
		const bool bBlue = Team == EFlickTeam::Player1;
		const float SlotX = bBlue ? Width * 0.5f - SlotWidth - 14.0f : Width * 0.5f + 14.0f;
		const FLinearColor Accent = GetTeamColor(Team);
		for (int32 PlayerSlot = 0; PlayerSlot < GameState.PrivateMatchSettings.PlayersPerTeam; ++PlayerSlot)
		{
			const float SlotY = SlotStartY + PlayerSlot * (SlotHeight + 10.0f);
			const AFlickPlayerState* SlotOwner = FindOwner(Team, PlayerSlot);
			const bool bLocalOwner = SlotOwner && SlotOwner == LocalState;
			const FBox2D Bounds(FVector2D(SlotX, SlotY), FVector2D(SlotX + SlotWidth, SlotY + SlotHeight));
			DrawShowcasePanel(
				SlotX,
				SlotY,
				SlotWidth,
				SlotHeight,
				bLocalOwner ? Accent.CopyWithNewOpacity(0.2f) : FLinearColor(0.006f, 0.016f, 0.028f, 0.94f),
				Accent,
				10.0f);
			DrawText(
				FString::Printf(TEXT("%s PLAYER %d"), bBlue ? TEXT("BLUE") : TEXT("ORANGE"), PlayerSlot + 1),
				Accent,
				SlotX + 20.0f,
				SlotY + 12.0f,
				GEngine ? GEngine->GetSmallFont() : nullptr,
				0.68f);
			DrawText(
				SlotOwner ? SlotOwner->GetPlayerName() : TEXT("OPEN - CLICK TO CLAIM"),
				SlotOwner ? FLinearColor::White : MutedTextColor,
				SlotX + 20.0f,
				SlotY + 39.0f,
				GEngine ? GEngine->GetLargeFont() : nullptr,
				0.66f);
			AddMenuHitRegion(EFlickMenuAction::TogglePrivateSlot, Bounds, EFlickMatchVariant::Classic, Team, PlayerSlot);
		}
	}

	const bool bOwnsSlot = LocalState && !LocalState->GetPrivateControlledSlots().IsEmpty();
	const bool bReady = LocalState && LocalState->IsLobbyReady();
	const float ButtonY = Height * 0.72f;
	const float ButtonWidth = FMath::Min(235.0f, Width * 0.25f);
	DrawMenuButton(EFlickMenuAction::PrivateSpectate, TEXT("SPECTATE"), Width * 0.5f - ButtonWidth * 1.55f, ButtonY, ButtonWidth, 54.0f, MutedTextColor);
	if (bOwnsSlot)
	{
		DrawMenuButton(EFlickMenuAction::ToggleLobbyReady, bReady ? TEXT("SET NOT READY") : TEXT("READY UP"), Width * 0.5f - ButtonWidth * 0.5f, ButtonY, ButtonWidth, 54.0f, FLinearColor(0.0f, 0.76f, 1.0f, 1.0f), true);
	}
	DrawMenuButton(EFlickMenuAction::LeaveLobby, TEXT("LEAVE PARTY"), Width * 0.5f + ButtonWidth * 0.55f, ButtonY, ButtonWidth, 54.0f, FLinearColor(0.65f, 0.24f, 0.12f, 1.0f));
	DrawCenteredText(
		bOwnsSlot ? TEXT("READY UP AFTER YOUR SLOT SELECTION IS FINAL") : TEXT("YOU ARE SPECTATING - CLAIM A SLOT TO PLAY"),
		Width * 0.5f,
		ButtonY + 76.0f,
		MutedTextColor,
		0.62f);
}

void AFlickHUD::DrawControls(const float Height)
{
	if (!Canvas)
	{
		return;
	}

	const FString Controls(TEXT("LMB  DRAG + RELEASE     RMB / ESC  CANCEL     R  RESTART"));
	DrawText(Controls, MutedTextColor, 20.0f, Height - 28.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
}

void AFlickHUD::DrawTeamBlock(
	const EFlickTeam Team,
	const int32 ActivePieces,
	const int32 StartingPieces,
	const int32 ShotsTaken,
	const int32 RoundsWon,
	const int32 RoundsToWin,
	const float X,
	const float Y,
	const float Width,
	const bool bCurrent)
{
	const FLinearColor TeamColor = GetTeamColor(Team);
	const bool bBobMode = StartingPieces > 4;
	DrawShowcasePanel(X, Y, Width, 176.0f, PanelColor, TeamColor.CopyWithNewOpacity(bCurrent ? 0.92f : 0.76f), 14.0f);

	DrawText(
		FString::Printf(TEXT("PLAYER %d"), GetTeamNumber(Team)),
		TeamColor,
		X + 62.0f,
		Y + 45.0f,
		GEngine ? GEngine->GetLargeFont() : nullptr,
		0.72f);
	DrawText(
		bBobMode
			? FString::Printf(TEXT("POCKETED  %d / %d"), StartingPieces - ActivePieces, StartingPieces)
			: FString::Printf(TEXT("ROUNDS  %d / %d"), RoundsWon, RoundsToWin),
		MutedTextColor,
		X + 62.0f,
		Y + 120.0f,
		GEngine ? GEngine->GetSmallFont() : nullptr,
		0.62f);

	if (!bBobMode)
	{
		for (int32 Index = 0; Index < RoundsToWin; ++Index)
		{
			DrawCircle(
				FVector2D(X + 70.0f + Index * 24.0f, Y + 89.0f),
				8.0f,
				Index < RoundsWon ? TeamColor : FLinearColor(TeamColor.R, TeamColor.G, TeamColor.B, 0.48f),
				24,
				Index < RoundsWon ? 3.0f : 1.5f);
		}
	}

	const float CountCenterX = X + Width - 80.0f;
	DrawCenteredText(FString::FromInt(ActivePieces), CountCenterX, Y + 51.0f, FLinearColor::White, 1.65f, true);
	DrawCenteredText(bBobMode ? TEXT("LEFT") : TEXT("ACTIVE"), CountCenterX, Y + 111.0f, MutedTextColor, 0.58f);
	(void)ShotsTaken;
}

void AFlickHUD::DrawCenteredText(
	const FString& Text,
	const float CenterX,
	const float Y,
	const FLinearColor& Color,
	const float Scale,
	const bool bLargeFont)
{
	UFont* Font = GEngine
		? (bLargeFont ? GEngine->GetLargeFont() : GEngine->GetSmallFont())
		: nullptr;
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	GetTextSize(Text, TextWidth, TextHeight, Font, Scale);
	DrawText(Text, Color, CenterX - TextWidth * 0.5f, Y, Font, Scale);
}

void AFlickHUD::DrawCircle(
	const FVector2D& Center,
	const float Radius,
	const FLinearColor& Color,
	const int32 Segments,
	const float Thickness)
{
	const int32 SafeSegments = FMath::Max(Segments, 8);
	FVector2D Previous = Center + FVector2D(Radius, 0.0f);
	for (int32 Segment = 1; Segment <= SafeSegments; ++Segment)
	{
		const float Angle = 2.0f * PI * static_cast<float>(Segment) / SafeSegments;
		const FVector2D Current = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		DrawLine(Previous.X, Previous.Y, Current.X, Current.Y, Color, Thickness);
		Previous = Current;
	}
}

void AFlickHUD::DrawFilledTriangle(
	const FVector2D& A,
	const FVector2D& B,
	const FVector2D& C,
	const FLinearColor& Color)
{
	if (!Canvas)
	{
		return;
	}
	FCanvasTriangleItem Triangle(A, B, C, GWhiteTexture);
	Triangle.SetColor(Color);
	Triangle.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Triangle);
}

void AFlickHUD::DrawPanel(
	const float X,
	const float Y,
	const float Width,
	const float Height,
	const FLinearColor& Color)
{
	if (Width > 0.0f && Height > 0.0f)
	{
		DrawRect(Color, X, Y, Width, Height);
	}
}

void AFlickHUD::DrawShowcasePanel(
	const float X,
	const float Y,
	const float Width,
	const float Height,
	const FLinearColor& Background,
	const FLinearColor& Accent,
	const float CutSize)
{
	if (Width <= 0.0f || Height <= 0.0f)
	{
		return;
	}

	DrawPanel(X, Y, Width, Height, Background);
	const float Cut = FMath::Clamp(CutSize, 0.0f, FMath::Min(Width, Height) * 0.24f);
	const TArray<FVector2D> Points = {
		FVector2D(X + Cut, Y), FVector2D(X + Width - Cut, Y),
		FVector2D(X + Width, Y + Cut), FVector2D(X + Width, Y + Height - Cut),
		FVector2D(X + Width - Cut, Y + Height), FVector2D(X + Cut, Y + Height),
		FVector2D(X, Y + Height - Cut), FVector2D(X, Y + Cut)};
	const FLinearColor Neutral(0.34f, 0.43f, 0.54f, FMath::Min(Background.A + 0.08f, 0.82f));
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FVector2D& Start = Points[Index];
		const FVector2D& End = Points[(Index + 1) % Points.Num()];
		DrawLine(Start.X, Start.Y, End.X, End.Y, Neutral, 1.0f);
	}
	for (int32 Index = 4; Index < 7; ++Index)
	{
		const FVector2D& Start = Points[Index];
		const FVector2D& End = Points[Index + 1];
		DrawLine(Start.X, Start.Y, End.X, End.Y, Accent, 1.35f);
	}
}

FLinearColor AFlickHUD::GetPowerColor(const float Power) const
{
	const float SafePower = FMath::Clamp(Power, 0.0f, 1.0f);
	if (SafePower < 0.65f)
	{
		return FLinearColor::LerpUsingHSV(
			FLinearColor(0.1f, 0.92f, 0.52f, 1.0f),
			FLinearColor(1.0f, 0.82f, 0.12f, 1.0f),
			SafePower / 0.65f);
	}

	return FLinearColor::LerpUsingHSV(
		FLinearColor(1.0f, 0.82f, 0.12f, 1.0f),
		FLinearColor(1.0f, 0.18f, 0.06f, 1.0f),
		(SafePower - 0.65f) / 0.35f);
}

bool AFlickHUD::HandleMenuClick(const FVector2D& ScreenPosition)
{
	AFlickGameMode* FlickGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr;
	AFlickPlayerController* FlickController = Cast<AFlickPlayerController>(GetOwningPlayerController());

	for (int32 Index = MenuHitRegions.Num() - 1; Index >= 0; --Index)
	{
		const FFlickMenuHitRegion& Region = MenuHitRegions[Index];
		if (ScreenPosition.X < Region.Bounds.Min.X
			|| ScreenPosition.X > Region.Bounds.Max.X
			|| ScreenPosition.Y < Region.Bounds.Min.Y
			|| ScreenPosition.Y > Region.Bounds.Max.Y)
		{
			continue;
		}
		if (!FlickGameMode
			&& Region.Action != EFlickMenuAction::Restart
			&& Region.Action != EFlickMenuAction::NextRound
			&& Region.Action != EFlickMenuAction::MainMenu
			&& Region.Action != EFlickMenuAction::ToggleLobbyReady
			&& Region.Action != EFlickMenuAction::LeaveLobby
			&& Region.Action != EFlickMenuAction::TogglePrivateSlot
			&& Region.Action != EFlickMenuAction::PrivateSpectate)
		{
			return false;
		}

		switch (Region.Action)
		{
		case EFlickMenuAction::Restart:
			if (FlickGameMode)
			{
				FlickGameMode->RestartMatch();
			}
			else if (FlickController)
			{
				FlickController->RequestRestartMatch();
			}
			break;
		case EFlickMenuAction::NextRound:
			if (FlickGameMode)
			{
				FlickGameMode->StartNextRound();
			}
			else if (FlickController)
			{
				FlickController->RequestNextRound();
			}
			break;
		case EFlickMenuAction::OpenLoadout:
			if (!FlickGameMode) return false;
			FlickGameMode->OpenLoadout();
			break;
		case EFlickMenuAction::StartMatch:
			FlickGameMode->StartSelectedMatch();
			break;
		case EFlickMenuAction::SelectMode:
			FlickGameMode->SelectMatchVariant(Region.MatchVariant);
			break;
		case EFlickMenuAction::OpenSettings:
			FlickGameMode->OpenSettings();
			break;
		case EFlickMenuAction::Quit:
			FlickGameMode->QuitGame();
			break;
		case EFlickMenuAction::Resume:
			FlickGameMode->TogglePauseMenu();
			break;
		case EFlickMenuAction::EditLoadout:
			FlickGameMode->ReturnToLoadout();
			break;
		case EFlickMenuAction::MainMenu:
			if (FlickGameMode) FlickGameMode->ReturnToMainMenu(); else if (FlickController) FlickController->LeaveNetworkSession();
			break;
		case EFlickMenuAction::ToggleLobbyReady:
			if (FlickController) FlickController->ToggleLobbyReady();
			break;
		case EFlickMenuAction::LeaveLobby:
			if (FlickController) FlickController->LeaveNetworkSession();
			break;
		case EFlickMenuAction::TogglePrivateSlot:
			if (FlickController) FlickController->RequestTogglePrivateMatchSlot(Region.Team, Region.PieceSlot);
			break;
		case EFlickMenuAction::PrivateSpectate:
			if (FlickController) FlickController->RequestPrivateMatchSpectate();
			break;
		case EFlickMenuAction::Back:
			if (FlickGameMode->GetFrontendScreen() == EFlickFrontendScreen::Loadout)
			{
				FlickGameMode->CloseLoadout();
			}
			else
			{
				FlickGameMode->CloseSettings();
			}
			break;
		case EFlickMenuAction::ToggleAimGuide:
			FlickGameMode->SetAimGuideEnabled(!FlickGameMode->IsAimGuideEnabled());
			break;
		case EFlickMenuAction::ToggleWorldEffects:
			FlickGameMode->SetImpactEffectsEnabled(!FlickGameMode->AreImpactEffectsEnabled());
			break;
		case EFlickMenuAction::SetCameraShake:
		{
			const float SliderValue = FMath::Clamp(
				(ScreenPosition.X - Region.Bounds.Min.X)
				/ FMath::Max(Region.Bounds.Max.X - Region.Bounds.Min.X, 1.0f),
				0.0f,
				1.0f);
			FlickGameMode->SetCameraShakeIntensity(SliderValue);
			break;
		}
		case EFlickMenuAction::SetMasterVolume:
		case EFlickMenuAction::SetEffectsVolume:
		case EFlickMenuAction::SetInterfaceVolume:
		{
			const float SliderValue = FMath::Clamp(
				(ScreenPosition.X - Region.Bounds.Min.X)
				/ FMath::Max(Region.Bounds.Max.X - Region.Bounds.Min.X, 1.0f),
				0.0f,
				1.0f);
			if (Region.Action == EFlickMenuAction::SetMasterVolume)
			{
				FlickGameMode->SetMasterVolume(SliderValue);
			}
			else if (Region.Action == EFlickMenuAction::SetEffectsVolume)
			{
				FlickGameMode->SetEffectsVolume(SliderValue);
			}
			else
			{
				FlickGameMode->SetInterfaceVolume(SliderValue);
			}
			break;
		}
		case EFlickMenuAction::ToggleVSync:
			FlickGameMode->ToggleVSync();
			break;
		case EFlickMenuAction::PreviousWindowMode:
			FlickGameMode->CycleWindowMode(-1);
			break;
		case EFlickMenuAction::NextWindowMode:
			FlickGameMode->CycleWindowMode(1);
			break;
		case EFlickMenuAction::PreviousResolution:
			FlickGameMode->CycleResolution(-1);
			break;
		case EFlickMenuAction::NextResolution:
			FlickGameMode->CycleResolution(1);
			break;
		case EFlickMenuAction::ApplyDisplay:
			FlickGameMode->ApplyDisplaySettings();
			break;
		case EFlickMenuAction::PreviousLoadoutPiece:
			FlickGameMode->CycleLoadoutPiece(Region.Team, Region.PieceSlot, -1);
			break;
		case EFlickMenuAction::NextLoadoutPiece:
			FlickGameMode->CycleLoadoutPiece(Region.Team, Region.PieceSlot, 1);
			break;
		case EFlickMenuAction::None:
		default:
			return false;
		}
		const bool bConfirmSound = Region.Action == EFlickMenuAction::OpenLoadout
			|| Region.Action == EFlickMenuAction::StartMatch
			|| Region.Action == EFlickMenuAction::Resume
			|| Region.Action == EFlickMenuAction::Restart
			|| Region.Action == EFlickMenuAction::NextRound
			|| Region.Action == EFlickMenuAction::EditLoadout
			|| Region.Action == EFlickMenuAction::ApplyDisplay;
		if (FlickGameMode)
		{
			FlickGameMode->PlayMenuSound(bConfirmSound);
		}
		return true;
	}

	return false;
}

void AFlickHUD::DrawMainMenu(const AFlickGameMode& GameMode, const float Width, const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.005f, 0.008f, 0.013f, 0.82f));
	const float Margin = FMath::Max(54.0f, Width * 0.055f);
	const float LeftWidth = FMath::Min(400.0f, Width * 0.34f);
	const float RightX = FMath::Max(Width * 0.48f, Margin + LeftWidth + 42.0f);
	const float RightWidth = Width - RightX - Margin;
	const FLinearColor SelectedColor = GetModeColor(GameMode.GetSelectedMatchVariant());

	DrawPanel(Margin, 76.0f, 7.0f, 190.0f, SelectedColor);
	DrawText(TEXT("FLICK"), FLinearColor::White, Margin + 28.0f, 74.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 3.1f);
	DrawText(TEXT("TABLETOP KNOCKOUT"), MutedTextColor, Margin + 31.0f, 176.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.9f);
	DrawText(
		GetMatchVariantName(GameMode.GetSelectedMatchVariant()),
		SelectedColor,
		Margin + 31.0f,
		226.0f,
		GEngine ? GEngine->GetLargeFont() : nullptr,
		1.05f);
	DrawText(
		GetMatchVariantSummary(GameMode.GetSelectedMatchVariant()),
		MutedTextColor,
		Margin + 31.0f,
		262.0f,
		GEngine ? GEngine->GetSmallFont() : nullptr,
		0.68f);

	const float ButtonY = Height - 238.0f;
	DrawMenuButton(EFlickMenuAction::OpenLoadout, TEXT("BUILD LINEUPS"), Margin, ButtonY, LeftWidth, 56.0f, SelectedColor, true);
	DrawMenuButton(EFlickMenuAction::OpenSettings, TEXT("SETTINGS"), Margin, ButtonY + 68.0f, LeftWidth, 48.0f, FLinearColor(0.38f, 0.43f, 0.48f, 1.0f));
	DrawMenuButton(EFlickMenuAction::Quit, TEXT("QUIT"), Margin, ButtonY + 128.0f, LeftWidth, 48.0f, FLinearColor(0.52f, 0.25f, 0.24f, 1.0f));

	DrawPanel(RightX - 24.0f, 56.0f, RightWidth + 48.0f, Height - 112.0f, FLinearColor(0.025f, 0.032f, 0.043f, 0.82f));
	DrawText(TEXT("SELECT MODE"), FLinearColor::White, RightX, 82.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.0f);
	DrawText(TEXT("FOUR PUCKS PER SIDE  |  BOB OBJECTIVE MODE"), MutedTextColor, RightX, 116.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);

	const float CardHeight = 104.0f;
	DrawModeCard(GameMode, EFlickMatchVariant::Classic, RightX, 190.0f, RightWidth, CardHeight);
	DrawModeCard(GameMode, EFlickMatchVariant::Bob, RightX, 326.0f, RightWidth, CardHeight);

	DrawText(TEXT("MOUSE"), SelectedColor, RightX, Height - 86.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
	DrawText(TEXT("SELECT  /  PULL  /  RELEASE"), MutedTextColor, RightX + 72.0f, Height - 86.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
}

void AFlickHUD::DrawLoadoutMenu(const AFlickGameMode& GameMode, const float Width, const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.004f, 0.007f, 0.011f, 0.9f));
	const float PanelWidth = FMath::Min(1120.0f, Width - 64.0f);
	const float PanelHeight = FMath::Min(640.0f, Height - 48.0f);
	const float X = (Width - PanelWidth) * 0.5f;
	const float Y = (Height - PanelHeight) * 0.5f;
	const float Accent = 5.0f;
	const FLinearColor ModeColor = GetModeColor(GameMode.GetSelectedMatchVariant());
	const int32 PieceCount = GameMode.GetCurrentStartingPiecesPerTeam();

	DrawPanel(X, Y, PanelWidth, PanelHeight, FLinearColor(0.018f, 0.024f, 0.033f, 0.98f));
	DrawPanel(X, Y, PanelWidth, Accent, ModeColor);
	DrawText(TEXT("BUILD YOUR LINEUP"), FLinearColor::White, X + 36.0f, Y + 26.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.55f);
	DrawText(
		FString::Printf(TEXT("%s  |  %d %s EACH"), *GetMatchVariantName(GameMode.GetSelectedMatchVariant()), PieceCount, PieceCount == 1 ? TEXT("PUCK") : TEXT("PUCKS")),
		ModeColor,
		X + 38.0f,
		Y + 76.0f,
		GEngine ? GEngine->GetSmallFont() : nullptr,
		0.78f);

	const float ContentX = X + 36.0f;
	const float Gap = 28.0f;
	const float ColumnWidth = (PanelWidth - 72.0f - Gap) * 0.5f;
	const float Player2X = ContentX + ColumnWidth + Gap;
	DrawText(TEXT("PLAYER 1"), GetTeamColor(EFlickTeam::Player1), ContentX, Y + 112.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 0.92f);
	DrawText(TEXT("PLAYER 2"), GetTeamColor(EFlickTeam::Player2), Player2X, Y + 112.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 0.92f);

	const float RowStartY = Y + 148.0f;
	constexpr float RowHeight = 66.0f;
	constexpr float RowStep = 76.0f;
	for (int32 SlotIndex = 0; SlotIndex < PieceCount; ++SlotIndex)
	{
		const float RowY = RowStartY + SlotIndex * RowStep;
		DrawLoadoutSlot(GameMode, EFlickTeam::Player1, SlotIndex, ContentX, RowY, ColumnWidth, RowHeight);
		DrawLoadoutSlot(GameMode, EFlickTeam::Player2, SlotIndex, Player2X, RowY, ColumnWidth, RowHeight);
	}

	DrawText(TEXT("PUCK CLASSES"), MutedTextColor, ContentX, Y + PanelHeight - 228.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.65f);
	DrawArchetypeReference(ContentX, Y + PanelHeight - 204.0f, PanelWidth - 72.0f);

	const float ButtonY = Y + PanelHeight - 68.0f;
	DrawMenuButton(EFlickMenuAction::Back, TEXT("BACK"), ContentX, ButtonY, 180.0f, 44.0f, FLinearColor(0.4f, 0.45f, 0.5f, 1.0f));
	DrawMenuButton(
		EFlickMenuAction::StartMatch,
		FString::Printf(TEXT("START %s"), *GetMatchVariantName(GameMode.GetSelectedMatchVariant())),
		X + PanelWidth - 256.0f,
		ButtonY,
		220.0f,
		44.0f,
		ModeColor,
		true);
}

void AFlickHUD::DrawLoadoutSlot(
	const AFlickGameMode& GameMode,
	const EFlickTeam Team,
	const int32 SlotIndex,
	const float X,
	const float Y,
	const float Width,
	const float Height)
{
	const EFlickPieceArchetype Archetype = GameMode.GetLoadoutPiece(Team, SlotIndex);
	const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
	const FLinearColor TeamColor = GetTeamColor(Team);
	const FLinearColor AccentColor = Rules.AccentColor;
	const FBox2D PreviousBounds(FVector2D(X + 50.0f, Y + 14.0f), FVector2D(X + 84.0f, Y + 52.0f));
	const FBox2D NextBounds(FVector2D(X + Width - 42.0f, Y + 14.0f), FVector2D(X + Width - 8.0f, Y + 52.0f));

	DrawPanel(X, Y, Width, Height, FLinearColor(0.045f, 0.055f, 0.07f, 0.96f));
	DrawPanel(X, Y, 4.0f, Height, AccentColor);
	DrawCircle(FVector2D(X + 27.0f, Y + Height * 0.5f), 15.0f, TeamColor, 24, 4.0f);
	DrawCenteredText(GetPieceArchetypeMark(Archetype), X + 27.0f, Y + 23.0f, AccentColor, 0.68f, true);

	AddMenuHitRegion(EFlickMenuAction::PreviousLoadoutPiece, PreviousBounds, EFlickMatchVariant::Classic, Team, SlotIndex);
	AddMenuHitRegion(EFlickMenuAction::NextLoadoutPiece, NextBounds, EFlickMatchVariant::Classic, Team, SlotIndex);
	DrawPanel(PreviousBounds.Min.X, PreviousBounds.Min.Y, 34.0f, 38.0f, IsMenuRegionHovered(PreviousBounds) ? TeamColor : FLinearColor(0.12f, 0.14f, 0.17f, 1.0f));
	DrawPanel(NextBounds.Min.X, NextBounds.Min.Y, 34.0f, 38.0f, IsMenuRegionHovered(NextBounds) ? TeamColor : FLinearColor(0.12f, 0.14f, 0.17f, 1.0f));
	DrawCenteredText(TEXT("<"), PreviousBounds.Min.X + 17.0f, PreviousBounds.Min.Y + 8.0f, FLinearColor::White, 0.75f, true);
	DrawCenteredText(TEXT(">"), NextBounds.Min.X + 17.0f, NextBounds.Min.Y + 8.0f, FLinearColor::White, 0.75f, true);

	const float TextX = X + 100.0f;
	DrawText(GetPieceArchetypeName(Archetype), AccentColor, TextX, Y + 10.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 0.8f);
	DrawText(Rules.Summary, MutedTextColor, TextX, Y + 37.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.6f);
}

void AFlickHUD::DrawArchetypeReference(const float X, const float Y, const float Width)
{
	constexpr int32 ColumnCount = 3;
	constexpr float Gap = 6.0f;
	constexpr float ItemHeight = 38.0f;
	const float ItemWidth = (Width - Gap * (ColumnCount - 1)) / ColumnCount;
	for (int32 Index = 0; Index < FlickPieceArchetypeRules::ArchetypeCount; ++Index)
	{
		const EFlickPieceArchetype Archetype = static_cast<EFlickPieceArchetype>(Index);
		const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
		const int32 Column = Index % ColumnCount;
		const int32 Row = Index / ColumnCount;
		const float ItemX = X + Column * (ItemWidth + Gap);
		const float ItemY = Y + Row * (ItemHeight + Gap);
		DrawPanel(ItemX, ItemY, ItemWidth, ItemHeight, FLinearColor(0.038f, 0.047f, 0.06f, 0.96f));
		DrawPanel(ItemX, ItemY, 3.0f, ItemHeight, Rules.AccentColor);
		DrawText(GetPieceArchetypeName(Archetype), Rules.AccentColor, ItemX + 10.0f, ItemY + 5.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 0.55f);
		DrawText(Rules.ClassLabel, MutedTextColor, ItemX + 10.0f, ItemY + 22.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.42f);
	}
}

void AFlickHUD::DrawPauseMenu(const AFlickGameMode& GameMode, const float Width, const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.004f, 0.006f, 0.01f, 0.72f));
	const float PanelWidth = FMath::Min(440.0f, Width - 80.0f);
	const float PanelHeight = FMath::Min(530.0f, Height - 90.0f);
	const float X = (Width - PanelWidth) * 0.5f;
	const float Y = (Height - PanelHeight) * 0.5f;
	const FLinearColor Accent = GetModeColor(GameMode.GetSelectedMatchVariant());

	DrawPanel(X, Y, PanelWidth, PanelHeight, FLinearColor(0.018f, 0.024f, 0.033f, 0.97f));
	DrawPanel(X, Y, PanelWidth, 5.0f, Accent);
	DrawCenteredText(TEXT("PAUSED"), Width * 0.5f, Y + 42.0f, FLinearColor::White, 1.7f, true);
	const AFlickGameState* GameState = GameMode.GetFlickGameState();
	DrawCenteredText(
		GameState
			? FString::Printf(
				TEXT("%s  |  ROUND %d  |  %d-%d"),
				*GetMatchVariantName(GameMode.GetSelectedMatchVariant()),
				GameState->RoundNumber,
				GameState->Player1RoundsWon,
				GameState->Player2RoundsWon)
			: GetMatchVariantName(GameMode.GetSelectedMatchVariant()),
		Width * 0.5f,
		Y + 94.0f,
		Accent,
		0.72f);

	const float ButtonX = X + 54.0f;
	const float ButtonWidth = PanelWidth - 108.0f;
	DrawMenuButton(EFlickMenuAction::Resume, TEXT("RESUME"), ButtonX, Y + 145.0f, ButtonWidth, 52.0f, Accent, true);
	DrawMenuButton(EFlickMenuAction::Restart, TEXT("RESTART MATCH"), ButtonX, Y + 211.0f, ButtonWidth, 48.0f, FLinearColor(0.42f, 0.47f, 0.52f, 1.0f));
	DrawMenuButton(EFlickMenuAction::OpenSettings, TEXT("SETTINGS"), ButtonX, Y + 273.0f, ButtonWidth, 48.0f, FLinearColor(0.42f, 0.47f, 0.52f, 1.0f));
	DrawMenuButton(EFlickMenuAction::MainMenu, TEXT("MAIN MENU"), ButtonX, Y + 335.0f, ButtonWidth, 48.0f, FLinearColor(0.52f, 0.25f, 0.24f, 1.0f));
	DrawCenteredText(TEXT("ESC  RESUME"), Width * 0.5f, Y + PanelHeight - 42.0f, MutedTextColor, 0.66f);
}

void AFlickHUD::DrawSettingsMenu(const AFlickGameMode& GameMode, const float Width, const float Height)
{
	DrawPanel(0.0f, 0.0f, Width, Height, FLinearColor(0.003f, 0.006f, 0.01f, 0.9f));
	const float PanelWidth = FMath::Min(1040.0f, Width - 56.0f);
	const float PanelHeight = FMath::Min(650.0f, Height - 40.0f);
	const float X = (Width - PanelWidth) * 0.5f;
	const float Y = (Height - PanelHeight) * 0.5f;
	const FLinearColor Accent(0.08f, 0.78f, 0.88f, 1.0f);
	const float ColumnGap = 42.0f;
	const float ColumnWidth = (PanelWidth - 138.0f) * 0.5f;
	const float LeftX = X + 48.0f;
	const float RightX = LeftX + ColumnWidth + ColumnGap;

	DrawPanel(X, Y, PanelWidth, PanelHeight, FLinearColor(0.018f, 0.024f, 0.033f, 0.98f));
	DrawPanel(X, Y, PanelWidth, 5.0f, Accent);
	DrawText(TEXT("SETTINGS"), FLinearColor::White, LeftX, Y + 24.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.35f);
	DrawText(TEXT("GAMEPLAY"), Accent, LeftX, Y + 82.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
	DrawToggleRow(EFlickMenuAction::ToggleAimGuide, TEXT("AIM + CONTACT GUIDE"), GameMode.IsAimGuideEnabled(), LeftX, Y + 105.0f, ColumnWidth);
	DrawToggleRow(EFlickMenuAction::ToggleWorldEffects, TEXT("WORLD IMPACT FX"), GameMode.AreImpactEffectsEnabled(), LeftX, Y + 157.0f, ColumnWidth);
	DrawSliderRow(EFlickMenuAction::SetCameraShake, TEXT("CAMERA SHAKE"), GameMode.GetCameraShakeIntensity(), LeftX, Y + 209.0f, ColumnWidth);

	DrawText(TEXT("AUDIO MIX"), Accent, LeftX, Y + 280.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
	DrawSliderRow(EFlickMenuAction::SetMasterVolume, TEXT("MASTER"), GameMode.GetMasterVolume(), LeftX, Y + 303.0f, ColumnWidth);
	DrawSliderRow(EFlickMenuAction::SetEffectsVolume, TEXT("PHYSICS FX"), GameMode.GetEffectsVolume(), LeftX, Y + 355.0f, ColumnWidth);
	DrawSliderRow(EFlickMenuAction::SetInterfaceVolume, TEXT("INTERFACE"), GameMode.GetInterfaceVolume(), LeftX, Y + 407.0f, ColumnWidth);

	DrawText(TEXT("DISPLAY"), Accent, RightX, Y + 82.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
	DrawToggleRow(EFlickMenuAction::ToggleVSync, TEXT("V-SYNC"), GameMode.IsVSyncEnabled(), RightX, Y + 105.0f, ColumnWidth);
	DrawCycleRow(
		EFlickMenuAction::PreviousWindowMode,
		EFlickMenuAction::NextWindowMode,
		TEXT("WINDOW MODE"),
		GameMode.GetWindowModeLabel(),
		RightX,
		Y + 157.0f,
		ColumnWidth);
	DrawCycleRow(
		EFlickMenuAction::PreviousResolution,
		EFlickMenuAction::NextResolution,
		TEXT("RESOLUTION"),
		GameMode.GetResolutionLabel(),
		RightX,
		Y + 209.0f,
		ColumnWidth);

	DrawPanel(RightX, Y + 292.0f, ColumnWidth, 160.0f, FLinearColor(0.032f, 0.041f, 0.054f, 0.94f));
	DrawPanel(RightX, Y + 292.0f, 4.0f, 160.0f, GetModeColor(GameMode.GetSelectedMatchVariant()));
	DrawText(TEXT("MATCH FORMAT"), MutedTextColor, RightX + 20.0f, Y + 314.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.68f);
	DrawText(GameMode.IsBobMode() ? TEXT("ONE BOARD") : TEXT("BEST OF FIVE"), FLinearColor::White, RightX + 20.0f, Y + 347.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 1.0f);
	DrawText(GameMode.IsBobMode() ? TEXT("STANDARD PUCKS ONLY") : TEXT("FIRST TO 3 ROUNDS"), GetModeColor(GameMode.GetSelectedMatchVariant()), RightX + 20.0f, Y + 386.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.72f);
	DrawText(TEXT("OPENING PLAYER ALTERNATES"), MutedTextColor, RightX + 20.0f, Y + 417.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.62f);

	const float ButtonY = Y + PanelHeight - 69.0f;
	DrawMenuButton(EFlickMenuAction::Back, TEXT("BACK"), LeftX, ButtonY, 180.0f, 44.0f, FLinearColor(0.4f, 0.45f, 0.5f, 1.0f));
	DrawMenuButton(EFlickMenuAction::ApplyDisplay, TEXT("APPLY DISPLAY"), X + PanelWidth - 254.0f, ButtonY, 200.0f, 44.0f, Accent, true);
}

void AFlickHUD::DrawMenuButton(
	const EFlickMenuAction Action,
	const FString& Label,
	const float X,
	const float Y,
	const float Width,
	const float Height,
	const FLinearColor& Accent,
	const bool bPrimary)
{
	const FBox2D Bounds(FVector2D(X, Y), FVector2D(X + Width, Y + Height));
	AddMenuHitRegion(Action, Bounds);
	const bool bHovered = IsMenuRegionHovered(Bounds);
	const FLinearColor Background = bHovered
		? FMath::Lerp(FLinearColor(0.08f, 0.1f, 0.13f, 0.96f), Accent, bPrimary ? 0.72f : 0.28f)
		: (bPrimary ? FMath::Lerp(FLinearColor(0.035f, 0.045f, 0.06f, 0.96f), Accent, 0.46f) : FLinearColor(0.055f, 0.066f, 0.082f, 0.94f));
	DrawPanel(X, Y, Width, Height, Background);
	DrawPanel(X, Y, bHovered || bPrimary ? 5.0f : 2.0f, Height, Accent);
	DrawCenteredText(Label, X + Width * 0.5f, Y + Height * 0.27f, FLinearColor::White, bPrimary ? 0.92f : 0.8f, bPrimary);
}

void AFlickHUD::DrawModeCard(
	const AFlickGameMode& GameMode,
	const EFlickMatchVariant Variant,
	const float X,
	const float Y,
	const float Width,
	const float Height)
{
	const FBox2D Bounds(FVector2D(X, Y), FVector2D(X + Width, Y + Height));
	AddMenuHitRegion(EFlickMenuAction::SelectMode, Bounds, Variant);
	const bool bSelected = GameMode.GetSelectedMatchVariant() == Variant;
	const bool bHovered = IsMenuRegionHovered(Bounds);
	const FLinearColor Accent = GetModeColor(Variant);
	DrawPanel(
		X,
		Y,
		Width,
		Height,
		bSelected || bHovered ? FLinearColor(0.075f, 0.09f, 0.11f, 0.98f) : FLinearColor(0.042f, 0.052f, 0.068f, 0.94f));
	DrawPanel(X, Y, bSelected ? 7.0f : 3.0f, Height, Accent);
	DrawText(GetMatchVariantName(Variant), bSelected ? Accent : FLinearColor::White, X + 24.0f, Y + 18.0f, GEngine ? GEngine->GetLargeFont() : nullptr, 0.95f);
	DrawText(GetMatchVariantSummary(Variant), MutedTextColor, X + 24.0f, Y + 58.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.66f);
	if (bSelected)
	{
		DrawCircle(FVector2D(X + Width - 32.0f, Y + Height * 0.5f), 10.0f, Accent, 24, 3.0f);
		DrawCircle(FVector2D(X + Width - 32.0f, Y + Height * 0.5f), 3.0f, FLinearColor::White, 16, 2.0f);
	}
}

void AFlickHUD::DrawToggleRow(
	const EFlickMenuAction Action,
	const FString& Label,
	const bool bEnabled,
	const float X,
	const float Y,
	const float Width)
{
	DrawText(Label, FLinearColor::White, X, Y + 13.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.78f);
	DrawPanel(X, Y + 51.0f, Width, 1.0f, FLinearColor(0.16f, 0.18f, 0.21f, 0.8f));
	const float ToggleWidth = 94.0f;
	const FBox2D Bounds(FVector2D(X + Width - ToggleWidth, Y + 8.0f), FVector2D(X + Width, Y + 42.0f));
	AddMenuHitRegion(Action, Bounds);
	const FLinearColor Accent = bEnabled ? FLinearColor(0.08f, 0.78f, 0.88f, 1.0f) : FLinearColor(0.23f, 0.25f, 0.28f, 1.0f);
	DrawPanel(Bounds.Min.X, Bounds.Min.Y, ToggleWidth, 34.0f, IsMenuRegionHovered(Bounds) ? FMath::Lerp(Accent, FLinearColor::White, 0.18f) : Accent);
	DrawCenteredText(bEnabled ? TEXT("ON") : TEXT("OFF"), Bounds.Min.X + ToggleWidth * 0.5f, Bounds.Min.Y + 8.0f, FLinearColor::White, 0.7f);
}

void AFlickHUD::DrawSliderRow(
	const EFlickMenuAction Action,
	const FString& Label,
	const float Value,
	const float X,
	const float Y,
	const float Width)
{
	const float SafeValue = FMath::Clamp(Value, 0.0f, 1.0f);
	DrawText(Label, FLinearColor::White, X, Y + 13.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.78f);
	DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(SafeValue * 100.0f)), MutedTextColor, X + Width - 344.0f, Y + 13.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.7f);
	DrawPanel(X, Y + 51.0f, Width, 1.0f, FLinearColor(0.16f, 0.18f, 0.21f, 0.8f));
	const float SliderWidth = 260.0f;
	const float SliderX = X + Width - SliderWidth;
	const FBox2D Bounds(FVector2D(SliderX, Y + 10.0f), FVector2D(SliderX + SliderWidth, Y + 42.0f));
	AddMenuHitRegion(Action, Bounds);
	DrawPanel(SliderX, Y + 22.0f, SliderWidth, 8.0f, FLinearColor(0.16f, 0.18f, 0.21f, 1.0f));
	DrawPanel(SliderX, Y + 22.0f, SliderWidth * SafeValue, 8.0f, FLinearColor(0.08f, 0.78f, 0.88f, 1.0f));
	DrawCircle(FVector2D(SliderX + SliderWidth * SafeValue, Y + 26.0f), IsMenuRegionHovered(Bounds) ? 9.0f : 7.0f, FLinearColor::White, 20, 3.0f);
}

void AFlickHUD::DrawCycleRow(
	const EFlickMenuAction PreviousAction,
	const EFlickMenuAction NextAction,
	const FString& Label,
	const FString& Value,
	const float X,
	const float Y,
	const float Width)
{
	DrawText(Label, FLinearColor::White, X, Y + 13.0f, GEngine ? GEngine->GetSmallFont() : nullptr, 0.78f);
	DrawPanel(X, Y + 51.0f, Width, 1.0f, FLinearColor(0.16f, 0.18f, 0.21f, 0.8f));
	const float ControlX = X + Width - 300.0f;
	const FBox2D PreviousBounds(FVector2D(ControlX, Y + 7.0f), FVector2D(ControlX + 42.0f, Y + 43.0f));
	const FBox2D NextBounds(FVector2D(ControlX + 258.0f, Y + 7.0f), FVector2D(ControlX + 300.0f, Y + 43.0f));
	AddMenuHitRegion(PreviousAction, PreviousBounds);
	AddMenuHitRegion(NextAction, NextBounds);
	DrawPanel(PreviousBounds.Min.X, PreviousBounds.Min.Y, 42.0f, 36.0f, IsMenuRegionHovered(PreviousBounds) ? FLinearColor(0.12f, 0.72f, 0.82f, 1.0f) : FLinearColor(0.18f, 0.21f, 0.25f, 1.0f));
	DrawPanel(NextBounds.Min.X, NextBounds.Min.Y, 42.0f, 36.0f, IsMenuRegionHovered(NextBounds) ? FLinearColor(0.12f, 0.72f, 0.82f, 1.0f) : FLinearColor(0.18f, 0.21f, 0.25f, 1.0f));
	DrawCenteredText(TEXT("<"), PreviousBounds.Min.X + 21.0f, PreviousBounds.Min.Y + 8.0f, FLinearColor::White, 0.8f, true);
	DrawCenteredText(TEXT(">"), NextBounds.Min.X + 21.0f, NextBounds.Min.Y + 8.0f, FLinearColor::White, 0.8f, true);
	DrawCenteredText(Value, ControlX + 150.0f, Y + 15.0f, FLinearColor(0.08f, 0.78f, 0.88f, 1.0f), 0.72f);
}

void AFlickHUD::AddMenuHitRegion(
	const EFlickMenuAction Action,
	const FBox2D& Bounds,
	const EFlickMatchVariant Variant,
	const EFlickTeam Team,
	const int32 PieceSlot)
{
	FFlickMenuHitRegion& Region = MenuHitRegions.AddDefaulted_GetRef();
	Region.Bounds = Bounds;
	Region.Action = Action;
	Region.MatchVariant = Variant;
	Region.Team = Team;
	Region.PieceSlot = PieceSlot;
}

bool AFlickHUD::IsMenuRegionHovered(const FBox2D& Bounds) const
{
	return MenuCursorPosition.X >= Bounds.Min.X
		&& MenuCursorPosition.X <= Bounds.Max.X
		&& MenuCursorPosition.Y >= Bounds.Min.Y
		&& MenuCursorPosition.Y <= Bounds.Max.Y;
}

FLinearColor AFlickHUD::GetModeColor(const EFlickMatchVariant Variant) const
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob:
		return FLinearColor(0.95f, 0.72f, 0.12f, 1.0f);
	case EFlickMatchVariant::Classic:
	default:
		return FLinearColor(0.0f, 0.72f, 0.92f, 1.0f);
	}
}
