#include "Core/FlickQuickChats.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickQuickChats
{
 const TCHAR* Section = TEXT("FLICK.QuickChats");
 const TArray<FPhrase>& GetPhrases()
 {
  static const TArray<FPhrase> Phrases = {
   {TEXT("GotIt"), TEXT("I got it!")}, {TEXT("TakeShot"), TEXT("Take the shot!")},
   {TEXT("Defending"), TEXT("Defending!")}, {TEXT("Switch"), TEXT("Going for the switch!")},
   {TEXT("NiceShot"), TEXT("Nice shot!")}, {TEXT("GreatPlay"), TEXT("Great play!")},
   {TEXT("NiceSave"), TEXT("What a save!")}, {TEXT("CloseOne"), TEXT("Close one!")},
   {TEXT("Thanks"), TEXT("Thanks!")}, {TEXT("Sorry"), TEXT("Sorry!")},
   {TEXT("NoProblem"), TEXT("No problem.")}, {TEXT("MyBad"), TEXT("My bad!")},
   {TEXT("GoodGame"), TEXT("Good game!")}, {TEXT("WellPlayed"), TEXT("Well played!")},
   {TEXT("GoodLuck"), TEXT("Good luck!")}, {TEXT("Rematch"), TEXT("Rematch?")},
   {TEXT("Help"), TEXT("Need some help!")}, {TEXT("Careful"), TEXT("Careful near the edge!")},
   {TEXT("NiceKO"), TEXT("Nice knockout!")}, {TEXT("YourTurn"), TEXT("Your turn!")},
   {TEXT("GreatPass"), TEXT("Great pass!")}, {TEXT("Ready"), TEXT("Ready!")},
   {TEXT("HoldOn"), TEXT("Hold on!")}, {TEXT("Teamwork"), TEXT("Great teamwork!")}
  };
  return Phrases;
 }
 const FPhrase* Find(const FString& Id) { return GetPhrases().FindByPredicate([&Id](const FPhrase& P) { return Id == P.Id; }); }
 FString GetSlot(int32 Group, int32 Slot)
 {
  if (Group < 0 || Group >= 4 || Slot < 0 || Slot >= 4) return FString();
  FString Id;
  if (GConfig) GConfig->GetString(Section, *FString::Printf(TEXT("Slot%d_%d"), Group, Slot), Id, GGameUserSettingsIni);
  return Find(Id) ? Id : FString(GetPhrases()[Group * 4 + Slot].Id);
 }
 bool SetSlot(int32 Group, int32 Slot, const FString& Id)
 {
  if (!GConfig || Group < 0 || Group >= 4 || Slot < 0 || Slot >= 4 || !Find(Id)) return false;
  GConfig->SetString(Section, *FString::Printf(TEXT("Slot%d_%d"), Group, Slot), *Id, GGameUserSettingsIni);
  GConfig->Flush(false, GGameUserSettingsIni); return true;
 }
 void Reset() { if (GConfig) { GConfig->EmptySection(Section, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); } }
 const TCHAR* GroupName(int32 Group) { static const TCHAR* Names[] = {TEXT("TACTICS"), TEXT("REACTIONS"), TEXT("RESPONSES"), TEXT("SPORTSMANSHIP")}; return Names[FMath::Clamp(Group, 0, 3)]; }
 const TCHAR* ControlId(int32 Index) { static const TCHAR* Ids[] = {TEXT("QuickChat1"), TEXT("QuickChat2"), TEXT("QuickChat3"), TEXT("QuickChat4")}; return Ids[FMath::Clamp(Index, 0, 3)]; }
}
