//shared internal Slate panels and drawing components
#pragma once

#include "UI/FlickGameLayer.h"
#include "UI/FlickUITheme.h"

#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickCosmeticCatalog.h"
#include "Core/FlickControlBindings.h"
#include "Core/FlickVisualSettings.h"
#include "Core/FlickRankRules.h"
#include "Core/FlickPresentationFrame.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameState.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "Pieces/FlickPiece.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "Online/FlickSessionSubsystem.h"
#include "Online/FlickPartySubsystem.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"
#include "UI/FlickHUD.h"
#include "UI/FlickLogoWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SInputKeySelector.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Images/SImage.h"
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
		constexpr float TrainingCardHeight = 168.0f;
		constexpr float FormatCardHeight = 164.0f;
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

	// The left-hand menu is designed for 16:9. On wider monitors its fixed
	// 450-unit cards otherwise dominate the diagonal panel; scale the whole
	// left column together so the wordmark, actions and profile stay related.
	float GetMainMenuColumnScale()
	{
		FVector2D ViewportSize(ReferenceWidth, ReferenceHeight);
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(ViewportSize);
		}
		if (ViewportSize.Y <= 0.0f) return 1.0f;
		const FVector2D FrameSize = FlickPresentationFrame::GetContainedSize(ViewportSize);
		const float Aspect = FrameSize.X / FrameSize.Y;
		return FMath::Clamp((16.0f / 9.0f) / Aspect, 0.78f, 1.0f);
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

	FSlateFontInfo WordmarkTaglineFont(const int32 Size)
	{
		FSlateFontInfo Font = UiFont(Size);
		Font.LetterSpacing = 55;
		return Font;
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

			// The menu column uses the 16:9 design canvas and is additionally
			// reduced on ultrawide monitors. Size the panel from that same canvas,
			// not the full ultrawide width, or its empty area grows while the cards
			// stay fixed and visually shrink inside it.
			const float DesignWidth = FMath::Min(LocalSize.X, LocalSize.Y * (16.0f / 9.0f));
			const float ColumnScale = GetMainMenuColumnScale();
			const float TopEdgeX = DesignWidth * 0.345f * ColumnScale;
			const float BottomEdgeX = DesignWidth * 0.395f * ColumnScale;
			// Slate custom vertices are not MSAA'd. Evaluate the diagonal's
			// signed-distance coverage in screen pixels instead of ending a pair of
			// triangles at an opaque, stair-stepped edge. Multiple narrow bands
			// approximate smoothstep, while the technical line uses Slate's AA path.
			const float PixelScale = FMath::Max(AllottedGeometry.Scale, 0.01f);
			constexpr float CoverageOffsets[] = {-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f};
			constexpr float CoverageAlpha[] = {1.0f, 1.0f, 0.90f, 0.5f, 0.10f, 0.0f, 0.0f};
			const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			const FLinearColor LeftPanelColor = PanelColor.Get() * InWidgetStyle.GetColorAndOpacityTint();
			FLinearColor RightPanelColor = LeftPanelColor;
			RightPanelColor.A *= 0.93f;
			const FColor LeftPanelTint = LeftPanelColor.ToFColor(true);

			TArray<FSlateVertex> Vertices;
			constexpr int32 Columns = 1 + UE_ARRAY_COUNT(CoverageOffsets);
			Vertices.Reserve(Columns * 2);
			const auto AddVertex = [&Vertices, &Transform](const FVector2f Position, const FVector2f Uv, const FColor Color)
			{
				Vertices.Add(FSlateVertex::Make(Transform, Position, Uv, Color));
			};
			TArray<SlateIndex> Indices;
			Indices.Reserve((Columns - 1) * 6);
			for (int32 Row = 0; Row < 2; ++Row)
			{
				const float Y = Row == 0 ? 0.0f : LocalSize.Y;
				const float EdgeX = Row == 0 ? TopEdgeX : BottomEdgeX;
				AddVertex(FVector2f(0.0f, Y), FVector2f(0.0f, static_cast<float>(Row)), LeftPanelTint);
				for (int32 Band = 0; Band < UE_ARRAY_COUNT(CoverageOffsets); ++Band)
				{
					FLinearColor BandColor = RightPanelColor;
					BandColor.A *= CoverageAlpha[Band];
					AddVertex(
						FVector2f(EdgeX + CoverageOffsets[Band] / PixelScale, Y),
						FVector2f(1.0f, static_cast<float>(Row)),
						BandColor.ToFColor(true));
				}
			}
			for (int32 Column = 0; Column < Columns - 1; ++Column)
			{
				const SlateIndex TopLeft = static_cast<SlateIndex>(Column);
				const SlateIndex BottomLeft = static_cast<SlateIndex>(Column + Columns);
				Indices.Add(TopLeft);
				Indices.Add(TopLeft + 1);
				Indices.Add(BottomLeft + 1);
				Indices.Add(TopLeft);
				Indices.Add(BottomLeft + 1);
				Indices.Add(BottomLeft);
			}
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId,
				WhiteBrush()->GetRenderingResource(),
				Vertices,
				Indices,
				nullptr,
				0,
				0);
			FSlateDrawElement::MakeLines(
				OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
				{FVector2D(TopEdgeX, 0.0f), FVector2D(BottomEdgeX, LocalSize.Y)},
				ESlateDrawEffect::None,
				EdgeColor.Get() * InWidgetStyle.GetColorAndOpacityTint(),
				true, 1.0f / PixelScale);
			return LayerId + 1;
		}

	private:
		TAttribute<FLinearColor> PanelColor;
		TAttribute<FLinearColor> EdgeColor;
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
			, _Primary(false)
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
			SLATE_ATTRIBUTE(bool, Primary)
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
			Primary = InArgs._Primary;
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

			// Small broken rails give each row the same technical, asymmetric character as the
			// larger HUD panels without turning the whole outline into a neon border.
			const bool bPrimary = Primary.Get();
			const FLinearColor RailColor = (bPrimary
				? FLinearColor(0.015f, 0.025f, 0.025f, 0.86f)
				: FMath::Lerp(FLinearColor(0.32f, 0.42f, 0.44f, 0.62f), HoverBorderColor.Get(), HighlightAlpha * 0.72f))
				* InWidgetStyle.GetColorAndOpacityTint();
			const FLinearColor AccentColor = (bPrimary ? FLinearColor(0.015f, 0.025f, 0.025f, 0.94f) : HoverBorderColor.Get())
				* InWidgetStyle.GetColorAndOpacityTint();

			const auto DrawRail = [&](const TArray<FVector2D>& Rail, const FLinearColor& Color, const float Width)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), Rail,
					ESlateDrawEffect::None, Color, true, Width);
			};

			DrawRail({FVector2D(10.0f, 1.0f), FVector2D(54.0f, 1.0f)}, RailColor, bPrimary ? 2.0f : 1.35f);
			DrawRail({FVector2D(Size.X - 72.0f, Size.Y - 1.0f), FVector2D(Size.X - 17.0f, Size.Y - 1.0f)}, RailColor, 1.35f);
			DrawRail({
				FVector2D(Size.X - RightCut - 3.0f, 1.0f),
				FVector2D(Size.X - 2.0f, Size.Y - 2.0f)}, AccentColor, bPrimary ? 2.4f : 1.5f);

			if (bPrimary || HighlightAlpha > 0.01f)
			{
				const float RailAlpha = bPrimary ? 1.0f : HighlightAlpha;
				DrawRail({FVector2D(2.0f, 11.0f), FVector2D(2.0f, Size.Y - 11.0f)}, AccentColor.CopyWithNewOpacity(AccentColor.A * RailAlpha), bPrimary ? 3.5f : 2.25f);
			}
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
		TAttribute<bool> Primary;
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
			const auto Draw = [&OutDrawElements, &AllottedGeometry, &Tint, LayerId](const TArray<FVector2D>& Points, const float Thickness = 2.15f, const float Opacity = 1.0f)
			{
				FSlateDrawElement::MakeLines(
					OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Points,
					ESlateDrawEffect::None, Tint.CopyWithNewOpacity(Tint.A * Opacity), true, Thickness);
			};
			const auto Circle = [](const FVector2D& CircleCenter, const float Radius)
			{
				TArray<FVector2D> Points;
				for (int32 Segment = 0; Segment <= 40; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / 40.0f;
					Points.Add(CircleCenter + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				return Points;
			};

			switch (Playlist)
			{
			case EFlickPlayPlaylist::Competitive:
				Draw({Center + FVector2D(0.0f, -21.0f), Center + FVector2D(20.0f, -12.0f), Center + FVector2D(16.0f, 11.0f), Center + FVector2D(0.0f, 23.0f), Center + FVector2D(-16.0f, 11.0f), Center + FVector2D(-20.0f, -12.0f), Center + FVector2D(0.0f, -21.0f)}, 2.25f);
				Draw({Center + FVector2D(-8.0f, 2.0f), Center + FVector2D(0.0f, -7.0f), Center + FVector2D(8.0f, 2.0f)}, 2.8f);
				break;
			case EFlickPlayPlaylist::Training:
				Draw(Circle(Center, 22.0f), 1.65f, 0.72f);
				Draw(Circle(Center, 13.0f), 2.0f, 0.9f);
				Draw(Circle(Center, 4.0f), 2.9f);
				Draw({Center + FVector2D(0.0f, -27.0f), Center + FVector2D(0.0f, -17.0f)});
				Draw({Center + FVector2D(27.0f, 0.0f), Center + FVector2D(17.0f, 0.0f)});
				break;
			case EFlickPlayPlaylist::PrivateMatch:
				Draw({Center + FVector2D(-17.0f, -3.0f), Center + FVector2D(-17.0f, 20.0f), Center + FVector2D(17.0f, 20.0f), Center + FVector2D(17.0f, -3.0f), Center + FVector2D(-17.0f, -3.0f)}, 2.2f);
				Draw({Center + FVector2D(-11.0f, -3.0f), Center + FVector2D(-11.0f, -13.0f), Center + FVector2D(-6.0f, -20.0f), Center + FVector2D(6.0f, -20.0f), Center + FVector2D(11.0f, -13.0f), Center + FVector2D(11.0f, -3.0f)}, 2.2f);
				Draw(Circle(Center + FVector2D(0.0f, 7.0f), 3.0f), 1.8f);
				break;
			case EFlickPlayPlaylist::Casual:
			default:
				Draw(Circle(Center + FVector2D(-14.0f, 7.0f), 14.0f), 2.3f);
				Draw(Circle(Center + FVector2D(14.0f, -7.0f), 14.0f), 2.3f);
				Draw(Circle(Center + FVector2D(-14.0f, 7.0f), 5.0f), 1.65f, 0.82f);
				Draw(Circle(Center + FVector2D(14.0f, -7.0f), 5.0f), 1.65f, 0.82f);
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
				for (int32 Segment = 0; Segment <= 28; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / 28.0f;
					Points.Add(Position + FVector2D(FMath::Cos(Angle) * 4.3f, FMath::Sin(Angle) * 3.2f));
				}
				Draw(LayerId + 2, Points, Color.CopyWithNewOpacity(0.95f), 1.65f);
				TArray<FVector2D> Hub;
				for (int32 Segment = 0; Segment <= 20; ++Segment)
				{
					const float Angle = 2.0f * PI * static_cast<float>(Segment) / 20.0f;
					Hub.Add(Position + FVector2D(FMath::Cos(Angle) * 1.8f, FMath::Sin(Angle) * 1.35f));
				}
				Draw(LayerId + 3, Hub, Color.CopyWithNewOpacity(0.72f), 0.85f);
			};

			Draw(LayerId, Ellipse(Size.X * 0.47f, Size.Y * 0.43f), FLinearColor(0.45f, 0.61f, 0.69f, 0.78f), 1.25f);
			Draw(LayerId, Ellipse(Size.X * 0.43f, Size.Y * 0.38f), FLinearColor(0.18f, 0.36f, 0.44f, 0.58f), 0.75f);
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
			Draw(LayerId + 1, Ellipse(5.0f, 4.0f), FLinearColor(0.48f, 0.62f, 0.68f, 0.62f), 0.85f);
			return LayerId + 3;
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
			const EFlickPieceArchetype VisualArchetype = Archetype.Get();
			const float RadiusRatio = FlickPieceArchetypeRules::Get(VisualArchetype).RadiusMultiplier;
			const float BaseRadius = FMath::Min(Size.X, Size.Y) * 0.39f * Scale * RadiusRatio;
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
			float TopScale = 0.84f;
			switch (VisualArchetype)
			{
			case EFlickPieceArchetype::Heavy: TopScale = 0.77f; break;
			case EFlickPieceArchetype::Blocker: TopScale = 0.87f; break;
			case EFlickPieceArchetype::Compact: TopScale = 0.80f; break;
			case EFlickPieceArchetype::Toppler: TopScale = 0.82f; break;
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
			DrawDisc(LayerId + 4, BaseRadius * 0.97f, FVector2f::ZeroVector,
				SilverHighlight, FLinearColor(0.20f, 0.27f, 0.33f, 1.0f));
			DrawRing(LayerId + 5, BaseRadius * 0.975f, TeamLight.CopyWithNewOpacity(0.94f), FineLine * 1.2f);
			DrawDisc(LayerId + 6, BaseRadius * TopScale, FVector2f::ZeroVector, DarkMetal, MidMetal);

			// The real mesh has broad lit rim panels separated by graphite lugs, not a uniform cyan outline.
			for (int32 WindowIndex = 0; WindowIndex < 12; ++WindowIndex)
			{
				const float CenterAngle = 2.0f * PI * static_cast<float>(WindowIndex) / 12.0f;
				DrawArc(LayerId + 7, BaseRadius * 0.89f, CenterAngle - 0.19f, CenterAngle + 0.19f,
					TeamLight, FineLine * 5.2f);
				if (WindowIndex % 3 == 0)
				{
					DrawArc(LayerId + 8, BaseRadius * 0.89f, CenterAngle - 0.065f, CenterAngle + 0.065f,
						DarkMetal, FineLine * 5.7f);
				}
			}
			DrawRing(LayerId + 9, BaseRadius * (TopScale - 0.06f), Silver.CopyWithNewOpacity(0.85f), FineLine * 0.9f);
			DrawDisc(LayerId + 10, BaseRadius * (TopScale - 0.14f), FVector2f::ZeroVector, MidMetal, DarkMetal);
			DrawRing(LayerId + 11, BaseRadius * (TopScale - 0.14f), TeamLight.CopyWithNewOpacity(0.98f), FineLine * 1.45f);

			const FVector2D SymbolCenter(Center.X, Center.Y);
			const float SymbolRadius = BaseRadius * 0.26f;
			switch (VisualArchetype)
			{
			case EFlickPieceArchetype::Standard:
				DrawRing(LayerId + 13, SymbolRadius * 0.72f, TeamLight, FineLine * 1.9f);
				break;
			case EFlickPieceArchetype::Toppler:
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, -SymbolRadius * 0.05f), SymbolCenter + FVector2D(0.0f, -SymbolRadius * 0.82f), SymbolCenter + FVector2D(SymbolRadius, -SymbolRadius * 0.05f)}, TeamLight, FineLine * 2.4f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.82f, SymbolRadius * 0.52f), SymbolCenter + FVector2D(0.0f, -SymbolRadius * 0.24f), SymbolCenter + FVector2D(SymbolRadius * 0.82f, SymbolRadius * 0.52f)}, TeamLight, FineLine * 2.3f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.34f, SymbolRadius * 0.92f), SymbolCenter + FVector2D(SymbolRadius * 0.34f, SymbolRadius * 0.92f)}, TeamLight, FineLine * 2.1f);
				break;
			case EFlickPieceArchetype::Bouncer:
				DrawDisc(LayerId + 13, SymbolRadius * 0.21f, FVector2f(0.0f, -SymbolRadius * 0.46f), TeamLight, TeamLight);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, 0.0f), SymbolCenter + FVector2D(-SymbolRadius * 0.62f, SymbolRadius * 0.08f), SymbolCenter + FVector2D(-SymbolRadius * 0.30f, SymbolRadius * 0.75f)}, TeamLight, FineLine * 2.1f);
				DrawSymbol({SymbolCenter + FVector2D(SymbolRadius, 0.0f), SymbolCenter + FVector2D(SymbolRadius * 0.62f, SymbolRadius * 0.08f), SymbolCenter + FVector2D(SymbolRadius * 0.30f, SymbolRadius * 0.75f)}, TeamLight, FineLine * 2.1f);
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
					DrawSymbol({SymbolCenter + FVector2D(X - SymbolRadius * 0.35f, -SymbolRadius * 0.68f), SymbolCenter + FVector2D(X + SymbolRadius * 0.25f, 0.0f), SymbolCenter + FVector2D(X - SymbolRadius * 0.35f, SymbolRadius * 0.68f)}, TeamLight, FineLine * 2.5f);
				}
				break;
			case EFlickPieceArchetype::Grippy:
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius, SymbolRadius * 0.60f), SymbolCenter + FVector2D(0.0f, -SymbolRadius * 0.82f), SymbolCenter + FVector2D(SymbolRadius, SymbolRadius * 0.60f)}, TeamLight, FineLine * 2.6f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.47f, SymbolRadius * 0.08f), SymbolCenter + FVector2D(0.0f, -SymbolRadius * 0.43f), SymbolCenter + FVector2D(SymbolRadius * 0.47f, SymbolRadius * 0.08f)}, TeamLight, FineLine * 2.3f);
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
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.48f, 0.0f), SymbolCenter + FVector2D(SymbolRadius * 0.48f, 0.0f)}, TeamLight, FineLine * 2.6f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.62f, -SymbolRadius * 0.65f), SymbolCenter + FVector2D(-SymbolRadius * 0.62f, SymbolRadius * 0.65f)}, TeamLight, FineLine * 5.0f);
				DrawSymbol({SymbolCenter + FVector2D(SymbolRadius * 0.62f, -SymbolRadius * 0.65f), SymbolCenter + FVector2D(SymbolRadius * 0.62f, SymbolRadius * 0.65f)}, TeamLight, FineLine * 5.0f);
				DrawSymbol({SymbolCenter + FVector2D(-SymbolRadius * 0.98f, -SymbolRadius * 0.32f), SymbolCenter + FVector2D(-SymbolRadius * 0.98f, SymbolRadius * 0.32f)}, TeamLight, FineLine * 2.5f);
				DrawSymbol({SymbolCenter + FVector2D(SymbolRadius * 0.98f, -SymbolRadius * 0.32f), SymbolCenter + FVector2D(SymbolRadius * 0.98f, SymbolRadius * 0.32f)}, TeamLight, FineLine * 2.5f);
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
			FLinearColor RequestedFill = BackgroundColor.Get();
			// Keep legacy dark panels in the same teal-black glass family as the
			// redesigned menu cards while preserving intentional colored states.
			if (RequestedFill.R < 0.08f && RequestedFill.G < 0.08f && RequestedFill.B < 0.08f)
			{
				RequestedFill.R = Panel.R;
				RequestedFill.G = Panel.G;
				RequestedFill.B = Panel.B;
				RequestedFill.A = FMath::Min(RequestedFill.A, Panel.A);
			}
			const FLinearColor Fill = RequestedFill * Tint;
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
					Outline, Effect, FLinearColor(0.27f, 0.36f, 0.39f, bUseAccentForOutline ? 0.82f : 0.62f) * Tint, true, BorderWidth);
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

	// Main-menu proof-of-concept frame: a restrained dark glass panel with a
	// complete chamfered outline and opposing team-lime corner brackets.
	class SFlickMainMenuPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickMainMenuPanel)
			: _BackgroundColor(FLinearColor(0.012f, 0.026f, 0.032f, 0.94f))
			, _AccentColor(Brand)
			, _CutSize(12.0f)
			, _BorderWidth(1.15f)
			, _ImageBrush(nullptr)
			, _Padding(FMargin(0.0f))
		{}
			SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
			SLATE_ATTRIBUTE(FLinearColor, AccentColor)
			SLATE_ARGUMENT(float, CutSize)
			SLATE_ARGUMENT(float, BorderWidth)
			SLATE_ATTRIBUTE(const FSlateBrush*, ImageBrush)
			SLATE_ARGUMENT(FMargin, Padding)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			BackgroundColor = InArgs._BackgroundColor;
			AccentColor = InArgs._AccentColor;
			CutSize = InArgs._CutSize;
			BorderWidth = InArgs._BorderWidth;
			ImageBrush = InArgs._ImageBrush;
			ChildSlot.Padding(InArgs._Padding)[InArgs._Content.Widget];
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			if (Size.X < 2.0f || Size.Y < 2.0f) return LayerId;
			const float Cut = FMath::Clamp(CutSize, 0.0f, FMath::Min(Size.X, Size.Y) * 0.2f);
			const TArray<FVector2D> Points = {
				{Cut, 0.0f}, {Size.X - Cut, 0.0f}, {Size.X, Cut}, {Size.X, Size.Y - Cut},
				{Size.X - Cut, Size.Y}, {Cut, Size.Y}, {0.0f, Size.Y - Cut}, {0.0f, Cut}};
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
			if (const FSlateBrush* Brush = ImageBrush.Get())
			{
				// Draw the artwork inside the same eight-sided geometry as the panel.
				// A centred vertical crop preserves the source image's aspect ratio.
				const FVector2D ImageSize = Brush->GetImageSize();
				const float SourceAspect = ImageSize.Y > 0.0f ? ImageSize.X / ImageSize.Y : 1.0f;
				const float PanelAspect = Size.X / Size.Y;
				const float UvHeight = FMath::Min(1.0f, SourceAspect / PanelAspect);
				const float UvTop = (1.0f - UvHeight) * 0.5f;
				for (int32 Index = 0; Index < Points.Num(); ++Index)
				{
					const FVector2D Uv(Points[Index].X / Size.X, UvTop + Points[Index].Y / Size.Y * UvHeight);
					Vertices[Index] = FSlateVertex::Make(Transform, FVector2f(Points[Index]), FVector2f(Uv), Tint.ToFColor(true));
				}
				const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*Brush);
				FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId + 1,
					Handle, Vertices, Indices, nullptr, 0, 0, Effect);
			}
			// Low-contrast architectural facets keep the glass from reading as a flat fill.
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X * 0.39f, 1.0f}, {Size.X * 0.61f, Size.Y - 1.0f}},
				Effect, FLinearColor::FromSRGBColor(FColor(22, 33, 36, 42)) * Tint, true, 42.0f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X * 0.72f, 1.0f}, {Size.X * 0.9f, Size.Y - 1.0f}},
				Effect, FLinearColor::FromSRGBColor(FColor(9, 17, 19, 38)) * Tint, true, 26.0f);
			TArray<FVector2D> Outline = Points;
			Outline.Add(Points[0]);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				Outline, Effect, FLinearColor(0.26f, 0.34f, 0.38f, 0.88f) * Tint, true, BorderWidth);

			const float Bracket = FMath::Min(34.0f, Size.X * 0.12f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{0.0f, Cut}, {Cut, 0.0f}, {Bracket, 0.0f}},
				Effect, Accent, false, FMath::Max(1.8f, BorderWidth));
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X - Bracket, Size.Y}, {Size.X - Cut, Size.Y}, {Size.X, Size.Y - Cut}},
				Effect, Accent, false, FMath::Max(1.8f, BorderWidth));
			return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
				OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
		}

	private:
		TAttribute<FLinearColor> BackgroundColor;
		TAttribute<FLinearColor> AccentColor;
		TAttribute<const FSlateBrush*> ImageBrush;
		float CutSize = 12.0f;
		float BorderWidth = 1.15f;
	};

	class SFlickPlayHeaderPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickPlayHeaderPanel) : _Padding(FMargin(0.0f)) {}
			SLATE_ARGUMENT(FMargin, Padding)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot.Padding(InArgs._Padding)[InArgs._Content.Widget];
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			if (Size.X < 2.0f || Size.Y < 2.0f) return LayerId;
			const float Cut = FMath::Min(14.0f, Size.Y * 0.2f);
			const TArray<FVector2D> Frame = {
				{0.0f, 0.0f}, {Size.X - Cut, 0.0f}, {Size.X, Cut},
				{Size.X, Size.Y}, {Cut, Size.Y}, {0.0f, Size.Y - Cut}};
			const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
			const ESlateDrawEffect Effect = bParentEnabled && IsEnabled() ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			const auto& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			const FLinearColor Fill = FLinearColor::FromSRGBColor(FColor(10, 18, 21, 232)) * Tint;
			for (const FVector2D& Point : Frame)
			{
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Point), FVector2f::ZeroVector, Fill.ToFColor(true)));
			}
			for (int32 Index = 1; Index + 1 < Frame.Num(); ++Index)
			{
				Indices.Append({0, static_cast<SlateIndex>(Index), static_cast<SlateIndex>(Index + 1)});
			}
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId,
				WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0, Effect);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X * 0.28f, 1.0f}, {Size.X * 0.36f, Size.Y - 1.0f}},
				Effect, FLinearColor(0.14f, 0.25f, 0.29f, 0.12f) * Tint, true, 46.0f);

			TArray<FVector2D> Outline = Frame;
			Outline.Add(FVector2D::ZeroVector);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				Outline, Effect, FLinearColor(0.31f, 0.42f, 0.47f, 0.82f) * Tint, true, 1.0f);
			const FLinearColor CoolRail(0.19f, 0.56f, 0.65f, 0.62f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X * 0.12f, 0.0f}, {Size.X * 0.22f, 0.0f}}, Effect, CoolRail * Tint, false, 1.2f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X * 0.61f, Size.Y}, {Size.X * 0.76f, Size.Y}}, Effect, CoolRail * Tint, false, 1.2f);

			const FLinearColor Lime = Brand * Tint;
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{Size.X - Cut, 0.0f}, {Size.X, Cut}},
				Effect, Lime, false, 5.0f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{0.0f, Size.Y - Cut}, {Cut, Size.Y}},
				Effect, Lime, false, 5.0f);
			return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
				OutDrawElements, LayerId + 5, InWidgetStyle, bParentEnabled);
		}
	};

	class SFlickPlaylistCardPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickPlaylistCardPanel) : _Selected(false), _Padding(FMargin(0.0f)) {}
			SLATE_ATTRIBUTE(bool, Selected)
			SLATE_ARGUMENT(FMargin, Padding)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Selected = InArgs._Selected;
			ChildSlot.Padding(InArgs._Padding)[InArgs._Content.Widget];
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			if (Size.X < 2.0f || Size.Y < 2.0f) return LayerId;
			const float Cut = FMath::Min(13.0f, Size.Y * 0.16f);
			const TArray<FVector2D> Frame = {
				{0.0f, 0.0f}, {Size.X - Cut, 0.0f}, {Size.X, Cut},
				{Size.X, Size.Y}, {Cut, Size.Y}, {0.0f, Size.Y - Cut}};
			const bool bSelected = Selected.Get(false);
			const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
			const ESlateDrawEffect Effect = bParentEnabled && IsEnabled() ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			const auto& Transform = AllottedGeometry.GetAccumulatedRenderTransform();

			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			const FLinearColor Fill = FLinearColor::FromSRGBColor(FColor(9, 17, 19, bSelected ? 240 : 226)) * Tint;
			for (const FVector2D& Point : Frame)
			{
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Point), FVector2f::ZeroVector, Fill.ToFColor(true)));
			}
			for (int32 Index = 1; Index + 1 < Frame.Num(); ++Index)
			{
				Indices.Append({0, static_cast<SlateIndex>(Index), static_cast<SlateIndex>(Index + 1)});
			}
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId,
				WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0, Effect);

			TArray<FVector2D> Outline = Frame;
			Outline.Add(Frame[0]);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				Outline, Effect, FLinearColor(0.30f, 0.40f, 0.44f, bSelected ? 0.92f : 0.72f) * Tint, true, 1.0f);

			if (bSelected)
			{
				const FLinearColor Lime = Brand * Tint;
				// The active state carries a fine gold-lime circuit continuously around the frame.
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					Outline, Effect, Lime.CopyWithNewOpacity(0.92f), true, 1.7f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{0.0f, Size.Y - Cut}, {Cut, Size.Y}}, Effect, Lime, false, 5.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{Size.X - Cut, 0.0f}, {Size.X, Cut}}, Effect, Lime, false, 6.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{0.0f, 0.0f}, {Size.X * 0.18f, 0.0f}}, Effect, Lime.CopyWithNewOpacity(0.82f), false, 1.8f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{Cut, Size.Y}, {Size.X * 0.72f, Size.Y}}, Effect, Lime.CopyWithNewOpacity(0.88f), false, 1.8f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{0.0f, Cut * 1.6f}, {0.0f, Size.Y - Cut}}, Effect, Lime.CopyWithNewOpacity(0.76f), false, 1.6f);
			}
			else
			{
				const FLinearColor Cool(0.38f, 0.46f, 0.49f, 0.78f);
				// Idle cards retain the heavy structural chamfer caps, but never inherit lime.
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{0.0f, Size.Y - Cut}, {Cut, Size.Y}}, Effect, Cool * Tint, false, 4.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{Size.X - Cut, 0.0f}, {Size.X, Cut}}, Effect, Cool * Tint, false, 4.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{0.0f, 0.0f}, {Size.X * 0.10f, 0.0f}}, Effect, Cool * Tint, false, 1.0f);
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
					TArray<FVector2D>{{Size.X * 0.82f, Size.Y}, {Size.X, Size.Y}}, Effect, Cool * Tint, false, 1.0f);
			}

			return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
				OutDrawElements, LayerId + 4, InWidgetStyle, bParentEnabled);
		}

	private:
		TAttribute<bool> Selected;
	};

	class SFlickPlayFooterPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SFlickPlayFooterPanel) : _Padding(FMargin(0.0f)) {}
			SLATE_ARGUMENT(FMargin, Padding)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot.Padding(InArgs._Padding)[InArgs._Content.Widget];
		}

		virtual int32 OnPaint(
			const FPaintArgs& Args, const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
			const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const override
		{
			const FVector2D Size = AllottedGeometry.GetLocalSize();
			if (Size.X < 2.0f || Size.Y < 2.0f) return LayerId;
			const float Cut = FMath::Min(12.0f, Size.Y * 0.18f);
			const TArray<FVector2D> Frame = {
				{0.0f, 0.0f}, {Size.X - Cut, 0.0f}, {Size.X, Cut},
				{Size.X, Size.Y - Cut}, {Size.X - Cut, Size.Y}, {Cut, Size.Y},
				{0.0f, Size.Y - Cut}};
			const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
			const ESlateDrawEffect Effect = bParentEnabled && IsEnabled() ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
			const auto& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			const FLinearColor Fill = Panel * Tint;
			for (const FVector2D& Point : Frame)
			{
				Vertices.Add(FSlateVertex::Make(Transform, FVector2f(Point), FVector2f::ZeroVector, Fill.ToFColor(true)));
			}
			for (int32 Index = 1; Index + 1 < Frame.Num(); ++Index)
			{
				Indices.Append({0, static_cast<SlateIndex>(Index), static_cast<SlateIndex>(Index + 1)});
			}
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId,
				WhiteBrush()->GetRenderingResource(), Vertices, Indices, nullptr, 0, 0, Effect);
			TArray<FVector2D> Outline = Frame;
			Outline.Add(Frame[0]);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
				Outline, Effect, FLinearColor(0.31f, 0.43f, 0.46f, 0.82f) * Tint, true, 1.0f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{0.0f, Size.Y - Cut}, {Cut, Size.Y}}, Effect, Brand * Tint, false, 3.5f);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2D>{{0.0f, 0.0f}, {Size.X * 0.18f, 0.0f}}, Effect,
				FLinearColor(0.18f, 0.58f, 0.64f, 0.54f) * Tint, false, 1.2f);
			return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect,
				OutDrawElements, LayerId + 3, InWidgetStyle, bParentEnabled);
		}
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
