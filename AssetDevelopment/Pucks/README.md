# Blue and orange puck workshop

Prototype 3D assets updated from the blue and orange reference sheets supplied in the chat.
The reference is interpreted as satin silver shoulders, graphite side armor,
team-colored segmented lights, dark center inserts and distinct class emblems.
Toppler has the revised arrow/triangle/base symbol and side supports; Bouncer has
a flat bounce emblem instead of a dome; Slider has three filled chevrons. The
shield, mountain and dumbbell use filled inlays matching the updated sheets.
The original uploaded image is not stored here; no local attachment path was available.

## Open and review

- `PuckWorkshop.blend`: both teams side by side in BLUE TEAM / ORANGE TEAM collections.
- `renders/blue_team_sheet.png` and `renders/orange_team_sheet.png`: actual Cycles renders.
- `exports/<Class>.fbx` and `.glb`: blue variants, preserving the original file paths.
- `exports/Orange/<Class>.fbx` and `.glb`: orange variants.
- `dimensions.json`: source dimensions, generated bounds and triangle counts.
- `scripts/generate_pucks.py`: reproducible Blender generator; no add-ons required.
- `scripts/validate_workshop.py` and `validation.json`: saved-scene and export checks.
- `renders/standard_blue_detail.png` and `renders/heavy_orange_detail.png`: close-up renders.
- `scripts/render_details.py`: regenerates close-ups from the saved workshop.

The detail pass adds machined shoulder grooves, lens surrounds and gaskets,
service plates, chamfered side armor, side fasteners, vents, small status lights,
layered braces with grip treads, inset bevels, lower bumper seams, radial shoulder
breaks, and micro-fasteners. Blender materials include procedural circular metal
grain and subtle surface roughness. Both teams share these construction details.

The 18 puck objects share exactly nine mesh datablocks. Each orange object uses
the blue object's mesh and overrides only its team material via an object-linked
material slot. Geometry edits therefore propagate to both teams. The other five
materials are shared. Individual exports are standalone copies for convenience;
a game integration should reuse one mesh per class with team material instances.
The previous workshop and generator were copied into `backups/` before this update.

Run from the project root with Blender 5.2:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/scripts/generate_pucks.py
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background AssetDevelopment/Pucks/PuckWorkshop.blend --python-exit-code 2 --python AssetDevelopment/Pucks/scripts/validate_workshop.py
```

Regeneration replaces the workshop and exports. Keep manual art revisions in a
separate `.blend` file. The script clears its active Blender scene, so run it in
the background command above, not inside an unsaved working scene.

## Dimensions and integration boundary

The generator reads the base radius/thickness from `FlickGameMode.h` and the class
multipliers from `FlickPieceArchetypeRules.cpp`. Blender coordinates are meters;
game source values are centimeters. Each exported mesh has its origin at the
center of the existing collider, with Z up in the Blender/FBX authoring scene.
glTF export performs its standard axis conversion. Studio positioning is applied
only after exporting. Visual bounds are checked against the source dimensions.

| Class | Radius (cm) | Thickness (cm) |
|---|---:|---:|
| Standard | 45 | 20 |
| Toppler | 45 | 27 |
| Bouncer | 43.2 | 18 |
| Compact | 36 | 18 |
| Blocker | 53.1 | 18.4 |
| Slider | 45 | 20 |
| Grippy | 45 | 20 |
| Striker | 41.4 | 16.4 |
| Heavy | 48.6 | 23.6 |

Diameter and gameplay dimensions take precedence over the reference. Exported
vertices use art-only height multipliers to create a stronger silhouette range;
the collider thickness, mass and archetype behavior remain unchanged.
The divider test arena now uses these visual meshes over the existing physics
bodies, with shared blue/orange materials and the original tuned dimensions.
Other arenas retain their existing visuals. See [the integration guide](../../Docs/TEST_ARENA_PUCKS.md).

## Prototype limitations

These are detailed review meshes with applied bevels, not optimized shipping
assets. They need LODs or a lower-detail bake, UVs and authored surface maps before
production use. Procedural grain and bump are available in the Blender scene;
they are not baked into portable texture maps. Exported materials retain their
basic PBR values but need texture baking to reproduce the workshop's microdetail.
FBX material transfer is limited; recreate the six named
material slots in Unreal, especially team emission. GLB preserves more of the
PBR description. The dedicated `exports/Unreal` FBX set has been imported and
checked in Unreal; it includes planar shading UVs and six native material slots.

The studio lights, labels and floor are review-only and are excluded from exports.
