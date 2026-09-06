#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "Engine/GameInstance.h"
#include "FlickGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FFlickProfileMatchRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int64 CompletedUnixTime = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	EFlickMatchVariant Variant = EFlickMatchVariant::Classic;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 PlayersPerTeam = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	bool bRanked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	bool bWon = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	bool bDraw = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Points = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Knockouts = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 DoubleKnockouts = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Shots = 0;
};

USTRUCT(BlueprintType)
struct FFlickProfileStats
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 MatchesPlayed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Wins = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Points = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Knockouts = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 DoubleKnockouts = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	int32 Shots = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FLICK|Profile")
	TArray<int32> AccoladeCounts;

	void RecordMatch(int32 MatchPoints, int32 MatchKnockouts, int32 MatchDoubleKnockouts, int32 MatchShots, bool bWon);
	void RecordAccolades(const TArray<int32>& MatchAccoladeCounts);
	int32 GetTotalAccolades() const;
	int32 GetAccoladeCount(EFlickAccolade Accolade) const;
};

UCLASS()
class FLICK_API UFlickGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	void NotifyFrontendReady();
	bool ShouldShowStartupPresentation() const { return !bStartupPresentationComplete; }
	void CompleteStartupPresentation() { bStartupPresentationComplete = true; }

	EFlickMatchVariant GetSelectedMatchVariant() const { return SelectedMatchVariant; }
	EFlickPieceArchetype GetLoadoutPiece(EFlickTeam Team, int32 SlotIndex) const;
	EFlickLineupPreset GetLoadoutPreset(EFlickTeam Team) const;
	EFlickPieceArchetype GetClassLoadoutPiece(EFlickLineupPreset Preset, int32 SlotIndex) const;
	bool IsAimGuideEnabled() const { return bAimGuideEnabled; }
	bool AreImpactEffectsEnabled() const { return bImpactEffectsEnabled; }
	bool IsControlOverviewEnabled() const { return bControlOverviewEnabled; }
	EFlickBotDifficulty GetBotDifficulty() const { return BotDifficulty; }
	float GetCameraShakeIntensity() const { return CameraShakeIntensity; }
	float GetMasterVolume() const { return MasterVolume; }
	float GetEffectsVolume() const { return EffectsVolume; }
	float GetInterfaceVolume() const { return InterfaceVolume; }
	const FFlickProfileStats& GetProfileStats() const { return ProfileStats; }
	const TArray<FFlickProfileMatchRecord>& GetRecentMatches() const { return RecentMatches; }

	void SetSelectedMatchVariant(EFlickMatchVariant Variant);
	void CycleLoadoutPiece(EFlickTeam Team, int32 SlotIndex, int32 Direction);
	void SetLoadoutPiece(EFlickTeam Team, int32 SlotIndex, EFlickPieceArchetype Archetype);
	void ApplyLoadoutPreset(EFlickTeam Team, EFlickLineupPreset Preset);
	void SetClassLoadoutPiece(EFlickLineupPreset Preset, int32 SlotIndex, EFlickPieceArchetype Archetype);
	void SetAimGuideEnabled(bool bEnabled);
	void SetImpactEffectsEnabled(bool bEnabled);
	void SetControlOverviewEnabled(bool bEnabled);
	void SetBotDifficulty(EFlickBotDifficulty Difficulty);
	void SetCameraShakeIntensity(float Intensity);
	void SetMasterVolume(float Volume);
	void SetEffectsVolume(float Volume);
	void SetInterfaceVolume(float Volume);
	void RecordCompletedMatch(
		int32 Points,
		int32 Knockouts,
		int32 DoubleKnockouts,
		int32 Shots,
		const TArray<int32>& AccoladeCounts,
		bool bWon,
		bool bDraw,
		EFlickMatchVariant Variant,
		int32 PlayersPerTeam,
		bool bRanked);
	void SaveFrontendSettings() const;

private:
	void SaveProfileStats() const;

	EFlickMatchVariant SelectedMatchVariant = EFlickMatchVariant::Classic;
	TArray<EFlickPieceArchetype> Player1Loadout;
	TArray<EFlickPieceArchetype> Player2Loadout;
	TArray<EFlickPieceArchetype> Class1Loadout;
	TArray<EFlickPieceArchetype> Class2Loadout;
	TArray<EFlickPieceArchetype> Class3Loadout;
	TArray<EFlickPieceArchetype> Class4Loadout;
	bool bAimGuideEnabled = true;
	bool bImpactEffectsEnabled = true;
	bool bControlOverviewEnabled = true;
	EFlickBotDifficulty BotDifficulty = EFlickBotDifficulty::Normal;
	float CameraShakeIntensity = 0.75f;
	float MasterVolume = 0.85f;
	float EffectsVolume = 0.85f;
	float InterfaceVolume = 0.7f;
	FFlickProfileStats ProfileStats;
	TArray<FFlickProfileMatchRecord> RecentMatches;
	bool bStartupLoadingScreenConfigured = false;
	bool bStartupPresentationComplete = false;
};
