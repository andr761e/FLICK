# Code optimization pass — 2026-11-16

Baseline commit: `86069b2586b55709b4e5608ff936b351b9b9b1cd`.
The native source inventory contained 314 C++/header files and 70,758 lines.
This pass targets provably unused code and repeated presentation work. It does
not establish that every subsystem is optimally implemented.

## Changes

- Removed 28 native functions with no source callers. These were not reflected
  Blueprint functions, RPC implementations, engine overrides or input handlers.
  Removed two fields that were written but never read: the predicted-contact
  actor pointer and a duplicate cinematic pullback value.
- Removed unreachable legacy spawn/elimination branches after those effects
  have already returned through their dedicated rendering paths.
- Retained per-puck ribbon/particle mesh scratch arrays instead of allocating
  and freeing ten arrays on every moving-trail update. Buffer reset retains
  capacity; the mesh submission and topology-update rules are unchanged.
- Calculated trail path directions, camera-facing normals and life factors once
  per sample rather than once for every ribbon lane. Packed repeated colours
  once for each pair/quad.
- Reused vertex/index scratch arrays in menu, puck-thumbnail and shop artwork.
  Slate still receives freshly generated positions, UVs and colours each paint.
  Shared the duplicate convex-polygon clipping loop, preserving each caller's
  original coordinate precision and clipping boundaries.
- Batched expired replay-pose removals while preserving the final two samples
  and retained buffer capacity. The usual replay puck roster fits inline storage.
  Audio voice cleanup likewise retains its array capacity.
- Combined repeated HUD frontend-screen checks into one switch with the same
  early-return cases.

Production-source diff: 221 lines removed, 163 added, **58 fewer lines overall**
(including the new reusable trail-buffer header). No test assertions were removed.
The temporary geometry/timing audit was saved outside production source under
`Saved/CodeOptimization/TrailGeometryAudit.cpp` after verification.

## Verification

Editor and standalone Win64 Development builds succeeded. A fresh cook/stage
updated `Builds/Development/Windows`; native metadata and packaged executable
were rebuilt together.

All 36 trail cases matched the reference renderer's geometry checksum exactly:
12 styles at 30/60/120 FPS. The checksum includes positions, normals, UVs,
colours and indices from both actual procedural mesh sections. The reference
and optimized loops took 0.150638 s and 0.145478 s in the short null-RHI run;
these timings are not a statistically controlled FPS benchmark. Avoid claiming
a general frame-rate gain from that small difference.

The full 130-test automation suite had identical results before and after:
120 successes, three successes with warnings and seven failures. Every test's
state, error count and warning count matched the baseline. The existing failures
are:

- `FLICK.UI.PlaylistArtwork`
- `FLICK.Visuals.KnockoutArenaTheme`
- `FLICK.Visuals.MenuArenaIsolation`
- `FLICK.Visuals.MenuPresentationIsolation`
- `FLICK.Visuals.OneVsOneCameraIsolation`
- `FLICK.Visuals.TestArenaExposure`
- `FLICK.Visuals.WorkshopArena`

The suite includes physics/render-rate independence, settings, cosmetics,
match/turn rules, achievements, progression, online rules, camera control and
replay compatibility tests. Original player replay/configuration hashes were
checked separately; runtime validation uses isolated profiles.

Rendered play/customization/shop captures were generated at 1280x720 and
3440x1440. Representative artwork, thumbnail and ultrawide frames were visually
inspected. Packaged Knockout and BOB record/save/load/seek checks and the 720p clip
workflow passed, including live handle previews, confirmation, looping and
player/free-camera options.

Evidence: `Saved/CodeOptimization` contains the before/after automation reports,
per-test comparison, removed-function inventory, trail geometry comparison,
build logs and UI-capture manifest. Packaged smoke output is under
`Saved/ReplayImplementation/SmokeClassic`, `SmokeBob`, `OptimizationClips720`
and `LightingStability/OptimizationAfter`.
