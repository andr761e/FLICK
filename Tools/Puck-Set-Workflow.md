# Puck set production and portrait workflow

Use this checklist when adding a new set. Retro Arcade is the worked example; do not copy its catalog index into another set.

## Inspect and preserve

- Inspect the cosmetic catalog, runtime mesh/material selection, replay cosmetics and SFlickPuckThumbnail before making changes.
- Keep existing skin indices stable. Add the new set for all intended archetypes and preserve saved loadouts.
- Preserve the gameplay collision cylinder, mass, friction, restitution and class size rules. Visual geometry must not introduce collision.
- Keep editable Blender sources, export scripts, reference art, manifests and renders under AssetDevelopment/Pucks/<Set>.
- Match the approved reference in a studio render first, then verify the real Unreal material. Offline renders alone do not prove the in-game result.

## Required UI images: five views for every archetype

The single source of truth is `Tools/FlickPuckPortraitViews.py`. Both the classic and Retro Arcade renderers use it. Use `apply_view`; do not invent a camera for a new skin. Positions below are in the normalized Blender portrait scene, with the puck grounded at z=0, front toward camera -Y, and its emblem upright in the Locker image. Keep the real class proportions; do not normalize every class to the same diameter or thickness.

| View | Camera XYZ | Look-at XYZ | Orthographic scale | PNG size | Transparent |
| --- | --- | --- | --- | --- | --- |
| Original | 1.15, -1.9, 1.75 | 0, 0, .05 | 1.48 | 512 x 384 | Yes |
| Locker | 0, -.95, 2.6 | 0, 0, .06 | 1.50 | 768 x 768 | No |
| Presentation | 1.05, -1.9, 1.85 | 0, 0, .065 | 1.55 | 1024 x 768 | No |
| Hero | .9, -1.9, 1.45 | 0, 0, .08 | 1.62 | 1024 x 768 | No |
| ProfileCutout | .65, -2, 1.18 | 0, 0, .07 | 1.42 | 1024 x 768 | Yes |

Render Standard, Heavy, Striker, Grippy, Slider, Blocker, Compact, Bouncer and Toppler: 45 images per full set. Preserve the naming convention `<Archetype>_<SkinIndex>.png` under AssetDevelopment/UI/PuckThumbnails, with view subfolders except Original. Import to corresponding `/Game/UI/PuckThumbnails` folders as `T_Puck_<Archetype>_<SkinIndex>`.

Match camera, target, model orientation, grounding, scale and framing to an existing skin of the SAME archetype. Keep consistent studio illumination, a dark reflective floor for opaque portraits, controlled neon bloom, readable dark faces and metallic highlights. Inspect at actual card size; tiny luminous details may need a simpler approved design. Do not bake labels, ownership ticks or UI frames into portraits.

## Retro Arcade portrait-only refresh

This path updates images without exporting geometry or rebuilding materials:

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python-exit-code 1 --python Tools/FlickRetroArcadePortraits.py
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' FLICK.uproject -run=pythonscript -script=Tools/Import-FlickRetroArcadePortraits.py -unattended -nop4 -nosplash
```

Use `-- --only Toppler` for a single archetype render. The importer requires all 45 existing files. The renderer checks source FBX hashes; the importer checks gameplay mesh/material hashes. Its report is Saved/RetroArcadePortraitImport.json. Full-set geometry/material import uses Tools/Import-FlickRetroArcade.py instead; do not run that for an image-only correction.

Blender requires the helper directory on sys.path. Always use `--python-exit-code 1`, because Blender otherwise may return success after a Python exception.

## Runtime evidence and checks

1. Capture each class in Customize with the new skin equipped. Compare its card beside an original finish of the same class, including emblem direction, angle, clipping and size. Toppler is useful for exposing angle/thickness discrepancies.
2. Capture Lineup (starting formation and available types), pre-match Class selection, Profile and Home. Check every portrait view used by those screens, hover/selected states and text. Home/locker live 3D previews also need separate verification.
3. Capture actual gameplay in Knockout and BOB, then Replay: high and low elevation, at least two orbit directions, normal and close zoom. Test lighting slider extremes and selection/impact states for washout or lost detail.
4. For mesh/material/catalog changes, build and run the relevant cosmetics, profile catalog, glare and replay tests. Package and run startup QA before distributing a package. For portrait-only edits, validate imports, inspect real menu captures and verify gameplay asset hashes; do not claim a new C++ build.
5. Store full screenshots, focused crops and a brief validation note in Deliverables. Label studio renders separately from actual gameplay captures. Never substitute generated imagery for evidence of implementation.

Example isolated capture (does not overwrite the normal player profile):

```powershell
./Tools/Capture-FlickUI.ps1 -Screen Customize -LockerCategory 11 -LockerItem 6 -Width 1920 -Height 1080 -CaptureDelaySeconds 4 -HideControlOverview
```

Category 11 and item 6 mean Toppler / Retro Arcade in the current catalog; confirm catalog values before reusing for a new set. Capture-FlickUI.ps1 lists supported screens and camera/skin options. Copy final captures out of Saved before cleaning it.
