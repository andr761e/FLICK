#include "UI/FlickGameLayerPrivate.h"
#include "Core/FlickPracticeCatalog.h"
#include "Game/FlickPracticeComponent.h"
#include "UI/FlickPlaylistWidgets.h"
#include "UI/FlickPracticeStyle.h"

namespace
{
	class SPracticeDifficultyGlyph final : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPracticeDifficultyGlyph) : _Color(FlickUITheme::Cyan), _Mastered(false) {}
			SLATE_ATTRIBUTE(FLinearColor, Color)
			SLATE_ATTRIBUTE(bool, Mastered)
		SLATE_END_ARGS()
		void Construct(const FArguments& Args)
		{
			Color=Args._Color; Mastered=Args._Mastered; SetCanTick(false);
			ForceVolatile(Color.IsBound() || Mastered.IsBound());
		}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(32,32); }
		virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& E,
			int32 L,const FWidgetStyle& Style,bool) const override
		{
			const FVector2D Size=G.GetLocalSize(), Center=Size*.5;
			const FLinearColor Tint=Color.Get()*Style.GetColorAndOpacityTint();
			// A closed annulus avoids the open seam left by a thick Slate polyline.
			// Transparent outer/inner bands provide subpixel edge coverage.
			constexpr int32 Segments=48;
			constexpr float Offsets[]={2.35f,1.75f,-1.75f,-2.35f};
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			for (int32 I=0; I<=Segments; ++I)
			{
				const float Angle=(I%Segments)*UE_TWO_PI/Segments;
				const FVector2D Direction(FMath::Cos(Angle),FMath::Sin(Angle));
				for (int32 Band=0; Band<4; ++Band)
				{
					FLinearColor ColorAtEdge=Tint;
					if (Band==0 || Band==3) ColorAtEdge.A=0;
					Vertices.Add(FSlateVertex::Make(G.GetAccumulatedRenderTransform(),
						FVector2f(Center+Direction*(Size.GetMin()*.38+Offsets[Band])),FVector2f::ZeroVector,ColorAtEdge.ToFColor(true)));
				}
				if (I>0) for (int32 Band=0; Band<3; ++Band)
				{
					const SlateIndex V=(I-1)*4+Band;
					Indices.Append({V,static_cast<SlateIndex>(V+1),static_cast<SlateIndex>(V+4),
						static_cast<SlateIndex>(V+1),static_cast<SlateIndex>(V+5),static_cast<SlateIndex>(V+4)});
				}
			}
			FSlateDrawElement::MakeCustomVerts(E,L,WhiteBrush()->GetRenderingResource(),Vertices,Indices,nullptr,0,0);
			if (Mastered.Get())
			{
				const TArray<FVector2D> Tick={Size*FVector2D(.30,.49),Size*FVector2D(.44,.63),Size*FVector2D(.72,.34)};
				FSlateDrawElement::MakeLines(E,L,G.ToPaintGeometry(),Tick,ESlateDrawEffect::None,Tint,true,2);
			}
			return L;
		}
	private:
		TAttribute<FLinearColor> Color;
		TAttribute<bool> Mastered;
	};
}

TSharedRef<SWidget> SFlickGameLayer::BuildPracticeMenu()
{
	using namespace FlickPracticeStyle;
	const auto CategoryForDetails=[this]() { return FMath::Clamp(SelectedPracticeCategory,0,FlickPracticeCatalog::CategoryCount-1); };
	const EFlickMenuArtwork Artwork[]={EFlickMenuArtwork::PracticePrecision,EFlickMenuArtwork::PracticeKnockouts,
		EFlickMenuArtwork::PracticeSwitch,EFlickMenuArtwork::PracticeBob};
	static_assert(UE_ARRAY_COUNT(Artwork)==FlickPracticeCatalog::CategoryCount);
	auto Categories=SNew(SGridPanel).FillColumn(0,1).FillColumn(1,1).FillRow(0,1).FillRow(1,1);
	for (int32 Category=0; Category<FlickPracticeCatalog::CategoryCount; ++Category)
	{
		TSharedRef<SButton> Button=SNew(SButton).Tag(FName(*FString::Printf(TEXT("PracticeFocus%d"),Category)))
			.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand).HAlign(HAlign_Fill).VAlign(VAlign_Fill)
			.ToolTipText(FText::FromString(FlickPracticeCatalog::Description(Category)))
			.OnClicked_Lambda([this,Category]() { SelectedPracticeCategory=Category; if (GameMode.IsValid()) GameMode->PlayMenuSound(true); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak=Button;
		const auto Active=[CategoryForDetails,Category,Weak]() { const auto B=Weak.Pin();
			return CategoryForDetails()==Category || (B && (B->IsHovered() || B->HasKeyboardFocus())); };
		auto Scores=SNew(SHorizontalBox);
		for (int32 Difficulty=0; Difficulty<FlickPracticeCatalog::DifficultyCount; ++Difficulty)
		{
			if (Difficulty>0) Scores->AddSlot().AutoWidth().Padding(10,0)
				[SNew(STextBlock).Text(FText::FromString(TEXT("|"))).Font(DisplayFont(10)).ColorAndOpacity(Brand)];
			Scores->AddSlot().AutoWidth()
				[SNew(STextBlock).Tag(FName(*FString::Printf(TEXT("PracticeScore%d_%d"),Category,Difficulty)))
					.Font(DisplayFont(10)).ColorAndOpacity(Brand).Text_Lambda([Category,Difficulty]() {
						return FText::FromString(FString::Printf(TEXT("%s %d/%d"),FlickPracticeCatalog::DifficultyName(Difficulty),
							FlickPracticeCatalog::BestScore(Category,Difficulty),FlickPracticeCatalog::ChallengesPerRun)); })];
		}
		Button->SetContent(SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SFlickAngularBorder).CutSize(13).UseAccentForOutline(true).Padding(1.5f)
				.BackgroundColor(Ink).AccentColor_Lambda([Active]() { return Active() ? Brand : Cyan.CopyWithNewOpacity(.52f); })
				[SNew(SFlickMenuArtwork).Artwork(Artwork[Category]).CutSize(11.5f).ShadeForText(true).TextShadeStart(.68f).EdgeFade(FMargin(140,0,0,0))]]
			+ SOverlay::Slot().Padding(28,18,22,16)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(FlickPracticeCatalog::Name(Category))).Font(DisplayFont(26)).ColorAndOpacity(Paper)
						.ShadowOffset(FVector2D(0,1)).ShadowColorAndOpacity(FLinearColor(0,0,0,.7f))]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,7,0,0)
					[SNew(STextBlock).Text(FText::FromString(FocusDescription(Category))).Font(UiFont(12)).ColorAndOpacity(Paper)
						.ShadowOffset(FVector2D(0,1)).ShadowColorAndOpacity(FLinearColor(0,0,0,.8f))]
					+ SVerticalBox::Slot().FillHeight(1)[SNew(SSpacer)]
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[Scores]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8,0,0,0)
						[SNew(STextBlock).Text(FText::FromString(TEXT("\u203A"))).Font(UiFont(28))
							.ColorAndOpacity_Lambda([Active]() { return Active() ? Brand : Paper; })]]]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).Active_Lambda(Active)]);
		Categories->AddSlot(Category%2,Category/2).Padding(FMargin(Category%2 ? CardGap/2 : 0,
			Category/2 ? CardGap/2 : 0,Category%2 ? 0 : CardGap/2,Category/2 ? 0 : CardGap/2))[Button];
	}
	auto Difficulties=SNew(SVerticalBox);
	for (int32 Difficulty=0; Difficulty<FlickPracticeCatalog::DifficultyCount; ++Difficulty)
	{
		const auto Selected=[this,Difficulty]() { return SelectedPracticeDifficulty==Difficulty; };
		const auto Color=TAttribute<FLinearColor>::CreateLambda([Selected]() { return Selected() ? Ink : Paper; });
		TSharedRef<SButton> Button=SNew(SButton).Tag(FName(*FString::Printf(TEXT("PracticeDifficulty%d"),Difficulty)))
			.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).HAlign(HAlign_Fill).VAlign(VAlign_Fill).Cursor(EMouseCursor::Hand)
			.ToolTipText_Lambda([CategoryForDetails,Difficulty]() { return FText::FromString(FString::Printf(TEXT("%s\nBEST %d/%d%s"),
				FlickPracticeStyle::DifficultyDescription(CategoryForDetails(),Difficulty),
				FlickPracticeCatalog::BestScore(CategoryForDetails(),Difficulty),FlickPracticeCatalog::ChallengesPerRun,
				FlickPracticeCatalog::BestScore(CategoryForDetails(),Difficulty)==FlickPracticeCatalog::ChallengesPerRun ? TEXT(" // MASTERED") : TEXT(""))); })
			.OnClicked_Lambda([this,Difficulty]() { SelectedPracticeDifficulty=Difficulty; if (GameMode.IsValid()) GameMode->PlayMenuSound(true); return FReply::Handled(); });
		const TWeakPtr<SButton> Weak=Button;
		Button->SetContent(SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(10).UseAccentForOutline(true).Padding(FMargin(20,7))
			.BackgroundColor_Lambda([Selected,Weak]() { const auto B=Weak.Pin(); return Selected() ? Brand
				: B && (B->IsHovered() || B->HasKeyboardFocus()) ? PanelRaised : Ink.CopyWithNewOpacity(.84f); })
			.AccentColor_Lambda([Selected,Weak]() { const auto B=Weak.Pin(); return Selected() || (B && B->HasKeyboardFocus()) ? Brand : Hairline; })
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,16,0)
				[SNew(SBox).WidthOverride(32).HeightOverride(32)
					[SNew(SPracticeDifficultyGlyph).Color_Lambda([Selected,Difficulty]() { return Selected() ? Ink : Difficulty==2 ? Orange : Cyan; })
						.Mastered_Lambda([CategoryForDetails,Difficulty]() { return FlickPracticeCatalog::BestScore(CategoryForDetails(),Difficulty)==FlickPracticeCatalog::ChallengesPerRun; })]]
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(STextBlock).Text(FText::FromString(FlickPracticeCatalog::DifficultyName(Difficulty))).Font(DisplayFont(15))
						.ColorAndOpacity_Lambda([Color]() { return FSlateColor(Color.Get()); })]
					+ SVerticalBox::Slot().AutoHeight().Padding(0,2,0,0)
					[SNew(STextBlock).Tag(FName(*FString::Printf(TEXT("PracticeDifficultyDescription%d"),Difficulty)))
						.Text_Lambda([CategoryForDetails,Difficulty]() { return FText::FromString(DifficultyDescription(CategoryForDetails(),Difficulty)); })
						.Font(UiFont(9)).ColorAndOpacity_Lambda([Selected]() { return Selected() ? Ink : Muted; }).AutoWrapText(true)]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,18,0)
				[SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(5).UseAccentForOutline(true).Padding(FMargin(9,5))
					.BackgroundColor_Lambda([Selected]() { return Selected() ? Ink.CopyWithNewOpacity(.075f) : Ink; })
					.AccentColor_Lambda([Selected]() { return Selected() ? Ink.CopyWithNewOpacity(.22f) : Cyan.CopyWithNewOpacity(.28f); })
				[SNew(STextBlock).Tag(FName(*FString::Printf(TEXT("PracticeDifficultyScore%d"),Difficulty))).Font(DisplayFont(15))
					.ColorAndOpacity_Lambda([Selected]() { return Selected() ? Ink : Brand; })
					.Text_Lambda([CategoryForDetails,Difficulty]() { return FText::FromString(FString::Printf(TEXT("%d / %d"),
						FlickPracticeCatalog::BestScore(CategoryForDetails(),Difficulty),FlickPracticeCatalog::ChallengesPerRun)); })]]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[SNew(FlickPlaylistWidgets::SCardArrow).Divider(false).Color(Color)]]]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).CutSize(10).Active_Lambda(Selected)]);
		Difficulties->AddSlot().AutoHeight().Padding(0,0,0,Difficulty<2 ? 10 : 0)
			[SNew(SBox).HeightOverride(DifficultyHeight)[Button]];
	}
	TSharedRef<SButton> Start=SNew(SButton).Tag(TEXT("PracticeStart")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.Cursor(EMouseCursor::Hand).HAlign(HAlign_Fill).VAlign(VAlign_Fill).IsEnabled_Lambda([this]() { return GameMode.IsValid(); })
		.ToolTipText(FText::FromString(TEXT("Five challenges, one attempt per challenge. Your best completed run is saved for this difficulty. 5/5 earns mastery. Restart begins a fresh run.")))
		.OnClicked_Lambda([this,CategoryForDetails]() { if (GameMode.IsValid()) GameMode->StartPractice(CategoryForDetails(),SelectedPracticeDifficulty); return FReply::Handled(); });
	const TWeakPtr<SButton> WeakStart=Start;
	Start->SetContent(SNew(SFlickAngularBorder).CutSize(13).UseAccentForOutline(true).AccentColor(Brand)
		.BackgroundColor_Lambda([WeakStart]() { const auto B=WeakStart.Pin(); return B && (B->IsHovered() || B->HasKeyboardFocus()) ? FLinearColor(.85f,1,.14f) : Brand; }).Padding(FMargin(24,0))
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,24,0)
			[SNew(SBox).WidthOverride(30).HeightOverride(30)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SFlickMainMenuIcon).Icon(EFlickMainMenuIcon::Play).Color(Ink)]]]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("START PRACTICE"))).Font(DisplayFont(24)).ColorAndOpacity(Ink)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12,0,0,0)
			[SNew(STextBlock).Text(FText::FromString(TEXT("\u2192"))).Font(UiFont(24)).ColorAndOpacity(Ink)]]);
	TSharedRef<SButton> Back=SNew(SButton).Tag(TEXT("PracticeBack")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() { SelectedTrainingActivity=EFlickTrainingActivity::None; if (GameMode.IsValid()) GameMode->PlayMenuSound(true); return FReply::Handled(); });
	Back->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(Back)));
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(ContentWidth).HeightOverride(ContentHeight)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(SBox).HeightOverride(HeaderHeight)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(SBox).WidthOverride(236).HeightOverride(90)[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
							[SNew(SFlickLogoWidget).VectorPath(TEXT("UI/FlickKnockoutWordmark.svg")).TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark")).DesiredSize(FVector2D(2048,683))]]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(26,0)
						[SNew(SBox).WidthOverride(1).HeightOverride(84)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Hairline)]]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
						[SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("PLAY"))).Font(DisplayFont(12)).ColorAndOpacity(Muted)]
								+ SHorizontalBox::Slot().AutoWidth().Padding(12,0)[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(DisplayFont(12)).ColorAndOpacity(Cyan)]
								+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("TRAINING"))).Font(DisplayFont(12)).ColorAndOpacity(Muted)]
								+ SHorizontalBox::Slot().AutoWidth().Padding(12,0)[SNew(STextBlock).Text(FText::FromString(TEXT("//"))).Font(DisplayFont(12)).ColorAndOpacity(Cyan)]
								+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(FText::FromString(TEXT("PRACTICE PACKS"))).Font(DisplayFont(12)).ColorAndOpacity(Paper)]]
							+ SVerticalBox::Slot().AutoHeight().Padding(0,12,0,0)
							[SNew(STextBlock).Text(FText::FromString(TEXT("BUILD YOUR PRECISION."))).Font(DisplayFont(35)).ColorAndOpacity(Paper)]
							+ SVerticalBox::Slot().AutoHeight().Padding(0,3,0,0)
							[SNew(STextBlock).Text(FText::FromString(TEXT("Five shots per run. Master every pack with a perfect score."))).Font(UiFont(12)).ColorAndOpacity(Muted)]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(24,0,0,0)
						[SNew(SBox).WidthOverride(330).HeightOverride(54)
							[SNew(SFlickAngularBorder).CutSize(12).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.6f)).BackgroundColor(Ink.CopyWithNewOpacity(.94f)).Padding(FMargin(26,0))
								[SNew(SHorizontalBox)
									+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
									[SNew(STextBlock).Text(FText::FromString(TEXT("MASTERY  //"))).Font(DisplayFont(15)).ColorAndOpacity(Muted)]
									+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right)
									[SNew(STextBlock).Tag(TEXT("PracticeMastery")).Font(DisplayFont(16)).ColorAndOpacity(Brand)
										.Text_Lambda([]() { return FText::FromString(FString::Printf(TEXT("%d / %d PACKS"),FlickPracticeCatalog::PerfectPacks(),
											FlickPracticeCatalog::CategoryCount*FlickPracticeCatalog::DifficultyCount)); })]]]]]]
				+ SVerticalBox::Slot().FillHeight(1).Padding(0,18,0,0)
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1)[Categories]
					+ SHorizontalBox::Slot().AutoWidth().Padding(ColumnGap,0,0,0)
					[SNew(SBox).WidthOverride(DetailWidth)
						[SNew(SFlickAngularBorder).Tag(TEXT("PracticeDetails")).CutSize(15).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.5f))
							.BackgroundColor(Ink.CopyWithNewOpacity(.94f)).Padding(DetailPadding)
							[SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()
								[SNew(STextBlock).Tag(TEXT("PracticeFocusTitle")).Text_Lambda([CategoryForDetails]() { return FText::FromString(FlickPracticeCatalog::Name(CategoryForDetails())); }).Font(DisplayFont(30)).ColorAndOpacity(Paper)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0,3,0,14)
								[SNew(STextBlock).Text(FText::FromString(TEXT("// CHOOSE DIFFICULTY"))).Font(DisplayFont(16)).ColorAndOpacity(Cyan)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)
								[SNew(SBox).HeightOverride(54)[SNew(STextBlock).Text_Lambda([CategoryForDetails]() { return FText::FromString(FlickPracticeCatalog::Description(CategoryForDetails())); })
									.Font(UiFont(12)).ColorAndOpacity(Muted).AutoWrapText(true)]]
								+ SVerticalBox::Slot().AutoHeight()[Difficulties]
								+ SVerticalBox::Slot().FillHeight(1)[SNew(SSpacer)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0,18,0,18)
								[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.18f))]]
								+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(StartHeight)[Start]]]]]]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,24,0,0)
				[SNew(SBox).HeightOverride(FooterHeight)
					[SNew(SFlickAngularBorder).CutSize(12).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.6f)).BackgroundColor(Ink.CopyWithNewOpacity(.9f)).Padding(FMargin(12,9))
						[SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(174)[Back]]
							+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).HAlign(HAlign_Right).Padding(24,0)
							[SNew(STextBlock).Text(FText::FromString(TEXT("5/5 earns mastery   //   Best completed run saved."))).Font(UiFont(13)).ColorAndOpacity(Muted)]]]]]];
}

TSharedRef<SWidget> SFlickGameLayer::BuildPracticeOverlay()
{
	const auto Practice = [this]() -> UFlickPracticeComponent* { return GameMode.IsValid() ? GameMode->GetPractice() : nullptr; };
	auto Results = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < FlickPracticeCatalog::ChallengesPerRun; ++Index)
	{
		Results->AddSlot().FillWidth(1).Padding(2)
		[
			SNew(SBorder).BorderImage(WhiteBrush()).Padding(6)
			.BorderBackgroundColor_Lambda([Practice, Index]() { const int32 Result = Practice() ? Practice()->GetResult(Index) : -1;
				return Result < 0 ? PanelRaised : Result ? FLinearColor(.08f, .22f, .02f, 1) : FLinearColor(.28f, .05f, .02f, 1); })
			[
				SNew(STextBlock).Font(UiFont(9, true)).ColorAndOpacity(Paper).Justification(ETextJustify::Center)
				.Text_Lambda([Practice, Index]() { const int32 Result = Practice() ? Practice()->GetResult(Index) : -1;
					return FText::FromString(Result < 0 ? FString::FromInt(Index + 1) : Result ? TEXT("PASS") : TEXT("MISS")); })
			]
		];
	}
	return SNew(SBox).WidthOverride(392)
		.Visibility_Lambda([this]() { return GameMode.IsValid() && GameMode->IsPracticeMode() && GameMode->GetFrontendScreen() == EFlickFrontendScreen::Playing
			? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		[
			SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Ink.CopyWithNewOpacity(.97f)).AccentColor(Cyan).UseAccentForOutline(true).CutSize(14).Padding(20)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Font(UiFont(10, true)).ColorAndOpacity(Brand).Text_Lambda([Practice]() { return FText::FromString(Practice()
					? FString::Printf(TEXT("PRACTICE  /  %s"), FlickPracticeCatalog::DifficultyName(Practice()->GetDifficulty())) : FString()); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 5)
				[SNew(STextBlock).Font(DisplayFont(25)).ColorAndOpacity(Paper).Text_Lambda([Practice]() { return FText::FromString(Practice() ? FlickPracticeCatalog::Name(Practice()->GetCategory()) : TEXT("PRACTICE")); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 10)
				[SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity(Cyan).Text_Lambda([Practice]() { return FText::FromString(Practice()
					? FString::Printf(TEXT("%s  /  SCORE %d OF 5"), Practice()->IsComplete() ? TEXT("RUN COMPLETE") : *FString::Printf(TEXT("CHALLENGE %d"), Practice()->GetChallengeNumber()), Practice()->GetScore()) : FString()); })]
				+ SVerticalBox::Slot().AutoHeight()[Results]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 14)
				[SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity(Paper).AutoWrapText(true).Text_Lambda([Practice]() { return FText::FromString(Practice() ? Practice()->GetObjective() : FString()); })]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
				[SNew(STextBlock).Font(UiFont(10)).ColorAndOpacity(Muted).AutoWrapText(true).Text_Lambda([Practice]() { return FText::FromString(Practice() ? Practice()->GetFeedback() : FString()); })]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
					[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).OnClicked_Lambda([this]() { if (GameMode.IsValid()) GameMode->RestartMatch(); return FReply::Handled(); })
						[SNew(STextBlock).Font(UiFont(10, true)).ColorAndOpacity(Paper).Text(FText::FromString(TEXT("RESTART RUN")))]]
					+ SHorizontalBox::Slot().FillWidth(1).Padding(4, 0, 0, 0)
					[SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).OnClicked_Lambda([this, Practice]() {
						SelectedPracticeCategory = Practice() ? Practice()->GetCategory() : INDEX_NONE;
						SelectedPlayPlaylist = EFlickPlayPlaylist::Training; SelectedTrainingActivity = EFlickTrainingActivity::PracticePacks;
						if (GameMode.IsValid()) { GameMode->ReturnToMainMenu(); GameMode->OpenModeSelect(); }
						return FReply::Handled(); })[SNew(STextBlock).Font(UiFont(10, true)).ColorAndOpacity(Paper).Text(FText::FromString(TEXT("PRACTICE PACKS")))]]
				]
			]
		];
}
