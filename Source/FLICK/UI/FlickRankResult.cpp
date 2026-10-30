#include "UI/FlickRankResult.h"
#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickRankTransition.h"

namespace
{
 // Bounded, deterministic Slate geometry. No particles, world lights or new assets.
 class SFlickRankRevealFX final : public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SFlickRankRevealFX) {}
   SLATE_ATTRIBUTE(float, Seconds)
   SLATE_ATTRIBUTE(int32, Direction)
   SLATE_ATTRIBUTE(FLinearColor, Accent)
  SLATE_END_ARGS()
  void Construct(const FArguments& Args)
  {
   Seconds = Args._Seconds; Direction = Args._Direction; Accent = Args._Accent;
   SetCanTick(false); ForceVolatile(true);
  }
  virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(220, 150); }
  virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
   FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const override
  {
   const float Time = Seconds.Get();
   const int32 Dir = Direction.Get();
   const FVector2D Center = Geometry.GetLocalSize() * .5;
   const FLinearColor Color = Accent.Get();
   const auto Tint = [&](FLinearColor C, float Alpha) { return C.CopyWithNewOpacity(Alpha) * Style.GetColorAndOpacityTint(); };
   const auto Stroke = [&](const TArray<FVector2D>& Points, FLinearColor C, float Alpha, float Width)
   {
    if (Alpha > .001f) FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points,
     ESlateDrawEffect::None, Tint(C, Alpha), true, Width);
   };
   const auto Arc = [&](float Radius, float Start, float Span, FLinearColor C, float Alpha, float Width)
   {
    if (Alpha <= .001f) return;
    TArray<FVector2D> Points; Points.Reserve(49);
    for (int32 Index = 0; Index <= 48; ++Index)
    {
     const float Angle = Start + Span * Index / 48.f;
     Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
    Stroke(Points, C, Alpha, Width);
   };
   // The finished card retains a restrained instrument-ring behind its badge.
   Arc(64, 0, UE_TWO_PI, Color, .08f, 1);
   for (int32 Index = 0; Index < 16; ++Index)
   {
    const float Angle = UE_TWO_PI * Index / 16.f;
    const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
    Stroke({Center + Ray * 68, Center + Ray * (Index % 4 == 0 ? 73 : 70)}, Color, .24f, 1);
   }
   if (Dir == 0 || Time >= FlickRankTransition::RevealSeconds + 1.65f) return Layer;
   const float Charge = FlickRankTransition::Phase(Time, .4f, FlickRankTransition::RevealSeconds - .4f);
   const float After = Time - FlickRankTransition::RevealSeconds;
   if (After < 0)
   {
    // Three arcs converge while the rating counts toward its accepted value.
    for (int32 Index = 0; Index < 3; ++Index)
    {
     const float Start = Index * UE_TWO_PI / 3.f + Time * Dir * 1.3f;
     Arc(72 - Charge * 9, Start, .3f + Charge * 1.15f, Color, .12f + Charge * .65f, 1.8f);
    }
    Arc(62, 0, UE_TWO_PI, Color, Charge * .10f, 12);
   }
   else
   {
    const float T = FlickRankTransition::Phase(After, 0, 1.15f);
    const float Fade = FMath::Square(1 - T);
    // Soft halo plus two crisp shock rings; a promotion expands more forcefully.
    Arc(52 + T * 21, 0, UE_TWO_PI, Color, Fade * .13f, 15);
    Arc(53 + T * 20, 0, UE_TWO_PI, Color, Fade * .9f, 1.8f);
    Arc(48 + T * 23, 0, UE_TWO_PI, FLinearColor::White, Fade * .45f, 1);
    // A directional sweep briefly lights the stage without washing out the emblem.
    const float SweepY = Center.Y + Dir * (42 - T * 84);
    Stroke({FVector2D(Center.X - 70, SweepY), FVector2D(Center.X + 70, SweepY)}, Color, Fade * .12f, 7);
    for (int32 Index = 0; Index < 22; ++Index)
    {
     const float Age = After - (Index % 5) * .025f;
     if (Age < 0 || Age > 1.3f) continue;
     const float P = Age / 1.3f;
     const float Angle = Index * 2.399963f;
     const FVector2D Ray(FMath::Cos(Angle), FMath::Sin(Angle));
     const FVector2D Position = Center + Ray * (55 + P * 21) + FVector2D(0, -Dir * P * 13);
     const float Alpha = FMath::Square(1 - P) * .85f;
     if (Dir > 0)
     {
      Stroke({Position - Ray * (2 + P * 5), Position + Ray * 2}, Color, Alpha * .18f, 5);
      Stroke({Position - Ray * (2 + P * 5), Position + Ray * 2}, Index % 4 == 0 ? FLinearColor::White : Color, Alpha, 1.2f);
     }
     else
     {
      // Demotion uses falling angular shards, rather than celebration sparks.
      Stroke({Position + FVector2D(-3, -2), Position, Position + FVector2D(2, 4)}, Color, Alpha * .7f, 1.2f);
     }
    }
    for (int32 Side : {-1, 1})
    for (int32 Index = 0; Index < 3; ++Index)
    {
     const float P = FlickRankTransition::Phase(After, Index * .1f, .8f);
     const float X = Center.X + Side * 83;
     const float Y = Center.Y - Dir * (P * 25 - 13 + Index * 10);
     Stroke({FVector2D(X - 5, Y + Dir * 4), FVector2D(X, Y - Dir * 2), FVector2D(X + 5, Y + Dir * 4)},
      Color, FMath::Sin(P * PI) * .75f, 1.8f);
    }
   }
   return Layer;
  }
 private:
  TAttribute<float> Seconds;
  TAttribute<int32> Direction;
  TAttribute<FLinearColor> Accent;
 };
}

void SFlickRankResult::Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime)
{
 SCompoundWidget::Tick(Geometry, CurrentTime, DeltaTime);
 const FFlickRatingUpdate Next = Update.Get();
 if (!Next.bAccepted) { Active = {}; Elapsed = 0; return; }
 if (Active.MatchId != Next.MatchId || Active.NewRating != Next.NewRating || !Active.bAccepted)
 {
  Active = Next;
  StartTime = Time.Get();
 }
 Elapsed = FMath::Max(0.f, static_cast<float>(Time.Get() - StartTime));
}

FLinearColor SFlickRankResult::Accent() const
{
 const int32 Direction = FlickRankTransition::Direction(Active);
 return Direction < 0 ? FLinearColor(1.f, .36f, .08f) : Direction > 0 ? FlickUITheme::Brand : FlickUITheme::Cyan;
}

void SFlickRankResult::Construct(const FArguments& Args)
{
 Update = Args._Update;
 Time = Args._Time;
 const auto Badge = [this](bool bNew) -> TSharedRef<SWidget>
 {
  return SNew(SBox).WidthOverride(110).HeightOverride(110).RenderTransformPivot(FVector2D(.5f, .5f))
   .RenderTransform_Lambda([this, bNew]()
   {
    const int32 Direction = FlickRankTransition::Direction(Active);
    if (bNew) return FSlateRenderTransform(FScale2D(FlickRankTransition::BadgeScale(Elapsed, Direction)), FVector2D(0, FlickRankTransition::BadgeOffset(Elapsed, Direction)));
    const float T = Direction == 0 ? 0 : FlickRankTransition::Smooth(FlickRankTransition::Phase(Elapsed, 1.05f, .6f));
    return FSlateRenderTransform(FScale2D(1.f - T * .42f), FVector2D(0, Direction < 0 ? T * 20.f : -T * 7.f));
   })
   [SNew(SBorder).BorderImage(FAppStyle::GetBrush("NoBrush")).Padding(0)
    .ColorAndOpacity_Lambda([this, bNew]()
    {
     const int32 Direction = FlickRankTransition::Direction(Active);
     const float Alpha = Direction == 0 ? (bNew == (Elapsed >= FlickRankTransition::RevealSeconds) ? 1.f : 0.f)
      : bNew ? FlickRankTransition::NewOpacity(Elapsed) : FlickRankTransition::OldOpacity(Elapsed);
     return FLinearColor(1, 1, 1, Alpha);
    })
    [SNew(SFlickRankBadge).Size(110).Progress_Lambda([this, bNew]() { return FlickRankTransition::Progress(Active, bNew); })]];
 };
 ChildSlot
 [SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(10).Padding(10)
  .BackgroundColor(Ink).AccentColor_Lambda([this]() { return Accent().CopyWithNewOpacity(.45f + .55f * FlickRankTransition::Pulse(Elapsed)); })
  [SNew(SVerticalBox)
   + SVerticalBox::Slot().AutoHeight()
   [SNew(STextBlock).Font(UiFont(14, true)).Justification(ETextJustify::Center).ColorAndOpacity_Lambda([this]() { return Accent(); })
    .Text_Lambda([this]() {
     if (Elapsed < FlickRankTransition::RevealSeconds) return FText::FromString(TEXT("COMPETITIVE RESULT"));
     const int32 Direction = FlickRankTransition::Direction(Active);
     return FText::FromString(Direction > 0 ? (Active.MatchesPlayed == FlickRankRules::PlacementMatchCount ? TEXT("PLACEMENTS COMPLETE") : TEXT("RANK UP"))
      : Direction < 0 ? TEXT("RANK DOWN") : TEXT("RATING UPDATED"));
    })]
   + SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
   [SNew(SBox).HeightOverride(150)
    [SNew(SOverlay).Clipping(EWidgetClipping::ClipToBounds)
     + SOverlay::Slot()
     [SNew(SFlickRankRevealFX).Seconds_Lambda([this]() { return Elapsed; })
      .Direction_Lambda([this]() { return FlickRankTransition::Direction(Active); }).Accent_Lambda([this]() { return Accent(); })]
     + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Badge(false)]
     + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[Badge(true)]
    ]]
   + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 6)
   [SNew(STextBlock).Font(UiFont(16, true)).Justification(ETextJustify::Center).ColorAndOpacity(Paper)
    .Text_Lambda([this]() { return FText::FromString(FlickRankRules::GetRankLabel(FlickRankTransition::Progress(Active, Elapsed >= FlickRankTransition::RevealSeconds))); })]
   + SVerticalBox::Slot().AutoHeight()
   [SNew(SHorizontalBox)
    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
    [SNew(STextBlock).Font(UiFont(24, true)).ColorAndOpacity(Cyan)
     .Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d MMR"), FlickRankTransition::RatingAt(Active, Elapsed))); })]
    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(7, 0, 0, 0)
    [SNew(SBorder).BorderImage(WhiteBrush()).Padding(FMargin(7, 4))
     .BorderBackgroundColor_Lambda([this]() { return Accent().CopyWithNewOpacity(.12f); })
     [SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity_Lambda([this]() { return Accent(); })
      .Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%+d"), Active.NewRating - Active.OldRating)); })]]]

  ]];
}
