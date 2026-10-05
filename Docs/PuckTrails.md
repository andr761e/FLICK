# Puck trails

Customize > Puck Trail now contains None plus twelve trails. Existing saved indices
0–5 retain their names; seven standalone styles are appended at 6–12. Spawn and
knockout selections are unchanged.

| Trail | Presentation |
|---|---|
| Ion Wake | Feathered cyan wake, silver core, fine speed filaments |
| Ember Wake | Flowing flame tongues and drifting orange embers |
| Cryo Ribbon | Layered frost streams and rotating crystal shards |
| Solar Cinders | Molten gold/orange flow and star-shaped cinders |
| Phase Stream | Interwoven violet/cyan strands and soft wisps |
| Lightning | Animated angular forks and electric sparks |
| Prism | Seven parallel rainbow bands and prismatic glints |
| Gold Rush | Champagne filaments and golden stars |
| Galaxy | Violet/cyan nebula ribbons and floating stars |
| Pixel Stream | Blue square tiles peeling off a thin digital stream |
| Heartbeat | Rose wake and floating, camera-facing hearts |
| Spirit Wake | Soft turquoise currents and rising spectral wisps |

## Implementation and tuning

`Core/FlickTrailStyle.cpp` holds each style's palette, lifetime, width, emission,
strand count, particle shape and particle spacing. Motion and sampling constants
live in `Pieces/FlickPieceCosmeticTrails.cpp`. The renderer is local presentation:
it follows authoritative puck positions without modifying physics or adding lights.
There are at most 48 history samples, seven strands and 48 particles per puck,
batched into two non-colliding mesh sections. Dedicated servers skip rendering.
Time-based interpolated sampling works at different frame rates; tails taper and
fade after movement stops. Teleports clear history. None clears and hides it.

The single trail-specific additive/unlit material supports vertex RGB and alpha,
soft transverse ribbon edges, animated flow/nebula modulation, and analytic
orb/crystal/pixel/heart/star masks. The trail-only locker camera pulls back slightly
to keep the moving puck and wake clear of the menu.
It has no texture or Niagara dependencies. It is included in packaging through
DefaultGame.ini. Regenerate a missing asset with:

```powershell
& "$env:FLICK_UNREAL_ENGINE_ROOT\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" .\FLICK.uproject -unattended -nop4 -ExecutePythonScript="Tools/Build-FlickTrailMaterial.py"
```

The script reuses an existing graph and validates/repairs its vertex-colour wire;
it does not delete or replace the graph.
Automated coverage: `FLICK.Cosmetics.PuckTrails` exercises every non-None trail at
30/60/120 FPS, transparent vertex fades, bounded geometry, cleanup and teleport
handling. Locker swatches share the same palettes as live trails.

## Verification (2026-10-04)

- FLICKEditor Win64 Development build succeeded.
- CosmeticCatalog, CollectionPresentation, InspectionAndOwnership and PuckTrails
  all passed; PuckTrails covers 36 style/frame-rate combinations and shader wiring.
- Actual locker captures were inspected for Prism, Heartbeat, Ember, Lightning,
  Galaxy, Spirit, Cryo and Pixel. Final capture logs contain no material compilation
  failures. Captures use isolated profiles, not the player's normal saved choices.
- Unreal still emits two pre-existing frame-zero `LogAutomationTest: Condition
  failed` startup errors before the selected tests begin. Those are not new trail
  test failures. Full test log: `Saved/Logs/PuckTrailTests.log`.
- A packaged build and live multiplayer session were not run.
