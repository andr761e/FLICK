# Shared puck palettes

Emerald, Amethyst, Crimson and Amber are available for all nine puck types in
Customize, alongside Classic Blue and Classic Orange. Each type saves its skin
independently; validated player-state replication uses the same six stable IDs.

Each set has a folder under `AssetDevelopment/Pucks` with its `set.json` color
definition. They share the optimized meshes in `ClassicBlue/exports`; there are
no duplicated FBXs, textures or per-archetype material instances. Materials live under
`Content/Cosmetics/Pucks/<Set>`.

Existing `MI_Standard_*` material names are retained for compatibility and reused
by every archetype. They target the same seven named material families on each
mesh, including the appropriate type emblem. No assets need regenerating merely
to enable a palette on another archetype.

The instances inherit the original Blue material instances and override vector
colors only. Geometry, shader, roughness, metallic, anisotropy and gameplay
collision, mass and friction remain unchanged.
Team-indicator rings remain
independent of the cosmetic color.

To regenerate the material instances from the definitions, run in PowerShell:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$PWD\FLICK.uproject" -run=pythonscript "-script=$PWD\Tools\Build-FlickPuckPalettes.py" -unattended -nop4 -nullrhi
```

Accent colors are also defined in `FlickCosmeticCatalog::GetPuckSkinColor`, which
drives runtime highlights and UI previews. Keep those values synchronized with
`accent_color` when tuning a palette. Base color overrides are defined only in
the JSON files. The normal puck-update command remains unchanged.
