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
	Party->Synchronize(TEXT("party-a"), TEXT("100"), Members, TEXT("100"));

	TestTrue(TEXT("Party becomes active after synchronization"), Party->IsActive());
	TestTrue(TEXT("Lobby owner is the local party leader"), Party->IsLocalLeader());
	TestEqual(TEXT("Both Steam lobby members are represented"), Party->GetMemberCount(), 2);
	const FFlickPartyMember* Friend = Party->GetMemberByUserId(TEXT("200"));
	TestNotNull(TEXT("Friend can be resolved by platform id"), Friend);
	const int32 FriendSlot = Friend ? Friend->Slot : INDEX_NONE;

	Members.Reset();
	Members.Emplace(TEXT("200"), TEXT("Friend"));
	Members.Emplace(TEXT("100"), TEXT("Leader"));
	Party->Synchronize(TEXT("party-a"), TEXT("200"), Members, TEXT("100"));
	TestFalse(TEXT("Leadership transfer updates local authority"), Party->IsLocalLeader());
	Friend = Party->GetMemberByUserId(TEXT("200"));
	TestTrue(TEXT("Promoted member is marked as leader"), Friend && Friend->bLeader);
	TestEqual(TEXT("Member slots remain stable when Steam changes participant order"),
		Friend ? Friend->Slot : INDEX_NONE, FriendSlot);

	Party->Clear();
	TestFalse(TEXT("Clearing the party removes active social state"), Party->IsActive());
	TestEqual(TEXT("Clearing removes all members"), Party->GetMemberCount(), 0);
	return true;
}

#endif
