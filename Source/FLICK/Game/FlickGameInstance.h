#pragma once

#include "CoreMinimal.h"
#include "Core/FlickTypes.h"
#include "Engine/GameInstance.h"
#include "FlickGameInstance.generated.h"

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
	bool IsAimGuideEnabled() const { return bAimGuideEnabled; }
	bool AreImpactEffectsEnabled() const { return bImpactEffectsEnabled; }
	float GetCameraShakeIntensity() const { return CameraShakeIntensity; }
	float GetMasterVolume() const { return MasterVolume; }
	float GetEffectsVolume() const { return EffectsVolume; }
	float GetInterfaceVolume() const { return InterfaceVolume; }

	void SetSelectedMatchVariant(EFlickMatchVariant Variant);
	void CycleLoadoutPiece(EFlickTeam Team, int32 SlotIndex, int32 Direction);
	void SetLoadoutPiece(EFlickTeam Team, int32 SlotIndex, EFlickPieceArchetype Archetype);
	void ApplyLoadoutPreset(EFlickTeam Team, EFlickLineupPreset Preset);
	void SetAimGuideEnabled(bool bEnabled);
	void SetImpactEffectsEnabled(bool bEnabled);
	void SetCameraShakeIntensity(float Intensity);
	void SetMasterVolume(float Volume);
	void SetEffectsVolume(float Volume);
	void SetInterfaceVolume(float Volume);
	void SaveFrontendSettings() const;

private:
	EFlickMatchVariant SelectedMatchVariant = EFlickMatchVariant::Classic;
	TArray<EFlickPieceArchetype> Player1Loadout;
	TArray<EFlickPieceArchetype> Player2Loadout;
	bool bAimGuideEnabled = true;
	bool bImpactEffectsEnabled = true;
	float CameraShakeIntensity = 0.75f;
	float MasterVolume = 0.85f;
	float EffectsVolume = 0.85f;
	float InterfaceVolume = 0.7f;
	bool bStartupLoadingScreenConfigured = false;
	bool bStartupPresentationComplete = false;
};
