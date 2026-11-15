#include "Game/FlickReplaySubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
	FString IndexPath(const FString& Id) { return UFlickReplaySubsystem::Root()/TEXT("Index")/(Id+TEXT(".json")); }
}

FString UFlickReplaySubsystem::Root() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Replays")); }
bool UFlickReplaySubsystem::IsValidId(const FString& Id)
{
	FGuid Guid;
	return Id.Len()==32 && FGuid::ParseExact(Id,EGuidFormats::Digits,Guid) && Guid.IsValid();
}

bool UFlickReplaySubsystem::WriteIndex(const FFlickSavedReplay& R)
{
	if (!IsValidId(R.Id) || R.Name.Len()>100 || R.Markers.Num()>20000 || R.Players.Num()>8) return false;
	auto O=MakeShared<FJsonObject>();
	O->SetNumberField(TEXT("Format"),1); O->SetStringField(TEXT("Id"),R.Id); O->SetStringField(TEXT("Name"),R.Name);
	O->SetStringField(TEXT("Date"),R.RecordedAt); O->SetStringField(TEXT("Build"),R.GameVersion);
	O->SetStringField(TEXT("Mode"),R.Mode); O->SetStringField(TEXT("Result"),R.Result);
	O->SetNumberField(TEXT("Variant"),static_cast<int32>(R.Variant)); O->SetNumberField(TEXT("TeamSize"),R.TeamSize);
	O->SetNumberField(TEXT("Duration"),R.Duration); O->SetBoolField(TEXT("Favourite"),R.bFavourite);
	O->SetNumberField(TEXT("PoseSampleRate"),R.PoseSampleRate);
	TArray<TSharedPtr<FJsonValue>> Players,Markers,Assets;
	for(const auto& Path:R.Assets) Assets.Add(MakeShared<FJsonValueString>(Path.ToString()));
	O->SetArrayField(TEXT("Assets"),Assets);
	for (const auto& P:R.Players) Players.Add(MakeShared<FJsonValueString>(P.Left(100)));
	for (const auto& M:R.Markers) { auto V=MakeShared<FJsonObject>(); V->SetNumberField(TEXT("Time"),M.Time); V->SetStringField(TEXT("Label"),M.Label.Left(100)); Markers.Add(MakeShared<FJsonValueObject>(V)); }
	O->SetArrayField(TEXT("Players"),Players); O->SetArrayField(TEXT("Markers"),Markers);
	FString Text;
	if (!FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&Text))) return false;
	IFileManager::Get().MakeDirectory(*(Root()/TEXT("Index")),true);
	const FString Path=IndexPath(R.Id), Temp=Path+TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Text,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
	return IFileManager::Get().Move(*Path,*Temp,true,true);
}

bool UFlickReplaySubsystem::ReadIndex(const FString& Id,FFlickSavedReplay& R)
{
	if (!IsValidId(Id)) return false;
	const auto Size=IFileManager::Get().FileSize(*IndexPath(Id));
	if (Size<2 || Size>4*1024*1024) return false;
	FString Text; TSharedPtr<FJsonObject> O;
	if (!FFileHelper::LoadFileToString(Text,*IndexPath(Id)) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O) || !O) return false;
	double Format=0, Variant=0, TeamSize=1, Duration=0;
	if (!O->TryGetNumberField(TEXT("Format"),Format) || Format!=1 || !O->TryGetStringField(TEXT("Name"),R.Name) || R.Name.Len()>100
		|| !O->TryGetNumberField(TEXT("Duration"),Duration) || !FMath::IsFinite(Duration) || Duration<0 || Duration>14400) return false;
	O->TryGetNumberField(TEXT("Variant"),Variant); O->TryGetNumberField(TEXT("TeamSize"),TeamSize);
	if (Variant<0 || Variant>2 || TeamSize<1 || TeamSize>4) return false;
	R.Id=Id; R.Duration=Duration; R.Variant=static_cast<EFlickMatchVariant>(static_cast<int32>(Variant)); R.TeamSize=static_cast<int32>(TeamSize);
	double SampleRate=8;
	O->TryGetNumberField(TEXT("PoseSampleRate"),SampleRate);
	if(!FMath::IsFinite(SampleRate)||SampleRate<1||SampleRate>240) return false;
	R.PoseSampleRate=SampleRate;
	O->TryGetStringField(TEXT("Date"),R.RecordedAt); O->TryGetStringField(TEXT("Build"),R.GameVersion);
	O->TryGetStringField(TEXT("Mode"),R.Mode); O->TryGetStringField(TEXT("Result"),R.Result); O->TryGetBoolField(TEXT("Favourite"),R.bFavourite);
	R.Players.Reset(); R.Markers.Reset(); R.Assets.Reset();
	const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
	if(O->TryGetArrayField(TEXT("Assets"),Values))
	{
		if(Values->Num()>4096) return false;
		for(const auto& V:*Values) {FString Path;if(!V->TryGetString(Path)||Path.Len()>512||!(Path.StartsWith(TEXT("/Game/"))||Path.StartsWith(TEXT("/Engine/BasicShapes/"))||Path.StartsWith(TEXT("/Engine/EngineMaterials/")))) continue; R.Assets.AddUnique(FSoftObjectPath(Path));}
	}
	if (O->TryGetArrayField(TEXT("Players"),Values)) { if (Values->Num()>8) return false; for(const auto& V:*Values) { FString P; if(!V->TryGetString(P)||P.Len()>100) return false; R.Players.Add(P); } }
	if (O->TryGetArrayField(TEXT("Markers"),Values))
	{
		if (Values->Num()>20000) return false;
		float Previous=-1;
		for (const auto& V:*Values)
		{
			const TSharedPtr<FJsonObject>* M=nullptr; double Time=0; FString Label;
			if (!V->TryGetObject(M)||!M||!M->IsValid()||!(*M)->TryGetNumberField(TEXT("Time"),Time)
				||!FMath::IsFinite(Time)||Time<Previous||Time>Duration+.1||!(*M)->TryGetStringField(TEXT("Label"),Label)||Label.Len()>100) return false;
			R.Markers.Add({static_cast<float>(Time),Label}); Previous=Time;
		}
	}
	R.Bytes=IFileManager::Get().FileSize(*(Root()/TEXT("Matches")/(Id+TEXT(".flick.replay"))));
	return R.Bytes>0;
}

TArray<FFlickSavedReplay> UFlickReplaySubsystem::List() const
{
	TArray<FString> Files; TArray<FFlickSavedReplay> Results;
	IFileManager::Get().FindFiles(Files,*(Root()/TEXT("Index")/TEXT("*.json")),true,false);
	for (const auto& File:Files) { FFlickSavedReplay R; if(ReadIndex(FPaths::GetBaseFilename(File),R)) Results.Add(MoveTemp(R)); }
	Results.Sort([](const auto& A,const auto& B) { return A.bFavourite!=B.bFavourite ? A.bFavourite : A.RecordedAt>B.RecordedAt; });
	return Results;
}

bool UFlickReplaySubsystem::RenameReplay(const FString& Id,const FString& Name)
{
	FFlickSavedReplay R;
	const FString Clean=Name.TrimStartAndEnd().Left(100);
	if (Clean.IsEmpty()||!ReadIndex(Id,R)) return false;
	R.Name=Clean; const bool bSaved=WriteIndex(R); if(bSaved) ++LibraryRevision; return bSaved;
}
bool UFlickReplaySubsystem::SetFavourite(const FString& Id,bool bValue)
{
	FFlickSavedReplay R; if(!ReadIndex(Id,R)) return false; R.bFavourite=bValue; const bool bSaved=WriteIndex(R); if(bSaved) ++LibraryRevision; return bSaved;
}
bool UFlickReplaySubsystem::Delete(const FString& Id)
{
	if (!IsValidId(Id)|| (IsTheatre()&&Playing.Id==Id)) return false;
	if (!IFileManager::Get().Delete(*(Root()/TEXT("Matches")/(Id+TEXT(".flick.replay"))),false,true)) return false;
	const bool bDeleted=IFileManager::Get().Delete(*IndexPath(Id),false,true); ++LibraryRevision; return bDeleted;
}
