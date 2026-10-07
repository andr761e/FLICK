#pragma once
#include "CoreMinimal.h"

// Stable, server-whitelisted phrases. Customization never accepts arbitrary network text.
namespace FlickQuickChats
{
 struct FPhrase { const TCHAR* Id; const TCHAR* Text; };
 const TArray<FPhrase>& GetPhrases();
 const FPhrase* Find(const FString& Id);
 FString GetSlot(int32 Group, int32 Slot);
 bool SetSlot(int32 Group, int32 Slot, const FString& Id);
 void Reset();
 const TCHAR* GroupName(int32 Group);
 const TCHAR* ControlId(int32 Index);
}
