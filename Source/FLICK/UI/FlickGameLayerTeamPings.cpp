#include "UI/FlickGameLayerPrivate.h"
#include "Player/FlickTeamPingComponent.h"
#include "Core/FlickQuickChats.h"

TSharedRef<SWidget> SFlickGameLayer::BuildTeamPingFeed()
{
	const auto Feed = [this]() -> UFlickTeamPingComponent*
	{ return PlayerController.IsValid() ? PlayerController->FindComponentByClass<UFlickTeamPingComponent>() : nullptr; };
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> Choices = SNew(SVerticalBox);
	Choices->AddSlot().AutoHeight().Padding(0, 0, 0, 6)
	[SNew(STextBlock).Font(UiFont(11, true)).ColorAndOpacity(Brand).Text_Lambda([Feed]() { return FText::FromString(FString::Printf(TEXT("TEAM QUICK CHAT / %s"), FlickQuickChats::GroupName(Feed() ? Feed()->GetQuickChatGroup() : 0))); })];
	for (int32 Slot = 0; Slot < 4; ++Slot)
		Choices->AddSlot().AutoHeight().Padding(0, 3)
		[SNew(STextBlock).Font(UiFont(11)).ColorAndOpacity(Paper).Text_Lambda([Feed, Slot]()
		{
			const auto* Phrase = FlickQuickChats::Find(FlickQuickChats::GetSlot(Feed() ? Feed()->GetQuickChatGroup() : 0, Slot));
			return FText::FromString(FString::Printf(TEXT("[%s]  %s"), *FlickControlBindings::GetKey(FlickQuickChats::ControlId(Slot)).GetDisplayName().ToString(), Phrase ? Phrase->Text : TEXT("")));
		})];
	Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
	[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Panel.CopyWithNewOpacity(.95f)).Padding(12)
	 .Visibility_Lambda([Feed]() { return Feed() && Feed()->GetQuickChatGroup() != INDEX_NONE ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })[Choices]];
	for (int32 Row = 0; Row < UFlickTeamPingComponent::MaximumMessages; ++Row)
	{
		Rows->AddSlot().AutoHeight().Padding(0, 0, 0, 5)
		[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor_Lambda([Feed, Row]() { return Panel.CopyWithNewOpacity(.85f * (Feed() ? Feed()->GetOpacity(Row) : 0.f)); }).Padding(FMargin(10, 7))
		 .Visibility_Lambda([Feed, Row]() { return Feed() && Feed()->GetOpacity(Row) > 0 ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		 [SNew(STextBlock).Text_Lambda([Feed, Row]() { return FText::FromString(Feed() ? Feed()->GetMessage(Row) : FString()); })
		  .Font(UiFont(11, true)).AutoWrapText(true)
		  .ColorAndOpacity_Lambda([Feed, Row]() { return (Feed() && Feed()->GetMessageTeam(Row) == EFlickTeam::Player2 ? Orange : Cyan).CopyWithNewOpacity(Feed() ? Feed()->GetOpacity(Row) : 0.f); })]];
	}
	Rows->AddSlot().AutoHeight().Padding(0, 3)
	[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Panel.CopyWithNewOpacity(.9f)).Padding(FMargin(10, 7))
	 .Visibility_Lambda([Feed]() { return Feed() && !Feed()->GetCooldownNotice().IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
	 [SNew(STextBlock).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)
	  .Text_Lambda([Feed]() { return FText::FromString(Feed() ? Feed()->GetCooldownNotice() : FString()); })]];
	return SNew(SBox).WidthOverride(360)
		.Visibility_Lambda([this, Feed]() { return ShouldShowGameplayControls() && Feed() && (Feed()->HasMessages() || Feed()->GetQuickChatGroup() != INDEX_NONE || !Feed()->GetCooldownNotice().IsEmpty()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		[Rows];
}
