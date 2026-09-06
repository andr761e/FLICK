#include "Game/FlickGameInstance.h"

#include "Core/FlickPieceArchetypeRules.h"
#include "MoviePlayer.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const TCHAR* FlickSettingsSection = TEXT("FLICK.GameplaySettings");
	const TCHAR* FlickProfileSection = TEXT("FLICK.ProfileStats");
	constexpr int32 MaxLoadoutSlots = 4;
	constexpr int32 MaxRecentProfileMatches = 8;
	const TCHAR* AccoladeConfigKeys[FlickAccoladeCount] =
	{
		TEXT("Accolade.SwitchKnockout"), TEXT("Accolade.DividerBank"), TEXT("Accolade.TrapShot"),
		TEXT("Accolade.DominoKnockout"), TEXT("Accolade.TeamWipeout"), TEXT("Accolade.PerfectTrade"),
		TEXT("Accolade.LongRangeKnockout"), TEXT("Accolade.Pinball"), TEXT("Accolade.PrecisionKnockout"),
		TEXT("Accolade.BuzzerBeater"), TEXT("Accolade.FlawlessRound")
	};

	int32 AddProfileCounter(const int32 CurrentValue, const int32 AddedValue)
	{
		return static_cast<int32>(FMath::Min<int64>(
			static_cast<int64>(MAX_int32),
			static_cast<int64>(FMath::Max(0, CurrentValue)) + FMath::Max(0, AddedValue)));
	}

	TArray<EFlickPieceArchetype> MakeDefaultLoadout()
	{
		return FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Balanced);
	}

	void LoadLoadout(
		const TCHAR* Key,
		const TArray<EFlickPieceArchetype>& DefaultLoadout,
		TArray<EFlickPieceArchetype>& OutLoadout)
	{
		OutLoadout = DefaultLoadout;
		TArray<FString> SavedValues;
		GConfig->GetArray(FlickSettingsSection, Key, SavedValues, GGameUserSettingsIni);
		for (int32 Index = 0; Index < FMath::Min(SavedValues.Num(), MaxLoadoutSlots); ++Index)
		{
			const int32 Value = FMath::Clamp(
				FCString::Atoi(*SavedValues[Index]),
				static_cast<int32>(EFlickPieceArchetype::Standard),
				static_cast<int32>(EFlickPieceArchetype::Toppler));
			OutLoadout[Index] = static_cast<EFlickPieceArchetype>(Value);
		}
	}

	TArray<FString> SaveLoadout(const TArray<EFlickPieceArchetype>& Loadout)
	{
		TArray<FString> SavedValues;
		SavedValues.Reserve(MaxLoadoutSlots);
		for (int32 Index = 0; Index < MaxLoadoutSlots; ++Index)
		{
			const EFlickPieceArchetype Archetype = Loadout.IsValidIndex(Index)
				? Loadout[Index]
				: EFlickPieceArchetype::Standard;
			SavedValues.Add(FString::FromInt(static_cast<int32>(Archetype)));
		}
		return SavedValues;
	}

	bool ParseProfileMatchRecord(const FString& Value, FFlickProfileMatchRecord& OutRecord)
	{
		TArray<FString> Fields;
		Value.ParseIntoArray(Fields, TEXT("|"), false);
		if (Fields.Num() != 10)
		{
			return false;
		}
		OutRecord.CompletedUnixTime = FMath::Max<int64>(0, FCString::Atoi64(*Fields[0]));
		OutRecord.Variant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(FMath::Clamp(FCString::Atoi(*Fields[1]), 0, 2)));
		OutRecord.PlayersPerTeam = FMath::Clamp(FCString::Atoi(*Fields[2]), 1, 3);
		OutRecord.bRanked = FCString::Atoi(*Fields[3]) != 0;
		OutRecord.bWon = FCString::Atoi(*Fields[4]) != 0;
		OutRecord.bDraw = FCString::Atoi(*Fields[5]) != 0;
		OutRecord.Points = FMath::Max(0, FCString::Atoi(*Fields[6]));
		OutRecord.Knockouts = FMath::Max(0, FCString::Atoi(*Fields[7]));
		OutRecord.DoubleKnockouts = FMath::Max(0, FCString::Atoi(*Fields[8]));
		OutRecord.Shots = FMath::Max(0, FCString::Atoi(*Fields[9]));
		return true;
	}

	FString SaveProfileMatchRecord(const FFlickProfileMatchRecord& Record)
	{
		return FString::Printf(
			TEXT("%lld|%d|%d|%d|%d|%d|%d|%d|%d|%d"),
			static_cast<long long>(Record.CompletedUnixTime),
			static_cast<int32>(Record.Variant),
			Record.PlayersPerTeam,
			Record.bRanked ? 1 : 0,
			Record.bWon ? 1 : 0,
			Record.bDraw ? 1 : 0,
			Record.Points,
			Record.Knockouts,
			Record.DoubleKnockouts,
			Record.Shots);
	}
}

void FFlickProfileStats::RecordMatch(
	const int32 MatchPoints,
	const int32 MatchKnockouts,
	const int32 MatchDoubleKnockouts,
	const int32 MatchShots,
	const bool bWon)
{
	MatchesPlayed = AddProfileCounter(MatchesPlayed, 1);
	Wins = AddProfileCounter(Wins, bWon ? 1 : 0);
	Points = AddProfileCounter(Points, MatchPoints);
	Knockouts = AddProfileCounter(Knockouts, MatchKnockouts);
	DoubleKnockouts = AddProfileCounter(DoubleKnockouts, MatchDoubleKnockouts);
	Shots = AddProfileCounter(Shots, MatchShots);
}

void FFlickProfileStats::RecordAccolades(const TArray<int32>& MatchAccoladeCounts)
{
	if (AccoladeCounts.Num() != FlickAccoladeCount)
	{
		AccoladeCounts.SetNumZeroed(FlickAccoladeCount);
	}
	for (int32 Index = 0; Index < FlickAccoladeCount; ++Index)
	{
		AccoladeCounts[Index] = AddProfileCounter(
			AccoladeCounts[Index],
			MatchAccoladeCounts.IsValidIndex(Index) ? MatchAccoladeCounts[Index] : 0);
	}
}

int32 FFlickProfileStats::GetTotalAccolades() const
{
	int32 Total = 0;
	for (const int32 Count : AccoladeCounts)
	{
		Total = AddProfileCounter(Total, Count);
	}
	return Total;
}

int32 FFlickProfileStats::GetAccoladeCount(const EFlickAccolade Accolade) const
{
	const int32 Index = static_cast<int32>(Accolade);
	return AccoladeCounts.IsValidIndex(Index) ? FMath::Max(0, AccoladeCounts[Index]) : 0;
}

void UFlickGameInstance::Init()
{
	Super::Init();

	bStartupPresentationComplete = FParse::Param(FCommandLine::Get(), TEXT("FlickSkipIntro"));
	if (!bStartupPresentationComplete && !IsRunningCommandlet() && !IsRunningDedicatedServer())
	{
		FSlateFontInfo LoadingStatusFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
		LoadingStatusFont.LetterSpacing = 180;
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.MinimumLoadingScreenDisplayTime = 1.35f;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
		LoadingScreen.bWaitForManualStop = true;
		LoadingScreen.bMoviesAreSkippable = false;
		LoadingScreen.WidgetLoadingScreen =
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.0f, 0.002f, 0.006f, 1.0f))
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SThrobber)
					.NumPieces(3)
					.Animate(SThrobber::All)
					.RenderTransform(FSlateRenderTransform(FScale2D(1.5f)))
					.RenderTransformPivot(FVector2D(0.5f, 0.5f))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("INITIALIZING ARENA")))
					.Font(LoadingStatusFont)
					.ColorAndOpacity(FLinearColor(0.0f, 0.82f, 1.0f, 1.0f))
				]
			];
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
		bStartupLoadingScreenConfigured = GetMoviePlayer()->PlayMovie();
	}

	int32 VariantValue = static_cast<int32>(SelectedMatchVariant);
	GConfig->GetInt(FlickSettingsSection, TEXT("SelectedMatchVariant"), VariantValue, GGameUserSettingsIni);
	VariantValue = FMath::Clamp(VariantValue, 0, 2);
	SelectedMatchVariant = NormalizeMatchVariant(static_cast<EFlickMatchVariant>(VariantValue));
	LoadLoadout(TEXT("Player1Loadout"), MakeDefaultLoadout(), Player1Loadout);
	LoadLoadout(TEXT("Player2Loadout"), MakeDefaultLoadout(), Player2Loadout);
	// Class 1 inherits the old Player 1 lineup on first launch so existing edits
	// are preserved when migrating from the previous single-loadout editor.
	LoadLoadout(TEXT("Class1Loadout"), Player1Loadout, Class1Loadout);
	LoadLoadout(TEXT("Class2Loadout"), FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Power), Class2Loadout);
	LoadLoadout(TEXT("Class3Loadout"), FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Speed), Class3Loadout);
	LoadLoadout(TEXT("Class4Loadout"), FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Control), Class4Loadout);

	GConfig->GetBool(FlickSettingsSection, TEXT("AimGuideEnabled"), bAimGuideEnabled, GGameUserSettingsIni);
	GConfig->GetBool(FlickSettingsSection, TEXT("ImpactEffectsEnabled"), bImpactEffectsEnabled, GGameUserSettingsIni);
	GConfig->GetBool(FlickSettingsSection, TEXT("ControlOverviewEnabled"), bControlOverviewEnabled, GGameUserSettingsIni);
	int32 BotDifficultyValue = static_cast<int32>(BotDifficulty);
	GConfig->GetInt(FlickSettingsSection, TEXT("BotDifficulty"), BotDifficultyValue, GGameUserSettingsIni);
	BotDifficulty = static_cast<EFlickBotDifficulty>(FMath::Clamp(
		BotDifficultyValue,
		static_cast<int32>(EFlickBotDifficulty::Easy),
		static_cast<int32>(EFlickBotDifficulty::Expert)));
	GConfig->GetFloat(FlickSettingsSection, TEXT("CameraShakeIntensity"), CameraShakeIntensity, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("EffectsVolume"), EffectsVolume, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("InterfaceVolume"), InterfaceVolume, GGameUserSettingsIni);
	CameraShakeIntensity = FMath::Clamp(CameraShakeIntensity, 0.0f, 1.0f);
	MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
	EffectsVolume = FMath::Clamp(EffectsVolume, 0.0f, 1.0f);
	InterfaceVolume = FMath::Clamp(InterfaceVolume, 0.0f, 1.0f);

	GConfig->GetInt(FlickProfileSection, TEXT("MatchesPlayed"), ProfileStats.MatchesPlayed, GGameUserSettingsIni);
	GConfig->GetInt(FlickProfileSection, TEXT("Wins"), ProfileStats.Wins, GGameUserSettingsIni);
	GConfig->GetInt(FlickProfileSection, TEXT("Points"), ProfileStats.Points, GGameUserSettingsIni);
	GConfig->GetInt(FlickProfileSection, TEXT("Knockouts"), ProfileStats.Knockouts, GGameUserSettingsIni);
	GConfig->GetInt(FlickProfileSection, TEXT("DoubleKnockouts"), ProfileStats.DoubleKnockouts, GGameUserSettingsIni);
	GConfig->GetInt(FlickProfileSection, TEXT("Shots"), ProfileStats.Shots, GGameUserSettingsIni);
	ProfileStats.AccoladeCounts.Init(0, FlickAccoladeCount);
	for (int32 Index = 0; Index < FlickAccoladeCount; ++Index)
	{
		GConfig->GetInt(FlickProfileSection, AccoladeConfigKeys[Index], ProfileStats.AccoladeCounts[Index], GGameUserSettingsIni);
		ProfileStats.AccoladeCounts[Index] = FMath::Max(0, ProfileStats.AccoladeCounts[Index]);
	}
	ProfileStats.MatchesPlayed = FMath::Max(0, ProfileStats.MatchesPlayed);
	ProfileStats.Wins = FMath::Clamp(ProfileStats.Wins, 0, ProfileStats.MatchesPlayed);
	ProfileStats.Points = FMath::Max(0, ProfileStats.Points);
	ProfileStats.Knockouts = FMath::Max(0, ProfileStats.Knockouts);
	ProfileStats.DoubleKnockouts = FMath::Max(0, ProfileStats.DoubleKnockouts);
	ProfileStats.Shots = FMath::Max(0, ProfileStats.Shots);
	TArray<FString> SavedRecentMatches;
	GConfig->GetArray(FlickProfileSection, TEXT("RecentMatches"), SavedRecentMatches, GGameUserSettingsIni);
	for (const FString& SavedMatch : SavedRecentMatches)
	{
		FFlickProfileMatchRecord Record;
		if (ParseProfileMatchRecord(SavedMatch, Record))
		{
			RecentMatches.Add(Record);
			if (RecentMatches.Num() >= MaxRecentProfileMatches)
			{
				break;
			}
		}
	}
}

void UFlickGameInstance::NotifyFrontendReady()
{
	if (bStartupLoadingScreenConfigured && GetMoviePlayer()->IsMovieCurrentlyPlaying())
	{
		GetMoviePlayer()->StopMovie();
	}
	bStartupLoadingScreenConfigured = false;
}

EFlickPieceArchetype UFlickGameInstance::GetLoadoutPiece(const EFlickTeam Team, const int32 SlotIndex) const
{
	const TArray<EFlickPieceArchetype>* Loadout = Team == EFlickTeam::Player2
		? &Player2Loadout
		: &Player1Loadout;
	return Loadout->IsValidIndex(SlotIndex)
		? (*Loadout)[SlotIndex]
		: EFlickPieceArchetype::Standard;
}

EFlickLineupPreset UFlickGameInstance::GetLoadoutPreset(const EFlickTeam Team) const
{
	const TArray<EFlickPieceArchetype>& Loadout = Team == EFlickTeam::Player2
		? Player2Loadout
		: Player1Loadout;
	for (const EFlickLineupPreset Preset : {
		EFlickLineupPreset::Balanced,
		EFlickLineupPreset::Power,
		EFlickLineupPreset::Speed,
		EFlickLineupPreset::Control})
	{
		if (Loadout == FlickPieceArchetypeRules::GetPreset(Preset))
		{
			return Preset;
		}
	}
	return EFlickLineupPreset::Custom;
}

EFlickPieceArchetype UFlickGameInstance::GetClassLoadoutPiece(
	const EFlickLineupPreset Preset,
	const int32 SlotIndex) const
{
	const TArray<EFlickPieceArchetype>* Loadout = nullptr;
	switch (Preset)
	{
	case EFlickLineupPreset::Power:
		Loadout = &Class2Loadout;
		break;
	case EFlickLineupPreset::Speed:
		Loadout = &Class3Loadout;
		break;
	case EFlickLineupPreset::Control:
		Loadout = &Class4Loadout;
		break;
	case EFlickLineupPreset::Balanced:
	case EFlickLineupPreset::Custom:
	default:
		Loadout = &Class1Loadout;
		break;
	}
	return Loadout && Loadout->IsValidIndex(SlotIndex)
		? (*Loadout)[SlotIndex]
		: EFlickPieceArchetype::Standard;
}

void UFlickGameInstance::SetSelectedMatchVariant(const EFlickMatchVariant Variant)
{
	SelectedMatchVariant = NormalizeMatchVariant(Variant);
	SaveFrontendSettings();
}

void UFlickGameInstance::CycleLoadoutPiece(
	const EFlickTeam Team,
	const int32 SlotIndex,
	const int32 Direction)
{
	TArray<EFlickPieceArchetype>& Loadout = Team == EFlickTeam::Player2
		? Player2Loadout
		: Player1Loadout;
	if (!Loadout.IsValidIndex(SlotIndex))
	{
		return;
	}

	Loadout[SlotIndex] = FlickPieceArchetypeRules::Cycle(Loadout[SlotIndex], Direction);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetLoadoutPiece(
	const EFlickTeam Team,
	const int32 SlotIndex,
	const EFlickPieceArchetype Archetype)
{
	TArray<EFlickPieceArchetype>& Loadout = Team == EFlickTeam::Player2
		? Player2Loadout
		: Player1Loadout;
	if (!Loadout.IsValidIndex(SlotIndex))
	{
		return;
	}

	const int32 SafeValue = FMath::Clamp(
		static_cast<int32>(Archetype),
		static_cast<int32>(EFlickPieceArchetype::Standard),
		static_cast<int32>(EFlickPieceArchetype::Toppler));
	Loadout[SlotIndex] = static_cast<EFlickPieceArchetype>(SafeValue);
	SaveFrontendSettings();
}

void UFlickGameInstance::ApplyLoadoutPreset(
	const EFlickTeam Team,
	const EFlickLineupPreset Preset)
{
	if (Team == EFlickTeam::None || Preset == EFlickLineupPreset::Custom)
	{
		return;
	}

	TArray<EFlickPieceArchetype>& Loadout = Team == EFlickTeam::Player2
		? Player2Loadout
		: Player1Loadout;
	Loadout = FlickPieceArchetypeRules::GetPreset(Preset);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetClassLoadoutPiece(
	const EFlickLineupPreset Preset,
	const int32 SlotIndex,
	const EFlickPieceArchetype Archetype)
{
	TArray<EFlickPieceArchetype>* Loadout = nullptr;
	switch (Preset)
	{
	case EFlickLineupPreset::Power:
		Loadout = &Class2Loadout;
		break;
	case EFlickLineupPreset::Speed:
		Loadout = &Class3Loadout;
		break;
	case EFlickLineupPreset::Control:
		Loadout = &Class4Loadout;
		break;
	case EFlickLineupPreset::Balanced:
	case EFlickLineupPreset::Custom:
	default:
		Loadout = &Class1Loadout;
		break;
	}
	if (!Loadout || !Loadout->IsValidIndex(SlotIndex))
	{
		return;
	}

	const int32 SafeValue = FMath::Clamp(
		static_cast<int32>(Archetype),
		static_cast<int32>(EFlickPieceArchetype::Standard),
		static_cast<int32>(EFlickPieceArchetype::Toppler));
	(*Loadout)[SlotIndex] = static_cast<EFlickPieceArchetype>(SafeValue);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetAimGuideEnabled(const bool bEnabled)
{
	bAimGuideEnabled = bEnabled;
	SaveFrontendSettings();
}

void UFlickGameInstance::SetImpactEffectsEnabled(const bool bEnabled)
{
	bImpactEffectsEnabled = bEnabled;
	SaveFrontendSettings();
}

void UFlickGameInstance::SetControlOverviewEnabled(const bool bEnabled)
{
	bControlOverviewEnabled = bEnabled;
	SaveFrontendSettings();
}

void UFlickGameInstance::SetBotDifficulty(const EFlickBotDifficulty Difficulty)
{
	const int32 SafeValue = FMath::Clamp(
		static_cast<int32>(Difficulty),
		static_cast<int32>(EFlickBotDifficulty::Easy),
		static_cast<int32>(EFlickBotDifficulty::Expert));
	BotDifficulty = static_cast<EFlickBotDifficulty>(SafeValue);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetCameraShakeIntensity(const float Intensity)
{
	CameraShakeIntensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetMasterVolume(const float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetEffectsVolume(const float Volume)
{
	EffectsVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	SaveFrontendSettings();
}

void UFlickGameInstance::SetInterfaceVolume(const float Volume)
{
	InterfaceVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	SaveFrontendSettings();
}

void UFlickGameInstance::RecordCompletedMatch(
	const int32 Points,
	const int32 Knockouts,
	const int32 DoubleKnockouts,
	const int32 Shots,
	const TArray<int32>& AccoladeCounts,
	const bool bWon,
	const bool bDraw,
	const EFlickMatchVariant Variant,
	const int32 PlayersPerTeam,
	const bool bRanked)
{
	ProfileStats.RecordMatch(Points, Knockouts, DoubleKnockouts, Shots, bWon);
	ProfileStats.RecordAccolades(AccoladeCounts);
	FFlickProfileMatchRecord& Record = RecentMatches.InsertDefaulted_GetRef(0);
	Record.CompletedUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
	Record.Variant = NormalizeMatchVariant(Variant);
	Record.PlayersPerTeam = FMath::Clamp(PlayersPerTeam, 1, 3);
	Record.bRanked = bRanked;
	Record.bWon = bWon;
	Record.bDraw = bDraw;
	Record.Points = FMath::Max(0, Points);
	Record.Knockouts = FMath::Max(0, Knockouts);
	Record.DoubleKnockouts = FMath::Max(0, DoubleKnockouts);
	Record.Shots = FMath::Max(0, Shots);
	if (RecentMatches.Num() > MaxRecentProfileMatches)
	{
		RecentMatches.SetNum(MaxRecentProfileMatches);
	}
	SaveProfileStats();
}

void UFlickGameInstance::SaveProfileStats() const
{
	GConfig->SetInt(FlickProfileSection, TEXT("MatchesPlayed"), ProfileStats.MatchesPlayed, GGameUserSettingsIni);
	GConfig->SetInt(FlickProfileSection, TEXT("Wins"), ProfileStats.Wins, GGameUserSettingsIni);
	GConfig->SetInt(FlickProfileSection, TEXT("Points"), ProfileStats.Points, GGameUserSettingsIni);
	GConfig->SetInt(FlickProfileSection, TEXT("Knockouts"), ProfileStats.Knockouts, GGameUserSettingsIni);
	GConfig->SetInt(FlickProfileSection, TEXT("DoubleKnockouts"), ProfileStats.DoubleKnockouts, GGameUserSettingsIni);
	GConfig->SetInt(FlickProfileSection, TEXT("Shots"), ProfileStats.Shots, GGameUserSettingsIni);
	for (int32 Index = 0; Index < FlickAccoladeCount; ++Index)
	{
		GConfig->SetInt(
			FlickProfileSection,
			AccoladeConfigKeys[Index],
			ProfileStats.GetAccoladeCount(static_cast<EFlickAccolade>(Index)),
			GGameUserSettingsIni);
	}
	TArray<FString> SavedRecentMatches;
	SavedRecentMatches.Reserve(RecentMatches.Num());
	for (const FFlickProfileMatchRecord& Record : RecentMatches)
	{
		SavedRecentMatches.Add(SaveProfileMatchRecord(Record));
	}
	GConfig->SetArray(FlickProfileSection, TEXT("RecentMatches"), SavedRecentMatches, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UFlickGameInstance::SaveFrontendSettings() const
{
	GConfig->SetInt(
		FlickSettingsSection,
		TEXT("SelectedMatchVariant"),
		static_cast<int32>(SelectedMatchVariant),
		GGameUserSettingsIni);
	GConfig->SetArray(
		FlickSettingsSection,
		TEXT("Player1Loadout"),
		SaveLoadout(Player1Loadout),
		GGameUserSettingsIni);
	GConfig->SetArray(
		FlickSettingsSection,
		TEXT("Player2Loadout"),
		SaveLoadout(Player2Loadout),
		GGameUserSettingsIni);
	GConfig->SetArray(FlickSettingsSection, TEXT("Class1Loadout"), SaveLoadout(Class1Loadout), GGameUserSettingsIni);
	GConfig->SetArray(FlickSettingsSection, TEXT("Class2Loadout"), SaveLoadout(Class2Loadout), GGameUserSettingsIni);
	GConfig->SetArray(FlickSettingsSection, TEXT("Class3Loadout"), SaveLoadout(Class3Loadout), GGameUserSettingsIni);
	GConfig->SetArray(FlickSettingsSection, TEXT("Class4Loadout"), SaveLoadout(Class4Loadout), GGameUserSettingsIni);
	GConfig->SetBool(FlickSettingsSection, TEXT("AimGuideEnabled"), bAimGuideEnabled, GGameUserSettingsIni);
	GConfig->SetBool(FlickSettingsSection, TEXT("ImpactEffectsEnabled"), bImpactEffectsEnabled, GGameUserSettingsIni);
	GConfig->SetBool(FlickSettingsSection, TEXT("ControlOverviewEnabled"), bControlOverviewEnabled, GGameUserSettingsIni);
	GConfig->SetInt(FlickSettingsSection, TEXT("BotDifficulty"), static_cast<int32>(BotDifficulty), GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("CameraShakeIntensity"), CameraShakeIntensity, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("EffectsVolume"), EffectsVolume, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("InterfaceVolume"), InterfaceVolume, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
