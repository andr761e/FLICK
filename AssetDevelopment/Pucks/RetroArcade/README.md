# Retro Arcade / all nine puck types

Production successor to standalone Neon Standard. Catalog index **6** remains stable for existing equipped/saved Standard skins; it is now named **RETRO ARCADE** and available for every archetype. The original six finishes keep their indices.

Reference: `Deliverables/Retro Arcade Set.png`. All nine use the approved solid-panel crown from `Deliverables/Neon-Crown-Concepts-2026-11-08/03-segmented-panels.png`. Heavy and Toppler receive vertical braces, Grippy ribbed grips, Blocker crown guards, Bouncer a rounded bumper, Slider glide seams and Striker a low-profile crown. Compact uses a small precision-ring emblem and smaller body. Every class has its reference emblem.

## Pipeline

For each of `Standard Heavy Striker Grippy Slider Blocker Compact Bouncer Toppler`:

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python AssetDevelopment/Pucks/RetroArcade/scripts/build_retro_arcade.py -- --archetype Heavy --render --portraits
```

Then run Unreal's Python commandlet with `Tools/Import-FlickRetroArcade.py`. The importer writes `/Game/Cosmetics/Pucks/RetroArcade/` and five portrait views per type to the existing `/Game/UI/PuckThumbnails/` paths.

The builder uses the existing radius/thickness multipliers and exports no collision. `mesh_cleanup.py` removes zero-area bevel triangles while preserving visible triangles, corner normals, UVs and colors. Historical standalone Neon source/assets remain as legacy material; runtime selection uses RetroArcade.

## Shared shading

Nine meshes share one PBR parent and ten material instances, without additional dynamic lights or texture masks. UV0 carries grain coordinates; UV1/UV2 pack canonical art-space position and the Standard-only central ring flag. The shader reverses Unreal's FBX V-coordinate flip before decoding this data, so all physical sizes retain the same lens/reflection treatment without duplicating material families per type.

Collision, mass, friction, restitution, center-of-mass rules and solver settings remain controlled by the unchanged gameplay cylinder. The material override exclusion protects the cyan/magenta/yellow composition from team palette overrides, including BOB and replays.

Validation and actual runtime captures: `Deliverables/Retro-Arcade-Set-2026-11-08/`.

## Portrait consistency and future sets

Follow [the puck-set workflow](../../../Tools/Puck-Set-Workflow.md) for all five shared camera presets, required runtime captures and validation. `Tools/FlickRetroArcadePortraits.py` rerenders only portraits from saved source scenes; `Tools/Import-FlickRetroArcadePortraits.py` refreshes only UI textures and verifies that gameplay assets are unchanged. Both the full builder and classic portrait renderer use `Tools/FlickPuckPortraitViews.py` to prevent camera drift.
