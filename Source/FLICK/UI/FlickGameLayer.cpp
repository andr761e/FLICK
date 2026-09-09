#include "UI/FlickGameLayer.h"
#include "UI/FlickUITheme.h"

#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickRankRules.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameState.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "Pieces/FlickPiece.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Online/FlickSessionSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"
#include "UI/FlickHUD.h"
#include "UI/FlickLogoWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	using namespace FlickUITheme;

	namespace UiMetrics
	{
		constexpr float ModeHeaderHeight = 104.0f;
		constexpr float ModeContentWidth = 1120.0f;
		constexpr float ModeFooterHeight = 82.0f;
		constexpr float PlaylistCardHeight = 154.0f;
		constexpr float TrainingCardHeight = 238.0f;
		constexpr float FormatCardHeight = 196.0f;
		constexpr float CardGap = 8.0f;
		constexpr float ActionHeight = 52.0f;
	}

	namespace MainMenuStackMetrics
	{
		constexpr float PrimaryHeight = 86.0f;
		constexpr float SecondaryHeight = 62.0f;
		constexpr float Gap = 8.0f;
		constexpr float FirstWidth = 450.0f;
		constexpr float LeftEdgeSlope = 0.0f;
		constexpr float RightEdgeSlope = 0.0f;

		constexpr float GetRowTop(const int32 RowIndex)
		{
			return RowIndex <= 0
				? 0.0f
				: PrimaryHeight + Gap + (RowIndex - 1) * (SecondaryHeight + Gap);
		}

		constexpr float GetRowHeight(const int32 RowIndex)
		{
			return RowIndex == 0 ? PrimaryHeight : SecondaryHeight;
		}

		constexpr float GetRowLeft(const int32 RowIndex)
		{
			return GetRowTop(RowIndex) * LeftEdgeSlope;
		}

		constexpr float GetRowWidth(const int32 RowIndex)
		{
			const float FirstTopRight = FirstWidth - PrimaryHeight * RightEdgeSlope;
			const float RowBottom = GetRowTop(RowIndex) + GetRowHeight(RowIndex);
			return FirstTopRight + RowBottom * RightEdgeSlope - GetRowLeft(RowIndex);
		}
	}

	FString GetLineupPresetSummary(const EFlickLineupPreset Preset)
	{
		switch (Preset)
		{
		case EFlickLineupPreset::Power: return TEXT("A HEAVIER LINEUP BUILT TO HOLD SPACE AND WIN COLLISIONS.");
		case EFlickLineupPreset::Speed: return TEXT("A MOBILE LINEUP BUILT FOR FAST ATTACKS, BANKS, AND RECOVERY.");
		case EFlickLineupPreset::Control: return TEXT("A PRECISE LINEUP BUILT TO STOP CLEANLY AND SHAPE THE BOARD.");
		case EFlickLineupPreset::Balanced:
		default: return TEXT("A VERSATILE LINEUP WITH AN ANSWER FOR EVERY SITUATION.");
		}
	}

	FString GetLineupPresetStrengths(const EFlickLineupPreset Preset)
	{
		switch (Preset)
		{
		case EFlickLineupPreset::Power: return TEXT("MASS  |  BOARD CONTROL  |  DEFENSE");
		case EFlickLineupPreset::Speed: return TEXT("PACE  |  REBOUNDS  |  MOBILITY");
		case EFlickLineupPreset::Control: return TEXT("PRECISION  |  GRIP  |  POSITIONING");
		case EFlickLineupPreset::Balanced:
		default: return TEXT("VERSATILITY  |  ADAPTABILITY  |  RELIABILITY");
		}
	}

	FString GetLineupPresetTradeoffs(const EFlickLineupPreset Preset)
	{
		switch (Preset)
		{
		case EFlickLineupPreset::Power: return TEXT("LOWER SPEED AND RECOVERY");
		case EFlickLineupPreset::Speed: return TEXT("LOWER MASS AND HOLDING POWER");
		case EFlickLineupPreset::Control: return TEXT("LOWER COAST AND HIGHER EXECUTION RISK");
		case EFlickLineupPreset::Balanced:
		default: return TEXT("NO EXTREME SPECIALTY");
		}
	}

	float GetDisplayStatValue(const FFlickPieceDisplayStats& Stats, const int32 StatIndex)
	{
		switch (StatIndex)
		{
		case 0: return Stats.Speed;
		case 1: return Stats.Weight;
		case 2: return Stats.Impact;
		case 3: return Stats.Control;
		case 4: return Stats.Coast;
		case 5: return Stats.Stability;
		default: return 0.0f;
		}
	}

	const FSlateBrush* WhiteBrush()
	{
		return FCoreStyle::Get().GetBrush("WhiteBrush");
	}

	FSlateFontInfo UiFont(const int32 Size, const bool bBold = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	FSlateFontInfo DisplayFont(const int32 Size, const bool bItalic = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bItalic ? "BoldCondensedItalic" : "BoldCondensed", Size);
	}

	class SFlickDiagonalPanel final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickDiagonalPanel)
			: _PanelColor(FLinearColor(0.002f, 0.009f, 0.016f, 0.82f))
			, _EdgeColor(FLinearColor(0.0f, 0.76f, 1.0f, 0.58f))
		{}
			SLATE_ATTRIBUTE(FLinearColor, PanelColor)
			SLATE_ATTRIBUTE(FLinearColor, EdgeColor)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			PanelColor = InArgs._PanelColor;
			EdgeColor = InArgs._EdgeColor;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(const float LayoutScaleMultiplier) const override
		{
			return FVector2D(1.0f, 1.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
			if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
			{
				return LayerId;
			}

			const float TopEdgeX = LocalSize.X * 0.345f;
			const float BottomEdgeX = LocalSize.X * 0.395f;
			const float EdgeWidth = FMath::Clamp(LocalSize.X * 0.00065f, 1.1f, 1.2f);
			const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			const FLinearColor LeftPanelColor = PanelColor.Get() * InWidgetStyle.GetColorAndOpacityTint();
			FLinearColor RightPanelColor = LeftPanelColor;
			RightPanelColor.A *= 0.93f;
			const FColor LeftPanelTint = LeftPanelColor.ToFColor(true);
			const FColor RightPanelTint = RightPanelColor.ToFColor(true);
			const FColor EdgeTint = (EdgeColor.Get() * InWidgetStyle.GetColorAndOpacityTint()).ToFColor(true);

			TArray<FSlateVertex> Vertices;
			Vertices.Reserve(8);
			const auto AddVertex = [&Vertices, &Transform](const FVector2f Position, const FVector2f Uv, const FColor Color)
			{
				Vertices.Add(FSlateVertex::Make(Transform, Position, Uv, Color));
			};
			AddVertex(FVector2f(0.0f, 0.0f), FVector2f(0.0f, 0.0f), LeftPanelTint);
			AddVertex(FVector2f(TopEdgeX, 0.0f), FVector2f(1.0f, 0.0f), RightPanelTint);
			AddVertex(FVector2f(BottomEdgeX, LocalSize.Y), FVector2f(1.0f, 1.0f), RightPanelTint);
			AddVertex(FVector2f(0.0f, LocalSize.Y), FVector2f(0.0f, 1.0f), LeftPanelTint);
			AddVertex(FVector2f(TopEdgeX - EdgeWidth, 0.0f), FVector2f(0.0f, 0.0f), EdgeTint);
			AddVertex(FVector2f(TopEdgeX, 0.0f), FVector2f(1.0f, 0.0f), EdgeTint);
			AddVertex(FVector2f(BottomEdgeX, LocalSize.Y), FVector2f(1.0f, 1.0f), EdgeTint);
			AddVertex(FVector2f(BottomEdgeX - EdgeWidth, LocalSize.Y), FVector2f(0.0f, 1.0f), EdgeTint);

			const TArray<SlateIndex> Indices = {
				0, 1, 2, 0, 2, 3,
				4, 5, 6, 4, 6, 7};
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId,
				WhiteBrush()->GetRenderingResource(),
				Vertices,
				Indices,
				nullptr,
				0,
				0);
			return LayerId;
		}

	private:
		TAttribute<FLinearColor> PanelColor;
		TAttribute<FLinearColor> EdgeColor;
	};

	class SFlickMainMenuImageOverlay final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickMainMenuImageOverlay)
			: _Opacity(0.8f)
		{}
			SLATE_ATTRIBUTE(float, Opacity)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Opacity = InArgs._Opacity;
			OverlayTexture.Reset(LoadObject<UTexture2D>(
				nullptr,
				TEXT("/Game/UI/MainMenuDiagonalOverlay_Source.MainMenuDiagonalOverlay_Source")));
			OverlayBrush.SetResourceObject(OverlayTexture.Get());
			OverlayBrush.ImageSize = FVector2D(1672.0f, 941.0f);
			OverlayBrush.DrawAs = ESlateBrushDrawType::Image;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(const float LayoutScaleMultiplier) const override
		{
			return OverlayBrush.ImageSize;
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			if (OverlayTexture.IsValid())
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					AllottedGeometry.ToPaintGeometry(),
					&OverlayBrush,
					ESlateDrawEffect::None,
					FLinearColor(1.0f, 1.0f, 1.0f, FMath::Clamp(Opacity.Get(), 0.0f, 1.0f)));
			}

			return LayerId;
		}

	private:
		TAttribute<float> Opacity;
		TStrongObjectPtr<UTexture2D> OverlayTexture;
		FSlateBrush OverlayBrush;
	};

	class SFlickRoundPip final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickRoundPip)
			: _Color(FLinearColor::White)
			, _Filled(false)
		{}
			SLATE_ATTRIBUTE(FLinearColor, Color)
			SLATE_ATTRIBUTE(bool, Filled)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Color = InArgs._Color;
			Filled = InArgs._Filled;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(20.0f, 20.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
			const float Radius = FMath::Min(Center.X, Center.Y) - 2.0f;
			TArray<FVector2D> Outline;
			constexpr int32 Segments = 28;
			for (int32 Segment = 0; Segment <= Segments; ++Segment)
			{
				const float Angle = 2.0f * PI * static_cast<float>(Segment) / Segments;
				Outline.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			const FLinearColor PipColor = Color.Get();
			FSlateDrawElement::MakeLines(
				OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Outline,
				ESlateDrawEffect::None, PipColor.CopyWithNewOpacity(Filled.Get() ? 1.0f : 0.48f), true, 1.6f);
			if (Filled.Get())
			{
				TArray<FVector2D> Inner;
				for (int32 Segment = 0; Segment <= Segments; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / Segments;
					Inner.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Radius - 3.5f));
				}
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Inner,
					ESlateDrawEffect::None, PipColor.CopyWithNewOpacity(0.82f), true, 3.0f);
			}
			return LayerId + 1;
		}

	private:
		TAttribute<FLinearColor> Color;
		TAttribute<bool> Filled;
	};

	class SFlickMainMenuFrame final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickMainMenuFrame)
			: _StartColor(FLinearColor(0.006f, 0.018f, 0.03f, 0.9f))
			, _EndColor(FLinearColor(0.012f, 0.032f, 0.052f, 0.9f))
			, _HoverStartColor(FLinearColor(0.01f, 0.055f, 0.09f, 0.94f))
			, _HoverEndColor(FLinearColor(0.015f, 0.11f, 0.17f, 0.94f))
			, _BorderColor(Hairline)
			, _HoverBorderColor(Cyan)
			, _Highlighted(false)
			, _BorderWidth(1.0f)
			, _Padding(FMargin(0.0f))
		{}
			SLATE_ATTRIBUTE(FLinearColor, StartColor)
			SLATE_ATTRIBUTE(FLinearColor, EndColor)
			SLATE_ATTRIBUTE(FLinearColor, HoverStartColor)
			SLATE_ATTRIBUTE(FLinearColor, HoverEndColor)
			SLATE_ATTRIBUTE(FLinearColor, BorderColor)
			SLATE_ATTRIBUTE(FLinearColor, HoverBorderColor)
			SLATE_ATTRIBUTE(bool, Highlighted)
			SLATE_ARGUMENT(float, BorderWidth)
			SLATE_ARGUMENT(FMargin, Padding)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			StartColor = InArgs._StartColor;
			EndColor = InArgs._EndColor;
			HoverStartColor = InArgs._HoverStartColor;
			HoverEndColor = InArgs._HoverEndColor;
			BorderColor = InArgs._BorderColor;
			HoverBorderColor = InArgs._HoverBorderColor;
			Highlighted = InArgs._Highlighted;
			BorderWidth = InArgs._BorderWidth;
			SetCanTick(true);
			ChildSlot.Padding(InArgs._Padding)[InArgs._Content.Widget];
		}

		virtual void Tick(
			const FGeometry& AllottedGeometry,
			const double InCurrentTime,
			const float InDeltaTime) override
		{
			SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
			const float Target = Highlighted.Get() ? 1.0f : 0.0f;
			const float PreviousAlpha = HighlightAlpha;
			HighlightAlpha = FMath::FInterpTo(HighlightAlpha, Target, InDeltaTime, 14.0f);
			if (FMath::Abs(Target - HighlightAlpha) < 0.002f)
			{
				HighlightAlpha = Target;
			}
			if (!FMath::IsNearlyEqual(PreviousAlpha, HighlightAlpha, 0.001f))
			{
				Invalidate(EInvalidateWidgetReason::Paint);
			}
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			// A short forward cut repeats the launch-direction motif.
			const float RightCut = 12.0f;
			const float LeftSkew = 0.0f;
			const TArray<FVector2D> Points = {
				FVector2D(0.0f, 0.0f),
				FVector2D(Size.X - RightCut, 0.0f),
				FVector2D(Size.X, Size.Y),
				FVector2D(LeftSkew, Size.Y)};
			const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			const FLinearColor Left = FMath::Lerp(StartColor.Get(), HoverStartColor.Get(), HighlightAlpha) * InWidgetStyle.GetColorAndOpacityTint();
			const FLinearColor Right = FMath::Lerp(EndColor.Get(), HoverEndColor.Get(), HighlightAlpha) * InWidgetStyle.GetColorAndOpacityTint();
			const FLinearColor OutlineColor = FMath::Lerp(BorderColor.Get(), HoverBorderColor.Get(), HighlightAlpha) * InWidgetStyle.GetColorAndOpacityTint();
			TArray<FSlateVertex> Vertices;
			Vertices.Reserve(5);
			Vertices.Add(FSlateVertex::Make(
				Transform,
				FVector2f(static_cast<float>(Size.X * 0.5f), static_cast<float>(Size.Y * 0.5f)),
				FVector2f(0.5f, 0.5f),
				FMath::Lerp(Left, Right, 0.5f).ToFColor(true)));
			for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
			{
				const FVector2D& Point = Points[PointIndex];
				const bool bRightPoint = PointIndex == 1 || PointIndex == 2;
				Vertices.Add(FSlateVertex::Make(
					Transform,
					FVector2f(static_cast<float>(Point.X), static_cast<float>(Point.Y)),
					FVector2f(static_cast<float>(Point.X / FMath::Max(Size.X, 1.0f)), static_cast<float>(Point.Y / FMath::Max(Size.Y, 1.0f))),
					(bRightPoint ? Right : Left).ToFColor(true)));
			}
			const TArray<SlateIndex> Indices = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1};
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements, LayerId, WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);

			TArray<FVector2D> Outline = Points;
			Outline.Add(Points[0]);

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(),
				Outline,
				ESlateDrawEffect::None,
				OutlineColor,
				true,
				BorderWidth);
			return SCompoundWidget::OnPaint(
				Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
		}

	private:
		TAttribute<FLinearColor> StartColor;
		TAttribute<FLinearColor> EndColor;
		TAttribute<FLinearColor> HoverStartColor;
		TAttribute<FLinearColor> HoverEndColor;
		TAttribute<FLinearColor> BorderColor;
		TAttribute<FLinearColor> HoverBorderColor;
		TAttribute<bool> Highlighted;
		float HighlightAlpha = 0.0f;
		float BorderWidth = 1.0f;
	};

	enum class EFlickMainMenuIcon : uint8
	{
		Play,
		Lineups,
		Profile,
		Stats,
		Leaderboard,
		History,
		Back,
		Shop,
		Settings,
		Quit,
		Rank,
		Social
	};

	class SFlickMainMenuIcon final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickMainMenuIcon)
			: _Icon(EFlickMainMenuIcon::Play)
			, _Color(FLinearColor::White)
		{}
			SLATE_ARGUMENT(EFlickMainMenuIcon, Icon)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Icon = InArgs._Icon;
			Color = InArgs._Color;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(38.0f, 38.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			const FVector2D Center = Size * 0.5f;
			const FLinearColor Tint = Color.Get();
			const auto DrawLines = [&OutDrawElements, &AllottedGeometry, &Tint, LayerId](const TArray<FVector2D>& Points, const float Width = 2.0f)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Tint, true, Width);
			};
			const auto CirclePoints = [](const FVector2D& CircleCenter, const float Radius, const float StartAngle = 0.0f, const float EndAngle = 2.0f * PI)
			{
				TArray<FVector2D> Points;
				constexpr int32 Segments = 24;
				for (int32 Segment = 0; Segment <= Segments; ++Segment)
				{
					const float Alpha = static_cast<float>(Segment) / Segments;
					const float Angle = FMath::Lerp(StartAngle, EndAngle, Alpha);
					Points.Add(CircleCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				return Points;
			};

			switch (Icon)
			{
			case EFlickMainMenuIcon::Play:
			{
				const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
				const FColor VertexColor = Tint.ToFColor(true);
				const TArray<FSlateVertex> Vertices = {
					FSlateVertex::Make(Transform, FVector2f(10.0f, 6.0f), FVector2f(0.0f, 0.0f), VertexColor),
					FSlateVertex::Make(Transform, FVector2f(31.0f, 19.0f), FVector2f(1.0f, 0.5f), VertexColor),
					FSlateVertex::Make(Transform, FVector2f(10.0f, 32.0f), FVector2f(0.0f, 1.0f), VertexColor)};
				const TArray<SlateIndex> Indices = {0, 1, 2};
				FSlateDrawElement::MakeCustomVerts(
					OutDrawElements, LayerId, WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);
				break;
			}
			case EFlickMainMenuIcon::Lineups:
			case EFlickMainMenuIcon::Social:
				DrawLines(CirclePoints(Center + FVector2D(-9.0f, -7.0f), 4.0f));
				DrawLines(CirclePoints(Center + FVector2D(0.0f, -10.0f), 4.5f));
				DrawLines(CirclePoints(Center + FVector2D(9.0f, -7.0f), 4.0f));
				DrawLines({FVector2D(5.0f, 30.0f), FVector2D(7.0f, 23.0f), FVector2D(13.0f, 19.0f), FVector2D(19.0f, 18.0f), FVector2D(25.0f, 19.0f), FVector2D(31.0f, 23.0f), FVector2D(33.0f, 30.0f)});
				break;
			case EFlickMainMenuIcon::Profile:
				DrawLines(CirclePoints(Center + FVector2D(0.0f, -7.0f), 7.0f), 2.2f);
				DrawLines({FVector2D(7.0f, 34.0f), FVector2D(8.5f, 27.0f), FVector2D(13.0f, 21.0f), FVector2D(20.0f, 19.0f), FVector2D(27.0f, 21.0f), FVector2D(31.5f, 27.0f), FVector2D(33.0f, 34.0f)}, 2.2f);
				break;
			case EFlickMainMenuIcon::Stats:
				DrawLines({FVector2D(7.0f, 31.0f), FVector2D(7.0f, 22.0f), FVector2D(13.0f, 22.0f), FVector2D(13.0f, 31.0f)}, 2.2f);
				DrawLines({FVector2D(17.0f, 31.0f), FVector2D(17.0f, 15.0f), FVector2D(23.0f, 15.0f), FVector2D(23.0f, 31.0f)}, 2.2f);
				DrawLines({FVector2D(27.0f, 31.0f), FVector2D(27.0f, 8.0f), FVector2D(33.0f, 8.0f), FVector2D(33.0f, 31.0f)}, 2.2f);
				DrawLines({FVector2D(5.0f, 33.0f), FVector2D(35.0f, 33.0f)}, 1.6f);
				break;
			case EFlickMainMenuIcon::Leaderboard:
				DrawLines({FVector2D(10.0f, 7.0f), FVector2D(30.0f, 7.0f), FVector2D(27.0f, 19.0f), FVector2D(20.0f, 24.0f), FVector2D(13.0f, 19.0f), FVector2D(10.0f, 7.0f)}, 2.2f);
				DrawLines({FVector2D(10.0f, 10.0f), FVector2D(5.0f, 10.0f), FVector2D(7.0f, 18.0f), FVector2D(13.0f, 20.0f)}, 1.8f);
				DrawLines({FVector2D(30.0f, 10.0f), FVector2D(35.0f, 10.0f), FVector2D(33.0f, 18.0f), FVector2D(27.0f, 20.0f)}, 1.8f);
				DrawLines({FVector2D(20.0f, 24.0f), FVector2D(20.0f, 30.0f), FVector2D(13.0f, 34.0f), FVector2D(27.0f, 34.0f), FVector2D(20.0f, 30.0f)}, 2.0f);
				break;
			case EFlickMainMenuIcon::History:
				DrawLines(CirclePoints(Center, 13.0f), 2.2f);
				DrawLines({Center, FVector2D(Center.X, 10.0f)}, 2.1f);
				DrawLines({Center, FVector2D(28.0f, 24.0f)}, 2.1f);
				break;
			case EFlickMainMenuIcon::Back:
				DrawLines({FVector2D(31.0f, 8.0f), FVector2D(14.0f, 19.0f), FVector2D(31.0f, 30.0f)}, 2.6f);
				DrawLines({FVector2D(14.0f, 19.0f), FVector2D(36.0f, 19.0f)}, 2.6f);
				break;
			case EFlickMainMenuIcon::Rank:
				DrawLines(CirclePoints(Center + FVector2D(0.0f, -3.0f), 12.0f), 2.2f);
				DrawLines(CirclePoints(Center + FVector2D(0.0f, -3.0f), 7.0f), 1.4f);
				DrawLines({FVector2D(11.0f, 25.0f), FVector2D(9.0f, 36.0f), FVector2D(16.0f, 32.0f), FVector2D(19.0f, 38.0f), FVector2D(20.0f, 27.0f)}, 2.0f);
				DrawLines({FVector2D(27.0f, 25.0f), FVector2D(29.0f, 36.0f), FVector2D(23.0f, 32.0f)}, 2.0f);
				DrawLines({FVector2D(19.0f, 12.0f), FVector2D(23.0f, 15.0f), FVector2D(19.0f, 20.0f)}, 1.8f);
				break;
			case EFlickMainMenuIcon::Shop:
				DrawLines({FVector2D(5.0f, 7.0f), FVector2D(10.0f, 7.0f), FVector2D(14.0f, 25.0f), FVector2D(30.0f, 25.0f), FVector2D(33.0f, 13.0f), FVector2D(12.0f, 13.0f)});
				DrawLines({FVector2D(15.0f, 19.0f), FVector2D(31.0f, 19.0f)}, 1.5f);
				DrawLines(CirclePoints(FVector2D(17.0f, 31.0f), 2.5f));
				DrawLines(CirclePoints(FVector2D(29.0f, 31.0f), 2.5f));
				break;
			case EFlickMainMenuIcon::Settings:
				DrawLines(CirclePoints(Center, 9.0f));
				DrawLines(CirclePoints(Center, 3.5f), 2.4f);
				for (int32 Spoke = 0; Spoke < 8; ++Spoke)
				{
					const float Angle = 2.0f * PI * Spoke / 8.0f;
					const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
					DrawLines({Center + Direction * 10.5f, Center + Direction * 15.0f}, 2.5f);
				}
				break;
			case EFlickMainMenuIcon::Quit:
				DrawLines(CirclePoints(Center + FVector2D(0.0f, 2.0f), 12.0f, -0.25f * PI, 1.25f * PI), 2.5f);
				DrawLines({FVector2D(Center.X, 4.0f), FVector2D(Center.X, 20.0f)}, 2.8f);
				break;
			}
			return LayerId;
		}

	private:
		EFlickMainMenuIcon Icon = EFlickMainMenuIcon::Play;
		TAttribute<FLinearColor> Color;
	};

	class SFlickMainMenuTechLines final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickMainMenuTechLines) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(1.0f, 1.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			const FLinearColor Line(0.0f, 0.46f, 0.64f, 0.12f);
			const auto Draw = [&OutDrawElements, &AllottedGeometry, &Line, LayerId](const TArray<FVector2D>& Points, const float Width = 1.0f)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Line, true, Width);
			};
			Draw({FVector2D(0.0f, Size.Y * 0.244f), FVector2D(Size.X * 0.025f, Size.Y * 0.274f), FVector2D(Size.X * 0.15f, Size.Y * 0.274f)});
			Draw({FVector2D(Size.X * 0.018f, Size.Y * 0.205f), FVector2D(Size.X * 0.145f, Size.Y * 0.205f), FVector2D(Size.X * 0.172f, Size.Y * 0.235f)});
			Draw({FVector2D(Size.X * 0.024f, Size.Y * 0.232f), FVector2D(Size.X * 0.164f, Size.Y * 0.232f)}, 1.2f);
			Draw({FVector2D(Size.X * 0.031f, Size.Y * 0.304f), FVector2D(Size.X * 0.118f, Size.Y * 0.304f), FVector2D(Size.X * 0.142f, Size.Y * 0.328f)});
			Draw({FVector2D(Size.X * 0.018f, Size.Y * 0.342f), FVector2D(Size.X * 0.105f, Size.Y * 0.342f)});
			Draw({FVector2D(Size.X * 0.026f, Size.Y * 0.62f), FVector2D(Size.X * 0.125f, Size.Y * 0.62f), FVector2D(Size.X * 0.158f, Size.Y * 0.66f)});
			Draw({FVector2D(Size.X * 0.03f, Size.Y * 0.69f), FVector2D(Size.X * 0.15f, Size.Y * 0.69f), FVector2D(Size.X * 0.202f, Size.Y * 0.758f)});
			Draw({FVector2D(0.0f, Size.Y * 0.79f), FVector2D(Size.X * 0.055f, Size.Y * 0.73f), FVector2D(Size.X * 0.165f, Size.Y * 0.73f)});
			Draw({FVector2D(Size.X * 0.02f, Size.Y * 0.842f), FVector2D(Size.X * 0.11f, Size.Y * 0.842f), FVector2D(Size.X * 0.137f, Size.Y * 0.812f)});
			Draw({FVector2D(Size.X * 0.045f, Size.Y * 0.884f), FVector2D(Size.X * 0.17f, Size.Y * 0.884f)});
			for (int32 DetailIndex = 0; DetailIndex < 5; ++DetailIndex)
			{
				const float X = Size.X * (0.128f + DetailIndex * 0.008f);
				Draw({FVector2D(X, Size.Y * 0.274f), FVector2D(X + 2.0f, Size.Y * 0.274f)}, 1.8f);
			}
			return LayerId;
		}
	};

	class SFlickInterfaceBackdrop final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickInterfaceBackdrop)
			: _Opacity(1.0f)
		{}
			SLATE_ATTRIBUTE(float, Opacity)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Opacity = InArgs._Opacity;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(1.0f, 1.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			if (Size.X < 2.0f || Size.Y < 2.0f)
			{
				return LayerId;
			}

			const float Alpha = FMath::Clamp(Opacity.Get() * InWidgetStyle.GetColorAndOpacityTint().A, 0.0f, 1.0f);
			const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			const TArray<FSlateVertex> Vertices = {
				FSlateVertex::Make(Transform, FVector2f(0.0f, 0.0f), FVector2f(0.0f, 0.0f), FLinearColor(0.009f, 0.013f, 0.012f, 0.30f * Alpha).ToFColor(true)),
				FSlateVertex::Make(Transform, FVector2f(Size.X, 0.0f), FVector2f(1.0f, 0.0f), FLinearColor(0.012f, 0.016f, 0.013f, 0.20f * Alpha).ToFColor(true)),
				FSlateVertex::Make(Transform, FVector2f(Size.X, Size.Y), FVector2f(1.0f, 1.0f), FLinearColor(0.003f, 0.006f, 0.005f, 0.46f * Alpha).ToFColor(true)),
				FSlateVertex::Make(Transform, FVector2f(0.0f, Size.Y), FVector2f(0.0f, 1.0f), FLinearColor(0.006f, 0.009f, 0.008f, 0.34f * Alpha).ToFColor(true))};
			const TArray<SlateIndex> Indices = {0, 1, 2, 0, 2, 3};
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements, LayerId, WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);

			const auto DrawLine = [&OutDrawElements, &AllottedGeometry, LayerId](
				const TArray<FVector2D>& Points,
				const FLinearColor& Color,
				const float Thickness,
				const int32 LayerOffset = 1)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					LayerId + LayerOffset,
					AllottedGeometry.ToPaintGeometry(),
					Points,
					ESlateDrawEffect::None,
					Color,
					true,
					Thickness);
			};

			// The arc and tangent echo the puck and its launch path.
			const FVector2D Center(Size.X * 0.76f, Size.Y * 0.52f);
			for (int32 Ring = 0; Ring < 3; ++Ring)
			{
				TArray<FVector2D> Arc;
				const float Radius = Size.Y * (0.30f + Ring * 0.10f);
				for (int32 Step = 0; Step <= 80; ++Step)
				{
					const float Angle = FMath::DegreesToRadians(-110.0f + Step * 3.4f);
					Arc.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				DrawLine(Arc, Paper.CopyWithNewOpacity(0.035f * Alpha), 1.0f);
			}
			DrawLine({{32.0f, 30.0f}, {100.0f, 30.0f}}, Brand.CopyWithNewOpacity(0.8f * Alpha), 3.0f, 2);
			DrawLine({{Size.X - 100.0f, Size.Y - 30.0f}, {Size.X - 32.0f, Size.Y - 30.0f}}, Brand.CopyWithNewOpacity(0.6f * Alpha), 3.0f, 2);

			return LayerId + 2;
		}

	private:
		TAttribute<float> Opacity;
	};

	class SFlickPlaylistGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickPlaylistGlyph)
			: _Playlist(EFlickPlayPlaylist::Casual)
			, _Color(Cyan)
		{}
			SLATE_ARGUMENT(EFlickPlayPlaylist, Playlist)
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Playlist = InArgs._Playlist;
			Color = InArgs._Color;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(72.0f, 54.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			const FVector2D Center = Size * 0.5f;
			const FLinearColor Tint = Color.Get();
			const auto Draw = [&OutDrawElements, &AllottedGeometry, &Tint, LayerId](const TArray<FVector2D>& Points, const float Thickness = 1.6f, const float Opacity = 1.0f)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Tint.CopyWithNewOpacity(Tint.A * Opacity), true, Thickness);
			};
			const auto Circle = [](const FVector2D& CircleCenter, const float Radius)
			{
				TArray<FVector2D> Points;
				for (int32 Segment = 0; Segment <= 28; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / 28.0f;
					Points.Add(CircleCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				return Points;
			};

			switch (Playlist)
			{
			case EFlickPlayPlaylist::Competitive:
				Draw({Center + FVector2D(0.0f, -21.0f), Center + FVector2D(20.0f, -12.0f), Center + FVector2D(16.0f, 11.0f), Center + FVector2D(0.0f, 23.0f), Center + FVector2D(-16.0f, 11.0f), Center + FVector2D(-20.0f, -12.0f), Center + FVector2D(0.0f, -21.0f)}, 1.8f);
				Draw({Center + FVector2D(-8.0f, 2.0f), Center + FVector2D(0.0f, -7.0f), Center + FVector2D(8.0f, 2.0f)}, 2.2f);
				break;
			case EFlickPlayPlaylist::Training:
				Draw(Circle(Center, 22.0f), 1.25f, 0.62f);
				Draw(Circle(Center, 13.0f), 1.5f, 0.82f);
				Draw(Circle(Center, 4.0f), 2.5f);
				Draw({Center + FVector2D(0.0f, -27.0f), Center + FVector2D(0.0f, -17.0f)});
				Draw({Center + FVector2D(27.0f, 0.0f), Center + FVector2D(17.0f, 0.0f)});
				break;
			case EFlickPlayPlaylist::Test:
				Draw(Circle(Center + FVector2D(-18.0f, 14.0f), 7.0f), 1.7f);
				Draw(Circle(Center + FVector2D(18.0f, -14.0f), 7.0f), 1.7f);
				Draw({Center + FVector2D(-11.0f, 14.0f), Center + FVector2D(9.0f, 14.0f), Center + FVector2D(9.0f, 2.0f)}, 1.5f, 0.78f);
				Draw({Center + FVector2D(11.0f, -14.0f), Center + FVector2D(-9.0f, -14.0f), Center + FVector2D(-9.0f, -2.0f)}, 1.5f, 0.78f);
				Draw({Center + FVector2D(-28.0f, 0.0f), Center + FVector2D(-5.0f, 0.0f)}, 3.0f);
				Draw({Center + FVector2D(5.0f, 0.0f), Center + FVector2D(28.0f, 0.0f)}, 3.0f);
				break;
			case EFlickPlayPlaylist::PrivateMatch:
				Draw({Center + FVector2D(-17.0f, -3.0f), Center + FVector2D(-17.0f, 20.0f), Center + FVector2D(17.0f, 20.0f), Center + FVector2D(17.0f, -3.0f), Center + FVector2D(-17.0f, -3.0f)}, 1.7f);
				Draw({Center + FVector2D(-11.0f, -3.0f), Center + FVector2D(-11.0f, -13.0f), Center + FVector2D(-6.0f, -20.0f), Center + FVector2D(6.0f, -20.0f), Center + FVector2D(11.0f, -13.0f), Center + FVector2D(11.0f, -3.0f)}, 1.7f);
				Draw(Circle(Center + FVector2D(0.0f, 7.0f), 3.0f), 1.8f);
				break;
			case EFlickPlayPlaylist::Casual:
			default:
				Draw(Circle(Center + FVector2D(-14.0f, 7.0f), 14.0f), 1.8f);
				Draw(Circle(Center + FVector2D(14.0f, -7.0f), 14.0f), 1.8f);
				Draw(Circle(Center + FVector2D(-14.0f, 7.0f), 5.0f), 1.25f, 0.72f);
				Draw(Circle(Center + FVector2D(14.0f, -7.0f), 5.0f), 1.25f, 0.72f);
				break;
			}
			return LayerId;
		}

	private:
		EFlickPlayPlaylist Playlist = EFlickPlayPlaylist::Casual;
		TAttribute<FLinearColor> Color;
	};

	class SFlickArenaDiagram final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickArenaDiagram)
			: _PlayersPerTeam(1)
			, _Bob(false)
		{}
			SLATE_ARGUMENT(int32, PlayersPerTeam)
			SLATE_ARGUMENT(bool, Bob)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			PlayersPerTeam = FMath::Clamp(InArgs._PlayersPerTeam, 1, 3);
			bBob = InArgs._Bob;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(116.0f, 62.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			const FVector2D Center = Size * 0.5f;
			const auto Ellipse = [&Center](const float RadiusX, const float RadiusY)
			{
				TArray<FVector2D> Points;
				for (int32 Segment = 0; Segment <= 40; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / 40.0f;
					Points.Add(Center + FVector2D(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY));
				}
				return Points;
			};
			const auto Draw = [&OutDrawElements, &AllottedGeometry](const int32 Layer, const TArray<FVector2D>& Points, const FLinearColor& Color, const float Thickness)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Color, true, Thickness);
			};
			const auto Puck = [&Draw, LayerId](const FVector2D& Position, const FLinearColor& Color)
			{
				TArray<FVector2D> Points;
				for (int32 Segment = 0; Segment <= 18; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / 18.0f;
					Points.Add(Position + FVector2D(FMath::Cos(Angle) * 4.3f, FMath::Sin(Angle) * 3.2f));
				}
				Draw(LayerId + 2, Points, Color, 1.35f);
			};

			Draw(LayerId, Ellipse(Size.X * 0.47f, Size.Y * 0.43f), FLinearColor(0.34f, 0.5f, 0.6f, 0.62f), 1.0f);
			Draw(LayerId, Ellipse(Size.X * 0.31f, Size.Y * 0.28f), FLinearColor(0.18f, 0.31f, 0.4f, 0.42f), 0.8f);
			Draw(LayerId, {FVector2D(Center.X, 5.0f), FVector2D(Center.X, Size.Y - 5.0f)}, FLinearColor(0.24f, 0.37f, 0.46f, 0.42f), 0.8f);
			Draw(LayerId, {FVector2D(7.0f, Center.Y), FVector2D(Size.X - 7.0f, Center.Y)}, FLinearColor(0.24f, 0.37f, 0.46f, 0.42f), 0.8f);
			Draw(LayerId + 1, {FVector2D(10.0f, Center.Y + 1.0f), FVector2D(16.0f, Center.Y + 10.0f), FVector2D(22.0f, Center.Y + 1.0f)}, Cyan.CopyWithNewOpacity(0.85f), 1.6f);
			Draw(LayerId + 1, {FVector2D(Size.X - 10.0f, Center.Y - 1.0f), FVector2D(Size.X - 16.0f, Center.Y - 10.0f), FVector2D(Size.X - 22.0f, Center.Y - 1.0f)}, Orange.CopyWithNewOpacity(0.85f), 1.6f);

			for (int32 Index = 0; Index < PlayersPerTeam; ++Index)
			{
				const float OffsetX = (static_cast<float>(Index) - static_cast<float>(PlayersPerTeam - 1) * 0.5f) * 13.0f;
				Puck(Center + FVector2D(OffsetX, 15.0f), Cyan);
				Puck(Center + FVector2D(OffsetX, -15.0f), bBob ? FLinearColor(0.18f, 0.82f, 0.48f, 1.0f) : Orange);
			}
			return LayerId + 2;
		}

	private:
		int32 PlayersPerTeam = 1;
		bool bBob = false;
	};

	class SFlickStatusGlobe final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickStatusGlobe)
			: _Color(Cyan)
		{}
			SLATE_ATTRIBUTE(FLinearColor, Color)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Color = InArgs._Color;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(26.0f, 26.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
			const FLinearColor Tint = Color.Get();
			const auto Draw = [&OutDrawElements, &AllottedGeometry, &Tint, LayerId](const TArray<FVector2D>& Points)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Tint, true, 1.25f);
			};
			const auto Arc = [&Center](const float RadiusX, const float RadiusY)
			{
				TArray<FVector2D> Points;
				for (int32 Segment = 0; Segment <= 24; ++Segment)
				{
					const float Angle = 2.0f * PI * Segment / 24.0f;
					Points.Add(Center + FVector2D(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY));
				}
				return Points;
			};
			Draw(Arc(11.0f, 11.0f));
			Draw(Arc(5.2f, 11.0f));
			Draw({FVector2D(2.0f, Center.Y), FVector2D(24.0f, Center.Y)});
			return LayerId;
		}

	private:
		TAttribute<FLinearColor> Color;
	};

	class SFlickPuckDisc final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickPuckDisc)
			: _TeamColor(Cyan)
			, _AccentColor(FLinearColor::White)
			, _Archetype(EFlickPieceArchetype::Standard)
			, _Selected(false)
			, _RadiusScale(1.0f)
		{}
			SLATE_ATTRIBUTE(FLinearColor, TeamColor)
			SLATE_ATTRIBUTE(FLinearColor, AccentColor)
			SLATE_ATTRIBUTE(EFlickPieceArchetype, Archetype)
			SLATE_ATTRIBUTE(bool, Selected)
			SLATE_ATTRIBUTE(float, RadiusScale)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			TeamColor = InArgs._TeamColor;
			AccentColor = InArgs._AccentColor;
			Archetype = InArgs._Archetype;
			Selected = InArgs._Selected;
			RadiusScale = InArgs._RadiusScale;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(const float LayoutScaleMultiplier) const override
		{
			return FVector2D(82.0f, 82.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			const FVector2f Center(static_cast<float>(Size.X * 0.5f), static_cast<float>(Size.Y * 0.5f));
			const float Scale = FMath::Clamp(RadiusScale.Get(), 0.78f, 1.18f);
			const float BaseRadius = FMath::Min(Size.X, Size.Y) * 0.42f * Scale;
			const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();

			const auto DrawDisc = [&OutDrawElements, &Transform, &Center](
				const int32 Layer,
				const float Radius,
				const FVector2f Offset,
				const FLinearColor& CenterColor,
				const FLinearColor& EdgeColor)
			{
				constexpr int32 SegmentCount = 48;
				TArray<FSlateVertex> Vertices;
				TArray<SlateIndex> Indices;
				Vertices.Reserve(SegmentCount + 1);
				Indices.Reserve(SegmentCount * 3);
				Vertices.Add(FSlateVertex::Make(
					Transform,
					Center + Offset,
					FVector2f(0.5f, 0.5f),
					CenterColor.ToFColor(true)));
				for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / SegmentCount;
					const FVector2f Direction(FMath::Cos(Angle), FMath::Sin(Angle));
					Vertices.Add(FSlateVertex::Make(
						Transform,
						Center + Offset + Direction * Radius,
						FVector2f(0.5f, 0.5f) + Direction * 0.5f,
						EdgeColor.ToFColor(true)));
				}
				for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
				{
					Indices.Add(0);
					Indices.Add(static_cast<SlateIndex>(Segment + 1));
					Indices.Add(static_cast<SlateIndex>((Segment + 1) % SegmentCount + 1));
				}
				FSlateDrawElement::MakeCustomVerts(
					OutDrawElements,
					Layer,
					WhiteBrush()->GetRenderingResource(),
					Vertices,
					Indices,
					nullptr,
					0,
					0);
			};
			const auto DrawRing = [&OutDrawElements, &AllottedGeometry, &Center](
				const int32 Layer,
				const float Radius,
				const FLinearColor& Color,
				const float Thickness)
			{
				constexpr int32 SegmentCount = 48;
				TArray<FVector2D> Points;
				Points.Reserve(SegmentCount + 1);
				for (int32 Segment = 0; Segment <= SegmentCount; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / SegmentCount;
					Points.Add(FVector2D(Center.X + FMath::Cos(Angle) * Radius, Center.Y + FMath::Sin(Angle) * Radius));
				}
				FSlateDrawElement::MakeLines(
					OutDrawElements,
					Layer,
					AllottedGeometry.ToPaintGeometry(),
					Points,
					ESlateDrawEffect::None,
					Color,
					true,
					Thickness);
			};
			const auto DrawArc = [&OutDrawElements, &AllottedGeometry, &Center](
				const int32 Layer,
				const float Radius,
				const float StartAngle,
				const float EndAngle,
				const FLinearColor& Color,
				const float Thickness)
			{
				constexpr int32 SegmentCount = 8;
				TArray<FVector2D> Points;
				Points.Reserve(SegmentCount + 1);
				for (int32 Segment = 0; Segment <= SegmentCount; ++Segment)
				{
					const float Alpha = static_cast<float>(Segment) / SegmentCount;
					const float Angle = FMath::Lerp(StartAngle, EndAngle, Alpha);
					Points.Add(FVector2D(Center.X + FMath::Cos(Angle) * Radius, Center.Y + FMath::Sin(Angle) * Radius));
				}
				FSlateDrawElement::MakeLines(
					OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Color, true, Thickness);
			};
			const auto DrawSymbol = [&OutDrawElements, &AllottedGeometry, LayerId](
				const TArray<FVector2D>& LocalPoints,
				const FLinearColor& Color,
				const float Thickness,
				const bool bClosed = false)
			{
				if (LocalPoints.Num() < 2)
				{
					return;
				}
				TArray<FVector2D> Points = LocalPoints;
				if (bClosed)
				{
					const FVector2D FirstPoint = Points[0];
					Points.Add(FirstPoint);
				}
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId + 13, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Color, true, Thickness);
			};

			const FLinearColor Team = TeamColor.Get();
			const EFlickPieceArchetype VisualArchetype = Archetype.Get();
			float TopScale = 0.82f;
			switch (VisualArchetype)
			{
			case EFlickPieceArchetype::Heavy: TopScale = 0.78f; break;
			case EFlickPieceArchetype::Blocker: TopScale = 0.86f; break;
			case EFlickPieceArchetype::Compact: TopScale = 0.78f; break;
			case EFlickPieceArchetype::Toppler: TopScale = 0.79f; break;
			case EFlickPieceArchetype::Standard:
			default: break;
			}
			const FLinearColor DarkMetal(0.014f, 0.023f, 0.035f, 1.0f);
			const FLinearColor MidMetal(0.065f, 0.083f, 0.105f, 1.0f);
			const FLinearColor Silver(0.52f, 0.62f, 0.7f, 1.0f);
			const FLinearColor SilverHighlight(0.86f, 0.93f, 0.98f, 1.0f);
			const FLinearColor TeamLight = FMath::Lerp(Team, FLinearColor::White, 0.1f);
			const float DetailScale = FMath::Clamp(BaseRadius / 34.0f, 0.42f, 1.6f);
			const float FineLine = FMath::Clamp(1.25f * DetailScale, 0.65f, 2.0f);

			// Soft cast and contact shadows give the icon the same physical height as the in-game mesh.
			DrawDisc(LayerId, BaseRadius * 1.08f, FVector2f(3.0f, 5.5f) * DetailScale,
				FLinearColor(0.0f, 0.0f, 0.0f, 0.30f), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			DrawDisc(LayerId + 1, BaseRadius * 0.98f, FVector2f(1.5f, 3.0f) * DetailScale,
				FLinearColor(0.0f, 0.0f, 0.0f, 0.70f), FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));

			// Keep selection readable without turning the whole puck into a flat team-colored disc.
			const FLinearColor HaloColor = Selected.Get()
				? FMath::Lerp(Team, FLinearColor::White, 0.2f).CopyWithNewOpacity(0.32f)
				: Team.CopyWithNewOpacity(0.08f);
			DrawDisc(LayerId + 2, BaseRadius + (Selected.Get() ? 4.5f : 2.0f) * DetailScale, FVector2f::ZeroVector,
				HaloColor, HaloColor.CopyWithNewOpacity(0.0f));

			// Layered sidewall, lower light channel and machined crown match the production pucks.
			DrawDisc(LayerId + 3, BaseRadius, FVector2f(0.0f, 2.4f) * DetailScale,
				MidMetal, FLinearColor(0.002f, 0.006f, 0.012f, 1.0f));
			DrawDisc(LayerId + 4, BaseRadius * 0.96f, FVector2f::ZeroVector,
				SilverHighlight, FLinearColor(0.18f, 0.25f, 0.31f, 1.0f));
			DrawRing(LayerId + 5, BaseRadius * 0.96f, TeamLight.CopyWithNewOpacity(0.94f), FineLine * 1.45f);
			DrawDisc(LayerId + 6, BaseRadius * TopScale, FVector2f::ZeroVector, DarkMetal, MidMetal);

			for (int32 WindowIndex = 0; WindowIndex < 8; ++WindowIndex)
			{
				const float CenterAngle = 2.0f * PI * static_cast<float>(WindowIndex) / 8.0f;
				DrawArc(LayerId + 7, BaseRadius * 0.87f, CenterAngle - 0.22f, CenterAngle + 0.22f,
					TeamLight, FineLine * 2.8f);
			}
			DrawRing(LayerId + 8, BaseRadius * (TopScale - 0.09f), Silver.CopyWithNewOpacity(0.9f), FineLine * 0.85f);
			DrawDisc(LayerId + 9, BaseRadius * (TopScale - 0.15f), FVector2f::ZeroVector, MidMetal, DarkMetal);
			DrawRing(LayerId + 10, BaseRadius * (TopScale - 0.15f), TeamLight.CopyWithNewOpacity(0.98f), FineLine * 1.5f);

			const FVector2D SymbolCenter(Center.X, Center.Y);
			const float SymbolRadius = BaseRadius * 0.22f;
			switch (VisualArchetype)
			{
			case EFlickPieceArchetype::Standard:
				DrawRing(LayerId + 13, SymbolRadius * 0.72f, TeamLight, FineLine * 1.9f);
				break;
			case EFlickPieceArchetype::Toppler:
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, SymbolRadius * 0.05f), SymbolCenter + FVector2D(0.0f, -SymbolRadius), SymbolCenter + FVector2D(SymbolRadius, SymbolRadius * 0.05f)}, TeamLight, FineLine * 2.0f);
				DrawSymbol({SymbolCenter + FVector2D(0.0f, -SymbolRadius), SymbolCenter + FVector2D(0.0f, SymbolRadius * 0.55f)}, TeamLight, FineLine * 2.0f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.42f, SymbolRadius * 0.82f), SymbolCenter + FVector2D(SymbolRadius * 0.42f, SymbolRadius * 0.82f)}, TeamLight, FineLine * 2.0f);
				break;
			case EFlickPieceArchetype::Bouncer:
				DrawDisc(LayerId + 13, SymbolRadius * 0.22f, FVector2f(0.0f, -SymbolRadius * 0.42f), TeamLight, TeamLight);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, SymbolRadius * 0.05f), SymbolCenter + FVector2D(-SymbolRadius * 0.48f, SymbolRadius * 0.72f), SymbolCenter + FVector2D(0.0f, SymbolRadius * 0.34f), SymbolCenter + FVector2D(SymbolRadius * 0.48f, SymbolRadius * 0.72f), SymbolCenter + FVector2D(SymbolRadius, SymbolRadius * 0.05f)}, TeamLight, FineLine * 1.75f);
				break;
			case EFlickPieceArchetype::Compact:
				DrawRing(LayerId + 13, SymbolRadius * 0.75f, TeamLight, FineLine * 1.7f);
				DrawDisc(LayerId + 13, SymbolRadius * 0.24f, FVector2f::ZeroVector, TeamLight, TeamLight);
				break;
			case EFlickPieceArchetype::Blocker:
			{
				TArray<FVector2D> Hexagon;
				for (int32 PointIndex = 0; PointIndex < 6; ++PointIndex)
				{
					const float Angle = -PI * 0.5f + 2.0f * PI * static_cast<float>(PointIndex) / 6.0f;
					Hexagon.Add(SymbolCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * SymbolRadius);
				}
				DrawSymbol(Hexagon, TeamLight, FineLine * 1.9f, true);
				break;
			}
			case EFlickPieceArchetype::Slider:
				for (int32 ChevronIndex = -1; ChevronIndex <= 1; ++ChevronIndex)
				{
					const float X = ChevronIndex * SymbolRadius * 0.64f;
					DrawSymbol({SymbolCenter + FVector2D(X - SymbolRadius * 0.35f, -SymbolRadius * 0.68f), SymbolCenter + FVector2D(X + SymbolRadius * 0.25f, 0.0f), SymbolCenter + FVector2D(X - SymbolRadius * 0.35f, SymbolRadius * 0.68f)}, TeamLight, FineLine * 1.8f);
				}
				break;
			case EFlickPieceArchetype::Grippy:
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, SymbolRadius * 0.72f), SymbolCenter + FVector2D(-SymbolRadius * 0.18f, -SymbolRadius * 0.65f), SymbolCenter + FVector2D(SymbolRadius * 0.16f, -SymbolRadius * 0.12f), SymbolCenter + FVector2D(SymbolRadius * 0.5f, -SymbolRadius * 0.52f), SymbolCenter + FVector2D(SymbolRadius, SymbolRadius * 0.72f)}, TeamLight, FineLine * 1.9f);
				break;
			case EFlickPieceArchetype::Striker:
				DrawRing(LayerId + 13, SymbolRadius * 0.55f, TeamLight, FineLine * 1.55f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, 0.0f), SymbolCenter + FVector2D(-SymbolRadius * 0.46f, 0.0f)}, TeamLight, FineLine * 1.7f);
				DrawSymbol({SymbolCenter + FVector2D(SymbolRadius * 0.46f, 0.0f), SymbolCenter + FVector2D(SymbolRadius, 0.0f)}, TeamLight, FineLine * 1.7f);
				DrawSymbol({SymbolCenter + FVector2D(0.0f, -SymbolRadius), SymbolCenter + FVector2D(0.0f, -SymbolRadius * 0.46f)}, TeamLight, FineLine * 1.7f);
				DrawSymbol({SymbolCenter + FVector2D(0.0f, SymbolRadius * 0.46f), SymbolCenter + FVector2D(0.0f, SymbolRadius)}, TeamLight, FineLine * 1.7f);
				DrawDisc(LayerId + 13, SymbolRadius * 0.16f, FVector2f::ZeroVector, TeamLight, TeamLight);
				break;
			case EFlickPieceArchetype::Heavy:
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.65f, 0.0f), SymbolCenter + FVector2D(SymbolRadius * 0.65f, 0.0f)}, TeamLight, FineLine * 2.2f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.7f, -SymbolRadius * 0.62f), SymbolCenter + FVector2D(-SymbolRadius * 0.7f, SymbolRadius * 0.62f)}, TeamLight, FineLine * 3.0f);
				DrawSymbol({SymbolCenter + FVector2D(SymbolRadius * 0.7f, -SymbolRadius * 0.62f), SymbolCenter + FVector2D(SymbolRadius * 0.7f, SymbolRadius * 0.62f)}, TeamLight, FineLine * 3.0f);
				break;
			default:
				break;
			}

			// A short cool specular stroke prevents the dark metal from reading as a flat black circle.
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 15,
				AllottedGeometry.ToPaintGeometry(),
				{FVector2D(Center.X - BaseRadius * 0.47f, Center.Y - BaseRadius * 0.48f),
				 FVector2D(Center.X + BaseRadius * 0.05f, Center.Y - BaseRadius * 0.69f)},
				ESlateDrawEffect::None,
				FLinearColor(0.66f, 0.88f, 1.0f, 0.42f),
				true,
				FineLine * 0.72f);
			return LayerId + 15;
		}

	private:
		TAttribute<FLinearColor> TeamColor;
		TAttribute<FLinearColor> AccentColor;
		TAttribute<EFlickPieceArchetype> Archetype;
		TAttribute<bool> Selected;
		TAttribute<float> RadiusScale;
	};

	class SFlickAngularBorder final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickAngularBorder)
			: _BackgroundColor(Panel)
			, _AccentColor(Hairline)
				, _CutSize(13.0f)
				, _BorderWidth(1.0f)
				, _DrawNeutralOutline(true)
				, _UseAccentForOutline(false)
				, _Padding(FMargin(0.0f))
		{}
			SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
			SLATE_ATTRIBUTE(FLinearColor, AccentColor)
				SLATE_ARGUMENT(float, CutSize)
				SLATE_ARGUMENT(float, BorderWidth)
				SLATE_ARGUMENT(bool, DrawNeutralOutline)
				SLATE_ARGUMENT(bool, UseAccentForOutline)
				SLATE_ARGUMENT(FMargin, Padding)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			BackgroundColor = InArgs._BackgroundColor;
			AccentColor = InArgs._AccentColor;
			CutSize = InArgs._CutSize;
			BorderWidth = InArgs._BorderWidth;
			bDrawNeutralOutline = InArgs._DrawNeutralOutline;
			bUseAccentForOutline = InArgs._UseAccentForOutline;
			ChildSlot.Padding(InArgs._Padding)[InArgs._Content.Widget];
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			if (Size.X < 2.0f || Size.Y < 2.0f) return LayerId;
			const float Cut = FMath::Clamp(CutSize, 0.0f, FMath::Min(Size.X, Size.Y) * 0.24f);
			const TArray<FVector2D> Points = {
				{0.0f, 0.0f}, {Size.X - Cut, 0.0f}, {Size.X, Cut},
				{Size.X, Size.Y}, {Cut, Size.Y}, {0.0f, Size.Y - Cut}};
			const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
			const FLinearColor Fill = BackgroundColor.Get() * Tint;
			const FLinearColor Accent = AccentColor.Get() * Tint;
			const auto& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			for (const FVector2D& Point : Points)
			{
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Point), FVector2f::ZeroVector, Fill.ToFColor(true)));
			}
			for (int32 Index = 1; Index + 1 < Points.Num(); ++Index)
			{
				Indices.Append({0, static_cast<SlateIndex>(Index), static_cast<SlateIndex>(Index + 1)});
			}
			const ESlateDrawEffect Effect = bParentEnabled && IsEnabled() ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId,
				WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0, Effect);
			if (bDrawNeutralOutline)
			{
				TArray<FVector2D> Outline = Points;
				Outline.Add(FVector2D::ZeroVector);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
					Outline, Effect, bUseAccentForOutline ? Accent : Hairline * Tint, true, BorderWidth);
			}
			if (Accent.A > KINDA_SMALL_NUMBER)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
					{FVector2D(0.0f, 0.0f), FVector2D(FMath::Min(Size.X - Cut, 36.0f), 0.0f)},
					Effect, Accent, false, FMath::Max(2.0f, BorderWidth));
			}
			return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
				OutDrawElements, LayerId + 2, InWidgetStyle, bParentEnabled);
		}

	private:
		TAttribute<FLinearColor> BackgroundColor;
		TAttribute<FLinearColor> AccentColor;
		float CutSize = 13.0f;
		float BorderWidth = 1.0f;
		bool bDrawNeutralOutline = true;
		bool bUseAccentForOutline = false;
	};

	class SFlickShowcaseProgress final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickShowcaseProgress) {}
			SLATE_ATTRIBUTE(float, Percent)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Percent = InArgs._Percent;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(const float LayoutScaleMultiplier) const override
		{
			return FVector2D(1.0f, 18.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				WhiteBrush(),
				ESlateDrawEffect::None,
				FLinearColor(0.055f, 0.055f, 0.05f, 0.92f));

			const float FillWidth = Size.X * FMath::Clamp(Percent.Get(), 0.0f, 1.0f);
			if (FillWidth > 0.5f)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId + 1,
					AllottedGeometry.ToPaintGeometry(FVector2D(FillWidth, Size.Y), FSlateLayoutTransform()),
					WhiteBrush(),
					ESlateDrawEffect::None,
					FLinearColor(0.0f, 0.74f, 0.94f, 0.96f));
			}
			return LayerId + 1;
		}

	private:
		TAttribute<float> Percent;
	};

	class SFlickRadarChart final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickRadarChart)
			: _AccentColor(Cyan)
		{}
			SLATE_ARGUMENT(TFunction<TArray<float>()>, ValueProvider)
			SLATE_ATTRIBUTE(FLinearColor, AccentColor)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ValueProvider = InArgs._ValueProvider;
			AccentColor = InArgs._AccentColor;
			SetCanTick(false);
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(240.0f, 210.0f);
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			const int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			const bool bParentEnabled) const override
		{
			constexpr int32 AxisCount = 6;
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			const FVector2D Center(Size.X * 0.5f, Size.Y * 0.51f);
			const float Radius = FMath::Max(1.0f, FMath::Min(Size.X, Size.Y) * 0.39f);
			const auto PointAt = [&Center](const float RadiusAtPoint, const int32 Axis)
			{
				const float Angle = -PI * 0.5f + 2.0f * PI * static_cast<float>(Axis) / AxisCount;
				return Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RadiusAtPoint;
			};

			for (int32 Ring = 1; Ring <= 4; ++Ring)
			{
				TArray<FVector2D> RingPoints;
				for (int32 Axis = 0; Axis < AxisCount; ++Axis) RingPoints.Add(PointAt(Radius * Ring / 4.0f, Axis));
				const FVector2D FirstRingPoint = RingPoints[0];
				RingPoints.Add(FirstRingPoint);
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), RingPoints,
					ESlateDrawEffect::None, FLinearColor(0.13f, 0.3f, 0.39f, Ring == 4 ? 0.68f : 0.34f), true, 1.0f);
			}
			for (int32 Axis = 0; Axis < AxisCount; ++Axis)
			{
				TArray<FVector2D> AxisLine = {Center, PointAt(Radius, Axis)};
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), AxisLine,
					ESlateDrawEffect::None, FLinearColor(0.1f, 0.25f, 0.34f, 0.42f), true, 1.0f);
			}

			TArray<float> Values = ValueProvider ? ValueProvider() : TArray<float>();
			Values.SetNum(AxisCount);
			const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			const FLinearColor Accent = AccentColor.Get();
			Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Center), FVector2f(0.5f, 0.5f), Accent.CopyWithNewOpacity(0.34f).ToFColor(true)));
			TArray<FVector2D> Profile;
			for (int32 Axis = 0; Axis < AxisCount; ++Axis)
			{
				const FVector2D Point = PointAt(Radius * FMath::Clamp(Values[Axis], 0.06f, 1.0f), Axis);
				Profile.Add(Point);
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Point), FVector2f(0.5f, 0.5f), Accent.CopyWithNewOpacity(0.34f).ToFColor(true)));
			}
			for (int32 Axis = 0; Axis < AxisCount; ++Axis)
			{
				Indices.Add(0);
				Indices.Add(static_cast<SlateIndex>(Axis + 1));
				Indices.Add(static_cast<SlateIndex>((Axis + 1) % AxisCount + 1));
			}
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements, LayerId + 1, WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0);
			const FVector2D FirstProfilePoint = Profile[0];
			Profile.Add(FirstProfilePoint);
			FSlateDrawElement::MakeLines(
				OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Profile,
				ESlateDrawEffect::None, Accent, true, 2.4f);
			return LayerId + 2;
		}

	private:
		TFunction<TArray<float>()> ValueProvider;
		TAttribute<FLinearColor> AccentColor;
	};
}

SFlickGameLayer::SFlickGameLayer()
{
	MenuButtonStyle = FButtonStyle()
		.SetNormal(FSlateColorBrush(Panel))
		.SetHovered(FSlateColorBrush(PanelRaised))
		.SetPressed(FSlateColorBrush(FLinearColor(0.0f, 0.46f, 0.68f, 1.0f)))
		.SetNormalPadding(FMargin(21.0f, 10.0f))
		.SetPressedPadding(FMargin(24.0f, 11.0f, 18.0f, 9.0f));
	PrimaryButtonStyle = FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.085f, 0.14f, 0.02f, 1.0f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.15f, 0.24f, 0.025f, 1.0f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.0f, 0.78f, 1.0f, 1.0f)))
		.SetNormalPadding(FMargin(21.0f, 10.0f))
		.SetPressedPadding(FMargin(24.0f, 11.0f, 18.0f, 9.0f));
	DangerButtonStyle = FButtonStyle()
		.SetNormal(FSlateColorBrush(FLinearColor(0.1f, 0.018f, 0.014f, 0.96f)))
		.SetHovered(FSlateColorBrush(FLinearColor(0.58f, 0.055f, 0.018f, 1.0f)))
		.SetPressed(FSlateColorBrush(FLinearColor(0.92f, 0.1f, 0.02f, 1.0f)))
		.SetNormalPadding(FMargin(21.0f, 10.0f))
		.SetPressedPadding(FMargin(24.0f, 11.0f, 18.0f, 9.0f));
	CompactMenuButtonStyle = MenuButtonStyle;
	CompactMenuButtonStyle
		.SetNormalPadding(FMargin(8.0f, 4.0f))
		.SetPressedPadding(FMargin(10.0f, 5.0f, 6.0f, 3.0f));
	CompactPrimaryButtonStyle = PrimaryButtonStyle;
	CompactPrimaryButtonStyle
		.SetNormalPadding(FMargin(8.0f, 4.0f))
		.SetPressedPadding(FMargin(10.0f, 5.0f, 6.0f, 3.0f));
	CompactDangerButtonStyle = DangerButtonStyle;
	CompactDangerButtonStyle
		.SetNormalPadding(FMargin(8.0f, 4.0f))
		.SetPressedPadding(FMargin(10.0f, 5.0f, 6.0f, 3.0f));
	TransparentButtonStyle = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f));
	ToggleStyle = FCheckBoxStyle()
		.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
		.SetUncheckedImage(FSlateColorBrush(Ink))
		.SetUncheckedHoveredImage(FSlateColorBrush(FLinearColor(0.035f, 0.16f, 0.22f, 1.0f)))
		.SetUncheckedPressedImage(FSlateColorBrush(FLinearColor(0.02f, 0.3f, 0.4f, 1.0f)))
		.SetCheckedImage(FSlateColorBrush(Brand))
		.SetCheckedHoveredImage(FSlateColorBrush(Paper))
		.SetCheckedPressedImage(FSlateColorBrush(FLinearColor(0.0f, 0.42f, 0.62f, 1.0f)))
		.SetPadding(FMargin(12.0f, 5.0f));
	SliderStyle = FSliderStyle()
		.SetNormalBarImage(FSlateColorBrush(FLinearColor(0.05f, 0.1f, 0.135f, 1.0f)))
		.SetHoveredBarImage(FSlateColorBrush(FLinearColor(0.07f, 0.18f, 0.24f, 1.0f)))
		.SetNormalThumbImage(FSlateColorBrush(Paper))
		.SetHoveredThumbImage(FSlateColorBrush(Brand))
		.SetBarThickness(5.0f);
	SliderStyle.NormalThumbImage.ImageSize = FVector2D(12.0f, 22.0f);
	SliderStyle.HoveredThumbImage.ImageSize = FVector2D(12.0f, 22.0f);
	SliderStyle.SetNormalBarImage(FSlateNoResource()).SetHoveredBarImage(FSlateNoResource());
	ShotClockBarStyle = FProgressBarStyle()
		.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.018f, 0.035f, 0.05f, 1.0f)))
		.SetFillImage(FSlateColorBrush(FLinearColor::White))
		.SetMarqueeImage(FSlateNoResource());
}

void SFlickGameLayer::Construct(const FArguments& InArgs)
{
	OwnerHud = InArgs._OwnerHud;
	GameMode = InArgs._GameMode;
	PlayerController = InArgs._PlayerController;
	if (const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
		? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
		: nullptr)
	{
		bStartupOverlayVisible = FlickGameInstance->ShouldShowStartupPresentation()
			&& GameMode.IsValid()
			&& GameMode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu;
	}

#if !UE_BUILD_SHIPPING
	bSocialPanelOpen = FParse::Param(FCommandLine::Get(), TEXT("FlickSocialPreview"));
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickPlayFormatPreview")))
	{
		SelectedPlayPlaylist = EFlickPlayPlaylist::Casual;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickLoadoutComparePreview")))
	{
		Player1HoveredLoadoutArchetype = EFlickPieceArchetype::Heavy;
	}
#endif

	ChildSlot
	[
		SNew(SScaleBox)
		.Stretch(EStretch::ScaleToFit)
		[
		SNew(SBox)
		.WidthOverride(FlickUITheme::ReferenceWidth)
		.HeightOverride(FlickUITheme::ReferenceHeight)
		[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SAssignNew(MainMenuScreenWidget, SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::MainMenu); })
			[
				BuildMainMenu()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::ModeSelect); })
			[
				BuildModeSelect()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::NetworkLobby); })
			[
				BuildNetworkLobby()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::PrivateMatch); })
			[
				BuildPrivateMatchSetup()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::OnlineBrowser); })
			[
				BuildOnlineBrowser()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::ItemShop); })
			[
				BuildItemShop()
			]
		]

		+ SOverlay::Slot()
		[
			SAssignNew(ProfileScreenWidget, SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Profile); })
			[
				BuildProfile()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Loadout); })
			[
				BuildLoadout()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::ClassSelect); })
			[
				BuildClassSelect()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Settings); })
			[
				BuildSettings()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetMatchHudVisibility(); })
			[
				BuildMatchHud()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				return GameMode.IsValid() && GameMode->IsCinematicReplayActive()
					? EVisibility::HitTestInvisible
					: EVisibility::Collapsed;
			})
			[
				BuildCinematicReplayOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScoreboardVisibility(); })
			[
				BuildScoreboardOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetRoundOverVisibility(); })
			[
				BuildRoundOverOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GetScreenVisibility(EFlickFrontendScreen::Paused); })
			[
				BuildPauseOverlay()
			]
		]

		+ SOverlay::Slot()
		[
			SAssignNew(StartupOverlayWidget, SBox)
			.Visibility_Lambda([this]() { return bStartupOverlayVisible ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				BuildStartupOverlay()
			]
		]
		]
		]
	];

}

void SFlickGameLayer::Tick(
	const FGeometry& AllottedGeometry,
	const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (bStartupOverlayVisible)
	{
		StartupOverlayElapsed += InDeltaTime;
		if (StartupOverlayWidget.IsValid())
		{
			const float FadeAlpha = StartupOverlayElapsed <= StartupOverlayHoldDuration
				? 1.0f
				: 1.0f - FMath::Clamp(
					(StartupOverlayElapsed - StartupOverlayHoldDuration)
					/ FMath::Max(StartupOverlayFadeDuration, KINDA_SMALL_NUMBER),
					0.0f,
					1.0f);
			StartupOverlayWidget->SetRenderOpacity(FadeAlpha);
		}
		if (StartupOverlayElapsed >= StartupOverlayHoldDuration + StartupOverlayFadeDuration)
		{
			bStartupOverlayVisible = false;
			bHasAppliedInitialFocus = false;
			if (UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
				? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
				: nullptr)
			{
				FlickGameInstance->CompleteStartupPresentation();
			}
		}
	}
	if (!GameMode.IsValid())
	{
		return;
	}

	const EFlickFrontendScreen CurrentScreen = GameMode->GetFrontendScreen();
	if (CurrentScreen == EFlickFrontendScreen::Loadout
		&& LastFocusedScreen != EFlickFrontendScreen::Loadout)
	{
		Player1SelectedLoadoutSlot = 0;
		Player2SelectedLoadoutSlot = 0;
		Player1HoveredLoadoutArchetype.Reset();
		Player2HoveredLoadoutArchetype.Reset();
	}

	LastFocusedScreen = CurrentScreen;
}

TSharedRef<SWidget> SFlickGameLayer::BuildStartupOverlay()
{
	// The movie-player splash owns the startup logo. Once the frontend is ready,
	// only fade its dark backdrop so a second copy cannot linger over the menu.
	return SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 1.0f));
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenu()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.08f))
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility(EVisibility::HitTestInvisible)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([this]()
			{
				const float Opacity = GameMode.IsValid()
					? GameMode->GetMenuPreviewTransitionOpacity()
					: 0.0f;
				return FLinearColor(0.0f, 0.006f, 0.012f, Opacity);
			})
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility(EVisibility::HitTestInvisible)
			.Opacity(0.9f)
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickDiagonalPanel)
			.Visibility(EVisibility::HitTestInvisible)
			.PanelColor(Ink)
			.EdgeColor(Hairline)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(52.0f, 62.0f, 0.0f, 0.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(TEXT("PHYSICS. PRECISION. RIVALRY."))).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			+ SVerticalBox::Slot().AutoHeight().Padding(-5.0f, 0.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK"))).Font(DisplayFont(116, true)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("Small move. Big consequences."))).Font(UiFont(16)).ColorAndOpacity(Muted)
				.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->GetFrontendScreen() == EFlickFrontendScreen::Profile ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
		]

		+ SOverlay::Slot()
		[
			SAssignNew(MainMenuInteractiveWidget, SOverlay)
			.Visibility_Lambda([this]()
			{
				if (!GameMode.IsValid()) return EVisibility::Collapsed;
				if (GameMode->GetFrontendScreen() == EFlickFrontendScreen::MainMenu) return EVisibility::Visible;
				return EVisibility::Collapsed;
			})
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(52.0f, 298.0f, 0.0f, 0.0f)
		[
			SAssignNew(MainMenuSelectionWidget, SBox)
			.WidthOverride(450.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(0))
					[
						MakeMainMenuButton(TEXT("PLAY"), FOnClicked::CreateLambda([this]()
						{
							SelectedPlayPlaylist = EFlickPlayPlaylist::None;
							SelectedTrainingActivity = EFlickTrainingActivity::None;
							if (GameMode.IsValid()) GameMode->OpenModeSelect();
							return FReply::Handled();
						}), true, false, MainMenuStackMetrics::GetRowHeight(0), &MainMenuDefaultButton)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(1), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox)
					.WidthOverride(MainMenuStackMetrics::GetRowWidth(1))
					[
						MakeMainMenuButton(TEXT("LINEUPS"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->OpenLoadout();
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(1))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(2), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(2))
					[
						MakeMainMenuButton(TEXT("PROFILE"), FOnClicked::CreateLambda([this]()
						{
							SelectedProfileTab = EFlickProfileTab::Stats;
							if (GameMode.IsValid()) GameMode->OpenProfile();
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(2))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(3), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(3))
					[
						MakeMainMenuButton(TEXT("ITEM SHOP"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->OpenItemShop();
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(3))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(4), 0.0f, 0.0f, MainMenuStackMetrics::Gap)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(4))
					[
						MakeMainMenuButton(TEXT("SETTINGS"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->OpenSettings();
							return FReply::Handled();
						}), false, false, MainMenuStackMetrics::GetRowHeight(4))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(MainMenuStackMetrics::GetRowLeft(5), 0.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(MainMenuStackMetrics::GetRowWidth(5))
					[
						MakeMainMenuButton(TEXT("QUIT"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->QuitGame();
							return FReply::Handled();
						}), false, true, MainMenuStackMetrics::GetRowHeight(5))
					]
				]
				]
			]
				+ SOverlay::Slot()
			.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(0.0f, 0.0f, 52.0f, 124.0f)
		[
			SNew(SBox)
			.WidthOverride(500.0f)
			.HeightOverride(186.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Ink)
				.AccentColor(Brand)
				.CutSize(10.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(34.0f, 22.0f, 34.0f, 20.0f))
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("ON THE TABLE")))
								.Font(UiFont(14, true))
								.ColorAndOpacity(FLinearColor(0.72f, 0.77f, 0.82f, 0.96f))
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("PREVIEW  /  01")))
								.Font(UiFont(15, true))
								.ColorAndOpacity(Brand)
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								const int32 TeamSize = GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1;
								return FText::FromString(Variant == EFlickMatchVariant::Bob
									? TEXT("BOB")
									: FString::Printf(TEXT("%dV%d KNOCKOUT"), TeamSize, TeamSize));
							})
						.Font(DisplayFont(38))
						.ColorAndOpacity(Paper)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								return FText::FromString(Variant == EFlickMatchVariant::Bob
									? GetMatchVariantSummary(Variant)
									: TEXT("Hold your ground. Knock out the opposition."));
							})
						.Font(UiFont(13))
						.ColorAndOpacity(FLinearColor(0.67f, 0.72f, 0.78f, 0.96f))
						.AutoWrapText(false)
					]
					+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Bottom).Padding(0.0f, 15.0f, 0.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(18.0f)
						[
							SNew(SFlickShowcaseProgress)
							.Percent_Lambda([this]() { return GameMode.IsValid() ? GameMode->GetMenuPreviewAlpha() : 0.0f; })
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.VAlign(VAlign_Bottom)
		.Padding(38.0f, 0.0f, 48.0f, 31.0f)
		[
			SNew(SBox).HeightOverride(59.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Ink)
				.AccentColor(FLinearColor(0.16f, 0.38f, 0.5f, 0.45f))
				.CutSize(3.0f)
				.BorderWidth(0.7f)
				.Padding(FMargin(34.0f, 0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 11.0f, 0.0f)
					[
						SNew(SFlickStatusGlobe).Color(Cyan.CopyWithNewOpacity(0.94f))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("READY TO FLICK"))).Font(UiFont(11, true)).ColorAndOpacity(Brand)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(28.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(0)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(1)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(2)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(3)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(4)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)[BuildMainMenuPartyMember(5)]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("TABLE RULES"))).Font(UiFont(10, true)).ColorAndOpacity(FLinearColor(0.84f, 0.89f, 0.93f, 0.94f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(11.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(10, true)).ColorAndOpacity(Orange)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								const int32 TeamSize = GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1;
								return FText::FromString(Variant == EFlickMatchVariant::Bob
									? TEXT("BOB")
									: FString::Printf(TEXT("%dV%d KNOCKOUT"), TeamSize, TeamSize));
							})
							.Font(UiFont(10, true)).ColorAndOpacity(FLinearColor(0.84f, 0.89f, 0.93f, 0.94f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(11.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(10, true)).ColorAndOpacity(Orange)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								return FText::FromString(Variant == EFlickMatchVariant::Bob
									? TEXT("POCKET YOUR COLOR") : TEXT("4 PUCKS PER PLAYER"));
							})
							.Font(UiFont(10, true)).ColorAndOpacity(FLinearColor(0.84f, 0.89f, 0.93f, 0.94f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(11.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
								return FText::FromString(Variant == EFlickMatchVariant::Bob
									? TEXT("STANDARD PUCKS ONLY") : TEXT("BEST OF 5"));
							})
							.Font(UiFont(10, true)).ColorAndOpacity(FLinearColor(0.84f, 0.89f, 0.93f, 0.94f))
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 28.0f, 270.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(390.0f).HeightOverride(72.0f)
			.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel)
				.AccentColor(Hairline)
				.UseAccentForOutline(false)
				.CutSize(12.0f)
				.BorderWidth(1.3f)
				.Padding(FMargin(20.0f, 9.0f, 12.0f, 9.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
						[
							SNew(SFlickMainMenuIcon)
							.Icon(EFlickMainMenuIcon::Rank)
							.Color(Brand)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									if (!GameMode.IsValid()) return FText::FromString(TEXT("CURRENT RANK"));
									const EFlickMatchVariant Variant = GameMode->GetActiveMatchVariant();
									return FText::FromString(Variant == EFlickMatchVariant::Bob
										? TEXT("BOB RANK")
										: FString::Printf(TEXT("%dV%d CURRENT RANK"), GameMode->GetPlayersPerTeam(), GameMode->GetPlayersPerTeam()));
								})
								.Font(UiFont(11, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
									const EFlickMatchVariant Variant = GameMode.IsValid()
										? GameMode->GetActiveMatchVariant() : EFlickMatchVariant::Classic;
									const int32 TeamSize = GameMode.IsValid() ? GameMode->GetPlayersPerTeam() : 1;
									return FText::FromString(Ranking
										? Ranking->GetProgressLabel(Variant, TeamSize)
										: TEXT("UNAVAILABLE"));
								})
								.Font(UiFont(14, true)).ColorAndOpacity(Brand)
							]
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 28.0f, 34.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(220.0f).HeightOverride(72.0f)
			.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel)
				.AccentColor(FLinearColor(0.43f, 0.8f, 0.88f, 0.82f))
				.UseAccentForOutline(false)
				.CutSize(12.0f)
				.BorderWidth(1.3f)
				.Padding(FMargin(1.5f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.ContentPadding(FMargin(14.0f, 8.0f))
					.OnClicked_Lambda([this]()
				{
					bSocialPanelOpen = !bSocialPanelOpen;
					if (bSocialPanelOpen)
					{
						if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr)
						{
							Sessions->RefreshFriends();
						}
					}
					return FReply::Handled();
				})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 17.0f, 0.0f)
						[
							SNew(SFlickMainMenuIcon)
							.Icon(EFlickMainMenuIcon::Social)
							.Color(FLinearColor(0.7f, 0.77f, 0.82f, 1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SOCIAL")))
							.Font(UiFont(17, true))
							.ColorAndOpacity(FLinearColor::White)
						]
					]
				]
			]
			]
			+ SOverlay::Slot()
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.ContentPadding(FMargin(0.0f))
				.Visibility_Lambda([this]() { return bSocialPanelOpen ? EVisibility::Visible : EVisibility::Collapsed; })
				.ToolTipText(FText::FromString(TEXT("Close social panel")))
				.OnClicked_Lambda([this]()
				{
					bSocialPanelOpen = false;
					return FReply::Handled();
				})
				[
					SNew(SSpacer)
				]
			]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(420.0f)
			.Visibility_Lambda([this]()
			{
				return bSocialPanelOpen ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				BuildSocialPanel()
			]
		]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfileStatsPanel()
{
	auto MakeStatCell = [this](const FString& Label, const FString& Detail, const int32 StatIndex, const FLinearColor& Accent) -> TSharedRef<SWidget>
	{
		return SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Hairline)
			.CutSize(9.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(18.0f, 11.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(UiFont(10, true))
					.ColorAndOpacity(Brand)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, StatIndex]()
					{
						const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
							? Cast<UFlickGameInstance>(PlayerController->GetGameInstance())
							: nullptr;
						if (!FlickGameInstance)
						{
							return FText::AsNumber(0);
						}
						const FFlickProfileStats& Stats = FlickGameInstance->GetProfileStats();
						switch (StatIndex)
						{
						case 0: return FText::AsNumber(Stats.MatchesPlayed);
						case 1: return FText::AsNumber(Stats.Wins);
						case 2: return FText::AsNumber(Stats.Points);
						case 3: return FText::AsNumber(Stats.Knockouts);
						case 4: return FText::AsNumber(Stats.DoubleKnockouts);
						case 5: return FText::AsNumber(Stats.Shots);
						case 6: return FText::AsNumber(Stats.GetTotalAccolades());
						case 7: return FText::AsNumber(Stats.GetAccoladeCount(EFlickAccolade::TeamWipeout));
						case 8: return FText::AsNumber(Stats.GetAccoladeCount(EFlickAccolade::FlawlessRound));
						default: return FText::AsNumber(0);
						}
					})
					.Font(DisplayFont(38))
					.ColorAndOpacity(Paper)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Detail))
					.Font(UiFont(10))
					.ColorAndOpacity(Muted)
				]
			];
	};

	return SNew(SFlickAngularBorder)
		.BackgroundColor(Panel)
		.AccentColor(Hairline)
		.UseAccentForOutline(false)
		.CutSize(14.0f)
		.BorderWidth(1.1f)
		.Padding(FMargin(22.0f, 17.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CAREER OVERVIEW")))
					.Font(DisplayFont(24))
					.ColorAndOpacity(Paper)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UFlickGameInstance* FlickGameInstance = PlayerController.IsValid()
							? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
						const FFlickProfileStats Stats = FlickGameInstance ? FlickGameInstance->GetProfileStats() : FFlickProfileStats();
						const int32 WinRate = Stats.MatchesPlayed > 0
							? FMath::RoundToInt(100.0f * Stats.Wins / static_cast<float>(Stats.MatchesPlayed)) : 0;
						return FText::FromString(FString::Printf(TEXT("ALL MODES  //  %d%% WIN RATE"), WinRate));
					})
					.Font(UiFont(9, true))
					.ColorAndOpacity(Brand)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 11.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f))
				+ SUniformGridPanel::Slot(0, 0)[MakeStatCell(TEXT("MATCHES PLAYED"), TEXT("COMPLETED SERIES"), 0, Cyan)]
				+ SUniformGridPanel::Slot(1, 0)[MakeStatCell(TEXT("MATCH WINS"), TEXT("ALL PLAYLISTS"), 1, FLinearColor(0.26f, 0.9f, 0.58f, 1.0f))]
				+ SUniformGridPanel::Slot(2, 0)[MakeStatCell(TEXT("CAREER POINTS"), TEXT("SHOTS, IMPACTS AND KOS"), 2, Orange)]
				+ SUniformGridPanel::Slot(0, 1)[MakeStatCell(TEXT("KNOCKOUTS"), TEXT("OPPONENT PUCKS REMOVED"), 3, Cyan)]
				+ SUniformGridPanel::Slot(1, 1)[MakeStatCell(TEXT("DOUBLE KOS"), TEXT("TWO PUCKS IN ONE SHOT"), 4, Orange)]
				+ SUniformGridPanel::Slot(2, 1)[MakeStatCell(TEXT("SHOTS TAKEN"), TEXT("ALL COMPLETED MATCHES"), 5, FLinearColor(0.68f, 0.77f, 0.86f, 1.0f))]
				+ SUniformGridPanel::Slot(0, 2)[MakeStatCell(TEXT("STYLE ACCOLADES"), TEXT("SPECIAL SHOTS AND PLAYS"), 6, FLinearColor(0.72f, 0.48f, 0.96f, 1.0f))]
				+ SUniformGridPanel::Slot(1, 2)[MakeStatCell(TEXT("TEAM WIPEOUTS"), TEXT("MULTI-KO ROUND FINISHES"), 7, Orange)]
				+ SUniformGridPanel::Slot(2, 2)[MakeStatCell(TEXT("FLAWLESS ROUNDS"), TEXT("NO FRIENDLY PUCKS LOST"), 8, Cyan)]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildProfile()
{
	auto MakeRankCard = [this](const int32 PlayersPerTeam, const FString& Label) -> TSharedRef<SWidget>
	{
		return SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Brand)
			.UseAccentForOutline(false).CutSize(12.0f).BorderWidth(1.0f).Padding(FMargin(26.0f, 24.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(11, true)).ColorAndOpacity(Muted)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, PlayersPerTeam]()
					{
						const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
						return FText::FromString(Ranking
							? Ranking->GetProgressLabel(EFlickMatchVariant::Classic, PlayersPerTeam)
							: TEXT("UNAVAILABLE"));
					})
					.Font(DisplayFont(24)).ColorAndOpacity(Paper)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("RANKED KNOCKOUT  //  CURRENT SEASON"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
			];
	};

	TSharedRef<SVerticalBox> HistoryRows = SNew(SVerticalBox);
	for (int32 MatchIndex = 0; MatchIndex < 8; ++MatchIndex)
	{
		HistoryRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
		[
			SNew(SBox).HeightOverride(60.0f)
			.Visibility_Lambda([this, MatchIndex]()
			{
				const UFlickGameInstance* Instance = PlayerController.IsValid()
					? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
				return Instance && Instance->GetRecentMatches().IsValidIndex(MatchIndex) ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(PanelRaised)
				.AccentColor_Lambda([this, MatchIndex]()
				{
					const UFlickGameInstance* Instance = PlayerController.IsValid()
						? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
					if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return Hairline;
					const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
					return Record.bDraw ? FLinearColor(0.9f, 0.75f, 0.2f, 0.75f)
						: Record.bWon ? FLinearColor(0.25f, 0.9f, 0.55f, 0.78f) : Orange.CopyWithNewOpacity(0.72f);
				})
				.CutSize(7.0f).BorderWidth(0.8f).Padding(FMargin(18.0f, 9.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(82.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this, MatchIndex]()
							{
								const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
								if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return FText::GetEmpty();
								const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
								return FText::FromString(Record.bDraw ? TEXT("DRAW") : Record.bWon ? TEXT("WIN") : TEXT("LOSS"));
							})
							.Font(UiFont(12, true)).ColorAndOpacity(Paper)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, MatchIndex]()
						{
							const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
							if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return FText::GetEmpty();
							const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
							const FString Mode = Record.Variant == EFlickMatchVariant::Bob
								? TEXT("BOB") : FString::Printf(TEXT("%dV%d KNOCKOUT"), Record.PlayersPerTeam, Record.PlayersPerTeam);
							return FText::FromString(FString::Printf(TEXT("%s  //  %s"), Record.bRanked ? TEXT("COMPETITIVE") : TEXT("CASUAL"), *Mode));
						})
						.Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, MatchIndex]()
						{
							const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
							if (!Instance || !Instance->GetRecentMatches().IsValidIndex(MatchIndex)) return FText::GetEmpty();
							const FFlickProfileMatchRecord& Record = Instance->GetRecentMatches()[MatchIndex];
							return FText::FromString(FString::Printf(TEXT("%d PTS   //   %d KOS   //   %d SHOTS"), Record.Points, Record.Knockouts, Record.Shots));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
				]
			]
		];
	}

	const auto MakeTab = [this](EFlickProfileTab Tab, const FString& Label) -> TSharedRef<SWidget>
	{
		TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0.0f)
			.OnClicked_Lambda([this, Tab]() { SelectedProfileTab = Tab; return FReply::Handled(); });
		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(SNew(SBorder).BorderImage(WhiteBrush()).Padding(FMargin(18.0f, 9.0f))
			.BorderBackgroundColor_Lambda([this, Tab, WeakButton]()
			{
				const auto Pinned = WeakButton.Pin();
				return SelectedProfileTab == Tab ? Brand : Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()) ? PanelRaised : Panel;
			})
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(11, true))
			.ColorAndOpacity_Lambda([this, Tab]() { return SelectedProfileTab == Tab ? Ink : Paper; })]);
		if (Tab == EFlickProfileTab::Stats) ProfileDefaultButton = Button;
		return Button;
	};
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.004f, 0.012f, 0.02f, 0.82f))]
		+ SOverlay::Slot()[SNew(SFlickInterfaceBackdrop).Visibility(EVisibility::HitTestInvisible).Opacity(0.22f)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(42.0f)
		[
			SNew(SBox)
			.MaxDesiredWidth(1180.0f)
			.MaxDesiredHeight(760.0f)
			[
			SNew(SFlickAngularBorder)
			.BackgroundColor(Panel.CopyWithNewOpacity(0.97f))
			.AccentColor(Brand)
			.UseAccentForOutline(false)
			.CutSize(18.0f)
			.BorderWidth(1.2f)
			.Padding(FMargin(28.0f, 22.0f))
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK  /  YOUR RECORD"))).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 14.0f)
			[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE"))).Font(DisplayFont(36)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)[MakeTab(EFlickProfileTab::Stats, TEXT("STATS"))]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)[MakeTab(EFlickProfileTab::Leaderboards, TEXT("LEADERBOARDS"))]
				+ SHorizontalBox::Slot().AutoWidth()[MakeTab(EFlickProfileTab::MatchHistory, TEXT("MATCH HISTORY"))]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(500.0f)
				[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Stats ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						BuildProfileStatsPanel()
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::Leaderboards ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false)
						.CutSize(14.0f)
						.BorderWidth(1.1f)
						.Padding(FMargin(28.0f, 24.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR RANKED PLAYLISTS"))).Font(UiFont(16, true)).ColorAndOpacity(Paper)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 18.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("PLACEMENT AND MMR PROGRESS ACROSS EACH TEAM SIZE"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SUniformGridPanel).SlotPadding(FMargin(7.0f))
								+ SUniformGridPanel::Slot(0, 0)[MakeRankCard(1, TEXT("1V1 DUEL"))]
								+ SUniformGridPanel::Slot(1, 0)[MakeRankCard(2, TEXT("2V2 DOUBLES"))]
								+ SUniformGridPanel::Slot(2, 0)[MakeRankCard(3, TEXT("3V3 CHAOS"))]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(7.0f, 18.0f, 7.0f, 0.0f)
							[
								SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(Hairline).CutSize(7.0f).BorderWidth(0.8f).Padding(FMargin(20.0f, 14.0f))
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("GLOBAL PLAYER TABLES WILL APPEAR HERE WHEN THE LIVE LEADERBOARD SERVICE IS CONNECTED."))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
								]
							]
						]
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return SelectedProfileTab == EFlickProfileTab::MatchHistory ? EVisibility::Visible : EVisibility::Collapsed; })
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false)
						.CutSize(14.0f)
						.BorderWidth(1.1f)
						.Padding(FMargin(28.0f, 24.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("RECENT MATCHES"))).Font(UiFont(16, true)).ColorAndOpacity(Paper)]
								+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("LAST 8  //  STORED LOCALLY"))).Font(UiFont(9, true)).ColorAndOpacity(Brand)]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
							[
								SNew(SBox)
								.Visibility_Lambda([this]()
								{
									const UFlickGameInstance* Instance = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
									return !Instance || Instance->GetRecentMatches().IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed;
								})
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("NO COMPLETED MATCHES YET  //  FINISH A SERIES TO START YOUR HISTORY"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)
								]
							]
							+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
							[SNew(SScrollBox) + SScrollBox::Slot()[HistoryRows]]
						]
					]
				]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(160.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CloseProfile(); return FReply::Handled(); }))]]
				+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE DATA SAVES AUTOMATICALLY"))).Font(UiFont(10)).ColorAndOpacity(Muted)]
			]
			]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMainMenuPartyMember(const int32 PartySlot)
{
	return SNew(SBox)
		.WidthOverride(124.0f)
		.HeightOverride(40.0f)
		.Visibility_Lambda([this, PartySlot]()
		{
			if (GameMode.IsValid() && GameMode->IsPartySession())
			{
				return GameMode->GetPartyMember(PartySlot) ? EVisibility::Visible : EVisibility::Collapsed;
			}
			return PartySlot == 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor_Lambda([this, PartySlot]()
			{
				const AFlickPlayerState* Member = GameMode.IsValid() && GameMode->IsPartySession()
					? GameMode->GetPartyMember(PartySlot) : nullptr;
				return Member && Member->IsPartyLeader()
					? Brand
					: Hairline;
			})
			.UseAccentForOutline(false)
			.CutSize(5.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(7.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(26.0f).HeightOverride(26.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(Panel)
						.AccentColor(Hairline)
						.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(1.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%02d"), PartySlot + 1)))
							.Font(UiFont(8, true)).Justification(ETextJustify::Center).ColorAndOpacity(Brand)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PartySlot]()
						{
							if (GameMode.IsValid() && GameMode->IsPartySession())
							{
								const AFlickPlayerState* Member = GameMode->GetPartyMember(PartySlot);
								return FText::FromString(Member ? Member->GetPlayerName() : FString());
							}
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER"));
						})
						.Font(UiFont(8, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PartySlot]()
						{
							const AFlickPlayerState* Member = GameMode.IsValid() && GameMode->IsPartySession()
								? GameMode->GetPartyMember(PartySlot) : nullptr;
							return FText::FromString(Member && Member->IsPartyLeader() ? TEXT("LEADER") : TEXT("IN PARTY"));
						})
						.Font(UiFont(7, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildSocialPanel()
{
	auto MakeSocialTab = [this](const FString& Label, const bool bRecentTab) -> TSharedRef<SWidget>
	{
		return SNew(SBox).HeightOverride(38.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor_Lambda([this, bRecentTab]()
				{
					const bool bSelected = bShowingRecentPlayers == bRecentTab;
					return bSelected
						? Brand : PanelRaised;
				})
				.AccentColor_Lambda([this, bRecentTab]()
				{
					return bShowingRecentPlayers == bRecentTab ? Brand : Hairline;
				})
				.UseAccentForOutline(false)
				.CutSize(6.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(1.0f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this, bRecentTab]()
					{
						bShowingRecentPlayers = bRecentTab;
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this, Label, bRecentTab]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							const int32 Count = Sessions
								? (bRecentTab ? Sessions->GetRecentPlayers().Num() : Sessions->GetFriends().Num())
								: 0;
							return FText::FromString(FString::Printf(TEXT("%s  %d"), *Label, Count));
						})
						.Font(UiFont(11, true))
						.ColorAndOpacity_Lambda([this, bRecentTab]()
						{
							return FSlateColor(bShowingRecentPlayers == bRecentTab ? Ink : Paper);
						})
					]
				]
			];
	};
	auto MakeQueueButton = [this](const int32 PlayersPerTeam) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.HeightOverride(44.0f)
			.IsEnabled_Lambda([this, PlayersPerTeam]()
			{
				return GameMode.IsValid() && GameMode->CanQueuePartyForMatchmaking(PlayersPerTeam);
			})
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(PanelRaised)
				.AccentColor(Hairline)
				.UseAccentForOutline(false)
				.CutSize(7.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(1.0f))
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this, PlayersPerTeam]()
					{
						if (GameMode.IsValid()) GameMode->QueuePartyForMatchmaking(PlayersPerTeam);
						return FReply::Handled();
					})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(9, true)).ColorAndOpacity(Paper)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("QUEUE %dV%d"), PlayersPerTeam, PlayersPerTeam)))
							.Font(UiFont(11, true))
							.ColorAndOpacity(Paper)
						]
					]
				]
			];
	};

	return SNew(SFlickAngularBorder)
		.BackgroundColor(Panel.CopyWithNewOpacity(1.0f))
		.AccentColor(Hairline)
		.UseAccentForOutline(false)
		.CutSize(14.0f)
		.BorderWidth(1.25f)
		.Padding(FMargin(18.0f, 18.0f, 18.0f, 16.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(54.0f).HeightOverride(54.0f)
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline)
							.UseAccentForOutline(false).CutSize(7.0f).BorderWidth(1.0f).Padding(FMargin(8.0f))
							[
								SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Brand)
							]
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
								return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER"));
							})
							.Font(DisplayFont(22)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
								const FString Service = Sessions ? Sessions->GetOnlineServiceName().ToUpper() : TEXT("LOCAL");
								return FText::FromString(FString::Printf(TEXT("%s  //  ONLINE  //  MAIN MENU"), *Service));
							})
							.Font(UiFont(8, true)).ColorAndOpacity(Brand)
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(44.0f).HeightOverride(44.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false)
						.CutSize(7.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(SButton)
							.ButtonStyle(&TransparentButtonStyle)
							.ContentPadding(FMargin(0.0f))
							.ToolTipText(FText::FromString(TEXT("Close social panel")))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this]()
							{
								bSocialPanelOpen = false;
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("X"))).Font(UiFont(16, true)).ColorAndOpacity(Paper)
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 2.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
				[
					MakeSocialTab(TEXT("FRIENDS"), false)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(3.0f, 0.0f, 0.0f, 0.0f)
				[
					MakeSocialTab(TEXT("RECENT PLAYERS"), true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 6.0f)
			[
				SNew(SBox).HeightOverride(31.0f)
				.Visibility_Lambda([this]() { return bShowingRecentPlayers ? EVisibility::Collapsed : EVisibility::Visible; })
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]()
					{
						bSocialPartyExpanded = !bSocialPartyExpanded;
						return FReply::Handled();
					})
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false).CutSize(3.0f).BorderWidth(0.75f).Padding(FMargin(9.0f, 4.0f))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return FText::FromString(bSocialPartyExpanded ? TEXT("v") : TEXT(">")); })
								.Font(UiFont(8, true)).ColorAndOpacity(Brand)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const int32 Members = GameMode.IsValid() && GameMode->IsPartySession() ? GameMode->GetPartyMemberCount() : 1;
									return FText::FromString(FString::Printf(TEXT("PARTY  (%d)"), Members));
								})
								.Font(UiFont(8, true)).ColorAndOpacity(Paper)
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("MAIN MENU"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(0)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(1)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(2)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(3)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)[BuildPartyMemberRow(4)]
			+ SVerticalBox::Slot().AutoHeight()[BuildPartyMemberRow(5)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 13.0f, 0.0f, 5.0f)
			[
				SNew(SBox).HeightOverride(29.0f)
				[
					SNew(SButton)
					.ButtonStyle(&TransparentButtonStyle)
					.ContentPadding(0.0f)
					.OnClicked_Lambda([this]()
					{
						if (bShowingRecentPlayers)
						{
							bSocialRecentExpanded = !bSocialRecentExpanded;
						}
						else
						{
							bSocialFriendsExpanded = !bSocialFriendsExpanded;
						}
						return FReply::Handled();
					})
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.UseAccentForOutline(false).CutSize(3.0f).BorderWidth(0.7f).Padding(FMargin(10.0f, 4.0f))
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
								if (bShowingRecentPlayers)
								{
									const int32 Count = Sessions ? Sessions->GetRecentPlayers().Num() : 0;
									return FText::FromString(FString::Printf(
										TEXT("%s  RECENT PLAYERS  (%d)"),
										bSocialRecentExpanded ? TEXT("v") : TEXT(">"), Count));
								}
								int32 OnlineCount = 0;
								if (Sessions)
								{
									for (const FFlickSocialPlayerEntry& Friend : Sessions->GetFriends())
									{
										OnlineCount += Friend.bOnline ? 1 : 0;
									}
								}
								return FText::FromString(FString::Printf(
									TEXT("%s  PLATFORM FRIENDS  //  %d ONLINE"),
									bSocialFriendsExpanded ? TEXT("v") : TEXT(">"), OnlineCount));
							})
							.Font(UiFont(8, true)).ColorAndOpacity(Paper)
						]
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SScrollBox)
				.ScrollBarAlwaysVisible(false)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 5.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(58.0f)
						.Visibility_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							const bool bEmpty = !Sessions || (bShowingRecentPlayers
								? Sessions->GetRecentPlayers().IsEmpty()
								: Sessions->GetFriends().IsEmpty());
							const bool bExpanded = bShowingRecentPlayers ? bSocialRecentExpanded : bSocialFriendsExpanded;
							return bEmpty && bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline.CopyWithNewOpacity(0.42f))
							.CutSize(5.0f).BorderWidth(0.8f).Padding(FMargin(14.0f, 10.0f))
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return FText::FromString(bShowingRecentPlayers
										? TEXT("NO RECENT PLAYERS  //  PLAY ONLINE TO BUILD YOUR LIST")
										: TEXT("NO PLATFORM FRIENDS FOUND  //  TRY REFRESH"));
								})
								.Font(UiFont(8, true)).ColorAndOpacity(Muted).AutoWrapText(true)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(0)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(1)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(2)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(3)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(4)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(5)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(6)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(7)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(8)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(9)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(10)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildSocialFriendRow(11)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(0)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(1)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(2)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(3)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(4)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(5)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(6)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(7)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(8)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(9)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(10)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 5.0f, 4.0f)[BuildRecentPlayerRow(11)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(SBox).HeightOverride(1.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.82f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SButton)
					.Visibility(EVisibility::Collapsed)
					.ButtonStyle(&TransparentButtonStyle)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.OnClicked_Lambda([this]()
					{
						if (GameMode.IsValid()) GameMode->ToggleRankedQueue();
						return FReply::Handled();
					})
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(GameMode.IsValid() && GameMode->IsRankedQueueSelected()
								? TEXT("QUEUE TYPE  /  RANKED") : TEXT("QUEUE TYPE  /  CASUAL"));
						})
						.Font(UiFont(10, true))
						.ColorAndOpacity_Lambda([this]()
						{
							return FSlateColor(GameMode.IsValid() && GameMode->IsRankedQueueSelected() ? Orange : Cyan);
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					.Visibility(EVisibility::Collapsed)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						MakeQueueButton(2)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						MakeQueueButton(3)
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
						return FText::FromString(Sessions ? Sessions->GetStatusMessage() : TEXT("STEAM SOCIAL UNAVAILABLE"));
					})
					.Font(UiFont(8, true)).ColorAndOpacity(Muted).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 5.0f, 0.0f)
					[
						SNew(SBox).HeightOverride(44.0f)
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline)
							.CutSize(6.0f)
							.BorderWidth(1.0f)
							.Padding(FMargin(1.0f))
							[
								SNew(SButton)
								.ButtonStyle(&TransparentButtonStyle)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.OnClicked_Lambda([this]()
								{
									if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr) Sessions->RefreshFriends();
									return FReply::Handled();
								})
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("\u21BB"))).Font(UiFont(17, true)).ColorAndOpacity(Paper)
									]
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[
										SNew(STextBlock).Text(FText::FromString(TEXT("REFRESH"))).Font(UiFont(11, true)).ColorAndOpacity(Paper)
									]
								]
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(5.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(130.0f)
						.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsPartySession() ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("DISBAND"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->DisbandParty();
								return FReply::Handled();
							}), false, true, 44.0f)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPartyMemberRow(const int32 PartySlot)
{
	return SNew(SBox).HeightOverride(48.0f)
		.Visibility_Lambda([this, PartySlot]()
		{
			if (bShowingRecentPlayers || !bSocialPartyExpanded)
			{
				return EVisibility::Collapsed;
			}
			if (GameMode.IsValid() && GameMode->IsPartySession())
			{
				return GameMode->GetPartyMember(PartySlot) ? EVisibility::Visible : EVisibility::Collapsed;
			}
			return PartySlot == 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Hairline)
			.CutSize(5.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(14.0f).HeightOverride(14.0f)
					[
						SNew(SFlickRoundPip)
						.Color_Lambda([this, PartySlot]()
						{
							const bool bOccupied = GameMode.IsValid() && GameMode->IsPartySession()
								? GameMode->GetPartyMember(PartySlot) != nullptr : PartySlot == 0;
							return bOccupied ? Brand : Muted;
						})
						.Filled_Lambda([this, PartySlot]()
						{
							return GameMode.IsValid() && GameMode->IsPartySession()
								? GameMode->GetPartyMember(PartySlot) != nullptr : PartySlot == 0;
						})
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(44.0f).HeightOverride(36.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.CutSize(6.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%02d"), PartySlot + 1)))
							.Font(UiFont(10, true))
							.Justification(ETextJustify::Center)
							.ColorAndOpacity(Brand)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text_Lambda([this, PartySlot]()
							{
								if (GameMode.IsValid() && GameMode->IsPartySession())
								{
									const AFlickPlayerState* Member = GameMode->GetPartyMember(PartySlot);
									return FText::FromString(Member ? Member->GetPlayerName() : TEXT("OPEN PARTY SLOT"));
								}
								if (PartySlot == 0)
								{
									const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
									return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("LOCAL PLAYER"));
								}
								return FText::FromString(TEXT("OPEN PARTY SLOT"));
							}).Font(UiFont(11, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("*")))
							.Font(UiFont(10, true))
							.ColorAndOpacity(Orange)
							.Visibility_Lambda([this, PartySlot]()
							{
								const AFlickPlayerState* Member = GameMode.IsValid() && GameMode->IsPartySession() ? GameMode->GetPartyMember(PartySlot) : nullptr;
								return (Member && Member->IsPartyLeader()) || ((!GameMode.IsValid() || !GameMode->IsPartySession()) && PartySlot == 0)
									? EVisibility::Visible : EVisibility::Collapsed;
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, PartySlot]()
						{
							const AFlickPlayerState* Member = GameMode.IsValid() && GameMode->IsPartySession() ? GameMode->GetPartyMember(PartySlot) : nullptr;
							return FText::FromString((Member && Member->IsPartyLeader()) || ((!GameMode.IsValid() || !GameMode->IsPartySession()) && PartySlot == 0)
								? TEXT("PARTY LEADER") : Member ? TEXT("IN PARTY") : TEXT("INVITE A FRIEND"));
						}).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton).ButtonStyle(&DangerButtonStyle)
					.Visibility_Lambda([this, PartySlot]()
					{
						return PartySlot > 0 && GameMode.IsValid() && GameMode->IsPartySession() && GameMode->GetPartyMember(PartySlot)
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.OnClicked_Lambda([this, PartySlot]()
					{
						if (GameMode.IsValid()) GameMode->RemovePartyMember(PartySlot);
						return FReply::Handled();
					})
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("REMOVE"))).Font(UiFont(8, true)).ColorAndOpacity(Orange)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildSocialFriendRow(const int32 FriendIndex)
{
	return SNew(SVerticalBox)
		.Visibility_Lambda([this, FriendIndex]()
		{
			const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
			return !bShowingRecentPlayers && bSocialFriendsExpanded && Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex)
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(23.0f)
			.Visibility_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return EVisibility::Collapsed;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				const int32 Group = Friend.bPlayingFlick ? 0 : Friend.bOnline ? 1 : 2;
				if (FriendIndex == 0) return EVisibility::Visible;
				const FFlickSocialPlayerEntry& Previous = Sessions->GetFriends()[FriendIndex - 1];
				const int32 PreviousGroup = Previous.bPlayingFlick ? 0 : Previous.bOnline ? 1 : 2;
				return Group != PreviousGroup ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.ContentPadding(0.0f)
				.OnClicked_Lambda([this, FriendIndex]()
				{
					const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
					if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return FReply::Handled();
					const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
					if (Friend.bPlayingFlick) bSocialInGameExpanded = !bSocialInGameExpanded;
					else if (Friend.bOnline) bSocialOnlineExpanded = !bSocialOnlineExpanded;
					else bSocialOfflineExpanded = !bSocialOfflineExpanded;
					return FReply::Handled();
				})
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(PanelRaised)
					.Padding(FMargin(9.0f, 3.0f))
					[
						SNew(STextBlock)
						.Text_Lambda([this, FriendIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return FText::GetEmpty();
							const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
							const bool bPlaying = Friend.bPlayingFlick;
							const bool bOnline = Friend.bOnline;
							const bool bExpanded = bPlaying ? bSocialInGameExpanded : bOnline ? bSocialOnlineExpanded : bSocialOfflineExpanded;
							int32 Count = 0;
							for (const FFlickSocialPlayerEntry& Candidate : Sessions->GetFriends())
							{
								Count += bPlaying
									? (Candidate.bPlayingFlick ? 1 : 0)
									: bOnline
										? (Candidate.bOnline && !Candidate.bPlayingFlick ? 1 : 0)
										: (!Candidate.bOnline ? 1 : 0);
							}
							const TCHAR* Label = bPlaying ? TEXT("IN-GAME") : bOnline ? TEXT("ONLINE") : TEXT("OFFLINE");
							return FText::FromString(FString::Printf(
								TEXT("%s  %s  (%d)"), bExpanded ? TEXT("v") : TEXT(">"), Label, Count));
						})
						.Font(UiFont(8, true)).ColorAndOpacity(Paper)
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(56.0f)
			.Visibility_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return EVisibility::Collapsed;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				const bool bExpanded = Friend.bPlayingFlick
					? bSocialInGameExpanded
					: Friend.bOnline ? bSocialOnlineExpanded : bSocialOfflineExpanded;
				return bExpanded ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
				return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bPlayingFlick
					? PanelRaised : Panel;
			})
			.AccentColor_Lambda([this, FriendIndex]()
			{
				const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
				if (!Sessions || !Sessions->GetFriends().IsValidIndex(FriendIndex)) return Hairline;
				const FFlickSocialPlayerEntry& Friend = Sessions->GetFriends()[FriendIndex];
				return Friend.bPlayingFlick
					? Brand
					: Friend.bOnline ? Muted : Hairline.CopyWithNewOpacity(0.46f);
			})
			.UseAccentForOutline(false)
			.CutSize(6.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 6.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(38.0f).HeightOverride(38.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(PanelRaised)
							.AccentColor(Hairline)
							.CutSize(5.0f).BorderWidth(0.8f).Padding(FMargin(7.0f))
							[
								SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Muted)
							]
						]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, -2.0f, -2.0f)
						[
							SNew(SBox).WidthOverride(11.0f).HeightOverride(11.0f)
							[
								SNew(SFlickRoundPip)
								.Color_Lambda([this, FriendIndex]()
								{
									const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
									return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline
										? Brand : Muted;
								})
								.Filled_Lambda([this, FriendIndex]()
								{
									const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
									return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline;
								})
							]
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, FriendIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) ? Sessions->GetFriends()[FriendIndex].DisplayName : FString());
						}).Font(UiFont(11, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, FriendIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) ? Sessions->GetFriends()[FriendIndex].Status : FString());
						}).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(84.0f).HeightOverride(34.0f)
					.Visibility_Lambda([this, FriendIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
						return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline
							? EVisibility::Visible : EVisibility::Collapsed;
					})
					.IsEnabled_Lambda([this, FriendIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
						const bool bPartyHasRoom = !GameMode.IsValid() || !GameMode->IsPartySession() || GameMode->GetPartyMemberCount() < FlickMaximumPartyMembers;
						return Sessions && Sessions->GetFriends().IsValidIndex(FriendIndex) && Sessions->GetFriends()[FriendIndex].bOnline && bPartyHasRoom;
					})
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.CutSize(5.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(SButton)
							.ButtonStyle(&TransparentButtonStyle)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this, FriendIndex]()
							{
								if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr) Sessions->InviteFriendToParty(FriendIndex);
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("INVITE"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)
							]
						]
					]
				]
			]
		]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildRecentPlayerRow(const int32 RecentIndex)
{
	return SNew(SBox).HeightOverride(56.0f)
		.Visibility_Lambda([this, RecentIndex]()
		{
			const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
			return bShowingRecentPlayers && bSocialRecentExpanded && Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex)
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(PanelRaised)
			.AccentColor(Hairline.CopyWithNewOpacity(0.62f))
			.UseAccentForOutline(false)
			.CutSize(6.0f)
			.BorderWidth(0.9f)
			.Padding(FMargin(9.0f, 6.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(38.0f).HeightOverride(38.0f)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline.CopyWithNewOpacity(0.7f))
						.CutSize(5.0f).BorderWidth(0.8f).Padding(FMargin(7.0f))
						[
							SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Social).Color(Muted)
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text_Lambda([this, RecentIndex]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(Sessions && Sessions->GetRecentPlayers().IsValidIndex(RecentIndex) ? Sessions->GetRecentPlayers()[RecentIndex].DisplayName : FString());
						}).Font(UiFont(11, true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("RECENTLY MET IN FLICK"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(84.0f).HeightOverride(34.0f)
					.IsEnabled_Lambda([this]() { return !GameMode.IsValid() || !GameMode->IsPartySession() || GameMode->GetPartyMemberCount() < FlickMaximumPartyMembers; })
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(PanelRaised)
						.AccentColor(Hairline)
						.CutSize(5.0f)
						.BorderWidth(1.0f)
						.Padding(FMargin(1.0f))
						[
							SNew(SButton)
							.ButtonStyle(&TransparentButtonStyle)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.OnClicked_Lambda([this, RecentIndex]()
							{
								if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr) Sessions->InviteRecentPlayerToParty(RecentIndex);
								return FReply::Handled();
							})
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("INVITE"))).Font(UiFont(8, true)).ColorAndOpacity(Brand)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildItemShop()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.002f, 0.006f, 0.011f, 0.9f))
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(116.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.008f, 0.016f, 0.94f))
				.AccentColor(Orange.CopyWithNewOpacity(0.76f))
				.CutSize(10.0f)
				.BorderWidth(1.1f)
				.Padding(FMargin(44.0f, 22.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(6.0f).HeightOverride(55.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Orange)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("ITEM SHOP"))).Font(UiFont(36, true)).ColorAndOpacity(FLinearColor::White)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("COSMETIC SHOWCASE"))).Font(UiFont(10, true)).ColorAndOpacity(Orange)]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK CREDITS"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString(TEXT("0"))).Font(UiFont(24, true)).ColorAndOpacity(FLinearColor::White)]
					]
				]
			]
		]
		+ SOverlay::Slot().Padding(44.0f, 142.0f, 44.0f, 104.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("FEATURED"))).Font(UiFont(18, true)).ColorAndOpacity(FLinearColor::White)]
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("STORE PREVIEW  /  COMING SOON"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 13.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(8.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)[BuildShopItemCard(TEXT("NEON CIRCUIT"), TEXT("PUCK FINISH"), TEXT("N"), Cyan)]
				+ SUniformGridPanel::Slot(1, 0)[BuildShopItemCard(TEXT("ARENA SIGNAL"), TEXT("RIM COLOR"), TEXT("A"), Orange)]
				+ SUniformGridPanel::Slot(2, 0)[BuildShopItemCard(TEXT("FOUNDERS MARK"), TEXT("PLAYER BANNER"), TEXT("F"), FLinearColor(0.72f, 0.38f, 1.0f, 1.0f))]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(82.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.93f))
				.AccentColor(Orange.CopyWithNewOpacity(0.58f))
				.CutSize(8.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(34.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(160.0f)
						[
							MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CloseItemShop();
								return FReply::Handled();
							}), false, false, 50.0f, &ItemShopDefaultButton)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("PURCHASES AND INVENTORY WILL ARRIVE IN A LATER MILESTONE"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildShopItemCard(
	const FString& Name,
	const FString& Type,
	const FString& Mark,
	const FLinearColor& Accent)
{
	TSharedRef<SWidget> Preview = SNew(SBox);
	if (Mark == TEXT("N"))
	{
		Preview = SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(205.0f).HeightOverride(205.0f)
				[
					SNew(SFlickPuckDisc).TeamColor(FLinearColor(0.025f, 0.08f, 0.12f, 1.0f)).AccentColor(Accent).Selected(true).RadiusScale(1.16f)
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("F"))).Font(UiFont(28, true)).ColorAndOpacity(FLinearColor::White)
			];
	}
	else if (Mark == TEXT("A"))
	{
		Preview = SNew(SOverlay)
			+ SOverlay::Slot().Padding(34.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.01f, 0.018f, 0.025f, 1.0f))
				.AccentColor(Accent)
				.CutSize(24.0f)
				.BorderWidth(4.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.025f, 0.04f, 0.052f, 1.0f))
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("ARENA RIM"))).Font(UiFont(14, true)).ColorAndOpacity(Accent)
			];
	}
	else
	{
		Preview = SNew(SOverlay)
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(260.0f).HeightOverride(105.0f)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(FMath::Lerp(Panel, Accent, 0.14f))
					.AccentColor(Accent)
					.CutSize(17.0f)
					.BorderWidth(2.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("F"))).Font(UiFont(54, true)).Justification(ETextJustify::Center).ColorAndOpacity(FLinearColor::White)
					]
				]
			];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.008f, 0.016f, 0.027f, 0.98f))
		.AccentColor(Accent)
		.CutSize(16.0f)
		.BorderWidth(2.0f)
		.Padding(FMargin(18.0f))
		[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(FMath::Lerp(FLinearColor(0.008f, 0.016f, 0.027f, 1.0f), Accent, 0.16f))
					[
						SNew(SOverlay)
						+ SOverlay::Slot()[Preview]
						+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(10.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("PREVIEW"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 15.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Type)).Font(UiFont(9, true)).ColorAndOpacity(Accent)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Name)).Font(UiFont(19, true)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(PanelRaised)
					.Padding(FMargin(10.0f, 7.0f))
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("NOT YET AVAILABLE"))).Font(UiFont(8, true)).Justification(ETextJustify::Center).ColorAndOpacity(Muted)
					]
				]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildModeSelect()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.003f, 0.008f, 0.68f))
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility(EVisibility::HitTestInvisible)
			.Opacity(0.78f)
		]
		+ SOverlay::Slot()
		[
			SNew(SFlickInterfaceBackdrop)
			.Visibility(EVisibility::HitTestInvisible)
			.Opacity(0.2f)
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(UiMetrics::ModeHeaderHeight)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel)
				.AccentColor_Lambda([this]()
				{
					return SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive
						? Orange.CopyWithNewOpacity(0.88f)
						: SelectedPlayPlaylist == EFlickPlayPlaylist::Training
							? FLinearColor(0.2f, 0.78f, 0.5f, 0.88f)
							: SelectedPlayPlaylist == EFlickPlayPlaylist::Test
								? FLinearColor(0.62f, 0.36f, 0.95f, 0.88f)
							: Brand.CopyWithNewOpacity(0.82f);
				})
				.UseAccentForOutline(true)
				.CutSize(11.0f)
				.BorderWidth(1.15f)
				.Padding(FMargin(34.0f, 13.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(190.0f).HeightOverride(62.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("FLICK"))).Font(DisplayFont(42, true)).ColorAndOpacity(Paper)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20.0f, 6.0f, 20.0f, 6.0f)
					[
						SNew(SBox).WidthOverride(2.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(0.72f))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::None) return FText::FromString(TEXT("FIND YOUR NEXT RIVALRY."));
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training && SelectedTrainingActivity == EFlickTrainingActivity::None) return FText::FromString(TEXT("TRAINING"));
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::Test) return FText::FromString(TEXT("TEST"));
								return FText::FromString(TEXT("SELECT MATCH FORMAT"));
							})
							.Font(DisplayFont(30))
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::None) return FText::FromString(TEXT("CHOOSE HOW YOU WANT TO PLAY"));
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training && SelectedTrainingActivity == EFlickTrainingActivity::None) return FText::FromString(TEXT("CHOOSE A TRAINING ACTIVITY"));
								if (SelectedPlayPlaylist == EFlickPlayPlaylist::Test) return FText::FromString(TEXT("SWITCHYARD  /  THE DIVIDER EXPERIMENT"));
								return FText::FromString(TEXT("SET THE ARENA RULESET AND TEAM SIZE"));
							})
							.Font(UiFont(10, true))
							.ColorAndOpacity(Brand)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("PLAY CONFIGURATION"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::None
									? TEXT("01  //  MODE")
									: SelectedPlayPlaylist == EFlickPlayPlaylist::Test
										? TEXT("02  //  EXPERIMENT")
										: TEXT("02  //  FORMAT"));
							})
							.Font(UiFont(13, true)).ColorAndOpacity(Brand)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]()
			{
				return SelectedPlayPlaylist == EFlickPlayPlaylist::Test
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(UiMetrics::CardGap)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(PanelRaised)
					.AccentColor(FLinearColor(0.62f, 0.36f, 0.95f, 0.72f))
					.CutSize(8.0f)
					.BorderWidth(0.9f)
					.Padding(FMargin(18.0f, 2.0f))
					[
						MakeCycleRow(
							TEXT("BOT DIFFICULTY"),
							TAttribute<FText>::CreateLambda([this]()
							{
								return FText::FromString(GameMode.IsValid() ? GameMode->GetBotDifficultyLabel() : TEXT("NORMAL"));
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleBotDifficulty(-1);
								return FReply::Handled();
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleBotDifficulty(1);
								return FReply::Handled();
							}))
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(UiMetrics::CardGap))
					+ SUniformGridPanel::Slot(0, 0)
					[
						BuildTrainingActivityCard(
							EFlickTrainingActivity::ArenaControlBotMatch,
							TEXT("SWITCHYARD 1V1"),
							TEXT("Shape the compact board with linked switches, then beat the bot."),
							TEXT("8 ACTIVE  /  20 LOCATIONS"),
							FLinearColor(0.62f, 0.36f, 0.95f, 1.0f),
							&TestActivityDefaultButton,
							1)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						BuildTrainingActivityCard(
							EFlickTrainingActivity::ArenaControlBotMatch,
							TEXT("SWITCHYARD 2V2"),
							TEXT("Coordinate two lineups across a larger, denser switchyard."),
							TEXT("12 ACTIVE  /  28 LOCATIONS"),
							FLinearColor(0.30f, 0.58f, 0.96f, 1.0f),
							nullptr,
							2)
					]
					+ SUniformGridPanel::Slot(2, 0)
					[
						BuildTrainingActivityCard(
							EFlickTrainingActivity::ArenaControlBotMatch,
							TEXT("SWITCHYARD 3V3"),
							TEXT("Six lineups collide inside the most chaotic experimental layout."),
							TEXT("14 ACTIVE  /  36 LOCATIONS"),
							FLinearColor(0.82f, 0.32f, 0.86f, 1.0f),
							nullptr,
							3)
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::None ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SGridPanel).FillColumn(0, 1.0f).FillColumn(1, 1.0f)
				+ SGridPanel::Slot(0, 0).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Casual, TEXT("CASUAL"), TEXT("LOCAL PLAY OR RELAXED ONLINE MATCHMAKING"), Cyan, true, &ModeSelectDefaultButton)]
				+ SGridPanel::Slot(1, 0).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Competitive, TEXT("COMPETITIVE"), TEXT("RANKED ONLINE MATCHES WITH MMR"), Orange, true)]
				+ SGridPanel::Slot(0, 1).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Training, TEXT("TRAINING"), TEXT("PRACTICE FREELY OR PLAY KNOCKOUT AND BOB AGAINST THE BOT"), FLinearColor(0.2f, 0.78f, 0.5f, 1.0f), true)]
				+ SGridPanel::Slot(1, 1).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::PrivateMatch, TEXT("PRIVATE MATCH"), TEXT("CUSTOM RULES, FLEXIBLE TEAMS, AND SPECTATORS"), FLinearColor(0.65f, 0.72f, 0.8f, 1.0f), true)]
				+ SGridPanel::Slot(0, 2).ColumnSpan(2).Padding(UiMetrics::CardGap)[BuildPlayPlaylistCard(EFlickPlayPlaylist::Test, TEXT("TEST"), TEXT("SWITCHYARD  /  LINKED SWITCHES. MOVING DIVIDERS. NEW ANGLES."), FLinearColor(0.62f, 0.36f, 0.95f, 1.0f), true)]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]()
			{
				return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
					&& SelectedTrainingActivity == EFlickTrainingActivity::None
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(UiMetrics::CardGap)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(PanelRaised)
					.AccentColor(Orange.CopyWithNewOpacity(0.66f))
					.CutSize(8.0f)
					.BorderWidth(0.9f)
					.Padding(FMargin(18.0f, 2.0f))
					[
						MakeCycleRow(
							TEXT("BOT DIFFICULTY"),
							TAttribute<FText>::CreateLambda([this]()
							{
								return FText::FromString(GameMode.IsValid() ? GameMode->GetBotDifficultyLabel() : TEXT("NORMAL"));
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleBotDifficulty(-1);
								return FReply::Handled();
							}),
							FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleBotDifficulty(1);
								return FReply::Handled();
							}))
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FMargin(UiMetrics::CardGap))
					+ SUniformGridPanel::Slot(0, 0)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::FreePlay,
						TEXT("FREE PLAY"),
						TEXT("BUILD A CUSTOM BOARD AND REPEAT ANY SHOT"),
						TEXT("BOARD EDITOR  |  INSTANT RESET  |  ANY ARENA"),
						FLinearColor(0.2f, 0.78f, 0.5f, 1.0f),
						&TrainingActivityDefaultButton)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::BotMatch,
						TEXT("1V1 VS BOT"),
						TEXT("PLAY A COMPLETE 1V1 KNOCKOUT SERIES"),
						TEXT("4 PUCKS EACH  |  BEST OF 5  |  OFFLINE"),
						Orange)
				]
				+ SUniformGridPanel::Slot(2, 0)
				[
					BuildTrainingActivityCard(
						EFlickTrainingActivity::BobBotMatch,
						TEXT("BOB VS BOT"),
						TEXT("POCKET YOUR COLOR BEFORE THE BOT CLEARS THEIRS"),
						TEXT("STANDARD PUCKS  |  BOB RULES  |  OFFLINE"),
						FLinearColor(0.18f, 0.82f, 0.48f, 1.0f))
				]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f, 116.0f, 28.0f, 98.0f)
		[
			SNew(SBox)
			.WidthOverride(UiMetrics::ModeContentWidth)
			.Visibility_Lambda([this]()
			{
				return (SelectedPlayPlaylist == EFlickPlayPlaylist::Casual
					|| SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive
					|| (SelectedPlayPlaylist == EFlickPlayPlaylist::Training
						&& SelectedTrainingActivity == EFlickTrainingActivity::FreePlay))
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(UiMetrics::CardGap))
				+ SUniformGridPanel::Slot(0, 0)[BuildPlayFormatCard(1, false, &ModeFormatDefaultButton)]
				+ SUniformGridPanel::Slot(1, 0)[BuildPlayFormatCard(2, false)]
				+ SUniformGridPanel::Slot(0, 1)[BuildPlayFormatCard(3, false)]
				+ SUniformGridPanel::Slot(1, 1)[BuildPlayFormatCard(1, true)]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(24.0f, 0.0f, 24.0f, 16.0f)
		[
			SNew(SBox).HeightOverride(UiMetrics::ModeFooterHeight)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.93f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.58f))
				.UseAccentForOutline(true)
				.CutSize(10.0f)
				.BorderWidth(1.1f)
				.Padding(FMargin(20.0f, 12.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(164.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]()
					{
						if (SelectedPlayPlaylist == EFlickPlayPlaylist::Training
							&& SelectedTrainingActivity != EFlickTrainingActivity::None)
						{
							SelectedTrainingActivity = EFlickTrainingActivity::None;
						}
						else if (SelectedPlayPlaylist != EFlickPlayPlaylist::None)
						{
							SelectedPlayPlaylist = EFlickPlayPlaylist::None;
							SelectedTrainingActivity = EFlickTrainingActivity::None;
						}
						else if (GameMode.IsValid())
						{
							GameMode->CloseModeSelect();
						}
						return FReply::Handled();
					}), false, false, UiMetrics::ActionHeight)]]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(218.0f)
						.Visibility_Lambda([this]()
						{
							return SelectedPlayPlaylist != EFlickPlayPlaylist::None
								&& SelectedPlayPlaylist != EFlickPlayPlaylist::Training
								&& SelectedPlayPlaylist != EFlickPlayPlaylist::Test
								&& GameMode.IsValid()
								&& GameMode->DoesSelectedModeSupportLoadouts()
								? EVisibility::Visible
								: EVisibility::Collapsed;
						})
						[
							MakeMenuButton(TEXT("EDIT LINEUPS"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->OpenLoadout(); return FReply::Handled(); }), false, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(224.0f)
						.IsEnabled_Lambda([this]() { return !GameMode.IsValid() || !GameMode->IsPartySession(); })
						.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Casual ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("LOCAL MATCH"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->StartSelectedMatch(); return FReply::Handled(); }), false, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(242.0f)
						.IsEnabled_Lambda([this]()
						{
							return GameMode.IsValid()
								&& (!GameMode->IsPartySession()
									|| GameMode->CanQueuePartyForMatchmaking(GameMode->GetMatchmakingPlayersPerTeam()));
						})
						.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Casual ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("FIND CASUAL"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid())
								{
									GameMode->SetRankedQueueSelected(false);
									GameMode->StartSelectedMatchmaking();
								}
								return FReply::Handled();
							}), true, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(254.0f)
						.Visibility_Lambda([this]()
						{
							return SelectedPlayPlaylist == EFlickPlayPlaylist::Training
								&& SelectedTrainingActivity == EFlickTrainingActivity::FreePlay
								? EVisibility::Visible
								: EVisibility::Collapsed;
						})
						[
							MakeMenuButton(TEXT("START TRAINING"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid())
								{
									GameMode->StartTrainingMode();
								}
								return FReply::Handled();
							}), true, false, UiMetrics::ActionHeight)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(276.0f)
						.IsEnabled_Lambda([this]()
						{
							return GameMode.IsValid()
								&& (!GameMode->IsPartySession()
									|| GameMode->CanQueuePartyForMatchmaking(GameMode->GetMatchmakingPlayersPerTeam()));
						})
						.Visibility_Lambda([this]() { return SelectedPlayPlaylist == EFlickPlayPlaylist::Competitive ? EVisibility::Visible : EVisibility::Collapsed; })
						[
							MakeMenuButton(TEXT("FIND COMPETITIVE"), FOnClicked::CreateLambda([this]()
							{
								if (GameMode.IsValid())
								{
									GameMode->SetRankedQueueSelected(true);
									GameMode->StartSelectedMatchmaking();
								}
								return FReply::Handled();
							}), true, false, UiMetrics::ActionHeight)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPrivateMatchSlot(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	const FLinearColor Accent = GetTeamAccent(Team);
	return SNew(SBox)
		.HeightOverride(82.0f)
		.Visibility_Lambda([this, PlayerSlot]()
		{
			return GameMode.IsValid()
				&& PlayerSlot < GameMode->GetPrivateMatchSettings().PlayersPerTeam
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, Team, PlayerSlot, Accent]()
			{
				const AFlickPlayerState* Owner = GameMode.IsValid()
					? GameMode->GetPrivateSlotOwner(Team, PlayerSlot)
					: nullptr;
				const AFlickPlayerState* Local = PlayerController.IsValid()
					? PlayerController->GetPlayerState<AFlickPlayerState>()
					: nullptr;
				return Owner && Owner == Local
					? Accent.CopyWithNewOpacity(0.18f)
					: FLinearColor(0.004f, 0.013f, 0.024f, 0.96f);
			})
			.AccentColor(Accent.CopyWithNewOpacity(0.9f))
			.CutSize(10.0f)
			.BorderWidth(1.1f)
			.Padding(1.0f)
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.Cursor(EMouseCursor::Hand)
				.OnClicked_Lambda([this, Team, PlayerSlot]()
				{
					if (PlayerController.IsValid())
					{
						PlayerController->RequestTogglePrivateMatchSlot(Team, PlayerSlot);
					}
					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(20.0f, 10.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%s PLAYER %d"), Team == EFlickTeam::Player1 ? TEXT("BLUE") : TEXT("ORANGE"), PlayerSlot + 1)))
							.Font(UiFont(9, true))
							.ColorAndOpacity(Accent)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this, Team, PlayerSlot]()
							{
								const AFlickPlayerState* Owner = GameMode.IsValid()
									? GameMode->GetPrivateSlotOwner(Team, PlayerSlot)
									: nullptr;
								return FText::FromString(Owner ? Owner->GetPlayerName() : TEXT("OPEN SLOT"));
							})
							.Font(UiFont(16, true))
							.ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 18.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team, PlayerSlot]()
						{
							const AFlickPlayerState* Owner = GameMode.IsValid()
								? GameMode->GetPrivateSlotOwner(Team, PlayerSlot)
								: nullptr;
							return FText::FromString(!Owner ? TEXT("CLAIM") : Owner->IsLobbyReady() ? TEXT("READY") : TEXT("NOT READY"));
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity_Lambda([this, Team, PlayerSlot, Accent]()
						{
							const AFlickPlayerState* Owner = GameMode.IsValid()
								? GameMode->GetPrivateSlotOwner(Team, PlayerSlot)
								: nullptr;
							return FSlateColor(Owner && Owner->IsLobbyReady() ? Accent : Muted);
						})
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPrivateMatchSetup()
{
	auto CycleSetting = [this](const FString& Label, const EFlickPrivateMatchSetting Setting, const TAttribute<FText>& Value)
	{
		return MakeCycleRow(
			Label,
			Value,
			FOnClicked::CreateLambda([this, Setting]()
			{
				if (GameMode.IsValid()) GameMode->CyclePrivateMatchSetting(Setting, -1);
				return FReply::Handled();
			}),
			FOnClicked::CreateLambda([this, Setting]()
			{
				if (GameMode.IsValid()) GameMode->CyclePrivateMatchSetting(Setting, 1);
				return FReply::Handled();
			}));
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.003f, 0.008f, 0.82f))]
		+ SOverlay::Slot().Padding(42.0f, 30.0f, 42.0f, 106.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("PRIVATE MATCH"))).Font(DisplayFont(38)).ColorAndOpacity(FLinearColor::White)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("CUSTOM RULES  /  FLEXIBLE PLAYER OWNERSHIP  /  SPECTATORS"))).Font(UiFont(10, true)).ColorAndOpacity(Cyan)]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(GameMode.IsValid()
							? FString::Printf(TEXT("PARTY  %d / %d"), FMath::Max(GameMode->GetPartyMemberCount(), 1), FlickMaximumPartyMembers)
							: TEXT("PRIVATE LOBBY"));
					})
					.Font(UiFont(11, true)).ColorAndOpacity(Muted)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 22.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.39f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.017f, 0.96f)).AccentColor(Hairline).CutSize(12.0f).BorderWidth(1.0f).Padding(FMargin(18.0f, 14.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 8.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("MATCH RULES"))).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("TEAM SIZE"), EFlickPrivateMatchSetting::TeamSize, TAttribute<FText>::CreateLambda([this]() { const int32 N = GameMode.IsValid() ? GameMode->GetPrivateMatchSettings().PlayersPerTeam : 2; return FText::FromString(FString::Printf(TEXT("%dV%d"), N, N)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("ROUNDS TO WIN"), EFlickPrivateMatchSetting::RoundsToWin, TAttribute<FText>::CreateLambda([this]() { return FText::AsNumber(GameMode.IsValid() ? GameMode->GetPrivateMatchSettings().RoundsToWin : 3); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("ARENA SIZE"), EFlickPrivateMatchSetting::ArenaScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), (GameMode.IsValid() ? GameMode->GetPrivateMatchSettings().ArenaScale : 1.0f) * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("FRICTION"), EFlickPrivateMatchSetting::FrictionScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), (GameMode.IsValid() ? GameMode->GetPrivateMatchSettings().FrictionScale : 1.0f) * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("LAUNCH POWER"), EFlickPrivateMatchSetting::LaunchSpeedScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), (GameMode.IsValid() ? GameMode->GetPrivateMatchSettings().LaunchSpeedScale : 1.0f) * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("BOUNCE"), EFlickPrivateMatchSetting::RestitutionScale, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(FString::Printf(TEXT("%.0f%%"), (GameMode.IsValid() ? GameMode->GetPrivateMatchSettings().RestitutionScale : 1.0f) * 100.0f)); }))]
						+ SVerticalBox::Slot().AutoHeight()[CycleSetting(TEXT("KICKOFF"), EFlickPrivateMatchSetting::SimultaneousKickoff, TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GameMode.IsValid() && GameMode->GetPrivateMatchSettings().bSimultaneousKickoff ? TEXT("SIMULTANEOUS") : TEXT("ALTERNATING")); }))]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.61f).Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 7.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 10.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("BLUE TEAM"))).Font(UiFont(14, true)).ColorAndOpacity(Cyan)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[BuildPrivateMatchSlot(EFlickTeam::Player1, 0)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[BuildPrivateMatchSlot(EFlickTeam::Player1, 1)]
						+ SVerticalBox::Slot().AutoHeight()[BuildPrivateMatchSlot(EFlickTeam::Player1, 2)]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(7.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 10.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("ORANGE TEAM"))).Font(UiFont(14, true)).ColorAndOpacity(Orange)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[BuildPrivateMatchSlot(EFlickTeam::Player2, 0)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)[BuildPrivateMatchSlot(EFlickTeam::Player2, 1)]
						+ SVerticalBox::Slot().AutoHeight()[BuildPrivateMatchSlot(EFlickTeam::Player2, 2)]
					]
				]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(88.0f)
			[
				SNew(SFlickAngularBorder).BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.96f)).AccentColor(Hairline).CutSize(8.0f).BorderWidth(1.0f).Padding(FMargin(34.0f, 16.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(180.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ClosePrivateMatchSetup(); return FReply::Handled(); }), false, false, 54.0f)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f)[SNew(SBox).WidthOverride(190.0f)[MakeMenuButton(TEXT("SPECTATE"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestPrivateMatchSpectate(); return FReply::Handled(); }), false, false, 54.0f)]]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(210.0f)
						.IsEnabled_Lambda([this]() { const AFlickPlayerState* Local = PlayerController.IsValid() ? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr; return Local && !Local->GetPrivateControlledSlots().IsEmpty(); })
						[
							MakeMenuButton(TEXT("READY UP"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->ToggleLobbyReady(); return FReply::Handled(); }), false, false, 54.0f, &PrivateMatchDefaultButton)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(250.0f).IsEnabled_Lambda([this]() { return GameMode.IsValid() && GameMode->CanStartPrivateMatch(); })
						[
							MakeMenuButton(TEXT("START MATCH"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestStartNetworkMatch(); return FReply::Handled(); }), true, false, 54.0f)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPlayPlaylistCard(
	const EFlickPlayPlaylist Playlist,
	const FString& Label,
	const FString& Summary,
	const FLinearColor& Accent,
	const bool bAvailable,
	TSharedPtr<SButton>* OutButton)
{
	const FString PlaylistCode = Playlist == EFlickPlayPlaylist::Casual ? TEXT("01")
		: Playlist == EFlickPlayPlaylist::Competitive ? TEXT("02")
		: Playlist == EFlickPlayPlaylist::Training ? TEXT("03")
		: Playlist == EFlickPlayPlaylist::PrivateMatch ? TEXT("04") : TEXT("05");
	const FString PlaylistTag = Playlist == EFlickPlayPlaylist::Casual ? TEXT("LOCAL + ONLINE")
		: Playlist == EFlickPlayPlaylist::Competitive ? TEXT("RANKED ONLINE")
		: Playlist == EFlickPlayPlaylist::Training ? TEXT("OFFLINE PRACTICE")
		: Playlist == EFlickPlayPlaylist::PrivateMatch ? TEXT("YOUR RULES") : TEXT("EXPERIMENTAL");
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.IsEnabled(bAvailable)
		.Cursor(bAvailable ? EMouseCursor::Hand : EMouseCursor::Default)
		.OnClicked_Lambda([this, Playlist]()
		{
			if (Playlist == EFlickPlayPlaylist::PrivateMatch)
			{
				if (GameMode.IsValid()) GameMode->OpenPrivateMatchSetup();
				return FReply::Handled();
			}
			SelectedPlayPlaylist = Playlist;
			SelectedTrainingActivity = EFlickTrainingActivity::None;
			if (GameMode.IsValid())
			{
				GameMode->SetRankedQueueSelected(Playlist == EFlickPlayPlaylist::Competitive);
				GameMode->SetMatchmakingPlayersPerTeam(1);
				GameMode->SelectMatchVariant(EFlickMatchVariant::Classic);
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakCardButton = CardButton;
	const auto IsActive = [WeakCardButton, bAvailable]()
	{
		const TSharedPtr<SButton> Button = WeakCardButton.Pin();
		return bAvailable && Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
	};
	CardButton->SetContent(
		SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor_Lambda([IsActive]() { return IsActive() ? Brand : Hairline.CopyWithNewOpacity(0.5f); })
		.Padding(1.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([IsActive]() { return IsActive() ? FMath::Lerp(PanelRaised, Brand, 0.04f) : Panel; })
			.Padding(FMargin(24.0f, 17.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(PlaylistCode + TEXT("   /   ") + PlaylistTag))
						.Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(34)).ColorAndOpacity(bAvailable ? Paper : Muted)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Summary)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox).WidthOverride(58.0f).HeightOverride(58.0f)
						[SNew(SFlickPlaylistGlyph).Playlist(Playlist).Color(Accent.CopyWithNewOpacity(bAvailable ? 0.72f : 0.25f))]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 7.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(bAvailable ? TEXT("PLAY  >") : TEXT("SOON")))
						.Font(UiFont(10, true)).ColorAndOpacity_Lambda([IsActive]() { return IsActive() ? Brand : Muted; })
					]
				]
			]
		]);
	if (OutButton) *OutButton = CardButton;
	return SNew(SBox).HeightOverride(UiMetrics::PlaylistCardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTrainingActivityCard(
	const EFlickTrainingActivity Activity,
	const FString& Label,
	const FString& Summary,
	const FString& Detail,
	const FLinearColor& Accent,
	TSharedPtr<SButton>* OutButton,
	const int32 TestPlayersPerTeam)
{
	const bool bTestActivity = Activity == EFlickTrainingActivity::ArenaControlBotMatch;
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this, Activity, bTestActivity, TestPlayersPerTeam]()
		{
			if (GameMode.IsValid())
			{
				GameMode->SetMatchmakingPlayersPerTeam(bTestActivity ? TestPlayersPerTeam : 1);
				const bool bBotMatch = Activity == EFlickTrainingActivity::BotMatch
					|| Activity == EFlickTrainingActivity::BobBotMatch
					|| bTestActivity;
				GameMode->SelectMatchVariant(
					Activity == EFlickTrainingActivity::BobBotMatch
						? EFlickMatchVariant::Bob
						: EFlickMatchVariant::Classic);
				if (bBotMatch)
				{
					if (bTestActivity)
					{
						GameMode->StartTestArenaBotMatch(TestPlayersPerTeam);
					}
					else
					{
						GameMode->StartTrainingBotMatch();
					}
					return FReply::Handled();
				}
			}

			SelectedTrainingActivity = Activity;
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakCardButton = CardButton;
	const auto IsActive = [WeakCardButton]()
	{
		const TSharedPtr<SButton> Button = WeakCardButton.Pin();
		return Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
	};
	CardButton->SetContent(
		SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor_Lambda([IsActive]() { return IsActive() ? Brand : Hairline.CopyWithNewOpacity(0.5f); })
		.Padding(1.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([IsActive]() { return IsActive() ? FMath::Lerp(PanelRaised, Brand, 0.04f) : Panel; })
			.Padding(FMargin(24.0f, 20.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[SNew(STextBlock).Text(FText::FromString(bTestActivity ? TEXT("THE LAB  /  EXPERIMENTAL") : TEXT("PRACTICE  /  OFFLINE"))).Font(UiFont(10, true)).ColorAndOpacity(Accent)]
					+ SHorizontalBox::Slot().AutoWidth()
					[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(12, true)).ColorAndOpacity(Brand)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(30)).ColorAndOpacity(Paper).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Summary)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 10.0f)
				[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.45f))]]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(Detail)).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Activity == EFlickTrainingActivity::FreePlay ? TEXT("CHOOSE YOUR ARENA  >") : TEXT("START MATCH  >")))
					.Font(UiFont(11, true)).ColorAndOpacity(Brand)
				]
			]
		]);
	if (OutButton) *OutButton = CardButton;
	return SNew(SBox).HeightOverride(UiMetrics::TrainingCardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPlayFormatCard(
	const int32 PlayersPerTeam,
	const bool bBob,
	TSharedPtr<SButton>* OutButton)
{
	const bool bAvailable = bBob || (PlayersPerTeam >= 1 && PlayersPerTeam <= 3);
	const FString Title = bBob ? TEXT("BOB") : FString::Printf(TEXT("%dV%d KNOCKOUT"), PlayersPerTeam, PlayersPerTeam);
	const auto IsSelected = [this, PlayersPerTeam, bBob]()
	{
		return GameMode.IsValid() && (bBob
			? GameMode->GetSelectedMatchVariant() == EFlickMatchVariant::Bob
			: GameMode->GetSelectedMatchVariant() == EFlickMatchVariant::Classic
				&& GameMode->GetMatchmakingPlayersPerTeam() == PlayersPerTeam);
	};
	TSharedRef<SButton> CardButton = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.IsEnabled(bAvailable)
		.Cursor(bAvailable ? EMouseCursor::Hand : EMouseCursor::Default)
		.OnClicked_Lambda([this, PlayersPerTeam, bBob]()
		{
			if (GameMode.IsValid())
			{
				GameMode->SetMatchmakingPlayersPerTeam(PlayersPerTeam);
				GameMode->SelectMatchVariant(bBob ? EFlickMatchVariant::Bob : EFlickMatchVariant::Classic);
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakCardButton = CardButton;
	const auto IsActive = [WeakCardButton, bAvailable]()
	{
		const TSharedPtr<SButton> Button = WeakCardButton.Pin();
		return bAvailable && Button.IsValid() && (Button->IsHovered() || Button->HasKeyboardFocus());
	};
	CardButton->SetContent(
		SNew(SBorder)
		.BorderImage(WhiteBrush())
		.BorderBackgroundColor_Lambda([IsActive, IsSelected]() { return IsSelected() || IsActive() ? Brand : Hairline.CopyWithNewOpacity(0.5f); })
		.Padding(1.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([IsSelected, IsActive]() { return IsSelected() ? FMath::Lerp(Panel, Brand, 0.065f) : IsActive() ? PanelRaised : Panel; })
			.Padding(FMargin(24.0f, 17.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[SNew(STextBlock).Text(FText::FromString(bBob ? TEXT("POCKET GAME  /  1V1") : TEXT("4 PUCKS PER PLAYER"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock).Text_Lambda([IsSelected]() { return FText::FromString(IsSelected() ? TEXT("[ SELECTED ]") : TEXT("[ SELECT ]")); })
						.Font(UiFont(10, true)).ColorAndOpacity_Lambda([IsSelected]() { return IsSelected() ? Brand : Muted; })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, PlayersPerTeam, bBob, Title]()
						{
							if (SelectedPlayPlaylist != EFlickPlayPlaylist::Training) return FText::FromString(Title);
							if (SelectedTrainingActivity == EFlickTrainingActivity::BotMatch) return FText::FromString(TEXT("1V1 VS BOT"));
							return FText::FromString(bBob ? TEXT("BOB PRACTICE") : FString::Printf(TEXT("%dV%d ARENA"), PlayersPerTeam, PlayersPerTeam));
						})
						.Font(DisplayFont(32)).ColorAndOpacity(bAvailable ? Paper : Muted)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
					[SNew(SFlickArenaDiagram).PlayersPerTeam(PlayersPerTeam).Bob(bBob)]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, bBob]()
					{
						return FText::FromString(SelectedPlayPlaylist == EFlickPlayPlaylist::Training
							? SelectedTrainingActivity == EFlickTrainingActivity::BotMatch
								? TEXT("You play blue. The bot plays orange.")
								: TEXT("Set up the board. Find your shot. Repeat.")
							: bBob ? TEXT("Pocket your color before your opponent clears theirs.") : TEXT("Clear the opposing team. First to three rounds wins."));
					})
					.Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, bBob, bAvailable]()
					{
						return FText::FromString(!bAvailable ? TEXT("UNAVAILABLE")
							: SelectedPlayPlaylist == EFlickPlayPlaylist::Training ? TEXT("OFFLINE PRACTICE")
							: bBob ? TEXT("ONE BOARD  /  STANDARD PUCKS") : TEXT("BEST OF FIVE  /  TEAM KNOCKOUT"));
					})
					.Font(UiFont(10, true)).ColorAndOpacity(Brand)
				]
			]
		]);
	if (OutButton) *OutButton = CardButton;
	return SNew(SBox).HeightOverride(UiMetrics::FormatCardHeight)[CardButton];
}

TSharedRef<SWidget> SFlickGameLayer::BuildOnlineBrowser()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.001f, 0.004f, 0.009f, 0.92f))
		]
		+ SOverlay::Slot().VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(116.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.008f, 0.016f, 0.95f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.76f))
				.CutSize(10.0f)
				.BorderWidth(1.1f)
				.Padding(FMargin(44.0f, 22.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(6.0f).HeightOverride(55.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("ONLINE LOBBIES"))).Font(UiFont(36, true)).ColorAndOpacity(FLinearColor::White)]
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("STEAM SESSION DISCOVERY"))).Font(UiFont(10, true)).ColorAndOpacity(Cyan)]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[SNew(STextBlock).Text(FText::FromString(TEXT("SIGNED IN AS"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 4.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
								return FText::FromString(Sessions ? Sessions->GetLocalDisplayName() : TEXT("OFFLINE"));
							})
							.Font(UiFont(17, true)).ColorAndOpacity(Cyan)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
								const EFlickMatchVariant Variant = GameMode.IsValid()
									? GameMode->GetSelectedMatchVariant() : EFlickMatchVariant::Classic;
								const int32 TeamSize = GameMode.IsValid() ? GameMode->GetMatchmakingPlayersPerTeam() : 1;
								return FText::FromString(Ranking
									? Ranking->GetProgressLabel(Variant, TeamSize)
									: TEXT("RANK UNAVAILABLE"));
							})
							.Font(UiFont(9, true)).ColorAndOpacity(Muted)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().Padding(44.0f, 142.0f, 44.0f, 105.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.006f, 0.018f, 0.03f, 0.97f))
				.AccentColor(Hairline)
				.CutSize(12.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(18.0f, 12.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(Sessions ? Sessions->GetStatusMessage() : TEXT("SESSION SERVICE UNAVAILABLE"));
						})
						.Font(UiFont(11, true))
						.ColorAndOpacity_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FSlateColor(Sessions && Sessions->GetState() == EFlickSessionState::Error ? Orange : Cyan);
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(FString::Printf(TEXT("SERVICE  /  %s"), Sessions ? *Sessions->GetOnlineServiceName() : TEXT("OFFLINE")));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(Muted)
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(0)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(1)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(2)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(3)]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 7.0f)[BuildOnlineSessionRow(4)]
				+ SVerticalBox::Slot().FillHeight(1.0f)[BuildOnlineSessionRow(5)]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(82.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.95f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.58f))
				.CutSize(8.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(34.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(170.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CloseOnlineBrowser(); return FReply::Handled(); }))]]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)[SNew(SBox).WidthOverride(230.0f)[MakeMenuButton(TEXT("HOST LOCAL"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->HostLocalNetworkMatch(); return FReply::Handled(); }))]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)[SNew(SBox).WidthOverride(230.0f)[MakeMenuButton(TEXT("JOIN LOCAL"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->JoinLocalNetworkMatch(); return FReply::Handled(); }))]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(190.0f)
						[
							MakeMenuButton(TEXT("REFRESH"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr) Sessions->FindSessions();
								return FReply::Handled();
							}), false, false, 50.0f, &OnlineBrowserDefaultButton)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(170.0f)
						[
							SNew(SButton)
							.ButtonStyle(&MenuButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.IsEnabled_Lambda([this]()
							{
								const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
								return !Sessions || !Sessions->IsMatchmakingActive();
							})
							.OnClicked_Lambda([this]()
							{
								if (GameMode.IsValid()) GameMode->ToggleRankedQueue();
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									return FText::FromString(GameMode.IsValid() && GameMode->IsRankedQueueSelected()
										? TEXT("RANKED") : TEXT("CASUAL"));
								})
								.Font(UiFont(13, true))
								.ColorAndOpacity_Lambda([this]()
								{
									return FSlateColor(GameMode.IsValid() && GameMode->IsRankedQueueSelected() ? Orange : Cyan);
								})
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(160.0f)
						[
							SNew(SButton)
							.ButtonStyle(&MenuButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([this]()
							{
								if (GameMode.IsValid()) GameMode->CycleMatchmakingPlayersPerTeam(1);
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const int32 TeamSize = GameMode.IsValid() ? GameMode->GetMatchmakingPlayersPerTeam() : 1;
									return FText::FromString(FString::Printf(TEXT("%dV%d"), TeamSize, TeamSize));
								})
								.Font(UiFont(14, true)).ColorAndOpacity(Cyan)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(260.0f)
						[
							SNew(SButton)
							.ButtonStyle(&PrimaryButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([this]()
							{
								if (!GameMode.IsValid()) return FReply::Handled();
								const UFlickSessionSubsystem* Sessions = GameMode->GetFlickSessionSubsystem();
								if (Sessions && Sessions->IsMatchmakingActive()) GameMode->CancelUnrankedMatchmaking();
								else GameMode->StartSelectedMatchmaking();
								return FReply::Handled();
							})
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
									if (Sessions && Sessions->IsMatchmakingActive()) return FText::FromString(TEXT("CANCEL QUEUE"));
									return FText::FromString(GameMode.IsValid() && GameMode->IsRankedQueueSelected()
										? TEXT("FIND RANKED") : TEXT("FIND CASUAL"));
								})
								.Font(UiFont(15, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildOnlineSessionRow(const int32 ResultIndex)
{
	return SNew(SBox)
		.Visibility_Lambda([this, ResultIndex]()
		{
			const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
			return Sessions && Sessions->GetBrowserEntries().IsValidIndex(ResultIndex) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.008f, 0.018f, 0.03f, 0.98f))
			.AccentColor(Hairline)
			.CutSize(12.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(19.0f, 7.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.34f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, ResultIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
						return FText::FromString(Sessions && Sessions->GetBrowserEntries().IsValidIndex(ResultIndex) ? Sessions->GetBrowserEntries()[ResultIndex].OwnerName : FString());
					}).Font(UiFont(14, true)).ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, ResultIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
						if (!Sessions || !Sessions->GetBrowserEntries().IsValidIndex(ResultIndex)) return FText::GetEmpty();
						const FFlickSessionBrowserEntry& Entry = Sessions->GetBrowserEntries()[ResultIndex];
						return FText::FromString(Entry.bMatchmaking
							? FString::Printf(
								TEXT("%s  /  %dV%d %s%s"),
								*GetMatchVariantName(Entry.Variant),
								Entry.PlayersPerTeam,
								Entry.PlayersPerTeam,
								Entry.bRanked ? TEXT("RANKED") : TEXT("CASUAL"),
								Entry.bRanked ? *FString::Printf(TEXT("  /  %d MMR"), Entry.QueueRating) : TEXT(""))
							: GetMatchVariantName(Entry.Variant));
					}).Font(UiFont(11, true)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().FillWidth(0.18f).VAlign(VAlign_Center)
				[
					SNew(STextBlock).Text_Lambda([this, ResultIndex]()
					{
						const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
						if (!Sessions || !Sessions->GetBrowserEntries().IsValidIndex(ResultIndex)) return FText::GetEmpty();
						const FFlickSessionBrowserEntry& Entry = Sessions->GetBrowserEntries()[ResultIndex];
						return FText::FromString(FString::Printf(TEXT("%d / %d   %d MS"), Entry.CurrentPlayers, Entry.MaximumPlayers, Entry.PingMilliseconds));
					}).Font(UiFont(10, true)).ColorAndOpacity(Muted)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(150.0f)
					[
						MakeMenuButton(TEXT("JOIN"), FOnClicked::CreateLambda([this, ResultIndex]()
						{
							if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr) Sessions->JoinSession(ResultIndex);
							return FReply::Handled();
						}), true, false, 42.0f)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildNetworkLobby()
{
	auto BuildPlayerSlot = [this](const EFlickTeam Team, const int32 PlayerSlot, const FLinearColor& Accent) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
		.Visibility_Lambda([this, PlayerSlot]()
		{
			return GameMode.IsValid() && PlayerSlot < GameMode->GetPlayersPerTeam()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[
		SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.006f, 0.015f, 0.026f, 0.96f))
			.AccentColor(Accent)
			.CutSize(15.0f)
			.BorderWidth(1.5f)
			.Padding(FMargin(20.0f, 14.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(
							TEXT("TEAM %d  /  PLAYER %d%s"),
							GetTeamNumber(Team),
							PlayerSlot + 1,
							Team == EFlickTeam::Player1 && PlayerSlot == 0 ? TEXT("  /  HOST") : TEXT(""))))
						.Font(UiFont(9, true)).ColorAndOpacity(Accent)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team, PlayerSlot]()
						{
							const AFlickPlayerState* Player = GameMode.IsValid() ? GameMode->GetLobbyPlayer(Team, PlayerSlot) : nullptr;
							return FText::FromString(Player ? Player->GetPlayerName() : TEXT("WAITING FOR PLAYER"));
						})
						.Font(UiFont(18, true)).ColorAndOpacity(FLinearColor::White)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team, PlayerSlot]()
					{
						const AFlickPlayerState* Player = GameMode.IsValid() ? GameMode->GetLobbyPlayer(Team, PlayerSlot) : nullptr;
						if (!Player) return FText::FromString(TEXT("OPEN SLOT"));
						if (GameMode.IsValid() && GameMode->IsRankedMatch() && !Player->IsRankedIdentityVerified())
						{
							return FText::FromString(TEXT("VERIFYING IDENTITY"));
						}
						return FText::FromString(Player->IsLobbyReady() ? TEXT("READY") : TEXT("NOT READY"));
					})
					.Font(UiFont(11, true))
					.ColorAndOpacity_Lambda([this, Team, PlayerSlot, Accent]()
					{
						const AFlickPlayerState* Player = GameMode.IsValid() ? GameMode->GetLobbyPlayer(Team, PlayerSlot) : nullptr;
						return FSlateColor(Player && Player->IsLobbyReady() ? Accent : Muted);
					})
				]
			]
		];
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.004f, 0.01f, 0.78f))
		]
		+ SOverlay::Slot().Padding(34.0f, 24.0f, 34.0f, 96.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(6.0f).HeightOverride(58.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(16.0f, 0.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return FText::FromString(GameMode.IsValid() && GameMode->IsMatchmakingSession()
								? GameMode->IsRankedMatch() ? TEXT("RANKED MATCHMAKING") : TEXT("CASUAL MATCHMAKING")
								: Sessions && Sessions->HasActiveSession() ? TEXT("STEAM LOBBY") : TEXT("LOCAL LOBBY"));
						})
						.Font(UiFont(34, true)).ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(GameMode.IsValid() && GameMode->IsMatchmakingSession()
								? GameMode->IsRankedMatch()
									? TEXT("MMR MATCHED  /  CONFIRM READY  /  RATING ACTIVE")
									: TEXT("FIND PLAYERS  /  CONFIRM READY  /  PLAY")
								: TEXT("CHOOSE A COMPETITION  /  READY UP  /  HOST STARTS"));
						})
						.Font(UiFont(9, true)).ColorAndOpacity(Cyan)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!GameMode.IsValid()) return FText::FromString(TEXT("WAITING FOR PLAYERS"));
						const AFlickGameState* State = GameMode->GetFlickGameState();
						if (State && State->bMatchmakingTimedOut) return FText::FromString(TEXT("QUEUE TIMED OUT"));
						if (GameMode->IsRankedMatch()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player1) == GameMode->GetPlayersPerTeam()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player2) == GameMode->GetPlayersPerTeam()
							&& !GameMode->AreRankedPlayersAuthenticated())
						{
							return FText::FromString(TEXT("VERIFYING STEAM IDENTITIES"));
						}
						if (GameMode->CanStartNetworkMatch()) return FText::FromString(TEXT("ALL PLAYERS READY"));
						if (GameMode->IsMatchmakingSession()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player1) == GameMode->GetPlayersPerTeam()
							&& GameMode->GetLobbyPlayerCount(EFlickTeam::Player2) == GameMode->GetPlayersPerTeam())
						{
							return FText::FromString(TEXT("MATCH FOUND - CONFIRM READY"));
						}
						return FText::FromString(TEXT("SEARCHING FOR PLAYERS"));
					})
					.Font(UiFont(12, true))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(GameMode.IsValid() && GameMode->CanStartNetworkMatch() ? Cyan : Muted); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player1, 0, Cyan)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player1, 1, Cyan)]
					+ SVerticalBox::Slot().AutoHeight()[BuildPlayerSlot(EFlickTeam::Player1, 2, Cyan)]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player2, 0, Orange)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildPlayerSlot(EFlickTeam::Player2, 1, Orange)]
					+ SVerticalBox::Slot().AutoHeight()[BuildPlayerSlot(EFlickTeam::Player2, 2, Orange)]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() ? GetMatchVariantName(GameMode->GetSelectedMatchVariant()) : TEXT("KNOCKOUT")); })
				.Font(UiFont(17, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetCurrentModeAccent()); })
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
			SNew(SUniformGridPanel)
				.SlotPadding(FMargin(8.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)[BuildModeCard(EFlickMatchVariant::Classic)]
				+ SUniformGridPanel::Slot(1, 0)[BuildModeCard(EFlickMatchVariant::Bob)]
			]
		]
		+ SOverlay::Slot().VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(82.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.002f, 0.007f, 0.014f, 0.95f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.58f))
				.CutSize(8.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(34.0f, 14.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(180.0f)[MakeMenuButton(GameMode.IsValid() && GameMode->IsMatchmakingSession() ? TEXT("CANCEL QUEUE") : TEXT("CANCEL LOBBY"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CancelNetworkLobby(); return FReply::Handled(); }), false, true)]]
					+ SHorizontalBox::Slot().AutoWidth().Padding(12.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(230.0f)
						.Visibility_Lambda([this]()
						{
							const UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr;
							return Sessions && Sessions->HasActiveSession()
								&& (!GameMode.IsValid() || !GameMode->IsMatchmakingSession())
								? EVisibility::Visible : EVisibility::Collapsed;
						})
						[
							MakeMenuButton(TEXT("INVITE FRIENDS"), FOnClicked::CreateLambda([this]()
							{
								if (UFlickSessionSubsystem* Sessions = GameMode.IsValid() ? GameMode->GetFlickSessionSubsystem() : nullptr) Sessions->OpenInviteOverlay();
								return FReply::Handled();
							}))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(210.0f)
						[
							SAssignNew(LobbyDefaultButton, SButton)
							.ButtonStyle(&MenuButtonStyle)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.OnClicked_Lambda([this]() { if (PlayerController.IsValid()) PlayerController->ToggleLobbyReady(); return FReply::Handled(); })
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const AFlickPlayerState* Player = PlayerController.IsValid() ? PlayerController->GetPlayerState<AFlickPlayerState>() : nullptr;
									return FText::FromString(Player && Player->IsLobbyReady() ? TEXT("SET NOT READY") : TEXT("READY UP"));
								})
								.Font(UiFont(14, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(240.0f)
						.IsEnabled_Lambda([this]() { return GameMode.IsValid() && GameMode->CanStartNetworkMatch(); })
						[
							MakeMenuButton(TEXT("START"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestStartNetworkMatch(); return FReply::Handled(); }), true)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildModeCard(const EFlickMatchVariant Variant)
{
	const FString CountLabel = GetMatchVariantFormatLabel(Variant);
	const FString IndexLabel = Variant == EFlickMatchVariant::Classic ? TEXT("01") : TEXT("02");
	const FString FormatBadge = Variant == EFlickMatchVariant::Bob ? TEXT("BOB") : CountLabel;
	return SNew(SBox)
		.HeightOverride(294.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor::Transparent)
			.AccentColor_Lambda([this, Variant]()
			{
				const bool bSelected = GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant;
				return bSelected ? GetModeAccent(Variant) : FLinearColor(0.075f, 0.11f, 0.14f, 0.9f);
			})
			.CutSize(18.0f)
			.BorderWidth(2.0f)
			.Padding(FMargin(2.0f))
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.IsEnabled_Lambda([this]() { return !GameMode.IsValid() || !GameMode->IsMatchmakingSession(); })
				.OnClicked_Lambda([this, Variant]()
				{
					if (GameMode.IsValid()) GameMode->SelectMatchVariant(Variant);
					return FReply::Handled();
				})
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor_Lambda([this, Variant]()
					{
						const bool bSelected = GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant;
					return bSelected ? FLinearColor(0.006f, 0.021f, 0.034f, 0.92f) : FLinearColor(0.003f, 0.009f, 0.016f, 0.86f);
				})
					.AccentColor(FLinearColor::Transparent)
					.CutSize(15.0f)
					.DrawNeutralOutline(false)
					.Padding(FMargin(28.0f, 20.0f, 24.0f, 20.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush())
								.BorderBackgroundColor(FLinearColor(0.025f, 0.075f, 0.105f, 0.82f))
								.Padding(FMargin(7.0f, 4.0f))
								[
									SNew(STextBlock).Text(FText::FromString(IndexLabel)).Font(UiFont(10, true)).ColorAndOpacity(FLinearColor(0.72f, 0.82f, 0.88f, 1.0f))
								]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SFlickAngularBorder)
								.BackgroundColor(FLinearColor(0.004f, 0.012f, 0.021f, 0.88f))
								.AccentColor_Lambda([this, Variant]() { return FSlateColor(GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant ? GetModeAccent(Variant) : Hairline).GetSpecifiedColor(); })
								.CutSize(5.0f)
								.BorderWidth(0.8f)
								.Padding(FMargin(15.0f, 6.0f))
								[
									SNew(STextBlock).Text(FText::FromString(FormatBadge)).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(FText::FromString(GetMatchVariantName(Variant)))
								.Font(UiFont(25, true))
								.ColorAndOpacity_Lambda([this, Variant]() { return FSlateColor(GetModeAccent(Variant)); })
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(SHorizontalBox)
								.Visibility_Lambda([this, Variant]() { return GameMode.IsValid() && GameMode->GetSelectedMatchVariant() == Variant ? EVisibility::Visible : EVisibility::Collapsed; })
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 7.0f, 0.0f)
								[
									SNew(SFlickRoundPip).Color_Lambda([this, Variant]() { return GetModeAccent(Variant); }).Filled(true)
								]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(STextBlock).Text(FText::FromString(TEXT("SELECTED"))).Font(UiFont(9, true)).ColorAndOpacity_Lambda([this, Variant]() { return FSlateColor(GetModeAccent(Variant)); })
								]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 23.0f, 0.0f, 0.0f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[
								SNew(SBox).WidthOverride(28.0f).HeightOverride(2.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor_Lambda([this, Variant]() { return GetModeAccent(Variant); })]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
							[
								SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.12f, 0.20f, 0.25f, 0.42f))]
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(GetMatchVariantSummary(Variant)))
							.Font(UiFont(11))
							.ColorAndOpacity(Muted)
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush())
							.BorderBackgroundColor(FLinearColor(0.012f, 0.03f, 0.044f, 0.88f))
							.Padding(FMargin(12.0f, 18.0f))
							[
								SNew(STextBlock).Text(FText::FromString(GetMatchVariantSeriesLabel(Variant))).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadout()
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.001f, 0.004f, 0.008f, 0.92f))
		]
		+ SOverlay::Slot().Padding(10.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::Both)
			[
				SNew(SBox).WidthOverride(1720.0f).HeightOverride(900.0f)
				[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(7.0f).HeightOverride(62.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(18.0f, 0.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("LINEUP"))).Font(UiFont(42, true)).ColorAndOpacity(FLinearColor::White)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const int32 Count = GameMode.IsValid() ? GameMode->GetLoadoutEditingPieceCount() : 4;
								return FText::FromString(FString::Printf(TEXT("EDIT YOUR %d PUCKS"), Count));
							})
							.Font(UiFont(12, true)).ColorAndOpacity(Cyan)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 0.0f, 0.0f, 5.0f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("EDIT LINEUP FOR"))).Font(UiFont(9, true)).ColorAndOpacity(Muted)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								BuildLoadoutModeButton(EFlickMatchVariant::Classic, TEXT("4 PUCKS"))
							]
						]
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 16.0f, 0.0f, 14.0f)
				[
					BuildLoadoutWorkspace()
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(170.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid()) GameMode->CloseLoadout();
							return FReply::Handled();
						}))]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const int32 Count = GameMode.IsValid() ? GameMode->GetLoadoutEditingPieceCount() : 4;
							return FText::FromString(FString::Printf(TEXT("%d PUCK LINEUP  /  %s"), Count,
								GameMode.IsValid() ? *GetLineupPresetName(GameMode->GetLoadoutPreset(EFlickTeam::Player1)) : TEXT("CLASS 1")));
						})
						.Font(UiFont(10, true)).ColorAndOpacity(Muted)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(285.0f)[MakeMenuButton(TEXT("SAVE LINEUP"), FOnClicked::CreateLambda([this]()
						{
							if (GameMode.IsValid())
							{
								GameMode->PlayMenuSound(true);
								GameMode->CloseLoadout();
							}
							return FReply::Handled();
						}), true, false, 52.0f)]
					]
				]
			]
			]
		]
		;
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutModeButton(
	const EFlickMatchVariant Variant,
	const FString& Label)
{
	return SNew(SBox)
		.WidthOverride(126.0f)
		.HeightOverride(42.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, Variant]()
			{
				const bool bSelected = GameMode.IsValid()
					&& GameMode->GetLoadoutEditingVariant() == Variant;
				return bSelected
					? GetModeAccent(Variant).CopyWithNewOpacity(0.2f)
					: FLinearColor(0.002f, 0.009f, 0.017f, 0.9f);
			})
			.AccentColor_Lambda([this, Variant]()
			{
				return GameMode.IsValid() && GameMode->GetLoadoutEditingVariant() == Variant
					? GetModeAccent(Variant)
					: Hairline;
			})
			.CutSize(7.0f)
			.BorderWidth(1.4f)
			.Padding(1.0f)
			[
				SNew(SButton)
				.ButtonStyle(&TransparentButtonStyle)
				.ContentPadding(0.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnClicked_Lambda([this, Variant]()
				{
					if (GameMode.IsValid())
					{
						GameMode->SelectLoadoutEditingVariant(Variant);
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.Font(UiFont(12, true))
					.ColorAndOpacity_Lambda([this, Variant]()
					{
						return FSlateColor(GameMode.IsValid() && GameMode->GetLoadoutEditingVariant() == Variant
							? GetModeAccent(Variant)
							: Muted);
					})
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutWorkspace()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(350.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.25f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
				[
					BuildLineupRadar()
				]
				+ SHorizontalBox::Slot().FillWidth(0.33f).Padding(10.0f, 0.0f)
				[
					BuildLoadoutFormation(EFlickTeam::Player1)
				]
				+ SHorizontalBox::Slot().FillWidth(0.42f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						BuildLoadoutComparison(EFlickTeam::Player1)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
					[
						BuildLoadoutPresetBar(EFlickTeam::Player1)
					]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 14.0f, 0.0f, 0.0f)
		[
			BuildArchetypePicker(EFlickTeam::Player1)
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLineupRadar()
{
	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.004f, 0.013f, 0.023f, 0.96f))
		.AccentColor(Hairline)
		.CutSize(10.0f)
		.Padding(FMargin(16.0f, 12.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("LINEUP PROFILE"))).Font(UiFont(12, true)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() ? GetLineupPresetName(GameMode->GetLoadoutPreset(EFlickTeam::Player1)) : TEXT("CLASS 1")); })
					.Font(UiFont(10, true)).ColorAndOpacity(FLinearColor::White)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().Padding(24.0f, 10.0f)
				[
					SNew(SFlickRadarChart)
					.ValueProvider([this]() { return GetLineupProfileStats(EFlickTeam::Player1); })
					.AccentColor(Cyan)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("SPEED"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 55.0f, 2.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("WEIGHT"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 2.0f, 44.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("IMPACT"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("CONTROL"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(2.0f, 0.0f, 0.0f, 44.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("COAST"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(2.0f, 55.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("STABILITY"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutFormation(const EFlickTeam Team)
{
	constexpr float FormationPuckSize = 112.0f;
	TSharedRef<SConstraintCanvas> Formation = SNew(SConstraintCanvas);
	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		Formation->AddSlot()
			.Anchors(TAttribute<FAnchors>::CreateLambda([this, SlotIndex]()
			{
				const bool bThreePuckFormation = GameMode.IsValid()
					&& GameMode->GetLoadoutEditingPieceCount() == 3;
				if (bThreePuckFormation)
				{
					const FVector2D Positions[] = {
						FVector2D(0.5f, 0.3f),
						FVector2D(0.29f, 0.7f),
						FVector2D(0.71f, 0.7f),
						FVector2D(0.5f, 0.9f)};
					return FAnchors(Positions[SlotIndex].X, Positions[SlotIndex].Y);
				}
				const FVector2D Positions[] = {
					FVector2D(0.29f, 0.31f),
					FVector2D(0.71f, 0.31f),
					FVector2D(0.31f, 0.7f),
					FVector2D(0.69f, 0.7f)};
				return FAnchors(Positions[SlotIndex].X, Positions[SlotIndex].Y);
			}))
			.Alignment(FVector2D(0.5f, 0.5f))
			.Offset(FMargin(0.0f, 0.0f, FormationPuckSize, FormationPuckSize))
			.AutoSize(false)
		[
			BuildFormationPuck(Team, SlotIndex)
		];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.004f, 0.013f, 0.023f, 0.96f))
		.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
		.CutSize(18.0f)
		.BorderWidth(1.4f)
		.Padding(FMargin(8.0f))
		[
			SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(1.0f).HeightOverride(150.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)
					]
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(150.0f).HeightOverride(1.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)
					]
				]
				+ SOverlay::Slot().Padding(10.0f)
				[
					Formation
				]
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(10.0f, 8.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("STARTING FORMATION"))).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 7.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]()
					{
						const int32 Slot = GetSelectedLoadoutSlot(Team);
						return FText::FromString(FString::Printf(TEXT("SLOT %02d  /  %s"),
							Slot + 1,
							GameMode.IsValid() ? *GetPieceArchetypeName(GameMode->GetLoadoutPiece(Team, Slot)) : TEXT("STANDARD")));
					})
					.Font(UiFont(10, true))
					.ColorAndOpacity(GetTeamAccent(Team))
				]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildFormationPuck(const EFlickTeam Team, const int32 SlotIndex)
{
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.ToolTipText_Lambda([this, Team, SlotIndex]()
		{
			return FText::FromString(GameMode.IsValid()
				? FString::Printf(TEXT("SLOT %02d  /  %s"), SlotIndex + 1, *GetPieceArchetypeName(GameMode->GetLoadoutPiece(Team, SlotIndex)))
				: TEXT("SELECT PUCK"));
		})
		.OnClicked_Lambda([this, Team, SlotIndex]()
		{
			SelectLoadoutSlot(Team, SlotIndex);
			return FReply::Handled();
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SFlickPuckDisc)
				.TeamColor(GetTeamAccent(Team))
				.Archetype_Lambda([this, Team, SlotIndex]()
				{
					return GameMode.IsValid()
						? GameMode->GetLoadoutPiece(Team, SlotIndex)
						: EFlickPieceArchetype::Standard;
				})
				.AccentColor_Lambda([this, Team, SlotIndex]()
				{
					return GameMode.IsValid()
						? FlickPieceArchetypeRules::GetVisualAccent(GameMode->GetLoadoutPiece(Team, SlotIndex), GetTeamAccent(Team))
						: FLinearColor::White;
				})
				.Selected_Lambda([this, Team, SlotIndex]() { return GetSelectedLoadoutSlot(Team) == SlotIndex; })
				.RadiusScale_Lambda([this, Team, SlotIndex]()
				{
					return GameMode.IsValid()
						? FlickPieceArchetypeRules::Get(GameMode->GetLoadoutPiece(Team, SlotIndex)).RadiusMultiplier
						: 1.0f;
				})
			]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(3.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%d"), SlotIndex + 1)))
				.Font(UiFont(7, true))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
	if (Team == EFlickTeam::Player1 && SlotIndex == 0)
	{
		LoadoutDefaultButton = Button;
	}
	return SNew(SBox)
		.Visibility_Lambda([this, SlotIndex]() { return GetLoadoutRowVisibility(SlotIndex); })
		[
			Button
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutComparison(const EFlickTeam Team)
{
	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.006f, 0.016f, 0.026f, 0.97f))
		.AccentColor_Lambda([this, Team]() { return FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team)).CopyWithNewOpacity(0.72f); })
		.CutSize(10.0f)
		.Padding(FMargin(18.0f, 14.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(FString::Printf(TEXT("SLOT %02d"), GetSelectedLoadoutSlot(Team) + 1)); })
					.Font(UiFont(8, true))
					.ColorAndOpacity(Muted)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]()
					{
						const bool bHovering = Team == EFlickTeam::Player1
							? Player1HoveredLoadoutArchetype.IsSet()
							: Player2HoveredLoadoutArchetype.IsSet();
						return FText::FromString(bHovering ? TEXT("HOVER PREVIEW") : TEXT("EQUIPPED"));
					})
					.Font(UiFont(9, true))
					.ColorAndOpacity_Lambda([this, Team]() { return FSlateColor(FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team))); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(GetPieceArchetypeName(GetPreviewLoadoutArchetype(Team))); })
					.Font(UiFont(25, true))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).ClassLabel); })
					.Font(UiFont(10, true))
					.ColorAndOpacity_Lambda([this, Team]() { return FSlateColor(FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team))); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team]() { return FText::FromString(FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Summary); })
				.Font(UiFont(9, true))
				.ColorAndOpacity(Muted)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team]() { return FText::FromString(TEXT("+  ") + FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Strengths); })
				.Font(UiFont(8, true))
				.ColorAndOpacity(FLinearColor(0.28f, 0.9f, 0.55f, 1.0f))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team]() { return FText::FromString(TEXT("-  ") + FlickPieceArchetypeRules::Get(GetPreviewLoadoutArchetype(Team)).Weaknesses); })
				.Font(UiFont(8, true))
				.ColorAndOpacity(FLinearColor(1.0f, 0.42f, 0.18f, 1.0f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 0.0f)[BuildLoadoutStatRow(Team, TEXT("SPEED"), 0)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("WEIGHT"), 1)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("IMPACT"), 2)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("CONTROL"), 3)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("COAST"), 4)]
			+ SVerticalBox::Slot().AutoHeight()[BuildLoadoutStatRow(Team, TEXT("STABILITY"), 5)]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutStatRow(
	const EFlickTeam Team,
	const FString& Label,
	const int32 StatIndex)
{
	constexpr float BarWidth = 270.0f;
	return SNew(SBox).HeightOverride(23.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(72.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(8, true)).ColorAndOpacity(Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(7.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.004f, 0.008f, 0.014f, 1.0f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this, Team, StatIndex]()
						{
							return FOptionalSize(BarWidth * GetLoadoutStatValue(Team, StatIndex, false));
						}))
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.35f, 0.43f, 0.49f, 0.76f))
						]
					]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this, Team, StatIndex]()
						{
							return FOptionalSize(BarWidth * GetLoadoutStatValue(Team, StatIndex, true));
						}))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush())
							.BorderBackgroundColor_Lambda([this, Team]()
							{
								return FlickPieceArchetypeRules::GetVisualAccent(GetPreviewLoadoutArchetype(Team), GetTeamAccent(Team)).CopyWithNewOpacity(0.88f);
							})
						]
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team, StatIndex]()
				{
					const bool bHovering = Team == EFlickTeam::Player1
						? Player1HoveredLoadoutArchetype.IsSet()
						: Player2HoveredLoadoutArchetype.IsSet();
					if (!bHovering) return FText::FromString(TEXT("--"));
					const int32 Delta = FMath::RoundToInt((GetLoadoutStatValue(Team, StatIndex, true)
						- GetLoadoutStatValue(Team, StatIndex, false)) * 100.0f);
					return FText::FromString(Delta > 0
						? FString::Printf(TEXT("+%d"), Delta)
						: FString::Printf(TEXT("%d"), Delta));
				})
				.Font(UiFont(8, true))
				.ColorAndOpacity_Lambda([this, Team, StatIndex]()
				{
					const float Delta = GetLoadoutStatValue(Team, StatIndex, true)
						- GetLoadoutStatValue(Team, StatIndex, false);
					return FSlateColor(Delta > 0.01f
						? FLinearColor(0.22f, 0.92f, 0.56f, 1.0f)
						: Delta < -0.01f ? FLinearColor(1.0f, 0.38f, 0.16f, 1.0f) : Muted);
				})
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLoadoutPresetBar(const EFlickTeam Team)
{
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(2.0f));
	int32 Column = 0;
	for (const EFlickLineupPreset Preset : {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control})
	{
		TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.OnClicked_Lambda([this, Team, Preset]()
			{
				if (GameMode.IsValid())
				{
					if (Team == EFlickTeam::Player1) Player1HoveredLoadoutArchetype.Reset();
					else Player2HoveredLoadoutArchetype.Reset();
					GameMode->SelectLoadoutEditingPreset(Preset);
				}
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor(FLinearColor(0.01f, 0.022f, 0.034f, 0.98f))
				.Padding(FMargin(4.0f, 7.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(GetLineupPresetName(Preset)))
					.Font(UiFont(10, true))
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FLinearColor::White)
				]
			];
		const TWeakPtr<SButton> WeakButton = Button;
		Grid->AddSlot(Column++, 0)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([this, Team, Preset, WeakButton]()
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				if (Pinned.IsValid() && Pinned->HasKeyboardFocus()) return FLinearColor::White;
				return GameMode.IsValid() && GameMode->GetLoadoutEditingPreset() == Preset
					? GetTeamAccent(Team)
					: Hairline;
			})
			.Padding(1.0f)
			[
				Button
			]
		];
	}
	return Grid;
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassPlayerRow(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	const FLinearColor TeamAccent = GetTeamAccent(Team);
	TSharedRef<SUniformGridPanel> ClassGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(3.0f));
	int32 Column = 0;
	for (const EFlickLineupPreset Preset : {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control})
	{
		TSharedRef<SButton> ClassButton = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Cursor(EMouseCursor::Hand)
			.OnClicked_Lambda([this, Team, PlayerSlot, Preset]()
			{
				if (PlayerController.IsValid())
				{
					PlayerController->RequestSelectClass(Preset);
				}
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor_Lambda([this, Team, PlayerSlot, Preset, TeamAccent]()
				{
					return GetSelectedClassDraft() == Preset
						? GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.2f)
						: FLinearColor(0.006f, 0.017f, 0.027f, 0.96f);
				})
				.Padding(FMargin(7.0f, 6.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(GetLineupPresetName(Preset)))
						.Font(UiFont(11, true))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(GetLineupPresetRole(Preset)))
						.Font(UiFont(8, true))
						.ColorAndOpacity_Lambda([this, Team, PlayerSlot, Preset, TeamAccent]()
						{
							return GetSelectedClassDraft() == Preset
								? FSlateColor(GetTeamAccent(GetClassSelectionTeam()))
								: FSlateColor(Muted);
						})
					]
				]
			];

		const TWeakPtr<SButton> WeakClassButton = ClassButton;
		ClassGrid->AddSlot(Column++, 0)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.004f, 0.012f, 0.02f, 0.98f))
			.AccentColor_Lambda([this, Team, PlayerSlot, Preset, WeakClassButton, TeamAccent]()
			{
				const TSharedPtr<SButton> Pinned = WeakClassButton.Pin();
				const bool bSelected = GetSelectedClassDraft() == Preset;
				return bSelected || (Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()))
					? GetTeamAccent(GetClassSelectionTeam())
					: Hairline;
			})
			.CutSize(5.0f)
			.BorderWidth(1.0f)
			.UseAccentForOutline(true)
			.Padding(1.0f)
			[
				ClassButton
			]
		];
	}

	return SNew(SBox)
		.HeightOverride(100.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.006f, 0.017f, 0.028f, 0.96f))
			.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.52f); })
			.CutSize(7.0f)
			.BorderWidth(0.8f)
			.Padding(FMargin(12.0f, 9.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f, 0.0f, 0.0f, 5.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("PLAYER %d"), GetClassSelectionPlayerSlot() + 1)); })
					.Font(UiFont(10, true))
					.ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					ClassGrid
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassPuckCard(const int32 PieceSlot)
{
	return SNew(SBox)
		.HeightOverride(166.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.004f, 0.014f, 0.024f, 0.98f))
			.AccentColor_Lambda([this, PieceSlot]()
			{
				return FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam())).CopyWithNewOpacity(0.72f);
			})
			.CutSize(8.0f)
			.BorderWidth(0.9f)
			.UseAccentForOutline(true)
			.Padding(FMargin(12.0f, 10.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SBox).WidthOverride(120.0f).HeightOverride(120.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SFlickPuckDisc)
							.TeamColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()); })
							.Archetype_Lambda([this, PieceSlot]() { return GetSelectedClassPiece(PieceSlot); })
							.AccentColor_Lambda([this, PieceSlot]()
							{
								return FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam()));
							})
							.Selected(true)
							.RadiusScale_Lambda([this, PieceSlot]()
							{
								return FlickPieceArchetypeRules::Get(GetSelectedClassPiece(PieceSlot)).RadiusMultiplier;
							})
						]
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(11.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PieceSlot]() { return FText::FromString(GetPieceArchetypeName(GetSelectedClassPiece(PieceSlot))); })
						.Font(DisplayFont(20))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, PieceSlot]()
						{
							return FText::FromString(FlickPieceArchetypeRules::Get(GetSelectedClassPiece(PieceSlot)).ClassLabel);
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity_Lambda([this, PieceSlot]()
						{
							return FSlateColor(FlickPieceArchetypeRules::GetVisualAccent(GetSelectedClassPiece(PieceSlot), GetTeamAccent(GetClassSelectionTeam())));
						})
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, PieceSlot]()
						{
							return FText::FromString(FlickPieceArchetypeRules::Get(GetSelectedClassPiece(PieceSlot)).Summary);
						})
						.Font(UiFont(8, true))
						.ColorAndOpacity(Muted)
						.AutoWrapText(true)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassProfileStatRow(
	const FString& Label,
	const int32 StatIndex)
{
	constexpr float BarWidth = 290.0f;
	return SNew(SBox).HeightOverride(28.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(86.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(9, true)).ColorAndOpacity(Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(BarWidth).HeightOverride(8.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.002f, 0.007f, 0.012f, 1.0f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox)
						.WidthOverride(TAttribute<FOptionalSize>::CreateLambda([this, StatIndex]()
						{
							return FOptionalSize(BarWidth * GetSelectedClassStatValue(StatIndex));
						}))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush())
							.BorderBackgroundColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()); })
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassShowcase()
{
	TSharedRef<SUniformGridPanel> PuckGrid = SNew(SUniformGridPanel).SlotPadding(FMargin(5.0f));
	for (int32 PieceSlot = 0; PieceSlot < 4; ++PieceSlot)
	{
		PuckGrid->AddSlot(PieceSlot % 2, PieceSlot / 2)[BuildClassPuckCard(PieceSlot)];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.002f, 0.011f, 0.021f, 0.98f))
		.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.76f); })
		.CutSize(12.0f)
		.BorderWidth(1.0f)
		.UseAccentForOutline(true)
		.Padding(FMargin(18.0f, 15.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.62f).Padding(0.0f, 0.0f, 15.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("YOUR FOUR-PUCK LINEUP"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					PuckGrid
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.38f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.005f, 0.017f, 0.029f, 0.98f))
				.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.48f); })
				.CutSize(9.0f)
				.BorderWidth(0.8f)
				.Padding(FMargin(20.0f, 17.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(GetLineupPresetName(GetSelectedClassDraft())); })
						.Font(DisplayFont(30))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 7.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(GetLineupPresetRole(GetSelectedClassDraft())); })
						.Font(UiFont(11, true))
						.ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(GetLineupPresetSummary(GetSelectedClassDraft())); })
						.Font(UiFont(10))
						.ColorAndOpacity(Muted)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(TEXT("+  ") + GetLineupPresetStrengths(GetSelectedClassDraft())); })
						.Font(UiFont(9, true))
						.ColorAndOpacity(FLinearColor(0.28f, 0.9f, 0.55f, 1.0f))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(TEXT("-  ") + GetLineupPresetTradeoffs(GetSelectedClassDraft())); })
						.Font(UiFont(9, true))
						.ColorAndOpacity(FLinearColor(1.0f, 0.42f, 0.18f, 1.0f))
					]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("SPEED"), 0)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("WEIGHT"), 1)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("IMPACT"), 2)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("CONTROL"), 3)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("COAST"), 4)]
					+ SVerticalBox::Slot().AutoHeight()[BuildClassProfileStatRow(TEXT("STABILITY"), 5)]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildClassSelect()
{
	const EFlickTeam SelectionTeam = GetClassSelectionTeam();
	const int32 PlayerSlot = GetClassSelectionPlayerSlot();
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.004f, 0.01f, 0.88f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(28.0f)
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::DownOnly)
			[
				SNew(SBox).WidthOverride(1320.0f).HeightOverride(830.0f)
				[
					SNew(SFlickAngularBorder)
					.BackgroundColor(FLinearColor(0.001f, 0.01f, 0.02f, 0.985f))
					.AccentColor_Lambda([this]() { return GetTeamAccent(GetClassSelectionTeam()).CopyWithNewOpacity(0.82f); })
					.CutSize(18.0f).BorderWidth(1.2f).UseAccentForOutline(true).Padding(FMargin(34.0f, 28.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(STextBlock)
									.Text_Lambda([this]() { return FText::FromString(GameMode.IsValid() && GameMode->IsChangingClassForNextRound() ? TEXT("CHANGE CLASS") : TEXT("SELECT CLASS")); })
									.Font(DisplayFont(34)).ColorAndOpacity(FLinearColor::White)
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text_Lambda([this]()
									{
										if (GameMode.IsValid() && GameMode->IsChangingClassForNextRound())
										{
											return FText::FromString(TEXT("YOUR NEW CLASS APPLIES WHEN THE NEXT ROUND STARTS"));
										}
										const AFlickGameState* State = GetScoreboardGameState();
										if (State && State->bNetworkClassSelectionActive)
										{
											return FText::FromString(TEXT("CHOOSE YOUR CLASS FOR THIS MATCH"));
										}
										return FText::FromString(GameMode.IsValid() && GameMode->IsPreparingTrainingBotMatch()
											? TEXT("CHOOSE YOUR CLASS; THE BOT RECEIVES A RANDOM CLASS")
											: TEXT("CHOOSE BLUE PLAYER 1'S CLASS; OTHER LOCAL PLAYERS RECEIVE RANDOM CLASSES"));
									})
									.Font(UiFont(11, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const EFlickTeam Team = GetClassSelectionTeam();
									return FText::FromString(FString::Printf(
										TEXT("%s TEAM  /  PLAYER %d"),
										Team == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
										GetClassSelectionPlayerSlot() + 1));
								})
								.Font(UiFont(10, true)).ColorAndOpacity_Lambda([this]() { return FSlateColor(GetTeamAccent(GetClassSelectionTeam())); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 12.0f)
						[
							BuildClassPlayerRow(SelectionTeam, PlayerSlot)
						]
						+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 0.0f, 0.0f, 18.0f)
						[
							BuildClassShowcase()
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SBox)
								.Visibility_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return State && State->bNetworkClassSelectionActive
										? EVisibility::Collapsed : EVisibility::Visible;
								})
								.WidthOverride(190.0f)
								[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CancelClassSelection(); return FReply::Handled(); }), false, false, 56.0f)]
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
							[
								SNew(STextBlock)
								.Visibility_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return (State && State->bNetworkClassSelectionActive)
										|| (GameMode.IsValid() && !GameMode->IsChangingClassForNextRound())
										? EVisibility::HitTestInvisible : EVisibility::Collapsed;
								})
								.Text_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									const float Remaining = State && State->bNetworkClassSelectionActive
										? State->GetNetworkClassSelectionTimeRemaining()
										: GameMode.IsValid() ? GameMode->GetInitialClassSelectionTimeRemaining() : 0.0f;
									return FText::FromString(FString::Printf(
										TEXT("AUTO CONFIRM IN %02d"),
										FMath::CeilToInt(Remaining)));
								})
								.Font(UiFont(10, true))
								.ColorAndOpacity(Cyan)
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SBox).WidthOverride(280.0f)
								[
									MakeMenuButton(TEXT("CONFIRM CLASS"), FOnClicked::CreateLambda([this]() { if (PlayerController.IsValid()) PlayerController->RequestConfirmClass(); return FReply::Handled(); }), true, false, 56.0f, &ClassSelectDefaultButton)
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildArchetypePicker(const EFlickTeam Team)
{
	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(6.0f));
	for (int32 Index = 0; Index < FlickPieceArchetypeRules::ArchetypeCount; ++Index)
	{
		Grid->AddSlot(Index % 5, Index / 5)
		[
			BuildArchetypeChoice(Team, static_cast<EFlickPieceArchetype>(Index))
		];
	}

	return SNew(SFlickAngularBorder)
		.BackgroundColor(FLinearColor(0.003f, 0.01f, 0.018f, 0.96f))
		.AccentColor(Hairline)
		.CutSize(12.0f)
		.Padding(FMargin(12.0f, 9.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6.0f, 0.0f, 6.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("AVAILABLE PUCK TYPES"))).Font(UiFont(13, true)).ColorAndOpacity(Cyan)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team]() { return FText::FromString(FString::Printf(TEXT("PREVIEWING SLOT %d"), GetSelectedLoadoutSlot(Team) + 1)); })
					.Font(UiFont(9, true)).ColorAndOpacity(Muted)
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				Grid
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildArchetypeChoice(
	const EFlickTeam Team,
	const EFlickPieceArchetype Archetype)
{
	const FFlickPieceArchetypeRules& Rules = FlickPieceArchetypeRules::Get(Archetype);
	const FFlickPieceDisplayStats Stats = FlickPieceArchetypeRules::GetDisplayStats(Archetype);
	const FLinearColor Accent = FlickPieceArchetypeRules::GetVisualAccent(Archetype, GetTeamAccent(Team));
	const FString ClassLabel = Rules.ClassLabel;
	const TArray<float> Values = {Stats.Speed, Stats.Weight, Stats.Impact, Stats.Control, Stats.Coast, Stats.Stability};
	const TArray<FString> Labels = {TEXT("SPD"), TEXT("WGT"), TEXT("IMP"), TEXT("CTL"), TEXT("CST"), TEXT("STB")};
	TSharedRef<SVerticalBox> StatRows = SNew(SVerticalBox);
	for (int32 StatIndex = 0; StatIndex < Values.Num(); ++StatIndex)
	{
		StatRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(27.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Labels[StatIndex])).Font(UiFont(6, true)).ColorAndOpacity(Muted)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(154.0f).HeightOverride(6.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.003f, 0.007f, 0.012f, 1.0f))]
					+ SOverlay::Slot().HAlign(HAlign_Left)
					[
						SNew(SBox).WidthOverride(154.0f * Values[StatIndex])
						[
							SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Accent)
						]
					]
				]
			]
		];
	}
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(0.0f)
		.OnHovered_Lambda([this, Team, Archetype]()
		{
			SetHoveredLoadoutArchetype(Team, Archetype);
		})
		.OnUnhovered_Lambda([this, Team, Archetype]()
		{
			ClearHoveredLoadoutArchetype(Team, Archetype);
		})
		.OnClicked_Lambda([this, Team, Archetype]()
		{
			if (GameMode.IsValid())
			{
				GameMode->SetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team), Archetype);
				GameMode->PlayMenuSound(false);
			}
			return FReply::Handled();
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, Team, Archetype, Accent]()
			{
				const bool bSelected = GameMode.IsValid() && GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team)) == Archetype;
				return bSelected ? FMath::Lerp(PanelRaised, Accent, 0.018f) : Panel;
			})
			.AccentColor_Lambda([this, Team, Archetype, Accent]()
			{
				const bool bSelected = GameMode.IsValid() && GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team)) == Archetype;
				return bSelected ? Accent : Hairline;
			})
			.CutSize(8.0f)
			.Padding(FMargin(12.0f, 8.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox).WidthOverride(34.0f).HeightOverride(34.0f)
						[
							SNew(SFlickPuckDisc).TeamColor(GetTeamAccent(Team)).AccentColor(Accent).Archetype(Archetype).RadiusScale(0.72f)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(7.0f, 0.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(GetPieceArchetypeName(Archetype))).Font(UiFont(11, true)).ColorAndOpacity(Accent)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(FText::FromString(ClassLabel)).Font(UiFont(7, true)).ColorAndOpacity(Muted)
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, 4.0f, 1.0f, 5.0f)
				[
					SNew(STextBlock).Text(FText::FromString(Rules.Summary)).Font(UiFont(7)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					StatRows
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team, Archetype]()
					{
						return FText::FromString(GameMode.IsValid() && GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team)) == Archetype ? TEXT("EQUIPPED") : TEXT("SELECT"));
					})
					.Font(UiFont(7, true)).ColorAndOpacity(Accent)
				]
			]
		];
	const TWeakPtr<SButton> WeakButton = Button;
	return SNew(SBox)
		.HeightOverride(154.0f)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor_Lambda([this, Team, Archetype, Accent, WeakButton]()
			{
				const TSharedPtr<SButton> Pinned = WeakButton.Pin();
				if (Pinned.IsValid() && (Pinned->HasKeyboardFocus() || Pinned->IsHovered())) return FLinearColor::White;
				const bool bSelected = GameMode.IsValid()
					&& GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team)) == Archetype;
				return bSelected ? Accent : Hairline;
			})
			.Padding(1.0f)
			[
				Button
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildSettings()
{
	const auto Checked = [](const bool bValue) { return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };
	const auto SectionHeading = [](const FString& Number, const FString& Title, const FString& Description) -> TSharedRef<SWidget>
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[SNew(STextBlock).Text(FText::FromString(Number)).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[SNew(STextBlock).Text(FText::FromString(Title)).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
			[SNew(STextBlock).Text(FText::FromString(Description)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)];
	};
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Ink.CopyWithNewOpacity(0.88f))]
		+ SOverlay::Slot()[SNew(SFlickInterfaceBackdrop).Visibility(EVisibility::HitTestInvisible).Opacity(0.35f)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(1320.0f).HeightOverride(820.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Panel).Padding(FMargin(36.0f, 24.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 20.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK  /  YOUR SETUP"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("MAKE IT YOURS."))).Font(DisplayFont(46)).ColorAndOpacity(Paper)]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(16.0f, 0.0f, 0.0f, 8.0f)
						[SNew(STextBlock).Text(FText::FromString(TEXT("SETTINGS"))).Font(UiFont(12, true)).ColorAndOpacity(Muted)]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 10.0f, 0.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT("01"), TEXT("GAME FEEL"), TEXT("Keep the feedback that helps you read the board."))]
										+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("AIM AND CONTACT GUIDE"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { return Checked(GameMode.IsValid() && GameMode->IsAimGuideEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetAimGuideEnabled(!GameMode->IsAimGuideEnabled()); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("WORLD IMPACT EFFECTS"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { return Checked(GameMode.IsValid() && GameMode->AreImpactEffectsEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetImpactEffectsEnabled(!GameMode->AreImpactEffectsEnabled()); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("CONTROL OVERVIEW"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { return Checked(GameMode.IsValid() && GameMode->IsControlOverviewEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetControlOverviewEnabled(!GameMode->IsControlOverviewEnabled()); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("CAMERA SHAKE"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetCameraShakeIntensity() : 0.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetCameraShakeIntensity(Value); }))]
									]
								]

							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(10.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[
									SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT("02"), TEXT("SOUND"), TEXT("Set the balance of the arena and the interface."))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("MASTER"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetMasterVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetMasterVolume(Value); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("PHYSICS EFFECTS"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetEffectsVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetEffectsVolume(Value); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("INTERFACE"), TAttribute<float>::CreateLambda([this]() { return GameMode.IsValid() ? GameMode->GetInterfaceVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetInterfaceVolume(Value); }))]
									]
								]
								+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 0.0f)
								[
									SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(24.0f, 18.0f))
									[
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT("03"), TEXT("DISPLAY"), TEXT("Choose your screen setup, then apply below."))]
										+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("V-SYNC"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { return Checked(GameMode.IsValid() && GameMode->IsVSyncEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->ToggleVSync(); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("WINDOW MODE"), TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetWindowModeLabel() : TEXT("WINDOWED")); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleWindowMode(-1); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleWindowMode(1); return FReply::Handled(); }))]
										+ SVerticalBox::Slot().AutoHeight()[MakeCycleRow(TEXT("RESOLUTION"), TAttribute<FText>::CreateLambda([this]() { return FText::FromString(GameMode.IsValid() ? GameMode->GetResolutionLabel() : TEXT("1920 x 1080")); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleResolution(-1); return FReply::Handled(); }), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CycleResolution(1); return FReply::Handled(); }))]
									]
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 18.0f, 0.0f, 18.0f)
					[SNew(SBox).HeightOverride(1.0f)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline.CopyWithNewOpacity(0.55f))]]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(150.0f)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->CloseSettings(); return FReply::Handled(); }), false, false, 52.0f)]]
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(24.0f, 0.0f)
						[SNew(STextBlock).Text(FText::FromString(TEXT("Gameplay and audio update immediately."))).Font(UiFont(11)).ColorAndOpacity(Muted)]
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(280.0f)[MakeMenuButton(TEXT("APPLY DISPLAY"), FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ApplyDisplaySettings(); return FReply::Handled(); }), true, false, 52.0f, &SettingsDefaultButton)]]
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildMatchHud()
{
	return SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.0f, 16.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsFreePlayTraining() ? EVisibility::Collapsed : EVisibility::Visible; })
			[BuildTeamPlate(EFlickTeam::Player1)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 16.0f, 24.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsFreePlayTraining() ? EVisibility::Collapsed : EVisibility::Visible; })
			[BuildTeamPlate(EFlickTeam::Player2)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24.0f, 16.0f, 0.0f, 0.0f)[BuildTrainingToolsPanel()]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.0f, 16.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(560.0f)
			.HeightOverride(102.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel)
				.AccentColor(Brand.CopyWithNewOpacity(0.72f))
				.CutSize(0.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(18.0f, 6.0f, 18.0f, 6.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(100.0f).HeightOverride(1.0f)
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)]
					]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(100.0f).HeightOverride(1.0f)
						[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)]
					]
					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const AFlickGameState* State = GetScoreboardGameState();
								return FText::FromString(GameMode.IsValid() && GameMode->IsTrainingBotMatch()
									? State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
										? TEXT("BOB BOT")
										: TEXT("BOT MATCH")
									: GameMode.IsValid() && GameMode->IsTrainingMode()
										? TEXT("TRAINING")
										: State && State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("BOB") : TEXT("FLICK"));
							})
							.Font(UiFont(11, true))
							.ColorAndOpacity(Brand)
						]
						+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 5.0f, 0.0f, 5.0f)
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFit)
							.StretchDirection(EStretchDirection::DownOnly)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return GetMatchStatusText(); })
								.Font(DisplayFont(20))
								.ColorAndOpacity_Lambda([this]() { return GetMatchStatusColor(); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 2.0f)
						[
							SNew(STextBlock)
							.Visibility_Lambda([this]() { return GetNextTurnVisibility(); })
							.Text_Lambda([this]() { return GetNextTurnText(); })
							.Font(UiFont(10, true))
							.ColorAndOpacity_Lambda([this]() { return GetNextTurnColor(); })
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const AFlickGameState* State = GetScoreboardGameState();
								if (!State) return FText::GetEmpty();
								if (GameMode.IsValid() && GameMode->IsTrainingMode())
								{
									if (GameMode->IsTrainingBotMatch())
									{
										const bool bBobBot = State->ActiveMatchVariant == EFlickMatchVariant::Bob;
										if (State->bShotClockActive)
										{
											if (bBobBot)
											{
												return FText::FromString(FString::Printf(
													TEXT("BOB TRAINING   /   TURN %d   /   SHOOT IN %02d"),
													State->TurnNumber,
													FMath::CeilToInt(State->GetShotClockTimeRemaining())));
											}
										return FText::FromString(FString::Printf(
											TEXT("%dV%d TRAINING   /   ROUND %d   /   SHOT %d   /   SHOOT IN %02d"),
											State->PlayersPerTeam,
											State->PlayersPerTeam,
											State->RoundNumber,
												State->TurnNumber,
												FMath::CeilToInt(State->GetShotClockTimeRemaining())));
										}
										if (bBobBot)
										{
											return FText::FromString(FString::Printf(
												TEXT("BOB TRAINING   /   TURN %d"),
												State->TurnNumber));
										}
									return FText::FromString(FString::Printf(
										TEXT("%dV%d TRAINING   /   ROUND %d   /   SHOT %d"),
										State->PlayersPerTeam,
										State->PlayersPerTeam,
										State->RoundNumber,
											State->TurnNumber));
									}
									if (GameMode->IsTrainingEditMode())
									{
										if (GameMode->IsBobMode())
										{
											return FText::FromString(TEXT("BOARD EDITOR   /   STANDARD PUCKS ONLY   /   T SAVE & DONE"));
										}
										return FText::FromString(FString::Printf(
											TEXT("BOARD EDITOR   /   %s   /   WHEEL PUCK TYPE   /   T SAVE & DONE"),
											*GetPieceArchetypeName(GameMode->GetTrainingPlacementArchetype())));
									}
									return FText::FromString(FString::Printf(
										TEXT("FREE PLAY   /   SHOT %d   /   T EDIT BOARD   /   R RESET"),
										State->TurnNumber));
								}
								if (State->bShotClockActive)
								{
									return FText::FromString(State->ActiveMatchVariant == EFlickMatchVariant::Bob
										? FString::Printf(TEXT("STANDARD PUCKS   /   SHOT %d   /   SHOOT IN %02d"), State->TurnNumber, FMath::CeilToInt(State->GetShotClockTimeRemaining()))
										: FString::Printf(TEXT("ROUND %d   /   SHOT %d   /   SHOOT IN %02d"), State->RoundNumber, State->TurnNumber, FMath::CeilToInt(State->GetShotClockTimeRemaining())));
								}
								return FText::FromString(State->ActiveMatchVariant == EFlickMatchVariant::Bob
									? FString::Printf(TEXT("STANDARD PUCKS   /   SHOT %d"), State->TurnNumber)
									: FString::Printf(TEXT("ROUND %d   /   SHOT %d"), State->RoundNumber, State->TurnNumber));
							})
							.Font(UiFont(10, true))
							.ColorAndOpacity(Muted)
						]
					]
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24.0f, 0.0f, 0.0f, 36.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				BuildPowerMeter()
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildControlHintPanel(false)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 24.0f, 36.0f)[BuildControlHintPanel(true)]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 0.0f, 36.0f)[BuildCameraOrbitHint()]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 112.0f, 24.0f, 0.0f)[BuildEventFeed()];
}

TSharedRef<SWidget> SFlickGameLayer::BuildCinematicReplayOverlay()
{
	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Top)
		[
			SNew(SBox).HeightOverride(78.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 0.97f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom)
		[
			SNew(SBox).HeightOverride(78.0f)
			[
				SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 0.97f))
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30.0f, 23.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(42.0f).HeightOverride(3.0f)
				[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Orange)]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("CINEMATIC REPLAY"))).Font(UiFont(12, true)).ColorAndOpacity(Paper)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0.0f, 24.0f, 30.0f, 0.0f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("ROUND-WINNING SHOT  //  TEST ARENA"))).Font(UiFont(10, true)).ColorAndOpacity(Muted)
		]
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(30.0f, 0.0f, 30.0f, 31.0f)
		[
			SNew(SBox).HeightOverride(3.0f)
			[
				SNew(SProgressBar)
				.Style(&ShotClockBarStyle)
				.Percent_Lambda([this]()
				{
					return TOptional<float>(GameMode.IsValid() ? GameMode->GetCinematicReplayProgress() : 0.0f);
				})
				.FillColorAndOpacity(Orange)
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 30.0f, 46.0f)
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("SLOW MOTION  //  PLAYBACK"))).Font(UiFont(9, true)).ColorAndOpacity(Orange)
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildScoreboardOverlay()
{
	const auto MakeColumnHeading = [](const FString& Label, const float Width) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(Width)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(UiFont(10, true))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor(0.56f, 0.68f, 0.76f, 1.0f))
			];
	};

	return SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush())
			.BorderBackgroundColor(FLinearColor(0.0f, 0.003f, 0.008f, 0.73f))
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(36.0f)
		[
			SNew(SBox)
			.WidthOverride(1080.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.001f, 0.009f, 0.017f, 0.985f))
				.AccentColor(Cyan.CopyWithNewOpacity(0.82f))
				.CutSize(18.0f)
				.BorderWidth(1.2f)
				.UseAccentForOutline(true)
				.Padding(FMargin(28.0f, 22.0f, 28.0f, 20.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 15.0f, 0.0f)
						[
							SNew(SBox).WidthOverride(4.0f).HeightOverride(47.0f)
							[
								SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return FText::FromString(GameMode.IsValid() && GameMode->IsTrainingBotMatch()
										? State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
											? TEXT("BOB BOT SCOREBOARD")
											: TEXT("BOT MATCH SCOREBOARD")
										: GameMode.IsValid() && GameMode->IsTrainingMode()
											? TEXT("TRAINING STATS")
											: TEXT("MATCH SCOREBOARD"));
								})
								.Font(UiFont(27, true))
								.ColorAndOpacity(FLinearColor::White)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, -2.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() { return GetScoreboardMatchSummary(); })
								.Font(UiFont(10, true))
								.ColorAndOpacity(Cyan)
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("HOLD  TAB")))
							.Font(UiFont(12, true))
							.ColorAndOpacity(FLinearColor(0.7f, 0.78f, 0.84f, 1.0f))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 17.0f, 0.0f, 6.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(16.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("PLAYER")))
							.Font(UiFont(10, true))
							.ColorAndOpacity(FLinearColor(0.56f, 0.68f, 0.76f, 1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth()[MakeColumnHeading(TEXT("SCORE"), 126.0f)]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(144.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
										? TEXT("POCKETS")
										: TEXT("KNOCKOUTS"));
								})
								.Font(UiFont(10, true))
								.Justification(ETextJustify::Center)
								.ColorAndOpacity(FLinearColor(0.56f, 0.68f, 0.76f, 1.0f))
							]
						]
						+ SHorizontalBox::Slot().AutoWidth()[MakeColumnHeading(TEXT("SHOTS"), 112.0f)]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(112.0f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const AFlickGameState* State = GetScoreboardGameState();
									return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
										? TEXT("IMPACTS")
										: TEXT("SURVIVORS"));
								})
								.Font(UiFont(10, true))
								.Justification(ETextJustify::Center)
								.ColorAndOpacity(FLinearColor(0.56f, 0.68f, 0.76f, 1.0f))
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight()[BuildScoreboardTeamSection(EFlickTeam::Player1)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 13.0f, 0.0f, 0.0f)[BuildScoreboardTeamSection(EFlickTeam::Player2)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 15.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
								? TEXT("SCORE  =  SHOTS  10   +   IMPACTS  5   +   POCKETS  100")
								: TEXT("SCORE  =  SHOTS  10   +   IMPACTS  5   +   KNOCKOUTS  100   +   SURVIVING PUCKS  50"));
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity(FLinearColor(0.42f, 0.53f, 0.61f, 1.0f))
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildScoreboardTeamSection(const EFlickTeam Team)
{
	const FLinearColor Accent = GetTeamAccent(Team);
	TSharedRef<SVerticalBox> PlayerRows = SNew(SVerticalBox);
	for (int32 PlayerSlot = 0; PlayerSlot < 3; ++PlayerSlot)
	{
		PlayerRows->AddSlot().AutoHeight().Padding(0.0f, PlayerSlot == 0 ? 5.0f : 4.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.Visibility_Lambda([this, PlayerSlot]() { return GetScoreboardPlayerVisibility(PlayerSlot); })
			[
				BuildScoreboardPlayerRow(Team, PlayerSlot)
			]
		];
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(40.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Accent.CopyWithNewOpacity(0.14f))
				.AccentColor(Accent.CopyWithNewOpacity(0.95f))
				.CutSize(7.0f)
				.BorderWidth(1.0f)
				.UseAccentForOutline(true)
				.Padding(FMargin(16.0f, 0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Team == EFlickTeam::Player1 ? TEXT("BLUE TEAM") : TEXT("ORANGE TEAM")))
						.Font(UiFont(14, true))
						.ColorAndOpacity(Accent)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]() { return GetScoreboardTeamSummary(Team); })
						.Font(UiFont(11, true))
						.ColorAndOpacity(FLinearColor(0.82f, 0.88f, 0.92f, 1.0f))
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[PlayerRows];
}

TSharedRef<SWidget> SFlickGameLayer::BuildScoreboardPlayerRow(
	const EFlickTeam Team,
	const int32 PlayerSlot)
{
	const FLinearColor Accent = GetTeamAccent(Team);
	const auto MakeStatCell = [this, Team, PlayerSlot](const int32 StatIndex, const float Width) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(Width)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, Team, PlayerSlot, StatIndex]()
				{
					return GetScoreboardStatText(Team, PlayerSlot, StatIndex);
				})
				.Font(UiFont(17, true))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(StatIndex == 0
					? FLinearColor::White
					: FLinearColor(0.78f, 0.85f, 0.9f, 1.0f))
			];
	};

	return SNew(SBox).HeightOverride(51.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([this, Team, PlayerSlot, Accent]()
			{
				const AFlickGameState* State = GetScoreboardGameState();
				const bool bActive = State
					&& State->CurrentTeam == Team
					&& State->CurrentTeamPlayerSlot == PlayerSlot
					&& State->MatchPhase != EFlickMatchPhase::RoundOver;
				return bActive
					? Accent.CopyWithNewOpacity(0.2f)
					: FLinearColor(0.01f, 0.025f, 0.039f, 0.96f);
			})
			.AccentColor(Accent.CopyWithNewOpacity(0.54f))
			.CutSize(6.0f)
			.BorderWidth(0.8f)
			.Padding(FMargin(16.0f, 0.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(5.0f).HeightOverride(27.0f)
					[
						SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Accent)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([this, Team, PlayerSlot]() { return GetScoreboardPlayerName(Team, PlayerSlot); })
					.Font(UiFont(15, true))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(0, 126.0f)]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(1, 144.0f)]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(2, 112.0f)]
				+ SHorizontalBox::Slot().AutoWidth()[MakeStatCell(3, 112.0f)]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTeamPlate(const EFlickTeam Team)
{
	const auto IsTeamToPlay = [this, Team]()
	{
		const AFlickGameState* State = GetScoreboardGameState();
		return State && State->CurrentTeam == Team
			&& (State->MatchPhase == EFlickMatchPhase::Aiming
				|| State->MatchPhase == EFlickMatchPhase::KickoffPlanning);
	};
	TSharedRef<SHorizontalBox> SeriesPips = SNew(SHorizontalBox);
	for (int32 PipIndex = 0; PipIndex < 3; ++PipIndex)
	{
		SeriesPips->AddSlot().AutoWidth().Padding(0.0f, 0.0f, PipIndex == 2 ? 0.0f : 6.0f, 0.0f)
				[
					SNew(SFlickRoundPip)
					.Color_Lambda([this, Team]() { return GetTeamAccent(Team); })
					.Filled_Lambda([this, Team, PipIndex]()
					{
						const AFlickGameState* State = GetScoreboardGameState();
						const int32 Rounds = State ? (Team == EFlickTeam::Player1 ? State->Player1RoundsWon : State->Player2RoundsWon) : 0;
						return PipIndex < Rounds;
					})
				];
	}

	return SNew(SBox)
		.WidthOverride(290.0f)
		.HeightOverride(84.0f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([IsTeamToPlay]() { return IsTeamToPlay() ? PanelRaised : Panel; })
			.AccentColor_Lambda([this, Team, IsTeamToPlay]() { return GetTeamAccent(Team).CopyWithNewOpacity(IsTeamToPlay() ? 1.0f : 0.28f); })
			.CutSize(0.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(14.0f, 6.0f, 14.0f, 8.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
							{
								return FText::FromString(Team == EFlickTeam::Player1 ? TEXT("YOU") : TEXT("BOT"));
							}
							if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
							{
								return FText::FromString(Team == EFlickTeam::Player1 ? TEXT("YOUR PUCKS") : TEXT("TARGETS"));
							}
							return FText::FromString(State && State->PlayersPerTeam > 1
								? FString::Printf(TEXT("TEAM %d"), GetTeamNumber(Team))
								: FString::Printf(TEXT("PLAYER %d"), GetTeamNumber(Team)));
						})
						.Font(UiFont(11, true))
						.ColorAndOpacity(GetTeamAccent(Team))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 1.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([IsTeamToPlay]() { return FText::FromString(IsTeamToPlay() ? TEXT("TO PLAY") : TEXT("")); })
						.Font(UiFont(8, true))
						.ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							return (GameMode.IsValid() && GameMode->IsFreePlayTraining())
								|| (State && State->ActiveMatchVariant == EFlickMatchVariant::Bob)
								? EVisibility::Collapsed : EVisibility::Visible;
						})
						[SeriesPips]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f)[SNew(SSpacer)]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]() { return GetRoundsText(Team); })
						.Font(UiFont(9, true))
						.ColorAndOpacity(Muted)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(12.0f, -6.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Team]() { return GetPieceCountText(Team); })
						.Font(DisplayFont(32))
						.ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, -5.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							const AFlickGameState* State = GetScoreboardGameState();
							return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob ? TEXT("LEFT TO POCKET") : TEXT("PUCKS IN PLAY"));
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity(Muted)
					]
				]
			]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
			[
				SNew(SBox).WidthOverride(4.0f)
				[
					SNew(SBorder).BorderImage(WhiteBrush())
					.BorderBackgroundColor_Lambda([this, Team, IsTeamToPlay]()
					{
						return GetTeamAccent(Team).CopyWithNewOpacity(IsTeamToPlay() ? 1.0f : 0.35f);
					})
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Bottom).Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.HeightOverride(3.0f)
				.Visibility_Lambda([this]()
				{
					const AFlickGameState* State = PlayerController.IsValid()
						? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
						: nullptr;
					return State && State->bShotClockActive
						? EVisibility::HitTestInvisible
						: EVisibility::Collapsed;
				})
				[
					SNew(SProgressBar)
					.Style(&ShotClockBarStyle)
					.BarFillType(EProgressBarFillType::LeftToRight)
					.BorderPadding(FVector2D::ZeroVector)
					.Percent_Lambda([this, Team]()
					{
						const AFlickGameState* State = PlayerController.IsValid()
							? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
							: nullptr;
						return TOptional<float>(State ? State->GetShotClockFraction(Team) : 1.0f);
					})
					.FillColorAndOpacity_Lambda([this, Team]()
					{
						const FLinearColor Accent = GetTeamAccent(Team);
						const AFlickGameState* State = PlayerController.IsValid()
							? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
							: nullptr;
						const bool bClockActive = State
							&& State->CurrentTeam == Team
							&& (State->MatchPhase == EFlickMatchPhase::Aiming
								|| State->MatchPhase == EFlickMatchPhase::KickoffPlanning);
						if (!bClockActive)
						{
							return FSlateColor(Accent.CopyWithNewOpacity(0.2f));
						}
						const float Fraction = State ? State->GetShotClockFraction(Team) : 1.0f;
						if (bClockActive && Fraction <= 1.0f / 3.0f)
						{
							return FSlateColor(Fraction <= 0.12f
								? FLinearColor(1.0f, 0.08f, 0.015f, 1.0f)
								: FLinearColor(1.0f, 0.34f, 0.12f, 1.0f));
						}
						return FSlateColor(Accent);
					})
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildTrainingToolsPanel()
{
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsFreePlayTraining()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		.WidthOverride(320.0f)
		.HeightOverride(128.0f)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.01f, 0.018f, 0.95f))
			.AccentColor(Cyan.CopyWithNewOpacity(0.82f))
			.CutSize(14.0f)
			.BorderWidth(1.1f)
			.UseAccentForOutline(true)
			.Padding(FMargin(20.0f, 14.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("TRAINING TOOLS")))
					.Font(UiFont(14, true))
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (!GameMode.IsValid() || !GameMode->IsTrainingEditMode())
						{
							return FText::FromString(TEXT("SHOT MODE  /  T EDIT BOARD"));
						}
						return FText::FromString(FString::Printf(
							TEXT("%s  /  %s"),
							GameMode->GetTrainingPlacementTeam() == EFlickTeam::Player1 ? TEXT("YOUR PUCK") : TEXT("TARGET PUCK"),
							GameMode->IsBobMode()
								? TEXT("STANDARD ONLY")
								: *GetPieceArchetypeName(GameMode->GetTrainingPlacementArchetype())));
					})
					.Font(UiFont(11, true))
					.ColorAndOpacity_Lambda([this]()
					{
						if (!GameMode.IsValid() || !GameMode->IsTrainingEditMode())
						{
							return FSlateColor(FLinearColor(0.2f, 0.78f, 0.5f, 1.0f));
						}
						return FSlateColor(GetTeamAccent(GameMode->GetTrainingPlacementTeam()));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						if (GameMode.IsValid() && GameMode->IsTrainingEditMode())
						{
							return FText::FromString(GameMode->IsBobMode()
								? TEXT("1 OWN  2 TARGET  |  LMB PLACE/DRAG  |  DEL REMOVE NON-STRIKERS")
								: TEXT("WHEEL TYPE  |  1 OWN  2 TARGET  |  LMB PLACE/DRAG  |  DEL REMOVE"));
						}
						return FText::FromString(TEXT("Aim and shoot normally   |   R restores your saved setup"));
					})
					.Font(UiFont(8, true))
					.AutoWrapText(true)
					.WrapTextAt(272.0f)
					.ColorAndOpacity(FLinearColor(0.68f, 0.76f, 0.82f, 1.0f))
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildControlHintPanel(const bool bRightSide)
{
	const auto MakeHintRow = [bRightSide](const bool bTraining, const bool bEditing) -> TSharedRef<SHorizontalBox>
	{
		TSharedRef<SHorizontalBox> HintRow = SNew(SHorizontalBox);
		const FLinearColor Accent = bRightSide ? Orange : Cyan;
		const auto AddHint = [&HintRow, Accent](const FString& Key, const FString& Label, const bool bLast)
		{
			HintRow->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
				.AccentColor(Accent.CopyWithNewOpacity(0.72f))
				.CutSize(4.0f)
				.BorderWidth(0.8f)
				.Padding(FMargin(7.0f, 3.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(Key))
					.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
				]
			];
			HintRow->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, bLast ? 0.0f : 14.0f, 0.0f)
			[
				SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
			];
		};

		if (bRightSide)
		{
			if (bTraining && bEditing)
			{
				AddHint(TEXT("T"), TEXT("SAVE / DONE"), false);
				AddHint(TEXT("C"), TEXT("CLEAR"), false);
				AddHint(TEXT("R"), TEXT("RESET"), true);
			}
			else if (bTraining)
			{
				AddHint(TEXT("T"), TEXT("EDIT BOARD"), false);
				AddHint(TEXT("R"), TEXT("RESET"), false);
				AddHint(TEXT("ESC"), TEXT("PAUSE"), true);
			}
			else
			{
				AddHint(TEXT("TAB"), TEXT("SCOREBOARD"), false);
				AddHint(TEXT("R"), TEXT("RESTART"), false);
				AddHint(TEXT("ESC"), TEXT("PAUSE"), true);
			}
		}
		else if (bTraining && bEditing)
		{
			AddHint(TEXT("LMB"), TEXT("PLACE / DRAG"), false);
			AddHint(TEXT("DEL"), TEXT("REMOVE"), false);
			AddHint(TEXT("1 / 2"), TEXT("OWN / TARGET"), true);
		}
		else if (bTraining)
		{
			AddHint(TEXT("LMB"), TEXT("AIM"), false);
			AddHint(TEXT("DRAG"), TEXT("POWER"), false);
			AddHint(TEXT("RELEASE"), TEXT("SHOOT"), true);
		}
		else
		{
			AddHint(TEXT("LMB"), TEXT("AIM"), false);
			AddHint(TEXT("DRAG"), TEXT("POWER"), false);
			AddHint(TEXT("RELEASE"), TEXT("SHOOT"), true);
		}
		return HintRow;
	};

	TSharedRef<SWidget> StandardHints = SNew(SBox)
		.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsFreePlayTraining() ? EVisibility::Collapsed : EVisibility::Visible; })
		[MakeHintRow(false, false)];
	TSharedRef<SWidget> TrainingShotHints = SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsFreePlayTraining() && !GameMode->IsTrainingEditMode()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[MakeHintRow(true, false)];
	TSharedRef<SWidget> TrainingEditHints = SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid() && GameMode->IsTrainingEditMode()
				? EVisibility::Visible
				: EVisibility::Collapsed;
		})
		[MakeHintRow(true, true)];

	return SNew(SBox)
		// Matching outer widths keep the centered camera panel optically and
		// mathematically equidistant from both bottom-corner panels.
		.WidthOverride(470.0f)
		.HeightOverride(66.0f)
		.Visibility_Lambda([this]()
		{
			return !GameMode.IsValid() || GameMode->IsControlOverviewEnabled()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.017f, 0.94f))
			.AccentColor((bRightSide ? Orange : Cyan).CopyWithNewOpacity(0.7f))
			.CutSize(13.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(15.0f, 7.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this, bRightSide]()
					{
						if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
						{
							if (GameMode->IsTrainingEditMode())
							{
								return FText::FromString(bRightSide ? TEXT("BOARD TOOLS") : TEXT("BOARD EDITOR"));
							}
							return FText::FromString(bRightSide ? TEXT("TRAINING") : TEXT("SHOT CONTROL"));
						}
						return FText::FromString(bRightSide ? TEXT("MATCH") : TEXT("SHOT CONTROL"));
					})
					.Font(UiFont(7, true))
					.ColorAndOpacity((bRightSide ? Orange : Cyan).CopyWithNewOpacity(0.9f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[StandardHints]
					+ SOverlay::Slot()[TrainingShotHints]
					+ SOverlay::Slot()[TrainingEditHints]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPowerMeter()
{
	TSharedRef<SHorizontalBox> PowerSegments = SNew(SHorizontalBox);
	for (int32 SegmentIndex = 0; SegmentIndex < 10; ++SegmentIndex)
	{
		PowerSegments->AddSlot().FillWidth(1.0f).Padding(
			0.0f,
			0.0f,
			SegmentIndex == 9 ? 0.0f : 3.0f,
			0.0f)
		[
			SNew(SBox).HeightOverride(14.0f)
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor_Lambda([this, SegmentIndex]()
				{
					const float Power = PlayerController.IsValid()
						? PlayerController->GetAimResult().NormalizedPower
						: 0.0f;
					return Power >= static_cast<float>(SegmentIndex + 1) / 10.0f
						? GetPowerColor().GetSpecifiedColor()
						: FLinearColor(0.065f, 0.068f, 0.06f, 1.0f);
				})
			]
		];
	}

	return SNew(SBox)
		.WidthOverride(320.0f)
		.Visibility_Lambda([this]() { return GetPowerVisibility(); })
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(Panel)
			.AccentColor(Brand)
			.CutSize(0.0f)
			.Padding(FMargin(18.0f, 14.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("SHOT POWER"))).Font(UiFont(10, true)).ColorAndOpacity(Paper)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock).Text_Lambda([this]() { return GetPowerText(); }).Font(DisplayFont(28)).ColorAndOpacity_Lambda([this]() { return GetPowerColor(); })
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 9.0f, 0.0f, 0.0f)
				[
					PowerSegments
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("RELEASE TO FLICK")))
					.Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("RMB / ESC  CANCEL")))
						.Font(UiFont(8, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildCameraOrbitHint()
{
	return SNew(SBox)
		.WidthOverride(610.0f)
		.HeightOverride(66.0f)
		.Visibility_Lambda([this]()
		{
			return GameMode.IsValid()
				&& GameMode->IsControlOverviewEnabled()
				&& GameMode->CanChangeCameraView()
				? EVisibility::HitTestInvisible
				: EVisibility::Collapsed;
		})
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor(FLinearColor(0.002f, 0.009f, 0.017f, 0.94f))
			.AccentColor(Cyan.CopyWithNewOpacity(0.7f))
			.CutSize(11.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(15.0f, 7.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return FText::FromString(GameMode.IsValid() && GameMode->IsTrainingEditMode()
							? TEXT("EDIT VIEW  /  VERTICAL ANGLE LOCKED")
							: TEXT("CAMERA"));
					})
					.Font(UiFont(7, true))
					.ColorAndOpacity(Cyan.CopyWithNewOpacity(0.9f))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
						.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
						.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(8.0f, 3.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Q / E")))
							.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("ORBIT"))).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return GameMode.IsValid() && GameMode->IsTrainingEditMode() && GameMode->IsBobMode()
								? EVisibility::Collapsed
								: EVisibility::Visible;
						})
						[
							SNew(SFlickAngularBorder)
							.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
							.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
							.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(8.0f, 3.0f))
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("WHEEL")))
								.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7.0f, 0.0f, 14.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (GameMode.IsValid() && GameMode->IsTrainingEditMode())
							{
								if (GameMode->IsBobMode())
								{
									return FText::FromString(TEXT("STANDARD PUCKS ONLY"));
								}
								return FText::FromString(FString::Printf(
									TEXT("PUCK TYPE  /  %s"),
									*GetPieceArchetypeName(GameMode->GetTrainingPlacementArchetype())));
							}
							return FText::FromString(TEXT("VERTICAL ANGLE"));
						})
						.Font(UiFont(9, true))
						.ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SFlickAngularBorder)
						.BackgroundColor(FLinearColor(0.014f, 0.03f, 0.045f, 0.98f))
						.AccentColor(Cyan.CopyWithNewOpacity(0.72f))
						.CutSize(4.0f).BorderWidth(0.8f).Padding(FMargin(8.0f, 3.0f))
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("F")))
							.Font(UiFont(9, true)).ColorAndOpacity(FLinearColor::White)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(7.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("RESET"))).Font(UiFont(9, true)).ColorAndOpacity(FLinearColor(0.78f, 0.84f, 0.89f, 1.0f))
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildEventFeed()
{
	TSharedRef<SVerticalBox> Feed = SNew(SVerticalBox);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Feed->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			SNew(SBox)
			.WidthOverride(320.0f)
			.Visibility_Lambda([this, Index]() { return GetEventVisibility(Index); })
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush())
				.BorderBackgroundColor(Panel.CopyWithNewOpacity(0.95f))
				.Padding(FMargin(0.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(3.0f)
						[
							SNew(SBorder).BorderImage(WhiteBrush())
							.BorderBackgroundColor_Lambda([this, Index]() { return GetEventColor(Index).GetSpecifiedColor(); })
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(13.0f, 12.0f, 0.0f, 12.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Index]() { return GetEventText(Index); })
						.Font(UiFont(10, true))
						.AutoWrapText(true)
						.ColorAndOpacity(Paper)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.0f)
					[
						SNew(STextBlock)
						.Visibility_Lambda([this, Index]() { return GetEventPointsVisibility(Index); })
						.Text_Lambda([this, Index]() { return GetEventPointsText(Index); })
						.Font(UiFont(14, true))
						.ColorAndOpacity(Brand)
					]
				]
			]
		];
	}
	return Feed;
}

TSharedRef<SWidget> SFlickGameLayer::BuildPauseOverlay()
{
	auto MakePauseButton = [this](
		const FString& Label,
		const bool bDanger,
		const FOnClicked& OnClicked,
		TSharedPtr<SButton>* OutButton = nullptr) -> TSharedRef<SWidget>
	{
		const bool bPrimary = Label == TEXT("RESUME");
		TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(&TransparentButtonStyle)
			.ContentPadding(0.0f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Cursor(EMouseCursor::Hand)
			.OnClicked(OnClicked);

		if (OutButton)
		{
			*OutButton = Button;
		}

		const TWeakPtr<SButton> WeakButton = Button;
		Button->SetContent(
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([WeakButton, bDanger, bPrimary]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				const bool bActive = PinnedButton.IsValid()
					&& (PinnedButton->IsHovered() || PinnedButton->HasKeyboardFocus());
				if (bDanger)
				{
					return bActive
						? FLinearColor(0.15f, 0.041f, 0.022f, 1.0f)
						: PanelRaised;
				}
				if (bPrimary) return bActive ? Paper : Brand;
				return bActive ? FLinearColor(0.085f, 0.09f, 0.078f, 1.0f) : PanelRaised;
			})
			.AccentColor_Lambda([WeakButton, bDanger, bPrimary]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				const bool bActive = PinnedButton.IsValid()
					&& (PinnedButton->IsHovered() || PinnedButton->HasKeyboardFocus());
				if (bDanger)
				{
					return Orange.CopyWithNewOpacity(bActive ? 1.0f : 0.38f);
				}
				return bPrimary || bActive ? Brand : Hairline;
			})
			.CutSize(5.0f)
			.BorderWidth(1.0f)
			.UseAccentForOutline(true)
			.Padding(FMargin(2.0f))
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(0.0f, 0.0f, 20.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(17, true)).ColorAndOpacity(bPrimary ? Ink : bDanger ? Orange : Muted)
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(22.0f, 0.0f, 42.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([this, Label]()
						{
							if (Label == TEXT("CHANGE CLASS")
								&& GameMode.IsValid()
								&& GameMode->HasPendingClassChanges())
							{
								return FText::FromString(TEXT("CHANGE CLASS  (QUEUED)"));
							}
							return FText::FromString(Label == TEXT("FORFEIT")
								&& GameMode.IsValid()
								&& GameMode->IsTrainingMode()
									? TEXT("EXIT TRAINING")
									: Label);
						})
						.Font(UiFont(15, true))
						.ColorAndOpacity(bPrimary ? Ink : bDanger ? Orange : Paper)
					]
				]
			]
		);

		return SNew(SBox).HeightOverride(bPrimary ? 64.0f : 54.0f)[Button];
	};

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.003f, 0.004f, 0.003f, 0.8f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(500.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel.CopyWithNewOpacity(0.985f))
				.AccentColor(Brand)
				.CutSize(10.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(32.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("FLICK / IN SESSION"))).Font(UiFont(10, true)).ColorAndOpacity(Brand)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 24.0f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("MATCH MENU"))).Font(DisplayFont(34)).ColorAndOpacity(Paper)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						MakePauseButton(TEXT("RESUME"), false, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->TogglePauseMenu(); return FReply::Handled(); }), &PauseDefaultButton)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
						{
							return GameMode.IsValid() && GameMode->CanOpenClassChange()
								? EVisibility::Visible
								: EVisibility::Collapsed;
						})
						[
							MakePauseButton(TEXT("CHANGE CLASS"), false, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->OpenClassChange(); return FReply::Handled(); }))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						MakePauseButton(TEXT("SETTINGS"), false, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->OpenSettings(); return FReply::Handled(); }))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						MakePauseButton(TEXT("FORFEIT"), true, FOnClicked::CreateLambda([this]() { if (GameMode.IsValid()) GameMode->ReturnToMainMenu(); return FReply::Handled(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("ESC   RETURN TO GAME")))
						.Font(UiFont(9, true)).ColorAndOpacity(Muted)
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::BuildRoundOverOverlay()
{
	FSlateFontInfo RoundHeaderFont = UiFont(11, true);
	RoundHeaderFont.LetterSpacing = 120;
	FSlateFontInfo RoundScoreFont = UiFont(18, true);
	RoundScoreFont.LetterSpacing = 40;
	FSlateFontInfo RoundButtonFont = UiFont(17, true);
	RoundButtonFont.LetterSpacing = 30;

	TSharedRef<SButton> NextActionButton = SAssignNew(RoundOverDefaultButton, SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(FMargin(0.0f))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this]()
		{
			const AFlickGameState* State = PlayerController.IsValid()
				? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
				: nullptr;
			if (State && State->bSeriesComplete && State->bMatchmakingLobby)
			{
				if (GameMode.IsValid()) GameMode->ReturnToMainMenu();
				else if (PlayerController.IsValid()) PlayerController->LeaveNetworkSession();
			}
			else if (PlayerController.IsValid())
			{
				if (State && State->bSeriesComplete) PlayerController->RequestRestartMatch();
				else PlayerController->RequestNextRound();
			}
			return FReply::Handled();
		});
	const TWeakPtr<SButton> WeakNextActionButton = NextActionButton;
	NextActionButton->SetContent(
		SNew(SFlickAngularBorder)
		.BackgroundColor_Lambda([WeakNextActionButton]()
		{
			const TSharedPtr<SButton> PinnedButton = WeakNextActionButton.Pin();
			return PinnedButton.IsValid()
				&& (PinnedButton->IsHovered() || PinnedButton->HasKeyboardFocus())
				? Paper : Brand;
		})
		.AccentColor(Brand)
		.CutSize(5.0f)
		.BorderWidth(1.0f)
		.UseAccentForOutline(true)
		.Padding(FMargin(22.0f, 0.0f))
		[
			SNew(SBox)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const AFlickGameState* State = PlayerController.IsValid()
						? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
						: nullptr;
					if (State && State->bSeriesComplete)
					{
						return FText::FromString(State->bMatchmakingLobby ? TEXT("RETURN TO PARTY") : TEXT("REMATCH"));
					}
					return FText::FromString(FString::Printf(
						TEXT("NEXT ROUND  /  %02d"),
						State ? FMath::CeilToInt(State->GetRoundAdvanceTimeRemaining()) : 0));
				})
				.Font(RoundButtonFont)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(Ink)
			]
		]
	);

	TSharedRef<SVerticalBox> ResultLayout = SNew(SVerticalBox);
	ResultLayout->AddSlot().AutoHeight()
	[
		SNew(SBox)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				const AFlickGameState* State = GetScoreboardGameState();
				if (State && State->bSeriesComplete) return FText::FromString(TEXT("MATCH COMPLETE"));
				return FText::FromString(State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
					? TEXT("BOB COMPLETE") : TEXT("ROUND COMPLETE"));
			})
			.Font(RoundHeaderFont)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(Brand)
		]
	];
	ResultLayout->AddSlot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 22.0f)
	[
		SNew(SBox).HeightOverride(76.0f)
		[
			SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly).HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return GetRoundResultText(); })
				.Font(DisplayFont(44))
				.Justification(ETextJustify::Center)
				.ColorAndOpacity_Lambda([this]() { return GetMatchStatusColor(); })
			]
		]
	];
	ResultLayout->AddSlot().AutoHeight()
	[
		SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(FMargin(18.0f, 16.0f))
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return GetRoundScoreText(); })
			.Font(RoundScoreFont)
			.AutoWrapText(true)
			.Justification(ETextJustify::Center)
			.ColorAndOpacity(Paper)
		]
	];
	ResultLayout->AddSlot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
	[
		SNew(STextBlock)
		.Visibility_Lambda([this]()
		{
			const AFlickGameState* State = PlayerController.IsValid()
				? PlayerController->GetWorld()->GetGameState<AFlickGameState>() : nullptr;
			return State && State->bSeriesComplete && State->bRankedMatch
				? EVisibility::Visible : EVisibility::Collapsed;
		})
		.Text_Lambda([this]()
		{
			const AFlickGameState* State = PlayerController.IsValid()
				? PlayerController->GetWorld()->GetGameState<AFlickGameState>() : nullptr;
			const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
			if (!State || !Ranking || !Ranking->HasRatingUpdateForMatch(State->MatchId))
			{
				return FText::FromString(TEXT("RANKED RESULT SYNCING..."));
			}
			const FFlickRatingUpdate& Update = Ranking->GetLastRatingUpdate();
			const FFlickRankProgress Progress = Ranking->GetProgress(Update.Variant, Update.PlayersPerTeam);
			return FText::FromString(FString::Printf(
				TEXT("RATING  %s%d  /  %s%s"),
				Update.RatingDelta >= 0 ? TEXT("+") : TEXT(""),
				Update.RatingDelta,
				*FlickRankRules::GetProgressLabel(Progress),
				Update.bForfeit ? TEXT("  /  FORFEIT") : TEXT("")));
		})
		.Font(UiFont(11, true))
		.AutoWrapText(true)
		.Justification(ETextJustify::Center)
		.ColorAndOpacity_Lambda([this]()
		{
			const UFlickRankingSubsystem* Ranking = GetRankingSubsystem();
			return FSlateColor(Ranking && Ranking->GetLastRatingUpdate().RatingDelta < 0 ? Orange : Brand);
		})
	];
	ResultLayout->AddSlot().AutoHeight().Padding(0.0f, 22.0f, 0.0f, 0.0f)
	[
		SNew(SBox).HeightOverride(62.0f)[NextActionButton]
	];

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f))]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(640.0f)
			[
				SNew(SFlickAngularBorder)
				.BackgroundColor(Panel.CopyWithNewOpacity(0.985f))
				.AccentColor(Brand)
				.CutSize(10.0f)
				.BorderWidth(1.0f)
				.Padding(FMargin(40.0f, 32.0f))
				[
					ResultLayout
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeMainMenuButton(
	const FString& Label,
	const FOnClicked& OnClicked,
	const bool bPrimary,
	const bool bDanger,
	const float Height,
	TSharedPtr<SButton>* OutButton)
{
	EFlickMainMenuIcon Icon = EFlickMainMenuIcon::Play;
	if (Label == TEXT("STATS")) Icon = EFlickMainMenuIcon::Stats;
	else if (Label.Contains(TEXT("LEADERBOARD"))) Icon = EFlickMainMenuIcon::Leaderboard;
	else if (Label.Contains(TEXT("HISTORY"))) Icon = EFlickMainMenuIcon::History;
	else if (Label == TEXT("BACK")) Icon = EFlickMainMenuIcon::Back;
	else if (Label.Contains(TEXT("LINEUP"))) Icon = EFlickMainMenuIcon::Lineups;
	else if (Label.Contains(TEXT("PROFILE"))) Icon = EFlickMainMenuIcon::Profile;
	else if (Label.Contains(TEXT("SHOP"))) Icon = EFlickMainMenuIcon::Shop;
	else if (Label.Contains(TEXT("SETTINGS"))) Icon = EFlickMainMenuIcon::Settings;
	else if (Label.Contains(TEXT("QUIT"))) Icon = EFlickMainMenuIcon::Quit;

	const FString Detail = Label == TEXT("PLAY") ? TEXT("Find your next rivalry")
		: Label == TEXT("LINEUPS") ? TEXT("Build your advantage")
		: Label == TEXT("PROFILE") ? TEXT("Your record. Your progress.")
		: Label == TEXT("ITEM SHOP") ? TEXT("Explore the collection")
		: Label == TEXT("SETTINGS") ? TEXT("Make yourself comfortable") : FString();
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(22.0f, 0.0f, 24.0f, 0.0f))
		.HAlign(HAlign_Fill).VAlign(VAlign_Center).OnClicked(OnClicked);
	const TWeakPtr<SButton> WeakButton = Button;
	Button->SetContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 18.0f, 0.0f)
		[SNew(SFlickMainMenuIcon).Icon(Icon).Color(bPrimary ? Ink : bDanger ? Muted : Brand)]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(bPrimary ? 30 : 21)).ColorAndOpacity(bPrimary ? Ink : Paper)]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Text(FText::FromString(Detail)).Font(UiFont(10)).ColorAndOpacity(bPrimary ? Ink : Muted)
				.Visibility(Detail.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[SNew(STextBlock).Text(FText::FromString(TEXT("\u2197"))).Font(UiFont(20)).ColorAndOpacity(bPrimary ? Ink : Brand)]);
	if (OutButton) *OutButton = Button;
	return SNew(SBox).HeightOverride(Height)
	[
		SNew(SFlickMainMenuFrame)
		.StartColor(bPrimary ? Brand : Panel).EndColor(bPrimary ? Brand : Panel)
		.HoverStartColor(bPrimary ? FMath::Lerp(Brand, Paper, 0.16f) : PanelRaised).HoverEndColor(bPrimary ? FMath::Lerp(Brand, Paper, 0.16f) : PanelRaised)
		.BorderColor(bPrimary ? Brand : Hairline).HoverBorderColor(bDanger ? Orange : Brand)
		.Highlighted_Lambda([WeakButton]()
		{
			const TSharedPtr<SButton> Pinned = WeakButton.Pin();
			return Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus());
		})
		.BorderWidth(1.0f).Padding(FMargin(1.0f))[Button]
	];
}

TSharedRef<SWidget> SFlickGameLayer::MakeMenuButton(
	const FString& Label,
	const FOnClicked& OnClicked,
	const bool bPrimary,
	const bool bDanger,
	const float Height,
	TSharedPtr<SButton>* OutButton)
{
	FString Icon = TEXT(">");
	if (Label.Contains(TEXT("BACK"))) Icon = TEXT("<");
	else if (Label.Contains(TEXT("LINEUP"))) Icon = TEXT("III");
	else if (Label.Contains(TEXT("SHOP"))) Icon = TEXT("$");
	else if (Label.Contains(TEXT("SETTINGS"))) Icon = TEXT("*");
	else if (Label.Contains(TEXT("QUIT")) || Label.Contains(TEXT("MAIN MENU"))) Icon = TEXT("X");
	else if (Label.Contains(TEXT("RESTART")) || Label.Contains(TEXT("REMATCH"))) Icon = TEXT("R");
	else if (Label.Contains(TEXT("REFRESH"))) Icon = TEXT("↻");
	else if (Label.Contains(TEXT("APPLY")) || Label.Contains(TEXT("SAVE"))) Icon = TEXT("+");
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(&TransparentButtonStyle)
		.ContentPadding(FMargin(12.0f, 6.0f))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.OnClicked(OnClicked)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(0.0f, 2.0f, 10.0f, 2.0f)
			[
				SNew(SBox).WidthOverride(bPrimary ? 5.0f : 3.0f)
				[
					SNew(SBorder)
					.BorderImage(WhiteBrush())
					.BorderBackgroundColor(bPrimary ? Ink : bDanger ? Orange : Brand)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 10.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(24.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Icon))
					.Font(UiFont(11, true))
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(bDanger ? Orange : bPrimary ? Ink : Brand)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.Font(UiFont(15, true))
				.ColorAndOpacity(bPrimary ? Ink : Paper)
			]
		];
	if (OutButton)
	{
		*OutButton = Button;
	}
	const TWeakPtr<SButton> WeakButton = Button;
	return SNew(SBox)
		.HeightOverride(Height)
		[
			SNew(SFlickAngularBorder)
			.BackgroundColor_Lambda([WeakButton, bPrimary, bDanger]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				const bool bActive = PinnedButton.IsValid()
					&& (PinnedButton->HasKeyboardFocus() || PinnedButton->IsHovered());
				if (bDanger)
				{
					return bActive
						? FLinearColor(0.15f, 0.027f, 0.012f, 0.94f)
						: FLinearColor(0.026f, 0.009f, 0.008f, 0.72f);
				}
				if (bPrimary)
				{
					return bActive
						? FMath::Lerp(Brand, Paper, 0.16f) : Brand;
				}
				return bActive
					? PanelRaised : Panel;
			})
			.AccentColor_Lambda([WeakButton, bPrimary, bDanger]()
			{
				const TSharedPtr<SButton> PinnedButton = WeakButton.Pin();
				if (PinnedButton.IsValid() && (PinnedButton->HasKeyboardFocus() || PinnedButton->IsHovered()))
				{
					return Brand;
				}
				return bPrimary ? Brand : bDanger ? Orange : Hairline;
			})
			.CutSize(8.0f)
			.BorderWidth(1.0f)
			.Padding(FMargin(1.0f))
			[
				Button
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeSectionLabel(const FString& Label) const
{
	return SNew(SFlickAngularBorder)
		.BackgroundColor(PanelRaised)
		.AccentColor(Brand)
		.CutSize(7.0f)
		.Padding(FMargin(14.0f, 8.0f))
		[
			SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(12, true)).ColorAndOpacity(Brand)
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeToggleRow(
	const FString& Label,
	const TAttribute<ECheckBoxState>& State,
	const FOnCheckStateChanged& OnChanged) const
{
	return SNew(SBox).HeightOverride(58.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(74.0f).HeightOverride(32.0f)
				[
					SNew(SCheckBox)
					.Style(&ToggleStyle)
					.IsChecked(State)
					.OnCheckStateChanged(OnChanged)
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([State]() { return FText::FromString(State.Get() == ECheckBoxState::Checked ? TEXT("ON") : TEXT("OFF")); })
						.Font(UiFont(10, true)).ColorAndOpacity_Lambda([State]() { return State.Get() == ECheckBoxState::Checked ? Ink : Paper; })
					]
				]
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeSliderRow(
	const FString& Label,
	const TAttribute<float>& Value,
	const FOnFloatValueChanged& OnChanged) const
{
	return SNew(SBox).HeightOverride(58.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.36f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
			+ SHorizontalBox::Slot().FillWidth(0.54f).VAlign(VAlign_Center)
			[
				SNew(SOverlay)
				+ SOverlay::Slot().VAlign(VAlign_Center)
				[
					SNew(SBox).HeightOverride(6.0f)
					[
						SNew(SProgressBar)
						.Style(&ShotClockBarStyle)
						.Percent_Lambda([Value]() { return TOptional<float>(Value.Get()); })
						.FillColorAndOpacity(Brand)
					]
				]
				+ SOverlay::Slot()
				[
					SNew(SSlider).Style(&SliderStyle).Value(Value).OnValueChanged(OnChanged)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.1f).VAlign(VAlign_Center).HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text_Lambda([Value]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value.Get() * 100.0f))); })
				.Font(UiFont(11, true)).ColorAndOpacity(Brand)
			]
		];
}

TSharedRef<SWidget> SFlickGameLayer::MakeCycleRow(
	const FString& Label,
	const TAttribute<FText>& Value,
	const FOnClicked& Previous,
	const FOnClicked& Next) const
{
	return SNew(SBox).HeightOverride(58.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(13, true)).ColorAndOpacity(FLinearColor::White)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(36.0f).HeightOverride(34.0f)[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(0.0f).OnClicked(Previous)[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(UiFont(13, true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)]]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(154.0f)[SNew(STextBlock).Text(Value).Font(UiFont(12, true)).Justification(ETextJustify::Center).ColorAndOpacity(Brand)]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(36.0f).HeightOverride(34.0f)[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(0.0f).OnClicked(Next)[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(13, true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)]]]
		];
}

EVisibility SFlickGameLayer::GetScreenVisibility(const EFlickFrontendScreen Screen) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (State && State->bNetworkClassSelectionActive)
	{
		return Screen == EFlickFrontendScreen::ClassSelect
			? EVisibility::Visible : EVisibility::Collapsed;
	}
	if (!GameMode.IsValid())
	{
		return EVisibility::Collapsed;
	}
	const EFlickFrontendScreen CurrentScreen = GameMode->GetFrontendScreen();
	if (CurrentScreen == Screen)
	{
		return EVisibility::Visible;
	}
	if (CurrentScreen == EFlickFrontendScreen::Settings
		&& GameMode->GetSettingsReturnScreen() == Screen
		&& Screen != EFlickFrontendScreen::Paused)
	{
		return EVisibility::HitTestInvisible;
	}
	return EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetMatchHudVisibility() const
{
	if (GameMode.IsValid())
	{
		if (GameMode->IsCinematicReplayActive())
		{
			return EVisibility::Collapsed;
		}
		const EFlickFrontendScreen Screen = GameMode->GetFrontendScreen();
		return Screen == EFlickFrontendScreen::Playing || Screen == EFlickFrontendScreen::Paused
			? EVisibility::SelfHitTestInvisible
			: EVisibility::Collapsed;
	}
	const AFlickGameState* State = GetScoreboardGameState();
	return State && State->IsGameplayActive()
		&& !State->bNetworkLobbyActive
		&& !State->bPrivateMatchLobbyActive
		? EVisibility::SelfHitTestInvisible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetScoreboardVisibility() const
{
	if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
	{
		return EVisibility::Collapsed;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickScoreboardPreview")))
	{
		return EVisibility::HitTestInvisible;
	}
	if (!PlayerController.IsValid() || !PlayerController->IsScoreboardVisible())
	{
		return EVisibility::Collapsed;
	}
	if (GameMode.IsValid() && GameMode->GetFrontendScreen() != EFlickFrontendScreen::Playing)
	{
		return EVisibility::Collapsed;
	}
	const AFlickGameState* State = GetScoreboardGameState();
	return State && State->IsGameplayActive()
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetScoreboardPlayerVisibility(const int32 PlayerSlot) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const int32 PlayersPerTeam = State ? State->PlayersPerTeam : 1;
	return PlayerSlot >= 0 && PlayerSlot < PlayersPerTeam
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetRoundOverVisibility() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return EVisibility::Visible;
	const AFlickGameState* State = GetScoreboardGameState();
	const bool bPlayingScreen = !GameMode.IsValid()
		|| GameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing;
	return bPlayingScreen && State && State->MatchPhase == EFlickMatchPhase::RoundOver
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetPowerVisibility() const
{
	return PlayerController.IsValid() && PlayerController->IsAimingShot() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetLoadoutRowVisibility(const int32 SlotIndex) const
{
	return GameMode.IsValid() && SlotIndex < GameMode->GetLoadoutEditingPieceCount() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFlickGameLayer::GetEventVisibility(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid() || !OwnerHud->GetWorld()) return EVisibility::Collapsed;
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex) && Events[EventIndex].ExpiresAt > OwnerHud->GetWorld()->GetTimeSeconds()
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

FText SFlickGameLayer::GetEventText(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return FText::GetEmpty();
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex) ? FText::FromString(Events[EventIndex].Message) : FText::GetEmpty();
}

FSlateColor SFlickGameLayer::GetEventColor(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return FSlateColor(FLinearColor::White);
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return FSlateColor(Events.IsValidIndex(EventIndex) ? Events[EventIndex].Color : FLinearColor::White);
}

EVisibility SFlickGameLayer::GetEventPointsVisibility(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return EVisibility::Collapsed;
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex) && !Events[EventIndex].PointsText.IsEmpty()
		? EVisibility::HitTestInvisible
		: EVisibility::Collapsed;
}

FText SFlickGameLayer::GetEventPointsText(const int32 IndexFromNewest) const
{
	if (!OwnerHud.IsValid()) return FText::GetEmpty();
	const TArray<FFlickHudEventMessage>& Events = OwnerHud->GetEventMessages();
	const int32 EventIndex = Events.Num() - 1 - IndexFromNewest;
	return Events.IsValidIndex(EventIndex)
		? FText::FromString(Events[EventIndex].PointsText)
		: FText::GetEmpty();
}

FText SFlickGameLayer::GetScoreboardPlayerName(
	const EFlickTeam Team,
	const int32 PlayerSlot) const
{
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		if (Team == EFlickTeam::Player2)
		{
			return FText::FromString(TEXT("TRAINING BOT"));
		}
		if (Team == EFlickTeam::Player1)
		{
			return FText::FromString(TEXT("YOU"));
		}
	}
	if (const AFlickGameState* State = GetScoreboardGameState())
	{
		for (const APlayerState* BasePlayerState : State->PlayerArray)
		{
			const AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(BasePlayerState);
			if (FlickPlayerState
				&& ((FlickPlayerState->GetTeam() == Team
						&& FlickPlayerState->GetTeamPlayerSlot() == PlayerSlot)
					|| FlickPlayerState->ControlsPrivateSlot(Team, PlayerSlot))
				&& !FlickPlayerState->GetPlayerName().IsEmpty())
			{
				return FText::FromString(FlickPlayerState->GetPlayerName());
			}
		}
	}

	return FText::FromString(Team == EFlickTeam::Player1
		? FString::Printf(TEXT("BLUE PLAYER %d"), PlayerSlot + 1)
		: FString::Printf(TEXT("ORANGE PLAYER %d"), PlayerSlot + 1));
}

FText SFlickGameLayer::GetScoreboardStatText(
	const EFlickTeam Team,
	const int32 PlayerSlot,
	const int32 StatIndex) const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickScoreboardPreview")))
	{
		static const int32 PreviewStats[2][3][4] =
		{
			{{245, 1, 6, 17}, {135, 1, 4, 5}, {65, 0, 3, 7}},
			{{220, 1, 5, 14}, {100, 0, 4, 12}, {55, 0, 3, 5}}
		};
		const int32 TeamIndex = Team == EFlickTeam::Player2 ? 1 : 0;
		return FText::AsNumber(PreviewStats[TeamIndex][FMath::Clamp(PlayerSlot, 0, 2)][FMath::Clamp(StatIndex, 0, 3)]);
	}

	const AFlickGameState* State = GetScoreboardGameState();
	const FFlickPlayerMatchStats* Stats = State
		? State->FindPlayerMatchStats(Team, PlayerSlot)
		: nullptr;
	if (!Stats)
	{
		return FText::AsNumber(0);
	}
	const int32 Value = StatIndex == 0
		? Stats->Score
		: StatIndex == 1
			? Stats->Knockouts
			: StatIndex == 2
				? Stats->Shots
				: State && State->ActiveMatchVariant == EFlickMatchVariant::Bob
					? Stats->Impacts
					: Stats->SurvivingPucks;
	return FText::AsNumber(Value);
}

FText SFlickGameLayer::GetScoreboardTeamSummary(const EFlickTeam Team) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State)
	{
		return FText::GetEmpty();
	}
	if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
	{
		const int32 ActivePieces = Team == EFlickTeam::Player1
			? State->Player1ActivePieces
			: State->Player2ActivePieces;
		return FText::FromString(FString::Printf(TEXT("PUCKS ACTIVE  %d"), ActivePieces));
	}
	if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		const int32 ActivePieces = Team == EFlickTeam::Player1
			? State->Player1ActivePieces
			: State->Player2ActivePieces;
		return FText::FromString(FString::Printf(TEXT("PUCKS LEFT  %d"), ActivePieces));
	}
	const int32 RoundWins = Team == EFlickTeam::Player1
		? State->Player1RoundsWon
		: State->Player2RoundsWon;
	return FText::FromString(FString::Printf(
		TEXT("ROUND WINS  %d   |   TEAM SCORE  %d"),
		RoundWins,
		State->GetTeamScore(Team)));
}

FText SFlickGameLayer::GetScoreboardMatchSummary() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State)
	{
		return FText::FromString(TEXT("LIVE MATCH"));
	}
	const FString Mode = State->ActiveMatchVariant == EFlickMatchVariant::Bob
		? TEXT("BOB")
		: FString::Printf(TEXT("%dV%d FLICK"), State->PlayersPerTeam, State->PlayersPerTeam);
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		return FText::FromString(FString::Printf(TEXT("%s   /   TRAINING BOT   /   ROUND %d"), *Mode, State->RoundNumber));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingMode())
	{
		return FText::FromString(FString::Printf(TEXT("%s   /   OFFLINE FREE PLAY"), *Mode));
	}
	return FText::FromString(FString::Printf(TEXT("%s   /   ROUND %d"), *Mode, State->RoundNumber));
}

const AFlickGameState* SFlickGameLayer::GetScoreboardGameState() const
{
	if (GameMode.IsValid())
	{
		return GameMode->GetFlickGameState();
	}
	return PlayerController.IsValid() && PlayerController->GetWorld()
		? PlayerController->GetWorld()->GetGameState<AFlickGameState>()
		: nullptr;
}

FText SFlickGameLayer::GetMatchStatusText() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FText::GetEmpty();
	if (GameMode.IsValid() && GameMode->IsCinematicReplayActive())
	{
		return FText::FromString(TEXT("ROUND-WINNING SHOT"));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		if (State->MatchPhase == EFlickMatchPhase::RoundOver)
		{
			return GetRoundResultText();
		}
		if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
		{
			return FText::FromString(GameMode->IsResolvingKickoff()
				? TEXT("KICKOFF IN MOTION")
				: TEXT("PUCKS IN MOTION"));
		}
		if (State->PlayersPerTeam > 1)
		{
			const TCHAR* TeamName = State->CurrentTeam == EFlickTeam::Player2
				? TEXT("ORANGE") : TEXT("BLUE");
			const int32 PlayerNumber = State->CurrentTeamPlayerSlot + 1;
			if (State->CurrentTeam == EFlickTeam::Player2)
			{
				return FText::FromString(State->MatchPhase == EFlickMatchPhase::KickoffPlanning
					? FString::Printf(TEXT("%s P%d  /  BOT SETTING KICKOFF"), TeamName, PlayerNumber)
					: FString::Printf(TEXT("%s P%d  /  BOT THINKING"), TeamName, PlayerNumber));
			}
			return FText::FromString(State->MatchPhase == EFlickMatchPhase::KickoffPlanning
				? FString::Printf(TEXT("%s P%d  /  SET YOUR KICKOFF"), TeamName, PlayerNumber)
				: FString::Printf(TEXT("%s P%d  /  YOUR TURN"), TeamName, PlayerNumber));
		}
		if (State->CurrentTeam == EFlickTeam::Player2)
		{
			return FText::FromString(TEXT("BOT THINKING"));
		}
		return FText::FromString(State->MatchPhase == EFlickMatchPhase::KickoffPlanning
			? TEXT("SET YOUR KICKOFF")
			: TEXT("YOUR TURN"));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingMode())
	{
		if (GameMode->IsTrainingEditMode())
		{
			return FText::FromString(GameMode->GetTrainingPlacementTeam() == EFlickTeam::Player1
				? TEXT("PLACE YOUR PUCKS")
				: TEXT("PLACE TARGET PUCKS"));
		}
		return FText::FromString(State->MatchPhase == EFlickMatchPhase::ResolvingPhysics
			? TEXT("PUCKS IN MOTION")
			: TEXT("SELECT ANY BLUE PUCK"));
	}
	if (State->MatchPhase == EFlickMatchPhase::KickoffPlanning)
	{
		return FText::FromString(State->PlayersPerTeam > 1
			? FString::Printf(
				TEXT("%s P%d SET KICKOFF  /  %d OF %d LOCKED"),
				State->CurrentTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
				State->CurrentTeamPlayerSlot + 1,
				State->KickoffShotsLocked,
				State->KickoffShotsRequired)
			: FString::Printf(TEXT("PLAYER %d SET KICKOFF"), GetTeamNumber(State->CurrentTeam)));
	}
	if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
	{
		if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
		{
			return FText::FromString(TEXT("TABLE IN MOTION"));
		}
		return FText::FromString(GameMode.IsValid() && GameMode->IsResolvingKickoff()
			? TEXT("KICKOFF IN MOTION")
			: TEXT("PUCKS IN MOTION"));
	}
	if (State->MatchPhase == EFlickMatchPhase::RoundOver) return GetRoundResultText();
	if (State->PlayersPerTeam > 1)
	{
		return FText::FromString(State->ActiveMatchVariant == EFlickMatchVariant::Bob
			? FString::Printf(
				TEXT("%s P%d STRIKER"),
				State->CurrentTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
				State->CurrentTeamPlayerSlot + 1)
			: FString::Printf(
				TEXT("%s P%d  /  TURN"),
				State->CurrentTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE"),
				State->CurrentTeamPlayerSlot + 1));
	}
	return FText::FromString(State->ActiveMatchVariant == EFlickMatchVariant::Bob
		? FString::Printf(TEXT("PLAYER %d STRIKER"), GetTeamNumber(State->CurrentTeam))
		: FString::Printf(TEXT("PLAYER %d TURN"), GetTeamNumber(State->CurrentTeam)));
}

FSlateColor SFlickGameLayer::GetMatchStatusColor() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return FSlateColor(Orange);
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FSlateColor(FLinearColor::White);
	if (State->bShotClockActive
		&& State->GetShotClockFraction(State->CurrentTeam) <= 1.0f / 3.0f)
	{
		return FSlateColor(FLinearColor(1.0f, 0.34f, 0.12f, 1.0f));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics)
		{
			return FSlateColor(FLinearColor(1.0f, 0.72f, 0.12f, 1.0f));
		}
		return FSlateColor(GetTeamAccent(
			State->MatchPhase == EFlickMatchPhase::RoundOver ? State->WinnerTeam : State->CurrentTeam));
	}
	if (GameMode.IsValid() && GameMode->IsTrainingMode())
	{
		if (GameMode->IsTrainingEditMode())
		{
			return FSlateColor(GetTeamAccent(GameMode->GetTrainingPlacementTeam()));
		}
		return FSlateColor(State->MatchPhase == EFlickMatchPhase::ResolvingPhysics
			? FLinearColor(1.0f, 0.72f, 0.12f, 1.0f)
			: FLinearColor(0.2f, 0.78f, 0.5f, 1.0f));
	}
	if (State->MatchPhase == EFlickMatchPhase::KickoffPlanning) return FSlateColor(GetTeamAccent(State->CurrentTeam));
	if (State->MatchPhase == EFlickMatchPhase::ResolvingPhysics) return FSlateColor(FLinearColor(1.0f, 0.72f, 0.12f, 1.0f));
	return FSlateColor(GetTeamAccent(State->MatchPhase == EFlickMatchPhase::RoundOver ? State->WinnerTeam : State->CurrentTeam));
}

FText SFlickGameLayer::GetNextTurnText() const
{
	EFlickTeam NextTeam = EFlickTeam::None;
	int32 NextPlayerSlot = INDEX_NONE;
	if (!GameMode.IsValid() || !GameMode->GetNextScheduledTurn(NextTeam, NextPlayerSlot))
	{
		return FText::GetEmpty();
	}
	return FText::FromString(FString::Printf(
		TEXT("NEXT TURN  P%d  /  %s"),
		NextPlayerSlot + 1,
		NextTeam == EFlickTeam::Player2 ? TEXT("ORANGE") : TEXT("BLUE")));
}

FSlateColor SFlickGameLayer::GetNextTurnColor() const
{
	EFlickTeam NextTeam = EFlickTeam::None;
	int32 NextPlayerSlot = INDEX_NONE;
	return FSlateColor(GameMode.IsValid()
		&& GameMode->GetNextScheduledTurn(NextTeam, NextPlayerSlot)
		? GetTeamAccent(NextTeam)
		: FLinearColor::Transparent);
}

EVisibility SFlickGameLayer::GetNextTurnVisibility() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	EFlickTeam NextTeam = EFlickTeam::None;
	int32 NextPlayerSlot = INDEX_NONE;
	return State
		&& State->PlayersPerTeam > 1
		&& (!GameMode.IsValid() || !GameMode->IsCinematicReplayActive())
		&& GameMode.IsValid()
		&& GameMode->GetNextScheduledTurn(NextTeam, NextPlayerSlot)
		? EVisibility::Visible
		: EVisibility::Collapsed;
}

FText SFlickGameLayer::GetRoundResultText() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return FText::FromString(TEXT("PLAYER 2 TAKES IT"));
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FText::GetEmpty();
	if (State->bDraw) return FText::FromString(State->bSeriesComplete ? TEXT("MATCH DRAW") : TEXT("ROUND DRAW"));
	if (GameMode.IsValid() && GameMode->IsTrainingBotMatch())
	{
		const bool bPlayerWon = State->WinnerTeam == EFlickTeam::Player1;
		return FText::FromString(State->bSeriesComplete
			? bPlayerWon ? TEXT("YOU WIN") : TEXT("BOT WINS")
			: bPlayerWon ? TEXT("YOU TAKE IT") : TEXT("BOT TAKES IT"));
	}
	if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		return FText::FromString(FString::Printf(TEXT("PLAYER %d CLEARS BOB"), GetTeamNumber(State->WinnerTeam)));
	}
	const TCHAR* WinnerLabel = State->PlayersPerTeam > 1 ? TEXT("TEAM") : TEXT("PLAYER");
	return FText::FromString(State->bSeriesComplete
		? FString::Printf(TEXT("%s %d WINS"), WinnerLabel, GetTeamNumber(State->WinnerTeam))
		: FString::Printf(TEXT("%s %d TAKES IT"), WinnerLabel, GetTeamNumber(State->WinnerTeam)));
}

FText SFlickGameLayer::GetRoundScoreText() const
{
	if (FParse::Param(FCommandLine::Get(), TEXT("FlickRoundOverPreview"))) return FText::FromString(TEXT("SERIES   0  -  1"));
	const AFlickGameState* State = GetScoreboardGameState();
	if (State && State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		return FText::FromString(FString::Printf(
			TEXT("POCKETED   %d  -  %d"),
			State->StartingPiecesPerTeam - State->Player1ActivePieces,
			State->StartingPiecesPerTeam - State->Player2ActivePieces));
	}
	if (!State)
	{
		return FText::GetEmpty();
	}
	if (State->bSeriesComplete && State->Player1RoundsWon == State->Player2RoundsWon)
	{
		return FText::FromString(FString::Printf(
			TEXT("SERIES   %d  -  %d    |    POINTS   %d  -  %d"),
			State->Player1RoundsWon,
			State->Player2RoundsWon,
			State->GetTeamScore(EFlickTeam::Player1),
			State->GetTeamScore(EFlickTeam::Player2)));
	}
	return FText::FromString(FString::Printf(TEXT("SERIES   %d  -  %d"), State->Player1RoundsWon, State->Player2RoundsWon));
}

FText SFlickGameLayer::GetPieceCountText(const EFlickTeam Team) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const int32 Count = !State ? 0 : Team == EFlickTeam::Player1 ? State->Player1ActivePieces : State->Player2ActivePieces;
	return FText::AsNumber(Count);
}

FText SFlickGameLayer::GetRoundsText(const EFlickTeam Team) const
{
	const AFlickGameState* State = GetScoreboardGameState();
	if (!State) return FText::GetEmpty();
	if (GameMode.IsValid() && GameMode->IsFreePlayTraining())
	{
		return FText::FromString(Team == EFlickTeam::Player1
			? TEXT("CONTROL ANY BLUE PUCK")
			: TEXT("CLEAR TO AUTO-RESET"));
	}
	if (State->ActiveMatchVariant == EFlickMatchVariant::Bob)
	{
		const int32 Remaining = Team == EFlickTeam::Player1 ? State->Player1ActivePieces : State->Player2ActivePieces;
		return FText::FromString(FString::Printf(
			TEXT("POCKETED  %d / %d"),
			State->StartingPiecesPerTeam - Remaining,
			State->StartingPiecesPerTeam));
	}
	const int32 Rounds = Team == EFlickTeam::Player1 ? State->Player1RoundsWon : State->Player2RoundsWon;
	return FText::FromString(FString::Printf(TEXT("ROUNDS  %d / %d"), Rounds, State->RoundsToWin));
}

FText SFlickGameLayer::GetLoadoutName(const EFlickTeam Team, const int32 SlotIndex) const
{
	return FText::FromString(GameMode.IsValid() ? GetPieceArchetypeName(GameMode->GetLoadoutPiece(Team, SlotIndex)) : TEXT("STANDARD"));
}

FText SFlickGameLayer::GetLoadoutSummary(const EFlickTeam Team, const int32 SlotIndex) const
{
	if (!GameMode.IsValid()) return FText::GetEmpty();
	return FText::FromString(FlickPieceArchetypeRules::Get(GameMode->GetLoadoutPiece(Team, SlotIndex)).Summary);
}

int32 SFlickGameLayer::GetSelectedLoadoutSlot(const EFlickTeam Team) const
{
	const int32 SelectedSlot = Team == EFlickTeam::Player2
		? Player2SelectedLoadoutSlot
		: Player1SelectedLoadoutSlot;
	const int32 ActiveSlots = GameMode.IsValid()
		? GameMode->GetLoadoutEditingPieceCount()
		: 4;
	return FMath::Clamp(SelectedSlot, 0, FMath::Max(ActiveSlots - 1, 0));
}

void SFlickGameLayer::SelectLoadoutSlot(const EFlickTeam Team, const int32 SlotIndex)
{
	const int32 ActiveSlots = GameMode.IsValid()
		? GameMode->GetLoadoutEditingPieceCount()
		: 4;
	const int32 SafeSlot = FMath::Clamp(SlotIndex, 0, FMath::Max(ActiveSlots - 1, 0));
	if (Team == EFlickTeam::Player2)
	{
		Player2SelectedLoadoutSlot = SafeSlot;
		Player2HoveredLoadoutArchetype.Reset();
	}
	else
	{
		Player1SelectedLoadoutSlot = SafeSlot;
		Player1HoveredLoadoutArchetype.Reset();
	}
	if (GameMode.IsValid())
	{
		GameMode->PlayMenuSound(false);
	}
}

EFlickPieceArchetype SFlickGameLayer::GetPreviewLoadoutArchetype(const EFlickTeam Team) const
{
	const TOptional<EFlickPieceArchetype>& Hovered = Team == EFlickTeam::Player2
		? Player2HoveredLoadoutArchetype
		: Player1HoveredLoadoutArchetype;
	if (Hovered.IsSet())
	{
		return Hovered.GetValue();
	}
	return GameMode.IsValid()
		? GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team))
		: EFlickPieceArchetype::Standard;
}

float SFlickGameLayer::GetLoadoutStatValue(
	const EFlickTeam Team,
	const int32 StatIndex,
	const bool bPreview) const
{
	const EFlickPieceArchetype Archetype = bPreview
		? GetPreviewLoadoutArchetype(Team)
		: GameMode.IsValid()
			? GameMode->GetLoadoutPiece(Team, GetSelectedLoadoutSlot(Team))
			: EFlickPieceArchetype::Standard;
	return GetDisplayStatValue(FlickPieceArchetypeRules::GetDisplayStats(Archetype), StatIndex);
}

TArray<float> SFlickGameLayer::GetLineupProfileStats(const EFlickTeam Team) const
{
	TArray<float> Profile;
	Profile.Init(0.0f, 6);
	const int32 PieceCount = GameMode.IsValid()
		? FMath::Clamp(GameMode->GetLoadoutEditingPieceCount(), 1, 4)
		: 4;
	const int32 SelectedSlot = GetSelectedLoadoutSlot(Team);
	for (int32 SlotIndex = 0; SlotIndex < PieceCount; ++SlotIndex)
	{
		const EFlickPieceArchetype Archetype = SlotIndex == SelectedSlot
			? GetPreviewLoadoutArchetype(Team)
			: GameMode.IsValid()
				? GameMode->GetLoadoutPiece(Team, SlotIndex)
				: EFlickPieceArchetype::Standard;
		const FFlickPieceDisplayStats Stats = FlickPieceArchetypeRules::GetDisplayStats(Archetype);
		for (int32 StatIndex = 0; StatIndex < Profile.Num(); ++StatIndex)
		{
			Profile[StatIndex] += GetDisplayStatValue(Stats, StatIndex) / static_cast<float>(PieceCount);
		}
	}
	return Profile;
}

void SFlickGameLayer::SetHoveredLoadoutArchetype(
	const EFlickTeam Team,
	const EFlickPieceArchetype Archetype)
{
	if (Team == EFlickTeam::Player2)
	{
		Player2HoveredLoadoutArchetype = Archetype;
	}
	else
	{
		Player1HoveredLoadoutArchetype = Archetype;
	}
}

void SFlickGameLayer::ClearHoveredLoadoutArchetype(
	const EFlickTeam Team,
	const EFlickPieceArchetype Archetype)
{
	TOptional<EFlickPieceArchetype>& Hovered = Team == EFlickTeam::Player2
		? Player2HoveredLoadoutArchetype
		: Player1HoveredLoadoutArchetype;
	if (Hovered.IsSet() && Hovered.GetValue() == Archetype)
	{
		Hovered.Reset();
	}
}

FText SFlickGameLayer::GetPowerText() const
{
	const float Power = PlayerController.IsValid() ? PlayerController->GetAimResult().NormalizedPower : 0.0f;
	return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Power * 100.0f)));
}

FSlateColor SFlickGameLayer::GetPowerColor() const
{
	const float Power = PlayerController.IsValid() ? PlayerController->GetAimResult().NormalizedPower : 0.0f;
	if (Power < 0.65f)
	{
		return FSlateColor(FLinearColor::LerpUsingHSV(FLinearColor(0.0f, 0.78f, 0.95f, 1.0f), FLinearColor(1.0f, 0.8f, 0.12f, 1.0f), Power / 0.65f));
	}
	return FSlateColor(FLinearColor::LerpUsingHSV(FLinearColor(1.0f, 0.8f, 0.12f, 1.0f), Orange, (Power - 0.65f) / 0.35f));
}

UFlickRankingSubsystem* SFlickGameLayer::GetRankingSubsystem() const
{
	const UGameInstance* GameInstance = PlayerController.IsValid()
		? PlayerController->GetGameInstance()
		: GameMode.IsValid() ? GameMode->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFlickRankingSubsystem>() : nullptr;
}

FLinearColor SFlickGameLayer::GetModeAccent(const EFlickMatchVariant Variant) const
{
	switch (Variant)
	{
	case EFlickMatchVariant::Bob: return FLinearColor(0.18f, 0.82f, 0.48f, 1.0f);
	case EFlickMatchVariant::Classic:
	default: return Cyan;
	}
}

FLinearColor SFlickGameLayer::GetCurrentModeAccent() const
{
	return GetModeAccent(GameMode.IsValid() ? GameMode->GetSelectedMatchVariant() : EFlickMatchVariant::Classic);
}

FLinearColor SFlickGameLayer::GetTeamAccent(const EFlickTeam Team) const
{
	return Team == EFlickTeam::Player2 ? Orange : Cyan;
}

EFlickTeam SFlickGameLayer::GetClassSelectionTeam() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const AFlickPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (State && State->bNetworkClassSelectionActive && LocalPlayerState)
	{
		return LocalPlayerState->GetTeam();
	}
	return EFlickTeam::Player1;
}

int32 SFlickGameLayer::GetClassSelectionPlayerSlot() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const AFlickPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (State && State->bNetworkClassSelectionActive && LocalPlayerState)
	{
		return FMath::Max(0, LocalPlayerState->GetTeamPlayerSlot());
	}
	return 0;
}

EFlickLineupPreset SFlickGameLayer::GetSelectedClassDraft() const
{
	const AFlickGameState* State = GetScoreboardGameState();
	const AFlickPlayerState* LocalPlayerState = PlayerController.IsValid()
		? PlayerController->GetPlayerState<AFlickPlayerState>()
		: nullptr;
	if (State && State->bNetworkClassSelectionActive && LocalPlayerState)
	{
		return LocalPlayerState->GetNetworkSelectedClass();
	}
	return GameMode.IsValid()
		? GameMode->GetPlayerClass(GetClassSelectionTeam(), GetClassSelectionPlayerSlot(), true)
		: EFlickLineupPreset::Balanced;
}

EFlickPieceArchetype SFlickGameLayer::GetSelectedClassPiece(const int32 PieceSlot) const
{
	return GameMode.IsValid()
		? GameMode->GetClassLoadoutPiece(GetSelectedClassDraft(), PieceSlot)
		: FlickPieceArchetypeRules::GetPreset(GetSelectedClassDraft()).IsValidIndex(PieceSlot)
			? FlickPieceArchetypeRules::GetPreset(GetSelectedClassDraft())[PieceSlot]
			: EFlickPieceArchetype::Standard;
}

float SFlickGameLayer::GetSelectedClassStatValue(const int32 StatIndex) const
{
	float Total = 0.0f;
	constexpr int32 ClassPieceCount = 4;
	for (int32 PieceSlot = 0; PieceSlot < ClassPieceCount; ++PieceSlot)
	{
		Total += GetDisplayStatValue(
			FlickPieceArchetypeRules::GetDisplayStats(GetSelectedClassPiece(PieceSlot)),
			StatIndex);
	}
	return FMath::Clamp(Total / static_cast<float>(ClassPieceCount), 0.0f, 1.0f);
}
