# BOB arena workshop

High-detail Blender presentation for every BOB game mode. The source dimensions
match the authoritative C++ board exactly. The exporter repairs downward-facing
centre-ring faces and adds a hidden UCX compound floor with round pocket openings.
Unreal renders the artwork without collision, and uses the UCX floor through a
separate hidden component. C++ owns rails, pocket scoring and physics materials.
Pucks fall under gravity before being scored; the old full-board slab is disabled.

## Export the optimized source

`BobArena.blend` is the artist-authored optimized version (69,860 triangles,
down from 83,960). Edit this file directly; it is no longer procedurally rebuilt.

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --disable-autoexec --python 'AssetDevelopment/BOB Arena/scripts/export_bob_arena.py'
```

This writes `exports/SM_BobArena_HighDetail.fbx` and `exports/source_report.json`,
without replacing the Blender source. Collision has 137 convex support hulls,
not the full detail mesh. The old generation command delegates to
this exporter for compatibility. Close Unreal Editor and run
`update-flick-assets.cmd` to reimport changed assets, including this arena.

Settings > Lighting applies the shared gameplay preset to BOB too, including
ambient, fill, key, rim, team accents, direct light and highlight strength.

Blender uses metres; Unreal uses centimetres. Do not resize the export to change
gameplay. Gameplay dimensions remain owned by `AFlickBobArena` and
`FlickModeRules`.
