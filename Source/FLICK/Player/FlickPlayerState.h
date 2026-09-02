#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "GameFramework/PlayerState.h"
#include "FlickPlayerState.generated.h"

UCLASS()
class FLICK_API AFlickPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AFlickPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetTeam(EFlickTeam InTeam);
	EFlickTeam GetTeam() const { return Team; }
	void SetTeamPlayerSlot(int32 InTeamPlayerSlot);
	int32 GetTeamPlayerSlot() const { return TeamPlayerSlot; }
	void SetLobbyReady(bool bInReady);
	bool IsLobbyReady() const { return bLobbyReady; }
	void ResetNetworkClassSelection(EFlickLineupPreset InClass = EFlickLineupPreset::Balanced);
	void SetNetworkSelectedClass(EFlickLineupPreset InClass);
	void SetNetworkClassConfirmed(bool bInConfirmed);
	EFlickLineupPreset GetNetworkSelectedClass() const { return NetworkSelectedClass; }
	bool IsNetworkClassConfirmed() const { return bNetworkClassConfirmed; }
	void SetPartyRole(bool bInLeader, int32 InPartySlot);
	void SetPartyIdentity(const FString& InPartyId, bool bInLeader, int32 InPartySlot);
	bool IsPartyLeader() const { return bPartyLeader; }
	int32 GetPartySlot() const { return PartySlot; }
	const FString& GetPartyId() const { return PartyId; }
	void SetRankedIdentity(bool bVerified, int32 InAuthoritativeRating);
	bool IsRankedIdentityVerified() const { return bRankedIdentityVerified; }
	int32 GetAuthoritativeRankedRating() const { return AuthoritativeRankedRating; }
	void SetVerifiedOnlineAccountId(const FString& AccountId);
	const FString& GetVerifiedOnlineAccountId() const { return VerifiedOnlineAccountId; }
	void SetPrivateControlledSlots(const TArray<int32>& InControlledSlots);
	void ClearPrivateControlledSlots();
	bool ControlsPrivateSlot(EFlickTeam InTeam, int32 InPlayerSlot) const;
	const TArray<int32>& GetPrivateControlledSlots() const { return PrivateControlledSlots; }

private:
	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Player")
	EFlickTeam Team = EFlickTeam::None;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Player")
	int32 TeamPlayerSlot = INDEX_NONE;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Player")
	bool bLobbyReady = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Player|Class")
	EFlickLineupPreset NetworkSelectedClass = EFlickLineupPreset::Balanced;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Player|Class")
	bool bNetworkClassConfirmed = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Party")
	bool bPartyLeader = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Party")
	int32 PartySlot = INDEX_NONE;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Party")
	FString PartyId;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Ranked")
	bool bRankedIdentityVerified = false;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Ranked")
	int32 AuthoritativeRankedRating = 1000;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Online")
	FString VerifiedOnlineAccountId;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "FLICK|Private Match")
	TArray<int32> PrivateControlledSlots;
};
