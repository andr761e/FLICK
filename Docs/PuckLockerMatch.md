# Locker-matched puck presentation (2026-10-26)

The actual locker puck is the reference. Controlled captures use Standard,
Amethyst and idle state in the locker, live Knockout and the real cinematic
replay camera. The comparison sheet also resizes crops to a common 112-pixel
puck width, separating screen-size differences from lighting differences.

## Exposure and bloom

Knockout gameplay and cinematic replays now share the approved locker values:

| Setting | Previous live value | Matched value |
| --- | --- | --- |
| Exposure bias | 0 | -0.45 |
| Effective bloom intensity | 0.38 | 0.32 |
| Effective bloom threshold | 1.15 | 1.35 |

Shared constants live in `FlickPuckFinish.h`. Fixed Knockout exposure is
retained, avoiding histogram drift. This is a whole-scene exposure change,
including arena reflections; the slight darkening was visually inspected.
The approved locker keeps its existing treatment. Home now opts in as described
in the follow-up below. BOB retains its distinct camera exposure.

Exposure alone improved tone but left nearly white crowns. Wider studio
lighting and a small key-angle adjustment also proved necessary. No puck-only
post-process mask, new materials, geometry or LODs were added.

## Studio coverage

The existing neutral key/fill pair remains isolated on puck channel 1. In play
and replay, fixture distance and dimensions increase by four, and power by
sixteen. Centre illumination is preserved while variation across the board is
reduced. Existing board-size scaling remains in place.

The frame follows camera yaw and elevation. Menu lighting uses the actual
viewing ray to the locker puck, accounting for the off-centre UI composition.
Gameplay now reflects its viewing ray across the puck plane, aligning the
actual key source rather than rotating it in the camera's pitch direction.
A 12-degree side-key adjustment keeps shape in the silver band. Free and
followed-player camera paths also update the studio.

Locker retains its original fixture layout. Reapplying arena settings
preserves studio pose and coverage, avoiding one-frame power/position resets.
Tagged actors are reused. Light count, shadow passes and material assets do
not increase; no FPS improvement is claimed without a benchmark.

## Finish and screen size

Dark-face coating, silver roughness, filtered machining, skin colours and LED
budgets remain unchanged. The comparisons did not justify changing the
approved material. Selection remains deliberately brighter. A gameplay puck
still resolves fewer details than the several-hundred-pixel locker close-up;
the equal-width crops expose that remaining limit. No temporal-filtering or
LOD changes were warranted by these checks.

## Verification

Both FLICKEditor and FLICK Win64 Development compiled successfully. The full
suite ran 116 tests: 107 successes, 2 successes with warnings and the same
7 pre-existing failures: Cosmetics.StarterSkins; Visuals.KnockoutArenaTheme,
MenuArenaIsolation, MenuPresentationIsolation, OneVsOneCameraIsolation,
TestArenaExposure and WorkshopArena.

PuckLockerPresentation checks matching exposure/bloom, no double compensation,
Home isolation and restoration. KnockoutLightingControls checks wide coverage,
area-scaled power, elevation response, reapplication without studio flashes,
and returning to the original locker layout. Finish, palette and physics tests
pass. Final focused puck tests and actual-viewport startup/input and Quit-button
smoke checks pass.

Visually inspected: matched gameplay/replay/locker, equal-width crops,
selection, elevation/orbit/zoom, 3v3 at Medium quality and BOB striker. Extreme
low-angle views naturally show less of the puck top. Moving-shot visual QA is
not claimed by the frozen replay comparison; physics rate-independence is
covered by the regression suite.

Evidence: `Deliverables/Puck-Locker-Match-2026-10-26`. `ExposureStudy` isolates
exposure before wider lighting. `CoverageStudy` retains intermediate trials.
The old `Before/Replay.png` used only an overlay; the genuine replay baseline
is `ExposureStudy/Replay-EV0.png`. The capture helper now invokes the actual
replay camera around a chosen class. Overrides use isolated profiles and do
not overwrite saved cosmetics or camera preferences.

## Home showcase follow-up

The main-menu puck now opts into the same reference exposure/bloom and wide
view-aligned studio. `SetCameraForFrontend` supplies an explicit Home-context
flag, including Settings opened over Home; ordinary browse screens keep their
existing treatment. Locker retains its original fixture arrangement and gets
exposure compensation exactly once. The larger Home showcase uses the direct
locker highlight angle rather than the extra 12-degree gameplay-size offset.
Its automatic orbit continues normally, with the lights tracking that view.

No puck assets, material values, UI layout, profile/cosmetic save data or physics
parameters change. Evidence is retained separately under
`Deliverables/Home-Puck-Match-2026-10-26`.

Both Development targets compiled. Matched Standard/Amethyst Home and locker
captures and a later Home-orbit view were inspected. The full suite again
returned 109/116 passes with the same seven pre-existing failures. Expanded
presentation tests verify Home exposure/bloom, ordinary browse-screen isolation
and no double compensation in locker. Actual-viewport startup/input and
Quit-button smoke checks passed.

## Gameplay elevation follow-up

Directly rotating the light with camera pitch moved its reflection off the
horizontal crown at low camera angles. `MakePuckStudioFrame` now aligns the
authored key vector with the viewing ray reflected across the puck plane.
Normal gameplay uses the ray from the actual camera location to the puck
plane, since camera tilt and camera height have separate framing controls.
Cinematic replay uses its moving focus; free camera uses its forward ray.

Alignment alone made the silver too bright. The gameplay key's specular scale
is separately tuneable in `FParameters` (0.055); diffuse illumination and LED
emission are unchanged. Home and locker retain their existing 0.55 key strength
and authored framing. Reapplying lighting preferences preserves the active
strength and pose, and leaving gameplay restores the menu treatment.

No exposure, material asset, physics, light count or shadow-pass changes are
part of this follow-up. Evidence and the final validation record are under
`Deliverables/Puck-Elevation-2026-10-26`.
