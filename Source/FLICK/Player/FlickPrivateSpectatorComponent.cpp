#include "Player/FlickPrivateSpectatorComponent.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Player/FlickCameraPawn.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameState.h"

UFlickPrivateSpectatorComponent::UFlickPrivateSpectatorComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UFlickPrivateSpectatorComponent::IsSpectator() const
{
	const auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
	const auto* Local = Controller ? Controller->GetPlayerState<AFlickPlayerState>() : nullptr;
	return Controller && Controller->IsLocalController() && State && State->bPrivateMatchActive
		&& Local && Local->HasChosenPrivateRole() && Controller->GetLocalTeam() == EFlickTeam::None;
}

EFlickTeam UFlickPrivateSpectatorComponent::GetTargetTeam() const
{
	return TargetSeat == INDEX_NONE ? EFlickTeam::None
		: TargetSeat < 3 ? EFlickTeam::Player1 : EFlickTeam::Player2;
}

const AFlickPlayerState* UFlickPrivateSpectatorComponent::GetTargetPlayer() const
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
	if (!State || TargetSeat == INDEX_NONE) return nullptr;
	for (const APlayerState* Base : State->PlayerArray)
	{
		const auto* Player = Cast<AFlickPlayerState>(Base);
		if (Player && (Player->ControlsPrivateSlot(GetTargetTeam(), GetTargetSlot())
			|| (Player->GetTeam() == GetTargetTeam() && Player->GetTeamPlayerSlot() == GetTargetSlot()))) return Player;
	}
	return nullptr;
}

FString UFlickPrivateSpectatorComponent::GetTargetName() const
{
	if (!bFollowing || TargetSeat == INDEX_NONE) return TEXT("SELECT PLAYER OR BOT");
	if (const auto* Player = GetTargetPlayer(); Player && !Player->GetPlayerName().IsEmpty())
		return Player->GetPlayerName();
	return FString::Printf(TEXT("%s BOT %d"), GetTargetTeam() == EFlickTeam::Player1 ? TEXT("BLUE") : TEXT("ORANGE"), GetTargetSlot() + 1);
}

void UFlickPrivateSpectatorComponent::CycleTarget(const int32 Direction)
{
	const auto* State = GetWorld() ? GetWorld()->GetGameState<AFlickGameState>() : nullptr;
	if (!IsSpectator() || !State || (State->bSeriesComplete && State->MatchPhase == EFlickMatchPhase::RoundOver)) return;
	const auto* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	if (Mode && Mode->GetFrontendScreen() != EFlickFrontendScreen::Playing) return;
	const int32 Count = FMath::Clamp(State->PlayersPerTeam, 1, 3);
	int32 Index = TargetSeat == INDEX_NONE ? INDEX_NONE : (TargetSeat < 3 ? TargetSeat : Count + TargetSeat - 3);
	Index = Index == INDEX_NONE || Index >= Count * 2 ? (Direction < 0 ? Count * 2 - 1 : 0)
		: (Index + (Direction < 0 ? -1 : Direction > 0 ? 1 : 0) + Count * 2) % (Count * 2);
	TargetSeat = Index < Count ? Index : 3 + Index - Count;
	bFollowing = true;
	AppliedSeat = INDEX_NONE;
}

void UFlickPrivateSpectatorComponent::StopFollowing()
{
	bFollowing = false;
	AppliedSeat = INDEX_NONE;
	AppliedPlayer.Reset();
	if (const auto* Controller = Cast<AFlickPlayerController>(GetOwner()))
		if (auto* Camera = Cast<AFlickCameraPawn>(Controller->GetPawn())) Camera->ClearSpectatorView();
}

void UFlickPrivateSpectatorComponent::ServerPublishCamera_Implementation(FFlickSpectatorView View)
{
	if (const auto* Controller = Cast<AFlickPlayerController>(GetOwner()))
		if (auto* Player = Controller->GetPlayerState<AFlickPlayerState>()) Player->SetPrivateCameraView(View);
}

void UFlickPrivateSpectatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	auto* Controller = Cast<AFlickPlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController()) return;
	auto* Camera = Cast<AFlickCameraPawn>(Controller->GetPawn());
	const auto* State = GetWorld()->GetGameState<AFlickGameState>();
	if (!Camera) return;
	if (!State || !State->bPrivateMatchActive || Controller->GetLocalTeam() != EFlickTeam::None)
	{
		if (bFollowing) StopFollowing();
		TargetSeat = INDEX_NONE;
	}
	// Only actual private-match participants publish, never spectators, replays,
	// public matchmaking, or post-match presentations. Unreliable traffic is 10 Hz.
	if (State && State->bPrivateMatchActive && State->IsGameplayActive() && !State->bSeriesComplete
		&& Controller->GetLocalTeam() != EFlickTeam::None && !Controller->IsCinematicReplayPresentationActive())
	{
		PublishElapsed += DeltaTime;
		if (PublishElapsed >= FMath::Max(0.05f, CameraPublishInterval))
		{
			PublishElapsed = 0.0f;
			ServerPublishCamera(Camera->CaptureSpectatorView());
		}
	}
	const auto* Mode = GetWorld()->GetAuthGameMode<AFlickGameMode>();
	if (IsSpectator() && TargetSeat == INDEX_NONE && !Camera->IsFreeCameraEnabled()) CycleTarget(1);
	if (!IsFollowing() || !State || !State->IsGameplayActive() || State->bSeriesComplete
		|| Controller->IsCinematicReplayPresentationActive() || Camera->IsFreeCameraEnabled()
		|| (Mode && Mode->GetFrontendScreen() != EFlickFrontendScreen::Playing))
	{
		Camera->ClearSpectatorView();
		AppliedSeat = INDEX_NONE;
		return;
	}
	if (GetTargetSlot() >= State->PlayersPerTeam) { TargetSeat = INDEX_NONE; CycleTarget(1); }
	const auto* Player = GetTargetPlayer();
	if (AppliedSeat != TargetSeat || AppliedPlayer.Get() != Player)
	{
		Camera->ClearSpectatorView();
		Camera->SetMenuPresentation(false);
		Camera->ResetGameplayView(GetTargetTeam() == EFlickTeam::Player2 ? 2 : 0, true);
		AppliedSeat = TargetSeat;
		AppliedPlayer = Player;
	}
	if (Player && Player->GetPrivateCameraView().bValid)
		Camera->SetSpectatorView(Player->GetPrivateCameraView());
	else Camera->ClearSpectatorView(); // Unoccupied seats are bots: normal team view.
}

void UFlickPrivateSpectatorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	StopFollowing();
	Super::EndPlay(Reason);
}
