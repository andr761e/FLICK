#pragma once

#include "CoreMinimal.h"

// Subdivide only where opacity changes, retaining the original clipped outline.
// This avoids rectangular image seams without blur targets or extra textures.
namespace FlickArtworkGeometry
{
	inline TArray<TArray<FVector2D>> FadePolygons(const TArray<FVector2D>& Outline, FVector2D Size, FMargin Fade)
	{
		TArray<TArray<FVector2D>> Polygons;
		Polygons.Add(Outline);
		const auto Split = [&Polygons](bool bX, float Boundary) {
			TArray<TArray<FVector2D>> Result;
			for (const auto& Polygon : Polygons)
				for (bool bBefore : {true,false})
				{
					TArray<FVector2D> Clipped;
					for (int32 I = 0; I < Polygon.Num(); ++I)
					{
						const FVector2D A = Polygon[I], B = Polygon[(I+1)%Polygon.Num()];
						const float Av = bX ? A.X : A.Y, Bv = bX ? B.X : B.Y;
						const bool bA = bBefore ? Av <= Boundary : Av >= Boundary;
						const bool bB = bBefore ? Bv <= Boundary : Bv >= Boundary;
						if (bA) Clipped.Add(A);
						if (bA != bB) Clipped.Add(FMath::Lerp(A,B,(Boundary-Av)/(Bv-Av)));
					}
					if (Clipped.Num() >= 3) Result.Add(MoveTemp(Clipped));
				}
			Polygons = MoveTemp(Result);
		};
		if (Fade.Left > 0) Split(true,FMath::Min(double(Fade.Left),Size.X*.5));
		if (Fade.Right > 0) Split(true,Size.X-FMath::Min(double(Fade.Right),Size.X*.5));
		if (Fade.Top > 0) Split(false,FMath::Min(double(Fade.Top),Size.Y*.5));
		if (Fade.Bottom > 0) Split(false,Size.Y-FMath::Min(double(Fade.Bottom),Size.Y*.5));
		return Polygons;
	}

	inline float Opacity(FVector2D P, FVector2D Size, FMargin Fade)
	{
		float Alpha = 1;
		if (Fade.Left > 0) Alpha *= FMath::Clamp(P.X/FMath::Min(double(Fade.Left),Size.X*.5),0.,1.);
		if (Fade.Right > 0) Alpha *= FMath::Clamp((Size.X-P.X)/FMath::Min(double(Fade.Right),Size.X*.5),0.,1.);
		if (Fade.Top > 0) Alpha *= FMath::Clamp(P.Y/FMath::Min(double(Fade.Top),Size.Y*.5),0.,1.);
		if (Fade.Bottom > 0) Alpha *= FMath::Clamp((Size.Y-P.Y)/FMath::Min(double(Fade.Bottom),Size.Y*.5),0.,1.);
		return Alpha;
	}
}
