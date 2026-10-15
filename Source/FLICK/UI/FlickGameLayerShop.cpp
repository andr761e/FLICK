#include "UI/FlickGameLayerPrivate.h"
#include "UI/FlickShopArtwork.h"
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
	TSharedRef<SButton> Button = SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		.Cursor(EMouseCursor::Hand).OnClicked_Lambda([this, ItemIndex]() { return OpenShopItem(ItemIndex); });
	const TWeakPtr<SButton> WeakButton = Button;
	Button->SetContent(SNew(SFlickAngularBorder).BackgroundColor(FLinearColor(.008f, .016f, .027f, 1))
		.AccentColor_Lambda([this, WeakButton, ItemIndex]()
		{
			const auto Pinned = WeakButton.Pin();
			return Pinned.IsValid() && (Pinned->IsHovered() || Pinned->HasKeyboardFocus())
				? FlickShopCatalog::Find(ItemIndex)->Accent : Hairline;
		}).CutSize(9).BorderWidth(1.3f).Padding(1)
		[SNew(SVerticalBox)
		 + SVerticalBox::Slot().AutoHeight()
		 [SNew(SBox).HeightOverride(3)[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(Item.Accent)]]
		 + SVerticalBox::Slot().AutoHeight()
		 [SNew(SBox).HeightOverride(180)[SNew(SFlickShopArtwork).ItemIndex(ItemIndex)]]
		 + SVerticalBox::Slot().AutoHeight().Padding(14, 10, 14, 0)
		 [ShopText(Item.Type, 9, Muted)]
		 + SVerticalBox::Slot().AutoHeight().Padding(14, 4, 14, 9)
		 [ShopText(Item.Name, 17, Paper)]
		 + SVerticalBox::Slot().AutoHeight().Padding(14, 0, 14, 12)
		 [SNew(SHorizontalBox)
		  + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[ShopText(Item.Rarity, 9, Item.Accent)]
		  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		  [SNew(STextBlock).Font(UiFont(12, true)).ColorAndOpacity(Paper).Text_Lambda([this, ItemIndex]()
		   { return FText::FromString(ShopDemo.Owns(ItemIndex) ? TEXT("IN COLLECTION") : ShopPrice(FlickShopCatalog::Find(ItemIndex)->Price)); })]
		 ]
		]);
	return Button;
}

TSharedRef<SWidget> SFlickGameLayer::BuildItemShop()
{
	const auto FeaturedHeight = []()
	{
		FVector2D PhysicalSize(1600, 900);
		if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(PhysicalSize);
		return PhysicalSize.Y < 850.f ? 270.f : 334.f;
	};
	TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const auto Filter = static_cast<EFlickShopFilter>(Index);
		Tabs->AddSlot().AutoWidth().Padding(0, 0, 10, 0)
		[SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0)
		 .OnClicked_Lambda([this, Filter]() { SelectedShopFilter = Filter; ShopStatus.Reset(); return FReply::Handled(); })
		 [SNew(SFlickAngularBorder).CutSize(6).Padding(FMargin(18, 11))
		  .BackgroundColor_Lambda([this, Filter]() { return SelectedShopFilter == Filter ? Brand : Panel; })
		  .AccentColor_Lambda([this, Filter]() { return SelectedShopFilter == Filter ? Brand : Hairline; })
		  [SNew(STextBlock).Text(FText::FromString(FlickShopCatalog::GetFilterName(Filter))).Font(UiFont(11, true))
		   .ColorAndOpacity_Lambda([this, Filter]() { return SelectedShopFilter == Filter ? Ink : Paper; })]]];
	}
	TSharedRef<SWrapBox> Cards = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(14, 14));
	const TWeakPtr<SWrapBox> WeakCards = Cards;
	for (int32 Index = 0; Index < FlickShopCatalog::GetItems().Num(); ++Index)
	{
		Cards->AddSlot()
		[SNew(SBox).WidthOverride_Lambda([WeakCards]()
		 {
			const auto CardLayout = WeakCards.Pin();
			const float Width = CardLayout.IsValid() ? CardLayout->GetCachedGeometry().GetLocalSize().X : 0.f;
			const int32 Columns = Width > 0 && Width < 1320.f ? 3 : 6;
			return Width > 0 ? FMath::Max(180.f, (Width - (Columns - 1) * 14.f - 2.f) / Columns) : 285.f;
		 }).Visibility_Lambda([this, Index]() { return ShopDemo.Matches(Index, SelectedShopFilter) ? EVisibility::Visible : EVisibility::Collapsed; })
		 [BuildShopItemCard(Index)]];
	}

	return SNew(SOverlay)
	 + SOverlay::Slot()[SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(.002f, .006f, .011f, .94f))]
	 + SOverlay::Slot().VAlign(VAlign_Top).Padding(54, 26, 54, 0)
	 [SNew(SHorizontalBox)
	  + SHorizontalBox::Slot().FillWidth(1)
	  [SNew(SVerticalBox)
	   + SVerticalBox::Slot().AutoHeight()[ShopText(TEXT("FLICK  /  STYLE WITHOUT COMPROMISE"), 11, Brand)]
	   + SVerticalBox::Slot().AutoHeight().Padding(0, 4)[SNew(STextBlock).Text(FText::FromString(TEXT("ITEM SHOP"))).Font(DisplayFont(43)).ColorAndOpacity(Paper)]]
	  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18, 0)
	  [SNew(SFlickAngularBorder).BackgroundColor(Panel).AccentColor(Hairline).CutSize(8).Padding(FMargin(18, 10))
	   [SNew(SVerticalBox)
	    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)[ShopText(TEXT("DEMO BALANCE"), 9, Muted)]
	    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
	    [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ShopPrice(ShopDemo.GetCredits())); }).Font(UiFont(21, true)).ColorAndOpacity(Brand)]]]
	  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ShopText(TEXT("SHOP PREVIEW\nNO REAL PURCHASES"), 10, Muted)]
	 ]
	 + SOverlay::Slot().Padding(54, 120, 54, 92)
	 [SNew(SScrollBox) + SScrollBox::Slot()
	  [SNew(SVerticalBox)
	   + SVerticalBox::Slot().AutoHeight()[Tabs]
	   + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
	   [SNew(SBox).Visibility_Lambda([this]() { return SelectedShopFilter == EFlickShopFilter::All ? EVisibility::Visible : EVisibility::Collapsed; })
	    [SNew(SHorizontalBox)
	     + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 18, 0)
	     [SNew(SBox).HeightOverride_Lambda(FeaturedHeight)
	      [SNew(SFlickAngularBorder).BackgroundColor(FLinearColor(.003f, .009f, .017f)).AccentColor(Cyan).CutSize(12).BorderWidth(1.4f).Padding(2)
	       [SNew(SHorizontalBox)
	        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
	        [SNew(SBox).WidthOverride(430)
	         [SNew(SVerticalBox)
	          + SVerticalBox::Slot().AutoHeight().Padding(28, 0)[ShopText(TEXT("FEATURED FINISH  /  EXOTIC"), 10, Cyan)]
	          + SVerticalBox::Slot().AutoHeight().Padding(28, 9, 0, 0)[SNew(STextBlock).Text(FText::FromString(TEXT("ION SHIFT"))).Font(DisplayFont(57)).ColorAndOpacity(Paper)]
	          + SVerticalBox::Slot().AutoHeight().Padding(28, 5)[ShopText(TEXT("Precision, with a pulse."), 15, Paper)]
	          + SVerticalBox::Slot().AutoHeight().Padding(28, 7, 15, 12)
	          [SNew(STextBlock).Text(FText::FromString(TEXT("Obsidian. Titanium. Ice-cyan light.\nA new signature for the arena."))).Font(UiFont(12)).ColorAndOpacity(Muted)]
	          + SVerticalBox::Slot().AutoHeight().Padding(28, 0, 28, 0)
	          [MakeMenuButton(TEXT("INSPECT  /  900 CR"), FOnClicked::CreateLambda([this]() { return OpenShopItem(0); }), true, false, 46)]
	         ]]
	        + SHorizontalBox::Slot().FillWidth(1)[SNew(SFlickShopArtwork).ItemIndex(0).Cover(true)]
	       ]]]
	     + SHorizontalBox::Slot().AutoWidth()
	     [SNew(SBox).WidthOverride(420).HeightOverride_Lambda(FeaturedHeight)
	      [SNew(SButton).ButtonStyle(&TransparentButtonStyle).ContentPadding(0).OnClicked_Lambda([this]() { return OpenShopItem(3); })
	       [SNew(SFlickAngularBorder).BackgroundColor(Panel).AccentColor(FlickShopCatalog::Find(3)->Accent).CutSize(12).Padding(2)
	        [SNew(SVerticalBox)
	         + SVerticalBox::Slot().FillHeight(1)[SNew(SFlickShopArtwork).ItemIndex(3).Cover(true)]
	         + SVerticalBox::Slot().AutoHeight().Padding(18, 10, 18, 5)[ShopText(TEXT("SPOTLIGHT  /  LEGENDARY"), 9, FlickShopCatalog::Find(3)->Accent)]
	         + SVerticalBox::Slot().AutoHeight().Padding(18, 0, 18, 15)
	         [SNew(SHorizontalBox)
	          + SHorizontalBox::Slot().FillWidth(1)[ShopText(TEXT("SUPERNOVA"), 16, Paper)]
	          + SHorizontalBox::Slot().AutoWidth()[ShopText(TEXT("800 CR"), 12, Paper)]]
	        ]]]]
	    ]]
	   + SVerticalBox::Slot().AutoHeight().Padding(0, 22, 0, 12)
	   [SNew(SHorizontalBox)
	    + SHorizontalBox::Slot().FillWidth(1)
	    [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(SelectedShopFilter == EFlickShopFilter::All ? TEXT("CURATED FOR THE ARENA") : FlickShopCatalog::GetFilterName(SelectedShopFilter)); }).Font(UiFont(15, true)).ColorAndOpacity(Paper)]
	    + SHorizontalBox::Slot().AutoWidth()[ShopText(TEXT("COSMETICS ONLY  /  NO GAMEPLAY ADVANTAGE"), 10, Muted)]
	   ]
	   + SVerticalBox::Slot().AutoHeight()[Cards]
	   + SVerticalBox::Slot().AutoHeight()
	   [SNew(SBox).Visibility_Lambda([this]() { return SelectedShopFilter == EFlickShopFilter::Collection && ShopDemo.GetOwnedCount() == 0 ? EVisibility::Visible : EVisibility::Collapsed; })
	    [SNew(SFlickAngularBorder).BackgroundColor(Panel).AccentColor(Hairline).Padding(32).CutSize(10)
	     [SNew(SVerticalBox)
	      + SVerticalBox::Slot().AutoHeight()[ShopText(TEXT("YOUR DEMO COLLECTION STARTS HERE"), 20, Paper)]
	      + SVerticalBox::Slot().AutoHeight().Padding(0, 9)[ShopText(TEXT("Inspect an item and use demo credits to try the shop flow. Real inventory stays untouched."), 12, Muted)]
	      + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 12)
	      [SNew(SBox).WidthOverride(210)[MakeMenuButton(TEXT("BROWSE ITEMS"), FOnClicked::CreateLambda([this]() { SelectedShopFilter = EFlickShopFilter::All; return FReply::Handled(); }), true, false, 44)]]
	     ]]]
	  ]]
	 + SOverlay::Slot().VAlign(VAlign_Bottom)
	 [SNew(SBorder).BorderImage(WhiteBrush()).BorderBackgroundColor(FLinearColor(.003f, .01f, .017f, .98f)).Padding(FMargin(44, 18))
	  [SNew(SHorizontalBox)
	   + SHorizontalBox::Slot().AutoWidth()
	   [SNew(SBox).WidthOverride(160)[MakeMenuButton(TEXT("BACK"), FOnClicked::CreateLambda([this]()
	    { CloseShopItem(); if (GameMode.IsValid()) GameMode->CloseItemShop(); else RemotePartyScreen = EFlickFrontendScreen::MainMenu; return FReply::Handled(); }), false, false, 46)]]
	   + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(24, 0)
	   [ShopText(TEXT("CONCEPT ITEMS  /  Demo credits have no real value. Nothing here changes your loadout."), 10, Muted)]
	   + SHorizontalBox::Slot().AutoWidth()
	   [SNew(SBox).WidthOverride(190)[MakeMenuButton(TEXT("RESET DEMO"), FOnClicked::CreateLambda([this]() { ShopDemo.Reset(); ShopStatus.Reset(); return FReply::Handled(); }), false, false, 46)]]
	  ]]
	 + SOverlay::Slot()
	 [SNew(SBox).Visibility_Lambda([this]() { return SelectedShopItem != INDEX_NONE ? EVisibility::Visible : EVisibility::Collapsed; })[BuildShopDetails()]];
}

TSharedRef<SWidget> SFlickGameLayer::BuildShopDetails()
{
	const auto Item = [this]() -> const FFlickShopItem& { return FlickShopCatalog::GetItems()[FMath::Clamp(SelectedShopItem, 0, FlickShopCatalog::GetItems().Num() - 1)]; };
	SAssignNew(ShopDetailCloseButton, SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(FMargin(16, 10))
	 .OnClicked_Lambda([this]() { return CloseShopItem(); })[ShopText(TEXT("CLOSE  /  ESC"), 11, Paper)];
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
	     + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[ShopText(TEXT("FLICK  /  ITEM INSPECTION"), 12, Muted)]
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
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(FString(Item().Rarity) + TEXT("  /  ") + Item().Type); }).Font(UiFont(11, true)).ColorAndOpacity_Lambda([Item]() { return Item().Accent; })]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 8)
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(Item().Name); }).Font(DisplayFont(43)).ColorAndOpacity(Paper).AutoWrapText(true)]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(Item().Tagline); }).Font(UiFont(15, true)).ColorAndOpacity(Paper)]
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(Item().Description); }).Font(UiFont(13)).ColorAndOpacity(Muted).AutoWrapText(true)]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 20)
	       [ShopText(TEXT("CONCEPT ART  /  COSMETIC ONLY\nNot an equippable item. No stat changes."), 10, Muted)]
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(STextBlock).Text_Lambda([Item]() { return FText::FromString(ShopPrice(Item().Price)); }).Font(DisplayFont(32)).ColorAndOpacity(Paper)]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 5, 0, 14)
	       [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(TEXT("YOUR DEMO BALANCE  /  ") + ShopPrice(ShopDemo.GetCredits())); }).Font(UiFont(10, true)).ColorAndOpacity(Muted)]
	       + SVerticalBox::Slot().AutoHeight()
	       [SNew(SBox).IsEnabled_Lambda([this]() { return !ShopDemo.Owns(SelectedShopItem); })
	        [SNew(SButton).ButtonStyle(&CompactMenuButtonStyle).ContentPadding(FMargin(18, 16)).OnClicked_Lambda([this]() { return AcquireShopItem(); })
	         [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ShopDemo.Owns(SelectedShopItem) ? TEXT("IN DEMO COLLECTION") : TEXT("GET ITEM  /  DEMO CREDITS")); }).Font(UiFont(13, true)).ColorAndOpacity(Brand).Justification(ETextJustify::Center)]]]
	       + SVerticalBox::Slot().AutoHeight().Padding(0, 12)
	       [SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(ShopStatus.IsEmpty() ? TEXT("No real money. No account or inventory changes.") : ShopStatus); }).Font(UiFont(11)).ColorAndOpacity(Cyan).AutoWrapText(true)]
	      ]]
	    ]
	    + SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 0)
	    [ShopText(TEXT("SHOP PREVIEW  /  Original cosmetic concepts. Purchases exist only in this demo session."), 10, Muted)]
	   ]]];
}
