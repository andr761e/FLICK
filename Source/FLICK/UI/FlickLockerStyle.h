#pragma once

#include "CoreMinimal.h"

// The locker uses the same 900-unit design frame as Home and Lineups.
namespace FlickLockerStyle
{
	constexpr float RailWidth = 268;
	constexpr float ContentsWidth = 586;
	constexpr float CardWidth = 190;
	constexpr float CardHeight = 204;
	constexpr float CardGap = 8;
	constexpr float CategoryHeight = 31;
	inline const FVector CameraOffset(-88, -207, 131);
	constexpr float PuckFocusHeight = 20;
	constexpr float PuckHorizontalFov = 74;
	inline float PreviewCenterX(const float Aspect)
	{
		// Centre of the right-hand UI column on the shared 900-unit canvas.
		return .5f + (RailWidth + 28 + ContentsWidth + 16) / (2.f * 900.f * Aspect);
	}
	inline float PuckFieldOfView(const float Aspect)
	{
		return FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(PuckHorizontalFov * .5f))
			* FMath::Max(1.f, Aspect / (16.f / 9.f))));
	}
	inline void PuckView(float SurfaceZ, float Scale, float Aspect, FVector& Location, FRotator& Rotation)
	{
		const FVector Focus(0, 0, SurfaceZ + PuckFocusHeight * Scale);
		Rotation = (-CameraOffset).Rotation();
		const FVector Right = Rotation.RotateVector(FVector::RightVector);
		const float Shift = 2.f * (PreviewCenterX(Aspect) - .5f) * CameraOffset.Size() * Scale
			* FMath::Tan(FMath::DegreesToRadians(PuckFieldOfView(Aspect) * .5f));
		// Translate parallel to the image plane, retaining an undistorted product
		// view while aligning it precisely with the caption/drag area below it.
		Location = Focus + CameraOffset * Scale - Right * Shift;
	}
}
