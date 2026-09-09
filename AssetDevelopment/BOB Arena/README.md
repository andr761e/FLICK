# BOB arena workshop

High-detail Blender presentation for every BOB game mode. The source dimensions
match the authoritative C++ board exactly. Unreal keeps the existing procedural
board, rails, pocket detection, and physics materials as invisible collision; the
imported mesh is presentation-only.

## Rebuild

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python 'AssetDevelopment/BOB Arena/scripts/generate_bob_arena.py'
```

This writes `BobArena.blend`, `exports/SM_BobArena_HighDetail.fbx`, a preview,
and `dimensions.json`. Close Unreal Editor and run `update-flick-assets.cmd` to
reimport all authored presentation assets, including this arena.

Blender uses metres; Unreal uses centimetres. Do not resize the export to change
gameplay. Gameplay dimensions remain owned by `AFlickBobArena` and
`FlickModeRules`.
