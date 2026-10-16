# Venue geometry optimization — 2026-10-16

Scope: the six modular Knockout arena meshes shared by 1v1, 2v2 and 3v3; the two Knockout stadium meshes; and the two BOB stadium meshes.

**The BOB playing arena and all pucks are excluded.** SHA-256 verification confirmed all 172 protected files unchanged, including existing material assets.

## Method and limits

Merged disjoint, coplanar face pairs without crossing material, UV or sharp-edge boundaries; welded exactly coincident vertices. Retained circular segments, bevel profiles, positions, material slots and authored corner normals (apart from Blender normal-storage quantization). No gameplay, collision, lighting or material parameters changed.

Each candidate passed bidirectional, per-material vertex/triangle-interior surface checks with maximum measured distance below 0.005 mm. Bounds remained unchanged within floating-point import precision. Unsafe broad dissolve/decimation candidates were rejected.

FBX arrays also use lossless zlib level 9 instead of level 1. This reduces source-file storage, not GPU work. Native Blender files remain compressed. GLB previews are replaced only when smaller.

Unreal triangulates polygons: fewer Blender faces do **not** necessarily mean fewer render triangles. The modest actual render reduction is listed separately. No FPS improvement is claimed. Original Unreal packages were retained where identical render counts and a larger reimport offered no benefit.

## File sizes

All values below are exact bytes. Totals count each shared asset once, not once per arena size.

| Group / format | Before | After | Saved | Reduction |
|---|---:|---:|---:|---:|
| Knockout 1v1 / 2v2 / 3v3 / FBX | 4,699,656 | 4,452,456 | 247,200 | 5.26% |
| Knockout 1v1 / 2v2 / 3v3 / GLB | 5,228,044 | 5,228,044 | 0 | 0.00% |
| Knockout 1v1 / 2v2 / 3v3 / BLEND | 7,285,957 | 7,297,409 | -11,452 | -0.16% |
| Knockout 1v1 / 2v2 / 3v3 / Unreal editor assets | 9,334,036 | 9,334,036 | 0 | 0.00% |
| Knockout 1v1 / 2v2 / 3v3 / ALL FILES | 26,547,693 | 26,311,945 | 235,748 | 0.89% |
| Knockout stadium / FBX | 3,225,192 | 3,393,752 | -168,560 | -5.23% |
| Knockout stadium / GLB | 3,985,324 | 3,985,324 | 0 | 0.00% |
| Knockout stadium / BLEND | 10,119,286 | 9,852,730 | 266,556 | 2.63% |
| Knockout stadium / Unreal editor assets | 3,913,122 | 3,901,847 | 11,275 | 0.29% |
| Knockout stadium / ALL FILES | 21,242,924 | 21,133,653 | 109,271 | 0.51% |
| BOB stadium / FBX | 5,875,144 | 5,401,128 | 474,016 | 8.07% |
| BOB stadium / GLB | 20,750,104 | 20,598,432 | 151,672 | 0.73% |
| BOB stadium / BLEND | 12,242,061 | 12,210,856 | 31,205 | 0.25% |
| BOB stadium / Unreal editor assets | 7,423,711 | 7,472,491 | -48,780 | -0.66% |
| BOB stadium / ALL FILES | 46,291,020 | 45,682,907 | 608,113 | 1.31% |
| **Total** | **94,081,637** | **93,128,505** | **953,132** | **1.01%** |

The Knockout stadium structure FBX grows on re-export despite fewer faces; its native Blender file and Unreal package shrink. This increase is included above, not hidden. Unreal editor asset bytes are not a measurement of cooked/download package size.

## Per-mesh geometry and FBX sizes

| Mesh | Blender faces before → after | Blender vertices before → after | Unreal triangles before → after | FBX bytes before → after |
|---|---:|---:|---:|---:|
| SM_TestArena_Divider | 1,372 → 1,372 | 1,344 → 1,344 | 2,632 → 2,632 | 74,956 → 71,212 |
| SM_TestArena_DividerSocket | 294 → 294 | 288 → 288 | 564 → 564 | 29,484 → 28,572 |
| SM_TestArena_SignalTrace | 294 → 294 | 288 → 288 | 564 → 564 | 29,404 → 28,572 |
| SM_TestArena_Static | 104,636 → 100,964 | 105,112 → 105,112 | 207,856 → 207,856 | 4,111,612 → 3,905,004 |
| SM_TestArena_SwitchDot | 2,884 → 2,788 | 3,072 → 3,072 | 6,136 → 6,136 | 134,396 → 124,460 |
| SM_TestArena_SwitchHousing | 7,940 → 7,556 | 8,192 → 8,192 | 16,376 → 16,376 | 319,804 → 294,636 |
| SM_TestStadium_Lights | 18,368 → 12,964 | 12,908 → 12,908 | 25,496 → 25,496 | 462,380 → 459,852 |
| SM_TestStadium_Structure | 91,512 → 76,248 | 75,452 → 75,452 | 149,664 → 149,664 | 2,762,812 → 2,933,900 |
| SM_BobStadium_Lights | 27,794 → 27,754 | 27,520 → 27,488 | 53,988 → 53,954 | 1,053,964 → 971,868 |
| SM_BobStadium_Structure | 132,312 → 130,450 | 132,704 → 130,902 | 256,740 → 256,632 | 4,821,180 → 4,429,260 |

Source faces total: 387,406 → 360,684. Source vertices: 366,880 → 365,046.

Actual Unreal render triangles total (one copy of each mesh): 720,016 → 719,874. Actual render vertices: 769,649 → 769,231.

## Verification and recovery

- Unreal import completed: 0 errors. BOB stadium import emitted degenerate tangent/near-zero normal/tangent warnings; these are recorded in `Saved/Logs/Venue-Legacy-After.log`, not suppressed. A separate unsaved import of the untouched originals also emitted tangent warnings (`Saved/Logs/Venue-Original-Warnings.log`). Blender inspection found 12 zero-length corner normals in the original lights FBX and the optimized lights FBX; no new zero normals were introduced. Materials and bounds passed comparison. No C++ compilation was needed or run for this asset-only change.
- Before captures: `Saved/UICaptures/20261016-191209-e01602/` (arena, switch rim, stadium stairs, BOB).
- Final captures: `Saved/UICaptures/20261016-194824-53b8e6/`. Compared arena, switch rim and stair views visually: no visible silhouette/detail change. BOB gameplay view is consistent but does not expose every stadium surface. Captures are not pixel-identical: temporal rendering and bot lineups vary between runs. Across the four views mean absolute RGB differences were below 1.7/255 per channel.
- Original FBX, changed GLB, BLEND and Unreal packages: `Saved/VenueOptimization/originals/`, mirroring their project-relative paths. These are local backups in the ignored Saved folder; do not delete them if you want a local rollback.
- Detailed baseline/candidate/runtime JSON and hashes: `Saved/VenueOptimization/`.
- Shared cleanup lives in `Tools/FlickMeshCleanup.py`; the three venue generators use it. Puck and BOB-board generators were not edited.
