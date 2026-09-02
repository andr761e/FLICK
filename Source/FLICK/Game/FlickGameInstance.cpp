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
	constexpr int32 MaxLoadoutSlots = 4;

	TArray<EFlickPieceArchetype> MakeDefaultLoadout()
	{
		return FlickPieceArchetypeRules::GetPreset(EFlickLineupPreset::Balanced);
	}

	void LoadLoadout(const TCHAR* Key, TArray<EFlickPieceArchetype>& OutLoadout)
	{
		OutLoadout = MakeDefaultLoadout();
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
	LoadLoadout(TEXT("Player1Loadout"), Player1Loadout);
	LoadLoadout(TEXT("Player2Loadout"), Player2Loadout);

	GConfig->GetBool(FlickSettingsSection, TEXT("AimGuideEnabled"), bAimGuideEnabled, GGameUserSettingsIni);
	GConfig->GetBool(FlickSettingsSection, TEXT("ImpactEffectsEnabled"), bImpactEffectsEnabled, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("CameraShakeIntensity"), CameraShakeIntensity, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("EffectsVolume"), EffectsVolume, GGameUserSettingsIni);
	GConfig->GetFloat(FlickSettingsSection, TEXT("InterfaceVolume"), InterfaceVolume, GGameUserSettingsIni);
	CameraShakeIntensity = FMath::Clamp(CameraShakeIntensity, 0.0f, 1.0f);
	MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
	EffectsVolume = FMath::Clamp(EffectsVolume, 0.0f, 1.0f);
	InterfaceVolume = FMath::Clamp(InterfaceVolume, 0.0f, 1.0f);
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
	GConfig->SetBool(FlickSettingsSection, TEXT("AimGuideEnabled"), bAimGuideEnabled, GGameUserSettingsIni);
	GConfig->SetBool(FlickSettingsSection, TEXT("ImpactEffectsEnabled"), bImpactEffectsEnabled, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("CameraShakeIntensity"), CameraShakeIntensity, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("EffectsVolume"), EffectsVolume, GGameUserSettingsIni);
	GConfig->SetFloat(FlickSettingsSection, TEXT("InterfaceVolume"), InterfaceVolume, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}
