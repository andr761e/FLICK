#include "Player/FlickTeamPingComponent.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"
#include "Pieces/FlickPiece.h"
#include "Arena/FlickTestArena.h"
#include "EngineUtils.h"
#include "Audio/FlickAudioDirector.h"
#include "Core/FlickQuickChats.h"

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
	if (!TryStartLocalCooldown()) return;
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
	DeliverTeamMessage(Message);
}

void UFlickTeamPingComponent::DeliverTeamMessage(const FString& Message)
{
	const auto* Sender = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	if (!Sender || !State) return;
	const EFlickTeam Team = Sender->GetLocalTeam();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto* Recipient = Cast<AFlickPlayerController>(It->Get());
		if (!Recipient || Recipient->GetLocalTeam() != Team) continue;
		if (auto* Feed = Recipient->FindComponentByClass<UFlickTeamPingComponent>())
			Feed->ClientReceivePing(State->MatchId, Team, Message);
	}
}

bool UFlickTeamPingComponent::CanUseQuickChat() const
{
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	const auto* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	return Controller && Controller->IsLocalController() && CanPing(State)
		&& Controller->GetLocalTeam() != EFlickTeam::None && !Controller->ShouldShowPrivateTeamMenu()
		&& !Controller->IsCinematicReplayPresentationActive() && !Controller->IsScoreboardVisible()
		&& !GetWorld()->IsPaused() && (!Mode || Mode->GetFrontendScreen() == EFlickFrontendScreen::Playing);
}

int32 UFlickTeamPingComponent::GetQuickChatGroup() const
{
	return CanUseQuickChat() && GetWorld()->GetRealTimeSeconds() - QuickChatOpenedAt < 3.0 ? QuickChatGroup : INDEX_NONE;
}

bool UFlickTeamPingComponent::CancelQuickChat()
{
	const bool bOpen = GetQuickChatGroup() != INDEX_NONE;
	QuickChatGroup = INDEX_NONE; return bOpen;
}

void UFlickTeamPingComponent::QuickChatInput(int32 Choice)
{
	if (Choice < 0 || Choice >= 4 || !CanUseQuickChat()) { CancelQuickChat(); return; }
	const int32 Group = GetQuickChatGroup();
	if (Group == INDEX_NONE) { QuickChatGroup = Choice; QuickChatOpenedAt = GetWorld()->GetRealTimeSeconds(); return; }
	CancelQuickChat();
	if (!TryStartLocalCooldown()) return;
	ServerQuickChat(FlickQuickChats::GetSlot(Group, Choice));
}

bool UFlickTeamPingComponent::TryStartLocalCooldown()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastLocalPing < FMath::Max(.5f, CooldownSeconds))
	{
		const auto* State = GetWorld()->GetGameState<AFlickGameState>();
		const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
		CooldownNoticeMatchId = State ? State->MatchId : FString();
		CooldownNoticeTeam = Controller ? Controller->GetLocalTeam() : EFlickTeam::None;
		return false;
	}
	CooldownNoticeTeam = EFlickTeam::None;
	LastLocalPing = Now;
	return true;
}

FString UFlickTeamPingComponent::GetCooldownNotice() const
{
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	if (!CanUseQuickChat() || !State || !Controller || CooldownNoticeTeam == EFlickTeam::None
		|| CooldownNoticeTeam != Controller->GetLocalTeam() || CooldownNoticeMatchId != State->MatchId) return FString();
	const double Remaining = FMath::Max(.5f, CooldownSeconds) - (GetWorld()->GetTimeSeconds() - LastLocalPing);
	if (Remaining <= 0) return FString();
	// Round up so the notice never reads zero while a send is still blocked.
	return FString::Printf(TEXT("Chat cooldown: %.1f s remaining"), FMath::CeilToDouble(Remaining * 10.0) / 10.0);
}

void UFlickTeamPingComponent::ServerQuickChat_Implementation(const FString& PhraseId)
{
	const auto* Sender = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	const auto* Player = Sender ? Sender->GetPlayerState<AFlickPlayerState>() : nullptr;
	const auto* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	if (!Player || !CanPing(State) || Sender->GetLocalTeam() == EFlickTeam::None
		|| (Mode && (Mode->IsCinematicReplayActive() || Mode->GetFrontendScreen() != EFlickFrontendScreen::Playing))) return;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastServerPing < FMath::Max(.5f, CooldownSeconds)) return;
	LastServerPing = Now;
	const auto* Phrase = FlickQuickChats::Find(PhraseId);
	if (!Phrase) return;
	DeliverTeamMessage(FString::Printf(TEXT("%s: %s"), *CleanName(Player->GetPlayerName()), Phrase->Text));
}

void UFlickTeamPingComponent::ClientReceivePing_Implementation(const FString& MatchId, EFlickTeam Team, const FString& Message)
{
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	if (!Controller || !State || Team == EFlickTeam::None || Controller->GetLocalTeam() != Team || State->MatchId != MatchId) return;
	Messages.RemoveAll([State, Team](const FMessage& Entry)
	{ return Entry.MatchId != State->MatchId || Entry.Team != Team; });
	Messages.Add({MatchId, Team, Message.Left(160), GetWorld()->GetTimeSeconds()});
	if (Messages.Num() > MaximumMessages) Messages.RemoveAt(0, Messages.Num() - MaximumMessages);
	// Retain history after fading. A new message brings the latest five back together.
	for (FMessage& Entry : Messages) Entry.ReceivedAt = GetWorld()->GetTimeSeconds();
	if (Controller->IsLocalController())
		for (TActorIterator<AFlickAudioDirector> Audio(GetWorld()); Audio; ++Audio)
		{
			Audio->PlayLocalNotification(false);
			break;
		}
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
