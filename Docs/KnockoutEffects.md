# Knockout effects

Profile → Customize → Knockout Effect equips the effect immediately and repeats
the existing physics-driven arena-edge preview. The puck launches, loses support
at the edge and is eliminated using the existing checks. Changing the selection
or leaving the locker destroys the previous preview effect.

Basic Burst remains unchanged. Saved indices 1–5 retain Shockwave, Spark Shower,
Cryo Shatter, Solar Nova and Phase Collapse, now with dedicated geometry and motion.
Fourteen additional selections follow them: Lightning, Toxic Melt, Gold Burst,
Pixel Break, Heart Pop, Water Splash, Spirit Release, Galaxy Rift, Comic Pow,
Confetti Pop, Holographic Fracture, Arcane Seal, Prism Flash and Smoke Puff.

`Core/FlickKnockoutStyle.cpp` holds the tuneable palette, radius, height, duration,
glow, motif and particle count. `Feedback/FlickWorldFeedbackKnockout.cpp` renders
four bounded batched sections, with analytic age-based motion and no collision,
overlaps, shadows, lights or particle actors. The replicated appearance descriptor
uses the existing elimination/replay/BOB hooks. Dedicated servers skip geometry.
All upgraded effects expire within 1.7 seconds; the original physics and scoring,
arena lighting, trails, spawn effects, skins and profile identity are untouched.

The one shared shader is `/Game/Cosmetics/Knockouts/M_PuckKnockout_SoftV1`, included
in AlwaysCook. It is additive, depth-tested, exposure-compensated and texture-free.
Rebuild it through Unreal's Python editor using `Tools/Build-FlickKnockoutMaterial.py`.
Smoke and mist are stylized luminous wisps, not volumetric fluid simulation.

Automation: `FLICK.Cosmetics.KnockoutEffects` checks saved-index compatibility,
equipping, shader wiring, no collision, bounded/finite geometry, motion at 30/60/120
FPS and expiry for every upgraded effect. Run existing cosmetic regression tests
alongside it. RHI locker captures check real material rendering at the arena edge.
Packaged multiplayer verification is still a separate manual smoke test.

## Validation — 2026-10-04

Windows `FLICKEditor Win64 Development` build succeeded. The seven selected tests
passed: CosmeticCatalog, CollectionPresentation, InspectionAndOwnership,
PuckTrails, SpawnEffects, KnockoutEffects and BOB.BotTurnIntegration. The first
build's two numeric-literal type errors were corrected. An initial 120 FPS vortex
transition failure was resolved by smoothly blending collapse into release.

Real offscreen RHI locker captures were inspected for Solar Nova, Galaxy Rift,
Lightning, Water Splash, Holographic Fracture and Cryo Shatter. Only the knockout
preview camera was widened; other preview and gameplay cameras are unchanged.
Final test output is in `Saved/Logs/KnockoutEffectTests.log`. Unreal still emits two
pre-existing `LogAutomationTest: Error: Condition failed` startup assertions before
the selected tests begin; all seven selected tests report `Result={Success}`.
No packaged build, network session or GPU performance benchmark was run.
