#pragma once

#include "Game/FlickGameMode.h"

#include "Arena/FlickArena.h"
#include "Arena/FlickBobArena.h"
#include "Arena/FlickTestArena.h"
#include "Audio/FlickAudioDirector.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Core/FlickAccoladeRules.h"
#include "Core/FlickBobRules.h"
#include "Core/FlickBotShotPlanner.h"
#include "Core/FlickLog.h"
#include "Core/FlickMatchmakingRules.h"
#include "Core/FlickModeRules.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickRankRules.h"
#include "Core/FlickTeamRules.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "EngineUtils.h"
#include "Feedback/FlickWorldFeedback.h"
#include "Game/FlickGameInstance.h"
#include "Game/FlickGameState.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/GameUserSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Online/FlickMatchmakingCoordinatorSubsystem.h"
#include "Online/FlickSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Pieces/FlickPiece.h"
#include "Player/FlickCameraPawn.h"
#include "Player/FlickPlayerController.h"
#include "Player/FlickPlayerState.h"
#include "Ranking/FlickRankedBackendSubsystem.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "TimerManager.h"
#include "UI/FlickHUD.h"

namespace FlickGameModePrivate
{
	inline bool bCoordinatorAutoQueueConsumed = false;

	inline FString GetPlayerOnlineId(const APlayerState* PlayerState)
	{
		if (const AFlickPlayerState* FlickPlayerState = Cast<AFlickPlayerState>(PlayerState);
			FlickPlayerState && !FlickPlayerState->GetVerifiedOnlineAccountId().IsEmpty())
		{
			return FlickPlayerState->GetVerifiedOnlineAccountId();
		}
		if (PlayerState)
		{
			const TSharedPtr<const FUniqueNetId> OnlineId = PlayerState->GetUniqueId().GetUniqueNetId();
			if (OnlineId.IsValid())
			{
				return OnlineId->ToString();
			}
		}
		return PlayerState ? PlayerState->GetPlayerName() : FString();
	}
}
