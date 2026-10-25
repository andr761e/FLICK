# Knockout lighting review — 2026-10-25

## Scope and visual criteria

Reviewed the existing Switchyard rig, lighting controls, fixed exposure,
post-processing, rim/stadium spill and puck finish. The reference remains
`Deliverables/UI-Visual-Upgrade-2026-10-22/Concepts/11-arena-no-menu.png`:
blue gunmetal, cyan fixtures and warm amber architectural reflections.
Do not copy its distorted arena geometry.

Priorities: readable dark puck faces at arbitrary orbits; readable board
markings on both halves; reflective metal rather than a flat colour wash;
amber light grounded around the skirt; consistent illumination as board size
changes. A strong opening-camera screenshot alone is not sufficient.

No gameplay, collision, physical material, source mesh, render scale, reflection
sample count or exposure changes. The previous anti-glare puck material fix is
retained. BOB's explicit light powers and colours are not retuned.

## Findings

- The old rig concentrated saturated cyan reflected energy on the far half of
  the deck while the near half looked much darker. More neutral coverage is a
  better visibility control than increasing softbox/glare power.
- Small, intense warm seating sources could reflect as white patches from
  certain orbits. A wider source with lower power gives a softer reflection.
- The amber skirt light already uses twelve stable, tagged, shadow-free
  sources. Their count, range and placement are retained; only power is eased.
- Scene scale already multiplies local powers by arena area and scales source
  dimensions/positions. Preserve that behaviour for 2v2, 3v3 and private scales.
- All seven saved lighting controls are shared with BOB. Preserve the keys,
  ranges and user values; tune the Knockout base rig rather than overwriting
  saved preferences or changing shared slider defaults.
- The normal settings-page backdrop is static concept artwork. The live arena
  is shown only by PREVIEW ARENA. Its hint now makes that distinction explicit.

## Controlled studies

`Tools/Capture-FlickUI.ps1 -LightingValues @{...}` injects validated overrides
into an isolated capture profile. It does not edit the normal player profile.
Quality defaults explicitly to Epic (3). Metadata records quality, orbit and
lighting values. The settings screenshot confirms the injected values are
actually visible to the game, rather than relying only on command-line text.

Three balances were compared at 0° and 45°, against an unchanged seeded board:

| Study | Ambient | Fill | Cool | Warm | Accents | Direct | Reflections |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Neutral steel | 1.10 | 1.20 | .70 | .82 | .55 | 1.00 | .30 |
| Cinematic | .90 | 1.10 | .88 | 1.05 | .65 | 1.00 | .25 |
| Readable | 1.15 | 1.40 | .60 | .85 | .35 | 1.10 | .24 |

The final base rig takes the neutral-fill direction without simply baking one
shared preset into every mode. The more subdued cyan colour, source areas and
warm floor spill are tuned independently of the sliders. The cinematic study
retained too much near/far contrast; the readability study lost some depth.

## Base-rig changes (650 cm board)

| Parameter | Before | After |
| --- | ---: | ---: |
| Sky multiplier | .65 | .75 |
| Diffuse-only fill intensity | 150,000 | 195,000 |
| Cool softbox base intensity | 260 | 190 |
| Warm softbox base intensity | 180 | 150 |
| Team accent base intensity | 3,200 | 1,800 |
| Cool source width × height (cm) | 800 × 180 | 900 × 220 |
| Warm source width × height (cm) | 540 × 120 | 600 × 150 |
| Warm seating source (lumens) | 180 | 150 |
| Warm seating source width × height (cm) | 120 × 60 | 180 × 75 |
| Warm seating specular multiplier | 1 | .55 |
| Each skirt-spill source (lumens) | 24 | 20 |

Cool light is slightly less saturated: linear RGB (.18, .52, 1) →
(.28, .58, 1). Warm softbox is (1, .44, .14) → (1, .48, .17).
Softbox multipliers remain 250; local light values still scale with board area.
The visible LED emission, puck-only neutral reflector, directional shadow
light, exposure, colour grade and source positions are unchanged.

Authoring values are in `FlickArenaLighting::FParameters`, mirrored by the
existing editable `AFlickGameMode` fields where applicable. Tests explicitly
compare host and client defaults to prevent drift. Seating source dimensions
are now exposed in FParameters alongside their existing power setting.

## Verification artifacts

All screenshots and the final test report are kept under
`Deliverables/Knockout-Lighting-Review-2026-10-25`, not only disposable Saved data.
`Before` contains the original three formats, orbits, home and settings.
`Studies` contains the three isolated balances and their settings proof.
Final verification and build/test results are recorded after visual review.

The first rebuilt base-rig matrix is retained in `Studies/base-rig-pass-1`.
It improved coverage but a diagonal seating reflection still appeared pale.
The final polish bounds seating specular separately from its diffuse light;
it does not dim the warm architectural spill or remove the reflection.

## Final visual review

The final matrix includes 1v1 at 0°, 45°, 135° and 225°, plus 2v2/3v3 at
15° and 135°. A 1080p 1v1 capture, the home menu, Amethyst locker and live
lighting-preview panel were also inspected. The default slider values remain
100% for the six power controls and 35% for direct-light reflections.
Existing custom slider values are preserved. RESET LIGHTING is optional if you
want the exact reviewed balance; it resets the shared BOB preferences too.

High (2) and Medium (1) quality captures retain readable pucks, markings and
team indicators. Medium's existing scalability settings reduce distant stadium
detail; this review does not change those settings. A maximum-reflections (100%)
stress capture is intentionally much hotter than the recommended 35% default,
but the type emblems remain readable. No new material white-out was introduced
at the reviewed default angles.

An eight-sample Epic sequence after eight seconds of warm-up was inspected at
its endpoints and checked numerically across all samples. Bright near-rim cyan
and amber masks had mean per-channel max–min ranges of 3.31 and 3.55 out of 255;
the largest individual channel ranges were 29 and 84, respectively, including
antialiased boundaries and reflected spill. There was no obvious on/off blink
in this short static-view check. This is not a guarantee against every temporal
artifact during camera motion, and is not an FPS benchmark.

There are 45 captured comparison/QA images including the eight temporal samples.
The final defaults preserve metallic depth and warm/cool separation while
reducing the old far-side cyan concentration and pale diagonal seating glare.

## Automated verification

`FLICK.Visuals.KnockoutLightingControls` independently exercises each slider at
zero and twice its default. Only the intended fixture group may change, even
after repeated applications. Reflections also control seating and the puck-only
reflector without changing diffuse fill. Existing tests cover all three board
sizes, private scale, host/client parity, BOB transitions, reset, light reuse
and dedicated-server exclusion.

The final full suite ran 114 tests: 105 successes, 2 successes with warnings,
and the same 7 pre-existing failures; none were skipped. LightingControls,
KnockoutLightingTheme, BobLightingTheme, PuckFaceGlare and
Physics.RenderRateIndependence passed. Existing failures remain:
Cosmetics.StarterSkins; Visuals.KnockoutArenaTheme, MenuArenaIsolation,
MenuPresentationIsolation, OneVsOneCameraIsolation, TestArenaExposure and
WorkshopArena. Their assertions were not removed or hidden.

Final report: `Saved/Automation/KnockoutLightingReviewFinal20261025/index.json`.
Editor build: `Saved/Logs/KnockoutLightingReviewEditorFinal.log` (succeeded).
Game build: `Saved/Logs/KnockoutLightingReviewGameFinal.log` (succeeded).
The real DX12 viewport/input smoke also passed, including entering the lighting
page, opening live preview, returning to settings and quitting through the UI.
Both `PACKAGED_STARTUP_SMOKE_TEST PASS` and `PACKAGED_STARTUP_QUIT_BUTTON PASS`
are recorded in `Saved/Logs/KnockoutLightingReviewSmoke.log`.
These are editor/development builds, not a fresh packaged cook or archive.
