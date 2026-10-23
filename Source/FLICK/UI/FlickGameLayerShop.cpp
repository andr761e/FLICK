#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickShopArtwork.h"
#include "UI/FlickShopStyle.h"
#include "UI/FlickShopWidgets.h"
#include "Widgets/Layout/SWrapBox.h"

namespace
{
	TSharedRef<SWidget> ShopText(const FString& Text, int32 Size, FLinearColor Color)
	{
		return SNew(STextBlock).Text(FText::FromString(Text)).Font(UiFont(Size, true)).ColorAndOpacity(Color);
	}
	FString ShopPrice(const int32 Price) { return FText::AsNumber(Price).ToString() + TEXT(" CR"); }
}

FReply SFlickGameLayer::OpenShopItem(const int32 ItemIndex)
{
	if (!FlickShopCatalog::Find(ItemIndex)) return FReply::Handled();
	ShopReturnFocus = FSlateApplication::Get().GetKeyboardFocusedWidget();
	SelectedShopItem = ItemIndex;
	ShopStatus.Reset();
	if (ShopDetailCloseButton.IsValid()) FSlateApplication::Get().SetKeyboardFocus(ShopDetailCloseButton, EFocusCause::SetDirectly);
	if (GameMode.IsValid()) GameMode->PlayMenuSound(true);
	return FReply::Handled();
}

FReply SFlickGameLayer::CloseShopItem()
{
	SelectedShopItem = INDEX_NONE;
	if (const auto Focus = ShopReturnFocus.Pin()) FSlateApplication::Get().SetKeyboardFocus(Focus, EFocusCause::SetDirectly);
	return FReply::Handled();
}

FReply SFlickGameLayer::AcquireShopItem()
{
	switch (ShopDemo.Acquire(SelectedShopItem))
	{
	case EFlickShopDemoResult::Acquired: ShopStatus = TEXT("Added to your demo collection. No real item was unlocked."); break;
	case EFlickShopDemoResult::AlreadyOwned: ShopStatus = TEXT("Already in your demo collection."); break;
	case EFlickShopDemoResult::InsufficientCredits: ShopStatus = TEXT("Not enough demo credits. Reset the demo balance to try again."); break;
	default: ShopStatus = TEXT("Select an item to inspect first."); break;
	}
	if (GameMode.IsValid()) GameMode->PlayMenuSound(true);
	return FReply::Handled();
}

FReply SFlickGameLayer::OnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (SelectedShopItem != INDEX_NONE && GetScreenVisibility(EFlickFrontendScreen::ItemShop) == EVisibility::Visible
		&& (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right)) return CloseShopItem();
	return SCompoundWidget::OnPreviewKeyDown(Geometry, Event);
}

TSharedRef<SWidget> SFlickGameLayer::BuildShopItemCard(const int32 ItemIndex)
{
	const auto& Item = FlickShopCatalog::GetItems()[ItemIndex];
	TSharedRef<SButton> Button = SNew(SButton).Tag(FName(*FString::Printf(TEXT("ShopItem%d"), ItemIndex)))
		.ButtonStyle(&TransparentButtonStyle).ContentPadding(0).Cursor(EMouseCursor::Hand)
		.ToolTipText(FText::FromString(FString(Item.Rarity) + TEXT(" // ") + Item.Description))
		.OnClicked_Lambda([this, ItemIndex]() { return OpenShopItem(ItemIndex); });
	const TWeakPtr<SButton> Weak = Button;
	const auto Active = [Weak]() { const auto Pinned = Weak.Pin(); return Pinned && (Pinned->IsHovered() || Pinned->HasKeyboardFocus()); };
	Button->SetContent(SNew(SFlickAngularBorder).BackgroundColor(Ink).UseAccentForOutline(true)
		.AccentColor_Lambda([Active, ItemIndex]() { return Active() ? Brand : FlickShopCatalog::Find(ItemIndex)->Accent.CopyWithNewOpacity(.65f); })
		.CutSize(9).BorderWidth(1).Padding(1.5f)
		[SNew(SOverlay)
			+ SOverlay::Slot()
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().FillHeight(1)
				[SNew(SFlickShopArtwork).ItemIndex(ItemIndex).Cover(true).CutSize(7.5f)]
				+ SVerticalBox::Slot().AutoHeight().Padding(14, 7, 14, 10)
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ShopText(Item.Type, 8, Muted)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
						[SNew(STextBlock).Text(FText::FromString(Item.Name)).Font(DisplayFont(13)).ColorAndOpacity(Paper)]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0, 0, 0)
						[SNew(STextBlock).Font(DisplayFont(9)).ColorAndOpacity_Lambda([this, ItemIndex]() { return ShopDemo.Owns(ItemIndex) ? Brand : Muted; })
							.Text_Lambda([this, ItemIndex]() { return FText::FromString(ShopDemo.Owns(ItemIndex) ? TEXT("IN COLLECTION") : ShopPrice(FlickShopCatalog::Find(ItemIndex)->Price)); })]]]]
			+ SOverlay::Slot()[SNew(FlickPlaylistWidgets::SSelectionEdge).Active_Lambda(Active)]]);
	return SNew(SBox).HeightOverride(FlickShopStyle::CardHeight)[Button];
}

TSharedRef<SWidget> SFlickGameLayer::BuildItemShop()
{
	TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const auto Filter = static_cast<EFlickShopFilter>(Index);
		Tabs->AddSlot().AutoWidth().Padding(0, 0, 5, 0)
		[SNew(SButton).Tag(FName(*FString::Printf(TEXT("ShopFilter%d"), Index))).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
			.OnClicked_Lambda([this, Filter]() { SelectedShopFilter = Filter; ShopStatus.Reset(); return FReply::Handled(); })
			[SNew(SFlickAngularBorder).CutSize(6).Padding(FMargin(22, 11)).UseAccentForOutline(true)
				.BackgroundColor_Lambda([this, Filter]() { return SelectedShopFilter == Filter ? Brand : Ink.CopyWithNewOpacity(.75f); })
				.AccentColor_Lambda([this, Filter]() { return SelectedShopFilter == Filter ? Brand : Hairline; })
				[SNew(STextBlock).Text(FText::FromString(FlickShopCatalog::GetFilterName(Filter))).Font(DisplayFont(11))
					.ColorAndOpacity_Lambda([this, Filter]() { return SelectedShopFilter == Filter ? Ink : Paper; })]]];
	}
	Tabs->AddSlot().FillWidth(1).VAlign(VAlign_Bottom).Padding(2, 0, 0, 0)
		[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.6f))]];

	TSharedRef<SWrapBox> Cards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(FlickShopStyle::CardGap, FlickShopStyle::CardGap));
	const TWeakPtr<SWrapBox> WeakCards = Cards;
	for (int32 Index = 0; Index < FlickShopCatalog::GetItems().Num(); ++Index)
	{
		Cards->AddSlot()
		[SNew(SBox).WidthOverride_Lambda([WeakCards]() {
			const auto Layout = WeakCards.Pin();
			const float Width = Layout ? Layout->GetCachedGeometry().GetLocalSize().X : 0;
			return Width > 0 ? FlickShopStyle::CardWidth(Width) : 230.f;
		}).Visibility_Lambda([this, Index]() { return ShopDemo.Matches(Index, SelectedShopFilter) ? EVisibility::Visible : EVisibility::Collapsed; })
			[BuildShopItemCard(Index)]];
	}
	TSharedRef<SButton> Back = SNew(SButton).Tag(TEXT("ShopBack")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() {
			CloseShopItem();
			if (GameMode.IsValid()) GameMode->CloseItemShop(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu;
			return FReply::Handled();
		});
	Back->SetContent(FlickPlaylistWidgets::BackContent(TWeakPtr<SButton>(Back)));

	TSharedRef<SButton> Inspect = SNew(SButton).Tag(TEXT("ShopFeaturedInspect")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.OnClicked_Lambda([this]() { return OpenShopItem(0); });
	Inspect->SetContent(SNew(SFlickAngularBorder).CutSize(10).Padding(FMargin(22, 0)).BackgroundColor(Brand).AccentColor(Brand).UseAccentForOutline(true)
		[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 24, 0)
			[SNew(STextBlock).Text(FText::FromString(TEXT("\u25B6"))).Font(UiFont(16)).ColorAndOpacity(Ink)]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ShopDemo.Owns(0) ? TEXT("INSPECT // IN COLLECTION") : TEXT("INSPECT // ") + ShopPrice(FlickShopCatalog::Find(0)->Price)); })
				.Font(DisplayFont(15)).ColorAndOpacity(Ink)]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[SNew(STextBlock).Text(FText::FromString(TEXT("\u2192"))).Font(UiFont(23)).ColorAndOpacity(Ink)]]);

	return SNew(SOverlay)
		+ SOverlay::Slot()[FlickPlaylistWidgets::Background()]
		+ SOverlay::Slot().Padding(FMargin(FlickShopStyle::Margin, 16, FlickShopStyle::Margin, 20))
		[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(SBox).HeightOverride(FlickShopStyle::HeaderHeight)
				[SNew(SOverlay)
					+ SOverlay::Slot()
					[SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()
							[SNew(SBox).WidthOverride(194).HeightOverride(59)
								[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
									[SNew(SFlickLogoWidget).VectorPath(TEXT("UI/FlickKnockoutWordmark.svg"))
										.TexturePath(TEXT("/Game/UI/FlickKnockoutWordmark.FlickKnockoutWordmark")).DesiredSize(FVector2D(2048, 683))]]]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18, 0)
							[ShopText(TEXT("|   P H Y S I C S .  P R E C I S I O N .  R I V A L R Y ."), 7, Muted)]]
						+ SVerticalBox::Slot().AutoHeight().Padding(2, 10, 0, 0)
						[SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth()[ShopText(TEXT("HOME"), 11, Muted)]
							+ SHorizontalBox::Slot().AutoWidth().Padding(13, 0)[ShopText(TEXT("//"), 11, Cyan)]
							+ SHorizontalBox::Slot().AutoWidth()[ShopText(TEXT("ITEM SHOP"), 11, Brand)]]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 0)
						[SNew(STextBlock).Text(FText::FromString(TEXT("STYLE WITHOUT COMPROMISE"))).Font(DisplayFont(31)).ColorAndOpacity(Paper)]]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(0, 22, 0, 0)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 18, 0)
						[SNew(SFlickAngularBorder).CutSize(10).UseAccentForOutline(true).AccentColor(Muted.CopyWithNewOpacity(.65f)).BackgroundColor(Ink.CopyWithNewOpacity(.85f)).Padding(FMargin(17, 9))
							[SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 15, 0)[SNew(FlickShopWidgets::SStatusIcon).Coins(true)]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()[ShopText(TEXT("DEMO BALANCE //"), 8, Muted)]
									+ SVerticalBox::Slot().AutoHeight()
									[SNew(STextBlock).Tag(TEXT("ShopBalance")).Text_Lambda([this]() { return FText::FromString(ShopPrice(ShopDemo.GetCredits())); }).Font(DisplayFont(21)).ColorAndOpacity(Brand)]]]]
						+ SHorizontalBox::Slot().AutoWidth()
						[SNew(SFlickAngularBorder).CutSize(10).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.6f)).BackgroundColor(Ink.CopyWithNewOpacity(.85f)).Padding(FMargin(17, 12))
							[SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 17, 0)[SNew(FlickShopWidgets::SStatusIcon)]
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[SNew(SVerticalBox)
									+ SVerticalBox::Slot().AutoHeight()[ShopText(TEXT("SHOP PREVIEW"), 9, Paper)]
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)[ShopText(TEXT("NO REAL PURCHASES"), 8, Muted)]]]]]]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)[SNew(SBox).HeightOverride(40)[Tabs]]
			+ SVerticalBox::Slot().FillHeight(1)
			[SNew(SScrollBox).Tag(TEXT("ShopCatalogue")).ScrollBarThickness(FVector2D(5, 5))
				+ SScrollBox::Slot()
				[SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(SBox).HeightOverride(FlickShopStyle::HeroHeight)
						.Visibility_Lambda([this]() { return SelectedShopFilter == EFlickShopFilter::All ? EVisibility::Visible : EVisibility::Collapsed; })
						[SNew(SFlickAngularBorder).BackgroundColor(Ink).UseAccentForOutline(true).AccentColor(Cyan.CopyWithNewOpacity(.65f)).CutSize(14).BorderWidth(1).Padding(1.5f)
							[SNew(SOverlay)
								+ SOverlay::Slot()[SNew(SFlickMenuArtwork).Artwork(EFlickMenuArtwork::Backdrop).CutSize(12.5f)]
								+ SOverlay::Slot()[SNew(SFlickShopArtwork).ItemIndex(0).Hero(true).CutSize(12.5f)]
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(30, 12)
								[SNew(SBox).WidthOverride(430)
									[SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[ShopText(TEXT("PUCK FINISH  //  EXOTIC"), 10, Cyan)]
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
										[SNew(STextBlock).Text(FText::FromString(TEXT("ION SHIFT"))).Font(DisplayFont(57)).ColorAndOpacity(Paper)]
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)[ShopText(TEXT("Precision, with a pulse."), 15, Paper)]
										+ SVerticalBox::Slot().AutoHeight().Padding(0, 9, 0, 14)
										[SNew(STextBlock).Text(FText::FromString(TEXT("Obsidian. Titanium. Ice-cyan light.\nA new signature for the arena."))).Font(UiFont(11)).ColorAndOpacity(Muted)]
										+ SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(49)[Inspect]]]]]]]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 10)
					[SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18, 0)
						[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedShopFilter == EFlickShopFilter::All ? TEXT("|  CURATED FOR THE ARENA") : TEXT("|  ") + FString(FlickShopCatalog::GetFilterName(SelectedShopFilter))); }).Font(DisplayFont(15)).ColorAndOpacity(Paper)]
						+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Bottom).Padding(0, 0, 18, 0)
						[SNew(SBox).HeightOverride(1)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Cyan.CopyWithNewOpacity(.4f))]]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[ShopText(TEXT("COSMETICS ONLY  //  NO GAMEPLAY ADVANTAGE"), 9, Muted)]]
					+ SVerticalBox::Slot().AutoHeight()[Cards]
					+ SVerticalBox::Slot().AutoHeight()
					[SNew(SBox).Visibility_Lambda([this]() { return SelectedShopFilter == EFlickShopFilter::Collection && ShopDemo.GetOwnedCount() == 0 ? EVisibility::Visible : EVisibility::Collapsed; })
						[SNew(SFlickAngularBorder).BackgroundColor(Panel).AccentColor(Hairline).Padding(32).CutSize(10)
							[SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[ShopText(TEXT("YOUR DEMO COLLECTION STARTS HERE"), 20, Paper)]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 9)
								[ShopText(TEXT("Inspect an item and use demo credits to try the shop flow. Real inventory stays untouched."), 12, Muted)]
								+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 12)
								[SNew(SBox).WidthOverride(210)
									[SNew(SButton).Tag(TEXT("ShopBrowse")).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
										.OnClicked_Lambda([this]() { SelectedShopFilter = EFlickShopFilter::All; return FReply::Handled(); })
										[SNew(SFlickAngularBorder).BackgroundColor(Brand).AccentColor(Brand).CutSize(8).Padding(FMargin(20, 13))
											[ShopText(TEXT("BROWSE ITEMS"), 12, Ink)]]]]]]]]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)
			[SNew(SBox).HeightOverride(52)
				[SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(160)[Back]]
					+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(30, 0, 16, 0)
					[ShopText(TEXT("CONCEPT ITEMS  //  DEMO CREDITS HAVE NO REAL VALUE."), 9, Muted)]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[SNew(SButton).Tag(TEXT("ShopReset")).ButtonStyle(&TransparentButtonStyle).ContentPadding(FMargin(14, 10))
						.OnClicked_Lambda([this]() { ShopDemo.Reset(); ShopStatus.Reset(); return FReply::Handled(); })
						[ShopText(TEXT("RESET DEMO"), 10, Muted)]]]]]
		+ SOverlay::Slot()
		[SNew(SBox).Visibility_Lambda([this]() { return SelectedShopItem != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed; })[BuildShopDetails()]];
}

TSharedRef<SWidget> SFlickGameLayer::BuildShopDetails()
{
	const auto Item = [this]() -> const FFlickShopItem& { return FlickShopCatalog::GetItems()[FMath::Clamp(SelectedShopItem, 0, FlickShopCatalog::GetItems().Num() - 1)]; };
	SAssignNew(ShopDetailCloseButton, SButton).Tag(TEXT("ShopDetailClose")).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(FMargin(16, 10))
	 .OnClicked_Lambda([this]() { return CloseShopItem(); })[ShopText(TEXT("CLOSE  //  ESC"), 11, Paper)];
	return SNew(SOverlay)
	 + SOverlay::Slot()
	 [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).IsFocusable(false).OnClicked_Lambda([this]() { return CloseShopItem(); })
	  [SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(0, .003f, .008f, .9f))]]
	 + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(36)
	 [SNew(SBox).WidthOverride(1480).HeightOverride(780)
	  [SNew(SFlickAngularBorder).BackgroundColor(FLinearColor(.006f, .014f, .024f, 1)).AccentColor_Lambda([Item]() { return Item().Accent; }).CutSize(16).BorderWidth(1.5f).Padding(28)
	   [SNew(SVerticalBox)
	    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 22)
	    [SNew(SHorizontalBox)
	     + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[ShopText(TEXT("FLICK  //  ITEM INSPECTION"), 12, Muted)]
	     + SHorizontalBox::Slot().AutoWidth()[ShopDetailCloseButton.ToSharedRef()]]
	    + SVerticalBox::Slot().FillHeight(1)
	    [SNew(SHorizontalBox)
	     + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 28, 0)
	     [SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(.002f, .005f, .012f))
	      [SNew(SFlickShopArtwork).ItemIndex_Lambda([this]() { return SelectedShopItem; })]]
	     + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
	     [SNew(SBox).WidthOverride(430)
	      [SNew(SVerticalBox)
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(FString(Item().Rarity) + TEXT("  //  ") + Item().Type); }).Font(UiFont(11, true)).ColorAndOpacity_Lambda([Item]() { return Item().Accent; })]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 8)
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(Item().Name); }).Font(DisplayFont(43)).ColorAndOpacity(Paper).AutoWrapText(true)]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(Item().Tagline); }).Font(UiFont(15, true)).ColorAndOpacity(Paper)]
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(Item().Description); }).Font(UiFont(13)).ColorAndOpacity(Muted).AutoWrapText(true)]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 20)
	       [ShopText(TEXT("CONCEPT ART  //  COSMETIC ONLY\nNot an equippable item. No stat changes."), 10, Muted)]
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(ShopPrice(Item().Price)); }).Font(DisplayFont(32)).ColorAndOpacity(Paper)]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 14)
	       [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(TEXT("YOUR DEMO BALANCE  //  ") + ShopPrice(ShopDemo.GetCredits())); }).Font(UiFont(10, true)).ColorAndOpacity(Muted)]
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(SBox).IsEnabled_Lambda([this]() { return !ShopDemo.Owns(SelectedShopItem); })
	        [SNew(SButton).Tag(TEXT("ShopAcquire")).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(FMargin(18, 16)).OnClicked_Lambda([this]() { return AcquireShopItem(); })
	         [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ShopDemo.Owns(SelectedShopItem) ? TEXT("IN DEMO COLLECTION") : TEXT("GET ITEM  //  DEMO CREDITS")); }).Font(UiFont(13, true)).ColorAndOpacity(Brand).Justification(ETextJustify::Center)]]]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 12)
	       [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ShopStatus.IsEmpty() ? TEXT("No real money. No account or inventory changes.") : ShopStatus); }).Font(UiFont(11)).ColorAndOpacity(Cyan).AutoWrapText(true)]
	      ]]
	    ]
	    + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
	    [ShopText(TEXT("SHOP PREVIEW  //  Original cosmetic concepts. Purchases exist only in this demo session."), 10, Muted)]
	   ]]];
}
