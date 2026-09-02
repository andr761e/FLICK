#include "Utility/FlickLaunchMath.h"

namespace FlickLaunchMath
{
	FFlickLaunchResult CalculateLaunch(
		const FVector& PieceLocation,
		const FVector& CursorWorldLocation,
		const float MaxDragDistance,
		const float MinDragDistance,
		const float PowerExponent)
	{
		FFlickLaunchResult Result;

		if (!FMath::IsFinite(MaxDragDistance) || MaxDragDistance <= KINDA_SMALL_NUMBER)
		{
			return Result;
		}

		FVector PullVector = PieceLocation - CursorWorldLocation;
		PullVector.Z = 0.0f;

		Result.DragDistance = PullVector.Size();
		if (!FMath::IsFinite(Result.DragDistance) || Result.DragDistance < MinDragDistance)
		{
			return Result;
		}

		const FVector Direction = PullVector.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			return Result;
		}

		const float ClampedDistance = FMath::Min(Result.DragDistance, MaxDragDistance);
		const float LinearPower = FMath::Clamp(ClampedDistance / MaxDragDistance, 0.0f, 1.0f);
		const float SafeExponent = FMath::Max(PowerExponent, KINDA_SMALL_NUMBER);

		Result.bValidShot = true;
		Result.Direction = Direction;
		Result.NormalizedPower = FMath::Clamp(FMath::Pow(LinearPower, SafeExponent), 0.0f, 1.0f);
		return Result;
	}
}
