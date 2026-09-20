#include "Online/FlickPartySubsystem.h"

#include "Algo/Compare.h"

void UFlickPartySubsystem::Synchronize(
	const FString& InPartyId,
	const FString& InLeaderUserId,
	const TArray<TPair<FString, FString>>& InMembers,
	const FString& InLocalUserId)
{
	TMap<FString, int32> ExistingSlots;
	for (const FFlickPartyMember& Member : Members)
	{
		ExistingSlots.Add(Member.UserId, Member.Slot);
	}

	TArray<FFlickPartyMember> UpdatedMembers;
	TSet<int32> UsedSlots;
	for (const TPair<FString, FString>& Source : InMembers)
	{
		if (Source.Key.IsEmpty() || UpdatedMembers.ContainsByPredicate(
			[&Source](const FFlickPartyMember& Existing) { return Existing.UserId == Source.Key; }))
		{
			continue;
		}
		FFlickPartyMember& Member = UpdatedMembers.AddDefaulted_GetRef();
		Member.UserId = Source.Key;
		Member.DisplayName = Source.Value.IsEmpty() ? TEXT("STEAM PLAYER") : Source.Value;
		Member.bLeader = Source.Key == InLeaderUserId;
		if (const int32* ExistingSlot = ExistingSlots.Find(Source.Key))
		{
			Member.Slot = *ExistingSlot;
			UsedSlots.Add(Member.Slot);
		}
	}

	for (FFlickPartyMember& Member : UpdatedMembers)
	{
		if (Member.Slot != INDEX_NONE)
		{
			continue;
		}
		for (int32 Slot = 0; Slot < FlickMaximumPartyMembers; ++Slot)
		{
			if (!UsedSlots.Contains(Slot))
			{
				Member.Slot = Slot;
				UsedSlots.Add(Slot);
				break;
			}
		}
	}
	UpdatedMembers.Sort([](const FFlickPartyMember& Left, const FFlickPartyMember& Right)
	{
		return Left.Slot < Right.Slot;
	});

	const bool bChanged = PartyId != InPartyId
		|| LeaderUserId != InLeaderUserId
		|| LocalUserId != InLocalUserId
		|| Members.Num() != UpdatedMembers.Num()
		|| !Algo::Compare(Members, UpdatedMembers, [](const FFlickPartyMember& Left, const FFlickPartyMember& Right)
		{
			return Left.UserId == Right.UserId
				&& Left.DisplayName == Right.DisplayName
				&& Left.Slot == Right.Slot
				&& Left.bLeader == Right.bLeader;
		});

	PartyId = InPartyId;
	LeaderUserId = InLeaderUserId;
	LocalUserId = InLocalUserId;
	Members = MoveTemp(UpdatedMembers);
	if (bChanged)
	{
		OnPartyChanged.Broadcast();
	}
}

void UFlickPartySubsystem::Clear()
{
	if (!IsActive() && PartyId.IsEmpty())
	{
		return;
	}
	PartyId.Reset();
	LeaderUserId.Reset();
	LocalUserId.Reset();
	Members.Reset();
	OnPartyChanged.Broadcast();
}

const FFlickPartyMember* UFlickPartySubsystem::GetMemberBySlot(const int32 Slot) const
{
	return Members.FindByPredicate([Slot](const FFlickPartyMember& Member) { return Member.Slot == Slot; });
}

const FFlickPartyMember* UFlickPartySubsystem::GetMemberByUserId(const FString& UserId) const
{
	return Members.FindByPredicate([&UserId](const FFlickPartyMember& Member) { return Member.UserId == UserId; });
}
