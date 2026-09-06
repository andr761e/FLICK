#pragma once

#include "CoreMinimal.h"

namespace FlickArenaControlRules
{
	inline bool DoCirclesOverlap(
		const FVector2D& FirstCenter,
		const float FirstRadius,
		const FVector2D& SecondCenter,
		const float SecondRadius)
	{
		const float CombinedRadius = FMath::Max(0.0f, FirstRadius) + FMath::Max(0.0f, SecondRadius);
		return FVector2D::DistSquared(FirstCenter, SecondCenter) <= FMath::Square(CombinedRadius);
	}

	/** Swept circle versus circle detects a fast puck touching the activation dot between frames. */
	inline bool DoesSweptCircleCrossCircle(
		const FVector2D& Start,
		const FVector2D& End,
		const float SweptRadius,
		const FVector2D& CircleCenter,
		const float CircleRadius)
	{
		const FVector2D Segment = End - Start;
		const float SegmentLengthSquared = Segment.SizeSquared();
		const float ClosestTime = SegmentLengthSquared > UE_SMALL_NUMBER
			? FMath::Clamp(FVector2D::DotProduct(CircleCenter - Start, Segment) / SegmentLengthSquared, 0.0f, 1.0f)
			: 0.0f;
		return DoCirclesOverlap(
			Start + Segment * ClosestTime,
			SweptRadius,
			CircleCenter,
			CircleRadius);
	}

	inline FVector2D ToOrientedBoxSpace(
		const FVector2D& Point,
		const FVector2D& BoxCenter,
		const float BoxAngleRadians)
	{
		const FVector2D Delta = Point - BoxCenter;
		const float CosAngle = FMath::Cos(BoxAngleRadians);
		const float SinAngle = FMath::Sin(BoxAngleRadians);
		return FVector2D(
			Delta.X * CosAngle + Delta.Y * SinAngle,
			-Delta.X * SinAngle + Delta.Y * CosAngle);
	}

	/** Partial puck overlap is enough to press the rectangular switch. */
	inline bool DoesCircleOverlapOrientedBox(
		const FVector2D& CircleCenter,
		const float CircleRadius,
		const FVector2D& BoxCenter,
		const FVector2D& BoxHalfExtents,
		const float BoxAngleRadians)
	{
		const FVector2D LocalCenter = ToOrientedBoxSpace(CircleCenter, BoxCenter, BoxAngleRadians);
		const FVector2D SafeExtents(
			FMath::Max(0.0f, BoxHalfExtents.X),
			FMath::Max(0.0f, BoxHalfExtents.Y));
		const FVector2D ClosestPoint(
			FMath::Clamp(LocalCenter.X, -SafeExtents.X, SafeExtents.X),
			FMath::Clamp(LocalCenter.Y, -SafeExtents.Y, SafeExtents.Y));
		return FVector2D::DistSquared(LocalCenter, ClosestPoint)
			<= FMath::Square(FMath::Max(0.0f, CircleRadius));
	}

	/** Swept overlap prevents a fast puck skipping a thin switch between frames. */
	inline bool DoesSweptCircleCrossOrientedBox(
		const FVector2D& Start,
		const FVector2D& End,
		const float CircleRadius,
		const FVector2D& BoxCenter,
		const FVector2D& BoxHalfExtents,
		const float BoxAngleRadians)
	{
		const FVector2D LocalStart = ToOrientedBoxSpace(Start, BoxCenter, BoxAngleRadians);
		const FVector2D LocalEnd = ToOrientedBoxSpace(End, BoxCenter, BoxAngleRadians);
		const float SafeRadius = FMath::Max(0.0f, CircleRadius);
		const FVector2D ExpandedExtents(
			FMath::Max(0.0f, BoxHalfExtents.X) + SafeRadius,
			FMath::Max(0.0f, BoxHalfExtents.Y) + SafeRadius);
		const FVector2D Delta = LocalEnd - LocalStart;
		float MinimumTime = 0.0f;
		float MaximumTime = 1.0f;

		const auto ClipAxis = [&MinimumTime, &MaximumTime](
			const float AxisStart,
			const float AxisDelta,
			const float Extent)
		{
			if (FMath::Abs(AxisDelta) <= UE_SMALL_NUMBER)
			{
				return AxisStart >= -Extent && AxisStart <= Extent;
			}
			float EnterTime = (-Extent - AxisStart) / AxisDelta;
			float ExitTime = (Extent - AxisStart) / AxisDelta;
			if (EnterTime > ExitTime)
			{
				Swap(EnterTime, ExitTime);
			}
			MinimumTime = FMath::Max(MinimumTime, EnterTime);
			MaximumTime = FMath::Min(MaximumTime, ExitTime);
			return MinimumTime <= MaximumTime;
		};

		return ClipAxis(LocalStart.X, Delta.X, ExpandedExtents.X)
			&& ClipAxis(LocalStart.Y, Delta.Y, ExpandedExtents.Y);
	}
}
