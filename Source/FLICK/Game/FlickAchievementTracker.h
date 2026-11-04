#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/FlickAchievementProgress.h"
#include "Core/FlickTypes.h"
#include "FlickAchievementTracker.generated.h"

class AFlickPiece;
class UPrimitiveComponent;

// Authority-only observation of existing physics and match events. No tick,
// force, collision response, or client-predicted outcome is introduced.
UCLASS()
class FLICK_API UFlickAchievementTracker : public UActorComponent
{
    GENERATED_BODY()
public:
    UFlickAchievementTracker();
    void StartMatch();
    void StartRound();
    void BeginResolution();
    void ShotLaunched(AFlickPiece* Piece);
    void ObserveTurn(EFlickTeam Team);
    void OpponentKnockedOut(AFlickPiece* Piece, EFlickTeam CreditedTeam, int32 Slot);
    void ArenaContact(AFlickPiece* Piece, UPrimitiveComponent* Component);
    void BobPocket(AFlickPiece* Piece, int32 PocketIndex);
    void SwitchActivated(AFlickPiece* Piece);
    void ResolutionFinished();
    void RoundFinished();
private:
    friend class FFlickAchievementGameplayTest;
    struct FPlayer
    {
        FFlickAchievementReport Report;
        int32 Shots = 0;
        uint8 Corners = 0;
        bool bStrikerPocketed = false, bTrailedZeroTwo = false;
    };
    struct FShot
    {
        int32 PieceId = INDEX_NONE;
        EFlickPieceArchetype Type = EFlickPieceArchetype::Standard;
        int32 Knockouts = 0, OwnCoins = 0;
    };
    static int32 Key(EFlickTeam Team, int32 Slot) { return (static_cast<int32>(Team) - 1) * 3 + Slot; }
    void Add(int32 Player, FlickAchievements::EId Id, int32 Amount = 1);
    bool IsLive() const;
    TMap<int32, FPlayer> Players;
    TMap<int32, FShot> Shots;
    TSet<int32> BankedCoins, DividerShots;
    TSet<int32> ScoredPieces;
    int32 LastCompletedRound = INDEX_NONE;
    int32 RoundStartingCount[2] = {};
    bool ComebackEligible[2] = {};
    bool bEnabled = false, bResolutionFinished = true, bMatchReported = false;
    EFlickTeam LastBankedFinalCoinTeam = EFlickTeam::None;
};
