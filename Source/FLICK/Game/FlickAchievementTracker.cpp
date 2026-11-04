#include "Game/FlickAchievementTracker.h"
#include "Game/FlickGameModePrivate.h"
#include "Misc/App.h"


UFlickAchievementTracker::UFlickAchievementTracker() { PrimaryComponentTick.bCanEverTick = false; }

bool UFlickAchievementTracker::IsLive() const
{
    const auto* Mode = Cast<AFlickGameMode>(GetOwner());
    return bEnabled && Mode && !Mode->bCinematicReplayActive && Mode->FrontendScreen == EFlickFrontendScreen::Playing;
}

void UFlickAchievementTracker::Add(int32 Player, FlickAchievements::EId Id, int32 Amount)
{
    if (auto* P = Players.Find(Player))
    {
        auto& V = P->Report.Values[FlickAchievements::Index(Id)];
        V = static_cast<int32>(FMath::Clamp<int64>(static_cast<int64>(V) + FMath::Max(0, Amount), 0, FlickAchievements::Get(Id).Goal));
    }
}

void UFlickAchievementTracker::StartMatch()
{
    auto* Mode = Cast<AFlickGameMode>(GetOwner());
    Players.Reset(); Shots.Reset(); bMatchReported = false; LastCompletedRound = INDEX_NONE;
    bEnabled = Mode && Mode->HasAuthority() && !Mode->IsTutorialMode() && !Mode->IsFreePlayTraining()
        && Mode->FrontendScreen == EFlickFrontendScreen::Playing && !FApp::IsUnattended() && !IsRunningCommandlet();
    if (!bEnabled) return;
    const FGuid Event = FGuid::NewGuid();
    for (int32 T = 1; T <= 2; ++T)
        for (int32 Slot = 0; Slot < Mode->CurrentPlayersPerTeam; ++Slot)
            Players.FindOrAdd(Key(static_cast<EFlickTeam>(T), Slot)).Report.EventId = Event;
    StartRound();
}

void UFlickAchievementTracker::StartRound()
{
    if (!bEnabled) return;
    auto* Mode = CastChecked<AFlickGameMode>(GetOwner());
    auto* State = Mode->GetFlickGameState();
    ComebackEligible[0] = ComebackEligible[1] = false;
    for (int32 T = 1; T <= 2; ++T)
        RoundStartingCount[T - 1] = Mode->CountActivePieces(static_cast<EFlickTeam>(T));
    if (State && State->RoundNumber == 1 && !Mode->IsBobMode())
        for (const AFlickPiece* Piece : Mode->Pieces)
            if (IsValid(Piece) && Piece->IsActive())
                if (auto* P = Players.Find(Key(Piece->GetTeam(), Piece->GetOwningPlayerSlot())))
                    P->Report.ArchetypeMask |= 1 << static_cast<int32>(Piece->GetArchetype());
}

void UFlickAchievementTracker::BeginResolution()
{
    Shots.Reset(); BankedCoins.Reset(); DividerShots.Reset(); ScoredPieces.Reset();
    bResolutionFinished = false; LastBankedFinalCoinTeam = EFlickTeam::None;
}

void UFlickAchievementTracker::ShotLaunched(AFlickPiece* Piece)
{
    if (!IsLive() || !IsValid(Piece)) return;
    auto* Mode = CastChecked<AFlickGameMode>(GetOwner());
    const auto Team = Piece->GetTeam(); const int32 K = Key(Team, Piece->GetOwningPlayerSlot());
    if (auto* P = Players.Find(K)) ++P->Shots; else return;
    FShot& Shot = Shots.FindOrAdd(K); Shot.PieceId = Piece->GetPieceId(); Shot.Type = Piece->GetArchetype();
    Add(K, FlickAchievements::EId::FirstFlick); Add(K, FlickAchievements::EId::FindYourAngle);
    if (!Mode->IsBobMode())
    {
        if (Shot.Type == EFlickPieceArchetype::Slider) Add(K, FlickAchievements::EId::Slipstream);
        ObserveTurn(Team);
    }
}

void UFlickAchievementTracker::ObserveTurn(const EFlickTeam Team)
{
    if (!IsLive()) return;
    const auto* Mode = CastChecked<AFlickGameMode>(GetOwner());
    const int32 T = static_cast<int32>(Team) - 1;
    if (!Mode->IsBobMode() && T >= 0 && T < 2 && Mode->CountActivePieces(Team) == 1
        && Mode->CountActivePieces(Team == EFlickTeam::Player1 ? EFlickTeam::Player2 : EFlickTeam::Player1) >= 3)
        ComebackEligible[T] = true;
}

void UFlickAchievementTracker::OpponentKnockedOut(AFlickPiece* Piece, EFlickTeam Team, int32 Slot)
{
    if (!IsLive() || !IsValid(Piece) || ScoredPieces.Contains(Piece->GetPieceId())) return;
    const int32 K = Key(Team, Slot);
    auto* Shot = Shots.Find(K);
    if (!Shot || Piece->GetTeam() == Team) return;
    ScoredPieces.Add(Piece->GetPieceId());
    ++Shot->Knockouts; Add(K, FlickAchievements::EId::TableControl);
    if (Shot->Type == EFlickPieceArchetype::Striker) Add(K, FlickAchievements::EId::StrikerForce);
    if (Shot->Type == EFlickPieceArchetype::Toppler) Add(K, FlickAchievements::EId::ToppleTheOdds);
    if (Shot->Type == EFlickPieceArchetype::Compact && Piece->GetArchetype() == EFlickPieceArchetype::Heavy) Add(K, FlickAchievements::EId::SmallButMighty);
    if (Shot->Type == EFlickPieceArchetype::Bouncer && DividerShots.Contains(Shot->PieceId)) Add(K, FlickAchievements::EId::BounceHouse);
}

void UFlickAchievementTracker::ArenaContact(AFlickPiece* Piece, UPrimitiveComponent* Component)
{
    if (!IsLive() || bResolutionFinished || !IsValid(Piece) || !Component) return;
    auto* Mode = CastChecked<AFlickGameMode>(GetOwner());
    int32 Divider = INDEX_NONE;
    if (Mode->IsBobMode() && Component->GetOwner() == Mode->BobArenaActor
        && Component->GetName().StartsWith(TEXT("Rail_"))) BankedCoins.Add(Piece->GetPieceId());
    else if (Mode->TestArenaActor && Mode->TestArenaActor->FindDividerIndex(Component, Divider)
        && Mode->TestArenaActor->IsDividerRaised(Divider)) DividerShots.Add(Piece->GetPieceId());
}

void UFlickAchievementTracker::BobPocket(AFlickPiece* Piece, int32 PocketIndex)
{
    if (!IsLive() || !IsValid(Piece) || ScoredPieces.Contains(Piece->GetPieceId())) return;
    const int32 K = Key(Piece->GetTeam(), 0);
    auto* P = Players.Find(K); if (!P) return;
    ScoredPieces.Add(Piece->GetPieceId());
    if (Piece->IsBobStriker()) { P->bStrikerPocketed = true; return; }
    Add(K, FlickAchievements::EId::InThePocket); Add(K, FlickAchievements::EId::PocketCollection);
    if (PocketIndex >= 0 && PocketIndex < 4) P->Corners |= 1 << PocketIndex;
    int32 N = 0; for (uint8 Mask = P->Corners; Mask; Mask &= Mask - 1) ++N;
    P->Report.Values[FlickAchievements::Index(FlickAchievements::EId::CornerOffice)] = N;
    const bool bBanked = BankedCoins.Contains(Piece->GetPieceId());
    if (bBanked) Add(K, FlickAchievements::EId::BankStatement);
    // Only this team's shot can earn a multi-pocket/opening-shot achievement.
    if (auto* Shot = Shots.Find(K))
    {
        ++Shot->OwnCoins;
        if (bBanked) LastBankedFinalCoinTeam = Piece->GetTeam();
        else LastBankedFinalCoinTeam = EFlickTeam::None;
    }
}

void UFlickAchievementTracker::SwitchActivated(AFlickPiece* Piece)
{
    if (IsLive() && IsValid(Piece)) Add(Key(Piece->GetTeam(), Piece->GetOwningPlayerSlot()), FlickAchievements::EId::Switchboard);
}

void UFlickAchievementTracker::ResolutionFinished()
{
    if (!IsLive() || bResolutionFinished) return;
    bResolutionFinished = true;
    auto* Mode = CastChecked<AFlickGameMode>(GetOwner());
    for (const auto& Pair : Shots)
    {
        const auto& S = Pair.Value;
        if (!Mode->IsBobMode())
        {
            if (S.Knockouts >= 2) Add(Pair.Key, FlickAchievements::EId::DoubleTrouble);
            if (S.Knockouts >= 3) Add(Pair.Key, FlickAchievements::EId::TripleThreat);
            if (S.Knockouts >= 2 && S.Type == EFlickPieceArchetype::Heavy) Add(Pair.Key, FlickAchievements::EId::HeavyHitter);
        }
        else
        {
            if (S.OwnCoins >= 2) { Add(Pair.Key, FlickAchievements::EId::DoubleDeposit); if (Players[Pair.Key].Shots == 1) Add(Pair.Key, FlickAchievements::EId::Breakthrough); }
            if (S.OwnCoins >= 3) Add(Pair.Key, FlickAchievements::EId::ThreePointPlan);
        }
    }
}

void UFlickAchievementTracker::RoundFinished()
{
    if (!IsLive() || bMatchReported) return;
    auto* Mode = CastChecked<AFlickGameMode>(GetOwner());
    auto* State = Mode->GetFlickGameState(); if (!State) return;
    if (LastCompletedRound == State->RoundNumber) return;
    LastCompletedRound = State->RoundNumber;
    const EFlickTeam Winner = State->WinnerTeam;
    const bool bBob = Mode->IsBobMode();
    for (auto& Pair : Players)
    {
        const auto Team = static_cast<EFlickTeam>(Pair.Key / 3 + 1);
        const int32 T = static_cast<int32>(Team) - 1;
        if (State->RoundsToWin == 3 && (T == 0 ? State->Player1RoundsWon == 0 && State->Player2RoundsWon == 2 : State->Player2RoundsWon == 0 && State->Player1RoundsWon == 2)) Pair.Value.bTrailedZeroTwo = true;
        if (Winner != Team || State->bDraw) continue;
        if (!bBob)
        {
            if (Mode->CountActivePieces(Team) == RoundStartingCount[T]) Add(Pair.Key, FlickAchievements::EId::HoldTheLine);
            if (ComebackEligible[T]) Add(Pair.Key, FlickAchievements::EId::LastPuckStanding);
            if (Mode->CountActivePieces(Team) == 1)
                for (const AFlickPiece* Piece : Mode->Pieces)
                    if (IsValid(Piece) && Piece->IsActive() && Piece->GetTeam() == Team && Piece->GetArchetype() == EFlickPieceArchetype::Grippy) Add(Pair.Key, FlickAchievements::EId::StayingPower);
        }
        if (!State->bSeriesComplete) continue;
        Add(Pair.Key, FlickAchievements::EId::CenturyClub); Add(Pair.Key, FlickAchievements::EId::ArenaVeteran);
        if (bBob)
        {
            Add(Pair.Key, FlickAchievements::EId::PocketPro);
            if (!Pair.Value.bStrikerPocketed) Add(Pair.Key, FlickAchievements::EId::CleanSweep);
            if (LastBankedFinalCoinTeam == Team) Add(Pair.Key, FlickAchievements::EId::FinishWithFlair);
        }
        else
        {
            Add(Pair.Key, FlickAchievements::EId::FirstRivalry);
            if (Pair.Value.bTrailedZeroTwo) Add(Pair.Key, FlickAchievements::EId::MomentumShift);
            if (Mode->CurrentPlayersPerTeam == 2) Add(Pair.Key, FlickAchievements::EId::SquadGoals);
            if (Mode->CurrentPlayersPerTeam == 3) Add(Pair.Key, FlickAchievements::EId::ThreesCompany);
            if (Pair.Value.Report.ArchetypeMask & (1 << static_cast<int32>(EFlickPieceArchetype::Blocker))) Add(Pair.Key, FlickAchievements::EId::BuiltToLast);
            if (Pair.Value.Report.ArchetypeMask & (1 << static_cast<int32>(EFlickPieceArchetype::Standard))) Add(Pair.Key, FlickAchievements::EId::StandardBearer);
            if (!Mode->bResolutionWasSimultaneous && State->LastShotTeam == Team && State->GetLastShootingPlayerSlot(Team) == Pair.Key % 3) Add(Pair.Key, FlickAchievements::EId::DecisiveTouch);
            if (const auto* Stats = State->FindPlayerMatchStats(Team, Pair.Key % 3); Stats && Mode->CurrentPlayersPerTeam > 1 && Stats->SelfKnockouts == 0) Add(Pair.Key, FlickAchievements::EId::SteadyTeammate);
        }
    }
    if (!State->bSeriesComplete) return;
    bMatchReported = true;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (auto* PC = Cast<AFlickPlayerController>(It->Get()))
            if (auto* PS = PC->GetPlayerState<AFlickPlayerState>(); PS && PS->GetTeam() != EFlickTeam::None)
                if (auto* P = Players.Find(Key(PS->GetTeam(), PS->GetTeamPlayerSlot())))
                {
                    if (const auto* Stats = State->FindPlayerMatchStats(PS->GetTeam(), PS->GetTeamPlayerSlot())) Add(Key(PS->GetTeam(), PS->GetTeamPlayerSlot()), FlickAchievements::EId::OnTheBoard, Stats->Score);
                    PC->ClientCommitAchievements(P->Report, State->bRankedMatch && Winner == PS->GetTeam(), State->ActiveMatchVariant, Mode->CurrentPlayersPerTeam);
                }
}
