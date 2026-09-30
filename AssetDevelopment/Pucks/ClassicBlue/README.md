# Classic Blue puck pipeline

This is the authoritative source tree for the nine pucks used in the Switchyard
test arena. Blue and orange share geometry; Unreal swaps only the two emissive
material families for the Classic Orange skin, authored separately in
`../ClassicOrange/set.json`. Skins are independent of team.

## Layout

- `source/standard/` — Standard puck Blender source, GLB and generator.
- `source/archetypes/models/` — source GLBs for the other eight puck types.
- `source/archetypes/generators/` — standalone generators for those GLBs.
- `scripts/` — deterministic Blender-to-Unreal FBX exporters.
- `exports/` — the nine Unreal-ready FBX files consumed by the import tools.
- `manifests/` — dimensions, material slots and validation results.
- `previews/` — current high-detail review images only.

The editable art is separate from Unreal's imported `.uasset` files under
`Content/TestArena/Pucks`. None of these meshes owns gameplay collision.

## Rebuild exports

### Update all pucks from the current nine FBX exports

Close Unreal Editor and run `.\update-flick-pucks.cmd` from the project root.
The command reads only the nine base files in `exports/`, reimports their existing
Unreal meshes in place and refreshes the Classic Orange materials. It does
not generate player-number variants, require Blender, modify the base FBXs or
import arenas/stadiums. Every player shares these nine meshes and can equip
either skin on either team from the locker. Set `FLICK_UNREAL_EDITOR`
if Unreal is installed elsewhere.

Import reports are written to `Saved/` and the commandlet log is
`Saved/Logs/FlickPuckUpdate.log`.

From the project root with Blender 5.2:

The two base exporters below rebuild from the older editable source models and
overwrite base FBX files. Do not run them when updating externally supplied FBX
replacements; use `update-flick-pucks.cmd` instead.

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/ClassicBlue/scripts/export_standard_unreal.py
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/ClassicBlue/scripts/export_archetypes_unreal.py
```

To update the in-game puck, arena, and stadium assets together after replacing
their FBX exports, close Unreal Editor and run from the project root:

```powershell
.\update-flick-assets.cmd
```

The command reimports the existing `.uasset` meshes in place, reapplies their
approved materials, validates dimensions/material slots, and writes reports to
`Saved/`. Commit the changed FBX source files and `Content/TestArena` `.uasset`
files so other machines receive both the editable exports and playable assets.

The exporters validate dimensions and preserve seven material sections:
graphite housing, machined highlights, gasket/sockets, skin diffuser, brushed
silver, recessed titanium and the type emblem.

## Gameplay boundary

The art keeps the established puck diameters and visual-height variation. The
existing primitive bodies remain authoritative for size, collision, mass,
friction, restitution, impulses and elimination. A failed asset load falls back
to the procedural runtime presentation without altering gameplay.
