# Cosmetic collections

Three freely owned, mix-and-match collections are available under Profile > Customize.
All previous items retain their save indices. None, Basic Drop and Basic Burst remain the defaults.

| Collection | Trail | Spawn | Knockout | Banner / frame | Tags |
| --- | --- | --- | --- | --- | --- |
| Cryo Circuit | Cryo Ribbon: tapered twin ice filaments | Cryo Lock: scanner rings and orbiting shards | Cryo Shatter: crystalline fan and frost rings | Cryo Circuit | Ice in My Veins; Cold Calculation |
| Solar Forge | Solar Cinders: molten ribbon with a white-hot core and drifting cinders | Solar Flare: ascending ember crown and expanding halos | Solar Nova: bright core, golden shock rings and embers | Solar Forge | Forged to Win; Heat Check |
| Phase Rift | Phase Stream: interwoven cyan/violet ribbons | Phase Gate: counter-rotating rings and contracting spiral | Phase Collapse: inward spiral followed by an aftershock | Phase Rift | Phase Shift; Beyond the Rim |

## Implementation and tuning

- `Core/FlickCosmeticCatalog.cpp` owns the stable item indices, descriptions and collection colours.
- `Pieces/FlickPieceCosmeticTrails.cpp` owns ribbon width, offset, history and emission. Trails are limited to 28 time-aged samples / 0.62 seconds / 300 world units. Stable-size mesh buffers are updated rather than recreated. One presentation mesh per equipped puck; no additional trail actors.
- `Feedback/FlickWorldFeedback.cpp` owns arrival/elimination choreography, lifetimes, ring radii and brightness. Extra geometry is allocated only for the new collection styles. All components are non-colliding, non-overlapping, non-shadow-casting and do not affect navigation. Effects never apply forces or change elimination timing. `FlickCosmeticMaterial.h` reuses the existing always-cooked arena emissive shader with independent MIDs and zero render-layer offset; it adds no shader assets and does not edit arena materials or lighting.
- `UI/FlickCosmeticWidgets.h` draws code-native avatar frames and effect swatches. Avatar centres remain unobstructed. The locker has tooltips describing each effect and wrapped labels for longer names. Effect cards fit six items in two rows; longer identity collections initially scroll to the equipped item.
- Existing effect replication and profile-save flow are reused. Cosmetic colours are independent of gameplay team ownership rings.
- Raw banner PNGs are staged by the existing `Content/UI/Banners/*.png` runtime dependency.

## Original banner assets

Created with the built-in image-generation tool. Kept at their original generated resolution; no third-party assets or model files were added.

- `Content/UI/Banners/CryoCircuit.png`
- `Content/UI/Banners/SolarForge.png`
- `Content/UI/Banners/PhaseRift.png`

### Exact prompt set

#### Cryo Circuit

Create a single original premium competitive sports game player banner BACKGROUND ONLY, no text, no lettering, no logo, no UI frame, no mockup, no puck. Ultra-wide horizontal 3:1 composition. Full bleed edge to edge. Left 65 percent is deep dark graphite with extremely subtle material detail and generous quiet negative space for a player avatar and readable white name/tag that will be added in game. Right third is the hero artwork, high quality dimensional materials, studio specular highlights, restrained light bloom, crisp authored details. Refined futuristic athletics, not generic sci-fi wallpaper. CRYO CIRCUIT: black brushed titanium with a flowing arc of frosted crystalline cyan and icy white energy, faceted sapphire glass fins and fine engraved silver circuit traces, bright cyan concentrated at right edge, elegant sweeping speed geometry.

#### Solar Forge

Create a single original premium competitive sports game player banner BACKGROUND ONLY, no text, no lettering, no logo, no UI frame, no mockup, no puck. Ultra-wide horizontal 3:1 composition. Full bleed edge to edge. Left 65 percent is deep dark graphite with extremely subtle material detail and generous quiet negative space for a player avatar and readable white name/tag that will be added in game. Right third is the hero artwork, high quality dimensional materials, studio specular highlights, restrained light bloom, crisp authored details. Refined futuristic athletics, not generic sci-fi wallpaper. SOLAR FORGE: charcoal-black hammered titanium with sculpted copper and champagne-gold aerodynamic ribbons, molten amber light glowing inside fine ceramic fractures, a small dramatic warm core on the right, sophisticated metallic finish, controlled embers.

#### Phase Rift

Create a single original premium competitive sports game player banner BACKGROUND ONLY, no text, no lettering, no logo, no UI frame, no mockup, no puck. Ultra-wide horizontal 3:1 composition. Full bleed edge to edge. Left 65 percent is deep dark graphite with extremely subtle material detail and generous quiet negative space for a player avatar and readable white name/tag that will be added in game. Right third is the hero artwork, high quality dimensional materials, studio specular highlights, restrained light bloom, crisp authored details. Refined futuristic athletics, not generic sci-fi wallpaper. PHASE RIFT: obsidian carbon composite with a striking dimensional ribbon of luminous amethyst glass, translucent violet and magenta refracted bands, a few pale cyan spectral highlights, curved orbital geometry on the right, deep black quiet left.

## Verification

Automation: `FLICK.Profile.CosmeticCatalog`, `FLICK.Cosmetics.CollectionPresentation`, and the existing `FLICK.Cosmetics.InspectionAndOwnership` regression. All three passed on 2026-10-04 after a successful FLICKEditor Development build. The engine logs two pre-existing frame-zero automation condition errors during startup, before the selected tests; the three test result records report Success and the process exits with code 0.

Example viewport capture: `Tools/Capture-FlickUI.ps1 -Screen Customize -LockerCategory 12 -LockerItem 5 -CaptureDelaySeconds 2`.

The final UI build succeeded. Reviewed actual Unreal viewport captures at 1600x900 (ribbon colours, arrivals, knockouts, banners and borders) and 1280x720 (banner tags and live profile preview). Final examples are under `Saved/UICaptures/20261004-131239-970f77`, `20261004-131257-6ea926`, and `20261004-131314-37951d`. These runs use isolated profiles and do not change the player's normal saved cosmetic selections. No packaged build or live multiplayer session was exercised.
