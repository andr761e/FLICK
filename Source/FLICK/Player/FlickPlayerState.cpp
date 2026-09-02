#include "Player/FlickPlayerState.h"

#include "Net/UnrealNetwork.h"

AFlickPlayerState::AFlickPlayerState()
{
	bReplicates = true;
}

void AFlickPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFlickPlayerState, Team);
	DOREPLIFETIME(AFlickPlayerState, TeamPlayerSlot);
	DOREPLIFETIME(AFlickPlayerState, bLobbyReady);
	DOREPLIFETIME(AFlickPlayerState, NetworkSelectedClass);
	DOREPLIFETIME(AFlickPlayerState, bNetworkClassConfirmed);
	DOREPLIFETIME(AFlickPlayerState, bPartyLeader);
	DOREPLIFETIME(AFlickPlayerState, PartySlot);
	DOREPLIFETIME(AFlickPlayerState, PartyId);
	DOREPLIFETIME(AFlickPlayerState, bRankedIdentityVerified);
	DOREPLIFETIME(AFlickPlayerState, AuthoritativeRankedRating);
	DOREPLIFETIME(AFlickPlayerState, VerifiedOnlineAccountId);
	DOREPLIFETIME(AFlickPlayerState, PrivateControlledSlots);
}

void AFlickPlayerState::ResetNetworkClassSelection(const EFlickLineupPreset InClass)
{
	if (HasAuthority())
	{
		NetworkSelectedClass = InClass == EFlickLineupPreset::Custom
			? EFlickLineupPreset::Balanced
			: InClass;
		bNetworkClassConfirmed = false;
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetNetworkSelectedClass(const EFlickLineupPreset InClass)
{
	if (HasAuthority() && !bNetworkClassConfirmed && InClass != EFlickLineupPreset::Custom)
	{
		NetworkSelectedClass = InClass;
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetNetworkClassConfirmed(const bool bInConfirmed)
{
	if (HasAuthority())
	{
		bNetworkClassConfirmed = bInConfirmed;
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetPrivateControlledSlots(const TArray<int32>& InControlledSlots)
{
	if (!HasAuthority())
	{
		return;
	}
	PrivateControlledSlots.Reset();
	for (const int32 EncodedSlot : InControlledSlots)
	{
		if (EncodedSlot >= 0 && EncodedSlot < 6)
		{
			PrivateControlledSlots.AddUnique(EncodedSlot);
		}
	}
	PrivateControlledSlots.Sort();
	ForceNetUpdate();
}

void AFlickPlayerState::ClearPrivateControlledSlots()
{
	if (HasAuthority() && !PrivateControlledSlots.IsEmpty())
	{
		PrivateControlledSlots.Reset();
		ForceNetUpdate();
	}
}

bool AFlickPlayerState::ControlsPrivateSlot(
	const EFlickTeam InTeam,
	const int32 InPlayerSlot) const
{
	return PrivateControlledSlots.Contains(EncodePrivatePlayerSlot(InTeam, InPlayerSlot));
}

void AFlickPlayerState::SetRankedIdentity(
	const bool bVerified,
	const int32 InAuthoritativeRating)
{
	if (HasAuthority())
	{
		bRankedIdentityVerified = bVerified;
		AuthoritativeRankedRating = FMath::Clamp(
			InAuthoritativeRating,
			0,
			3000);
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetVerifiedOnlineAccountId(const FString& AccountId)
{
	if (HasAuthority())
	{
		VerifiedOnlineAccountId = AccountId.Left(128);
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetTeamPlayerSlot(const int32 InTeamPlayerSlot)
{
	if (HasAuthority())
	{
		TeamPlayerSlot = InTeamPlayerSlot;
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetLobbyReady(const bool bInReady)
{
	if (HasAuthority())
	{
		bLobbyReady = bInReady;
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetTeam(const EFlickTeam InTeam)
{
	if (HasAuthority())
	{
		Team = InTeam;
	}
}

void AFlickPlayerState::SetPartyRole(const bool bInLeader, const int32 InPartySlot)
{
	if (HasAuthority())
	{
		bPartyLeader = bInLeader;
		PartySlot = InPartySlot;
		ForceNetUpdate();
	}
}

void AFlickPlayerState::SetPartyIdentity(
	const FString& InPartyId,
	const bool bInLeader,
	const int32 InPartySlot)
{
	if (HasAuthority())
	{
		PartyId = InPartyId;
		bPartyLeader = bInLeader;
		PartySlot = InPartySlot;
		ForceNetUpdate();
	}
}
