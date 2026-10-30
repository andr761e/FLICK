#pragma once

#include "Core/FlickRankRules.h"

// Presentation only: accepted ratings remain owned by the ranking subsystem.
namespace FlickRankTransition
{
	constexpr float RevealSeconds = 1.65f;
	inline float Phase(float Seconds, float Start, float Duration)
	{
		return FMath::Clamp((Seconds - Start) / Duration, 0.f, 1.f);
	}
	inline float Smooth(float T) { return T * T * (3.f - 2.f * T); }
	inline FFlickRankProgress Progress(const FFlickRatingUpdate& Update, bool bNew)
	{
		FFlickRankProgress Result;
		Result.Rating = bNew ? Update.NewRating : Update.OldRating;
		Result.MatchesPlayed = FMath::Max(0, Update.MatchesPlayed - (bNew ? 0 : 1));
		return Result;
	}
	inline int32 Direction(const FFlickRatingUpdate& Update)
	{
		return FMath::Sign(FlickRankRules::GetBadgeIndex(Progress(Update, true))
			- FlickRankRules::GetBadgeIndex(Progress(Update, false)));
	}
	inline int32 RatingAt(const FFlickRatingUpdate& Update, float Seconds)
	{
		// Finish the count exactly at the badge reveal, so rank and MMR agree.
		const float T = Phase(Seconds, .45f, RevealSeconds - .45f);
		return FMath::RoundToInt(FMath::Lerp(static_cast<float>(Update.OldRating), static_cast<float>(Update.NewRating), Smooth(T)));
	}
	inline float OldOpacity(float Seconds) { return 1.f - Smooth(Phase(Seconds, 1.12f, RevealSeconds - 1.12f)); }
	inline float NewOpacity(float Seconds) { return Smooth(Phase(Seconds, RevealSeconds, .16f)); }
	inline float BadgeScale(float Seconds, int32 Direction)
	{
		const float T = Phase(Seconds, RevealSeconds, .95f);
		if (T >= 1.f || Direction == 0) return 1.f;
		// Promotion springs outward; demotion lands with a restrained rebound.
		return Direction > 0 ? .35f + .65f * (1.f - FMath::Exp(-6.f * T) * FMath::Cos(10.f * T))
			: .8f + .2f * Smooth(T) + .055f * FMath::Sin(T * PI);
	}
	inline float BadgeOffset(float Seconds, int32 Direction)
	{
		const float T = Phase(Seconds, RevealSeconds, .75f);
		return Direction == 0 ? 0.f : (Direction > 0 ? 26.f : -26.f) * (1.f - Smooth(T));
	}
	inline float Pulse(float Seconds)
	{
		const float T = FMath::Clamp((Seconds - RevealSeconds) / .85f, 0.f, 1.f);
		return FMath::Sin(T * PI);
	}
}
