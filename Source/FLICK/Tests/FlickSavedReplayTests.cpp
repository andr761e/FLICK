#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Game/FlickReplaySubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickSavedReplayLibraryTest,"FLICK.Replays.Library",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlickSavedReplayLibraryTest::RunTest(const FString&)
{
	TestFalse(TEXT("Traversal rejected"),UFlickReplaySubsystem::IsValidId(TEXT("../../outside")));
	TestFalse(TEXT("Zero identifier rejected"),UFlickReplaySubsystem::IsValidId(FString::ChrN(32,TCHAR('0'))));
	FFlickSavedReplay Entry; Entry.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits); Entry.Name=TEXT("Replay test");
	Entry.RecordedAt=FDateTime::UtcNow().ToIso8601(); Entry.Duration=60; Entry.Players={TEXT("Blue"),TEXT("Orange")};
	Entry.PoseSampleRate=60;
	Entry.Assets={FSoftObjectPath(TEXT("/Game/TestArena/Pucks/HighDetail/SM_Puck_Heavy_HighDetail.SM_Puck_Heavy_HighDetail"))};
	Entry.Markers={{0,TEXT("Round 1")},{2,TEXT("Shot 1")},{6,TEXT("Knockout")}};
	const FString Payload=UFlickReplaySubsystem::Root()/TEXT("Matches")/(Entry.Id+TEXT(".flick.replay"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Payload),true);
	// Library metadata tests intentionally use a stub payload; engine decoding is
	// exercised separately by the real record/save/seek smoke test.
	FFileHelper::SaveStringToFile(TEXT("test payload"),*Payload);
	TestTrue(TEXT("Index written"),UFlickReplaySubsystem::WriteIndex(Entry));
	FFlickSavedReplay Read;
	TestTrue(TEXT("Index reopened"),UFlickReplaySubsystem::ReadIndex(Entry.Id,Read));
	TestEqual(TEXT("Title round-trips"),Read.Name,Entry.Name);
	TestEqual(TEXT("Preload manifest round-trips"),Read.Assets.Num(),1);
	TestEqual(TEXT("Event markers round-trip"),Read.Markers.Num(),3);
	TestEqual(TEXT("Duration round-trips"),Read.Duration,60.f);
	TestEqual(TEXT("Recording sample rate round-trips"),Read.PoseSampleRate,60.f);
	auto* GI=NewObject<UGameInstance>(); auto* Library=NewObject<UFlickReplaySubsystem>(GI);
	TestFalse(TEXT("Blank rename rejected"),Library->RenameReplay(Entry.Id,TEXT("   ")));
	TestTrue(TEXT("Rename succeeds"),Library->RenameReplay(Entry.Id,TEXT("  A favourite shot  ")));
	TestTrue(TEXT("Favourite succeeds"),Library->SetFavourite(Entry.Id,true));
	TestTrue(TEXT("Metadata survives edits"),UFlickReplaySubsystem::ReadIndex(Entry.Id,Read));
	TestEqual(TEXT("Rename trimmed"),Read.Name,FString(TEXT("A favourite shot")));
	TestTrue(TEXT("Favourite retained"),Read.bFavourite);
	TestTrue(TEXT("Delete removes own files"),Library->Delete(Entry.Id));
	TestFalse(TEXT("Deleted entry absent"),UFlickReplaySubsystem::ReadIndex(Entry.Id,Read));
	TestFalse(TEXT("Invalid delete rejected"),Library->Delete(TEXT("../unrelated")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickSavedReplayCorruptionTest,"FLICK.Replays.CorruptIndex",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlickSavedReplayCorruptionTest::RunTest(const FString&)
{
	const FString Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Index=UFlickReplaySubsystem::Root()/TEXT("Index")/(Id+TEXT(".json"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Index),true);
	FFlickSavedReplay Entry;
	FFileHelper::SaveStringToFile(TEXT("{invalid"),*Index);
	TestFalse(TEXT("Malformed JSON rejected"),UFlickReplaySubsystem::ReadIndex(Id,Entry));
	FFileHelper::SaveStringToFile(TEXT("{\"Format\":99,\"Name\":\"Future\",\"Duration\":60}"),*Index);
	TestFalse(TEXT("Future format rejected"),UFlickReplaySubsystem::ReadIndex(Id,Entry));
	FFileHelper::SaveStringToFile(TEXT("{\"Format\":1,\"Name\":\"Bad\",\"Duration\":-20}"),*Index);
	TestFalse(TEXT("Negative duration rejected"),UFlickReplaySubsystem::ReadIndex(Id,Entry));
	IFileManager::Get().Delete(*Index,false,true);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickSavedReplayClipTest,"FLICK.Replays.Clips",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlickSavedReplayClipTest::RunTest(const FString&)
{
	FFlickSavedReplay Source;Source.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);Source.Name=TEXT("Original match");Source.Duration=60;
	Source.Markers={{0,TEXT("Round 1")},{12,TEXT("Shot 2")},{30,TEXT("Round 2")}};
	const FString Payload=UFlickReplaySubsystem::Root()/TEXT("Matches")/(Source.Id+TEXT(".flick.replay"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Payload),true);
	FFileHelper::SaveStringToFile(TEXT("clip test payload"),*Payload);
	TestTrue(TEXT("Source written"),UFlickReplaySubsystem::WriteIndex(Source));
	auto* GI=NewObject<UGameInstance>();auto* Library=NewObject<UFlickReplaySubsystem>(GI);
	TestTrue(TEXT("Reversed range rejected"),Library->SaveClip(Source.Id,TEXT("Bad"),20,10).IsEmpty());
	TestTrue(TEXT("Outside source rejected"),Library->SaveClip(Source.Id,TEXT("Bad"),10,61).IsEmpty());
	TestTrue(TEXT("Negative time rejected"),Library->SaveClip(Source.Id,TEXT("Bad"),-1,10).IsEmpty());
	TestTrue(TEXT("Zero duration rejected"),Library->SaveClip(Source.Id,TEXT("Bad"),10,10).IsEmpty());
	TestTrue(TEXT("Blank name rejected"),Library->SaveClip(Source.Id,TEXT("  "),10,20).IsEmpty());
	const FString ClipId=Library->SaveClip(Source.Id,TEXT("  Winning shot  "),10,20);
	FFlickSavedReplay Clip;
	TestTrue(TEXT("Clip can be read"),UFlickReplaySubsystem::ReadIndex(ClipId,Clip));
	TestTrue(TEXT("Entry identifies as a clip"),Clip.IsClip());
	TestEqual(TEXT("Name trimmed"),Clip.Name,FString(TEXT("Winning shot")));
	TestEqual(TEXT("Shared source"),Clip.PayloadId(),Source.Id);
	TestEqual(TEXT("Clip duration"),Clip.ViewDuration(),10.f);
	TestEqual(TEXT("Source clock retained"),Clip.Duration,60.f);
	TestEqual(TEXT("Markers retain source times"),Clip.Markers[1].Time,12.f);
	const FString NestedId=Library->SaveClip(ClipId,TEXT("Contact"),2,4);
	FFlickSavedReplay Nested;
	TestTrue(TEXT("Nested clip readable"),UFlickReplaySubsystem::ReadIndex(NestedId,Nested));
	TestEqual(TEXT("Nested clip flattened"),Nested.PayloadId(),Source.Id);
	TestEqual(TEXT("Nested start is absolute"),Nested.ClipStart,12.f);
	TestEqual(TEXT("Nested end is absolute"),Nested.ClipEnd,14.f);
	TestEqual(TEXT("Original match title retained"),Nested.SourceName,Source.Name);
	TestTrue(TEXT("Nested out-of-bounds rejected"),Library->SaveClip(ClipId,TEXT("Bad"),0,11).IsEmpty());
	FFlickSavedReplay Invalid=Clip;Invalid.SourceId=TEXT("../escape");
	TestFalse(TEXT("Unsafe payload ID rejected"),UFlickReplaySubsystem::WriteIndex(Invalid));
	Invalid=Clip;Invalid.ClipEnd=61;TestFalse(TEXT("Invalid clip metadata rejected"),UFlickReplaySubsystem::WriteIndex(Invalid));
	TestTrue(TEXT("Original entry deleted"),Library->Delete(Source.Id));
	TestTrue(TEXT("Clip survives deleting original"),UFlickReplaySubsystem::ReadIndex(ClipId,Clip));
	TestTrue(TEXT("First clip deleted"),Library->Delete(ClipId));
	TestTrue(TEXT("Nested clip survives independently"),UFlickReplaySubsystem::ReadIndex(NestedId,Nested));
	TestTrue(TEXT("Final clip deleted"),Library->Delete(NestedId));
	TestEqual(TEXT("Payload removed only after last reference"),IFileManager::Get().FileSize(*Payload),int64(-1));
	return true;
}
#endif
