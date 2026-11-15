#pragma once

#include "CoreMinimal.h"

namespace FlickReplayInterpolation
{
	// Use the recorded endpoint velocities without inventing overshoot between
	// sparse samples. Limit each Hermite tangent to a monotone interval.
	inline FVector Position(const FVector& A, const FVector& VelocityA, const FVector& B,
		const FVector& VelocityB, float Seconds, float Alpha)
	{
		if(Seconds<=SMALL_NUMBER || (VelocityA.IsNearlyZero() && VelocityB.IsNearlyZero()))
			return FMath::Lerp(A,B,Alpha);
		FVector Result;
		for(int32 Axis=0;Axis<3;++Axis)
		{
			const double Delta=B[Axis]-A[Axis];
			if(FMath::Abs(Delta)<.0001) {Result[Axis]=A[Axis];continue;}
			double U=FMath::Clamp(VelocityA[Axis]*Seconds/Delta,0.,3.);
			double V=FMath::Clamp(VelocityB[Axis]*Seconds/Delta,0.,3.);
			const double Length=FMath::Sqrt(U*U+V*V);
			if(Length>3.) {U*=3./Length;V*=3./Length;}
			Result[Axis]=FMath::Clamp(FMath::CubicInterp(A[Axis],U*Delta,B[Axis],V*Delta,Alpha),
				FMath::Min(A[Axis],B[Axis]),FMath::Max(A[Axis],B[Axis]));
		}
		return Result;
	}
}
