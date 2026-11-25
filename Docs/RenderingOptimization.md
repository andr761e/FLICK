# Rendering optimization — 18 November 2026

This pass reduces repeated rendering and presentation work while preserving the
existing geometry, material values, lighting preferences and graphics presets.
Physics, collisions, gameplay rules, replay recording rates and camera controls
were not edited. The local Windows Development package has been rebuilt, cooked
and updated.

## Implementation

- Repeated Switchyard switch housings and divider sockets share identical
  cosmetic material instances. Separate caches preserve the different switch
  backing and socket offsets. Sharing survives the concept-presentation clone
  layer, which previously created a separate instance for every component.
  Live mechanism accents retain their own state-owned materials. The caches
  belong to one arena initialization; there is no global material manager.
- The fixed deck and stadium inlays use rigid shadow-cache invalidation and
  omit deformation-only velocity writes. Their existing material offsets and
  hardware-ray-tracing evaluation remain active. Actual transform changes still
  invalidate rigid shadows. A format/height/pattern refresh explicitly refreshes
  the two fixed surfaces. Moving mechanisms and pucks retain their normal paths.
- Puck selection pulses update their existing halo transform separately from
  the full appearance update. Selection/hover setters skip unchanged states;
  hit flashes, ownership, viewer and accessibility changes still refresh the
  appearance. Saved-replay highlights retain their full visibility/light
  refresh. Detailed pucks no longer set their light to zero immediately before
  restoring its final intensity in the same appearance update.

No asset files or graphics defaults were changed. Resolution, render scale,
reflection trace counts, TSR quality, shadow quality, light coverage/intensity
and cosmetic appearance retain their existing settings.

## Performance

RTX 3070 / i7-9700KF, packaged Win64 Development, DX12, High groups (2), 1920 ×
1080 output, 100% scene scale, hardware Lumen. VSync/frame cap were disabled only
in isolated test profiles. Fixed-seed settled boards, identical camera distance
0.9, normal actor/physics ticks and replay recording; automated shots, Steam and
audio disabled. The normal player's settings file retained its original SHA-256.

Each run captures 1,800 frames and measures the final 900. Comparisons run
sequentially, swapping only the executable/PDB at the same package path while
retaining identical cooked content and profile inputs. The optimized executable
is restored afterward. FPS is 1,000 divided by mean frame time.

Final warmed paired comparison:

| Scene | Before FPS | After FPS | Change | Frame ms before / after | Draw calls before / after |
| --- | ---: | ---: | ---: | ---: | ---: |
| Knockout 1v1 | 103.5 | 107.6 | +4.0% | 9.665 / 9.293 | 721 / 561 |
| Knockout 3v3 | 93.5 | 97.7 | +4.4% | 10.694 / 10.240 | 1,209 / 913 |

GPU means were 9.225 → 8.990 ms and 10.141 → 9.832 ms respectively. Frame-time
p95 was 10.853 → 10.130 ms in 1v1 and 11.508 → 11.205 ms in 3v3. Draw calls
fell about 22% and 24%. Game-thread means fell 1.830 → 1.736 ms and 2.207 →
2.095 ms. These markers describe overlapping pipeline work and are not additive.

BOB measured 125.2 → 127.1 FPS in the earlier paired matrix, with unchanged draw
calls (385). Treat this small difference as normal variation, not an established
BOB rendering gain. The major batching/cache changes apply to Knockout.

Earlier captures varied more: initial 1v1/3v3 runs showed substantially lower
rates. All raw runs are retained; the table uses the later explicit verification
pair rather than presenting the largest observed percentage. These settled-scene
measurements do not guarantee gains in every active match, menu, replay or GPU.
The result is a modest improvement and does not establish a 120 FPS target.

## Validation

- `FLICKEditor Win64 Development` and `FLICK Win64 Development` compiled
  successfully. BuildCookRun then completed with exit code 0 and updated
  `Builds/Development/Windows`.
- All **131 automated tests** ran: 121 successes, three successes with warnings
  and seven existing failures. All 130 existing tests retained their previous
  state, error count and warning count compared with the saved November 16
  report. The new puck test passed across all nine archetypes, checking animated
  selection, fading hit flashes, hover restoration and independent neighbouring
  puck materials/lights. The arena concept test checks shared cosmetic finishes
  through repeated 1v1/2v2/3v3/menu transitions alongside its existing geometry,
  offset, collision and divider-animation checks.
- Rendered packaged Knockout and BOB checks both passed with exit code 0:
  match completion, recording, named save, load, puck counts, pause, forward/back
  seeks, stable poses, free camera, profile isolation and library persistence.
- The packaged saved-replay lighting audit passed with exit code 0: repeated
  seeks retained light identities, camera movement woke temporal sampling, and
  free/opposite-player views settled correctly. Visually inspected reference
  and optimized arena/free/player captures retained the same lighting and art.
  Mean absolute RGB differences were 0.97/255 for the paused overview, 0.81/255
  in free camera and 0.50/255 in the opposite-player view. Temporal state and
  the small playback timestamp difference prevent pixel-identical comparisons.
- Paused deck brightness was 118.896 → 118.893; paused temporal standard
  deviation was approximately zero on both sides. Seek deck temporal standard
  deviation was 0.5533 → 0.5542, effectively unchanged. The previous flicker
  stabilization remains in place.

Existing failures: `FLICK.UI.PlaylistArtwork`, `FLICK.Visuals.KnockoutArenaTheme`,
`MenuArenaIsolation`, `MenuPresentationIsolation`, `OneVsOneCameraIsolation`,
`TestArenaExposure`, and `WorkshopArena` (the latter six share the
`FLICK.Visuals.` prefix). These were not weakened or removed.

The CSV profiler's auto-exit still returns **777003** after finalizing its data
and logging shutdown, for both reference and optimized executables. Unreal names
this `CrashReporterCrashed`. It remains unresolved; those captures are not clean
shutdown tests. The separate normal replay/lighting checks exited successfully.

## Artifacts and experiments

- `Saved/RenderingOptimization/comparison-Verify.json`: final paired timings.
- `Saved/RenderingOptimization/comparison.json`: initial eight-run matrix.
- `Saved/Performance/RenderingOpt-*`: complete CSVs, effective settings and logs.
- `Saved/RenderingOptimization/Final/Report/index.json` and
  `final-regression-comparison.json`: test results and unchanged-existing-test audit.
- `Saved/RenderingOptimization/SmokeClassic` and `SmokeBob`: packaged checks.
- `Saved/ReplayImplementation/LightingStability/RenderingOptBefore` and
  `RenderingOptAfter`: reference/optimized captures, settings and lighting audit.
- `Saved/RenderingOptimization/visual-comparison.json`: image comparison.
- `Saved/RenderingOptimization/Before`: pre-change production files and game
  executable/PDB backups. The temporary reference executable was removed from
  the deliverable after testing.

An experimental shared puck-finish cache found no reusable roughness-clamp
overrides across all nine archetypes and seven palettes, so it was removed.
Asynchronous reflection/TSR scheduling trials did not establish a sufficiently
repeatable gain to justify changing defaults. Existing reflection geometry and
light coverage were retained to preserve the current appearance.
