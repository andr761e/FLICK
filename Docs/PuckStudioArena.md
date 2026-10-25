# Puck studio and Knockout arena follow-up (2026-10-25)

## Intent

Use the existing Amethyst locker puck as the live-puck reference. Arena
preferences should change the surroundings without washing out the puck face,
silver hardware or cosmetic hue. Bring Knockout closer to concept
`11-arena-no-menu.png`, particularly its paired cyan reflections and warm
peripheral light.

## Implementation

- Workshop puck meshes use lighting channel 1 exclusively. A shared neutral
  studio key and softer cool fill replace their adjustable arena illumination.
  Both are shadow-free, have zero indirect/volumetric contribution, and retain
  fixed energy/specular response when any of the seven arena sliders change.
- Studio orientation follows camera azimuth, retaining the same lighting
  relationship during orbit. Distances, source sizes and power scale with board
  size. Tagged actors are reused on settings, format and scene changes.
- The shared puck material attenuates environment response to 0.12 while
  preserving its lit metallic finish, satin face coating and LED colour. This
  reduces indirect influence; it does not replace scene reflections or tone
  mapping with an unlit image.
- The existing hidden collider mesh casts the contact shadow on arena channel
  0. Its geometry and physics are untouched. Eliminated pucks disable it.
- Knockout gains a second broad cyan deck reflection source, controlled by the
  existing Cool Softbox preference. The amber card moves toward the periphery;
  seating/rim spill receive modest increases. Deck tint, roughness and fine
  relief are tuned together. No arena geometry or collision changes.
- Lighting settings explain the separation between arena and puck lighting.
  No new user settings or save keys are introduced.

This adds two shared shadow-free fixtures relative to the previous rig: one
puck fill and one Knockout deck card. There are no new lights per puck. BOB
retains its arena rig and distinct pearl striker face.

## Evidence

Captures live under `Deliverables/Puck-Studio-Arena-2026-10-25`. Every capture
uses an isolated user profile, so lighting experiments do not overwrite player
preferences. `MatchPuckSkin` is a capture-only override for a like-for-like
cosmetic comparison.

- `Before`: original default/minimum/maximum arena lighting and Amethyst locker.
- `Studies/first-studio-pass`: deliberately overpowered initial studio pair,
  rejected after inspection because it lifted the dark face and silver too far.
- `After`: final matched-skin minimum/default/maximum comparisons, camera orbit,
  multiplayer format, locker and BOB checks.

The extremes deliberately set all arena controls to 0 or their maximums (3 for
lighting, 1 for reflections); they are stress checks, not suggested presets.

## Validation

FLICKEditor and FLICK Win64 Development builds succeeded. The shared material
builder completed with zero errors. No packaged archive was generated.

The final full suite ran 114 tests: 105 successes, 2 successes with warnings and
the same 7 pre-existing failures: Cosmetics.StarterSkins;
Visuals.KnockoutArenaTheme, MenuArenaIsolation, MenuPresentationIsolation,
OneVsOneCameraIsolation, TestArenaExposure and WorkshopArena. The first full run
also exposed the old ConceptPresentation roughness cap of 0.20; that assertion
was updated for the softer reflective finish and passed on the final run.

KnockoutLightingControls checks all seven slider groups while asserting both
studio powers stay fixed, repeated applications never add lights, studio
channels cannot illuminate the floor, and camera rotation preserves their
relative orientation. WorkshopPucks checks the independent render channel,
unchanged collision mesh/mass/simulation, and contact-shadow removal on
elimination. PuckFaceGlare checks the shared material response and coated faces
across all six finish families. Those tests, both lighting-theme tests,
ConceptPresentation and Physics.RenderRateIndependence all pass.

Minimum/default/maximum images at the same 15-degree orbit use Amethyst on both
teams. A simple RGB sample of eight puck-top ellipses changes by 4.74% of full
scale at minimum and 3.81% at maximum, compared with default. These are image
differences including hardware, LEDs and antialiasing, not perceptual colour
measurements; the method and regions are recorded in
`After/lighting-samples.json`. Residual scene-reflection differences are
expected. The large board-lighting difference remains visible as intended.

Visually inspected: the Amethyst locker close-up, 1v1 at minimum/default/maximum
arena controls, 0/15/45-degree default views, a 135-degree maximum-light view,
2v2, 3v3, the BOB striker close-up, and Blue/Amethyst gameplay at 1920x1080.
The coated faces stay dark and readable; the BOB target face remains distinct.
The actual-viewport startup/input and Quit-button smoke checks also passed.

Final automation report: `Saved/Automation/PuckStudioVerified20261025`.
Build logs: `Saved/Logs/PuckStudioEditorFinal.log` and
`Saved/Logs/PuckStudioGameFinal.log`.
