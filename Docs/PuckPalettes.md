# Standard puck palettes

Emerald, Amethyst, Crimson and Amber are available in Customize > Standard Puck.
Only Standard has these sets; the other eight puck types retain Classic Blue and
Classic Orange.

Each set has a folder under `AssetDevelopment/Pucks` with its `set.json` color
definition. They share `ClassicBlue/exports/Standard.fbx`; there are no duplicated
FBXs or new textures. Generated material instances live under
`Content/Cosmetics/Pucks/<Set>`.

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
