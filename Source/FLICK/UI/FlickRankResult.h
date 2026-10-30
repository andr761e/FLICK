#pragma once

#include "Core/FlickRankRules.h"
#include "Widgets/SCompoundWidget.h"

class SFlickRankResult final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFlickRankResult) {}
		SLATE_ATTRIBUTE(FFlickRatingUpdate, Update)
		SLATE_ATTRIBUTE(double, Time)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual void Tick(const FGeometry&, double, float) override;
private:
	TAttribute<FFlickRatingUpdate> Update;
	TAttribute<double> Time;
	FFlickRatingUpdate Active;
	double StartTime = 0;
	float Elapsed = 0;
	FLinearColor Accent() const;
};
