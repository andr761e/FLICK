#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickLightingSettings.h"

TSharedRef<SWidget> SFlickGameLayer::BuildLightingSettings()
{
	using namespace FlickLightingSettings;
	const auto Scene = []() { return EScene::Menu; };
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
							.Percent_Lambda([Scene, Control]() { const auto& D = GetControl(Control); return TOptional<float>((GetValue(Scene(), Control) - D.Minimum) / (D.Maximum - D.Minimum)); })]
					]
					+ SOverlay::Slot()
					[
						SNew(SSlider).Style(&SliderStyle).MinValue(0.f).MaxValue(2.f).StepSize(.01f)
						.Value_Lambda([Scene, Control]() { const auto& D = GetControl(Control); return 2.f * (GetValue(Scene(), Control) - D.Minimum) / (D.Maximum - D.Minimum); })
						.OnValueChanged_Lambda([Scene, Control, Refresh](float Value) { const auto& D = GetControl(Control); SetValue(Scene(), Control, FMath::Lerp(D.Minimum, D.Maximum, Value * .5f)); Refresh(); })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(0.14f).VAlign(VAlign_Center).HAlign(HAlign_Right)
				[SNew(STextBlock).Text_Lambda([Scene, Control]()
				{
					const float Value = GetValue(Scene(), Control);
					const auto& D = GetControl(Control);
					const float DisplayValue = 2.f * (Value - D.Minimum) / (D.Maximum - D.Minimum);
					return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DisplayValue * 100.f)));
				}).Font(UiFont(11, true)).ColorAndOpacity(Brand)]
			]
		];
	}
	return SNew(SFlickAngularBorder).BackgroundColor(PanelRaised).AccentColor(Hairline).CutSize(10.0f).Padding(FMargin(24.0f, 18.0f))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[SNew(STextBlock).Text(FText::FromString(TEXT("ARENA LIGHTING"))).Font(DisplayFont(25)).ColorAndOpacity(Paper).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 12.0f)
		[SNew(STextBlock).Text(FText::FromString(TEXT("Arena and stadium lighting; shared across the main menu, Knockout and BOB. Puck lighting stays consistent. Saved locally."))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 4.0f)
		[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(bLightingPreview
			? TEXT("LIVE ARENA // Fill improves visibility without adding glare.")
			: TEXT("100% is recommended. The 0-200% sliders adjust lighting by up to 25%. PREVIEW ARENA shows live changes.")); }).Font(UiFont(10)).ColorAndOpacity(Cyan).AutoWrapText(true)]
		+ SVerticalBox::Slot().AutoHeight()[Rows]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBox)
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
