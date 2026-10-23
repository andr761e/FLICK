# Physics timing and BOB resolution

## Fixed simulation, variable rendering

The project uses Chaos's fixed-rate async accumulator at **120 Hz**
(`Config/DefaultEngine.ini`, `bTickPhysicsAsync` and `AsyncFixedTimeStepSize`).
Rendering at 40 FPS advances three simulation steps per frame; rendering at
400 FPS advances a step only when the accumulator has collected enough time.
The renderer interpolates the published physics poses instead of changing the
collision timestep to match its own cadence.

`p.UseAsyncInterpolation=1` is required for Chaos's fixed-step path in UE 5.8.
`p.AsyncPhysicsBlockMode=1` catches up queued simulation work; mode 2 must not be
used because it can drop simulation steps under load. Substep limits provide a
120 Hz / eight-step fallback rather than the former variable 60 Hz maximum.

Collider meshes, physical materials, masses, restitution, friction, damping,
gravity, shot strength and CCD are unchanged. This fixes the simulation cadence;
it is not a promise of bit-identical results across CPUs, engine versions,
network conditions, different shot inputs or every chaotic multi-body scenario.
Game-thread shot input, divider controls and presentation still run on the game
thread; rendering rate is no longer used as the contact solver's timestep.

`FLICK.Physics.RenderRateIndependence` simulates identical shots against the
actual arena/puck colliders at 40, 60, 120, 144, 240 and 400 render updates per
second, plus a fluctuating 40/400/55/144 pattern. It covers BOB puck contact,
BOB rail rebound, Knockout puck contact, and tilted BOB puck contact. It requires
matching final positions within 1 cm and checks that collisions really occurred
and no body tunnels through the bed or rails. Each scenario runs a fresh world
with identical initial state and no graphics load.

## BOB's stuck-resolution fixes

- A puck supported by another puck or rail is not airborne merely because its
  bottom is above the nominal playing plane. A short collision query confirms
  actual support. A zero-velocity airborne apex still blocks turn completion.
- Pocket capture sweeps the descending capture-plane crossing between the
  previous and current frame positions. A fast puck cannot skip a pocket just
  because its next rendered endpoint is already beyond the opening.
- A puck that has clearly escaped below the board without entering a pocket is
  returned to an unoccupied playing position. This also covers pucks stranded
  outside the board on the surrounding stadium floor. Recovery thresholds use
  the existing board/puck dimensions and kill plane. This does not count as a
  pocket, striker penalty or elimination. Ordinary airborne shots are left alone.
- The timeout still waits for genuinely moving pucks. There is no arbitrary
  deadline that deletes or freezes a shot in progress.

`FLICK.BOB.ResolutionReliability` covers supported stacks, full turn advancement,
airborne apexes, meaningful motion at timeout, non-scoring escaped-puck recovery,
and all four swept pocket openings. The recovery warnings in this test are
explicitly expected; normal gameplay retains those warnings for diagnosis.

## Epic rendering

The later four-mode reflection-budget pass is documented separately in
[EpicRenderingPerformance.md](EpicRenderingPerformance.md). The measurements
below describe this earlier surface-cache change, not the combined result.

The dominant GPU cost was Lumen's per-hit reflection lighting, not the small
physics scene or material base pass. The project now uses Lumen's surface-cache
lighting path (`r.Lumen.HardwareRayTracing.LightingMode=0`) while retaining real
ray/scene intersections, hardware tracing support, materials, reflection geometry,
the stable rim trace settings and the player's Epic scalability preset.
This reuses cached lighting rather than evaluating full material/lighting and
shadow rays at every reflected hit. It trades some reflection-lighting precision
for frame time; it does not disable reflections or reduce render resolution.

Physics/collision accuracy must not be used as a graphics performance knob.

## Validation, 2026-10-21

Both `FLICKEditor Win64 Development` and standalone `FLICK Win64 Development`
compiled successfully. A packaged/cooked distribution was not rebuilt.

Final full automation report: `Saved/Automation/FixedPhysicsVerified/`.
86 tests passed (including two audio tests warning that their real-device
integration portion cannot run under `-nosound`), seven visual tests failed,
and none were left unrun. All BOB, collision timing, match rules, training,
divider, undo and network-flow tests passed. Across all 56 reported puck
endpoints in the four cadence scenarios, the largest logged deviation was
0.0001 cm; no collision-shape/material tolerances were relaxed.

The seven failures are older visual expectations: `StarterSkins`,
`KnockoutArenaTheme`, `MenuArenaIsolation`, `MenuPresentationIsolation`,
`OneVsOneCameraIsolation`, `TestArenaExposure`, and `WorkshopArena`.
They were rerun with fixed-rate physics disabled, the old block mode and the
old reflection-lighting mode restored; all seven reproduce identical errors.
That control report is `Saved/Automation/VisualAssertionsPreviousSettings/`.
Those tests were not weakened or silently marked passing in this change.

Live view checks: `Saved/UICaptures/20261021-224233-b5c33f/` (BOB and Knockout).
Local Epic/1920x1080 benchmark: RTX 3070, identical presets/camera/resolution,
dynamic resolution off, 1,500 frames each, last 600 measured after warmup:

| Mean milliseconds | Before | After |
| --- | ---: | ---: |
| Frame | 13.822 | 12.868 |
| GPU | 12.942 | 11.988 |
| Lumen reflections | 3.481 | 2.524 |

Equivalent average frame rate is roughly 72.3 -> 77.7 FPS, a 7.4% improvement
in this local BOB view, not a guarantee for every machine/scene. Almost all of
the measured GPU saving is in reflection lighting. No resolution or Epic
scalability group was reduced. Captures are under
`Saved/Performance/BobReliability-Before/` and `BobReliability-After/`.
