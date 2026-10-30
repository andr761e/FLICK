#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Core/FlickRankTransition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickRankTransitionTest, "FLICK.UI.RankResultTransition",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlickRankTransitionTest::RunTest(const FString&)
{
 for (bool bDown : {false, true})
 {
  FFlickRatingUpdate Update;
  Update.OldRating = bDown ? 1504 : 1488;
  Update.NewRating = bDown ? 1488 : 1504;
  Update.MatchesPlayed = 21;
  TestEqual(TEXT("Crossing a division selects the correct animation"), FlickRankTransition::Direction(Update), bDown ? -1 : 1);
  TestEqual(TEXT("Old rating is held initially"), FlickRankTransition::RatingAt(Update, 0), Update.OldRating);
  TestEqual(TEXT("Counting ends at the accepted rating"), FlickRankTransition::RatingAt(Update, 3), Update.NewRating);
  TestEqual(TEXT("Badge reveal and MMR finish together"), FlickRankTransition::RatingAt(Update, FlickRankTransition::RevealSeconds), Update.NewRating);
  TestEqual(TEXT("Final value never loops or drifts"), FlickRankTransition::RatingAt(Update, 600), Update.NewRating);
  int32 Last = Update.OldRating;
  for (int32 Frame = 0; Frame <= 120; ++Frame)
  {
   const int32 Current = FlickRankTransition::RatingAt(Update, Frame / 30.f);
   TestTrue(TEXT("Count is monotonic without overshoot"), bDown ? Current <= Last && Current >= Update.NewRating : Current >= Last && Current <= Update.NewRating);
   Last = Current;
  }
  TestEqual(TEXT("Old badge fully clears the reveal"), FlickRankTransition::OldOpacity(FlickRankTransition::RevealSeconds), 0.f);
  TestEqual(TEXT("New badge fully visible after reveal"), FlickRankTransition::NewOpacity(2.f), 1.f);
  TestEqual(TEXT("Badge settles without indefinite movement"), FlickRankTransition::BadgeScale(10.f, bDown ? -1 : 1), 1.f);
  TestEqual(TEXT("Badge finishes centered"), FlickRankTransition::BadgeOffset(10.f, bDown ? -1 : 1), 0.f);
  const float Entry = FlickRankTransition::BadgeOffset(FlickRankTransition::RevealSeconds, bDown ? -1 : 1);
  TestTrue(TEXT("Promotion rises; demotion falls"), bDown ? Entry < 0 : Entry > 0);
  for (int32 Frame = 0; Frame <= 240; ++Frame)
  {
   const float Scale = FlickRankTransition::BadgeScale(Frame / 60.f, bDown ? -1 : 1);
   TestTrue(TEXT("Badge remains finite and within its stage"), FMath::IsFinite(Scale) && Scale > 0 && Scale < 1.3f);
  }
 }
 FFlickRatingUpdate Placement;
 Placement.OldRating = 1488; Placement.NewRating = 1504; Placement.MatchesPlayed = 5;
 TestEqual(TEXT("Final placement starts unranked"), FlickRankRules::GetBadgeIndex(FlickRankTransition::Progress(Placement, false)), -1);
 TestTrue(TEXT("Final placement reveals the earned badge"), FlickRankTransition::Direction(Placement) > 0);
 Placement.MatchesPlayed = 4;
 TestEqual(TEXT("Incomplete placements never animate an earned rank"), FlickRankTransition::Direction(Placement), 0);
 return true;
}
#endif
