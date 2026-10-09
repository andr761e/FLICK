# Divider lifts

Switchyard switches can now raise an occupied divider. After the existing activation cue/delay, the simple collider rises from below the deck at constant speed. Chaos contact lifts/tips the puck; no launch impulse or puck teleport is added. Retraction uses the same motion.

- `AFlickTestArena::DividerDeploymentDelay`: existing cue delay (0.10 s).
- `AFlickTestArena::DividerRiseDuration`: full lift duration (0.075 s, tuneable; previously 0.05 s).
- `AFlickTestArena::DividerCrownDeflectionForce`: continuous contact-only sideways force (8000 force units), with an upward component while rising. Tips perched pucks toward their greater overhang, then lean/motion, with an inward tie-breaker. No impulse, teleport or velocity replacement; puck mass/inertia still apply. Glancing contacts may slide off rather than gain height.
- `AFlickTestArena::DividerRetractionDuration`: full retraction duration (0.30 s, tuneable).
- Divider dimensions, friction, restitution and puck physics remain unchanged.
- Shot resolution waits for pending and moving dividers. Tabletop flight containment/self-righting/flattening excludes pucks near raised or retracting divider footprints, preserving actual support.
- Server advances collision during shot resolution; replicated lift fractions and target masks drive client presentation. Replay frames capture/interpolate the lift fractions. Reset, board edit and undo restore exact static states.

Automation: `FLICK.Arena.DividerLift` checks occupied-switch activation, cue delay, bounded collider movement, sleeping Standard/Heavy puck contact lifts (centred and partially overlapping), finite velocities and reset at 30/60/120 Hz. Also run `FLICK.Arena.DividerRimClearance` and `FLICK.Training`.
