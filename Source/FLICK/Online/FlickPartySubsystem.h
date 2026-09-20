#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlickPartySubsystem.generated.h"

struct FFlickPartyMember
{
	FString UserId;
	FString DisplayName;
	int32 Slot = INDEX_NONE;
	bool bLeader = false;
};

DECLARE_MULTICAST_DELEGATE(FOnFlickPartyChanged);

/**
 * Persistent, world-independent party presentation state. Steam owns the
 * membership; this subsystem gives every local frontend a stable model without
 * requiring the players to share an Unreal world.
 */
UCLASS()
class FLICK_API UFlickPartySubsystem final : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void Synchronize(
		const FString& InPartyId,
		const FString& InLeaderUserId,
		const TArray<TPair<FString, FString>>& InMembers,
		const FString& InLocalUserId);
	void Clear();

	bool IsActive() const { return !PartyId.IsEmpty() && !Members.IsEmpty(); }
	bool IsLocalLeader() const { return IsActive() && LocalUserId == LeaderUserId; }
	const FString& GetPartyId() const { return PartyId; }
	const FString& GetLeaderUserId() const { return LeaderUserId; }
	const FString& GetLocalUserId() const { return LocalUserId; }
	int32 GetMemberCount() const { return Members.Num(); }
	const TArray<FFlickPartyMember>& GetMembers() const { return Members; }
	const FFlickPartyMember* GetMemberBySlot(int32 Slot) const;
	const FFlickPartyMember* GetMemberByUserId(const FString& UserId) const;

	FOnFlickPartyChanged OnPartyChanged;

private:
	FString PartyId;
	FString LeaderUserId;
	FString LocalUserId;
	TArray<FFlickPartyMember> Members;
};
