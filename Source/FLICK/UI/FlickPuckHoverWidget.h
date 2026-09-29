#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class AFlickPlayerController;
class AFlickPiece;

// Inspection is cosmetic only: never selects a piece or exposes its aim state.
class SFlickPuckHoverWidget final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickPuckHoverWidget) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AFlickPlayerController>, Controller)
		SLATE_ARGUMENT(TFunction<FString(const AFlickPiece*)>, OwnerName)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual void Tick(const FGeometry& Geometry, double Time, float DeltaTime) override;
	virtual FVector2D ComputeDesiredSize(float Scale) const override { return FVector2D(1600, 900); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry,
		const FSlateRect& Clip, FSlateWindowElementList& Elements, int32 Layer,
		const FWidgetStyle& Style, bool Enabled) const override;
private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FFlickPuckInspectionTest;
#endif
	TWeakObjectPtr<AFlickPlayerController> Controller;
	TWeakObjectPtr<AFlickPiece> LastPiece;
	TFunction<FString(const AFlickPiece*)> OwnerName;
	FString GetLabelText(const AFlickPiece* Piece) const;
	float Alpha = 0;
};
