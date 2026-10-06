#include "Player/FlickTeamPingComponent.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Arena/FlickTestArena.h"
#include "EngineUtils.h"

namespace
{
	bool CanPing(const AFlickGameState* State)
	{
		return State && !State->bSeriesComplete && !State->bNetworkClassSelectionActive
			&& !State->bPrivateMatchAssignmentActive && (State->MatchPhase == EFlickMatchPhase::Aiming
			|| State->MatchPhase == EFlickMatchPhase::KickoffPlanning || State->MatchPhase == EFlickMatchPhase::ResolvingPhysics);
	}
	FString CleanName(FString Name)
	{
		Name.ReplaceInline(TEXT("\r"), TEXT(" ")); Name.ReplaceInline(TEXT("\n"), TEXT(" "));
		return Name.Len() > 28 ? Name.Left(25) + TEXT("...") : Name;
	}
	const AFlickTestArena* PingArena(UWorld* World)
	{
		if (const auto* Mode = World->GetAuthGameMode<AFlickGameMode>()) return Mode->GetTestArena();
		for (TActorIterator<AFlickTestArena> It(World); It; ++It) return *It;
		return nullptr;
	}
}

UFlickTeamPingComponent::UFlickTeamPingComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UFlickTeamPingComponent::TryPing(AFlickPiece* Piece, const int32 SwitchIndex)
{
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	const auto* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	if (!Controller || !Controller->IsLocalController() || !CanPing(State)
		|| Controller->GetLocalTeam() == EFlickTeam::None || Controller->ShouldShowPrivateTeamMenu()
		|| Controller->IsCinematicReplayPresentationActive() || Controller->IsFreeCameraActive()
		|| Controller->IsScoreboardVisible() || (Mode && Mode->GetFrontendScreen() != EFlickFrontendScreen::Playing)) return;
	if (Piece)
	{
		if (!Piece->IsActive() || Piece->GetTeam() == EFlickTeam::None || Piece->GetTeam() == Controller->GetLocalTeam()) return;
	}
	else
	{
		const auto* Arena = PingArena(GetWorld());
		if (!Arena || SwitchIndex < 0 || SwitchIndex >= Arena->GetMechanismCount()) return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastLocalPing < FMath::Max(.5f, CooldownSeconds)) return;
	LastLocalPing = Now;
	ServerPingTarget(Piece ? Piece->GetPieceId() : INDEX_NONE, Piece ? INDEX_NONE : SwitchIndex);
}

void UFlickTeamPingComponent::ServerPingTarget_Implementation(const int32 PieceId, const int32 SwitchIndex)
{
	auto* Sender = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	const auto* SenderState = Sender ? Sender->GetPlayerState<AFlickPlayerState>() : nullptr;
	const auto* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	if (!Sender || !SenderState || !CanPing(State) || Sender->GetLocalTeam() == EFlickTeam::None
		|| (Mode && (Mode->IsCinematicReplayActive() || Mode->GetFrontendScreen() != EFlickFrontendScreen::Playing))) return;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastServerPing < FMath::Max(.5f, CooldownSeconds)) return;
	// Also throttle invalid target requests. No client-provided name/team/text is trusted.
	LastServerPing = Now;
	const EFlickTeam Team = Sender->GetLocalTeam();
	FString Message;
	if (SwitchIndex != INDEX_NONE)
	{
		const auto* Arena = PingArena(GetWorld());
		if (PieceId != INDEX_NONE || !Arena || SwitchIndex < 0 || SwitchIndex >= Arena->GetMechanismCount()) return;
		Message = FString::Printf(TEXT("%s: Switch %s"), *CleanName(SenderState->GetPlayerName()), *Arena->GetDividerLabel(SwitchIndex));
	}
	else
	{
		AFlickPiece* Target = nullptr;
		for (TActorIterator<AFlickPiece> It(GetWorld()); It; ++It)
			if (It->GetPieceId() == PieceId && It->IsActive()) { Target = *It; break; }
		if (!Target || Target->GetTeam() == EFlickTeam::None || Target->GetTeam() == Team) return;
		FString TargetName = FString::Printf(TEXT("%s %s %d"), Target->GetTeam() == EFlickTeam::Player1 ? TEXT("Blue") : TEXT("Orange"),
			State->bPrivateMatchActive ? TEXT("Bot") : TEXT("Player"), Target->GetOwningPlayerSlot() + 1);
		for (const APlayerState* Base : State->PlayerArray)
		{
			const auto* Player = Cast<AFlickPlayerState>(Base);
			if (Player && ((Player->GetTeam() == Target->GetTeam() && Player->GetTeamPlayerSlot() == Target->GetOwningPlayerSlot())
				|| Player->ControlsPrivateSlot(Target->GetTeam(), Target->GetOwningPlayerSlot())))
			{
				TargetName = Player->GetPlayerName(); break;
			}
		}
		Message = FString::Printf(TEXT("%s: Target %s - %s"), *CleanName(SenderState->GetPlayerName()),
			*CleanName(TargetName), *GetPieceArchetypeName(Target->GetArchetype()));
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto* Recipient = Cast<AFlickPlayerController>(It->Get());
		if (!Recipient || Recipient->GetLocalTeam() != Team) continue;
		if (auto* Feed = Recipient->FindComponentByClass<UFlickTeamPingComponent>())
			Feed->ClientReceivePing(State->MatchId, Team, Message);
	}
}

void UFlickTeamPingComponent::ClientReceivePing_Implementation(const FString& MatchId, EFlickTeam Team, const FString& Message)
{
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	if (!Controller || !State || Team == EFlickTeam::None || Controller->GetLocalTeam() != Team || State->MatchId != MatchId) return;
	Messages.RemoveAll([this, State](const FMessage& Entry)
	{ return Entry.MatchId != State->MatchId || GetWorld()->GetTimeSeconds() - Entry.ReceivedAt >= MessageLifetime; });
	Messages.Add({MatchId, Team, Message.Left(160), GetWorld()->GetTimeSeconds()});
	if (Messages.Num() > MaximumMessages) Messages.RemoveAt(0, Messages.Num() - MaximumMessages);
}

float UFlickTeamPingComponent::GetOpacity(const int32 Row) const
{
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	if (!Messages.IsValidIndex(Row) || !Controller || !CanPing(State)) return 0;
	const FMessage& Entry = Messages[Row];
	if (Entry.Team != Controller->GetLocalTeam() || Entry.MatchId != State->MatchId) return 0;
	const double Age = GetWorld()->GetTimeSeconds() - Entry.ReceivedAt;
	return FMath::Clamp(static_cast<float>((MessageLifetime - Age) / FMath::Clamp(FadeSeconds, .1f, MessageLifetime)), 0.f, 1.f);
}

FString UFlickTeamPingComponent::GetMessage(int32 Row) const { return GetOpacity(Row) > 0 ? Messages[Row].Text : FString(); }
EFlickTeam UFlickTeamPingComponent::GetMessageTeam(int32 Row) const { return Messages.IsValidIndex(Row) ? Messages[Row].Team : EFlickTeam::None; }
bool UFlickTeamPingComponent::HasMessages() const { for (int32 Row = 0; Row < Messages.Num(); ++Row) if (GetOpacity(Row) > 0) return true; return false; }
