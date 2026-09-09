# Current high-detail puck pipeline

This is the authoritative source tree for the nine pucks used in the Switchyard
test arena. Blue and orange share geometry; Unreal swaps only the two emissive
material families for the orange team.

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

From the project root with Blender 5.2:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/HighDetail/scripts/export_standard_unreal.py
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/HighDetail/scripts/export_archetypes_unreal.py
```

Then run the Unreal Python importers:

- `Tools/Import-FlickBlueStandardPrototype.py`
- `Tools/Import-FlickHighDetailPucks.py`

The exporters validate dimensions and preserve seven material sections:
graphite housing, machined highlights, gasket/sockets, team diffuser, brushed
silver, recessed titanium and the team emblem.

## Gameplay boundary

The art keeps the established puck diameters and visual-height variation. The
existing primitive bodies remain authoritative for size, collision, mass,
friction, restitution, impulses and elimination. A failed asset load falls back
to the procedural runtime presentation without altering gameplay.
