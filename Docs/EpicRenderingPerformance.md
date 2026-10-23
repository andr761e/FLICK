# Epic rendering budget

This pass is **rendering-budget tuning**, not a physics or mesh simplification.
It builds on the previous surface-cache lighting improvement documented in
[PhysicsTiming.md](PhysicsTiming.md); that earlier gain is not counted again.

## Changes and trade-off

- `Config/DefaultScalability.ini` gives the project's Epic reflection group a
  **2x1 checkerboard tracing layout**, rather than tracing every pixel. Lumen's
  spatial/temporal reconstruction produces the final reflections. This is half
  the new reflection rays, not a quarter-resolution 2x2 tracing grid.
- The Knockout camera uses the standard reconstruction multiplier of 1.0 instead
  of 1.5. At Epic this means five reconstruction samples instead of eight. BOB
  already used the standard multiplier and does not inherit this override.
- Lower reflection presets explicitly clear checkerboarding when selected;
  Cinematic retains Unreal's full tracing grid. Ordinary video settings expose
  Low through Epic, so Cinematic is a developer console/capture option, not a new
  menu entry.

This is **not mathematically lossless**. Newly revealed/reflected fine detail can
be softer or take longer to converge. Geometry, materials, roughness, lighting,
exposure, shadows, GI and TSR history resolution have not been reduced. The
existing scene-trace overrides that stabilized the Knockout rim remain in place.
No dynamic-resolution scheme was introduced, and the saved render-scale setting
is unchanged. No physics, collisions, damping, friction, shot power or network
timing was changed by this pass.

Trials enabling reflection async-compute, fully async TSR, or reflection radiance
cache reuse did not produce a convincing improvement on this GPU. None of those
experimental console settings was added to the project defaults.

## Reproduce a comparison

Build `FLICKEditor Win64 Development`, close other game/editor instances, and run
sequentially from the project root (choose unused labels):

```powershell
python Tools/Benchmark-Flick.py --label Epic-Control --previous-budget
python Tools/Benchmark-Flick.py --label Epic-Optimized
```

The script uses only Python's standard library. Engine discovery accepts
`FLICK_UNREAL_ENGINE_ROOT` or `--engine-root`. It launches an actual DX12/SM6 game
viewport offscreen, with isolated profiles, fixed random seeds, the same camera,
and settled starting boards in BOB and Knockout 1v1/2v2/3v3. The development-only
`-FlickPerformancePreview` flag suppresses shot-clock/bot input and BOB's preview
shot, not physics ticks. Thus faster and slower captures keep the same board
instead of reaching a timeout and launching shots at different measured frames.
The script rejects captures containing a kickoff/launch. This flag is unavailable
in Shipping and normal play never enables it. The Knockout views
have 8/16/24 pucks respectively. No builds or other GPU tests should run alongside
the benchmark. It does not touch the normal player's settings or connect Steam;
audio is disabled in these GPU comparisons.

Each process captures 1,500 frames, removes the frame cap/VSync at frame 300, and
reports the last 600 frames after warmup. It verifies Epic scalability groups in
the log. CSVs, logs, effective rendering settings, frame/GPU timings and a JSON
summary are saved under `Saved/Performance/<label>/`. The parser uses Unreal's
final CSV header, because new stat columns can appear during a capture.

`--previous-budget` restores the old full tracing grid and old effective
reconstruction sample count (8 for Knockout, 5 for BOB) through process-local
console overrides. The camera-quality multiplier's other use in UE 5.8 is the
downsampling threshold at 0.25; neither 1.0 nor 1.5 crosses it. Thus the control
recreates the previous reflection work without rebuilding or undoing files.
It intentionally retains the earlier fixed-rate physics and surface-cache
lighting changes. `--full-reflections` changes only the tracing grid, useful for
isolating checkerboarding from the reconstruction-budget change.

These are **settled-board rendering measurements**, not full-match, moving-shot,
replay, network or minimum-FPS guarantees. FPS is 1,000 divided by mean measured
frame time. Small gains, especially BOB's, should not be overinterpreted across
different hardware, camera angles or background workloads.

## Measured results, 2026-10-21

RTX 3070, i7-9700KF, DX12 hardware Lumen, Epic groups (3), 1920x1080 output,
the existing **110% render scale** (2112x1188 scene rendering), dynamic resolution
off, identical fixed-seed starting formations and camera distance 1.25.
These figures are from the editor executable's `-game` viewport, not a newly
cooked package. Both sides retain the preceding 120 Hz physics and cached
reflection-lighting implementation.

| Mode | Before FPS | After FPS | FPS change | GPU ms before / after |
| --- | ---: | ---: | ---: | ---: |
| BOB | 85.8 | 90.2 | +5.2% | 11.161 / 10.653 |
| Knockout 1v1 | 72.1 | 76.4 | +5.9% | 13.294 / 12.567 |
| Knockout 2v2 | 67.4 | 72.9 | +8.2% | 14.102 / 13.170 |
| Knockout 3v3 | 67.0 | 68.5 | +2.1% | 14.300 / 13.818 |

3v3's improvement is modest and closer to normal timing variation; do not
advertise a guaranteed percentage. Source captures and JSON summaries:
`Saved/Performance/EpicSettled-Control/` and `EpicSettled-After/`.
Earlier `EpicFurther-*` experiments are **not** the comparison above: preview
shot-clock timeouts could advance those matches and change their formations.
The final captures use `-FlickPerformancePreview`, and all eight logs were also
checked for kickoff/shot input. No automated shot occurred.

Both `FLICKEditor Win64 Development` and standalone `FLICK Win64 Development`
compiled successfully after the changes. An initial new-test compile error
(`FConfigFile::Read` returns void) was corrected and both builds rerun. No cooked
or packaged distribution was regenerated; package again to distribute the change.

Final automation report: `Saved/Automation/EpicFurtherFinal/`. **87 tests passed**
(including two successful audio tests with no-device warnings under `-nosound`),
seven failed, none were left unrun. The seven older visual failures and their
error messages are identical to the preceding `FixedPhysicsVerified` report;
they were not weakened. See [PhysicsTiming.md](PhysicsTiming.md) for that list.
The new `EpicReflectionBudget` check and existing Knockout/BOB lighting,
render-rate collision independence and BOB resolution reliability checks pass.

Live visual QA: `Saved/UICaptures/20261021-234652-27799c/`, eight captured frames
each for BOB, Knockout and the orbiting main menu. The inspected BOB/Knockout
endpoints show no obvious new rim/reflection artifacts; this is a limited visual
check, not a claim that checkerboarding is identical to full-grid tracing or
that every camera/moving shot has been exhaustively tested.

## Restore the previous rendering budget

For a developer comparison without editing source, use `--previous-budget` as
above. For a permanent source rollback of this pass's visual tuning only, remove
the new `DefaultScalability.ini` and restore `LumenReflectionQuality = 1.5f` in
`FlickCameraPawn.cpp`. Do not revert the unrelated physics/reliability changes in
`DefaultEngine.ini` or the BOB implementation.
