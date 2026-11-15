#pragma once
#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "FlickProgression.generated.h"

// Only the authority fills match events. Reward events use deterministic IDs.
USTRUCT()
struct FFlickCareerEvent
{
 GENERATED_BODY()
 UPROPERTY() FString Id;
 UPROPERTY() int32 Kind = 0; // 0 match, 1 practice, 2 tutorial, 3 challenge, 4 historical snapshot
 UPROPERTY() int32 Playlist = 0; // 0 casual, 1 ranked, 2 bots (never a rank)
 UPROPERTY() int32 Format = 0; // 0/1/2 Knockout 1v1/2v2/3v3, 3 BOB
 UPROPERTY() bool bEligible = false;
 UPROPERTY() bool bWon = false;
 UPROPERTY() bool bDraw = false;
 UPROPERTY() int32 Seconds = 0;
 UPROPERTY() int32 Shots = 0;
 UPROPERTY() int32 Points = 0;
 UPROPERTY() int32 Knockouts = 0;
 UPROPERTY() int32 DoubleKnockouts = 0;
 UPROPERTY() int32 SelfKnockouts = 0;
 UPROPERTY() int32 Switches = 0;
 UPROPERTY() int32 Pockets = 0;
 UPROPERTY() int32 Fouls = 0;
 UPROPERTY() int32 MultiPockets = 0;
 UPROPERTY() TArray<int32> Accolades;
 UPROPERTY() int32 CompletionXP = 0;
 UPROPERTY() int32 TimeXP = 0;
 UPROPERTY() int32 WinXP = 0;
 UPROPERTY() int32 StyleXP = 0;
 UPROPERTY() int32 BonusXP = 0;
 int32 XP() const { return CompletionXP + TimeXP + WinXP + StyleXP + BonusXP; }
};

namespace FlickProgression
{
 struct FLevel { int32 Number = 1; int32 Progress = 0; int32 Required = 1000; float Fraction() const { return float(Progress)/Required; } };
 FLICK_API FLevel Level(int64 XP);
 FLICK_API void RewardMatch(FFlickCareerEvent& Event);
}
