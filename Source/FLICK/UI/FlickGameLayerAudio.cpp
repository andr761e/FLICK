#include "UI/FlickGameLayerPrivate.h"
#include "Audio/FlickMenuRadioComponent.h"

TSharedRef<SWidget> SFlickGameLayer::BuildMenuRadio()
{
	auto Radio = [this]() -> UFlickMenuRadioComponent* { return PlayerController.IsValid() ? PlayerController->GetMenuRadio() : nullptr; };
	auto Button = [Radio](const TCHAR* Label, const TCHAR* Tooltip, int32 Action) -> TSharedRef<SWidget>
	{
		return SNew(SButton).ButtonStyle(FCoreStyle::Get(), "NoBorder").ContentPadding(FMargin(10, 7))
			.ToolTipText(FText::FromString(Tooltip))
			.OnClicked_Lambda([Radio, Action]()
			{
				if (auto* Player = Radio())
				{
					if (Action == 0) Player->TogglePlayback(); else Player->Skip(Action);
				}
				return FReply::Handled();
			})
			[SNew(STextBlock).Text_Lambda([Radio, Action, Label]()
			{
				return FText::FromString(Action == 0 ? (Radio() && Radio()->IsEnabled() ? TEXT("PAUSE") : TEXT("PLAY")) : Label);
			}).Font(UiFont(10, true)).ColorAndOpacity(Cyan)];
	};
	return SNew(SBox).WidthOverride(338)
		[SNew(SFlickAngularBorder).BackgroundColor(Ink.CopyWithNewOpacity(0.94f)).AccentColor(Cyan.CopyWithNewOpacity(0.6f))
		.CutSize(10).Padding(FMargin(16, 10))
		[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("FLICK RADIO"))).Font(UiFont(9, true)).ColorAndOpacity(Cyan)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[SNew(STextBlock)
			.Text_Lambda([Radio]() { return FText::FromString(Radio() ? Radio()->GetTrackTitle() : TEXT("TUNING IN...")); })
			.Font(UiFont(16, true)).ColorAndOpacity(Paper)]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock)
			.Text_Lambda([Radio]() { return FText::FromString(Radio() ? Radio()->GetStatus() : TEXT("")); })
			.Font(UiFont(9)).ColorAndOpacity(Muted)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 3)[SNew(SBox).HeightOverride(2)
			[SNew(SProgressBar).Percent_Lambda([Radio]() { return Radio() ? Radio()->GetProgress() : 0; }).FillColorAndOpacity(Cyan)]]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[Button(TEXT("PREV"), TEXT("Previous track"), -1)]
			+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Center)[Button(TEXT(""), TEXT("Pause or resume menu music"), 0)]
			+ SHorizontalBox::Slot().AutoWidth()[Button(TEXT("NEXT"), TEXT("Next track"), 1)]]]];
}
