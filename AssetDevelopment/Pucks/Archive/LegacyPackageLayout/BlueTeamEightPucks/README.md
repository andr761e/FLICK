# Blue Team — eight additional pucks

Based on your attached `create_puck_fixed_v3(1).py` Standard puck script. Original graphite, brushed
silver, cyan emission strengths (5 for strips / 2 for symbols), and studio
lighting are retained. The Blender 5 compositor compatibility fix is included.
Standard is not rebuilt by this package.

## Generate an asset in Blender

1. Extract the ZIP to a writable folder.
2. Open Blender → Scripting → Text Editor → Open.
3. Select the corresponding `scripts/create_<name>.py` file.
4. Click Run Script, or press Alt-P while the pointer is in the Text Editor.
5. The script creates a new scene and exports both `<Name>.blend` and `<Name>.glb`.
6. Press F12 for the final camera render. Save the Render Result separately.

Output folder selection follows your supplied script: the current saved Blender
file folder first, then the script folder, then a writable Documents fallback.
The console prints the output paths.

Each script is completely standalone. No shared Python module or external
texture is needed. Run only the scripts for the assets you want. Existing
scenes are retained. The active scene switches to the newly created puck.
Rerunning a script creates another scene and saves its named Blender file again.

The viewport starts in Material Preview, matching the first Standard script.
Rendered mode and F12 show the scene lighting and material response. The glow
compositor is configured for the final render. This package keeps the original
look rather than the stronger lighting and shinier metal of the v2 script.

## Dimensions

Meshes use meters, with Z up and the origin at the center of the visible base.
The dimension sheet's **visible height** determines the actual visual mesh.

| Puck | Diameter | Visible mesh height | Physics cylinder height |
|---|---:|---:|---:|
| Toppler | 90.3 cm | 30.7 cm | 27.0 cm |
| Bouncer | 86.5 cm | 16.1 cm | 18.0 cm |
| Compact | 71.9 cm | 14.7 cm | 18.0 cm |
| Blocker | 106.1 cm | 16.5 cm | 18.4 cm |
| Slider | 89.9 cm | 18.3 cm | 20.0 cm |
| Grippy | 90.3 cm | 20.9 cm | 20.0 cm |
| Striker | 82.7 cm | 13.4 cm | 16.4 cm |
| Heavy | 97.5 cm | 27.3 cm | 23.6 cm |

Every Blender scene has a separate, hidden `COLLISION` collection containing a
64-sided convex cylinder named `UCX_<Name>_00`. It shares the visible diameter
and is centered vertically on the visible mesh. It may therefore extend above
or below a shorter visible mesh. Physics height is also stored on the root
object. This is an optional helper; no rigid-body simulation is enabled.
Set `CREATE_COLLISION_PROXY = False` near the start of a script to omit it.
The collision helper is excluded from the visual GLB files.

## Individual designs

- **Toppler:** tall housing, reinforced vertical braces, layered upper brow,
  upward chevron, lower triangular arrowhead, and short dash.
- **Bouncer:** interlocking impact panels, rounded lower bumper, center ball,
  and two curved rebound strokes.
- **Compact:** smaller body, tapered armor wedges, and a thick circular symbol.
- **Blocker:** broad silver crown, wide shield panels, nested hexagonal symbol,
  dark front rim inserts, and small cyan side indicators.
- **Slider:** swept side panels, three rightward chevrons, dark front rim,
  and three small front telemetry lights.
- **Grippy:** reinforced corner braces with extra bolts, stepped side armor,
  and a jagged mountain-shaped grip symbol.
- **Striker:** low body, angled impact panels, reinforced rim pads, target ring,
  center dot, and four crosshair marks.
- **Heavy:** tall, wider housing, broad metallic braces, layered lower armor,
  reinforced rim pads, and a dumbbell symbol.

## Ready-made meshes

`models/*.glb` contains the eight visual models with separate named mesh parts
and basic PBR/emissive materials. In Blender use File → Import → glTF 2.0.
Blender converts the GLB's Y-up coordinates to Z up automatically.

Use the scripts for the full procedural brushed-metal materials, studio lights,
camera, compositor, and optional collision helper. These Blender-specific
features are not included in the GLBs. Visual GLBs contain dimensional metadata.

## Preview and validation

`Puck_Collection_Preview.png` compares all eight meshes at the same scale.
The previews are independent software renders of the actual generated mesh;
they verify silhouettes, symbols, and overlaps. They are not Blender renders
and approximate the materials without the full Blender shaders.

All eight scripts passed Python syntax checks. All meshes passed checks for
exact dimensions, maximum radius, finite coordinates, unit normals, triangle
winding, and GLB buffer integrity. Per-asset counts are in `validation.json`.

Blender is unavailable in the generation environment, so these new scripts
and the final Cycles renders could not be executed here. They reuse the
geometry and material APIs from the Standard script you successfully ran.
The single-view references do not specify the underside or hidden back details;
those details are inferred. These are detailed visual models, without baked
textures, UV unwrapping, or game LODs. Unreal collision import configuration
and final gameplay integration remain separate from creating these assets.
