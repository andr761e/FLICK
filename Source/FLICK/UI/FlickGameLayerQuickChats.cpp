#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickQuickChats.h"

TSharedRef<SWidget> SFlickGameLayer::BuildQuickChatSettings()
{
	if (QuickChatPhraseOptions.IsEmpty())
		for (const auto& Phrase : FlickQuickChats::GetPhrases()) QuickChatPhraseOptions.Add(MakeShared<FString>(Phrase.Id));
	const auto Label = [](const FString& Id) { const auto* Phrase = FlickQuickChats::Find(Id); return FText::FromString(Phrase ? Phrase->Text : TEXT("")); };
	TSharedRef<SUniformGridPanel> Groups = SNew(SUniformGridPanel).SlotPadding(FMargin(6));
	for (int32 Group = 0; Group < 4; ++Group)
	{
		TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 12)
		[SNew(STextBlock).Font(UiFont(13, true)).ColorAndOpacity(Cyan).Text_Lambda([Group]()
		{ return FText::FromString(FString::Printf(TEXT("%s  /  %s"), *FlickControlBindings::GetKey(FlickQuickChats::ControlId(Group)).GetDisplayName().ToString(), FlickQuickChats::GroupName(Group))); })];
		for (int32 Slot = 0; Slot < 4; ++Slot)
		{
			Rows->AddSlot().AutoHeight().Padding(0, 3)
			[SNew(SHorizontalBox)
			 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
			 [SNew(SBox).WidthOverride(38)[SNew(STextBlock).Font(UiFont(11, true)).ColorAndOpacity(Muted).Text_Lambda([Slot]() { return FlickControlBindings::GetKey(FlickQuickChats::ControlId(Slot)).GetDisplayName(); })]]
			 + SHorizontalBox::Slot().FillWidth(1)
			 [SNew(SComboBox<TSharedPtr<FString>>).ComboBoxStyle(&DropdownStyle).ItemStyle(&DropdownRowStyle)
			  .OptionsSource(&QuickChatPhraseOptions)
			  .OnGenerateWidget_Lambda([Label](TSharedPtr<FString> Id) { return SNew(STextBlock).Text(Label(Id.IsValid() ? *Id : FString())).Font(UiFont(11)).ColorAndOpacity(Paper); })
			  .OnSelectionChanged_Lambda([Group, Slot](TSharedPtr<FString> Id, ESelectInfo::Type) { if (Id.IsValid()) FlickQuickChats::SetSlot(Group, Slot, *Id); })
			  [SNew(STextBlock).Text_Lambda([Group, Slot, Label]() { return Label(FlickQuickChats::GetSlot(Group, Slot)); }).Font(UiFont(12)).ColorAndOpacity(Paper)]]];
		}
		Groups->AddSlot(Group % 2, Group / 2)
		[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(PanelRaised).Padding(18)[Rows]];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("QUICK CHAT"))).Font(DisplayFont(25)).ColorAndOpacity(Paper)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 12)
		[SNew(STextBlock).Text(FText::FromString(TEXT("Press a group key, then a choice key to send. Team-only, with the same colour, fade and 2-second cooldown as pings. Escape cancels; groups close after 3 seconds. Rebind keys in Controls."))).AutoWrapText(true).Font(UiFont(11)).ColorAndOpacity(Muted)]
		+ SVerticalBox::Slot().AutoHeight()[Groups]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(6, 12)
		[SNew(SBox).WidthOverride(280)[MakeMenuButton(TEXT("RESTORE QUICK CHAT DEFAULTS"), FOnClicked::CreateLambda([]() { FlickQuickChats::Reset(); return FReply::Handled(); }), false, false, 44)]];
}
