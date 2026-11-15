#include "Core/FlickProgression.h"
#include "Misc/ConfigCacheIni.h"
namespace FlickProgression
{
 FLevel Level(int64 XP)
 {
  XP=FMath::Max<int64>(0,XP); FLevel L;
  while (L.Number<21 && XP>=L.Required) { XP-=L.Required; ++L.Number; L.Required=1000+100*(L.Number-1); }
  if(L.Number>=21) { L.Number=21+int32(FMath::Min<int64>(XP/3000,MAX_int32-21)); XP%=3000; L.Required=3000; }
  L.Progress=int32(XP); return L;
 }
 static int32 Rule(const TCHAR* Key,int32 Default) { int32 V=Default; if(GConfig) GConfig->GetInt(TEXT("FLICK.Progression"),Key,V,GGameIni); return FMath::Clamp(V,0,10000); }
 void RewardMatch(FFlickCareerEvent& E)
 {
  E.CompletionXP=E.TimeXP=E.WinXP=E.StyleXP=E.BonusXP=0;
  if(!E.bEligible || E.Shots<=0 || E.Seconds<10 || E.Kind!=0) return;
  E.CompletionXP=Rule(TEXT("CompletionXP"),250);
  E.TimeXP=FMath::Min(Rule(TEXT("TimeCapXP"),250),int32(int64(FMath::Max(0,E.Seconds))*Rule(TEXT("MinuteXP"),25)/60));
  E.WinXP=E.bWon ? Rule(TEXT("WinXP"),100) : 0;
  int64 Style=0;
  for(int32 I=0;I<E.Accolades.Num();++I) Style+=int64(FMath::Max(0,E.Accolades[I]))*(I==4 || I==10 ? 25 : 10);
  if(E.Format==3) Style+=int64(FMath::Max(0,E.Pockets))*10+int64(FMath::Max(0,E.MultiPockets))*15;
  E.StyleXP=int32(FMath::Min<int64>(Rule(TEXT("StyleCapXP"),100),Style));
  if(E.Playlist==2) { E.CompletionXP/=2; E.TimeXP/=2; E.WinXP/=2; E.StyleXP/=2; }
 }
}
