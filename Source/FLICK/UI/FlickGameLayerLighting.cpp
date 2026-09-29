#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickLightingSettings.h"

TSharedRef<SWidget> SFlickGameLayer::BuildLightingSettings()
{
	using namespace FlickLightingSettings;
	if (LightingSceneOptions.IsEmpty())
	{
		LightingSceneOptions.Add(MakeShared<int32>(0));
		LightingSceneOptions.Add(MakeShared<int32>(1));
	}
	const auto Scene = [this]() { return bEditMenuLighting ? EScene::Menu : EScene::Gameplay; };
	const auto IsMenuContext = [this]()
	{
		return !GameMode.IsValid() || GameMode->GetSettingsReturnScreen() == EFlickFrontendScreen::MainMenu;
	};
	const auto Refresh = [this]() { if (PlayerController.IsValid()) PlayerController->RefreshLocalLighting(); };
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Index = 0; Index < static_cast<int32>(EControl::Count); ++Index)
	{
		const EControl Control = static_cast<EControl>(Index);
		const FControl& Definition = GetControl(Control);
		Rows->AddSlot().AutoHeight()
		[
			SNew(SBox).HeightOverride(44.0f).ToolTipText(FText::FromString(Definition.Description))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.43f).VAlign(VAlign_Center).Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[SNew(STextBlock).Text(FText::FromString(Definition.Label)).Font(UiFont(10, true)).ColorAndOpacity(Paper).AutoWrapText(true)]
				+ SHorizontalBox::Slot().FillWidth(0.43f).VAlign(VAlign_Center)
				[
					SNew(SOverlay)
					+ SOverlay::Slot().VAlign(VAlign_Center)
					[
						SNew(SBox).HeightOverride(6.0f)
						[SNew(SProgressBar).Style(&ShotClockBarStyle).FillColorAndOpacity(Brand)
							.Percent_Lambda([Scene, Control]() { return TOptional<float>(GetValue(Scene(), Control) / GetControl(Control).Maximum); })]
					]
					+ SOverlay::Slot()
					[
						SNew(SSlider).Style(&SliderStyle).MinValue(0.0f).MaxValue(Definition.Maximum).StepSize(0.01f)
						.Value_Lambda([Scene, Control]() { return GetValue(Scene(), Control); })
						.OnValueChanged_Lambda([Scene, Control, Refresh](float Value) { SetValue(Scene(), Control, Value); Refresh(); })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.14f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[SNew(STextBlock).Text_Lambda([Scene, Control]() { return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(GetValue(Scene(), Control) * 100.0f))); }).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			]
		];
	}
	return SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(Hairline).CutSize(10.0f).Padding(FMargin(24.0f, 18.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(FText::FromString(TEXT("ARENA LIGHTING"))).Font(DisplayFont(25)).ColorAndOpacity(Paper).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 12.0f)
		[SNew(STextBlock).Text(FText::FromString(TEXT("Shared by Knockout and BOB. Separate menu/match presets; saved locally."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SComboBox<TSharedPtr<int32>>).ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle).OptionsSource(&LightingSceneOptions)
			.OnGenerateWidget_Lambda([](TSharedPtr<int32> Option)
			{
				return SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor::Transparent).Padding(8.0f)
				[SNew(STextBlock).Text(FText::FromString(Option.IsValid() && *Option == 1 ? TEXT("GAMEPLAY") : TEXT("MAIN MENU"))).Font(UiFont(11, true)).ColorAndOpacity(Paper)];
			})
			.OnSelectionChanged_Lambda([this](TSharedPtr<int32> Option, ESelectInfo::Type) { if (Option.IsValid()) { bEditMenuLighting = *Option == 0; bLightingPreview = false; } })
			[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(bEditMenuLighting ? TEXT("MAIN MENU") : TEXT("GAMEPLAY")); }).Font(UiFont(11, true)).ColorAndOpacity(Paper)]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 4.0f)
		[SNew(STextBlock).Text_Lambda([this, IsMenuContext]()
		{
			return FText::FromString(bEditMenuLighting == IsMenuContext()
				? TEXT("Live in this scene. Fill increases brightness without adding glare.")
				: bEditMenuLighting ? TEXT("Return to the main menu to preview this preset.") : TEXT("Open settings during a Knockout or BOB match to preview this preset."));
		}).Font(UiFont(10)).ColorAndOpacity(Cyan).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight()[Rows]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBox).IsEnabled_Lambda([this, IsMenuContext]() { return bLightingPreview || bEditMenuLighting == IsMenuContext(); })
				[
					SNew(SBox).HeightOverride(44.0f)
					[
						SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(FMargin(12.0f, 8.0f))
						.OnClicked_Lambda([this]() { bLightingPreview = !bLightingPreview; return FReply::Handled(); })
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(bLightingPreview ? TEXT("BACK TO SETTINGS") : TEXT("PREVIEW ARENA")); }).Font(UiFont(11, true)).ColorAndOpacity(Brand).Justification(ETextJustify::Center)]
					]
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[MakeMenuButton(TEXT("RESET LIGHTING"), FOnClicked::CreateLambda([Refresh]() { ResetToDefaults(); Refresh(); return FReply::Handled(); }), false, false, 44.0f)]
		]
	];
}
