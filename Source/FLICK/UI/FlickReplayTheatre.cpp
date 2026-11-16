#include "UI/FlickReplayTheatre.h"
#include "UI/FlickGameLayerPrivate.h"
#include "Player/FlickReplayController.h"
#include "Game/FlickReplaySubsystem.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	FString Clock(float Time)
	{
		const int32 T=FMath::Max(0,FMath::FloorToInt(Time));
		return FString::Printf(TEXT("%02d:%02d"),T/60,T%60);
	}
	class SReplayMarkers : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SReplayMarkers) {} SLATE_ARGUMENT(TWeakObjectPtr<UFlickReplaySubsystem>,Replay) SLATE_END_ARGS()
		void Construct(const FArguments& A) { Replay=A._Replay; SetCanTick(false); ForceVolatile(true); }
		virtual FVector2D ComputeDesiredSize(float) const override { return {300,30}; }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,int32 L,const FWidgetStyle&,bool) const override
		{
			const auto* R=Replay.Get(); if(!R||R->GetViewDuration()<=0) return L;
			if(R->IsEditingClip())
			{
				const float Start=G.GetLocalSize().X*R->GetClipIn()/R->GetViewDuration();
				const float Width=G.GetLocalSize().X*(R->GetClipOut()-R->GetClipIn())/R->GetViewDuration();
				FSlateDrawElement::MakeBox(E,L,G.ToPaintGeometry(FVector2D(Width,18),FSlateLayoutTransform(FVector2D(Start,0))),FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,Brand.CopyWithNewOpacity(.25f));
			}
			for(const auto& M:R->GetPlaying().Markers)
			{
				if(M.Time<R->GetViewStart()||M.Time>R->GetViewEnd()) continue;
				const float X=G.GetLocalSize().X*FMath::Clamp((M.Time-R->GetViewStart())/R->GetViewDuration(),0.f,1.f);
				const FLinearColor Color=M.Label.StartsWith(TEXT("Round"))?Cyan:M.Label==TEXT("Knockout")||M.Label==TEXT("Pocket")?Brand:Muted.CopyWithNewOpacity(.5f);
				FSlateDrawElement::MakeLines(E,L,G.ToPaintGeometry(),TArray<FVector2D>{{X,0},{X,9}},ESlateDrawEffect::None,Color,true,2);
			}
			return L;
		}
	private: TWeakObjectPtr<UFlickReplaySubsystem> Replay;
	};
}

TSharedRef<SWidget> MakeFlickReplayTheatre(AFlickReplayController* Controller)
{
	const TWeakObjectPtr<AFlickReplayController> C=Controller;
	const TWeakObjectPtr<UFlickReplaySubsystem> R=Controller->Replay();
	static const FButtonStyle ButtonStyle=FButtonStyle().SetNormal(FSlateNoResource()).SetHovered(FSlateNoResource()).SetPressed(FSlateNoResource());
	static const FSliderStyle TimelineStyle=[]()
	{
		FSlateColorBrush Thumb(Cyan),Hover(Brand);Thumb.ImageSize=FVector2D(9,18);Hover.ImageSize=Thumb.ImageSize;
		return FSliderStyle().SetNormalBarImage(FSlateColorBrush(FLinearColor(.025f,.11f,.15f,1)))
			.SetHoveredBarImage(FSlateColorBrush(FLinearColor(.04f,.22f,.28f,1)))
			.SetNormalThumbImage(Thumb).SetHoveredThumbImage(Hover).SetBarThickness(3);
	}();
	const auto Button=[](const FString& Label,const FString& Tip,TFunction<void()> Action,bool bPrimary=false)->TSharedRef<SWidget>
	{
		return SNew(SButton).ButtonStyle(&ButtonStyle).ContentPadding(0).ToolTipText(FText::FromString(Tip))
			.OnClicked_Lambda([Action]() {Action();return FReply::Handled();})
			[SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(4).Padding(FMargin(9,6))
			 .AccentColor(bPrimary?Brand:Cyan.CopyWithNewOpacity(.35f)).BackgroundColor(bPrimary?Brand:Panel)
			 [SNew(STextBlock).Text(FText::FromString(Label)).Font(UiFont(10,true)).ColorAndOpacity(bPrimary?Ink:Paper)]];
	};
	const auto MenuItem=[Button](const FString& Label,TFunction<void()> Action)
	{
		return Button(Label,TEXT(""),[Action]() {Action();FSlateApplication::Get().DismissAllMenus();});
	};
	const auto CameraLabel=[C]()
	{
		if(!C.IsValid()) return FText::FromString(TEXT("CAMERA"));
		if(C->GetCameraMode()==EFlickReplayCamera::Player) return FText::FromString(C->TargetLabel());
		const TCHAR* Names[]={TEXT("ARENA"),TEXT("PLAYER"),TEXT("FOLLOW PUCK"),TEXT("FREE CAMERA"),TEXT("CINEMATIC")};
		return FText::FromString(Names[int32(C->GetCameraMode())]);
	};
	TSharedRef<SHorizontalBox> Controls=SNew(SHorizontalBox);
	Controls->AddSlot().AutoWidth()[SNew(SButton).ButtonStyle(&ButtonStyle).ContentPadding(0)
		.ToolTipText(FText::FromString(TEXT("Play / pause [Space]")))
		.OnClicked_Lambda([C]() {if(C.IsValid()) C->TogglePause();return FReply::Handled();})
		[SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).CutSize(4).Padding(FMargin(16,6)).BackgroundColor(Brand).AccentColor(Brand)
		 [SNew(SBox).WidthOverride(40).HAlign(HAlign_Center)
		  [SNew(STextBlock).Text_Lambda([R]() {return FText::FromString(R.IsValid()&&R->IsPaused()?TEXT("PLAY"):TEXT("PAUSE"));}).Font(UiFont(10,true)).ColorAndOpacity(Ink)]]]];
	Controls->AddSlot().AutoWidth().Padding(4,0)[Button(TEXT("-5s"),TEXT("Back five seconds [Left]"),[R]() {if(R.IsValid()) R->Seek(R->GetTime()-5);})];
	Controls->AddSlot().AutoWidth()[Button(TEXT("+5s"),TEXT("Forward five seconds [Right]"),[R]() {if(R.IsValid()) R->Seek(R->GetTime()+5);})];
	Controls->AddSlot().FillWidth(1).Padding(8,0)[SNew(SComboButton).ButtonStyle(&ButtonStyle).ContentPadding(FMargin(6,3))
		.ToolTipText(FText::FromString(TEXT("Choose a player or camera [C cycles cameras]")))
		.ButtonContent()[SNew(STextBlock).Text_Lambda(CameraLabel).Font(UiFont(10,true)).ColorAndOpacity(Paper).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]
		.OnGetMenuContent_Lambda([C,MenuItem]()->TSharedRef<SWidget>
		{
			TSharedRef<SVerticalBox> Menu=SNew(SVerticalBox);
			Menu->AddSlot().AutoHeight()[MenuItem(TEXT("ARENA OVERVIEW"),[C]() {if(C.IsValid()) C->SelectCamera(EFlickReplayCamera::Overview);})];
			if(C.IsValid()) for(int32 Seat=0;Seat<C->PlayerCount();++Seat)
				Menu->AddSlot().AutoHeight().Padding(0,2)[MenuItem(C->PlayerLabel(Seat),[C,Seat]() {if(C.IsValid()) C->SelectPlayer(Seat);})];
			for(const auto Mode:{EFlickReplayCamera::Puck,EFlickReplayCamera::Free,EFlickReplayCamera::Cinematic})
			{
				const TCHAR* Name=Mode==EFlickReplayCamera::Puck?TEXT("FOLLOW PUCK"):Mode==EFlickReplayCamera::Free?TEXT("FREE CAMERA"):TEXT("CINEMATIC");
				Menu->AddSlot().AutoHeight().Padding(0,2)[MenuItem(Name,[C,Mode]() {if(C.IsValid()) C->SelectCamera(Mode);})];
			}
			return SNew(SBox).WidthOverride(290)[Menu];
		})];
	Controls->AddSlot().AutoWidth()[Button(TEXT("<"),TEXT("Previous target [Left bracket]"),[C]() {if(C.IsValid()) C->CycleTarget(-1);})];
	Controls->AddSlot().AutoWidth().Padding(4,0)[Button(TEXT(">"),TEXT("Next target [Right bracket]"),[C]() {if(C.IsValid()) C->CycleTarget(1);})];
	Controls->AddSlot().AutoWidth()[SNew(SComboButton).ButtonStyle(&ButtonStyle).ContentPadding(FMargin(7,3))
		.ToolTipText(FText::FromString(TEXT("Playback speed")))
		.ButtonContent()[SNew(STextBlock).Text_Lambda([R]() {return FText::FromString(FString::Printf(TEXT("%gx"),R.IsValid()?R->GetSpeed():1));}).Font(UiFont(10,true)).ColorAndOpacity(Paper)]
		.OnGetMenuContent_Lambda([R,MenuItem]()->TSharedRef<SWidget>
		{
			TSharedRef<SVerticalBox> Menu=SNew(SVerticalBox);
			for(float Speed:{.25f,.5f,1.f,2.f}) Menu->AddSlot().AutoHeight().Padding(0,2)
				[MenuItem(FString::Printf(TEXT("%gx"),Speed),[R,Speed]() {if(R.IsValid()) R->SetSpeed(Speed);})];
			return Menu;
		})];
	Controls->AddSlot().AutoWidth().Padding(6,0)[SNew(SButton).ButtonStyle(&ButtonStyle).ContentPadding(FMargin(9,6))
		.ToolTipText(FText::FromString(TEXT("Capture mouse to fly: WASD move, Q/E down/up, Shift boost. Escape releases.")))
		.OnClicked_Lambda([C]() {if(C.IsValid()) C->ToggleFly();return FReply::Handled();})
		[SNew(STextBlock).Text_Lambda([C]() {return FText::FromString(C.IsValid()&&C->IsFlying()?TEXT("RELEASE [ESC]"):TEXT("FLY"));}).Font(UiFont(10,true)).ColorAndOpacity(Cyan)]];
	Controls->AddSlot().AutoWidth()[SNew(SComboButton).ButtonStyle(&ButtonStyle).ContentPadding(FMargin(7,3))
		.ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("MORE"))).Font(UiFont(10,true)).ColorAndOpacity(Paper)]
		.OnGetMenuContent_Lambda([C,R,MenuItem]()->TSharedRef<SWidget>
		{
			return SNew(SBox).WidthOverride(265)[SNew(SVerticalBox)
			 +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,5)[MenuItem(TEXT("CREATE CLIP"),[C,R]() {if(C.IsValid()) C->SelectCamera(C->GetCameraMode());if(R.IsValid()) R->BeginClip();})]
			 +SVerticalBox::Slot().AutoHeight()[MenuItem(TEXT("RESET CAMERA"),[C]() {if(C.IsValid()) C->ResetView();})]
			 +SVerticalBox::Slot().AutoHeight().Padding(0,2)[MenuItem(TEXT("PREVIOUS SHOT"),[R]() {if(R.IsValid()) R->JumpMarker(-1);})]
			 +SVerticalBox::Slot().AutoHeight()[MenuItem(TEXT("NEXT SHOT"),[R]() {if(R.IsValid()) R->JumpMarker(1);})]
			 +SVerticalBox::Slot().AutoHeight().Padding(0,2)[MenuItem(TEXT("PREVIOUS ROUND"),[R]() {if(R.IsValid()) R->JumpMarker(-1,true);})]
			 +SVerticalBox::Slot().AutoHeight()[MenuItem(TEXT("NEXT ROUND"),[R]() {if(R.IsValid()) R->JumpMarker(1,true);})]
			 +SVerticalBox::Slot().AutoHeight().Padding(8,10)[SNew(STextBlock).Text(FText::FromString(TEXT("Space: play / pause\nLeft / Right: seek five seconds\nComma / Period: frame step\nC: camera   [ / ]: target\nRMB: look / orbit   Wheel: zoom\nFree camera: WASD + Q/E\nShift: boost   Esc: release / exit\nF: hide or show controls"))).Font(UiFont(10)).ColorAndOpacity(Paper)]];
		})];
	Controls->AddSlot().AutoWidth().Padding(6,0,0,0)[Button(TEXT("HIDE [F]"),TEXT("Hide or restore all controls [F]"),[C]() {if(C.IsValid()) C->ToggleControls();})];
	return SNew(SOverlay)
	 +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(14,12)
	 [SNew(SBox).MaxDesiredWidth(520).Visibility_Lambda([C]() {return C.IsValid()&&C->ShowsControls()?EVisibility::SelfHitTestInvisible:EVisibility::Collapsed;})
	  [SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(Ink.CopyWithNewOpacity(.78f)).Padding(FMargin(10,5))
	   [SNew(STextBlock).Text_Lambda([R]() {return FText::FromString(R.IsValid()?R->GetPlaying().Name:TEXT("Replay"));})
	    .Font(UiFont(10,true)).ColorAndOpacity(Muted).OverflowPolicy(ETextOverflowPolicy::Ellipsis)]]]
	 +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(14,12)
	 [SNew(SBox).Visibility_Lambda([C]() {return C.IsValid()&&C->ShowsControls()?EVisibility::SelfHitTestInvisible:EVisibility::Collapsed;})
	  [Button(TEXT("EXIT [ESC]"),TEXT("Return to saved replays"),[R]() {if(R.IsValid()) R->ExitTheatre();})]]
	 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(12,10)
	 [SNew(SBox).MaxDesiredWidth(1060).Visibility_Lambda([C]() {return C.IsValid()&&C->ShowsControls()?EVisibility::SelfHitTestInvisible:EVisibility::Collapsed;})
	  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
	   [SNew(SBox).WidthOverride(1060)
	    [SNew(SFlickAngularBorder).HomeSurface(true).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.5f))
	     .BackgroundColor(Ink.CopyWithNewOpacity(.90f)).CutSize(7).Padding(FMargin(10,7))
	     [SNew(SVerticalBox)
	      +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
	       +SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[SNew(SBox).HeightOverride(18)[SNew(SOverlay)
	        +SOverlay::Slot()[SNew(SSlider).Style(&TimelineStyle)
	         .Value_Lambda([R]() {return R.IsValid()&&R->GetViewDuration()>0?R->GetDisplayTime()/R->GetViewDuration():0.f;})
	         .OnMouseCaptureBegin_Lambda([R]() {if(R.IsValid()) R->BeginScrub();})
	         .OnMouseCaptureEnd_Lambda([R]() {if(R.IsValid()) R->EndScrub();})
	         .OnValueChanged_Lambda([R](float V) {if(R.IsValid()) R->PreviewScrub(R->GetViewStart()+V*R->GetViewDuration());})
	         .ToolTipText(FText::FromString(TEXT("Drag the timeline, then release to jump. Cyan: rounds. Orange: knockouts / pockets.")))]
	        +SOverlay::Slot()[SNew(SReplayMarkers).Replay(R).Visibility(EVisibility::HitTestInvisible)]]]
	       +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10,0,0,0)
	       [SNew(STextBlock).Text_Lambda([R]() {return FText::FromString(R.IsValid()?FString::Printf(TEXT("%s / %s%s"),*Clock(R->GetDisplayTime()),*Clock(R->GetViewDuration()),R->IsSeeking()?TEXT(" ..."):TEXT("")):TEXT("LOADING"));}).Font(UiFont(10,true)).ColorAndOpacity(Paper)]]
	      +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[Controls]
	     ]]]]]
	 +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(12,0,12,90)
	 [SNew(SBox).MaxDesiredWidth(820).Visibility_Lambda([R,C]() {return R.IsValid()&&R->IsEditingClip()&&C.IsValid()&&C->ShowsControls()?EVisibility::SelfHitTestInvisible:EVisibility::Collapsed;})
	  [SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)[MakeFlickReplayClipEditor(Controller->Replay())]]];
}

TSharedRef<SWidget> MakeFlickReplayLoading(UFlickReplaySubsystem* Replay)
{
	const TWeakObjectPtr<UFlickReplaySubsystem> R=Replay;
	return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(Ink)
	.HAlign(HAlign_Center).VAlign(VAlign_Center)
	[SNew(SBox).WidthOverride(700)[SNew(SVerticalBox)
	 + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SThrobber)]
	 + SVerticalBox::Slot().AutoHeight().Padding(0,25)[SNew(STextBlock).Text(FText::FromString(TEXT("PREPARING YOUR REPLAY"))).Font(DisplayFont(32)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)]
	 + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text_Lambda([R]() {return FText::FromString(R.IsValid()?R->GetStatus():TEXT("Loading..."));}).Font(UiFont(16)).ColorAndOpacity(Cyan).Justification(ETextJustify::Center)]
	 + SVerticalBox::Slot().AutoHeight().Padding(0,20)[SNew(STextBlock).Text(FText::FromString(TEXT("Playback starts paused. Choose any player, follow a puck, or explore with the free camera."))).AutoWrapText(true).Font(UiFont(13)).ColorAndOpacity(Muted).Justification(ETextJustify::Center)]
	 + SVerticalBox::Slot().AutoHeight().Padding(0,20).HAlign(HAlign_Center)[SNew(SButton).ContentPadding(FMargin(24,12))
	   .OnClicked_Lambda([R]() {if(R.IsValid()) R->ExitTheatre();return FReply::Handled();})
	   [SNew(STextBlock).Text(FText::FromString(TEXT("CANCEL / RETURN TO LIBRARY"))).Font(UiFont(12,true))]]

	]];
}
