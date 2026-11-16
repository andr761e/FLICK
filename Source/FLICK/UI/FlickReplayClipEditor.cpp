#include "UI/FlickReplayTheatre.h"
#include "UI/FlickGameLayerPrivate.h"
#include "Game/FlickReplaySubsystem.h"
#include "Misc/DefaultValueHelper.h"

namespace
{
	FString ClipClock(float Time)
	{
		// Integer milliseconds avoid displaying 00:60.000 near a minute boundary
		// or rounding an endpoint past the actual recording duration.
		const int32 Millis=FMath::Max(0,FMath::FloorToInt(Time*1000.f));
		return FString::Printf(TEXT("%02d:%02d.%03d"),Millis/60000,(Millis/1000)%60,Millis%1000);
	}
	bool ParseClipClock(const FString& Text,float& Time)
	{
		const FString Clean=Text.TrimStartAndEnd();FString Minutes,Seconds;
		if(Clean.Split(TEXT(":"),&Minutes,&Seconds))
		{
			int32 M=0;float S=0;
			if(!FDefaultValueHelper::IsStringValidInteger(Minutes)||!FDefaultValueHelper::ParseInt(Minutes,M)||M<0||M>240
				||!FDefaultValueHelper::IsStringValidFloat(Seconds)||!FDefaultValueHelper::ParseFloat(Seconds,S)||S<0||S>=60) return false;
			Time=M*60.f+S;
		}
		else if(!FDefaultValueHelper::IsStringValidFloat(Clean)||!FDefaultValueHelper::ParseFloat(Clean,Time)) return false;
		return FMath::IsFinite(Time)&&Time>=0;
	}

	// A temporary panel above the existing dock, so cameras and transport remain
	// available while previewing. Boundary dragging coalesces native seeks.
	class SReplayClipEditor : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SReplayClipEditor) {} SLATE_ARGUMENT(TWeakObjectPtr<UFlickReplaySubsystem>,Replay) SLATE_END_ARGS()
		void Construct(const FArguments& A)
		{
			Replay=A._Replay;
			Style=FButtonStyle().SetNormal(FSlateNoResource()).SetHovered(FSlateNoResource()).SetPressed(FSlateNoResource());
			ChildSlot[SNew(SBox).WidthOverride(820)
			 [SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(10).AccentColor(Brand)
			  .BackgroundColor(Ink.CopyWithNewOpacity(.97f)).Padding(16)
			  [SNew(SVerticalBox)
			   +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
			    +SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Text_Lambda([this]() {return FText::FromString(bSaved?TEXT("CLIP SAVED"):bReview?TEXT("READY TO SAVE YOUR CLIP?"):TEXT("CREATE REPLAY CLIP"));}).Font(UiFont(16,true)).ColorAndOpacity(Brand)]
			    +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("CLOSE [ESC]"),[this]() {if(Replay.IsValid()) Replay->CloseClip();})]]
			   +SVerticalBox::Slot().AutoHeight().Padding(0,7)[SNew(STextBlock).Text_Lambda([this]()
			   {
			   	if(bSaved) return FText::FromString(TEXT("Your moment is now in Profile / Saved Replays. Every camera angle is included."));
			   	if(bReview) return FText::FromString(TEXT("Check the name and selected range below, then confirm to save."));
			   	return FText::FromString(TEXT("Drag START or END to see that moment in the arena. Release to settle on the exact frame."));
			   }).Font(UiFont(11)).ColorAndOpacity(Paper).AutoWrapText(true)]
			   +SVerticalBox::Slot().AutoHeight().Padding(0,3)[SNew(SBox).IsEnabled_Lambda([this]() {return !bSaved&&!bReview;})[RangeRow(true)]]
			   +SVerticalBox::Slot().AutoHeight().Padding(0,3)[SNew(SBox).IsEnabled_Lambda([this]() {return !bSaved&&!bReview;})[RangeRow(false)]]
			   +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[SNew(SHorizontalBox)
			    +SHorizontalBox::Slot().FillWidth(1)[SAssignNew(Name,SEditableTextBox).HintText(FText::FromString(TEXT("Name your clip"))).Font(UiFont(12)).SelectAllTextWhenFocused(true).IsReadOnly_Lambda([this]() {return bSaved||bReview;})]
			    +SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(SButton).ButtonStyle(&Style).ContentPadding(FMargin(10,7))
			     .OnClicked_Lambda([this]() {if(Replay.IsValid()) {if(bSaved) Replay->BeginClip();else if(bReview) bReview=false;else Replay->PreviewClip();}return FReply::Handled();})
			     [SNew(STextBlock).Text_Lambda([this]() {return FText::FromString(bSaved?TEXT("ANOTHER CLIP"):bReview?TEXT("BACK TO EDIT"):Replay.IsValid()&&Replay->IsPreviewingClip()?TEXT("STOP PREVIEW"):TEXT("LOOP PREVIEW"));}).Font(UiFont(11,true)).ColorAndOpacity(Cyan)]]
			    +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).ButtonStyle(&Style).ContentPadding(0)
			     .OnClicked_Lambda([this]() {SaveAction();return FReply::Handled();})
			     [SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(5).Padding(FMargin(14,10)).AccentColor(Brand).BackgroundColor(Brand)
			      [SNew(STextBlock).Text_Lambda([this]() {return FText::FromString(bSaved?TEXT("DONE"):bReview?TEXT("CONFIRM & SAVE"):TEXT("REVIEW & SAVE"));}).Font(UiFont(12,true)).ColorAndOpacity(Ink)]]]]
			   +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,3)[SNew(STextBlock).Text_Lambda([this]()
			   {
			   	return FText::FromString(Replay.IsValid()?FString::Printf(TEXT("%s  TO  %s    //    %.2f SECONDS"),*ClipClock(Replay->GetClipIn()),*ClipClock(Replay->GetClipOut()),Replay->GetClipOut()-Replay->GetClipIn()):FString());
			   }).Font(UiFont(13,true)).ColorAndOpacity(bSaved?Cyan:Paper)]
			   +SVerticalBox::Slot().AutoHeight().Padding(0,5,0,0)[SNew(STextBlock).Text_Lambda([this]()
			   {
			   	return FText::FromString(!Error.IsEmpty()?Error:bSaved?TEXT("SAVED SUCCESSFULLY  //  Original replay preserved."):bReview?TEXT("Save a separate replay clip with all players and cameras. Your original replay stays unchanged."):Replay.IsValid()&&Replay->IsSeeking()?TEXT("Updating footage preview..."):TEXT("Tip: timestamps accept m:ss or seconds. LOOP PREVIEW plays your selected range."));
			   }).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true)]
			  ]]];
		}
		virtual void Tick(const FGeometry& G,double T,float D) override
		{
			SCompoundWidget::Tick(G,T,D);
			if(Replay.IsValid()&&Replay->IsEditingClip()&&EditorRevision!=Replay->GetClipEditorRevision())
			{
				EditorRevision=Replay->GetClipEditorRevision();
				Name->SetText(FText::FromString(Replay->GetPlaying().Name.Left(95)+TEXT(" clip")));Error.Reset();bInvalidRange=bReview=bSaved=false;
			}
		}
	private:
		void SaveAction()
		{
			if(!Replay.IsValid()) return;
			if(bSaved) {Replay->CloseClip();return;}
			if(bInvalidRange) return;
			const FString Clean=Name->GetText().ToString().TrimStartAndEnd();
			if(Clean.IsEmpty()||Clean.Len()>100) {Error=TEXT("Enter a clip name (1-100 characters).");return;}
			if(!bReview)
			{
				if(Replay->IsPreviewingClip()) Replay->PreviewClip();
				Replay->SetPaused(true);Name->SetText(FText::FromString(Clean));bReview=true;Error.Reset();return;
			}
			const FString Id=Replay->SaveClip(Replay->GetPlaying().Id,Clean,Replay->GetClipIn(),Replay->GetClipOut());
			if(Id.IsEmpty()) Error=Replay->GetStatus();else {bSaved=true;bReview=false;Error.Reset();}
		}
		TSharedRef<SWidget> Button(const FString& Label,TFunction<void()> Action,bool bPrimary=false)
		{
			return SNew(SButton).ButtonStyle(&Style).ContentPadding(0)
			 .OnClicked_Lambda([Action]() {Action();return FReply::Handled();})
			 [SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(4).Padding(FMargin(10,7))
			  .AccentColor(bPrimary?Brand:Cyan.CopyWithNewOpacity(.4f)).BackgroundColor(bPrimary?Brand:Panel)
			  [SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(11,true)).ColorAndOpacity(bPrimary?Ink:Paper)]];
		}
		float Value(bool bStart) const {return !Replay.IsValid()?0:bStart?Replay->GetClipIn():Replay->GetClipOut();}
		void SetValue(bool bStart,float Value)
		{
			if(!Replay.IsValid()) return;
			const float Previous=bStart?Replay->GetClipIn():Replay->GetClipOut();
			// Slider events continue beyond the other handle, but their clamped
			// time no longer changes. Do not restart the preview at that limit.
			if(FMath::IsNearlyEqual(Value,Previous,.0001f)) {bInvalidRange=false;Error.Reset();return;}
			bInvalidRange=!Replay->SetClipRange(bStart?Value:Replay->GetClipIn(),bStart?Replay->GetClipOut():Value);
			if(bInvalidRange)
				Error=TEXT("Start must precede end by at least 0.1 seconds, within this replay.");
			else {Error.Reset();Replay->PreviewClipBoundary(bStart);}
		}
		TSharedRef<SWidget> RangeRow(bool bStart)
		{
			return SNew(SHorizontalBox)
			 +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(48)
			  [SNew(STextBlock).Text(FText::FromString(bStart?TEXT("START"):TEXT("END"))).Font(UiFont(11,true)).ColorAndOpacity(Paper)]]
			 +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(6,0)[SNew(SSlider)
			  .OnMouseCaptureEnd_Lambda([this,bStart]() {if(Replay.IsValid()) Replay->PreviewClipBoundary(bStart,true);})
			  .OnControllerCaptureEnd_Lambda([this,bStart]() {if(Replay.IsValid()) Replay->PreviewClipBoundary(bStart,true);})
			  .Value_Lambda([this,bStart]() {return Replay.IsValid()&&Replay->GetViewDuration()>0?Value(bStart)/Replay->GetViewDuration():0.f;})
			  .OnValueChanged_Lambda([this,bStart](float V)
			  {
			  	if(!Replay.IsValid()) return;
			  	const float Time=V*Replay->GetViewDuration();
			  	SetValue(bStart,bStart?FMath::Min(Time,Replay->GetClipOut()-.101f):FMath::Max(Time,Replay->GetClipIn()+.101f));
			  }).ToolTipText(FText::FromString(bStart?TEXT("Drag the start of your selection"):TEXT("Drag the end of your selection")))]
			 +SHorizontalBox::Slot().AutoWidth().Padding(6,0)[SNew(SBox).WidthOverride(106)
			  [SNew(SEditableTextBox).Text_Lambda([this,bStart]() {return FText::FromString(ClipClock(Value(bStart)));}).Font(UiFont(12))
			   .SelectAllTextWhenFocused(true).OnTextCommitted_Lambda([this,bStart](const FText& T,ETextCommit::Type)
			   {
			   	if(T.ToString()==ClipClock(Value(bStart))) {bInvalidRange=false;Error.Reset();return;} // Preserve the exact endpoint when unchanged.
			   	float Time=0;if(ParseClipClock(T.ToString(),Time)) {SetValue(bStart,Time);if(!bInvalidRange&&Replay.IsValid()) Replay->PreviewClipBoundary(bStart,true);}
			   	else {bInvalidRange=true;Error=TEXT("Enter a time as m:ss (e.g. 0:12.5) or seconds.");}
			   })]]
			 +SHorizontalBox::Slot().AutoWidth()[Button(TEXT("USE CURRENT"),[this,bStart]() {if(Replay.IsValid()) SetValue(bStart,Replay->GetDisplayTime());})];
		}
		TWeakObjectPtr<UFlickReplaySubsystem> Replay;
		TSharedPtr<SEditableTextBox> Name;
		FButtonStyle Style;
		FString Error;
		int32 EditorRevision=-1;
		bool bInvalidRange=false;
		bool bReview=false,bSaved=false;
	};
}

TSharedRef<SWidget> MakeFlickReplayClipEditor(UFlickReplaySubsystem* Replay)
{
	return SNew(SReplayClipEditor).Replay(Replay);
}
