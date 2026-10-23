# Settings artwork // 2026-10-23

Generated with the built-in image-generation tool (not CLI). Both masters are 1672x941,
text-free illustrations. These are menu art, not screenshots or changes to game geometry.

- `SettingsBackdropV2.png`: settings background, referenced from the settings concept and
  actual Standard blue presentation render. Native controls/panels cover the left-hand area.
- `SettingsImpactV2.png`: contextual Shot Feel illustration, referenced from the actual
  Standard blue render. It illustrates contact feedback; it does not introduce an in-game effect.

Original outputs remain in the image-generation library. Selected files were copied into this
project. Unreal paths: `/Game/UI/MenuArtwork/T_Menu_SettingsBackdropV2` (max 2048) and
`/Game/UI/MenuArtwork/T_Menu_SettingsImpactV2` (max 1024), sRGB/BC7/UI/mips/streamable.
Existing `/Game/UI` cook coverage includes them. No runtime scene capture or new shader.

Reimport only these two assets (from the project root):

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$PWD/FLICK.uproject" -run=pythonscript "-script=$PWD/Tools/Import-FlickMenuArtwork.py" -FlickSettingsArtwork -unattended -nop4 -nullrhi -nosound -nosteam
```

## Backdrop prompt

Use case: stylized-concept. Asset type: finished text-free full-screen 16:9 background image for the FLICK settings menu, not a UI mockup. Input image 1 is a reference for lighting/composition only: the provided settings concept. Input image 2 is a reference for the actual Standard cyan puck design. Generate a premium real-time-game-style 3D arena background closely matching the concept's underlying scene: brushed graphite-metal arena surface, subtle circular white court rings, stadium tiers with soft amber lights and cool cyan edge strips, realistic reflections, shallow background depth of field. A single Standard cyan/silver/black puck with the reference's ring face emblem sits on the arena within a white circular ring on the far right. CRITICAL layout: puck centre at 87% canvas width and 56% height, entire puck between 79% and 96% canvas width, fully inside frame. Left 74% mostly clear darker reflective arena floor with subdued detail because native settings panels will cover it. One small amber circular arena switch above/left of the puck around 81% width 29% height. Reference palette and atmosphere, no new architecture. NO TEXT, NO logo, NO menus, NO UI panels, NO controls, NO typography, NO watermarks. The output is only reusable background art. Landscape 16:9 composition.

## Impact prompt

Use case: stylized-concept. Asset type: text-free small illustration in the ABOUT SHOT FEEL help card of FLICK settings, landscape 16:9. Use the referenced actual Standard puck as the identity/design reference. A single cyan/silver/charcoal Standard puck with circular ring emblem sliding across dark brushed arena metal, very slightly tilted in the instant it makes contact with a low metallic edge at bottom right. A small restrained burst of warm amber contact sparks, a subtle cyan highlight, believable metallic reflections and premium realistic 3D game look. Close-up three-quarter view, entire puck visible centred slightly left; impact sparks on the right. Sharp readable silhouette at thumbnail size. Dark atmospheric stadium background with soft amber/cyan bokeh. Match the realistic premium FLICK arena style, no oversized explosion, no extra pucks. No text, no logo, no UI, no border, no watermark.
