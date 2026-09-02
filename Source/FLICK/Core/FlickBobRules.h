#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"

namespace FlickBobRules
{
	constexpr int32 PucksPerTeam = 12;

	FLICK_API EFlickMatchOutcome EvaluateOutcome(
		int32 Player1Remaining,
		int32 Player2Remaining,
		EFlickTeam ShootingTeam);
}
