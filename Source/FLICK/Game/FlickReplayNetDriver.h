#pragma once

#include "CoreMinimal.h"
#include "Engine/DemoNetDriver.h"
#include "FlickReplayNetDriver.generated.h"

// FLICK arenas contain only a few dozen replicated actors. Finish a scrub's
// checkpoint atomically: deferred actor spawning would leave a paused frame
// missing pucks after the native seek-complete callback has already fired.
UCLASS(Transient)
class FLICK_API UFlickReplayNetDriver : public UDemoNetDriver
{
	GENERATED_BODY()
public:
	virtual bool ShouldQueueBunchesForActorGUID(FNetworkGUID InGUID) const override { return false; }
};
