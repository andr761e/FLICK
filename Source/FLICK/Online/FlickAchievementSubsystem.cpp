#include "Online/FlickAchievementSubsystem.h"
#include "Core/FlickLog.h"
#include "Core/FlickRankRules.h"
#include "HAL/FileManager.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "OnlineSubsystem.h"
#if WITH_FLICK_STEAMWORKS
#include "steam/steam_api.h"
#endif

namespace
{
    bool IsPreviewRun() { return IsRunningCommandlet() || FApp::IsUnattended() || IsRunningDedicatedServer(); }
    int32 PlaylistIndex(EFlickMatchVariant V, int32 Size) { return V == EFlickMatchVariant::Bob ? 3 : FMath::Clamp(Size, 1, 3) - 1; }
    const TCHAR* MaskNames[] = {TEXT("STAT_INTERNAL_ARCHETYPE_MASK"), TEXT("STAT_INTERNAL_PRACTICE_MASK"), TEXT("STAT_INTERNAL_RANKED_WIN_MASK"), TEXT("STAT_INTERNAL_PLACED_MASK")};
}

// Steam v1.64 synchronizes the current user's stats before game startup.
// Do not call the removed RequestCurrentStats or own the global Steam lifecycle.
class FFlickSteamAchievementBridge
{
public:
    explicit FFlickSteamAchievementBridge(UFlickAchievementSubsystem* InOwner) : Owner(InOwner)
    {
#if WITH_FLICK_STEAMWORKS
        Stored.Register(this, &FFlickSteamAchievementBridge::OnStored);
#endif
    }
    ~FFlickSteamAchievementBridge()
    {
#if WITH_FLICK_STEAMWORKS
        Stored.Unregister();
#endif
    }
    void Sync()
    {
#if WITH_FLICK_STEAMWORKS
        auto* Stats = SteamUserStats();
        if (!Stats || !SteamUtils() || SteamUtils()->GetAppID() != FlickAchievements::AppId) return;
        if (bInFlight && FPlatformTime::Seconds() - SubmittedAt < 60) return;
        if (bInFlight) { bInFlight = false; Owner->bDirty = true; }
        const auto BeforeValues = Owner->Progress.Values;
        bool bRemoteLoaded = false;
        uint32* Masks[] = {&Owner->Progress.Archetypes, &Owner->Progress.Practice, &Owner->Progress.RankedWins, &Owner->Progress.Placed};
        const uint32 Limits[] = {511, 4095, 15, 15};
        for (int32 I = 0; I < 4; ++I)
        {
            int32 Remote = 0;
            if (!Loaded.Contains(MaskNames[I]) && Stats->GetStat(TCHAR_TO_UTF8(MaskNames[I]), &Remote))
            {
                *Masks[I] |= static_cast<uint32>(Remote) & Limits[I];
                Loaded.Add(MaskNames[I]); Owner->bDirty = bRemoteLoaded = true;
            }
        }
        for (int32 I = 0; I < FlickAchievements::Count; ++I)
        {
            const auto& Def = FlickAchievements::Definitions[I];
            if (*Def.Stat && !Loaded.Contains(Def.Stat))
            {
                int32 Remote = 0;
                if (Stats->GetStat(TCHAR_TO_UTF8(Def.Stat), &Remote))
                {
                    for (int32 J = 0; J < FlickAchievements::Count; ++J)
                        if (FCString::Strcmp(Def.Stat, FlickAchievements::Definitions[J].Stat) == 0)
                        {
                            Owner->Progress.Best(static_cast<FlickAchievements::EId>(J), FlickAchievements::MergeRemoteCounter(Owner->Progress.Values[J], Owner->Pending[J], Remote, FlickAchievements::Definitions[J].Goal));
                        }
                    Loaded.Add(Def.Stat); Owner->bDirty = bRemoteLoaded = true;
                }
            }
            bool bUnlocked = false;
            if (Stats->GetAchievement(TCHAR_TO_UTF8(Def.Api), &bUnlocked) && bUnlocked)
                Owner->Progress.Best(static_cast<FlickAchievements::EId>(I), Def.Goal);
        }
        Owner->Progress.RefreshUniqueProgress();
        if (bRemoteLoaded || BeforeValues != Owner->Progress.Values) Owner->Save();
        if (!Owner->bDirty) return;
        TMap<FString, int32> Totals;
        for (int32 I = 0; I < FlickAchievements::Count; ++I)
            if (*FlickAchievements::Definitions[I].Stat) Totals.FindOrAdd(FlickAchievements::Definitions[I].Stat) = FMath::Max(Totals.FindRef(FlickAchievements::Definitions[I].Stat), Owner->Progress.Values[I]);
        Submitted.Init(0, FlickAchievements::Count);
        bool bMissingSchema = false, bAny = false;
        for (const auto& Pair : Totals)
        {
            if (Loaded.Contains(Pair.Key) && Stats->SetStat(TCHAR_TO_UTF8(*Pair.Key), Pair.Value))
            {
                bAny = true;
                for (int32 I = 0; I < FlickAchievements::Count; ++I)
                    if (Pair.Key == FlickAchievements::Definitions[I].Stat) Submitted[I] = Owner->Pending[I];
            }
            else if (Pair.Value > 0) bMissingSchema = true;
        }
        for (int32 I = 0; I < 4; ++I)
            if (Loaded.Contains(MaskNames[I])) { bAny |= Stats->SetStat(TCHAR_TO_UTF8(MaskNames[I]), static_cast<int32>(*Masks[I])); }
            else if (*Masks[I]) bMissingSchema = true;
        for (int32 I = 0; I < FlickAchievements::Count; ++I)
            if (Owner->Progress.Values[I] >= FlickAchievements::Definitions[I].Goal)
            {
                if (Stats->SetAchievement(TCHAR_TO_UTF8(FlickAchievements::Definitions[I].Api))) bAny = true;
                else bMissingSchema = true;
            }
        if (bMissingSchema && !bLoggedSchema)
        {
            UE_LOG(LogFlick, Warning, TEXT("Steam achievement/stat schema is incomplete for App 5548920; progress is retained locally. See Deliverables/Steamworks-Setup-5548920."));
            bLoggedSchema = true;
        }
        if (bAny)
        {
            Owner->bDirty = bMissingSchema;
            SubmittedAt = FPlatformTime::Seconds(); bInFlight = true;
            if (!Stats->StoreStats()) { bInFlight = false; Owner->bDirty = true; }
        }
#endif
    }
private:
    UFlickAchievementSubsystem* Owner;
    TSet<FString> Loaded;
    TArray<int32> Submitted;
    bool bInFlight = false, bLoggedSchema = false;
    double SubmittedAt = 0;
#if WITH_FLICK_STEAMWORKS
    CCallbackManual<FFlickSteamAchievementBridge, UserStatsStored_t> Stored;
    void OnStored(UserStatsStored_t* Result)
    {
        if (!Result || Result->m_nGameID != FlickAchievements::AppId || !bInFlight) return;
        bInFlight = false;
        if (Result->m_eResult == k_EResultOK)
            for (int32 I = 0; I < FlickAchievements::Count; ++I) Owner->Pending[I] = FlickAchievements::AcknowledgeCounter(Owner->Pending[I], Submitted[I]);
        else { Owner->bDirty = true; Loaded.Reset(); }
        Owner->Save();
    }
#endif
};

void UFlickAchievementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Pending.Init(0, FlickAchievements::Count);
    if (IsPreviewRun()) return;
    SelectAccount();
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UFlickAchievementSubsystem::TickSteam), 5.f);
}

void UFlickAchievementSubsystem::Deinitialize()
{
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    delete Bridge; Bridge = nullptr;
    Super::Deinitialize();
}

void UFlickAchievementSubsystem::SelectAccount()
{
    FString NewAccount;
    if (!FParse::Param(FCommandLine::Get(), TEXT("nosteam")))
        if (auto* OSS = IOnlineSubsystem::Get(FName(TEXT("STEAM"))))
            if (auto Identity = OSS->GetIdentityInterface(); Identity.IsValid())
                if (auto Id = Identity->GetUniquePlayerId(0); Id.IsValid()) NewAccount = Id->ToString();
    if (NewAccount.IsEmpty()) NewAccount = Account.IsEmpty() ? TEXT("Local") : Account;
    if (NewAccount == Account) return;
    delete Bridge; Bridge = nullptr;
    Account = NewAccount; Progress = FFlickAchievementProgress(); Pending.Init(0, FlickAchievements::Count); SeenReports.Reset();
    Section = FPaths::ProjectSavedDir() / TEXT("SaveGames") / (TEXT("SteamAchievements-") + Account + TEXT(".ini"));
    FConfigFile File; File.Read(Section);
    for (int32 I = 0; I < FlickAchievements::Count; ++I)
    {
        int32 Value = 0; File.GetInt(TEXT("Progress"), FlickAchievements::Definitions[I].Api, Value); Progress.Best(static_cast<FlickAchievements::EId>(I), Value);
        Value = 0; File.GetInt(TEXT("Pending"), FlickAchievements::Definitions[I].Api, Value); Pending[I] = FMath::Clamp(Value, 0, FlickAchievements::Definitions[I].Goal);
    }
    uint32* Masks[] = {&Progress.Archetypes, &Progress.Practice, &Progress.RankedWins, &Progress.Placed};
    const uint32 Limits[] = {511,4095,15,15};
    for (int32 I = 0; I < 4; ++I) { int32 M = 0; File.GetInt(TEXT("Masks"), MaskNames[I], M); *Masks[I] = static_cast<uint32>(M) & Limits[I]; }
    File.GetArray(TEXT("Reports"), TEXT("Ids"), SeenReports);
    Progress.RefreshUniqueProgress();
    bDirty = Progress.Values.ContainsByPredicate([](int32 V) { return V > 0; });
}

void UFlickAchievementSubsystem::Save() const
{
    if (Section.IsEmpty() || IsPreviewRun()) return;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Section), true);
    FConfigFile File;
    for (int32 I = 0; I < FlickAchievements::Count; ++I)
    {
        File.SetString(TEXT("Progress"), FlickAchievements::Definitions[I].Api, *FString::FromInt(Progress.Values[I]));
        File.SetString(TEXT("Pending"), FlickAchievements::Definitions[I].Api, *FString::FromInt(Pending[I]));
    }
    const uint32 Masks[] = {Progress.Archetypes, Progress.Practice, Progress.RankedWins, Progress.Placed};
    for (int32 I = 0; I < 4; ++I) File.SetString(TEXT("Masks"), MaskNames[I], *FString::FromInt(static_cast<int32>(Masks[I])));
    File.SetArray(TEXT("Reports"), TEXT("Ids"), SeenReports);
    File.Write(Section);
}

bool UFlickAchievementSubsystem::TickSteam(float)
{
    if (IsPreviewRun() || FParse::Param(FCommandLine::Get(), TEXT("nosteam"))) return true;
    SelectAccount();
#if WITH_FLICK_STEAMWORKS
    if (Account != TEXT("Local") && SteamUtils() && SteamUtils()->GetAppID() == FlickAchievements::AppId && SteamUserStats())
    {
        if (!Bridge) Bridge = new FFlickSteamAchievementBridge(this);
        Bridge->Sync();
    }
#endif
    return true;
}

void UFlickAchievementSubsystem::Changed(const TArray<int32>& Before)
{
    for (int32 I = 0; I < FlickAchievements::Count; ++I)
        if (*FlickAchievements::Definitions[I].Stat && !FlickAchievements::Definitions[I].bBest)
            Pending[I] = FMath::Min(FlickAchievements::Definitions[I].Goal, Pending[I] + FMath::Max(0, Progress.Values[I] - Before[I]));
    bDirty = true; Save();
}

void UFlickAchievementSubsystem::AcceptMatch(const FFlickAchievementReport& Report)
{
    if (IsPreviewRun() || !Report.EventId.IsValid() || Report.Values.Num() != FlickAchievements::Count) return;
    SelectAccount();
    const FString Id = Report.EventId.ToString();
    if (SeenReports.Contains(Id)) return;
    const auto Before = Progress.Values;
    Progress.Apply(Report); SeenReports.Add(Id);
    if (SeenReports.Num() > 512) SeenReports.RemoveAt(0, SeenReports.Num() - 512);
    Changed(Before);
}

void UFlickAchievementSubsystem::RecordPractice(int32 Category, int32 Difficulty, int32 Score)
{
    if (IsPreviewRun()) return;
    SelectAccount(); const auto Before = Progress.Values;
    Progress.RecordPractice(Category, Difficulty, Score); Changed(Before);
}

void UFlickAchievementSubsystem::RecordPracticeShot()
{
    if (IsPreviewRun()) return;
    SelectAccount(); const auto Before = Progress.Values;
    Progress.Best(FlickAchievements::EId::FirstFlick, 1); Progress.Add(FlickAchievements::EId::FindYourAngle); Changed(Before);
}

void UFlickAchievementSubsystem::RecordCustomLineup()
{
    if (IsPreviewRun()) return;
    SelectAccount(); const auto Before = Progress.Values; Progress.Best(FlickAchievements::EId::TailorMade, 1); Changed(Before);
}

void UFlickAchievementSubsystem::RecordPlacements(EFlickMatchVariant Variant, int32 TeamSize, int32 Matches)
{
    if (IsPreviewRun()) return;
    SelectAccount(); const auto Before = Progress.Values;
    Progress.RecordPlacements(PlaylistIndex(Variant, TeamSize), Matches); Changed(Before);
}

void UFlickAchievementSubsystem::RecordRankedWin(EFlickMatchVariant Variant, int32 TeamSize)
{
    if (IsPreviewRun()) return;
    SelectAccount(); const auto Before = Progress.Values;
    Progress.RecordRankedWin(PlaylistIndex(Variant, TeamSize)); Changed(Before);
}
