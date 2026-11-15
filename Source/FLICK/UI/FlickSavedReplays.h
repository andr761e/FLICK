#pragma once
#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
class UGameInstance;
class SWidget;
TSharedRef<SWidget> MakeFlickSavedReplays(UGameInstance* Instance,FOnClicked Back);
