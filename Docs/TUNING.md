# FLICK Tuning

Most prototype values live on `AFlickGameMode`, `AFlickPiece`, and `AFlickArena`.

## Current Defaults

| Parameter | Default | Increase | Decrease |
| --- | ---: | --- | --- |
| ArenaRadius | 650 cm | More room, longer tactical shots | Faster contact, easier ring-outs |
| ArenaThickness | 50 cm | Chunkier table edge | Thinner visual platform |
| ArenaSurfaceZ | 250 cm | More visible falling space | Less visible falling space |
| FormationRadiusFraction | 0.45 | Starts teams nearer the rim and each other | Pulls formations toward the center |
| KillZ | 50 cm | Earlier eliminations | Later eliminations |
| PieceRadius | 45 cm | Easier hits, more blocking | More precise aiming |
| PieceThickness | 20 cm | Chunkier pucks | Flatter pucks |
| PieceMassKg | 4 kg | Harder to move with equal impulse if launch model changes | Easier future mass-sensitive movement |
| PieceFriction | 0.12 | Shorter slides | Longer slides |
| PieceRestitution | 0.32 | Bouncier impacts | Deader impacts |
| LinearDamping | 0.55 | Faster turn resolution | Longer coasts |
| AngularDamping | 1.8 | Less spinning and edge rotation | More natural roll and longer wobble |
| MaxLaunchSpeed | 1450 cm/s | More dangerous shots | Gentler shots |
| MaxDragDistance | 280 cm | More physical pull needed for max power | Easier full-power shots |
| MinDragDistance | 15 cm | Fewer accidental weak shots | Easier tiny taps |
| PowerExponent | 1.2 | More low-power precision | More linear power response |
| SleepLinearVelocityThreshold | 7.5 cm/s | Turns end sooner | Waits for more complete rest |
| SleepAngularVelocityThreshold | 22 deg/s | Ignores more spin | Waits for calmer spin |
| SettledDuration | 0.5 s | More stable turn changes | Faster turn changes |
| MaximumResolutionDuration | 8 s | More time for weird physics to settle | Faster timeout cleanup |
| PositionSolverIterations | 12 | More stable contacts at higher CPU cost | Cheaper but less stable rim contact |
| VelocitySolverIterations | 4 | More accurate impact response at higher CPU cost | Cheaper but less accurate response |
| bUseContinuousCollisionDetection | true | Prevents fast pucks tunneling through contacts | Lower physics cost |
| bAllowEdgeTipping | true | Enables pitch and roll for natural falls | Keeps pucks mechanically upright |
| bUseSimultaneousKickoff | true | Both players commit before the opening collision | Uses the normal alternating first shot |
| CameraLocation | (0, -1700, 1600) cm | Move farther/higher for more board margin | Move closer for larger pieces |
| CameraRotation | (-40, 90, 0) degrees | Tune with location to keep the arena below the HUD | Tune with location to keep the arena below the HUD |
| FieldOfView | 50 degrees | Wider framing | More focused framing |
| MinimumImpactFeedback | 90 cm/s velocity change | Fewer small-hit effects | More sensitive impact feedback |
| StrongImpactFeedback | 820 cm/s velocity change | Requires harder hits for maximum feedback | Strong feedback arrives sooner |
| MaximumShakeLocation | 10 cm | More camera movement | Calmer impacts |
| MaximumShakeRotation | 1.1 degrees | More rotational kick | Steadier view |
| ShakeDecayPerSecond | 2.8 | Camera settles faster | Camera response lingers |

## First Values To Tune

Start with `MaxLaunchSpeed`, `PieceFriction`, `LinearDamping`, and `PieceRestitution`.

If full-power hits launch too many pucks off the board, lower `MaxLaunchSpeed` or `PieceRestitution`.

If shots feel sticky, lower `PieceFriction` or `LinearDamping`.

If turns drag on, raise `LinearDamping` or `SleepLinearVelocityThreshold`.

If pucks barely affect each other, raise `MaxLaunchSpeed` before adding new mechanics.

Tune physics before feedback. A collision should first feel correct with camera impulses mentally ignored; then adjust `MinimumImpactFeedback`, `StrongImpactFeedback`, and the camera values so presentation reinforces the result without disguising it.

## Kickoff

`bUseSimultaneousKickoff` removes the uncontested opening hit without adding immunity or scripted collision exceptions. The alternating round opener commits a puck, direction, and power first. The opponent then commits a response and both impulses are applied before the next Chaos step. After the board settles, normal alternating play begins with the round opener.

`FormationRadiusFraction` controls how far each formation begins from the arena center. The default `0.45` leaves more reaction space than the previous outer formation. Tune it with each mode's arena radius and piece count; always verify mixed-size lineups begin without overlap.

## Mode Rules

| Parameter | 4v4 Knockout | 3v3 Knockout | BOB |
| --- | ---: | ---: | ---: |
| Ruleset | Knockout | Knockout | BOB |
| Pucks per team | 4 | 3 | 12 Standard |
| Rounds to win | 3 | 3 | 1 board |
| Simultaneous kickoff | Yes | Yes | No |
| Arena radius / half-extent | 650 cm | 590 cm | 620 cm square |
| Piece radius | 45 cm | 43 cm | 24 cm |
| Piece thickness | 20 cm | 20 cm | 14 cm |
| Max launch speed | 1450 cm/s | 1600 cm/s | 1320 cm/s |
| Max drag distance | 280 cm | 240 cm | 240 cm |
| Piece friction | 0.12 | 0.12 | 0.05 |
| Piece restitution | 0.32 | 0.32 | 0.28 |
| Linear damping | 0.55 | 0.62 | 0.18 |
| Angular damping | 1.8 | 2.0 | 1.35 |
| Resolution timeout | 8 s | 6.5 s | 10 s |

These values live in `FlickModeRules`, which is applied before every match rebuild. Shared body properties such as mass, CCD, and solver iterations remain on `AFlickGameMode`; friction and restitution are mode-specific so returning from BOB restores Knockout feel. For BOB, `ArenaRadius` is interpreted as square half-extent.

## Puck Archetypes

Archetype values multiply the selected mode's shared puck values. `COM` is a vertical center-of-mass offset expressed as a fraction of final puck thickness; zero keeps the authored body center.

| Archetype | Class | Mass | Friction | Restitution | Linear damping | Launch speed | Radius | Thickness | COM |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Standard | Core | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 1.00 | 0.00 |
| Heavy | Power | 1.70 | 1.30 | 0.72 | 1.05 | 0.82 | 1.08 | 1.18 | 0.00 |
| Striker | Speed | 0.70 | 0.70 | 1.25 | 0.78 | 1.18 | 0.92 | 0.82 | 0.00 |
| Grippy | Control | 1.00 | 3.80 | 0.58 | 1.45 | 0.90 | 1.00 | 1.00 | 0.00 |
| Slider | Momentum | 1.00 | 0.45 | 0.85 | 0.58 | 1.00 | 1.00 | 1.00 | 0.00 |
| Blocker | Defense | 0.75 | 1.00 | 0.78 | 1.05 | 0.84 | 1.18 | 0.92 | 0.00 |
| Compact | Precision | 1.00 | 0.88 | 1.00 | 0.92 | 1.05 | 0.80 | 0.90 | 0.00 |
| Bouncer | Trickshot | 0.85 | 0.82 | 1.45 | 0.78 | 1.02 | 0.96 | 0.90 | 0.00 |
| Toppler | Risk | 0.90 | 1.05 | 0.85 | 0.90 | 0.95 | 1.00 | 1.35 | 0.14 |

Launches use velocity change, so mass does not accidentally make Heavy impossible to control. Mass still affects subsequent Chaos collision momentum. Friction uses average combine mode against the arena's `0.10` surface friction, allowing Grippy to stop distinctly sooner and Slider to retain the longest coast. Restitution remains composed with the arena material, so Bouncer earns stronger banks through the same Chaos contacts as every other puck. Toppler's raised center of mass and taller body create its instability without scripted tipping.

The four one-action lineup presets are Balanced (Standard, Heavy, Striker, Grippy), Power (Heavy, Blocker, Slider, Standard), Speed (Striker, Slider, Compact, Bouncer), and Control (Grippy, Blocker, Compact, Toppler). Any individual slot edit is represented as Custom unless it exactly recreates one of these four sequences.

The lineup comparison bars are presentation values in the normalized `0.08-0.98` range, calculated by `GetDisplayStats`. Speed comes from launch speed, Weight from mass, Impact from mass times launch speed, Control from friction/damping/rebound resistance, Coast from inverse surface resistance, and Stability from footprint, thickness, angular damping, mass, center of mass, and rebound. They are not separate balance inputs and should not be tuned independently of the physical values above.

BOB does not apply this table. All colored pucks and both dark strikers use the Standard column. The BOB mode's smaller shared radius and thickness are arena-scale adjustments, not archetype or special-piece modifiers.

## BOB Arena

`AFlickBobArena` defaults to a `620 cm` half-extent, `66 cm` pocket radius, `150 cm` pocket inset, `42 cm` rail height, and `34 cm` rail thickness. The inset leaves `84 cm` between the outside of a pocket and the rail, enough for a Standard BOB puck to travel or settle behind it. The board surface uses `0.035` friction and `0.18` restitution; rails use `0.04` friction and `0.48` restitution to support long slides and deliberate banks. A pocket scores when a puck center crosses the visible radius. Increase `PocketRadius` for an easier game and decrease it for stricter precision.

## Audio Mix

The default persistent mix is Master `0.85`, Physics FX `0.85`, and Interface `0.70`. `AFlickAudioDirector` derives pitch and envelope from event context:

| Cue | Primary inputs |
| --- | --- |
| Launch | Archetype and normalized shot power |
| Impact | Collision strength and combined puck mass |
| Scrape | Fastest active puck's speed |
| Ring-out | Fixed descending impact cue at the arena edge |
| Turn/result | Team, draw state, and series completion |

Procedural effects are intentionally short and retained in a bounded runtime pool. Tune event volume in the director before changing the persisted mixer defaults.

## Presentation

The Knockout arena uses one authoritative collision cylinder plus non-colliding graphite fields, concentric rings, center plate, four inward chevrons, a continuous cyan rim accent, and 28 alternating cyan rim segments. Pucks add a non-colliding side band and restrained underglow around the authoritative body. These components may be recolored or resized without changing physics.

## Edge Tipping

Tune tipping with `PieceRadius`, `PieceThickness`, `PieceMassKg`, friction, and angular damping. The current center of mass is the cylinder's geometric center. For a quicker drop at the rim, increase thickness or lower angular damping slightly. For calmer table motion while retaining tipping, raise angular damping in small increments and verify that half-supported pucks still rotate off under gravity.
