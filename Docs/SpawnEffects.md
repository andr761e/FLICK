# Puck spawn effects

Choose an arrival in **Profile → Customize → Spawn Effect**. Selecting a different
arrival clears the previous preview immediately. The preview repeats on the arena,
with a dedicated camera that leaves space above the puck. Basic Drop is unchanged.

## Saved selections

Indices are append-only; existing profiles need no migration.

| Index | Arrival | Presentation |
| --- | --- | --- |
| 0 | Basic Drop | Original lowering animation, no extra particles |
| 1 | Pulse Arrival | Descending scan rings, cyan filaments and ion dust |
| 2 | Spark Arrival | Ignition seal, spiral sparks and a landing shower |
| 3 | Cryo Lock | Growing faceted ice crown and drifting frost diamonds |
| 4 | Solar Flare | Turbulent flame tongues, twisting ribbons and embers |
| 5 | Phase Gate | Counter-rotating violet/cyan portals that descend and collapse |
| 6 | Energy Beam | Tall filaments, travelling rings and a feathered column |
| 7 | Lightning Strike | Changing zigzag spines, side branches and electric motes |
| 8 | Prism Arrival | Seven spectrum-coloured shafts and rainbow seals |
| 9 | Golden Ascent | Gold shafts, concentric seals and rising star sparks |
| 10 | Heartfall | Floating shaped pink hearts around a rose seal |
| 11 | Pixel Assembly | Descending voxel-like sprites and digital dust |
| 12 | Galaxy Gate | Tilted spiral arms, luminous star core and orbital particles |
| 13 | Spirit Arrival | Turbulent turquoise tongues and curling spectral ribbons |

## Implementation and tuning

- `Core/FlickSpawnStyle.cpp` defines palettes, radius, height, emission, afterglow,
  motif masks and particle counts. These are presentation-only values.
- `Feedback/FlickWorldFeedbackSpawn.cpp` owns the spawn-only renderer. A single
  procedural component batches up to four layers: seals, ribbons, motes and facets.
  Geometry stays under 1,800 vertices per effect. It uses no particle actors, lights,
  collision, navigation, overlaps or shadow casting.
- Animation positions come from elapsed age rather than per-frame integration.
  The landing pulse follows the existing tuneable `PuckArrivalDuration`, replicated
  with the feedback appearance. The puck's arrival movement and physics are not
  replaced. Floor seals originate two units above the arena surface, not above the
  puck's centre.
- `M_PuckSpawn_SoftV1` is a single unlit, additive, two-sided material with analytic
  feathered ring/ribbon, glow, diamond, voxel, heart, star and turbulent flame masks.
  Vertex RGB supplies colour; vertex alpha supplies fade. Exposure compensation and
  colour shaping live in this material, not the arena lighting. The material renders
  before depth of field and respects depth testing against the puck and arena.
- `Tools/Build-FlickSpawnMaterial.py` creates/updates only this material, asserting
  its connections. Run it via UnrealEditor-Cmd's `-ExecutePythonScript` option.
  `/Game/Cosmetics/Spawns` is included in packaging. No textures or Niagara systems
  are required.
- Dedicated servers skip the new render geometry. The existing feedback actor
  replicates the chosen style; effects expire automatically and clear their mesh.
- Trails, knockout effects, cosmetics ownership and arena lighting are unchanged.

## Verification

Verified on 2026-10-04: editor build succeeded and all five tests below returned
`Result={Success}`. Spawn shader generation also completed successfully on reruns.
Rendered locker previews were inspected for Cryo, Solar, Prism, Lightning, Hearts,
Galaxy and Spirit. The final Solar/Spirit captures include the turbulent tongues.

Build `FLICKEditor Win64 Development` and run these automation tests:

```
FLICK.Profile.CosmeticCatalog
FLICK.Cosmetics.CollectionPresentation
FLICK.Cosmetics.PuckTrails
FLICK.Cosmetics.InspectionAndOwnership
FLICK.Cosmetics.SpawnEffects
```

SpawnEffects exercises all fourteen selections at 30, 60 and 120 FPS. It checks
saved-index compatibility, shader wiring, bounded/finite geometry, valid triangles,
frame-rate consistency, physics isolation, hidden legacy cubes and eventual cleanup.
Live rendered locker captures are stored under `Saved/UICaptures`; use:

```powershell
.\Tools\Capture-FlickUI.ps1 -Screen Customize -LockerCategory 13 -LockerItem 4 -Width 1600 -Height 900 -CaptureDelaySeconds 3
```

The test runs emit two pre-existing engine startup `LogAutomationTest: Error:
Condition failed` assertions before the selected project tests execute. Check each
selected test's actual `Result={Success}` entry rather than treating those startup
assertions as a project-test result. Packaged builds and live multiplayer have not
been exercised for this update.
