#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Online/FlickPartySubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickPartySubsystemStateTest,
	"FLICK.Online.PersistentPartyState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickPartySubsystemStateTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UFlickPartySubsystem* Party = NewObject<UFlickPartySubsystem>(GameInstance);
	TArray<TPair<FString, FString>> Members;
	Members.Emplace(TEXT("100"), TEXT("Leader"));
	Members.Emplace(TEXT("200"), TEXT("Friend"));
	TMap<FString, EFlickPieceArchetype> ShowcasePucks;
	ShowcasePucks.Add(TEXT("100"), EFlickPieceArchetype::Striker);
	ShowcasePucks.Add(TEXT("200"), EFlickPieceArchetype::Heavy);
	Party->Synchronize(TEXT("party-a"), TEXT("100"), Members, TEXT("100"), ShowcasePucks);

	TestTrue(TEXT("Party becomes active after synchronization"), Party->IsActive());
	TestTrue(TEXT("Lobby owner is the local party leader"), Party->IsLocalLeader());
	TestEqual(TEXT("Both Steam lobby members are represented"), Party->GetMemberCount(), 2);
	const FFlickPartyMember* Friend = Party->GetMemberByUserId(TEXT("200"));
	TestNotNull(TEXT("Friend can be resolved by platform id"), Friend);
	TestTrue(TEXT("Friend's display puck is retained"), Friend && Friend->ShowcaseArchetype == EFlickPieceArchetype::Heavy);
	const int32 FriendSlot = Friend ? Friend->Slot : INDEX_NONE;

	Members.Reset();
	Members.Emplace(TEXT("200"), TEXT("Friend"));
	Members.Emplace(TEXT("100"), TEXT("Leader"));
	ShowcasePucks.Add(TEXT("200"), EFlickPieceArchetype::Bouncer);
	Party->Synchronize(TEXT("party-a"), TEXT("200"), Members, TEXT("100"), ShowcasePucks);
	TestFalse(TEXT("Leadership transfer updates local authority"), Party->IsLocalLeader());
	Friend = Party->GetMemberByUserId(TEXT("200"));
	TestTrue(TEXT("Promoted member is marked as leader"), Friend && Friend->bLeader);
	TestTrue(TEXT("A friend's new display puck propagates without changing their slot"), Friend && Friend->ShowcaseArchetype == EFlickPieceArchetype::Bouncer);
	TestEqual(TEXT("Member slots remain stable when Steam changes participant order"),
		Friend ? Friend->Slot : INDEX_NONE, FriendSlot);

	Party->Clear();
	TestFalse(TEXT("Clearing the party removes active social state"), Party->IsActive());
	TestEqual(TEXT("Clearing removes all members"), Party->GetMemberCount(), 0);
	return true;
}

#endif
