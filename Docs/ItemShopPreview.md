# Item shop preview

The native Slate storefront replaces the original three placeholder panels.
It uses six generated product renders, a featured finish, an effect spotlight,
category filters, a demo collection and a large item-inspection overlay.

## Scope and safety

- Every product is fictional concept art, not an equippable cosmetic.
- The header, detail view and footer identify the preview/demo economy.
- A UI session starts with 2000 demo credits. Successful acquisitions deduct the
  displayed price exactly once; duplicates and insufficient balances never charge.
- Ownership affects only the demo collection. It is not saved or replicated and
  never calls inventory, account, loadout or payment code.
- Reset Demo restores the demo balance and clears only demo ownership.
- Closing details returns keyboard focus; Escape/gamepad-back closes the detail
  view before leaving the storefront. Item cards are keyboard-focusable buttons.
- No artificial countdown, false discount, real-money offer or backend is added.

Catalog/prices/copy: `Source/FLICK/Core/FlickShopCatalog.cpp`.
Demo transaction rules: `FFlickShopDemoState` in the same module.
UI: `Source/FLICK/UI/FlickGameLayerShop.cpp`.

## Artwork

Generated using the built-in image-generation tool, one call per product:
Ion Shift, Solar Wake, Zero Point, Supernova, Nightfall and Prism Halo.
Final PNGs: `AssetDevelopment/UI/Shop/exports/`.
Exact prompts: `AssetDevelopment/UI/Shop/ShopArtwork.prompts.md`.

`Scripts/ImportShopArtwork.py` imports `/Game/UI/Shop/` with BC7 compression,
UI texture grouping, mip filtering and clamped sampling. The existing `/Game/UI`
always-cook rule includes them in packaged builds. `SFlickShopArtwork` loads on
its first visible tick, retains a strong reference and disables ticks for static
cards. It does not flush the renderer or repeatedly load images during paint.
Images are aspect-fit in cards/details; the featured image uses an explicit
aspect-fill crop. Typography and all pricing remain native Slate, not baked art.

## Verification

Build `FLICKEditor`, import the artwork, then run `Automation RunTests FLICK.Shop`.
Capture the main storefront with `Tools/Capture-FlickUI.ps1 -Screen Shop`.
Use `-ShopItem 0` (0..5) to inspect a product, `-ShopFilter 2` for effects,
`-ShopFilter 4` for an empty collection, or add `-ShopOwnedPreview` to inspect the
owned/collection states. Capture profiles remain isolated from the normal user.
