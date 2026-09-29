#include "UI/FlickPuckHoverWidget.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickPlayerController.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickVisualSettings.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"

void SFlickPuckHoverWidget::Construct(const FArguments& Args)
{
	Controller = Args._Controller;
	OwnerName = Args._OwnerName;
	SetCanTick(true);
	ForceVolatile(true);
}

void SFlickPuckHoverWidget::Tick(const FGeometry& Geometry, double Time, float DeltaTime)
{
	const AFlickPiece* Piece = Controller.IsValid() ? Controller->GetInspectedPiece() : nullptr;
	if (!Piece) { Alpha = 0; LastPiece.Reset(); return; }
	// No dwell timer. A switch to another puck updates immediately, without an
	// animated trail or delayed old name. Only the initial appearance fades in.
	if (LastPiece.IsValid() && LastPiece.Get() != Piece) Alpha = 1;
	LastPiece = const_cast<AFlickPiece*>(Piece);
	Alpha = FMath::Min(1.0f, Alpha + DeltaTime / .055f);
}

FString SFlickPuckHoverWidget::GetLabelText(const AFlickPiece* Piece) const
{
	FString Type = GetPieceArchetypeName(Piece->GetArchetype()).ToLower();
	if (!Type.IsEmpty()) Type[0] = FChar::ToUpper(Type[0]);
	switch (FlickVisualSettings::GetPuckHoverDetail())
	{
	case 0: return FString();
	case 1: return OwnerName(Piece).Left(48);
	case 2: return Type;
	default: return OwnerName(Piece).Left(48) + TEXT(" - ") + Type;
	}
}

int32 SFlickPuckHoverWidget::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& Clip, FSlateWindowElementList& Elements, int32 Layer,
	const FWidgetStyle& Style, bool Enabled) const
{
	const AFlickPiece* Piece = Controller.IsValid() ? Controller->GetInspectedPiece() : nullptr;
	if (!Piece || Alpha <= 0 || !OwnerName) return Layer;
	FVector2D Screen;
	if (!Controller->ProjectWorldLocationToScreen(Piece->GetActorLocation()
		+ FVector(0, 0, Piece->GetPieceThickness() * .5f + 12), Screen)) return Layer;
	int32 Width = 0, Height = 0;
	Controller->GetViewportSize(Width, Height);
	if (Width <= 0 || Height <= 0) return Layer;
	const FVector2D Size = Geometry.GetLocalSize();
	Screen *= FVector2D(Size.X / Width, Size.Y / Height);
	const FString Text = GetLabelText(Piece);
	if (Text.IsEmpty()) return Layer;
	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", FlickVisualSettings::GetPuckHoverSize());
	const FVector2D Extent = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
	const FVector2D Pos(FMath::Clamp(Screen.X - Extent.X * .5f, 8.0f, FMath::Max(8.0f, Size.X - Extent.X - 8)),
		FMath::Clamp(Screen.Y - Extent.Y - 12, 8.0f, FMath::Max(8.0f, Size.Y - Extent.Y - 8)));
	FSlateDrawElement::MakeText(Elements, Layer, Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(Pos + FVector2D(1, 2))),
		Text, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Alpha));
	FSlateDrawElement::MakeText(Elements, Layer + 1, Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(Pos)),
		Text, Font, ESlateDrawEffect::None, GetTeamColor(Piece->GetTeam()).CopyWithNewOpacity(Alpha) * Style.GetColorAndOpacityTint());
	return Layer + 1;
}
