# BOB arena presentation

BOB's ivory/walnut/foundry finish changes materials, the existing local lighting
rig and camera grading only. It does not change the board, stadium or puck meshes.

For the subsequent fixed-rate physics, resolution recovery and faster Epic
reflection-lighting configuration, see [PhysicsTiming.md](PhysicsTiming.md).
The presentation-pass benchmark below records the earlier lighting mode, not
the optimized mode described in that document.

## Materials

`Tools/Build-FlickBobArchitecturalMaterials.py` is the canonical authoring pass.
It edits the two existing parents and eighteen existing material instances in
`/Game/BOB/Arena` and `/Game/BOB/Stadium`. Run it with Unreal's
`-run=pythonscript -script=<absolute script path>` commandlet. It never calls
either geometry importer. Both importers reapply this pass to their own material
group after a future reimport, so source imports do not restore the old palette.

The playing bed uses warm ivory sealed-maple grain; the frame uses walnut and
bronze; the stadium floor uses fine-grain charcoal composite. Existing markings
remain burgundy and pockets keep their restrained red rims. Amber/cyan fixture
identities and the original mesh material slots are preserved.

The shared graph provides `BaseColor`, `Metallic`, `Roughness`, `Specular`,
`Emission`, `RoughnessVariation`, `FinishType`, `FinishDetail`, `NormalStrength`,
`GrainScale` and `IndirectEmissionScale`. Finish types are plain lacquer (0),
maple (1), walnut (2), stone (3) and brushed metal (4). The script's two spec
dictionaries are the editable palette. Detail is procedural, world-space and
derivative-filtered; no new textures are required. Normal variation is shading
only: there is no displacement or world-position offset. The traced emission
path is bounded separately from visible LED emission.

The bed retains 0.26 roughness for live reflections. The large stadium floor
uses 0.44 roughness with bounded variation (minimum above 0.4); concrete uses
0.45. This keeps soft direct specular light and the material's satin character
without tracing sharp Lumen reflections over the entire stadium floor at the
default reflection cutoff. No global reflection settings are reduced.

Graph authoring reuses parameter nodes and tagged helper nodes. BOB assets load
during CDO initialization, rooting their original expressions; deleting those
expressions through Unreal 5.8's editing API asserts. A few disconnected original
nodes are retained as editor-only data. Repeated authoring does not accumulate
helper nodes or add shader work for those disconnected expressions.

## Lighting and grading

`FlickArenaLighting::FParameters` contains the BOB-specific tuning values.
The existing key, rim, fill and two accent actors are reused; no additional BOB
lights, shadow maps or volumetric effects are introduced.

- Broad warm key: 650 lm; cool rim: 250 lm; diffuse fill: 650 lm.
- Amber/cyan side-rail sources: 120/80 lm, 760 cm long, 500 cm range.
- Ambient intensity: 0.28; directional intensity: 0.75.
- Local dimensions scale from the authored 620 cm board half-extent and the
  actual playing height. Local power scales with board area.
- Existing lighting preferences multiply this baseline without being overwritten.
- BOB uses the existing environment cubemap for reflections. Its camera has
  neutral gamma, no artificial film grain, restrained bloom and a -0.65 exposure
  bias. Player scalability remains authoritative: BOB does not inherit
  Knockout's reflection-quality or scene-trace overrides.

On returning to Knockout, the reused local fixtures recover unitless intensity,
ordinary accent dimensions and orientation. Knockout-specific rim/stadium
fixtures stay disabled in BOB. Dedicated servers skip the presentation rig.

## Physics invariants

High-detail art remains non-colliding. The welded planar floor and four physical
rails still control puck contact independently of the rendered material:

- Floor friction/restitution: 0.035 / 0.18.
- Rail friction/restitution: 0.04 / 0.48.
- Both retain explicit Average combining.
- Puck mass, size, damping, shot strength, pocket cutouts, board dimensions and
  gameplay rules are unchanged.

## Verification

Relevant automation: `FLICK.BOB`, `FLICK.MatchRules.BobOutcomes`,
`FLICK.Visuals.BobLightingTheme`, `FLICK.Visuals.KnockoutLightingTheme`,
`FLICK.Visuals.LightingRigLookup` and `FLICK.Arena.ConceptPresentation`.
The new BOB regressions cover shared material identity, unchanged collision
geometry/physical values, repeated initialization, fixture reuse, preference
scaling and BOB/Knockout mode transitions.

Live capture: `Tools/Capture-FlickUI.ps1 -Screen Bob,BobPocket -Width 1920
-Height 1080 -CameraDistance 1.25 -HideControlOverview`.
For temporal inspection add `-CaptureFrames 8`; captures use isolated profiles.

2026-10-21 validation: the Development editor build succeeded. Nine tests passed
with no failed or skipped tests in `Saved/Automation/BobArchitecturalSatinFinal`.
This includes slow pocket entry, the planar collision surface, BOB rules,
material/physics invariants, lighting preferences and Knockout transitions.
No packaged build was produced by this presentation pass.

Live final frames: `Saved/UICaptures/20261021-123358-257245/`. Across eight frames,
the sampled amber/cyan floor regions had no pixel changes above 32/255. The
sampled board highlight had at most 14 such pixels out of 43,200; the front trim
had at most 1 out of 20,000. Small temporal noise remains: this is a sampled
stability check, not a zero-flicker guarantee on every GPU/view.

Local performance check: RTX 3070, 1920x1080, Epic scalability, identical saved
settings, real offscreen gameplay, 1,500 frames per run. Statistics below use
the last 600 frames, excluding startup/shader warmup. GPU times are more useful
than CPU/render-thread timings here because those include waits/system jitter.

| Milliseconds | Original | First polished-floor pass | Final satin floor |
| --- | ---: | ---: | ---: |
| GPU mean | 11.558 | 14.327 | 12.654 |
| GPU median | 11.424 | 14.286 | 12.455 |
| GPU P95 | 11.993 | 15.267 | 13.784 |
| Lumen reflections median | 2.414 | 5.455 | 3.217 |

Final GPU cost is about 9.5% above the original, rather than the first pass's
24% increase. The remaining cost is principally the genuinely polished playing
bed. Logs/CSVs are retained under `Saved/Performance/BOB-Before`, `BOB-After`
and `BOB-After-Satin`. No physics settings were changed to offset rendering cost.
