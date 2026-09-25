#include "Online/FlickSessionSubsystem.h"

#include "Online/FlickMatchmakingCoordinatorSubsystem.h"
#include "Online/FlickPartySubsystem.h"

#include "Core/FlickLog.h"
#include "Core/FlickMatchmakingRules.h"
#include "Core/FlickPieceArchetypeRules.h"
#include "Core/FlickRankRules.h"
#include "Core/FlickTeamRules.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/EngineBaseTypes.h"
#include "Game/FlickGameMode.h"
#include "Game/FlickGameInstance.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Player/FlickPlayerState.h"
#include "Ranking/FlickRankingSubsystem.h"
#include "TimerManager.h"
#include "Styling/SlateBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Async/Async.h"

#if WITH_FLICK_STEAMWORKS
#include "steam/steam_api.h"

namespace
{
	class FFlickSteamInviteListener
	{
	public:
		explicit FFlickSteamInviteListener(UFlickSessionSubsystem* InOwner) : Owner(InOwner)
		{
			InviteCallback.Register(this, &FFlickSteamInviteListener::OnLobbyInvite);
		}
		~FFlickSteamInviteListener() { InviteCallback.Unregister(); }

	private:
		void OnLobbyInvite(LobbyInvite_t* Invite)
		{
			if (!Invite) return;
			const uint64 InviterId = Invite->m_ulSteamIDUser;
			const uint64 LobbyId = Invite->m_ulSteamIDLobby;
			const uint64 GameId = Invite->m_ulGameID;
			const TWeakObjectPtr<UFlickSessionSubsystem> WeakOwner = Owner;
			AsyncTask(ENamedThreads::GameThread, [WeakOwner, InviterId, LobbyId, GameId]()
			{
				if (WeakOwner.IsValid() && SteamUtils() && CGameID(GameId).AppID() == SteamUtils()->GetAppID())
				{
					WeakOwner->NotifySteamPartyInvite(InviterId, LobbyId);
				}
			});
		}
		TWeakObjectPtr<UFlickSessionSubsystem> Owner;
		CCallbackManual<FFlickSteamInviteListener, LobbyInvite_t> InviteCallback;
	};
	TUniquePtr<FFlickSteamInviteListener> GSteamInviteListener;
}
#endif

namespace
{
	const FName FlickGameKey(TEXT("FLICK_GAME"));
	const FName FlickVariantKey(TEXT("FLICK_VARIANT"));
	const FName FlickPurposeKey(TEXT("FLICK_PURPOSE"));
	const FName FlickTeamSizeKey(TEXT("FLICK_TEAM_SIZE"));
	const FName FlickAcceptingPlayersKey(TEXT("FLICK_ACCEPTING_PLAYERS"));
	const FName FlickPartyIdKey(TEXT("FLICK_PARTY_ID"));
	const FName FlickPartySizeKey(TEXT("FLICK_PARTY_SIZE"));
	const FName FlickPartyCommandKey(TEXT("FLICK_PARTY_COMMAND"));
	const FName FlickPartyMatchIdKey(TEXT("FLICK_PARTY_MATCH_ID"));
	const FName FlickPartyServerIdKey(TEXT("FLICK_PARTY_SERVER_ID"));
	const FName FlickPartyAddressKey(TEXT("FLICK_PARTY_ADDRESS"));
	const FName FlickPartyMatchVariantKey(TEXT("FLICK_PARTY_MATCH_VARIANT"));
	const FName FlickPartyMatchTeamSizeKey(TEXT("FLICK_PARTY_MATCH_TEAM_SIZE"));
	const FName FlickPartyMatchRankedKey(TEXT("FLICK_PARTY_MATCH_RANKED"));
	const FName FlickPartyMatchExpiryKey(TEXT("FLICK_PARTY_MATCH_EXPIRY"));
	const FName FlickPartyReservationsKey(TEXT("FLICK_PARTY_RESERVATIONS"));
	const FName FlickRankedKey(TEXT("FLICK_RANKED"));
	const FName FlickMmrKey(TEXT("FLICK_MMR"));
	const FString FlickGameValue(TEXT("FLICK_V1"));
	const FString FlickMatchPurpose(TEXT("MATCH"));
	const FString FlickPartyPurpose(TEXT("PARTY"));
	const FString FlickMatchmakingPurpose(TEXT("MATCHMAKING"));
	const TCHAR* FlickSocialSection = TEXT("FLICK.Social");
	const TCHAR* FlickShowcaseSection = TEXT("FLICK.ProfileCosmetics");
	const char* FlickShowcaseMemberDataKey = "FLICK_SHOWCASE_PUCK";
	constexpr int32 MaximumRecentPlayers = 12;
	constexpr int32 MaximumRankedSearchAttempts = 5;

	IOnlineSubsystem* GetFlickOnlineSubsystem(const UObject* Context)
	{
		return Context ? Online::GetSubsystem(Context->GetWorld()) : nullptr;
	}

#if WITH_FLICK_STEAMWORKS
	uint64 GetSteamLobbyId(const FString& SessionId)
	{
		// OSS Steam formats lobby sessions as "Lobby[0x...]", not a decimal id.
		FString Number = SessionId;
		if (Number.StartsWith(TEXT("Lobby[0x")) && Number.EndsWith(TEXT("]")))
		{
			Number = Number.Mid(8, Number.Len() - 9);
			return FCString::Strtoui64(*Number, nullptr, 16);
		}
		return FCString::Strtoui64(*Number, nullptr, 10);
	}
#endif
}

void UFlickSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	int32 SavedShowcaseArchetype = 0;
	GConfig->GetInt(FlickShowcaseSection, TEXT("ShowcasePuck"), SavedShowcaseArchetype, GGameUserSettingsIni);
	if (SavedShowcaseArchetype >= 0 && SavedShowcaseArchetype < FlickPieceArchetypeRules::ArchetypeCount)
	{
		LocalShowcaseArchetype = static_cast<EFlickPieceArchetype>(SavedShowcaseArchetype);
	}
	RegisterOnlineDelegates();
#if WITH_FLICK_STEAMWORKS
	if (IsSteamAvailable() && SteamFriends() && !GSteamInviteListener)
	{
		GSteamInviteListener = MakeUnique<FFlickSteamInviteListener>(this);
	}
#endif
	LoadRecentPlayers();
	RefreshFriends();
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UFlickSessionSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UFlickSessionSubsystem::HandleTravelFailure);
	}

	SetState(
		EFlickSessionState::Idle,
		IsSteamAvailable()
			? FString::Printf(TEXT("STEAM CONNECTED AS %s"), *GetLocalDisplayName())
			: TEXT("STEAM IS OFFLINE - LOCAL DEVELOPMENT OPTIONS REMAIN AVAILABLE"));
}

void UFlickSessionSubsystem::Deinitialize()
{
#if WITH_FLICK_STEAMWORKS
	GSteamInviteListener.Reset();
#endif
	TransferPartyLeadershipBeforeLeaving();
	StopPartySynchronization();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PartyJoinTimeoutTimer);
		GetWorld()->GetTimerManager().ClearTimer(PrivateMatchJoinTimeoutTimer);
	}
	if (UFlickPartySubsystem* Party = GetPartySubsystem())
	{
		Party->Clear();
	}
	ClearOnlineDelegates();
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}
	ActiveSearch.Reset();
	PendingJoinResult.Reset();
	BrowserEntries.Reset();
	Friends.Reset();
	RecentPlayers.Reset();
	Super::Deinitialize();
}

bool UFlickSessionSubsystem::RegisterOnlineDelegates()
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!Sessions.IsValid())
	{
		return false;
	}

	if (!CreateSessionHandle.IsValid())
	{
		CreateSessionHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleCreateSessionComplete));
		FindSessionsHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleFindSessionsComplete));
		JoinSessionHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleJoinSessionComplete));
		DestroySessionHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleDestroySessionComplete));
		InviteAcceptedHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
			FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleInviteAccepted));
		FindInvitingFriendHandle = Sessions->AddOnFindFriendSessionCompleteDelegate_Handle(0,
			FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleFindInvitingFriendSession));
		ParticipantJoinedHandle = Sessions->AddOnSessionParticipantJoinedDelegate_Handle(
			FOnSessionParticipantJoinedDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleSessionParticipantJoined));
		ParticipantLeftHandle = Sessions->AddOnSessionParticipantLeftDelegate_Handle(
			FOnSessionParticipantLeftDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleSessionParticipantLeft));
		SessionSettingsUpdatedHandle = Sessions->AddOnSessionSettingsUpdatedDelegate_Handle(
			FOnSessionSettingsUpdatedDelegate::CreateUObject(this, &UFlickSessionSubsystem::HandleSessionSettingsUpdated));
	}
	return true;
}

void UFlickSessionSubsystem::ClearOnlineDelegates()
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionHandle);
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsHandle);
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionHandle);
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
		Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteAcceptedHandle);
		Sessions->ClearOnFindFriendSessionCompleteDelegate_Handle(0, FindInvitingFriendHandle);
		Sessions->ClearOnSessionParticipantJoinedDelegate_Handle(ParticipantJoinedHandle);
		Sessions->ClearOnSessionParticipantLeftDelegate_Handle(ParticipantLeftHandle);
		Sessions->ClearOnSessionSettingsUpdatedDelegate_Handle(SessionSettingsUpdatedHandle);
	}
	CreateSessionHandle.Reset();
	FindSessionsHandle.Reset();
	JoinSessionHandle.Reset();
	DestroySessionHandle.Reset();
	InviteAcceptedHandle.Reset();
	FindInvitingFriendHandle.Reset();
	ParticipantJoinedHandle.Reset();
	ParticipantLeftHandle.Reset();
	SessionSettingsUpdatedHandle.Reset();
}

UFlickPartySubsystem* UFlickSessionSubsystem::GetPartySubsystem() const
{
	return GetGameInstance() ? GetGameInstance()->GetSubsystem<UFlickPartySubsystem>() : nullptr;
}

bool UFlickSessionSubsystem::IsSteamAvailable() const
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	if (!OnlineSubsystem || OnlineSubsystem->GetSubsystemName() != FName(TEXT("STEAM")))
	{
		return false;
	}
	const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface();
	return Identity.IsValid() && Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
}

bool UFlickSessionSubsystem::HasActiveSession() const
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	return Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession) != nullptr;
}

FString UFlickSessionSubsystem::GetOnlineServiceName() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UFlickMatchmakingCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>();
			Coordinator && Coordinator->ShouldUseCoordinator())
		{
			return TEXT("COORDINATOR");
		}
	}
	const IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	return OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : TEXT("OFFLINE");
}

FString UFlickSessionSubsystem::GetLocalDisplayName() const
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FString Nickname = Identity.IsValid() ? Identity->GetPlayerNickname(0) : FString();
	return Nickname.IsEmpty() ? TEXT("LOCAL PLAYER") : Nickname;
}

void UFlickSessionSubsystem::CycleShowcaseArchetype(const int32 Direction)
{
	LocalShowcaseArchetype = FlickPieceArchetypeRules::Cycle(LocalShowcaseArchetype, Direction);
	GConfig->SetInt(FlickShowcaseSection, TEXT("ShowcasePuck"),
		static_cast<int32>(LocalShowcaseArchetype), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	PublishShowcaseArchetypeToParty();
	if (ActivePurpose == EFlickSessionPurpose::Party)
	{
		RefreshPartyFromSession();
	}
}

void UFlickSessionSubsystem::PublishShowcaseArchetypeToParty() const
{
#if WITH_FLICK_STEAMWORKS
	if (ActivePurpose != EFlickSessionPurpose::Party || !SteamMatchmaking() || !SteamUser())
	{
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	const uint64 LobbyId = NamedSession ? GetSteamLobbyId(NamedSession->GetSessionIdStr()) : 0;
	if (LobbyId == 0)
	{
		return;
	}
	const CSteamID Lobby(LobbyId);
	const FString Value = FString::FromInt(static_cast<int32>(LocalShowcaseArchetype));
	const char* Existing = SteamMatchmaking()->GetLobbyMemberData(Lobby, SteamUser()->GetSteamID(), FlickShowcaseMemberDataKey);
	if (!Existing || Value != UTF8_TO_TCHAR(Existing))
	{
		SteamMatchmaking()->SetLobbyMemberData(Lobby, FlickShowcaseMemberDataKey, TCHAR_TO_UTF8(*Value));
	}
#endif
}

const FSlateBrush* UFlickSessionSubsystem::GetLocalAvatarBrush()
{
#if WITH_FLICK_STEAMWORKS
	if (SteamUser())
	{
		return GetAvatarBrush(FString::Printf(TEXT("%llu"), SteamUser()->GetSteamID().ConvertToUint64()));
	}
#endif
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	return UserId.IsValid() ? GetAvatarBrush(UserId->ToString()) : nullptr;
}

const FSlateBrush* UFlickSessionSubsystem::GetAvatarBrush(const FString& UserId)
{
	if (const TSharedPtr<FSlateBrush>* Existing = AvatarBrushes.Find(UserId))
	{
		return Existing->Get();
	}

#if WITH_FLICK_STEAMWORKS
	if (UserId.IsEmpty() || !SteamFriends() || !SteamUtils())
	{
		return nullptr;
	}

	const uint64 NumericId = FCString::Strtoui64(*UserId, nullptr, 10);
	if (NumericId == 0)
	{
		return nullptr;
	}
	const int32 ImageHandle = SteamFriends()->GetLargeFriendAvatar(CSteamID(NumericId));
	if (ImageHandle <= 0)
	{
		// -1 means Steam has started an asynchronous download. Slate asks again on
		// subsequent paints, so the image appears as soon as Steam caches it.
		return nullptr;
	}

	uint32 Width = 0;
	uint32 Height = 0;
	if (!SteamUtils()->GetImageSize(ImageHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		return nullptr;
	}
	TArray<uint8> Pixels;
	Pixels.SetNumUninitialized(Width * Height * 4);
	if (!SteamUtils()->GetImageRGBA(ImageHandle, Pixels.GetData(), Pixels.Num()))
	{
		return nullptr;
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty())
	{
		return nullptr;
	}
	Texture->SRGB = true;
	void* Destination = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Destination, Pixels.GetData(), Pixels.Num());
	Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
	Texture->UpdateResource();
	AvatarTextures.Add(UserId, Texture);

	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Texture);
	Brush->ImageSize = FVector2D(static_cast<float>(Width), static_cast<float>(Height));
	Brush->DrawAs = ESlateBrushDrawType::Image;
	AvatarBrushes.Add(UserId, Brush);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().InvalidateAllWidgets(false);
	}
	return Brush.Get();
#else
	return nullptr;
#endif
}

void UFlickSessionSubsystem::RefreshSteamAvatarCache()
{
	bool bWaitingForAvatar = GetLocalAvatarBrush() == nullptr;
	for (const FFlickSocialPlayerEntry& Friend : Friends)
	{
		bWaitingForAvatar |= GetAvatarBrush(Friend.UserId) == nullptr;
	}
	for (const FFlickRecentPlayerEntry& RecentPlayer : RecentPlayers)
	{
		bWaitingForAvatar |= GetAvatarBrush(RecentPlayer.UserId) == nullptr;
	}
	OnSessionsChanged.Broadcast();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().InvalidateAllWidgets(false);
	}

	++AvatarRefreshAttempts;
	if (bWaitingForAvatar && AvatarRefreshAttempts < 8 && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AvatarRefreshTimer,
			this,
			&UFlickSessionSubsystem::RefreshSteamAvatarCache,
			0.75f,
			false);
	}
	else
	{
		UE_LOG(LogFlick, Log, TEXT("Steam avatars cached: %d/%d after %d attempts"), AvatarBrushes.Num(), Friends.Num() + RecentPlayers.Num() + 1, AvatarRefreshAttempts);
	}
}

void UFlickSessionSubsystem::SetPersistentPartyIdentity(
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bLeader)
{
	PersistentPartyId = PartyId;
	PersistentPartySlot = FMath::Clamp(PartySlot, 0, FlickMaximumPartyMembers - 1);
	PersistentPartySize = FMath::Clamp(PartySize, 1, FlickMaximumPartyMembers);
	bPersistentPartyLeader = bLeader;
	UE_LOG(
		LogFlick,
		Log,
		TEXT("Persistent party identity set: %s slot %d/%d leader=%d"),
		*PersistentPartyId,
		PersistentPartySlot + 1,
		PersistentPartySize,
		bPersistentPartyLeader ? 1 : 0);
}

void UFlickSessionSubsystem::ClearPersistentPartyIdentity()
{
	PersistentPartyId.Reset();
	PersistentPartySlot = INDEX_NONE;
	PersistentPartySize = 0;
	bPersistentPartyLeader = false;
	PartyRestoreAttempts = 0;
}

bool UFlickSessionSubsystem::HostSession(const EFlickMatchVariant Variant, const int32 MaximumPlayers)
{
	if (!IsSteamAvailable() || !RegisterOnlineDelegates())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM IS NOT AVAILABLE. START STEAM AND LAUNCH FLICK AGAIN."));
		return false;
	}

	PendingHostVariant = Variant;
	PendingMaximumPlayers = FMath::Max(2, MaximumPlayers);
	PendingPlayersPerTeam = FMath::Clamp(PendingMaximumPlayers / 2, 1, 3);
	PendingPartySize = 1;
	PendingPurpose = EFlickSessionPurpose::Match;
	bPendingRanked = false;
	PendingQueueRating = FlickRankRules::DefaultRating;
	if (HasActiveSession())
	{
		bHostAfterDestroy = true;
		BeginDestroySession(false);
		return true;
	}
	return BeginCreateSession(Variant, PendingMaximumPlayers, PendingPurpose);
}

bool UFlickSessionSubsystem::BeginCreateSession(
	const EFlickMatchVariant Variant,
	const int32 MaximumPlayers,
	const EFlickSessionPurpose Purpose)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!Sessions.IsValid())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM SESSION INTERFACE IS UNAVAILABLE"));
		return false;
	}

	FOnlineSessionSettings Settings;
	if (Purpose == EFlickSessionPurpose::Party && PersistentPartyId.IsEmpty())
	{
		PersistentPartyId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
		PersistentPartySlot = 0;
		PersistentPartySize = 1;
		bPersistentPartyLeader = true;
	}
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = MaximumPlayers;
	Settings.NumPrivateConnections = 0;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowInvites = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bAllowJoinViaPresenceFriendsOnly = Purpose == EFlickSessionPurpose::Party;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bUseLobbiesVoiceChatIfAvailable = false;
	Settings.BuildUniqueId = GetTypeHash(FString(TEXT("FLICK_SESSION_V1")));
	Settings.Set(SETTING_MAPNAME, GetWorld() ? GetWorld()->GetOutermost()->GetName() : FString(), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickGameKey, FlickGameValue, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickVariantKey, static_cast<int32>(Variant), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickTeamSizeKey, PendingPlayersPerTeam, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickAcceptingPlayersKey, 1, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickPartyIdKey, PersistentPartyId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickPartySizeKey, FMath::Max(PendingPartySize, PersistentPartySize), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickRankedKey, bPendingRanked ? 1 : 0, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(FlickMmrKey, PendingQueueRating, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Settings.Set(
		FlickPurposeKey,
		Purpose == EFlickSessionPurpose::Party
			? FlickPartyPurpose
			: Purpose == EFlickSessionPurpose::Matchmaking ? FlickMatchmakingPurpose : FlickMatchPurpose,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	SetState(
		Purpose == EFlickSessionPurpose::Matchmaking ? EFlickSessionState::Queued : EFlickSessionState::Creating,
		Purpose == EFlickSessionPurpose::Party
			? TEXT("CREATING STEAM PARTY...")
			: Purpose == EFlickSessionPurpose::Matchmaking
				? FString::Printf(
					TEXT("NO COMPATIBLE LOBBY FOUND - OPENING A %s QUEUE..."),
					bPendingRanked ? TEXT("RANKED") : TEXT("CASUAL"))
				: TEXT("CREATING STEAM LOBBY..."));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		if (Purpose == EFlickSessionPurpose::Matchmaking)
		{
			bMatchmakingActive = false;
			bMatchmakingSearch = false;
		}
		SetState(EFlickSessionState::Error, TEXT("STEAM REJECTED THE CREATE-LOBBY REQUEST"));
		return false;
	}
	return true;
}

bool UFlickSessionSubsystem::FindSessions()
{
	if (!IsSteamAvailable() || !RegisterOnlineDelegates())
	{
		BrowserEntries.Reset();
		SetState(EFlickSessionState::Error, TEXT("STEAM IS NOT AVAILABLE. LOCAL DEVELOPMENT OPTIONS ARE BELOW."));
		return false;
	}

	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!Sessions.IsValid())
	{
		return false;
	}

	ActiveSearch = MakeShared<FOnlineSessionSearch>();
	ActiveSearch->MaxSearchResults = 50;
	ActiveSearch->PingBucketSize = 50;
	ActiveSearch->bIsLanQuery = false;
	ActiveSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	ActiveSearch->QuerySettings.Set(FlickGameKey, FlickGameValue, EOnlineComparisonOp::Equals);
	BrowserEntries.Reset();
	SetState(EFlickSessionState::Searching, TEXT("SEARCHING FOR FLICK LOBBIES..."));
	if (!Sessions->FindSessions(0, ActiveSearch.ToSharedRef()))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM REJECTED THE LOBBY SEARCH"));
		return false;
	}
	return true;
}

bool UFlickSessionSubsystem::JoinSession(const int32 ResultIndex)
{
	if (!ActiveSearch.IsValid() || !BrowserEntries.IsValidIndex(ResultIndex))
	{
		SetState(EFlickSessionState::Error, TEXT("THAT LOBBY IS NO LONGER AVAILABLE. REFRESH THE LIST."));
		return false;
	}
	const int32 SearchResultIndex = BrowserEntries[ResultIndex].SearchResultIndex;
	if (!ActiveSearch->SearchResults.IsValidIndex(SearchResultIndex))
	{
		SetState(EFlickSessionState::Error, TEXT("THAT LOBBY IS NO LONGER AVAILABLE. REFRESH THE LIST."));
		return false;
	}
	return JoinSearchResult(ActiveSearch->SearchResults[SearchResultIndex]);
}

bool UFlickSessionSubsystem::UpdateSessionVariant(const EFlickMatchVariant Variant)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession)
	{
		return false;
	}

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.Set(FlickVariantKey, static_cast<int32>(Variant), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT UPDATE THE LOBBY MODE"));
		return false;
	}
	return true;
}

bool UFlickSessionSubsystem::UpdatePartyMemberCount(const int32 PartySize)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || ActivePurpose != EFlickSessionPurpose::Party)
	{
		return false;
	}

	PersistentPartySize = FMath::Clamp(PartySize, 1, FlickMaximumPartyMembers);
	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.Set(
		FlickPartySizeKey,
		PersistentPartySize,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(
		FlickAcceptingPlayersKey,
		PersistentPartySize < FlickMaximumPartyMembers ? 1 : 0,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		UE_LOG(LogFlick, Warning, TEXT("Steam could not refresh the advertised party size"));
		return false;
	}
	OnSessionsChanged.Broadcast();
	return true;
}

bool UFlickSessionSubsystem::StartMatchmaking(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 PartySize,
	const bool bRanked,
	const int32 QueueRating)
{
	if (!IsSteamAvailable() || !RegisterOnlineDelegates())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM IS REQUIRED FOR ONLINE MATCHMAKING"));
		return false;
	}
	const int32 TeamSize = FMath::Clamp(PlayersPerTeam, 1, 3);
	if (!FlickMatchmakingRules::IsPartySizeValid(PartySize, TeamSize))
	{
		SetState(EFlickSessionState::Error, TEXT("THE CURRENT PARTY DOES NOT FIT THAT TEAM SIZE"));
		return false;
	}
	if (HasActiveSession())
	{
		SetState(EFlickSessionState::Error, TEXT("LEAVE THE CURRENT LOBBY BEFORE STARTING A SOLO QUEUE"));
		return false;
	}

	PendingHostVariant = Variant;
	PendingPlayersPerTeam = TeamSize;
	PendingPartySize = PartySize;
	PendingMaximumPlayers = FlickMatchmakingRules::GetRequiredPlayerCount(TeamSize);
	PendingPurpose = EFlickSessionPurpose::Matchmaking;
	bPendingRanked = bRanked;
	PendingQueueRating = FMath::Clamp(QueueRating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating);
	MatchmakingSearchAttempt = 0;
	CurrentRankedSearchRange = FlickRankRules::GetSearchRangeForAttempt(0);
	bMatchmakingActive = true;
	bMatchmakingSearch = true;
	if (!FindSessions())
	{
		bMatchmakingActive = false;
		bMatchmakingSearch = false;
		return false;
	}
	return true;
}

bool UFlickSessionSubsystem::ConvertPartyToMatchmakingQueue(
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const int32 PartySize,
	const bool bRanked,
	const int32 QueueRating)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const int32 TeamSize = FMath::Clamp(PlayersPerTeam, 1, 3);
	if (!Sessions.IsValid() || !HasActiveSession() || ActivePurpose != EFlickSessionPurpose::Party
		|| !FlickMatchmakingRules::IsPartySizeValid(PartySize, TeamSize))
	{
		SetState(EFlickSessionState::Error, TEXT("THE PARTY CANNOT ENTER THAT MATCHMAKING QUEUE"));
		return false;
	}
	if (PersistentPartyId.IsEmpty())
	{
		SetPersistentPartyIdentity(
			FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens),
			0,
			PartySize,
			true);
	}

	PendingHostVariant = Variant;
	PendingPlayersPerTeam = TeamSize;
	PendingPartySize = PartySize;
	PendingMaximumPlayers = TeamSize * 2;
	PendingPurpose = EFlickSessionPurpose::Matchmaking;
	bPendingRanked = bRanked;
	PendingQueueRating = FMath::Clamp(QueueRating, FlickRankRules::MinimumRating, FlickRankRules::MaximumRating);
	MatchmakingSearchAttempt = 0;
	CurrentRankedSearchRange = FlickRankRules::GetSearchRangeForAttempt(0);
	bMatchmakingActive = true;
	bMatchmakingSearch = true;
	bPartyMatchmakingSearch = true;
	SetState(
		EFlickSessionState::Searching,
		FString::Printf(
			TEXT("SEARCHING FOR A COMPATIBLE %s %dV%d PARTY MATCH..."),
			bPendingRanked ? TEXT("RANKED") : TEXT("CASUAL"),
			TeamSize,
			TeamSize));
	if (!FindSessions())
	{
		bMatchmakingActive = false;
		bMatchmakingSearch = false;
		bPartyMatchmakingSearch = false;
		return false;
	}
	return true;
}

bool UFlickSessionSubsystem::OpenCurrentPartyAsMatchmakingQueue()
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || ActivePurpose != EFlickSessionPurpose::Party)
	{
		SetState(EFlickSessionState::Error, TEXT("THE PARTY TRANSPORT WAS LOST BEFORE QUEUE CREATION"));
		return false;
	}

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.NumPublicConnections = FlickMatchmakingRules::GetRequiredPlayerCount(PendingPlayersPerTeam);
	UpdatedSettings.bShouldAdvertise = true;
	UpdatedSettings.bAllowJoinInProgress = true;
	UpdatedSettings.bAllowInvites = true;
	UpdatedSettings.bAllowJoinViaPresence = true;
	UpdatedSettings.bAllowJoinViaPresenceFriendsOnly = false;
	UpdatedSettings.Set(FlickVariantKey, static_cast<int32>(PendingHostVariant), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickTeamSizeKey, PendingPlayersPerTeam, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickAcceptingPlayersKey, 1, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickPurposeKey, FlickMatchmakingPurpose, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickPartyIdKey, PersistentPartyId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickPartySizeKey, PendingPartySize, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickRankedKey, bPendingRanked ? 1 : 0, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickMmrKey, PendingQueueRating, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT OPEN THE PARTY MATCHMAKING QUEUE"));
		return false;
	}

	ActivePurpose = EFlickSessionPurpose::Matchmaking;
	bMatchmakingActive = true;
	bMatchmakingSearch = false;
	bPartyMatchmakingSearch = false;
	SetState(
		EFlickSessionState::Queued,
		FString::Printf(
			TEXT("%s %dV%d PARTY QUEUE OPEN - WAITING FOR OPPONENTS"),
			bPendingRanked ? TEXT("RANKED") : TEXT("CASUAL"),
			PendingPlayersPerTeam,
			PendingPlayersPerTeam));
	return true;
}

bool UFlickSessionSubsystem::LockMatchmakingLobby()
{
	if (ActivePurpose != EFlickSessionPurpose::Matchmaking)
	{
		return true;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession)
	{
		return false;
	}
	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.bShouldAdvertise = false;
	UpdatedSettings.bAllowJoinInProgress = false;
	UpdatedSettings.Set(FlickAcceptingPlayersKey, 0, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT LOCK THE FULL MATCHMAKING LOBBY"));
		return false;
	}
	bMatchmakingActive = false;
	SetState(EFlickSessionState::InSession, TEXT("MATCH FOUND - WAITING FOR PLAYER CONFIRMATION"));
	return true;
}

bool UFlickSessionSubsystem::ReopenMatchmakingLobby()
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || ActivePurpose != EFlickSessionPurpose::Matchmaking)
	{
		return false;
	}

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.bShouldAdvertise = true;
	UpdatedSettings.bAllowJoinInProgress = true;
	UpdatedSettings.Set(FlickAcceptingPlayersKey, 1, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT REOPEN THE MATCHMAKING QUEUE"));
		return false;
	}

	bMatchmakingActive = true;
	SetState(
		EFlickSessionState::Queued,
		FString::Printf(
			TEXT("%s %dV%d QUEUE REOPENED - WAITING FOR PLAYERS"),
			bPendingRanked ? TEXT("RANKED") : TEXT("CASUAL"),
			PendingPlayersPerTeam,
			PendingPlayersPerTeam));
	return true;
}

bool UFlickSessionSubsystem::ConvertMatchmakingToParty(const int32 MaximumPartyMembers)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || ActivePurpose != EFlickSessionPurpose::Matchmaking)
	{
		return false;
	}
	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.NumPublicConnections = FMath::Max(
		FMath::Clamp(MaximumPartyMembers, 1, FlickMaximumPartyMembers),
		NamedSession->RegisteredPlayers.Num());
	UpdatedSettings.bShouldAdvertise = true;
	UpdatedSettings.bAllowJoinInProgress = true;
	UpdatedSettings.bAllowJoinViaPresenceFriendsOnly = true;
	UpdatedSettings.Set(FlickAcceptingPlayersKey, 1, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickPurposeKey, FlickPartyPurpose, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickPartyIdKey, PersistentPartyId, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	UpdatedSettings.Set(FlickPartySizeKey, PersistentPartySize, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	if (!Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT RESTORE THE PRE-MATCH PARTY"));
		return false;
	}
	ActivePurpose = EFlickSessionPurpose::Party;
	bMatchmakingActive = false;
	SetState(EFlickSessionState::InSession, TEXT("PARTY RESTORED AFTER MATCH"));
	return true;
}

bool UFlickSessionSubsystem::CancelMatchmaking(const bool bReturnToFrontend)
{
	bMatchmakingActive = false;
	bMatchmakingSearch = false;
	bPartyMatchmakingSearch = false;
	bRankedRetryScheduled = false;
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (State == EFlickSessionState::Searching && Sessions.IsValid())
	{
		bIgnoreNextFindCompletion = true;
		Sessions->CancelFindSessions();
	}
	if (HasActiveSession() && ActivePurpose == EFlickSessionPurpose::Matchmaking)
	{
		return LeaveSession(bReturnToFrontend);
	}
	if (HasActiveSession() && ActivePurpose == EFlickSessionPurpose::Party)
	{
		SetState(EFlickSessionState::InSession, TEXT("MATCHMAKING CANCELLED - PARTY PRESERVED"));
		return true;
	}
	SetState(EFlickSessionState::Idle, TEXT("MATCHMAKING CANCELLED"));
	return true;
}

bool UFlickSessionSubsystem::BeginPartyMatchMigration(
	const FString& TargetSessionId,
	const FString& PartyId,
	const int32 PartySlot,
	const int32 PartySize,
	const bool bLeader)
{
	if (TargetSessionId.IsEmpty() || PartyId.IsEmpty())
	{
		SetState(EFlickSessionState::Error, TEXT("THE MATCHMAKING PARTY HANDOFF WAS INCOMPLETE"));
		return false;
	}
	SetPersistentPartyIdentity(PartyId, PartySlot, PartySize, bLeader);
	PendingTargetSessionId = TargetSessionId;
	bFindTargetAfterDestroy = false;
	bTargetSessionSearch = false;
	bMatchmakingActive = true;
	if (HasActiveSession())
	{
		bFindTargetAfterDestroy = true;
		BeginDestroySession(false);
		return true;
	}
	return BeginTargetSessionSearch();
}

bool UFlickSessionSubsystem::BeginTargetSessionSearch()
{
	if (PendingTargetSessionId.IsEmpty())
	{
		return false;
	}
	bTargetSessionSearch = true;
	bMatchmakingSearch = false;
	if (!FindSessions())
	{
		bTargetSessionSearch = false;
		return false;
	}
	SetState(EFlickSessionState::Searching, TEXT("MOVING THE PARTY INTO THE FOUND MATCH..."));
	return true;
}

bool UFlickSessionSubsystem::RestorePersistentParty(const bool bLeader)
{
	if (!HasPersistentPartyIdentity())
	{
		return LeaveSession(true);
	}
	bRestoreAsPartyLeader = bLeader;
	if (bRestoreAsPartyLeader)
	{
		bPersistentPartyLeader = true;
	}
	bRestorePartyAfterDestroy = false;
	bPartyRestoreSearch = false;
	PartyRestoreAttempts = 0;
	if (HasActiveSession())
	{
		bRestorePartyAfterDestroy = true;
		BeginDestroySession(false);
		return true;
	}
	if (bRestoreAsPartyLeader)
	{
		PendingMaximumPlayers = FlickMaximumPartyMembers;
		PendingPartySize = PersistentPartySize;
		PendingPurpose = EFlickSessionPurpose::Party;
		return BeginCreateSession(EFlickMatchVariant::Classic, PendingMaximumPlayers, PendingPurpose);
	}
	return BeginPartyRestoreSearch();
}

bool UFlickSessionSubsystem::BeginPartyRestoreSearch()
{
	if (!HasPersistentPartyIdentity())
	{
		return false;
	}
	++PartyRestoreAttempts;
	bPartyRestoreSearch = true;
	bMatchmakingSearch = false;
	if (!FindSessions())
	{
		bPartyRestoreSearch = false;
		SchedulePartyRestoreRetry();
		return false;
	}
	SetState(EFlickSessionState::Searching, TEXT("RECONNECTING TO YOUR PARTY..."));
	return true;
}

void UFlickSessionSubsystem::SchedulePartyRestoreRetry()
{
	if (!GetWorld())
	{
		return;
	}
	if (PartyRestoreAttempts >= 15)
	{
		SetState(EFlickSessionState::Error, TEXT("YOUR PARTY COULD NOT BE RESTORED - RETURNING TO THE FRONTEND"));
		ClearPersistentPartyIdentity();
		TravelToFrontend();
		return;
	}
	const int32 TakeoverAttempt = 3 + FMath::Max(0, PersistentPartySlot - 1) * 2;
	if (!bPersistentPartyLeader && PersistentPartySlot > 0 && PartyRestoreAttempts >= TakeoverAttempt)
	{
		bPersistentPartyLeader = true;
		bRestoreAsPartyLeader = true;
		PendingMaximumPlayers = FlickMaximumPartyMembers;
		PendingPartySize = FMath::Max(PersistentPartySize - 1, 1);
		PendingPurpose = EFlickSessionPurpose::Party;
		SetState(EFlickSessionState::Creating, TEXT("PARTY HOST LOST - REBUILDING THE PARTY..."));
		BeginCreateSession(EFlickMatchVariant::Classic, PendingMaximumPlayers, PendingPurpose);
		return;
	}
	FTimerHandle RetryTimer;
	GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]() { BeginPartyRestoreSearch(); }, 1.0f, false);
}

void UFlickSessionSubsystem::ScheduleRankedSearchRetry()
{
	if (!GetWorld() || !bMatchmakingActive || !bMatchmakingSearch || !bPendingRanked)
	{
		return;
	}
	bRankedRetryScheduled = true;
	CurrentRankedSearchRange = FlickRankRules::GetSearchRangeForAttempt(MatchmakingSearchAttempt);
	SetState(
		EFlickSessionState::Searching,
		FString::Printf(
			TEXT("RANKED SEARCH EXPANDED TO +/-%d MMR..."),
			CurrentRankedSearchRange));
	FTimerHandle RetryTimer;
	GetWorld()->GetTimerManager().SetTimer(RetryTimer, [this]()
	{
		bRankedRetryScheduled = false;
		if (bMatchmakingActive && bMatchmakingSearch && bPendingRanked)
		{
			if (!FindSessions())
			{
				bMatchmakingActive = false;
				bMatchmakingSearch = false;
			}
		}
	}, 1.5f, false);
}

bool UFlickSessionSubsystem::JoinSearchResult(const FOnlineSessionSearchResult& SearchResult)
{
	if (!RegisterOnlineDelegates())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM SESSION INTERFACE IS UNAVAILABLE"));
		return false;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!Sessions.IsValid())
	{
		return false;
	}

	if (HasActiveSession())
	{
		SetState(EFlickSessionState::Error, TEXT("LEAVE THE CURRENT SESSION BEFORE JOINING ANOTHER"));
		return false;
	}
	FString Purpose;
	SearchResult.Session.SessionSettings.Get(FlickPurposeKey, Purpose);
	ActivePurpose = Purpose.Equals(FlickPartyPurpose, ESearchCase::IgnoreCase)
		? EFlickSessionPurpose::Party
		: Purpose.Equals(FlickMatchmakingPurpose, ESearchCase::IgnoreCase)
			? EFlickSessionPurpose::Matchmaking
			: EFlickSessionPurpose::Match;
	SearchResult.Session.SessionSettings.Get(FlickTeamSizeKey, PendingPlayersPerTeam);
	PendingPlayersPerTeam = FMath::Clamp(PendingPlayersPerTeam, 1, 3);
	int32 VariantValue = static_cast<int32>(EFlickMatchVariant::Classic);
	int32 RankedValue = 0;
	int32 AdvertisedRating = FlickRankRules::DefaultRating;
	SearchResult.Session.SessionSettings.Get(FlickVariantKey, VariantValue);
	SearchResult.Session.SessionSettings.Get(FlickRankedKey, RankedValue);
	SearchResult.Session.SessionSettings.Get(FlickMmrKey, AdvertisedRating);
	PendingHostVariant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(FMath::Clamp(
		VariantValue,
		static_cast<int32>(EFlickMatchVariant::Classic),
		static_cast<int32>(EFlickMatchVariant::Blitz))));
	bPendingRanked = ActivePurpose == EFlickSessionPurpose::Matchmaking && RankedValue != 0;
	if (bPendingRanked)
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const UFlickRankingSubsystem* Ranking = GameInstance->GetSubsystem<UFlickRankingSubsystem>())
			{
				PendingQueueRating = Ranking->GetQueueRating(PendingHostVariant, PendingPlayersPerTeam);
			}
		}
	}
	else
	{
		PendingQueueRating = FMath::Clamp(AdvertisedRating, 0, 3000);
	}
	if (ActivePurpose == EFlickSessionPurpose::Party)
	{
		SearchResult.Session.SessionSettings.Get(FlickPartyIdKey, PersistentPartyId);
		SearchResult.Session.SessionSettings.Get(FlickPartySizeKey, PersistentPartySize);
		PersistentPartySize = FMath::Clamp(PersistentPartySize, 1, FlickMaximumPartyMembers);
		bPersistentPartyLeader = false;
	}
	bMatchmakingActive = ActivePurpose == EFlickSessionPurpose::Matchmaking;

	SetState(EFlickSessionState::Joining, FString::Printf(TEXT("JOINING %s..."), *SearchResult.Session.OwningUserName));
	if (!Sessions->JoinSession(0, NAME_GameSession, SearchResult))
	{
		bMatchmakingActive = false;
		SetState(EFlickSessionState::Error, TEXT("STEAM REJECTED THE JOIN REQUEST"));
		return false;
	}
	if (ActivePurpose == EFlickSessionPurpose::Party && GetWorld())
	{
		bPartyJoinTimedOut = false;
		GetWorld()->GetTimerManager().SetTimer(
			PartyJoinTimeoutTimer, this, &UFlickSessionSubsystem::HandlePartyJoinTimeout, 15.0f, false);
	}
	return true;
}

void UFlickSessionSubsystem::HandlePartyJoinTimeout()
{
	if (ActivePurpose != EFlickSessionPurpose::Party || State != EFlickSessionState::Joining)
	{
		return;
	}
	UE_LOG(LogFlick, Error, TEXT("Steam party JoinSession callback timed out after 15 seconds"));
	bPartyJoinTimedOut = true;
	if (HasActiveSession())
	{
		BeginDestroySession(false);
	}
	else
	{
		ActivePurpose = EFlickSessionPurpose::Match;
		SetState(EFlickSessionState::Error, TEXT("STEAM PARTY JOIN TIMED OUT - TRY THE INVITE AGAIN"));
	}
}

bool UFlickSessionSubsystem::LeaveSession(const bool bReturnToFrontend)
{
	const bool bLeavingParty = ActivePurpose == EFlickSessionPurpose::Party;
	if (bReturnToFrontend && ActivePurpose == EFlickSessionPurpose::Matchmaking && HasPersistentPartyIdentity())
	{
		return RestorePersistentParty(bPersistentPartyLeader);
	}
	if (bLeavingParty)
	{
		StopPartySynchronization();
		if (UFlickPartySubsystem* Party = GetPartySubsystem())
		{
			Party->Clear();
		}
		ClearPersistentPartyIdentity();
	}
	bHostAfterDestroy = false;
	bJoinAfterDestroy = false;
	PendingJoinResult.Reset();
	if (!HasActiveSession())
	{
		bDisbandingParty = false;
		SetState(EFlickSessionState::Idle, TEXT("SESSION CLOSED"));
		if (bReturnToFrontend && !bLeavingParty)
		{
			TravelToFrontend();
		}
		return true;
	}
	BeginDestroySession(bReturnToFrontend && !bLeavingParty);
	return true;
}

void UFlickSessionSubsystem::BeginDestroySession(const bool bReturnToFrontend)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	bReturnAfterDestroy = bReturnToFrontend;
	SetState(EFlickSessionState::Destroying, TEXT("LEAVING STEAM LOBBY..."));
	if (!Sessions.IsValid() || !Sessions->DestroySession(NAME_GameSession))
	{
		const bool bShouldReturn = bReturnAfterDestroy;
		bReturnAfterDestroy = false;
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT CLEANLY CLOSE THE LOBBY"));
		if (bShouldReturn && ActivePurpose != EFlickSessionPurpose::Party)
		{
			TravelToFrontend();
		}
	}
}

bool UFlickSessionSubsystem::OpenInviteOverlay()
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineExternalUIPtr ExternalUi = OnlineSubsystem ? OnlineSubsystem->GetExternalUIInterface() : nullptr;
	if (!HasActiveSession() || !ExternalUi.IsValid() || !ExternalUi->ShowInviteUI(0, NAME_GameSession))
	{
		SetState(EFlickSessionState::Error, TEXT("THE STEAM INVITE OVERLAY IS NOT AVAILABLE"));
		return false;
	}
	SetState(EFlickSessionState::InSession, TEXT("STEAM INVITE OVERLAY OPENED"));
	return true;
}

bool UFlickSessionSubsystem::RefreshFriends()
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	if (!IsSteamAvailable() || !FriendsInterface.IsValid())
	{
		Friends.Reset();
		OnSessionsChanged.Broadcast();
		return false;
	}

	return FriendsInterface->ReadFriendsList(
		0,
		EFriendsLists::ToString(EFriendsLists::Default),
		FOnReadFriendsListComplete::CreateUObject(this, &UFlickSessionSubsystem::HandleReadFriendsComplete));
}

void UFlickSessionSubsystem::HandleReadFriendsComplete(
	const int32 LocalUserNum,
	const bool bWasSuccessful,
	const FString& ListName,
	const FString& ErrorString)
{
	Friends.Reset();
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineFriendsPtr FriendsInterface = OnlineSubsystem ? OnlineSubsystem->GetFriendsInterface() : nullptr;
	TArray<TSharedRef<FOnlineFriend>> OnlineFriends;
	if (bWasSuccessful && FriendsInterface.IsValid()
		&& FriendsInterface->GetFriendsList(LocalUserNum, ListName, OnlineFriends))
	{
		for (const TSharedRef<FOnlineFriend>& OnlineFriend : OnlineFriends)
		{
			const FOnlineUserPresence& Presence = OnlineFriend->GetPresence();
			FFlickSocialPlayerEntry& Entry = Friends.AddDefaulted_GetRef();
			Entry.DisplayName = OnlineFriend->GetDisplayName();
			Entry.UserId = OnlineFriend->GetUserId()->ToString();
			// Steam's aggregate bIsOnline flag has reported stale/optimistic values in
			// some presence payloads. The explicit state is what the Steam friends UI
			// uses to distinguish the ALL and ONLINE lists.
			Entry.bOnline = Presence.Status.State != EOnlinePresenceState::Offline;
			Entry.bPlayingFlick = Presence.bIsPlayingThisGame;
			Entry.bJoinable = Presence.bIsJoinable;
			Entry.Status = Entry.bPlayingFlick
				? TEXT("PLAYING FLICK")
				: !Presence.Status.StatusStr.IsEmpty()
					? Presence.Status.StatusStr.ToUpper()
					: Entry.bOnline ? TEXT("ONLINE") : TEXT("OFFLINE");
		}

#if WITH_FLICK_STEAMWORKS
		// The generic Steam OSS friends list can omit offline users or expose stale
		// aggregate presence. Build the visible roster from Steam's immediate-friend
		// relationship and persona state—the same source used by the Steam client.
		if (SteamFriends() && SteamUtils())
		{
			Friends.Reset();
			const AppId_t CurrentAppId = SteamUtils()->GetAppID();
			const int32 FriendCount = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
			for (int32 FriendIndex = 0; FriendIndex < FriendCount; ++FriendIndex)
			{
				const CSteamID SteamId = SteamFriends()->GetFriendByIndex(FriendIndex, k_EFriendFlagImmediate);
				if (!SteamId.IsValid()) continue;
				const EPersonaState PersonaState = SteamFriends()->GetFriendPersonaState(SteamId);
				FriendGameInfo_t GameInfo;
				const bool bHasGame = SteamFriends()->GetFriendGamePlayed(SteamId, &GameInfo);
				FFlickSocialPlayerEntry& Entry = Friends.AddDefaulted_GetRef();
				Entry.DisplayName = UTF8_TO_TCHAR(SteamFriends()->GetFriendPersonaName(SteamId));
				Entry.UserId = FString::Printf(TEXT("%llu"), SteamId.ConvertToUint64());
				Entry.bOnline = PersonaState != k_EPersonaStateOffline;
				Entry.bPlayingFlick = bHasGame && GameInfo.m_gameID.AppID() == CurrentAppId;
				Entry.bJoinable = Entry.bPlayingFlick && GameInfo.m_steamIDLobby.IsValid();
				Entry.Status = Entry.bPlayingFlick ? TEXT("PLAYING FLICK")
					: Entry.bOnline ? TEXT("ONLINE") : TEXT("OFFLINE");
			}
		}
#endif
		Friends.StableSort([](const FFlickSocialPlayerEntry& Left, const FFlickSocialPlayerEntry& Right)
		{
			if (Left.bPlayingFlick != Right.bPlayingFlick)
			{
				return Left.bPlayingFlick;
			}
			if (Left.bOnline != Right.bOnline)
			{
				return Left.bOnline;
			}
			return Left.DisplayName < Right.DisplayName;
		});
		int32 OnlineCount = 0;
		for (const FFlickSocialPlayerEntry& Entry : Friends) OnlineCount += Entry.bOnline ? 1 : 0;
		int32 bNativeRoster = 0;
#if WITH_FLICK_STEAMWORKS
		bNativeRoster = SteamFriends() != nullptr ? 1 : 0;
#endif
		UE_LOG(LogFlick, Log, TEXT("Steam social roster: all=%d online=%d offline=%d native=%d"), Friends.Num(), OnlineCount, Friends.Num() - OnlineCount, bNativeRoster);
		AvatarRefreshAttempts = 0;
		RefreshSteamAvatarCache();
		SetState(
			HasActiveSession() ? EFlickSessionState::InSession : EFlickSessionState::Idle,
			FString::Printf(TEXT("%d STEAM FRIENDS AVAILABLE"), Friends.Num()));
	}
	else
	{
		UE_LOG(LogFlick, Warning, TEXT("Steam friends refresh failed: %s"), *ErrorString);
		SetState(EFlickSessionState::Error, TEXT("STEAM FRIENDS COULD NOT BE REFRESHED"));
	}
}

bool UFlickSessionSubsystem::InviteFriendToParty(const int32 FriendIndex)
{
	if (!Friends.IsValidIndex(FriendIndex) || !Friends[FriendIndex].bOnline)
	{
		SetState(EFlickSessionState::Error, TEXT("THAT FRIEND IS NOT AVAILABLE FOR A PARTY INVITE"));
		return false;
	}

	const FFlickSocialPlayerEntry& Friend = Friends[FriendIndex];
	return InvitePlayerToParty(Friend.UserId, Friend.DisplayName);
}

bool UFlickSessionSubsystem::InviteRecentPlayerToParty(const int32 RecentPlayerIndex)
{
	if (!RecentPlayers.IsValidIndex(RecentPlayerIndex))
	{
		SetState(EFlickSessionState::Error, TEXT("THAT RECENT PLAYER IS NO LONGER AVAILABLE"));
		return false;
	}
	const FFlickRecentPlayerEntry& RecentPlayer = RecentPlayers[RecentPlayerIndex];
	return InvitePlayerToParty(RecentPlayer.UserId, RecentPlayer.DisplayName);
}

bool UFlickSessionSubsystem::InvitePlayerToParty(const FString& UserId, const FString& DisplayName)
{
	if (UserId.IsEmpty())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT IDENTIFY THAT PLAYER"));
		return false;
	}
	if (HasActiveSession())
	{
		IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
		const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
		const FNamedOnlineSession* PartySession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
		if (PartySession && PartySession->NumOpenPublicConnections <= 0)
		{
			SetState(EFlickSessionState::Error, TEXT("YOUR LOBBY IS FULL"));
			return false;
		}
		return SendPartyInvite(UserId, DisplayName);
	}

	PendingPartyInviteUserId = UserId;
	PendingPartyInviteDisplayName = DisplayName;
	PendingHostVariant = EFlickMatchVariant::Classic;
	PendingMaximumPlayers = FlickMaximumPartyMembers;
	PendingPartySize = 1;
	PendingPurpose = EFlickSessionPurpose::Party;
	bPendingRanked = false;
	return BeginCreateSession(PendingHostVariant, PendingMaximumPlayers, PendingPurpose);
}

bool UFlickSessionSubsystem::SendPendingPartyInvite()
{
	if (PendingPartyInviteUserId.IsEmpty())
	{
		return false;
	}
	const FString UserId = PendingPartyInviteUserId;
	const FString DisplayName = PendingPartyInviteDisplayName;
	PendingPartyInviteUserId.Reset();
	PendingPartyInviteDisplayName.Reset();
	return SendPartyInvite(UserId, DisplayName);
}

bool UFlickSessionSubsystem::SendPartyInvite(const FString& FriendUserId, const FString& FriendDisplayName)
{
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr FriendId = Identity.IsValid() ? Identity->CreateUniquePlayerId(FriendUserId) : nullptr;
	if (!HasActiveSession() || !Sessions.IsValid() || !FriendId.IsValid()
		|| !Sessions->SendSessionInviteToFriend(0, NAME_GameSession, *FriendId))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT SEND THAT LOBBY INVITE"));
		return false;
	}
	SetState(EFlickSessionState::InSession, FString::Printf(TEXT("LOBBY INVITE SENT TO %s"), *FriendDisplayName.ToUpper()));
	return true;
}

bool UFlickSessionSubsystem::PromotePartyMember(const FString& UserId)
{
	UFlickPartySubsystem* Party = GetPartySubsystem();
	if (!Party || !Party->IsLocalLeader() || UserId.IsEmpty() || UserId == Party->GetLocalUserId())
	{
		SetState(EFlickSessionState::Error, TEXT("ONLY THE PARTY LEADER CAN PROMOTE ANOTHER MEMBER"));
		return false;
	}
#if WITH_FLICK_STEAMWORKS
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	const uint64 LobbyId = NamedSession ? GetSteamLobbyId(NamedSession->GetSessionIdStr()) : 0;
	const uint64 TargetId = FCString::Strtoui64(*UserId, nullptr, 10);
	if (LobbyId != 0 && TargetId != 0 && SteamMatchmaking()
		&& SteamMatchmaking()->SetLobbyOwner(CSteamID(LobbyId), CSteamID(TargetId)))
	{
		SetState(EFlickSessionState::InSession, TEXT("TRANSFERRING PARTY LEADERSHIP..."));
		RefreshPartyFromSession();
		return true;
	}
#endif
	SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT TRANSFER PARTY LEADERSHIP"));
	return false;
}

bool UFlickSessionSubsystem::RemovePartyMember(const FString& UserId)
{
	UFlickPartySubsystem* Party = GetPartySubsystem();
	if (!Party || !Party->IsLocalLeader() || UserId.IsEmpty() || UserId == Party->GetLocalUserId())
	{
		return false;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession)
	{
		return false;
	}
	FOnlineSessionSettings Settings = NamedSession->SessionSettings;
	Settings.Set(FlickPartyCommandKey,
		FString::Printf(TEXT("KICK:%s:%s"), *UserId, *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
		EOnlineDataAdvertisementType::ViaOnlineService);
	if (!Sessions->UpdateSession(NAME_GameSession, Settings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT REMOVE THAT PARTY MEMBER"));
		return false;
	}
	SetState(EFlickSessionState::InSession, TEXT("REMOVING PARTY MEMBER..."));
	return true;
}

bool UFlickSessionSubsystem::DisbandParty()
{
	UFlickPartySubsystem* Party = GetPartySubsystem();
	if (!Party || !Party->IsLocalLeader())
	{
		return false;
	}
	bDisbandingParty = true;
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession)
	{
		return LeaveParty();
	}
	FOnlineSessionSettings Settings = NamedSession->SessionSettings;
	Settings.Set(FlickPartyCommandKey,
		FString::Printf(TEXT("DISBAND:%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
		EOnlineDataAdvertisementType::ViaOnlineService);
	Sessions->UpdateSession(NAME_GameSession, Settings, true);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(PartyCommandTimer, [this]() { LeaveParty(); }, 0.5f, false);
		return true;
	}
	return LeaveParty();
}

bool UFlickSessionSubsystem::LeaveParty()
{
	if (ActivePurpose != EFlickSessionPurpose::Party)
	{
		return false;
	}
	TransferPartyLeadershipBeforeLeaving();
	return LeaveSession(false);
}

void UFlickSessionSubsystem::TransferPartyLeadershipBeforeLeaving()
{
#if WITH_FLICK_STEAMWORKS
	const UFlickPartySubsystem* Party = GetPartySubsystem();
	if (ActivePurpose != EFlickSessionPurpose::Party || bDisbandingParty || !Party || !Party->IsLocalLeader()
		|| Party->GetMemberCount() < 2 || !SteamMatchmaking())
	{
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	const uint64 LobbyId = NamedSession ? GetSteamLobbyId(NamedSession->GetSessionIdStr()) : 0;
	if (LobbyId == 0)
	{
		return;
	}
	const CSteamID Lobby(LobbyId);
	const int32 LobbyMemberCount = SteamMatchmaking()->GetNumLobbyMembers(Lobby);
	for (const FFlickPartyMember& Member : Party->GetMembers())
	{
		if (Member.UserId == Party->GetLocalUserId())
		{
			continue;
		}
		const uint64 MemberId = FCString::Strtoui64(*Member.UserId, nullptr, 10);
		for (int32 Index = 0; MemberId != 0 && Index < LobbyMemberCount; ++Index)
		{
			if (SteamMatchmaking()->GetLobbyMemberByIndex(Lobby, Index).ConvertToUint64() == MemberId)
			{
				if (SteamMatchmaking()->SetLobbyOwner(Lobby, CSteamID(MemberId)))
				{
					UE_LOG(LogFlick, Log, TEXT("Transferred Steam party leadership to %s before leaving"), *Member.UserId);
				}
				else
				{
					UE_LOG(LogFlick, Warning, TEXT("Steam could not transfer party leadership before departure"));
				}
				return;
			}
		}
	}
#endif
}

bool UFlickSessionSubsystem::BeginPrivateMatchForParty()
{
	UFlickPartySubsystem* Party = GetPartySubsystem();
	UWorld* World = GetWorld();
	AFlickGameMode* GameMode = World ? World->GetAuthGameMode<AFlickGameMode>() : nullptr;
	if (!Party || !Party->IsActive() || !Party->IsLocalLeader() || !World || !GameMode)
	{
		SetState(EFlickSessionState::Error, TEXT("ONLY THE PARTY LEADER CAN START A PRIVATE MATCH"));
		return false;
	}

	if (!GameMode->HostPrivateMatchForPersistentParty(Party->GetPartyId(), Party->GetMemberCount()))
	{
		SetState(EFlickSessionState::Error, TEXT("THE PRIVATE MATCH LOBBY COULD NOT OPEN"));
		return false;
	}

	SetState(EFlickSessionState::InSession, TEXT("CONFIGURING PRIVATE MATCH"));
	return true;
}

bool UFlickSessionSubsystem::LaunchPrivateMatchForParty()
{
	UFlickPartySubsystem* Party = GetPartySubsystem();
	UWorld* World = GetWorld();
	if (!Party || !Party->IsActive() || !Party->IsLocalLeader() || !World)
	{
		SetState(EFlickSessionState::Error, TEXT("ONLY THE PARTY LEADER CAN LAUNCH A PRIVATE MATCH"));
		return false;
	}
	const FString CurrentMap = World->GetOutermost()->GetName();
	FURL ListenUrl(nullptr, *CurrentMap, TRAVEL_Absolute);
	ListenUrl.AddOption(TEXT("listen"));
	ListenUrl.AddOption(TEXT("FlickParty"));
	if (!World->GetNetDriver() && !World->Listen(ListenUrl))
	{
		SetState(EFlickSessionState::Error, TEXT("THE PRIVATE MATCH HOST COULD NOT START"));
		return false;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession)
	{
		return false;
	}
	FOnlineSessionSettings Settings = NamedSession->SessionSettings;
	Settings.Set(FlickPartyCommandKey,
		FString::Printf(TEXT("PRIVATE:%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
		EOnlineDataAdvertisementType::ViaOnlineService);
	if (!Sessions->UpdateSession(NAME_GameSession, Settings, true))
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT SIGNAL THE PRIVATE MATCH"));
		return false;
	}
	SetState(EFlickSessionState::InSession, TEXT("PRIVATE MATCH STARTING"));
	return true;
}

void UFlickSessionSubsystem::ResumePartySynchronizationAfterTravel()
{
	if (bAwaitingPrivateMatchTravel && ActivePurpose == EFlickSessionPurpose::Party && GetPartySubsystem())
	{
		bAwaitingPrivateMatchTravel = false;
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PrivateMatchJoinTimeoutTimer);
		StartPartySynchronization();
	}
}

bool UFlickSessionSubsystem::PublishPartyCoordinatorAllocation(const FFlickCoordinatorAllocation& Allocation)
{
	UFlickPartySubsystem* Party = GetPartySubsystem();
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	if (!Party || !Party->IsActive() || !Party->IsLocalLeader() || !NamedSession
		|| Allocation.MatchId.IsEmpty() || Allocation.Address.IsEmpty())
	{
		SetState(EFlickSessionState::Error, TEXT("THE PARTY MATCH ALLOCATION WAS INCOMPLETE"));
		return false;
	}

	TArray<FString> EncodedReservations;
	for (const FFlickCoordinatorReservation& Reservation : Allocation.Reservations)
	{
		EncodedReservations.Add(FString::Printf(
			TEXT("%s|%s|%d|%d"),
			*FGenericPlatformHttp::UrlEncode(Reservation.AccountId),
			*FGenericPlatformHttp::UrlEncode(Reservation.Token),
			static_cast<int32>(Reservation.Team),
			Reservation.PlayerSlot));
	}
	const FString Command = FString::Printf(
		TEXT("MATCH:%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	FOnlineSessionSettings Settings = NamedSession->SessionSettings;
	Settings.Set(FlickPartyMatchIdKey, Allocation.MatchId, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyServerIdKey, Allocation.ServerId, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyAddressKey, Allocation.Address, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyMatchVariantKey, static_cast<int32>(Allocation.Variant), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyMatchTeamSizeKey, Allocation.PlayersPerTeam, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyMatchRankedKey, Allocation.bRanked ? 1 : 0, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyMatchExpiryKey, LexToString(Allocation.ExpiresUnixTime), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyReservationsKey, FString::Join(EncodedReservations, TEXT(";")), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(FlickPartyCommandKey, Command, EOnlineDataAdvertisementType::ViaOnlineService);
	LastProcessedPartyCommand = Command;
	if (!Sessions->UpdateSession(NAME_GameSession, Settings, true))
	{
		LastProcessedPartyCommand.Reset();
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT SIGNAL THE FOUND MATCH TO THE PARTY"));
		return false;
	}

	const FFlickCoordinatorReservation* LocalReservation = Allocation.Reservations.FindByPredicate(
		[Party](const FFlickCoordinatorReservation& Reservation)
		{
			return Reservation.AccountId == Party->GetLocalUserId();
		});
	UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>() : nullptr;
	const FFlickPartyMember* LocalMember = Party->GetMemberByUserId(Party->GetLocalUserId());
	APlayerController* LocalController = GetGameInstance()
		? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!LocalReservation || !Coordinator || !LocalController)
	{
		SetState(EFlickSessionState::Error, TEXT("THE FOUND MATCH DID NOT RESERVE A PARTY LEADER SLOT"));
		return false;
	}
	Coordinator->AdoptLocalReservation(
		Allocation,
		*LocalReservation,
		Party->GetPartyId(),
		LocalMember ? LocalMember->Slot : 0,
		Party->GetMemberCount(),
		true);
	if (UFlickGameInstance* FlickGameInstance = Cast<UFlickGameInstance>(GetGameInstance()))
	{
		FlickGameInstance->PrepareTravelPresentation(TEXT("MATCH FOUND"));
	}
	StopPartySynchronization();
	SetState(EFlickSessionState::InSession, TEXT("MATCH FOUND - JOINING DEDICATED SERVER"));
	return Coordinator->TravelToAllocatedMatch(LocalController);
}

void UFlickSessionSubsystem::RecordRecentPlayer(const FString& UserId, const FString& DisplayName)
{
	if (UserId.IsEmpty() || DisplayName.IsEmpty())
	{
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr LocalId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	if (LocalId.IsValid() && UserId == LocalId->ToString())
	{
		return;
	}

	RecentPlayers.RemoveAll([&UserId](const FFlickRecentPlayerEntry& Entry) { return Entry.UserId == UserId; });
	FFlickRecentPlayerEntry Entry;
	Entry.DisplayName = DisplayName;
	Entry.UserId = UserId;
	Entry.LastEncounteredUtc = FDateTime::UtcNow();
	RecentPlayers.Insert(MoveTemp(Entry), 0);
	if (RecentPlayers.Num() > MaximumRecentPlayers)
	{
		RecentPlayers.SetNum(MaximumRecentPlayers);
	}
	SaveRecentPlayers();
	OnSessionsChanged.Broadcast();
}

void UFlickSessionSubsystem::LoadRecentPlayers()
{
	RecentPlayers.Reset();
	TArray<FString> SavedEntries;
	GConfig->GetArray(FlickSocialSection, TEXT("RecentPlayers"), SavedEntries, GGameUserSettingsIni);
	for (const FString& SavedEntry : SavedEntries)
	{
		TArray<FString> Fields;
		SavedEntry.ParseIntoArray(Fields, TEXT("|"), false);
		if (Fields.Num() != 3 || Fields[0].IsEmpty() || Fields[1].IsEmpty())
		{
			continue;
		}
		FFlickRecentPlayerEntry& Entry = RecentPlayers.AddDefaulted_GetRef();
		Entry.UserId = Fields[0];
		Entry.DisplayName = Fields[1];
		Entry.LastEncounteredUtc = FDateTime::FromUnixTimestamp(FCString::Atoi64(*Fields[2]));
	}
}

void UFlickSessionSubsystem::SaveRecentPlayers() const
{
	TArray<FString> SavedEntries;
	for (const FFlickRecentPlayerEntry& Entry : RecentPlayers)
	{
		FString SafeName = Entry.DisplayName;
		SafeName.ReplaceInline(TEXT("|"), TEXT(" "));
		SavedEntries.Add(FString::Printf(
			TEXT("%s|%s|%lld"),
			*Entry.UserId,
			*SafeName,
			Entry.LastEncounteredUtc.ToUnixTimestamp()));
	}
	GConfig->SetArray(FlickSocialSection, TEXT("RecentPlayers"), SavedEntries, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UFlickSessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession)
	{
		return;
	}
	if (!bWasSuccessful || !GetWorld())
	{
		if (PendingPurpose == EFlickSessionPurpose::Matchmaking)
		{
			bMatchmakingActive = false;
			bMatchmakingSearch = false;
		}
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT CREATE THE LOBBY"));
		return;
	}

	ActivePurpose = PendingPurpose;
	SetState(
		ActivePurpose == EFlickSessionPurpose::Matchmaking ? EFlickSessionState::Queued : EFlickSessionState::InSession,
		ActivePurpose == EFlickSessionPurpose::Party
			? TEXT("STEAM PARTY CREATED")
			: ActivePurpose == EFlickSessionPurpose::Matchmaking
				? FString::Printf(
					TEXT("%s QUEUE OPEN - WAITING FOR COMPATIBLE PLAYERS..."),
					bPendingRanked ? TEXT("RANKED") : TEXT("CASUAL"))
				: TEXT("STEAM LOBBY CREATED - OPENING FLICK LOBBY..."));
	const FString CurrentMap = GetWorld()->GetOutermost()->GetName();
	FString TravelOptions;
	if (ActivePurpose == EFlickSessionPurpose::Party)
	{
		// Party membership is a Steam lobby concern, not a gameplay connection.
		// Keep the current frontend world and HUD alive on every member's machine.
		StartPartySynchronization();
		SendPendingPartyInvite();
		SetState(EFlickSessionState::InSession, TEXT("STEAM PARTY READY"));
		return;
	}
	else
	{
		TravelOptions = FString::Printf(
			TEXT("listen?FlickNetworkMatch?FlickPlayersPerTeam=%d%s%s%s"),
			PendingPlayersPerTeam,
			ActivePurpose == EFlickSessionPurpose::Matchmaking ? TEXT("?FlickMatchmaking") : TEXT(""),
			bPendingRanked ? TEXT("?FlickRanked") : TEXT(""),
			bPendingRanked
				? *FString::Printf(TEXT("?FlickQueueRating=%d"), PendingQueueRating)
				: TEXT(""));
	}
	UGameplayStatics::OpenLevel(
		GetWorld(),
		FName(*CurrentMap),
		true,
		*TravelOptions);
}

void UFlickSessionSubsystem::HandleFindSessionsComplete(const bool bWasSuccessful)
{
	BrowserEntries.Reset();
	if (bIgnoreNextFindCompletion)
	{
		bIgnoreNextFindCompletion = false;
		SetState(
			ActivePurpose == EFlickSessionPurpose::Party ? EFlickSessionState::InSession : EFlickSessionState::Idle,
			ActivePurpose == EFlickSessionPurpose::Party
				? TEXT("MATCHMAKING CANCELLED - PARTY PRESERVED")
				: TEXT("LOBBY SEARCH CANCELLED"));
		return;
	}
	if (!bWasSuccessful || !ActiveSearch.IsValid())
	{
		if (bPartyRestoreSearch)
		{
			bPartyRestoreSearch = false;
			SchedulePartyRestoreRetry();
			return;
		}
		bMatchmakingActive = false;
		bMatchmakingSearch = false;
		bPartyMatchmakingSearch = false;
		bTargetSessionSearch = false;
		SetState(EFlickSessionState::Error, TEXT("STEAM LOBBY SEARCH FAILED"));
		return;
	}

	for (int32 SearchResultIndex = 0; SearchResultIndex < ActiveSearch->SearchResults.Num(); ++SearchResultIndex)
	{
		const FOnlineSessionSearchResult& Result = ActiveSearch->SearchResults[SearchResultIndex];
		FString GameMarker;
		if (!Result.Session.SessionSettings.Get(FlickGameKey, GameMarker) || GameMarker != FlickGameValue)
		{
			continue;
		}
		FString Purpose;
		Result.Session.SessionSettings.Get(FlickPurposeKey, Purpose);
		if (Purpose.Equals(FlickPartyPurpose, ESearchCase::IgnoreCase))
		{
			continue;
		}

		int32 VariantValue = static_cast<int32>(EFlickMatchVariant::Classic);
		Result.Session.SessionSettings.Get(FlickVariantKey, VariantValue);
		VariantValue = FMath::Clamp(
			VariantValue,
			static_cast<int32>(EFlickMatchVariant::Classic),
			static_cast<int32>(EFlickMatchVariant::Blitz));

		FFlickSessionBrowserEntry& Entry = BrowserEntries.AddDefaulted_GetRef();
		Entry.OwnerName = Result.Session.OwningUserName.IsEmpty() ? TEXT("STEAM HOST") : Result.Session.OwningUserName;
		Entry.SessionId = Result.GetSessionIdStr();
		Entry.Variant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(VariantValue));
		Result.Session.SessionSettings.Get(FlickTeamSizeKey, Entry.PlayersPerTeam);
		Entry.PlayersPerTeam = FMath::Clamp(Entry.PlayersPerTeam, 1, 3);
			Entry.bMatchmaking = Purpose.Equals(FlickMatchmakingPurpose, ESearchCase::IgnoreCase);
			int32 RankedValue = 0;
			Result.Session.SessionSettings.Get(FlickRankedKey, RankedValue);
			Result.Session.SessionSettings.Get(FlickMmrKey, Entry.QueueRating);
			Entry.QueueRating = FMath::Clamp(Entry.QueueRating, 0, 3000);
			Entry.bRanked = Entry.bMatchmaking && RankedValue != 0;
		Entry.MaximumPlayers = Result.Session.SessionSettings.NumPublicConnections;
		Entry.CurrentPlayers = FMath::Max(0, Entry.MaximumPlayers - Result.Session.NumOpenPublicConnections);
		Entry.PingMilliseconds = Result.PingInMs;
		Entry.SearchResultIndex = SearchResultIndex;
	}

	if (bTargetSessionSearch)
	{
		bTargetSessionSearch = false;
		for (const FOnlineSessionSearchResult& Result : ActiveSearch->SearchResults)
		{
			if (Result.GetSessionIdStr() == PendingTargetSessionId)
			{
				PendingTargetSessionId.Reset();
				JoinSearchResult(Result);
				return;
			}
		}
		bMatchmakingActive = false;
		SetState(EFlickSessionState::Error, TEXT("THE FOUND MATCH WAS NO LONGER AVAILABLE"));
		if (HasPersistentPartyIdentity())
		{
			RestorePersistentParty(bPersistentPartyLeader);
		}
		return;
	}

	if (bPartyRestoreSearch)
	{
		bPartyRestoreSearch = false;
		for (const FOnlineSessionSearchResult& Result : ActiveSearch->SearchResults)
		{
			FString Purpose;
			FString PartyId;
			Result.Session.SessionSettings.Get(FlickPurposeKey, Purpose);
			Result.Session.SessionSettings.Get(FlickPartyIdKey, PartyId);
			if (Purpose.Equals(FlickPartyPurpose, ESearchCase::IgnoreCase)
				&& PartyId == PersistentPartyId)
			{
				JoinSearchResult(Result);
				return;
			}
		}
		SchedulePartyRestoreRetry();
		return;
	}

	if (bMatchmakingSearch)
	{
		int32 BestSearchResult = INDEX_NONE;
		int32 BestPing = MAX_int32;
		int32 BestRatingGap = MAX_int32;
		for (int32 SearchResultIndex = 0; SearchResultIndex < ActiveSearch->SearchResults.Num(); ++SearchResultIndex)
		{
			const FOnlineSessionSearchResult& Result = ActiveSearch->SearchResults[SearchResultIndex];
			FString Purpose;
			Result.Session.SessionSettings.Get(FlickPurposeKey, Purpose);
			if (!Purpose.Equals(FlickMatchmakingPurpose, ESearchCase::IgnoreCase))
			{
				continue;
			}
			int32 VariantValue = static_cast<int32>(EFlickMatchVariant::Classic);
			int32 LobbyTeamSize = 1;
			int32 bAcceptingPlayers = 1;
			int32 LobbyHostPartySize = 1;
			int32 LobbyRankedValue = 0;
			int32 LobbyQueueRating = FlickRankRules::DefaultRating;
			Result.Session.SessionSettings.Get(FlickVariantKey, VariantValue);
			Result.Session.SessionSettings.Get(FlickTeamSizeKey, LobbyTeamSize);
			Result.Session.SessionSettings.Get(FlickAcceptingPlayersKey, bAcceptingPlayers);
			Result.Session.SessionSettings.Get(FlickPartySizeKey, LobbyHostPartySize);
			Result.Session.SessionSettings.Get(FlickRankedKey, LobbyRankedValue);
			Result.Session.SessionSettings.Get(FlickMmrKey, LobbyQueueRating);
			const EFlickMatchVariant LobbyVariant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(FMath::Clamp(
				VariantValue,
				static_cast<int32>(EFlickMatchVariant::Classic),
				static_cast<int32>(EFlickMatchVariant::Blitz))));
			const int32 RatingGap = FMath::Abs(PendingQueueRating - LobbyQueueRating);
			const bool bRankCompatible = (LobbyRankedValue != 0) == bPendingRanked
				&& (!bPendingRanked || RatingGap <= CurrentRankedSearchRange);
			if (bRankCompatible && FlickMatchmakingRules::IsCompatibleLobby(
				PendingHostVariant,
				PendingPlayersPerTeam,
				PendingPartySize,
				LobbyVariant,
				LobbyTeamSize,
				Result.Session.NumOpenPublicConnections,
				bAcceptingPlayers != 0,
				LobbyHostPartySize)
				&& (RatingGap < BestRatingGap
					|| (RatingGap == BestRatingGap && Result.PingInMs < BestPing)))
			{
				BestSearchResult = SearchResultIndex;
				BestPing = Result.PingInMs;
				BestRatingGap = RatingGap;
			}
		}

		if (ActiveSearch->SearchResults.IsValidIndex(BestSearchResult))
		{
			bMatchmakingSearch = false;
			SetState(
				EFlickSessionState::Joining,
				FString::Printf(
					TEXT("COMPATIBLE %s LOBBY FOUND - %d MS%s"),
					bPendingRanked ? TEXT("RANKED") : TEXT("CASUAL"),
					BestPing,
					bPendingRanked ? *FString::Printf(TEXT(" - %d MMR GAP"), BestRatingGap) : TEXT("")));
			if (bPartyMatchmakingSearch)
			{
				bPartyMatchmakingSearch = false;
				const FString TargetSessionId = ActiveSearch->SearchResults[BestSearchResult].GetSessionIdStr();
				if (AFlickGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFlickGameMode>() : nullptr)
				{
					GameMode->PreparePartyMigrationToMatch(TargetSessionId);
				}
				else
				{
					SetState(EFlickSessionState::Error, TEXT("THE PARTY LEADER COULD NOT START THE MATCH HANDOFF"));
				}
				return;
			}
			JoinSearchResult(ActiveSearch->SearchResults[BestSearchResult]);
			return;
		}
		if (bPendingRanked && MatchmakingSearchAttempt < MaximumRankedSearchAttempts)
		{
			++MatchmakingSearchAttempt;
			ScheduleRankedSearchRetry();
			return;
		}
		bMatchmakingSearch = false;
		if (bPartyMatchmakingSearch)
		{
			bPartyMatchmakingSearch = false;
			OpenCurrentPartyAsMatchmakingQueue();
			return;
		}
		BeginCreateSession(PendingHostVariant, PendingMaximumPlayers, EFlickSessionPurpose::Matchmaking);
		return;
	}

	SetState(
		EFlickSessionState::Idle,
		BrowserEntries.IsEmpty()
			? TEXT("NO FLICK LOBBIES FOUND - HOST ONE OR REFRESH")
			: FString::Printf(TEXT("FOUND %d FLICK %s"), BrowserEntries.Num(), BrowserEntries.Num() == 1 ? TEXT("LOBBY") : TEXT("LOBBIES")));
}

void UFlickSessionSubsystem::HandleJoinSessionComplete(
	const FName SessionName,
	const EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionName == NAME_GameSession && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PartyJoinTimeoutTimer);
	}
	if (bPartyJoinTimedOut)
	{
		return;
	}
	if (SessionName != NAME_GameSession || Result != EOnJoinSessionCompleteResult::Success)
	{
		bMatchmakingActive = false;
		bMatchmakingSearch = false;
		SetState(EFlickSessionState::Error, FString::Printf(TEXT("COULD NOT JOIN LOBBY: %s"), LexToString(Result)));
		if (ActivePurpose == EFlickSessionPurpose::Matchmaking && HasPersistentPartyIdentity())
		{
			RestorePersistentParty(bPersistentPartyLeader);
		}
		return;
	}

	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (ActivePurpose == EFlickSessionPurpose::Party)
	{
		if (!Sessions.IsValid() || !Sessions->GetNamedSession(NAME_GameSession))
		{
			SetState(EFlickSessionState::Error, TEXT("STEAM JOINED THE PARTY WITHOUT LOBBY STATE"));
			return;
		}
		StartPartySynchronization();
		SetState(EFlickSessionState::InSession, TEXT("JOINED STEAM PARTY"));
		return;
	}
	FString ConnectString;
	if (!Sessions.IsValid() || !Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString) || ConnectString.IsEmpty())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT RESOLVE THE HOST ADDRESS"));
		if (ActivePurpose == EFlickSessionPurpose::Matchmaking && HasPersistentPartyIdentity())
		{
			RestorePersistentParty(bPersistentPartyLeader);
		}
		return;
	}

	APlayerController* LocalController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (!LocalController)
	{
		SetState(EFlickSessionState::Error, TEXT("NO LOCAL PLAYER WAS AVAILABLE FOR TRAVEL"));
		return;
	}

	SetState(EFlickSessionState::InSession, TEXT("CONNECTED - ENTERING FLICK LOBBY..."));
	if (UFlickGameInstance* FlickGameInstance = Cast<UFlickGameInstance>(GetGameInstance()))
	{
		FlickGameInstance->PrepareTravelPresentation(TEXT("JOINING MATCH"));
	}
	if (ActivePurpose == EFlickSessionPurpose::Matchmaking
		&& HasPersistentPartyIdentity())
	{
		ConnectString += FString::Printf(
			TEXT("?FlickPartyId=%s?FlickPartySlot=%d?FlickPartySize=%d?FlickPartyLeader=%d"),
			*PersistentPartyId,
			PersistentPartySlot,
			PersistentPartySize,
			bPersistentPartyLeader ? 1 : 0);
	}
	if (ActivePurpose == EFlickSessionPurpose::Matchmaking && bPendingRanked)
	{
		ConnectString += FString::Printf(
			TEXT("?FlickRanked?FlickQueueRating=%d"),
			PendingQueueRating);
	}
	LocalController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}

void UFlickSessionSubsystem::HandleDestroySessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	if (SessionName != NAME_GameSession)
	{
		return;
	}
	if (bPartyJoinTimedOut)
	{
		bPartyJoinTimedOut = false;
		ActivePurpose = EFlickSessionPurpose::Match;
		SetState(EFlickSessionState::Error, TEXT("STEAM PARTY JOIN TIMED OUT - TRY THE INVITE AGAIN"));
		return;
	}
	const bool bShouldHost = bHostAfterDestroy;
	const bool bShouldJoin = bJoinAfterDestroy;
	const bool bShouldReturn = bReturnAfterDestroy;
	const bool bShouldFindTarget = bFindTargetAfterDestroy;
	const bool bShouldRestoreParty = bRestorePartyAfterDestroy;
	bHostAfterDestroy = false;
	bJoinAfterDestroy = false;
	bReturnAfterDestroy = false;
	bFindTargetAfterDestroy = false;
	bRestorePartyAfterDestroy = false;

	if (bShouldFindTarget)
	{
		if (bWasSuccessful)
		{
			ActivePurpose = EFlickSessionPurpose::Match;
			BeginTargetSessionSearch();
		}
		else
		{
			SetState(EFlickSessionState::Error, TEXT("THE PARTY COULD NOT LEAVE ITS OLD TRANSPORT"));
		}
		return;
	}
	if (bShouldRestoreParty)
	{
		if (!bWasSuccessful)
		{
			SetState(EFlickSessionState::Error, TEXT("THE MATCH SESSION COULD NOT BE CLOSED FOR PARTY RESTORE"));
			return;
		}
		ActivePurpose = EFlickSessionPurpose::Match;
		bMatchmakingActive = false;
		if (bRestoreAsPartyLeader)
		{
			PendingMaximumPlayers = FlickMaximumPartyMembers;
			PendingPartySize = PersistentPartySize;
			PendingPurpose = EFlickSessionPurpose::Party;
			BeginCreateSession(EFlickMatchVariant::Classic, PendingMaximumPlayers, PendingPurpose);
		}
		else
		{
			BeginPartyRestoreSearch();
		}
		return;
	}

	if (bShouldHost)
	{
		BeginCreateSession(PendingHostVariant, PendingMaximumPlayers, PendingPurpose);
		return;
	}
	if (bShouldJoin && PendingJoinResult.IsValid())
	{
		const FOnlineSessionSearchResult InviteResult = *PendingJoinResult;
		PendingJoinResult.Reset();
		if (bWasSuccessful)
		{
			JoinSearchResult(InviteResult);
		}
		else
		{
			SetState(EFlickSessionState::Error, TEXT("COULD NOT LEAVE THE CURRENT LOBBY TO ACCEPT THE INVITE"));
		}
		return;
	}

	SetState(
		bWasSuccessful ? EFlickSessionState::Idle : EFlickSessionState::Error,
		bWasSuccessful ? TEXT("STEAM LOBBY CLOSED") : TEXT("STEAM REPORTED A LOBBY CLEANUP ERROR"));
	if (bWasSuccessful)
	{
		bDisbandingParty = false;
		ActivePurpose = EFlickSessionPurpose::Match;
		bMatchmakingActive = false;
		bMatchmakingSearch = false;
	}
	if (bShouldReturn)
	{
		TravelToFrontend();
	}
}

void UFlickSessionSubsystem::NotifySteamPartyInvite(const uint64 InviterId, const uint64 LobbyId)
{
#if WITH_FLICK_STEAMWORKS
	if (!IsSteamAvailable() || InviterId == 0 || LobbyId == 0 || !SteamFriends()
		|| !CSteamID(LobbyId).IsLobby()
		|| SteamFriends()->GetFriendRelationship(CSteamID(InviterId)) != k_EFriendRelationshipFriend)
	{
		return;
	}
	if (PendingIncomingInviteLobbyId == LobbyId || bIncomingPartyInviteSearch)
	{
		return;
	}
	if (HasActiveSession())
	{
		IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
		const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
		const FNamedOnlineSession* Current = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
		if (Current && GetSteamLobbyId(Current->GetSessionIdStr()) == LobbyId) return;
	}
	PendingIncomingInviteLobbyId = LobbyId;
	PendingIncomingInviterId = InviterId;
	PendingIncomingInviteName = UTF8_TO_TCHAR(SteamFriends()->GetFriendPersonaName(CSteamID(InviterId)));
	UE_LOG(LogFlick, Log, TEXT("Steam party invite received from %s for lobby %llu"), *PendingIncomingInviteName, LobbyId);
	OnSessionsChanged.Broadcast();
#endif
}

void UFlickSessionSubsystem::DeclinePendingPartyInvite()
{
	PendingIncomingInviteLobbyId = 0;
	PendingIncomingInviterId = 0;
	PendingIncomingInviteName.Reset();
	OnSessionsChanged.Broadcast();
}

bool UFlickSessionSubsystem::AcceptPendingPartyInvite()
{
	if (PendingIncomingInviteLobbyId == 0 || PendingIncomingInviterId == 0 || bIncomingPartyInviteSearch)
	{
		return false;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	const FUniqueNetIdPtr InviterId = Identity.IsValid()
		? Identity->CreateUniquePlayerId(FString::Printf(TEXT("%llu"), PendingIncomingInviterId)) : nullptr;
	if (!Sessions.IsValid() || !InviterId.IsValid() || !RegisterOnlineDelegates())
	{
		SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT LOOK UP THAT PARTY INVITE"));
		return false;
	}
	bIncomingPartyInviteSearch = true;
	if (!Sessions->FindFriendSession(0, *InviterId))
	{
		bIncomingPartyInviteSearch = false;
		SetState(EFlickSessionState::Error, TEXT("THE INVITING FRIEND'S PARTY IS NO LONGER AVAILABLE"));
		return false;
	}
	SetState(EFlickSessionState::Searching, TEXT("LOOKING UP YOUR FRIEND'S PARTY..."));
	return true;
}

void UFlickSessionSubsystem::HandleFindInvitingFriendSession(
	const int32 LocalUserNum, const bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& Results)
{
	if (!bIncomingPartyInviteSearch || LocalUserNum != 0) return;
	bIncomingPartyInviteSearch = false;
	for (const FOnlineSessionSearchResult& Result : Results)
	{
		FString GameMarker;
		FString Purpose;
		Result.Session.SessionSettings.Get(FlickGameKey, GameMarker);
		Result.Session.SessionSettings.Get(FlickPurposeKey, Purpose);
		if (bWasSuccessful && Result.IsValid()
			&& GameMarker == FlickGameValue
			&& Purpose.Equals(FlickPartyPurpose, ESearchCase::IgnoreCase)
			&& GetSteamLobbyId(Result.GetSessionIdStr()) == PendingIncomingInviteLobbyId)
		{
			DeclinePendingPartyInvite();
			HandleInviteAccepted(true, LocalUserNum, nullptr, Result);
			return;
		}
	}
	SetState(EFlickSessionState::Error, TEXT("THAT PARTY INVITE EXPIRED OR THE PARTY IS FULL"));
}

void UFlickSessionSubsystem::HandleInviteAccepted(
	const bool bWasSuccessful,
	const int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult)
{
	DeclinePendingPartyInvite();
	if (!bWasSuccessful || !InviteResult.IsValid())
	{
		SetState(EFlickSessionState::Error, TEXT("THE STEAM INVITE WAS NO LONGER VALID"));
		return;
	}
	if (State == EFlickSessionState::Joining || bJoinAfterDestroy || PendingJoinResult.IsValid())
	{
		UE_LOG(LogFlick, Verbose, TEXT("Ignored duplicate party invite acceptance while a join is already in progress"));
		return;
	}
	UE_LOG(LogFlick, Log, TEXT("Steam lobby invite accepted by local controller %d"), ControllerId);
	if (HasActiveSession())
	{
		if (ActivePurpose == EFlickSessionPurpose::Party)
		{
			StopPartySynchronization();
			if (UFlickPartySubsystem* Party = GetPartySubsystem())
			{
				Party->Clear();
			}
			ClearPersistentPartyIdentity();
		}
		PendingJoinResult = MakeShared<FOnlineSessionSearchResult>(InviteResult);
		bJoinAfterDestroy = true;
		BeginDestroySession(false);
		return;
	}
	JoinSearchResult(InviteResult);
}

void UFlickSessionSubsystem::HandleSessionParticipantJoined(
	const FName SessionName,
	const FUniqueNetId& UserId)
{
	if (SessionName == NAME_GameSession && ActivePurpose == EFlickSessionPurpose::Party)
	{
		UE_LOG(LogFlick, Log, TEXT("Steam party member joined: %s"), *UserId.ToString());
		RefreshPartyFromSession();
	}
}

void UFlickSessionSubsystem::HandleSessionParticipantLeft(
	const FName SessionName,
	const FUniqueNetId& UserId,
	const EOnSessionParticipantLeftReason Reason)
{
	if (SessionName == NAME_GameSession && ActivePurpose == EFlickSessionPurpose::Party)
	{
		UE_LOG(LogFlick, Log, TEXT("Steam party member left: %s (%s)"), *UserId.ToString(), ToLogString(Reason));
		RefreshPartyFromSession();
	}
}

void UFlickSessionSubsystem::HandleSessionSettingsUpdated(
	const FName SessionName,
	const FOnlineSessionSettings& Settings)
{
	if (SessionName == NAME_GameSession && ActivePurpose == EFlickSessionPurpose::Party)
	{
		RefreshPartyFromSession();
	}
}

void UFlickSessionSubsystem::StartPartySynchronization()
{
	StopPartySynchronization();
	RefreshPartyFromSession();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			PartySynchronizationTimer,
			this,
			&UFlickSessionSubsystem::RefreshPartyFromSession,
			0.5f,
			true);
	}
}

void UFlickSessionSubsystem::StopPartySynchronization()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(PartySynchronizationTimer);
		GetWorld()->GetTimerManager().ClearTimer(PartyCommandTimer);
	}
}

void UFlickSessionSubsystem::HandlePrivateMatchJoinTimeout()
{
	if (ActivePurpose != EFlickSessionPurpose::Party) return;
	bAwaitingPrivateMatchTravel = false;
	UE_LOG(LogFlick, Error, TEXT("PRIVATE_MATCH_JOIN_TIMEOUT: the listen host did not admit this party member"));
	if (UFlickGameInstance* Instance = Cast<UFlickGameInstance>(GetGameInstance()))
	{
		Instance->NotifyFrontendReady();
	}
	SetState(EFlickSessionState::Error, TEXT("PRIVATE MATCH JOIN TIMED OUT - STILL IN PARTY"));
	StartPartySynchronization();
}

void UFlickSessionSubsystem::RefreshPartyFromSession()
{
	if (ActivePurpose != EFlickSessionPurpose::Party)
	{
		return;
	}
	IOnlineSubsystem* OnlineSubsystem = GetFlickOnlineSubsystem(this);
	const IOnlineSessionPtr Sessions = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const IOnlineIdentityPtr Identity = OnlineSubsystem ? OnlineSubsystem->GetIdentityInterface() : nullptr;
	FNamedOnlineSession* NamedSession = Sessions.IsValid() ? Sessions->GetNamedSession(NAME_GameSession) : nullptr;
	UFlickPartySubsystem* Party = GetPartySubsystem();
	if (!NamedSession || !Party)
	{
		return;
	}
	PublishShowcaseArchetypeToParty();

	FString PartyId;
	NamedSession->SessionSettings.Get(FlickPartyIdKey, PartyId);
	if (PartyId.IsEmpty())
	{
		PartyId = NamedSession->GetSessionIdStr();
	}
	FString LeaderId = NamedSession->OwningUserId.IsValid()
		? NamedSession->OwningUserId->ToString() : FString();
	const FUniqueNetIdPtr LocalNetId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
	const FString LocalId = LocalNetId.IsValid() ? LocalNetId->ToString() : FString();

	TArray<TPair<FString, FString>> Members;
	TMap<FString, EFlickPieceArchetype> ShowcaseArchetypes;
	const auto AddMember = [this, &Members, &Identity, &LocalId](const FString& UserId, const FUniqueNetId* ParticipantId)
	{
		if (UserId.IsEmpty() || Members.ContainsByPredicate(
			[&UserId](const TPair<FString, FString>& Entry) { return Entry.Key == UserId; }))
		{
			return;
		}
		FString DisplayName = Identity.IsValid() && ParticipantId
			? Identity->GetPlayerNickname(*ParticipantId) : FString();
		if (DisplayName.IsEmpty())
		{
			if (const FFlickSocialPlayerEntry* Friend = Friends.FindByPredicate(
				[&UserId](const FFlickSocialPlayerEntry& Entry) { return Entry.UserId == UserId; }))
			{
				DisplayName = Friend->DisplayName;
			}
		}
		if (DisplayName.IsEmpty() && UserId == LocalId)
		{
			DisplayName = GetLocalDisplayName();
		}
		Members.Emplace(UserId, DisplayName);
	};

#if WITH_FLICK_STEAMWORKS
	const uint64 SteamLobbyId = GetSteamLobbyId(NamedSession->GetSessionIdStr());
	if (SteamLobbyId != 0 && SteamMatchmaking())
	{
		const CSteamID Lobby(SteamLobbyId);
		const CSteamID Owner = SteamMatchmaking()->GetLobbyOwner(Lobby);
		if (Owner.IsValid())
		{
			LeaderId = LexToString(Owner.ConvertToUint64());
		}
		const int32 MemberCount = SteamMatchmaking()->GetNumLobbyMembers(Lobby);
		for (int32 Index = 0; Index < MemberCount; ++Index)
		{
			const CSteamID Member = SteamMatchmaking()->GetLobbyMemberByIndex(Lobby, Index);
			if (!Member.IsValid())
			{
				continue;
			}
			const FString UserId = LexToString(Member.ConvertToUint64());
			FString DisplayName;
			if (SteamFriends())
			{
				DisplayName = UTF8_TO_TCHAR(SteamFriends()->GetFriendPersonaName(Member));
			}
			Members.Emplace(UserId, DisplayName);
			const char* PublishedPuck = SteamMatchmaking()->GetLobbyMemberData(Lobby, Member, FlickShowcaseMemberDataKey);
			const int32 ArchetypeValue = PublishedPuck ? FCString::Atoi(UTF8_TO_TCHAR(PublishedPuck)) : 0;
			ShowcaseArchetypes.Add(UserId,
				ArchetypeValue >= 0 && ArchetypeValue < FlickPieceArchetypeRules::ArchetypeCount
					? static_cast<EFlickPieceArchetype>(ArchetypeValue)
					: EFlickPieceArchetype::Standard);
		}
	}
#endif
	if (Members.IsEmpty())
	{
		// Non-Steam development fallback. Steam lobbies do not reliably populate
		// RegisteredPlayers, so the live lobby roster above is authoritative.
		for (const FUniqueNetIdRef& ParticipantId : NamedSession->RegisteredPlayers)
		{
			AddMember(ParticipantId->ToString(), &ParticipantId.Get());
		}
		if (NamedSession->OwningUserId.IsValid())
		{
			AddMember(NamedSession->OwningUserId->ToString(), NamedSession->OwningUserId.Get());
		}
		if (LocalNetId.IsValid())
		{
			AddMember(LocalId, LocalNetId.Get());
		}
	}
	if (!LocalId.IsEmpty())
	{
		// The local selection changes immediately, even before Steam echoes the
		// updated member data back to this client.
		ShowcaseArchetypes.Add(LocalId, LocalShowcaseArchetype);
	}
	Party->Synchronize(PartyId, LeaderId, Members, LocalId, ShowcaseArchetypes);
	PersistentPartyId = PartyId;
	PersistentPartySize = Party->GetMemberCount();
	bPersistentPartyLeader = Party->IsLocalLeader();
	if (const FFlickPartyMember* LocalMember = Party->GetMemberByUserId(LocalId))
	{
		PersistentPartySlot = LocalMember->Slot;
	}

	FString PartyCommand;
	NamedSession->SessionSettings.Get(FlickPartyCommandKey, PartyCommand);
	if (!PartyCommand.IsEmpty() && PartyCommand != LastProcessedPartyCommand)
	{
		LastProcessedPartyCommand = PartyCommand;
		if (PartyCommand.StartsWith(TEXT("MATCH:")))
		{
			FFlickCoordinatorAllocation Allocation;
			int32 VariantValue = static_cast<int32>(EFlickMatchVariant::Classic);
			int32 RankedValue = 0;
			FString ExpiryValue;
			FString EncodedReservations;
			NamedSession->SessionSettings.Get(FlickPartyMatchIdKey, Allocation.MatchId);
			NamedSession->SessionSettings.Get(FlickPartyServerIdKey, Allocation.ServerId);
			NamedSession->SessionSettings.Get(FlickPartyAddressKey, Allocation.Address);
			NamedSession->SessionSettings.Get(FlickPartyMatchVariantKey, VariantValue);
			NamedSession->SessionSettings.Get(FlickPartyMatchTeamSizeKey, Allocation.PlayersPerTeam);
			NamedSession->SessionSettings.Get(FlickPartyMatchRankedKey, RankedValue);
			NamedSession->SessionSettings.Get(FlickPartyMatchExpiryKey, ExpiryValue);
			NamedSession->SessionSettings.Get(FlickPartyReservationsKey, EncodedReservations);
			Allocation.Variant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(VariantValue));
			Allocation.PlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(Allocation.PlayersPerTeam);
			Allocation.bRanked = RankedValue != 0;
			Allocation.ExpiresUnixTime = FCString::Atoi64(*ExpiryValue);
			TArray<FString> ReservationRows;
			EncodedReservations.ParseIntoArray(ReservationRows, TEXT(";"), true);
			for (const FString& Row : ReservationRows)
			{
				TArray<FString> Fields;
				Row.ParseIntoArray(Fields, TEXT("|"), false);
				if (Fields.Num() != 4)
				{
					continue;
				}
				FFlickCoordinatorReservation& Reservation = Allocation.Reservations.AddDefaulted_GetRef();
				Reservation.AccountId = FGenericPlatformHttp::UrlDecode(Fields[0]);
				Reservation.Token = FGenericPlatformHttp::UrlDecode(Fields[1]);
				Reservation.Team = static_cast<EFlickTeam>(FMath::Clamp(FCString::Atoi(*Fields[2]), 0, 2));
				Reservation.PlayerSlot = FCString::Atoi(*Fields[3]);
			}
			const FFlickCoordinatorReservation* LocalReservation = Allocation.Reservations.FindByPredicate(
				[&LocalId](const FFlickCoordinatorReservation& Reservation)
				{
					return Reservation.AccountId == LocalId;
				});
			UFlickMatchmakingCoordinatorSubsystem* Coordinator = GetGameInstance()
				? GetGameInstance()->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>() : nullptr;
			APlayerController* LocalController = GetGameInstance()
				? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
			if (!LocalReservation || !Coordinator || !LocalController
				|| Allocation.MatchId.IsEmpty() || Allocation.Address.IsEmpty())
			{
				SetState(EFlickSessionState::Error, TEXT("THE FOUND MATCH DID NOT CONTAIN YOUR RESERVATION"));
				return;
			}
			const FFlickPartyMember* LocalMember = Party->GetMemberByUserId(LocalId);
			Coordinator->AdoptLocalReservation(
				Allocation,
				*LocalReservation,
				Party->GetPartyId(),
				LocalMember ? LocalMember->Slot : 0,
				Party->GetMemberCount(),
				Party->IsLocalLeader());
			if (UFlickGameInstance* FlickGameInstance = Cast<UFlickGameInstance>(GetGameInstance()))
			{
				FlickGameInstance->PrepareTravelPresentation(TEXT("MATCH FOUND"));
			}
			StopPartySynchronization();
			SetState(EFlickSessionState::InSession, TEXT("MATCH FOUND - JOINING DEDICATED SERVER"));
			Coordinator->TravelToAllocatedMatch(LocalController);
			return;
		}
		if (PartyCommand.StartsWith(TEXT("PRIVATE:")) && !Party->IsLocalLeader())
		{
			// A completed private game leaves party members connected to the
			// leader's listen world. The next game starts by replication, not by
			// travelling to the same server a second time.
			if (GetWorld() && GetWorld()->GetNetMode() == NM_Client && GetWorld()->GetNetDriver())
			{
				UE_LOG(LogFlick, Log, TEXT("PRIVATE_MATCH_RESTART: already connected to the party leader"));
				return;
			}
			FString ConnectString;
			if (!Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString) || ConnectString.IsEmpty())
			{
				SetState(EFlickSessionState::Error, TEXT("STEAM COULD NOT RESOLVE THE PRIVATE MATCH HOST"));
				return;
			}
			const FFlickPartyMember* LocalMember = Party->GetMemberByUserId(LocalId);
			ConnectString += FString::Printf(
				TEXT("?FlickParty?FlickPartyId=%s?FlickPartySlot=%d?FlickPartySize=%d?FlickPartyLeader=0"),
				*Party->GetPartyId(),
				LocalMember ? LocalMember->Slot : 1,
				Party->GetMemberCount());
			// Keep the guest's frontend visible until travel swaps in the arena.
			// Private matches do not need a persistent movie-player loading screen.
			StopPartySynchronization();
			if (APlayerController* LocalController = GetGameInstance()
				? GetGameInstance()->GetFirstLocalPlayerController() : nullptr)
			{
				bAwaitingPrivateMatchTravel = true;
				if (GetWorld()) GetWorld()->GetTimerManager().SetTimer(
					PrivateMatchJoinTimeoutTimer, this, &UFlickSessionSubsystem::HandlePrivateMatchJoinTimeout, 25.0f, false);
				LocalController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
			return;
		}
		const bool bDisbandCommand = PartyCommand.StartsWith(TEXT("DISBAND:"));
		const bool bKickCommand = PartyCommand.StartsWith(FString::Printf(TEXT("KICK:%s:"), *LocalId));
		if ((bDisbandCommand && !Party->IsLocalLeader()) || bKickCommand)
		{
			SetState(EFlickSessionState::InSession,
				bDisbandCommand ? TEXT("PARTY DISBANDED") : TEXT("REMOVED FROM PARTY"));
			LeaveParty();
			return;
		}
	}
	OnSessionsChanged.Broadcast();
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().InvalidateAllWidgets(false);
	}
}

void UFlickSessionSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	const ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickMatchmakingCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>())
		{
			APlayerController* LocalController = GameInstance->GetFirstLocalPlayerController();
			if (Coordinator->TryReconnect(LocalController))
			{
				UE_LOG(LogFlick, Warning, TEXT("Dedicated match connection lost; using coordinator reconnect reservation"));
				return;
			}
		}
	}
	if (!HasActiveSession() && State != EFlickSessionState::Joining && State != EFlickSessionState::InSession)
	{
		return;
	}
	UE_LOG(LogFlick, Warning, TEXT("Online network failure (%d): %s"), static_cast<int32>(FailureType), *ErrorString);
	SetState(EFlickSessionState::Error, FString::Printf(TEXT("CONNECTION LOST: %s"), *ErrorString.ToUpper()));
	if (ActivePurpose == EFlickSessionPurpose::Party)
	{
		bAwaitingPrivateMatchTravel = false;
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PrivateMatchJoinTimeoutTimer);
		if (UFlickGameInstance* Instance = Cast<UFlickGameInstance>(GetGameInstance())) Instance->NotifyFrontendReady();
		StartPartySynchronization();
		return;
	}
	if (ActivePurpose == EFlickSessionPurpose::Matchmaking
		&& HasPersistentPartyIdentity())
	{
		RestorePersistentParty(ShouldLeadPartyRestorationAfterFailure());
		return;
	}
	LeaveSession(true);
}

void UFlickSessionSubsystem::HandleTravelFailure(
	UWorld* World,
	const ETravelFailure::Type FailureType,
	const FString& ErrorString)
{
	UE_LOG(LogFlick, Warning, TEXT("Online travel failure (%d): %s"), static_cast<int32>(FailureType), *ErrorString);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UFlickMatchmakingCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>())
		{
			if (Coordinator->TryReconnect(GameInstance->GetFirstLocalPlayerController()))
			{
				return;
			}
		}
	}
	SetState(EFlickSessionState::Error, FString::Printf(TEXT("COULD NOT ENTER LOBBY: %s"), *ErrorString.ToUpper()));
	if (ActivePurpose == EFlickSessionPurpose::Party)
	{
		bAwaitingPrivateMatchTravel = false;
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(PrivateMatchJoinTimeoutTimer);
		if (UFlickGameInstance* Instance = Cast<UFlickGameInstance>(GetGameInstance())) Instance->NotifyFrontendReady();
		StartPartySynchronization();
		return;
	}
	if (ActivePurpose == EFlickSessionPurpose::Matchmaking
		&& HasPersistentPartyIdentity())
	{
		RestorePersistentParty(ShouldLeadPartyRestorationAfterFailure());
		return;
	}
	LeaveSession(true);
}

bool UFlickSessionSubsystem::ShouldLeadPartyRestorationAfterFailure() const
{
	if (bPersistentPartyLeader)
	{
		return true;
	}

	// If the original listen host disappears, the next stable party slot is the
	// deterministic replacement. Everyone else searches for that recreated lobby.
	return PersistentPartySlot == 1;
}

void UFlickSessionSubsystem::TravelToFrontend()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UGameplayStatics::OpenLevel(World, FName(*World->GetOutermost()->GetName()), true);
}

void UFlickSessionSubsystem::SetState(const EFlickSessionState NewState, const FString& Message)
{
	State = NewState;
	StatusMessage = Message;
	UE_LOG(LogFlick, Log, TEXT("Online session: %s"), *StatusMessage);
	OnSessionsChanged.Broadcast();
}

bool UFlickSessionSubsystem::IsMatchmakingActive() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UFlickMatchmakingCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>())
		{
			if (Coordinator->IsQueueActive())
			{
				return true;
			}
		}
	}
	return bMatchmakingActive || ActivePurpose == EFlickSessionPurpose::Matchmaking;
}

EFlickSessionState UFlickSessionSubsystem::GetState() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UFlickMatchmakingCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>())
		{
			if (Coordinator->GetQueueState() == EFlickCoordinatorQueueState::Error)
			{
				return EFlickSessionState::Error;
			}
		}
	}
	return State;
}

const FString& UFlickSessionSubsystem::GetStatusMessage() const
{
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UFlickMatchmakingCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UFlickMatchmakingCoordinatorSubsystem>())
		{
			if (Coordinator->ShouldUseCoordinator()
				&& (Coordinator->IsQueueActive()
					|| Coordinator->GetQueueState() == EFlickCoordinatorQueueState::Allocated
					|| Coordinator->GetQueueState() == EFlickCoordinatorQueueState::Error))
			{
				return Coordinator->GetStatusMessage();
			}
		}
	}
	return StatusMessage;
}
