# Knockout arena material and lighting refinement

2026-10-23. Reference:
`Deliverables/UI-Visual-Upgrade-2026-10-22/Concepts/11-arena-no-menu.png`.

## Scope

Presentation only, shared by 1v1/2v2/3v3. Existing arena dimensions, meshes,
collision, physical materials, puck designs, switch colours, placements and
gameplay logic are retained. The generated reference's distorted centre circle
and bent sockets are not copied: this pass targets its finish and illumination.
BOB's presentation materials and lighting branch are not changed.

## Changes

- Deeper blue gunmetal reflectance with less neutral cubemap wash. This is paired
  with actual coloured reflected sources, not simply a darker flat albedo.
- Opt-in `DeckRelief` on the existing architectural material: rotated/domain-warped
  fine surface normals and roughness variation break up fixture reflections.
  Derivative-based footprint filtering suppresses subpixel detail. The relief
  exists only in the shading normal; it never displaces vertices or collision.
- Narrower, separated cyan/amber softboxes replace the broad white ceiling-like
  reflection. Existing four stadium fixtures now sit above the playing plane,
  with a smaller source area and bounded range, so they can reflect on the deck.
- Darker, more reflective concourse metal, softer amber sidewall emission, and
  reduced local rim-spill power. The integrated, filtered cyan lens/gasket shader
  and its stable reflection path are retained; no overlapping rim meshes added.

## Tuning and performance boundaries

- `AFlickTestArena`: `OneVsOneDeckTint`, `OneVsOneDeckMetallic`,
  `OneVsOneDeckRoughness`, and `DeckFinishRelief` (default .16 cm shader relief).
- `AFlickGameMode`: existing ambient, fill and softbox multipliers; base sky
  multiplier is now .65. Local saved lighting controls remain in effect and are
  not overwritten.
- `FlickArenaLighting::FParameters`: source dimensions/colours, warm fixture and
  rim-spill powers. Dimensions and powers still scale with board size.
- No extra light actors, shadow passes, textures, live captures, reflection samples
  or resolution increase. Shader ALU and light coverage changed, so this is not
  presented as an FPS improvement or a zero-cost change; no new benchmark claimed.
- Architectural shader regeneration: `Tools/Build-FlickArchitecturalMaterial.py`.
  Only `Content/TestArena/Arena/M_ArchitecturalMetal.uasset` is rebuilt. Its original
  asset is backed up at `Saved/Backups/ArenaFinish20261023/M_ArchitecturalMetal-before.uasset`.
  The new relief defaults to zero, so other users of this parent do not opt in.

## Visual checks

Actual DX12 viewport captures, same seeded 1v1 board and unchanged camera:

- Before: `Saved/UICaptures/20261023-151010-6fdf9e/match-1920x1080.png`.
- Final: `Saved/UICaptures/20261023-153022-6441e0/match-1920x1080.png`.
- 3v3 at 720p, four consecutive frames:
  `Saved/UICaptures/20261023-153137-7f56fd/match-1280x720/`.

An initial regular value-noise finish looked tiled under strip reflections; it
was replaced with filtered, warped machining coordinates. A low stadium-source
placement produced oversized floor hotspots; final sources were raised and
narrowed. Final captures were inspected with no material compile errors or
fallback-material warnings. The short static frame sequence is a visual spot
check, not a guarantee against every possible temporal artifact during play.

Delivered images: `Deliverables/Arena-Material-Lighting-2026-10-23/`.

## Regression checks

Final Editor and standalone `FLICK Win64 Development` builds both succeeded:
`Saved/Logs/ArenaFinishEditorBuildFinal.log` and `ArenaFinishGameBuildFinal.log`.
Material authoring succeeded in `Saved/Logs/ArenaFinishMaterialFinal.log`, and
the final real viewport captures compiled/rendered the new material without a
fallback shader. The material's SHA-256 is
`97096F834465544D47578979E07F912F53750E590574C68C599C796689CD48BB`.

Final report: `Saved/Automation/ArenaFinishFinal20261023/index.json`.
105 passed (103 successes plus 2 warning results), 7 baseline-failing test paths,
0 not run. New relief routing, source colours/aspect, and above-plane fixture
placement assertions pass. `FLICK.Arena.ConceptPresentation`,
`FLICK.Visuals.KnockoutLightingTheme`, `FLICK.Physics.RenderRateIndependence`, and
BOB presentation/collision/resolution tests pass. The physics test exercises
40/60/120/144/240/400 FPS and variable frame intervals.

Remaining failing paths are `FLICK.Cosmetics.StarterSkins`,
`FLICK.Visuals.KnockoutArenaTheme`, `FLICK.Visuals.MenuArenaIsolation`,
`FLICK.Visuals.MenuPresentationIsolation`, `FLICK.Visuals.OneVsOneCameraIsolation`,
`FLICK.Visuals.TestArenaExposure`, and `FLICK.Visuals.WorkshopArena`. They were
already failing before this pass. The stadium-emission expectation was updated
specifically to the new .40 warm multiplier; unrelated failing assertions were
not suppressed or removed.

No fresh packaged cook/stage or live Steam/private-match test is claimed.
