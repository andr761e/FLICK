# Cosmetic effects visual upgrade — 2026-10-28

The live trails, arrivals and knockouts share a new optical treatment: fine
light cores within soft coloured falloff, antialiased silhouettes, animated
flow, shaped highlights and soft intersections with the floor/puck. Large
uniform additive surfaces no longer supply the entire effect.

## Rendering

`Tools/FlickCosmeticMaterial.py` authors the existing three production materials.
It embeds `Tools/Shaders/FlickCosmeticMasks.hlsl`; there is no runtime Python,
loose shader file, imported texture pack or Niagara dependency. The per-family
build scripts remain usable; `Tools/Build-FlickEffects.py` rebuilds all materials.

The analytic mask returns RGB shading and alpha coverage from one evaluation.
Only flow/flame/cloud masks evaluate the three fixed noise octaves. Thin cores,
bevels, lens glints and cloud shading share that field. Vertex colour supplies
the style palette, vertex alpha supplies lifetime, and a short depth fade softens
intersections. All families now compensate exposure consistently. Effect age,
rather than global shader time, controls animation, including frozen review poses.

Two shared surfaces complement the additive light: `M_PuckAtmosphere` supplies
the knockout smoke/mist layer, while `M_PuckFacet` supplies solid crystal and
foil faces. Facet shading derives the actual triangle normal from position
derivatives, with a fixed neutral studio response and view-dependent glints.
It writes depth and dissolves with lifetime, preventing rear faces from washing
the entire fragment into flat neon. Both reuse the existing Knockouts AlwaysCook
directory. These are bounded meshes/cards, not a fluid simulation or a new
scene-lighting system.

## Trails

All twelve active trails receive the new shader. Ribbons bank toward the camera
and separate slightly in height, retaining width at low angles. Ember, Solar,
Lightning and Gold particles align their tapered streaks with projected motion.
Crystal, pixel and heart motifs have internal shading and edge highlights;
Galaxy/Phase/Spirit use flowing filaments and soft wisps. Existing palettes,
saved indices and sampling/history limits remain stable. None stays clear.
Brief hitches keep a bounded set of the newest samples instead of clearing the
trail; teleports and gaps longer than the effect lifetime still reset history.

## Arrivals

All thirteen effect arrivals receive higher-resolution seals, soft outer glow,
small assembly ticks, shaped particles and age-driven material animation. The
existing class-specific shafts, portals, flames, hearts, pixels and galaxy arms
retain their identities. Shafts gain a soft light curtain and Galaxy gains a
nebula layer. Cryo adds shaded crystal faces and fine edges; Spark, Solar, Gold and
Lightning add tapered orbiting sparks. The seal releases on the actual puck
arrival duration. Basic Drop remains the clean arrival without extra particles.

## Knockouts

All twenty selections use the bounded renderer, including Basic Burst, which now
uses a compact team-coloured wave and fine sparks rather than legacy cubes.
Higher-resolution pressure rings combine a fine edge and soft halo. Most styles
open with a brief geometric lens glint; fragments follow analytic ballistic arcs,
and spark tails face the camera. The existing elemental, spectral, graphic and
celebration motifs remain distinct. Organic effects use shaded transparent clouds;
Ice, Hologram and Prism use solid faceted fragments. Confetti pieces tumble in
three dimensions with foil highlights and separate sparkling motes.

## Locker presentation

Effect previews focus on the actual arrival/trail/arena-edge effect position,
with a wider depth of field (f/5.6). The knockout composition aims higher so the
burst stays within the preview column. The approved puck-only locker close-up
retains its existing framing and f/2.4 treatment; Home and gameplay cameras retain
their framing and exposure.

## Scope and budgets

These changes affect presentation only. Selection/config indices, replication,
arrival timing, collision, friction, restitution, scoring and replay rules are
unchanged. Trails still use two draw sections, at most 48 history samples and
48 particles. Arrivals and knockouts still use four sections, bounded respectively
by the existing 1,800/2,400-vertex tests. No particle actors, scene lights, shadows,
collision or navigation are added. More detailed pixels cost shader work; no FPS
improvement or zero-cost claim is made without a GPU benchmark.

## Visual review

`FLICK.Visuals.EffectGallery` is an opt-in ClientContext review utility. In a
disposable `-game` process, pass `-FlickEffectReview` and optionally
`-FlickEffectReviewDir=...`. It writes all 45 active styles at two controlled
animation stages, plus a CSV identifying positions and names. It freezes only
that review process; it does not save cosmetics or camera settings. Finish it with
`-TestExit="Automation Test Queue Empty"`. Normal gameplay never invokes it.

Evidence and validation results are under
`Deliverables/Effects-Upgrade-2026-10-28`.
