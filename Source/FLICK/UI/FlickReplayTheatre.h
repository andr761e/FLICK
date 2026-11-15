#pragma once
#include "CoreMinimal.h"
class AFlickReplayController;
class SWidget;
TSharedRef<SWidget> MakeFlickReplayTheatre(AFlickReplayController* Controller);

class UFlickReplaySubsystem;
TSharedRef<SWidget> MakeFlickReplayLoading(UFlickReplaySubsystem* Replay);
