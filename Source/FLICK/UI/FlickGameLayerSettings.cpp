#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickSettingsStyle.h"
#include "UI/FlickSettingsWidgets.h"

namespace
{
	struct FSettingsHelp { const TCHAR* Title; const TCHAR* Body; };
	FSettingsHelp SettingsHelp(EFlickSettingsTab Tab)
	{
		switch (Tab)
		{
		case EFlickSettingsTab::Camera: return {TEXT("ABOUT CAMERA // YOUR VIEW"), TEXT("Adjust the view, not the rules. Camera distance, field of view and sensitivity change how you see and move around the board, without changing puck physics or shot power.")};
		case EFlickSettingsTab::Interface: return {TEXT("ABOUT INTERFACE // READ THE BOARD"), TEXT("Choose the guides and information that help you read the board. Opponent indicators, hover details and colour-blind assistance are personal presentation preferences.")};
		case EFlickSettingsTab::Sound: return {TEXT("ABOUT AUDIO // YOUR MIX"), TEXT("Balance music, puck impacts and interface cues. The menu radio is also available from Home. Volume preferences save locally and do not change gameplay.")};
		case EFlickSettingsTab::StreamSafe: return {TEXT("ABOUT STREAM SAFE // MUSIC"), TEXT("Mute FLICK's menu and replay music while keeping gameplay and interface sounds. This preference does not control Steam overlays, voice chat or audio from other applications.")};
		case EFlickSettingsTab::Lighting: return {TEXT("ABOUT LIGHTING // LIVE PREVIEW"), TEXT("Preview your lighting on the actual arena. One shared preset is saved locally for the main menu, Knockout and BOB. Brightness, reflections and colour do not alter collision or puck physics.")};
		default: return {TEXT("ABOUT SHOT FEEL"), TEXT("These settings affect how your shots look and feel. Impact effects can help you see and judge contact, while mouse sensitivity changes how quickly your aim responds. Find what feels right for your playstyle.")};
		}
	}
}

TSharedRef<SWidget> SFlickGameLayer::BuildSettings()
{
	if (!bDisplayOptionsInitialized) RefreshDisplayOptions();
	const auto Checked = [](const bool bValue) { return bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };
	if (PuckHoverSizeOptions.IsEmpty())
		for (int32 Size : {10, 12, 14, 16, 18}) PuckHoverSizeOptions.Add(MakeShared<int32>(Size));
	if (PuckHoverDetailOptions.IsEmpty())
		for (int32 Detail = 0; Detail < 4; ++Detail) PuckHoverDetailOptions.Add(MakeShared<int32>(Detail));
	const auto HoverSizeLabel = [](int32 Size)
	{
		return FText::FromString(FString::Printf(TEXT("%d UI UNITS"), Size));
	};
	const auto HoverDetailLabel = [](int32 Detail)
	{
		static const TCHAR* Labels[] = {TEXT("OFF"), TEXT("NAME ONLY"), TEXT("PUCK TYPE ONLY"), TEXT("NAME + PUCK TYPE")};
		return FText::FromString(Labels[FMath::Clamp(Detail, 0, 3)]);
	};
	const auto MakeHoverSelector = [this](TArray<TSharedPtr<int32>>& Options,
		TFunction<int32()> Getter, TFunction<void(int32)> Setter, TFunction<FText(int32)> Label) -> TSharedRef<SWidget>
	{
		return SNew(SComboBox<TSharedPtr<int32>>)
			.ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle).OptionsSource(&Options)
			.OnGenerateWidget_Lambda([Label](TSharedPtr<int32> Option)
			{
				return SNew(STextBlock).Text(Label(Option.IsValid() ? *Option : 0)).Font(UiFont(10)).ColorAndOpacity(Paper);
			})
			.OnSelectionChanged_Lambda([Setter](TSharedPtr<int32> Option, ESelectInfo::Type)
			{
				if (Option.IsValid()) Setter(*Option);
			})
			[SNew(STextBlock).Text_Lambda([Getter, Label]() { return Label(Getter()); }).Font(UiFont(10, true)).ColorAndOpacity(Paper)];
	};
	const TSharedRef<SWidget> HoverSizeSelector = MakeHoverSelector(PuckHoverSizeOptions,
		FlickVisualSettings::GetPuckHoverSize, FlickVisualSettings::SetPuckHoverSize, HoverSizeLabel);
	const TSharedRef<SWidget> HoverDetailSelector = MakeHoverSelector(PuckHoverDetailOptions,
		FlickVisualSettings::GetPuckHoverDetail, FlickVisualSettings::SetPuckHoverDetail, HoverDetailLabel);
	if (PuckIndicatorOptions.IsEmpty())
		for (int32 Style = 0; Style < 3; ++Style) PuckIndicatorOptions.Add(MakeShared<int32>(Style));
	const TSharedRef<SWidget> IndicatorSelector = MakeHoverSelector(PuckIndicatorOptions,
		FlickVisualSettings::GetPuckIndicatorStyle, FlickVisualSettings::SetPuckIndicatorStyle,
		[](int32 Style) { static const TCHAR* Labels[] = {TEXT("COLOUR RINGS"), TEXT("SOLID ALLY / DASHED FOE"), TEXT("PATTERNS + ALLY / FOE LABELS")}; return FText::FromString(Labels[FMath::Clamp(Style, 0, 2)]); });
	if (CameraShakeOptions.IsEmpty())
	{
		for (int32 Level = 0; Level < 4; ++Level) CameraShakeOptions.Add(MakeShared<int32>(Level));
	}
	const auto ShakeLabel = [](int32 Level) -> FText
	{
		static const TCHAR* Labels[] = { TEXT("OFF"), TEXT("LOW"), TEXT("MEDIUM"), TEXT("HIGH") };
		return FText::FromString(Labels[FMath::Clamp(Level, 0, 3)]);
	};
	const TSharedRef<SWidget> ShakeSelector = SNew(SComboBox<TSharedPtr<int32>>)
		.ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle)
		.OptionsSource(&CameraShakeOptions)
		.OnGenerateWidget_Lambda([ShakeLabel](TSharedPtr<int32> Option)
		{
			return SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(8.0f, 5.0f))
			[SNew(STextBlock).Text(ShakeLabel(Option.IsValid() ? *Option : 0)).Font(UiFont(10)).ColorAndOpacity(Paper)];
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Option, ESelectInfo::Type)
		{
			if (Option.IsValid())
				if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr)
					I->SetCameraShakeIntensity(*Option == 0 ? 0.0f : *Option == 1 ? 0.25f : *Option == 2 ? 0.5f : 0.75f);
		})
		[SNew(STextBlock).Text_Lambda([this, ShakeLabel]()
		{
			const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
			const float Intensity = I ? I->GetCameraShakeIntensity() : 0.75f;
			return ShakeLabel(Intensity <= 0.01f ? 0 : Intensity < 0.375f ? 1 : Intensity < 0.625f ? 2 : 3);
		}).Font(UiFont(10, true)).ColorAndOpacity(Paper)];
	TSharedRef<SVerticalBox> ControlRows = SNew(SVerticalBox);
	FString LastGroup;
	for (const FlickControlBindings::FControl& Control : FlickControlBindings::GetControls())
	{
		if (LastGroup != Control.Group)
		{
			LastGroup = Control.Group;
			ControlRows->AddSlot().AutoHeight().Padding(0.0f, 13.0f, 0.0f, 5.0f)
			[SNew(STextBlock).Text(FText::FromString(LastGroup)).Font(UiFont(11, true)).ColorAndOpacity(Brand)];
		}
		const FString Id(Control.Id);
		ControlRows->AddSlot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(Control.Label)).Font(UiFont(12)).ColorAndOpacity(Paper)]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(210.0f)
				[
					SNew(SInputKeySelector)
					.ButtonStyle(&CompactMenuButtonStyle)
					.SelectedKey_Lambda([Id]() { return FInputChord(FlickControlBindings::GetKey(*Id)); })
					.Font(UiFont(12, true)).AllowModifierKeys(false).AllowGamepadKeys(false)
					.EscapeCancelsSelection(false)
					.OnKeySelected_Lambda([this, Id](const FInputChord& Chord)
					{
						if (FlickControlBindings::SetKey(*Id, Chord.Key))
						{
							if (PlayerController.IsValid()) PlayerController->RefreshControlBindings();
							ControlBindingMessage = TEXT("CONTROL SAVED");
						}
						else ControlBindingMessage = TEXT("KEY ALREADY IN USE OR UNSUPPORTED");
					})
				]
			]
		];
	}
	TSharedRef<SVerticalBox> BasicGraphicsRows = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> GraphicsRows = SNew(SVerticalBox);
	const auto QualityLabel = [](int32 Level) -> FText
	{
		static const TCHAR* Labels[] = {TEXT("LOW"), TEXT("MEDIUM"), TEXT("HIGH"), TEXT("EPIC")};
		return FText::FromString(Level >= 0 && Level < 4 ? Labels[Level] : TEXT("CUSTOM"));
	};
	const auto AddQualityRow = [&GraphicsRows, QualityLabel, this](const TCHAR* Label, auto Getter, auto Setter)
	{
		GraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(Label,
			TAttribute<FText>::CreateLambda([Getter, QualityLabel]()
			{
				const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
				return Settings ? QualityLabel((Settings->*Getter)()) : FText::FromString(TEXT("EPIC"));
			}),
			FOnClicked::CreateLambda([Getter, Setter]()
			{
				if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
				{
					(Settings->*Setter)(FMath::Clamp((Settings->*Getter)() - 1, 0, 3));
					Settings->ApplyNonResolutionSettings(); Settings->SaveSettings();
					FlickVisualSettings::ApplySaved();
				}
				return FReply::Handled();
			}),
			FOnClicked::CreateLambda([Getter, Setter]()
			{
				if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
				{
					(Settings->*Setter)(FMath::Clamp((Settings->*Getter)() + 1, 0, 3));
					Settings->ApplyNonResolutionSettings(); Settings->SaveSettings();
					FlickVisualSettings::ApplySaved();
				}
				return FReply::Handled();
			}))];
	};
	BasicGraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(TEXT("OVERALL QUALITY"),
		TAttribute<FText>::CreateLambda([QualityLabel]()
		{
			const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
			return Settings ? QualityLabel(Settings->GetOverallScalabilityLevel()) : FText::FromString(TEXT("EPIC"));
		}),
		FOnClicked::CreateLambda([]()
		{
			if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
			{
				const int32 Current = Settings->GetOverallScalabilityLevel();
				Settings->SetOverallScalabilityLevel(FMath::Clamp(Current < 0 ? 2 : Current - 1, 0, 3));
				Settings->ApplyNonResolutionSettings(); Settings->SaveSettings(); FlickVisualSettings::ApplySaved();
			}
			return FReply::Handled();
		}),
		FOnClicked::CreateLambda([]()
		{
			if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
			{
				const int32 Current = Settings->GetOverallScalabilityLevel();
				Settings->SetOverallScalabilityLevel(FMath::Clamp(Current < 0 ? 2 : Current + 1, 0, 3));
				Settings->ApplyNonResolutionSettings(); Settings->SaveSettings(); FlickVisualSettings::ApplySaved();
			}
			return FReply::Handled();
		}))];
	BasicGraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(TEXT("RENDER SCALE"),
		TAttribute<FText>::CreateLambda([]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FlickVisualSettings::GetRenderScale())); }),
		FOnClicked::CreateLambda([]()
		{
			static const int32 Steps[] = {50, 67, 75, 85, 100, 110};
			const int32 Current = FlickVisualSettings::GetRenderScale();
			int32 Next = Steps[0];
			for (const int32 Step : Steps) { if (Step < Current) Next = Step; }
			FlickVisualSettings::SetRenderScale(Next); return FReply::Handled();
		}),
		FOnClicked::CreateLambda([]()
		{
			static const int32 Steps[] = {50, 67, 75, 85, 100, 110};
			const int32 Current = FlickVisualSettings::GetRenderScale();
			int32 Next = Steps[UE_ARRAY_COUNT(Steps) - 1];
			for (const int32 Step : Steps) { if (Step > Current) { Next = Step; break; } }
			FlickVisualSettings::SetRenderScale(Next); return FReply::Handled();
		}))];
	BasicGraphicsRows->AddSlot().AutoHeight()[MakeCycleRow(TEXT("HARDWARE LUMEN"),
		TAttribute<FText>::CreateLambda([]() { return FText::FromString(FlickVisualSettings::IsHardwareLumenEnabled() ? TEXT("ON") : TEXT("OFF")); }),
		FOnClicked::CreateLambda([]() { FlickVisualSettings::SetHardwareLumenEnabled(false); return FReply::Handled(); }),
		FOnClicked::CreateLambda([]() { FlickVisualSettings::SetHardwareLumenEnabled(true); return FReply::Handled(); }))];
	AddQualityRow(TEXT("GLOBAL ILLUMINATION"), &UGameUserSettings::GetGlobalIlluminationQuality, &UGameUserSettings::SetGlobalIlluminationQuality);
	AddQualityRow(TEXT("REFLECTIONS"), &UGameUserSettings::GetReflectionQuality, &UGameUserSettings::SetReflectionQuality);
	AddQualityRow(TEXT("SHADOWS"), &UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality);
	AddQualityRow(TEXT("ANTI-ALIASING"), &UGameUserSettings::GetAntiAliasingQuality, &UGameUserSettings::SetAntiAliasingQuality);
	AddQualityRow(TEXT("VIEW DISTANCE"), &UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality);
	AddQualityRow(TEXT("POST-PROCESSING"), &UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality);
	AddQualityRow(TEXT("VISUAL EFFECTS"), &UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality);
	AddQualityRow(TEXT("TEXTURES"), &UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality);
	AddQualityRow(TEXT("SHADING"), &UGameUserSettings::GetShadingQuality, &UGameUserSettings::SetShadingQuality);
	const auto ResolutionLabel = [](const FIntPoint Resolution)
	{
		const float Aspect = Resolution.Y > 0 ? static_cast<float>(Resolution.X) / Resolution.Y : 16.0f / 9.0f;
		const TCHAR* Ratio = FMath::IsNearlyEqual(Aspect, 16.0f / 9.0f, 0.025f) ? TEXT("16:9")
			: FMath::IsNearlyEqual(Aspect, 21.0f / 9.0f, 0.08f) ? TEXT("21:9")
			: FMath::IsNearlyEqual(Aspect, 16.0f / 10.0f, 0.025f) ? TEXT("16:10")
			: FMath::IsNearlyEqual(Aspect, 4.0f / 3.0f, 0.025f) ? TEXT("4:3")
			: FMath::IsNearlyEqual(Aspect, 5.0f / 4.0f, 0.025f) ? TEXT("5:4") : TEXT("");
		return FText::FromString(FString::Printf(TEXT("%d x %d  %s"), Resolution.X, Resolution.Y, Ratio));
	};
	const auto ModeLabel = [](const int32 Mode)
	{
		return FText::FromString(Mode == 0 ? TEXT("FULLSCREEN") : Mode == 2 ? TEXT("WINDOWED") : TEXT("BORDERLESS"));
	};
	const auto LimitLabel = [](const int32 Limit)
	{
		return FText::FromString(Limit <= 0 ? TEXT("UNCAPPED") : FString::Printf(TEXT("%d FPS"), Limit));
	};
	const auto MakeOptionRow = [this](const TCHAR* Label, const TSharedRef<SWidget>& Selector) -> TSharedRef<SWidget>
	{
		return SNew(SBox).HeightOverride(45.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(11, true)).ColorAndOpacity(Paper)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(SBox).WidthOverride(230.0f)[Selector]]
		];
	};
	const TSharedRef<SWidget> ResolutionSelector = SNew(SComboBox<TSharedPtr<FIntPoint>>)
		.ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle)
		.OptionsSource(&ResolutionOptions).MaxListHeight(330.0f)
		.IsEnabled_Lambda([this]() { return PendingWindowMode != 1; })
		.OnGenerateWidget_Lambda([ResolutionLabel](TSharedPtr<FIntPoint> Option)
		{
			return SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(8.0f, 5.0f))
			[SNew(STextBlock).Text(Option.IsValid() ? ResolutionLabel(*Option) : FText::GetEmpty()).Font(UiFont(10)).ColorAndOpacity(Paper)];
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<FIntPoint> Option, ESelectInfo::Type)
		{
			if (Option.IsValid()) PendingResolution = *Option;
		})
		[SNew(STextBlock).Text_Lambda([this, ResolutionLabel]()
		{
			const UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
			return ResolutionLabel(PendingWindowMode == 1 && Settings ? Settings->GetDesktopResolution() : PendingResolution);
		}).Font(UiFont(10, true)).ColorAndOpacity(Paper)];
	const TSharedRef<SWidget> ModeSelector = SNew(SComboBox<TSharedPtr<int32>>)
		.ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle)
		.OptionsSource(&WindowModeOptions)
		.OnGenerateWidget_Lambda([ModeLabel](TSharedPtr<int32> Option)
		{
			return SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(8.0f, 5.0f))
			[SNew(STextBlock).Text(Option.IsValid() ? ModeLabel(*Option) : FText::GetEmpty()).Font(UiFont(10)).ColorAndOpacity(Paper)];
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Option, ESelectInfo::Type)
		{
			if (Option.IsValid()) PendingWindowMode = *Option;
		})
		[SNew(STextBlock).Text_Lambda([this, ModeLabel]() { return ModeLabel(PendingWindowMode); }).Font(UiFont(10, true)).ColorAndOpacity(Paper)];
	const TSharedRef<SWidget> FrameLimitSelector = SNew(SComboBox<TSharedPtr<int32>>)
		.ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle)
		.OptionsSource(&FrameLimitOptions).MaxListHeight(330.0f)
		.OnGenerateWidget_Lambda([LimitLabel](TSharedPtr<int32> Option)
		{
			return SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Transparent).Padding(FMargin(8.0f, 5.0f))
			[SNew(STextBlock).Text(Option.IsValid() ? LimitLabel(*Option) : FText::GetEmpty()).Font(UiFont(10)).ColorAndOpacity(Paper)];
		})
		.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Option, ESelectInfo::Type)
		{
			if (Option.IsValid()) PendingFrameLimit = *Option;
		})
		[SNew(STextBlock).Text_Lambda([this, LimitLabel]() { return LimitLabel(PendingFrameLimit); }).Font(UiFont(10, true)).ColorAndOpacity(Paper)];
	const auto SectionHeading = [](const FString& Number, const FString& Title, const FString& Description) -> TSharedRef<SWidget>
	{
		(void)Number;
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(FText::FromString(Title)).Font(DisplayFont(25)).ColorAndOpacity(Paper)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 10.0f)
			[SNew(STextBlock).Text(FText::FromString(Description)).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)];
	};

	const auto IsWide = [this]() {
		return SelectedSettingsTab == EFlickSettingsTab::Display || SelectedSettingsTab == EFlickSettingsTab::Controls
			|| SelectedSettingsTab == EFlickSettingsTab::QuickChat
#if !UE_BUILD_SHIPPING
			|| SelectedSettingsTab == EFlickSettingsTab::DevelopmentRanks
#endif
			;
	};
	const auto ImpactState = TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() {
		const auto* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
		return Checked(GameMode.IsValid() ? GameMode->AreImpactEffectsEnabled() : I && I->AreImpactEffectsEnabled());
	});
	const auto Sensitivity = TAttribute<float>::CreateLambda([this]() {
		const auto* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr;
		return GameMode.IsValid() ? GameMode->GetShotMouseSensitivity() : I ? I->GetShotMouseSensitivity() : .5f;
	});
	const TSharedRef<SWidget> FeelSlider = SNew(SBox).HeightOverride(34)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(SOverlay)
				+ SOverlay::Slot().VAlign(VAlign_Center)
				[SNew(SBox).HeightOverride(6)[SNew(SProgressBar).Style(&ShotClockBarStyle)
					.Percent_Lambda([Sensitivity]() { return TOptional<float>(Sensitivity.Get()); }).FillColorAndOpacity(Brand)]]
				+ SOverlay::Slot()
				[SNew(SSlider).Tag(TEXT("SettingsShotSensitivity")).Style(&SliderStyle).Value(Sensitivity)
					.OnValueChanged_Lambda([this](float Value) {
						if (GameMode.IsValid()) GameMode->SetShotMouseSensitivity(Value);
						else if (auto* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetShotMouseSensitivity(Value);
					})]]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Right).Padding(20,0,0,0)
			[SNew(SBox).WidthOverride(48)
				[SNew(STextBlock).Text_Lambda([Sensitivity]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Sensitivity.Get()*100))); })
					.Font(DisplayFont(14)).ColorAndOpacity(Brand).Justification(ETextJustify::Right)]]];
	const auto Rule = []() -> TSharedRef<SWidget> {
		return SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.20f))];
	};
	const TSharedRef<SWidget> GameFeel = SNew(SFlickAngularBorder).RefinedSurface(true).Tag(TEXT("SettingsGameplayPanel")).CutSize(13).Padding(FMargin(22,18))
		.UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.35f)).BackgroundColor(Ink.CopyWithNewOpacity(.78f))
		[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("GAMEPLAY"))).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
				+ SHorizontalBox::Slot().AutoWidth().Padding(18,0)[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(DisplayFont(23)).ColorAndOpacity(Cyan)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("SHOT FEEL"))).Font(DisplayFont(18)).ColorAndOpacity(Paper)]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,6,0,18)
			[SNew(STextBlock).Text(FText::FromString(TEXT("Tune aiming and the physical feedback of every shot."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight()[Rule()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,18,0,0)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("WORLD IMPACT EFFECTS"))).Font(DisplayFont(14)).ColorAndOpacity(Paper)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,6,16,0)
					[SNew(STextBlock).Text(FText::FromString(TEXT("Show visual effects when the puck hits surfaces in the arena.\nHelps you read the physical feedback of your shots."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
				[SNew(SBox).WidthOverride(64).HeightOverride(32)
					[SNew(SCheckBox).Tag(TEXT("SettingsImpactEffects")).Style(&SwitchToggleStyle).IsChecked(ImpactState)
						.OnCheckStateChanged_Lambda([this](ECheckBoxState) {
							if (GameMode.IsValid()) GameMode->SetImpactEffectsEnabled(!GameMode->AreImpactEffectsEnabled());
							else if (auto* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetImpactEffectsEnabled(!I->AreImpactEffectsEnabled());
						})[SNew(SFlickToggleGlyph).State(ImpactState)]]]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,22,0,17)[Rule()]
			+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("SHOT MOUSE SENSITIVITY"))).Font(DisplayFont(14)).ColorAndOpacity(Paper)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,6,0,12)
			[SNew(STextBlock).Text(FText::FromString(TEXT("Adjust how fast your aim moves when using a mouse.\nHigher values feel faster, lower values feel more precise."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]
			+ SVerticalBox::Slot().AutoHeight()[FeelSlider]];


	const TSharedRef<SWidget> SettingsContents = SNew(SOverlay)
+ SOverlay::Slot()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel ? EVisibility::Visible : EVisibility::Collapsed; })[GameFeel]]
+ SOverlay::Slot()
[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::QuickChat ? EVisibility::Visible : EVisibility::Collapsed; })[BuildQuickChatSettings()]]
#if !UE_BUILD_SHIPPING
+ SOverlay::Slot()
[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::DevelopmentRanks ? EVisibility::Visible : EVisibility::Collapsed; })[BuildDevelopmentRankSettings()]]
#endif
+ SOverlay::Slot()
[
	SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Lighting ? EVisibility::Visible : EVisibility::Collapsed; })
	[BuildLightingSettings()]
]
+ SOverlay::Slot()
[
	SNew(SBorder).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Controls ? EVisibility::Visible : EVisibility::Collapsed; })
	.BorderImage(WhiteBrush()).BorderBackgroundColor(Ink.CopyWithNewOpacity(.78f)).Padding(FMargin(24.0f, 18.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT(""), TEXT("CONTROLS"), TEXT("Select a binding, then press a new key or mouse button. Duplicate keys are rejected."))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 10.0f)
		[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ControlBindingMessage); }).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.0f, 16.0f)
		[SNew(SBox).WidthOverride(270.0f)[MakeMenuButton(TEXT("RESET TO DEFAULTS"), FOnClicked::CreateLambda([this]()
		{
			FlickControlBindings::ResetToDefaults();
			if (PlayerController.IsValid()) PlayerController->RefreshControlBindings();
			ControlBindingMessage = TEXT("ALL CONTROLS RESET TO DEFAULT");
			return FReply::Handled();
		}), false, false, 44.0f)]]
		+ SVerticalBox::Slot().AutoHeight()[ControlRows]
	]
]
+ SOverlay::Slot()
[
	SNew(SVerticalBox)
	.Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera || SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Ink.CopyWithNewOpacity(.78f)).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.35f)).CutSize(10.0f).Padding(FMargin(24.0f, 18.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[SectionHeading(TEXT("02"), TEXT("CAMERA"), TEXT("Adjust camera motion, orbit response, and free-camera control."))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[SectionHeading(TEXT("03"), TEXT("INTERFACE"), TEXT("Choose the guides and information shown during play."))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeOptionRow(TEXT("OTHER PUCK INDICATORS"), IndicatorSelector)]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeOptionRow(TEXT("PUCK HOVER TEXT SIZE"), HoverSizeSelector)]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeOptionRow(TEXT("PUCK HOVER INFORMATION"), HoverDetailSelector)]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeToggleRow(TEXT("COLOUR-BLIND ASSISTANCE"), TAttribute<ECheckBoxState>::CreateLambda([Checked]() { return Checked(FlickVisualSettings::IsColorBlindAssistEnabled()); }), FOnCheckStateChanged::CreateLambda([](ECheckBoxState State) { FlickVisualSettings::SetColorBlindAssistEnabled(State == ECheckBoxState::Checked); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeToggleRow(TEXT("AIM AND CONTACT GUIDE"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return Checked(GameMode.IsValid() ? GameMode->IsAimGuideEnabled() : I && I->IsAimGuideEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetAimGuideEnabled(!GameMode->IsAimGuideEnabled()); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetAimGuideEnabled(!I->IsAimGuideEnabled()); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Interface ? EVisibility::Visible : EVisibility::Collapsed; })[MakeToggleRow(TEXT("CONTROL OVERVIEW"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return Checked(GameMode.IsValid() ? GameMode->IsControlOverviewEnabled() : I && I->IsControlOverviewEnabled()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { if (GameMode.IsValid()) GameMode->SetControlOverviewEnabled(!GameMode->IsControlOverviewEnabled()); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetControlOverviewEnabled(!I->IsControlOverviewEnabled()); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("CAMERA DISTANCE"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? (I->GetGameplayCameraDistance() - 0.75f) / 0.5f : 0.3f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetGameplayCameraDistance(FMath::Lerp(0.75f, 1.25f, Value)); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("FIELD OF VIEW"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? (I->GetGameplayCameraFieldOfView() - 40.0f) / 30.0f : 0.3067f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetGameplayCameraFieldOfView(FMath::Lerp(40.0f, 70.0f, Value)); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeOptionRow(TEXT("CAMERA SHAKE"), ShakeSelector)]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("GAMEPLAY CAMERA SENSITIVITY"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetGameplayCameraSensitivity() : 0.35f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetGameplayCameraSensitivity(Value); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("FREE CAMERA LOOK"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetFreeCameraLookSensitivity() : 0.33f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetFreeCameraLookSensitivity(Value); }))]]
			+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Camera ? EVisibility::Visible : EVisibility::Collapsed; })[MakeSliderRow(TEXT("FREE CAMERA MOVEMENT"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetFreeCameraMoveSensitivity() : 0.38f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetFreeCameraMoveSensitivity(Value); }))]]
		]
	]

]
+ SOverlay::Slot()
[
	SNew(SVerticalBox)
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SFlickAngularBorder).RefinedSurface(true).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Sound ? EVisibility::Visible : EVisibility::Collapsed; }).BackgroundColor(Ink.CopyWithNewOpacity(.78f)).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.35f)).CutSize(10.0f).Padding(FMargin(24.0f, 18.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT("02"), TEXT("SOUND"), TEXT("Set the balance of the arena and the interface."))]
			+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("MASTER"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return GameMode.IsValid() ? GameMode->GetMasterVolume() : I ? I->GetMasterVolume() : 1.0f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (GameMode.IsValid()) GameMode->SetMasterVolume(Value); else if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetMasterVolume(Value); }))]
			+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("PHYSICS EFFECTS"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetEffectsVolume() : 0.85f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetEffectsVolume(Value); }))]
			+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("INTERFACE"), TAttribute<float>::CreateLambda([this]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetInterfaceVolume() : 0.7f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetInterfaceVolume(Value); }))]
			+ SVerticalBox::Slot().AutoHeight()[MakeSliderRow(TEXT("MUSIC"), TAttribute<float>::CreateLambda([this]() { const auto* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return I ? I->GetMusicVolume() : 0.45f; }), FOnFloatValueChanged::CreateLambda([this](float Value) { if (auto* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetMusicVolume(Value); }))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)[BuildMenuRadio()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[SNew(STextBlock).Text(FText::FromString(TEXT("Music fades out for matches. Interface volume controls your-turn and team-ping cues. Gameplay effects include switches, dividers and BOB pockets."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]
		]
	]
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SFlickAngularBorder).RefinedSurface(true).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::StreamSafe ? EVisibility::Visible : EVisibility::Collapsed; }).BackgroundColor(Ink.CopyWithNewOpacity(.78f)).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.35f)).CutSize(10.0f).Padding(FMargin(24.0f, 18.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT(""), TEXT("STREAM SAFE"), TEXT("Mute menu and replay music without muting gameplay or interface feedback."))]
			+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("MUTE ALL MUSIC"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { const UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr; return Checked(I && I->IsReplayMusicMutedForStreaming()); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State) { if (UFlickGameInstance* I = PlayerController.IsValid() ? Cast<UFlickGameInstance>(PlayerController->GetGameInstance()) : nullptr) I->SetReplayMusicMutedForStreaming(State == ECheckBoxState::Checked); }))]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK's menu radio and replay music are original procedural instrumentals. This setting does not affect Steam overlays or other system audio."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]
		]
	]
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SFlickAngularBorder).RefinedSurface(true).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Display ? EVisibility::Visible : EVisibility::Collapsed; }).BackgroundColor(Ink.CopyWithNewOpacity(.78f)).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.35f)).CutSize(10.0f).Padding(FMargin(24.0f, 18.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[SectionHeading(TEXT(""), TEXT("VIDEO"), TEXT("Choose a supported display mode, then apply it. Graphics quality saves immediately."))]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 20.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("WINDOW SETTINGS"))).Font(UiFont(12, true)).ColorAndOpacity(Cyan)]
					+ SVerticalBox::Slot().AutoHeight()[MakeOptionRow(TEXT("DISPLAY MODE"), ModeSelector)]
					+ SVerticalBox::Slot().AutoHeight()[MakeOptionRow(TEXT("RESOLUTION"), ResolutionSelector)]
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(TEXT("Borderless always uses your desktop resolution."))).Font(UiFont(9)).ColorAndOpacity(Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
					[MakeToggleRow(TEXT("VERTICAL SYNC"), TAttribute<ECheckBoxState>::CreateLambda([this, Checked]() { return Checked(bPendingVSync); }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState State) { bPendingVSync = State == ECheckBoxState::Checked; }))]
					+ SVerticalBox::Slot().AutoHeight()[MakeOptionRow(TEXT("FRAME RATE LIMIT"), FrameLimitSelector)]
					+ SVerticalBox::Slot().AutoHeight()[MakeToggleRow(TEXT("DISPLAY CURRENT FPS"), TAttribute<ECheckBoxState>::CreateLambda([Checked]() { return Checked(FlickVisualSettings::IsFpsVisible()); }), FOnCheckStateChanged::CreateLambda([](ECheckBoxState State) { FlickVisualSettings::SetFpsVisible(State == ECheckBoxState::Checked); }))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 15.0f, 0.0f, 7.0f)
					[SNew(STextBlock).Text(FText::FromString(TEXT("BASIC QUALITY"))).Font(UiFont(12, true)).ColorAndOpacity(Cyan)]
					+ SVerticalBox::Slot().AutoHeight()[BasicGraphicsRows]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[SNew(STextBlock).Text(FText::FromString(TEXT("ADVANCED QUALITY"))).Font(UiFont(12, true)).ColorAndOpacity(Cyan)]
					+ SVerticalBox::Slot().FillHeight(1.0f)
					[SNew(SScrollBox) + SScrollBox::Slot()[GraphicsRows]]
				]
			]
		]
	]
];

	TSharedRef<SVerticalBox> Navigation = SNew(SVerticalBox);
	const auto MakeSettingsTab = [this](EFlickSettingsTab Tab, const TCHAR* Label) -> TSharedRef<SWidget> {
		const auto Selected = [this,Tab]() { return SelectedSettingsTab == Tab; };
		const auto Color = TAttribute<FLinearColor>::CreateLambda([Selected]() { return Selected() ? Ink : Paper; });
		const EFlickMainMenuIcon Icon = Tab == EFlickSettingsTab::Camera ? EFlickMainMenuIcon::Camera
			: Tab == EFlickSettingsTab::Interface ? EFlickMainMenuIcon::Display
			: Tab == EFlickSettingsTab::Sound ? EFlickMainMenuIcon::Sound
			: Tab == EFlickSettingsTab::Controls ? EFlickMainMenuIcon::Controls
			: Tab == EFlickSettingsTab::QuickChat ? EFlickMainMenuIcon::Chat : EFlickMainMenuIcon::Code;
		const int32 Kind = Tab == EFlickSettingsTab::GameFeel ? 0 : Tab == EFlickSettingsTab::StreamSafe ? 1
			: Tab == EFlickSettingsTab::Display ? 2 : Tab == EFlickSettingsTab::Lighting ? 3 : -1;
		TSharedRef<SWidget> Glyph = Kind >= 0
			? StaticCastSharedRef<SWidget>(SNew(FlickSettingsWidgets::SGlyph).Kind(Kind).Color(Color))
			: StaticCastSharedRef<SWidget>(SNew(SFlickMainMenuIcon).Icon(Icon).Color(Color));
		bool bDev = false;
#if !UE_BUILD_SHIPPING
		bDev = Tab == EFlickSettingsTab::DevelopmentRanks;
#endif
		TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("SettingsTab%d"), static_cast<int32>(Tab))))
			.ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
			.OnClicked_Lambda([this,Tab]() { SelectedSettingsTab=Tab; bLightingPreview=false; if (GameMode.IsValid()) GameMode->PlayMenuSound(true); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak = Button;
		Button->SetContent(SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(7).Padding(FMargin(18,0)).UseAccentForOutline(true).DrawNeutralOutline(false)
			.BackgroundColor_Lambda([Selected,Weak]() { const auto P=Weak.Pin(); return Selected() ? Brand : P && (P->IsHovered() || P->HasKeyboardFocus()) ? PanelRaised : FLinearColor::Transparent; })
			.AccentColor_Lambda([Selected,Weak]() { const auto P=Weak.Pin(); return Selected() || (P && P->HasKeyboardFocus()) ? Brand : FLinearColor::Transparent; })
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,18,0)
				[SNew(SBox).WidthOverride(30).HeightOverride(30)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[Glyph]]]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(13)).ColorAndOpacity_Lambda([Color]() { return FSlateColor(Color.Get()); })]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6,0,18,0)
				[SNew(SFlickAngularBorder).RefinedSurface(true).Visibility(bDev ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
					.UseAccentForOutline(true).AccentColor(Cyan).CutSize(5).Padding(FMargin(5,2)).BackgroundColor(Ink)
					[SNew(STextBlock).Text(FText::FromString(TEXT("DEV"))).Font(DisplayFont(9)).ColorAndOpacity(Cyan)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(STextBlock).Text(FText::FromString(TEXT("\u203A"))).Font(UiFont(23)).ColorAndOpacity_Lambda([Color]() { return FSlateColor(Color.Get()); })]]);
		return SNew(SBox).HeightOverride(FlickSettingsStyle::RailRowHeight)[Button];
	};
	const auto AddTab = [&Navigation,&MakeSettingsTab,&Rule](EFlickSettingsTab Tab, const TCHAR* Label) {
		Navigation->AddSlot().AutoHeight()[MakeSettingsTab(Tab,Label)];
		Navigation->AddSlot().AutoHeight()[Rule()];
	};
	AddTab(EFlickSettingsTab::GameFeel,TEXT("GAMEPLAY")); AddTab(EFlickSettingsTab::Camera,TEXT("CAMERA"));
	AddTab(EFlickSettingsTab::Interface,TEXT("INTERFACE")); AddTab(EFlickSettingsTab::StreamSafe,TEXT("STREAM SAFE"));
	AddTab(EFlickSettingsTab::Display,TEXT("VIDEO")); AddTab(EFlickSettingsTab::Lighting,TEXT("LIGHTING"));
	AddTab(EFlickSettingsTab::Sound,TEXT("AUDIO")); AddTab(EFlickSettingsTab::Controls,TEXT("CONTROLS"));
	AddTab(EFlickSettingsTab::QuickChat,TEXT("QUICK CHAT"));
#if !UE_BUILD_SHIPPING
	AddTab(EFlickSettingsTab::DevelopmentRanks,TEXT("DEV RANKS"));
#endif

	const TSharedRef<SWidget> Help = SNew(SBox).HeightOverride(FlickSettingsStyle::HelpHeight)
		.Visibility_Lambda([IsWide]() { return IsWide() ? EVisibility::Collapsed : EVisibility::Visible; })
		[SNew(SFlickAngularBorder).RefinedSurface(true).Tag(TEXT("SettingsHelp")).CutSize(13).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.5f))
			.BackgroundColor(Ink.CopyWithNewOpacity(.82f)).Padding(12)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).Padding(8,4,20,4)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,16,0)
						[SNew(SBox).WidthOverride(28).HeightOverride(28)[SNew(FlickSettingsWidgets::SGlyph).Kind(4).Color(Cyan)]]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SettingsHelp(SelectedSettingsTab).Title); }).Font(DisplayFont(13)).ColorAndOpacity(Paper).AutoWrapText(true)]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,9,0,0)
					[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SettingsHelp(SelectedSettingsTab).Body); }).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SBox).WidthOverride(FlickSettingsStyle::HelpImageWidth)
					[SNew(SOverlay)
						+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::SettingsImpact).CutSize(8)
							.Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })]
						+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::Training).CutSize(8)
							.Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::GameFeel ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]]]]];

	TSharedRef<SButton> Back = SNew(SButton).Tag(TEXT("SettingsBack")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() {
			RefreshDisplayOptions();
			if (GameMode.IsValid()) GameMode->CloseSettings();
			else if (PlayerController.IsValid() && PlayerController->GetLocalMatchMenuScreen() == EFlickFrontendScreen::Settings) PlayerController->SetLocalMatchMenuScreen(EFlickFrontendScreen::Paused);
			else RemotePartyScreen=EFlickFrontendScreen::MainMenu;
			return FReply::Handled();
		});
	Back->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(Back)));

	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Tag(TEXT("SettingsBackdrop")).Artwork(EFlickMenuArtwork::SettingsBackdrop)
			.Visibility_Lambda([this]() { return bLightingPreview ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(24)
		[SNew(SBox).WidthOverride(560).Visibility_Lambda([this]() { return bLightingPreview ? EVisibility::Visible : EVisibility::Collapsed; })[BuildLightingSettings()]]
		+ SOverlay::Slot().Padding(FMargin(48,24,48,26))
		[SNew(SBox).Visibility_Lambda([this]() { return bLightingPreview ? EVisibility::Collapsed : EVisibility::Visible; })
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)
				[SNew(SBox).HeightOverride(90)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(SBox).WidthOverride(260).HeightOverride(82)
							[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SFlickLogoWidget).VectorPath(TEXT("UI/FlickKnockoutWordmark.svg"))
								.TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark")).DesiredSize(FVector2D(2048,683))]]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22,0)
						[SNew(SBox).WidthOverride(1).HeightOverride(42)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("HOME"))).Font(DisplayFont(14)).ColorAndOpacity(Muted)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(13,0)[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(DisplayFont(14)).ColorAndOpacity(Cyan)]
							+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("SETTINGS"))).Font(DisplayFont(14)).ColorAndOpacity(Paper)]]]]
				+ SVerticalBox::Slot().FillHeight(1)
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0,8,16,0)
					[SNew(SBox).WidthOverride(FlickSettingsStyle::RailWidth)
						[SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(14).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.6f))
							.BackgroundColor(Ink.CopyWithNewOpacity(.92f)).Padding(FMargin(3,8))
							[SNew(SScrollBox).ScrollBarThickness(FVector2D(3,3)) + SScrollBox::Slot()[Navigation]]]]
					+ SHorizontalBox::Slot().AutoWidth()
					[SNew(SBox).WidthOverride_Lambda([IsWide]() { return IsWide() ? FlickSettingsStyle::WidePanelWidth : FlickSettingsStyle::PanelWidth; })
						[SNew(SFlickAngularBorder).RefinedSurface(true).Tag(TEXT("SettingsContentFrame")).CutSize(18).UseAccentForOutline(true)
							.AccentColor(Cyan.CopyWithNewOpacity(.6f)).BackgroundColor(Ink.CopyWithNewOpacity(.80f)).Padding(FMargin(20,20,20,18))
							[SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight().Padding(4,0,0,14)
								[SNew(STextBlock).Text(FText::FromString(TEXT("MAKE IT YOURS."))).Font(DisplayFont(44)).ColorAndOpacity(Paper)]
								+ SVerticalBox::Slot().FillHeight(1)
								[SNew(SScrollBox).Tag(TEXT("SettingsContentScroll")).ScrollBarThickness(FVector2D(5,5))
									+ SScrollBox::Slot()
									[SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[SettingsContents]
										+ SVerticalBox::Slot().AutoHeight().Padding(0,18,0,0)[Help]]]]]]
					+ SHorizontalBox::Slot().FillWidth(1)[SNew(SBox)]]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,20,0,0)
				[SNew(SBox).HeightOverride(56)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(174)[Back]]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(24,0)
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedSettingsTab == EFlickSettingsTab::Controls
							? TEXT("Controls save immediately.") : SelectedSettingsTab == EFlickSettingsTab::Display ? TEXT("Apply display mode, resolution, V-Sync and FPS changes.") : TEXT("Changes save immediately.")); }).Font(UiFont(11)).ColorAndOpacity(Muted)]
						+ SHorizontalBox::Slot().AutoWidth()
						[SNew(SBox).WidthOverride(250).Visibility_Lambda([this]() { return SelectedSettingsTab == EFlickSettingsTab::Display ? EVisibility::Visible : EVisibility::Collapsed; })
							[MakeMenuButton(TEXT("APPLY SETTINGS"), FOnClicked::CreateLambda([this]() { ApplyPendingDisplaySettings(); return FReply::Handled(); }),true,false,52)]]]]]];
}
