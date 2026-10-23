#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickCosmeticWidgets.h"
#include "UI/FlickLockerWidgets.h"

void SFlickGameLayer::EquipLockerItem(const int32 Category, const int32 Index)
{
	if (Category < 0 || Category >= FlickCosmeticCatalog::CategoryCount || !FlickCosmeticCatalog::GetItems(Category).IsValidIndex(Index)) return;
	if (Category == 0) SelectedBannerStyle = Index;
	else if (Category == 1) SelectedBannerTag = Index;
	else if (Category == 2) SelectedAvatarBorder = Index;
	else if (FlickCosmeticCatalog::IsPuckCategory(Category)) SelectedPuckSkins[Category - FlickCosmeticCatalog::PuckCategoryStart] = Index;
	else SelectedPuckEffects[Category - FlickCosmeticCatalog::TrailCategory] = Index;
	GConfig->SetInt(TEXT("FLICK.ProfileCosmetics"), *FlickCosmeticCatalog::GetConfigKey(Category), Index, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
	if (FlickCosmeticCatalog::IsPuckCategory(Category) && PlayerController.IsValid()) PlayerController->SubmitLocalPuckSkins();
	if (Category >= FlickCosmeticCatalog::TrailCategory && PlayerController.IsValid()) PlayerController->SubmitLocalPuckEffects();
	if (GameMode.IsValid() && SelectedProfileTab == EFlickProfileTab::Customization) GameMode->SetLockerPreview(Category);
}

TSharedRef<SWidget> SFlickGameLayer::BuildLockerCategory(const int32 Category)
{
	const auto Selected = [this, Category]() { return SelectedLockerCategory == Category; };
	const FString Label = Category == FlickCosmeticCatalog::TrailCategory ? TEXT("TRAILS")
		: Category == FlickCosmeticCatalog::SpawnCategory ? TEXT("SPAWN")
		: Category == FlickCosmeticCatalog::KnockoutCategory ? TEXT("KNOCKOUT") : FlickCosmeticCatalog::GetCategoryName(Category);
	TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("LockerCategory%d"), Category)))
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
		.OnClicked_Lambda([this, Category]() {
			SelectedLockerCategory = Category;
			if (GameMode.IsValid() && SelectedProfileTab == EFlickProfileTab::Customization) GameMode->SetLockerPreview(Category);
			return FReply::Handled();
		});
	const TWeakPtr<SButton> Weak = Button;
	Button->SetContent(SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(7).Padding(FMargin(12, 4)).UseAccentForOutline(true)
		.DrawNeutralOutline(false).BorderWidth(.7f)
		.BackgroundColor_Lambda([Selected, Weak]() {
			const auto Pinned = Weak.Pin();
			return Selected() ? Brand : Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()) ? PanelRaised : FLinearColor::Transparent;
		})
		.AccentColor_Lambda([Selected]() { return Selected() ? Brand : FLinearColor::Transparent; })
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
			[SNew(FlickLockerWidgets::SCategoryGlyph).Category(Category).Color_Lambda([Selected]() { return Selected() ? Ink : Muted; })]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(Label)).Font(DisplayFont(11)).ColorAndOpacity_Lambda([Selected]() { return Selected() ? Ink : Paper; })]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("\u203A"))).Font(UiFont(16)).ColorAndOpacity(Cyan)
				.Visibility_Lambda([Selected]() { return Selected() ? EVisibility::HitTestInvisible : EVisibility::Hidden; })]]);
	return SNew(SBox).HeightOverride(FlickLockerStyle::CategoryHeight)[Button];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLockerPuckCard(const int32 Category, const int32 Index)
{
	const auto Equipped = [this, Category, Index]() { return GetSelectedCosmeticIndex(Category) == Index; };
	TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("LockerItem%d_%d"), Category, Index)))
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
		.ToolTipText(FText::FromString(FlickCosmeticCatalog::GetDescription(Category, Index)))
		.OnClicked_Lambda([this, Category, Index]() { EquipLockerItem(Category, Index); return FReply::Handled(); });
	const TWeakPtr<SButton> Weak = Button;
	const auto Active = [Equipped, Weak]() { const auto Pinned = Weak.Pin(); return Equipped() || (Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus())); };
	static const FSlateRoundedBoxBrush CheckBackground(Brand, 10.f);
	Button->SetContent(SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(10).Padding(1.5f).UseAccentForOutline(true)
		.BackgroundColor(Ink).AccentColor_Lambda([Active]() { return Active() ? Brand : Cyan.CopyWithNewOpacity(.45f); })
		[SNew(SOverlay)
			+ SOverlay::Slot()[SNew(SFlickPuckThumbnail).View(EFlickPuckThumbnailView::Locker).CutSize(8.5f).EdgeFade(FMargin(14,34,14,52))
				.Archetype(FlickCosmeticCatalog::GetPuckArchetype(Category)).Skin(Index)]
			+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(14, 10)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[SNew(STextBlock).Text(FText::FromString(FlickCosmeticCatalog::GetItems(Category)[Index])).Font(DisplayFont(12)).ColorAndOpacity(Paper)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
				[SNew(STextBlock).Text_Lambda([Equipped]() { return FText::FromString(Equipped() ? TEXT("EQUIPPED") : TEXT("OWNED")); })
					.Font(DisplayFont(9)).ColorAndOpacity_Lambda([Equipped]() { return Equipped() ? Brand : Muted; })]]
			+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(10)
			[SNew(SBox).WidthOverride(22).HeightOverride(22).Visibility_Lambda([Equipped]() { return Equipped() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[SNew(SBorder).BorderImage(&CheckBackground).Padding(0).VAlign(VAlign_Center)
					[SNew(STextBlock).Text(FText::FromString(TEXT("\u2713"))).Font(UiFont(12, true)).ColorAndOpacity(Ink).Justification(ETextJustify::Center)]]]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).Active_Lambda(Active)]]);
	return SNew(SBox).WidthOverride(FlickLockerStyle::CardWidth).HeightOverride(FlickLockerStyle::CardHeight)[Button];
}

TSharedRef<SWidget> SFlickGameLayer::BuildLockerPage(const TSharedRef<SWidget>& Categories, const TSharedRef<SWidget>& Contents)
{
	const auto IsPuck = [this]() { return GameMode.IsValid() && FlickCosmeticCatalog::IsPuckCategory(SelectedLockerCategory); };
	const auto Rotate = [this](float Degrees) { if (GameMode.IsValid()) GameMode->RotateLockerPreview(Degrees); };
	TSharedRef<SButton> Back = SNew(SButton).Tag(TEXT("LockerBack")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() {
			SelectedProfileTab = EFlickProfileTab::Overview;
			if (GameMode.IsValid()) GameMode->SetLockerPreview(INDEX_NONE);
			return FReply::Handled();
		});
	Back->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(Back)));
	return SNew(SOverlay)
		+ SOverlay::Slot()[SNew(FlickLockerWidgets::SReadabilityShade)]
		+ SOverlay::Slot()
		[
		SNew(SBox).Padding(FMargin(44, 18, 44, 26))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[FrontendBrandHeader(TEXT("PROFILE  //  CUSTOMIZE"))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 20)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 2, 20, 0)
			[SNew(SBox).WidthOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan)]]
			+ SHorizontalBox::Slot().FillWidth(1)
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("YOUR LOCKER"))).Font(DisplayFont(36)).ColorAndOpacity(Paper)]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
				[SNew(STextBlock).Text(FText::FromString(TEXT("Make every puck and every moment yours."))).Font(UiFont(13)).ColorAndOpacity(Muted)]]]
		+ SVerticalBox::Slot().FillHeight(1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 28, 0)
			[SNew(SBox).WidthOverride(FlickLockerStyle::RailWidth)
				[SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Ink.CopyWithNewOpacity(.92f)).AccentColor(Cyan.CopyWithNewOpacity(.48f))
					.UseAccentForOutline(true).CutSize(14).Padding(FMargin(10, 14))[SNew(SScrollBox) + SScrollBox::Slot()[Categories]]]]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(FlickLockerStyle::ContentsWidth)[Contents]]
			+ SHorizontalBox::Slot().FillWidth(1).Padding(16, 0, 0, 0)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()[SNew(SFlickPreviewDrag).Tag(TEXT("LockerPreviewDrag")).OnRotate(FOnFloatValueChanged::CreateLambda(Rotate))
					.Visibility_Lambda([IsPuck]() { return IsPuck() ? EVisibility::Visible : EVisibility::Collapsed; })]
				+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
				[SNew(SBox).WidthOverride(340).HeightOverride(300).Visibility_Lambda([this]() { return SelectedLockerCategory < FlickCosmeticCatalog::PuckCategoryStart ? EVisibility::Visible : EVisibility::Collapsed; })
					[BuildProfileIdentityCard(false)]]
				+ SOverlay::Slot().VAlign(VAlign_Bottom).Padding(0, 0, 0, 8)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[SNew(STextBlock).Tag(TEXT("LockerPreviewName")).Text_Lambda([this]() {
						const FString CategoryName = FlickCosmeticCatalog::IsPuckCategory(SelectedLockerCategory)
							? GetPieceArchetypeName(FlickCosmeticCatalog::GetPuckArchetype(SelectedLockerCategory)) : FlickCosmeticCatalog::GetCategoryName(SelectedLockerCategory);
						return FText::FromString(CategoryName + TEXT("  //  ") + FlickCosmeticCatalog::GetItems(SelectedLockerCategory)[GetSelectedCosmeticIndex(SelectedLockerCategory)]);
					}).Font(DisplayFont(21)).ColorAndOpacity(Paper).AutoWrapText(true).Justification(ETextJustify::Center)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 5, 0, 14)
					[SNew(STextBlock).Text(FText::FromString(TEXT("EQUIPPED"))).Font(DisplayFont(10)).ColorAndOpacity(Brand)]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox).Visibility_Lambda([IsPuck]() { return IsPuck() ? EVisibility::Visible : EVisibility::Collapsed; })
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Tag(TEXT("LockerRotatePrevious")).ButtonStyle(&TransparentButtonStyle).ContentPadding(10).OnClicked_Lambda([Rotate]() { Rotate(-20); return FReply::Handled(); })
							[SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(UiFont(16)).ColorAndOpacity(Paper)]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[SNew(SFlickAngularBorder).RefinedSurface(true).BackgroundColor(Ink.CopyWithNewOpacity(.7f)).AccentColor(FLinearColor::Transparent).DrawNeutralOutline(false).CutSize(9).Padding(FMargin(12, 6))
							[SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 9, 0)
								[SNew(FlickLockerWidgets::SCategoryGlyph).Category(FlickCosmeticCatalog::CategoryCount).Color(Muted)]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[SNew(STextBlock).Text(FText::FromString(TEXT("DRAG TO ROTATE"))).Font(DisplayFont(9)).ColorAndOpacity(Muted)]]]
						+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Tag(TEXT("LockerRotateNext")).ButtonStyle(&TransparentButtonStyle).ContentPadding(10).OnClicked_Lambda([Rotate]() { Rotate(20); return FReply::Handled(); })
							[SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(UiFont(16)).ColorAndOpacity(Paper)]]
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
		[
			SNew(SBox).HeightOverride(54)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(226)[Back]]
				+ SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right).VAlign(VAlign_Center)
				[SNew(SFlickAngularBorder).RefinedSurface(true).CutSize(10).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.42f)).BackgroundColor(Ink.CopyWithNewOpacity(.8f)).Padding(FMargin(22, 12))
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 20, 0)[SNew(STextBlock).Text(FText::FromString(TEXT("|"))).Font(DisplayFont(14)).ColorAndOpacity(Brand)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("SELECT TO EQUIP  //  SAVES AUTOMATICALLY"))).Font(DisplayFont(10)).ColorAndOpacity(Muted)]]]
			]
		]
		]
	];
}
