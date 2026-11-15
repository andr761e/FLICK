#pragma once
#include "Game/FlickProgressionSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Styling/CoreStyle.h"
namespace FlickCareerUI
{
 inline TSharedRef<SWidget> Progress(UFlickProgressionSubsystem* Subsystem,bool bAnimate=false,TAttribute<double> AnimationAge=TAttribute<double>())
 {
  TWeakObjectPtr<UFlickProgressionSubsystem> Weak=Subsystem;
  const auto Age=[Weak,AnimationAge]() { return AnimationAge.IsSet()?AnimationAge.Get():Weak.IsValid()?Weak->GetAwardAge():0.; };
  const auto XP=[Weak,bAnimate,Age]() -> int64
  {
   auto* S=Weak.Get(); if(!S) return 0;
   const int64 Total=S->GetXP();
   if(!bAnimate || S->GetLastAward().XP()<=0) return Total;
   if(Age()>6.) return Total;
   const double T=FMath::Clamp((Age()-.25)/2.5,0.,1.);
   return S->GetAwardStartXP()+int64(S->GetLastAward().XP()*(T*T*(3.-2.*T)));
  };
  return SNew(SVerticalBox)
   + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",9))
    .ColorAndOpacity(FLinearColor(.6f,.83f,.9f))
    .Text_Lambda([XP,Weak,bAnimate,Age]() { const auto L=FlickProgression::Level(XP());
     const bool Up=bAnimate && Weak.IsValid() && Age()<6 && L.Number>FlickProgression::Level(Weak->GetAwardStartXP()).Number;
     return FText::FromString(FString::Printf(TEXT("%sLEVEL %d     //     %s / %s XP"),Up?TEXT("LEVEL UP!  "):TEXT(""),L.Number,*FText::AsNumber(L.Progress).ToString(),*FText::AsNumber(L.Required).ToString())); })]
   + SVerticalBox::Slot().AutoHeight().Padding(0,5,0,0)[SNew(SBox).HeightOverride(3)
    [SNew(SProgressBar).Percent_Lambda([XP]() { return FlickProgression::Level(XP()).Fraction(); })
     .FillColorAndOpacity(FLinearColor(0.f,.75f,1.f)).BorderPadding(FVector2D::ZeroVector)]];
 }
}
