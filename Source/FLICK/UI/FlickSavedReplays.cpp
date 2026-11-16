#include "UI/FlickSavedReplays.h"
#include "UI/FlickGameLayerPrivate.h"
#include "Game/FlickReplaySubsystem.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"

namespace
{
	class SSavedReplays : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SSavedReplays) {} SLATE_ARGUMENT(TWeakObjectPtr<UFlickReplaySubsystem>,Replay) SLATE_EVENT(FOnClicked,Back) SLATE_END_ARGS()
		void Construct(const FArguments& A)
		{
			Replay=A._Replay;
			Style=FButtonStyle().SetNormal(FSlateNoResource()).SetHovered(FSlateNoResource()).SetPressed(FSlateNoResource());
			NameStyle=FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
			NameStyle.SetBackgroundImageNormal(FSlateNoResource()).SetBackgroundImageHovered(FSlateNoResource()).SetBackgroundImageFocused(FSlateNoResource())
			 .SetForegroundColor(Paper).SetFont(UiFont(18,true)).SetPadding(FMargin(3,4));
			SAssignNew(Rows,SVerticalBox);
			ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1240).HeightOverride(720)
			[SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Ink).AccentColor(Cyan).UseAccentForOutline(true).CutSize(18).Padding(28)
			[SNew(SVerticalBox)
			 + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("PROFILE  //  SAVED REPLAYS"))).Font(UiFont(13,true)).ColorAndOpacity(Brand)]
			 + SVerticalBox::Slot().AutoHeight().Padding(0,8)[SNew(STextBlock).Text(FText::FromString(TEXT("EVERY ANGLE. EVERY MOMENT."))).Font(DisplayFont(32)).ColorAndOpacity(Paper)]
			 + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,18)[SNew(STextBlock).Text(FText::FromString(TEXT("Revisit complete matches. Follow any player, inspect a shot, or find your own camera angle."))).Font(UiFont(12)).ColorAndOpacity(Muted)]
			 + SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
			   + SHorizontalBox::Slot().FillWidth(1)[SNew(SSearchBox).HintText(FText::FromString(TEXT("Search names, players or playlist"))).OnTextChanged_Lambda([this](const FText& T) { Search=T.ToString(); Refresh(); })]
			   + SHorizontalBox::Slot().AutoWidth().Padding(10,0)[Button(TEXT("NEWEST"),[this]() {Sort=0;Refresh();})]
			   + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("FAVOURITES"),[this]() {Sort=1;Refresh();})]
			   + SHorizontalBox::Slot().AutoWidth().Padding(10,0)[Button(TEXT("MODE"),[this]() {Sort=2;Refresh();})]
			   + SHorizontalBox::Slot().AutoWidth()[Button(TEXT("REFRESH"),[this]() {Refresh();})]]
			 + SVerticalBox::Slot().AutoHeight().Padding(0,10)[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(Summary); }).Font(UiFont(11)).ColorAndOpacity(Muted)]
			 + SVerticalBox::Slot().FillHeight(1)[SNew(SScrollBox)+SScrollBox::Slot()[Rows.ToSharedRef()]]
			 + SVerticalBox::Slot().AutoHeight().Padding(0,10)[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(!WatchPending.IsEmpty()?TEXT("Watching leaves your online room and party. If you host, the room closes. Click LEAVE ROOM & WATCH to confirm."):Replay.IsValid()?Replay->GetStatus():FString()); }).Font(UiFont(12,true)).ColorAndOpacity(Brand).AutoWrapText(true)]
			 + SVerticalBox::Slot().AutoHeight()[SNew(SButton).ButtonStyle(&Style).ContentPadding(0).OnClicked(A._Back)
			   [SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Cyan).UseAccentForOutline(true).CutSize(8).Padding(12)
			     [SNew(STextBlock).Text(FText::FromString(TEXT("BACK TO PROFILE"))).Font(UiFont(13,true)).ColorAndOpacity(Paper)]]]
			]]]];
			SetCanTick(true);
			Refresh();
		}
		virtual void Tick(const FGeometry& G,double T,float D) override
		{
			SCompoundWidget::Tick(G,T,D);
			if (Replay.IsValid()&&Revision!=Replay->GetLibraryRevision()) Refresh();
		}
	private:
		TSharedRef<SWidget> Button(const FString& Text,TFunction<void()> Action)
		{
			return SNew(SButton).ButtonStyle(&Style).ContentPadding(0).OnClicked_Lambda([Action]() {Action();return FReply::Handled();})
			[SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Cyan.CopyWithNewOpacity(.55f)).UseAccentForOutline(true).CutSize(7).Padding(FMargin(12,8))
			 [SNew(STextBlock).Text(FText::FromString(Text)).Font(UiFont(11,true)).ColorAndOpacity(Paper)]];
		}
		void Refresh()
		{
			if (!Rows.IsValid()||!Replay.IsValid()) return;
			Revision=Replay->GetLibraryRevision(); Rows->ClearChildren();
			auto Entries=Replay->List(); int64 Bytes=0;TSet<FString> Payloads;
			for(const auto& E:Entries) if(!Payloads.Contains(E.PayloadId())) {Payloads.Add(E.PayloadId());Bytes+=E.Bytes;}
			Summary=FString::Printf(TEXT("%d SAVED REPLAYS / CLIPS   //   %.1f MB ON THIS COMPUTER   //   EDIT A TITLE TO RENAME"),Entries.Num(),Bytes/(1024.0*1024.0));
			if (Sort==0) Entries.Sort([](const auto& A,const auto& B) {return A.RecordedAt>B.RecordedAt;});
			if (Sort==2) Entries.Sort([](const auto& A,const auto& B) {return A.Variant!=B.Variant?static_cast<int32>(A.Variant)<static_cast<int32>(B.Variant):A.RecordedAt>B.RecordedAt;});
			int32 Visible=0;
			for(const auto& E:Entries)
			{
				if (!Search.IsEmpty()&&!(E.Name+E.Mode+E.SourceName+FString::Join(E.Players,TEXT(" "))).Contains(Search)) continue;
				++Visible;
				const FString Id=E.Id;
				Rows->AddSlot().AutoHeight().Padding(0,0,0,12)
				[SNew(SFlickAngularBorder).HomeSurface(true).BackgroundColor(Panel).AccentColor(Cyan.CopyWithNewOpacity(.4f)).UseAccentForOutline(true).CutSize(12).Padding(18)
				[SNew(SHorizontalBox)
				 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,16,0)
				   [Button(E.bFavourite?TEXT("★"):TEXT("☆"),[this,Id,b=E.bFavourite]() {Replay->SetFavourite(Id,!b);Refresh();})]
				 + SHorizontalBox::Slot().FillWidth(1)[SNew(SVerticalBox)
				   + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(E.IsClip()?TEXT("CLIP  //  ")+E.SourceName:TEXT("FULL MATCH"))).Font(UiFont(10,true)).ColorAndOpacity(Brand).AutoWrapText(true)]
				   + SVerticalBox::Slot().AutoHeight()[SNew(SEditableTextBox).Style(&NameStyle).Text(FText::FromString(E.Name)).Font(UiFont(18,true))
				     .OnTextCommitted_Lambda([this,Id](const FText& Name,ETextCommit::Type) {if(!Replay->RenameReplay(Id,Name.ToString())) Revision=-1;})]
				   + SVerticalBox::Slot().AutoHeight().Padding(0,7)[SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s  //  %s %dv%d  //  %02d:%02d  //  %s UTC"),
				     *E.Mode,E.Variant==EFlickMatchVariant::Bob?TEXT("BOB"):TEXT("KNOCKOUT"),E.TeamSize,E.TeamSize,int32(E.ViewDuration())/60,int32(E.ViewDuration())%60,*E.RecordedAt.Left(16).Replace(TEXT("T"),TEXT(" "))))).Font(UiFont(11,true)).ColorAndOpacity(Ice)]
				   + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(E.Result+TEXT("   //   ")+FString::Join(E.Players,TEXT("  ·  ")))).Font(UiFont(11)).ColorAndOpacity(Muted).AutoWrapText(true)]]
				 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16,0,8,0)
				   [Button(WatchPending==Id?TEXT("LEAVE ROOM & WATCH"):E.IsClip()?TEXT("WATCH CLIP"):TEXT("WATCH REPLAY"),[this,Id]()
				   { if(Replay->NeedsSessionExit()){if(WatchPending==Id) Replay->WatchAfterLeaving(Id);else {WatchPending=Id;Refresh();}}else Replay->Watch(Id); })]
				 + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				   [Button(DeletePending==Id?TEXT("CONFIRM DELETE"):TEXT("DELETE"),[this,Id]() {if(DeletePending==Id){Replay->Delete(Id);DeletePending.Reset();}else DeletePending=Id;Refresh();})]]];
			}
			if (!Visible) Rows->AddSlot().AutoHeight().Padding(25,55)[SNew(STextBlock).Text(FText::FromString(Search.IsEmpty()?TEXT("NO SAVED REPLAYS YET\nChoose SAVE REPLAY after a match to keep the complete game."):TEXT("NO MATCHING REPLAYS"))).Font(UiFont(19,true)).ColorAndOpacity(Muted).Justification(ETextJustify::Center)];
			if (!DeletePending.IsEmpty()) Rows->AddSlot().AutoHeight()[Button(TEXT("CANCEL DELETE"),[this]() {DeletePending.Reset();Refresh();})];
			if (!WatchPending.IsEmpty()) Rows->AddSlot().AutoHeight()[Button(TEXT("CANCEL WATCH"),[this]() {WatchPending.Reset();Refresh();})];
		}
		TWeakObjectPtr<UFlickReplaySubsystem> Replay;
		TSharedPtr<SVerticalBox> Rows;
		FButtonStyle Style;
		FEditableTextBoxStyle NameStyle;
		FString Search,Summary,DeletePending,WatchPending;
		int32 Revision=-1,Sort=0;
	};
}
TSharedRef<SWidget> MakeFlickSavedReplays(UGameInstance* Instance,FOnClicked Back)
{
	return SNew(SSavedReplays).Replay(Instance?Instance->GetSubsystem<UFlickReplaySubsystem>():nullptr).Back(Back);
}
