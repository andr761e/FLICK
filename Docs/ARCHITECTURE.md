# FLICK Architecture

This prototype is intentionally code-first. It should run in a plain map without Level Blueprint gameplay logic.

## Ownership

`UFlickGameInstance` stores preferences that survive map and match rebuilds: selected mode, both four-slot team loadouts, aim guide visibility, impact effects, camera shake intensity, and the master/effects/interface audio mix. It persists these values in the game user settings ini.

`UFlickSessionSubsystem` owns the platform-session boundary across map travel. It reads Steam identity, advertises and filters FLICK lobbies, resolves joins and accepted invitations, opens the Steam invite overlay, and destroys local session state on leave or failure. It does not own replicated lobby readiness or gameplay. Steam discovery hands a resolved address to Unreal networking, while `-nosteam` keeps the direct-IP path on `IpNetDriver` for local development.

`AFlickGameMode` owns the local-only frontend and match flow: main menu, pause, settings routing, selecting the Knockout or BOB ruleset, spawning the appropriate board and pieces, launching shots, resolving physics, changing turns, scoring, wins, draws, and restarts.

`AFlickGameState` stores readable round and series state for presentation: current phase, alternating round opener, first-to-three Knockout score, active piece counts, winner/draw state, shot counts, last-shot power, impacts, and eliminations.

`AFlickPlayerController` owns mouse and controller input: hover/selection, projecting the cursor onto the arena plane, calculating aim, sweeping for first puck contact, cycling the four gameplay camera sides, canceling, and restart input. It exposes read-only aim and prediction state to the HUD.

`AFlickCameraPawn` owns the cinematic menu framing, four gameplay viewpoints, smooth 90-degree transitions, post processing, and cosmetic impact shake. Gameplay viewpoints are rotations of one tuned base transform around the board origin, which keeps all modes framed consistently. Mouse deprojection and gamepad screen axes read the live camera transform, so changing sides does not require alternate shot math.

`AFlickPiece` owns puck physics configuration, archetype identity, launch application, team identity, layered visual state, impact detection, and one-time elimination state.

`AFlickArena` owns the runtime platform collider, physical material, layered graphite fields, concentric markings, cyan segmented rim, cardinal chevrons, pedestal, and backdrop. Only the main cylinder participates in collision.

`AFlickBobArena` owns the separate square BOB deck, colliding raised rails, four radial pocket targets, center rack markings, player baselines, pedestal, and backdrop. The deck remains a predictable flat collider; GameMode authoritatively detects puck centers entering the visible pocket mouths and removes them from play.

`AFlickHUD` owns presentation lifetime and the world-projected aim guide. It attaches `SFlickGameLayer`, a native Slate layer that renders the frontend, settings, loadouts, score plates, power meter, event feed, pause screen, and round results with DPI-aware typography. Reusable code-native primitives provide cut-corner panels, focused command states, puck discs, comparison bars, and a six-axis lineup radar so every screen shares one visual language without binary widget dependencies. Canvas remains limited to geometry that must be projected directly from world space. The shared palette and reference dimensions live in `FlickUITheme.h`; a 1600 x 900 Slate canvas scales to fit the viewport. The Switchyard test arena uses color-linked switches and raised divider caps without on-screen divider text. See `Docs/UI_DESIGN.md` for presentation boundaries and repeatable capture commands.

`AFlickWorldFeedback` is a short-lived, code-only burst effect used for launches, impacts, and ring-outs. Camera response and all world feedback are cosmetic and do not feed back into Chaos physics.

`AFlickAudioDirector` plays short procedural sound waves rendered by the pure `FlickSoundSynthesis` module. Physics events provide strength, mass, speed, class, and location inputs; the audio layer never modifies gameplay state. See `Docs/AUDIO_DESIGN.md` for cue design and waveform validation.

`FlickLaunchMath` is pure shot math so input behavior can be tested without Chaos physics.

`FlickModeRules` is the single source of truth for per-mode piece counts, arena dimensions, shot speed, damping, and resolution timing. It also builds the supported spawn formations and is covered by automation tests.

`FlickBobRules` is the pure, tested BOB end-condition policy. It interprets remaining owner-colored pucks in the opposite direction from Knockout: reaching zero means that owner wins, and a same-shot double clear is awarded to the shooter.

`FlickPieceArchetypeRules` defines the physical and visual multipliers for all nine Knockout pucks: Standard, Heavy, Striker, Grippy, Slider, Blocker, Compact, Bouncer, and Toppler. It also owns four internal lineup presets, presented to players as Class 1 through Class 4, plus plain-language strengths, weaknesses, and normalized ratings derived from the same physical multipliers. GameMode composes the selected archetype with the active mode's base values before spawning each Chaos body; selecting an individual slot leaves preset matching in `Custom` unless the choices exactly match a class preset.

`FlickSeriesRules` is the pure, tested configurable-series and alternating-opener policy used by GameState. Knockout supplies three wins; BOB supplies one completed board.

## Frontend Flow

The runtime starts in `MainMenu` with a non-interactive cinematic preview behind a translucent, diagonal left-side navigation panel. A separate active-preview variant rotates through 4v4, 3v3, and BOB while the player's selected and persisted next match remains unchanged. Each automatic change uses an eased veil and swaps the arena at the transition midpoint instead of exposing an abrupt rebuild. Play enters `ModeSelect`, which exposes those three formats and restores the selected mode as its background. Knockout can start with saved lineups or route into `Loadout`; BOB hides that route and forces Standard pucks without modifying saved Knockout choices. Loadout edits Player 1's active formation as one focused workspace and remembers whether it was opened from Home or Mode Select so Back restores the correct frontend context. Its 3v3/4v4 formation diagrams select slots directly, Class 1 through Class 4 replace the whole active formation, and hovering any of the nine puck types previews strengths, weaknesses, slot deltas, and an aggregate physics-derived radar without committing the change. `ItemShop` is currently an isolated cosmetic-store placeholder with no inventory or purchasing state. Starting a match blends to the competitive camera and rebuilds the selected arena. Every new aiming turn begins at that player's home-side view; before selecting a puck, the player may rotate through all four board sides. Selection is suspended during the short camera blend, and an active drag prevents rotation. Knockout rounds open in `KickoffPlanning` and continue until a player reaches three wins. BOB opens directly in `Aiming` with both persistent strikers on their owner baselines and ends after one player clears their color. Escape enters `Paused`; Settings remembers whether Back should return to pause or the main menu. Returning home rebuilds a fresh preview and clears any in-progress aim.

Training is a standalone-only branch of the same arena and physics pipeline. The player chooses an arena format and may use the saved Knockout lineup, then controls any blue puck while orange pucks remain passive targets. Training skips kickoff, alternating turns, round scoring, and result popups. It returns to aiming after every settled shot, preserves session statistics across board resets, resets automatically when either side is cleared, and maps the normal restart input to a manual board reset. No session, matchmaking, ranking, or backend system participates in this flow.

## Contact Preview

The controller sweeps a sphere from the selected puck along the current launch direction, querying only `ECC_PhysicsBody`. The guide distance scales with drag power and the puck's launch-speed multiplier. The result is strictly presentational: it does not predict rebounds, arena-edge outcomes, or future Chaos positions.

## Shot Flow

1. Player clicks a puck and the controller asks GameMode whether it is selectable.
2. Controller deprojects the cursor onto the arena plane and `FlickLaunchMath` converts the pull vector into direction and normalized power.
3. During `KickoffPlanning`, the opener's release stores an immutable puck, direction, and power without applying physics. The locked puck and projected launch line remain visible.
4. The opponent plans normally. Their release records both shots and calls `AFlickPiece::Launch` for both pucks in the same GameMode update, before Chaos advances.
5. During normal `Aiming`, a validated release immediately calls `AFlickPiece::Launch` for the current player's puck.
6. A launch applies one mass-independent velocity-change impulse and GameMode enters `ResolvingPhysics`.
7. GameMode waits until active pucks remain below velocity thresholds for `SettledDuration`.
8. GameMode checks eliminations and win/draw. A resolved kickoff returns the normal first turn to that round's alternating opener; later shots alternate as usual.

## Feedback Flow

Puck-to-puck hit events calculate a mass-normalized impact value. Meaningful impacts flash both pucks, spawn a short burst, update last-shot statistics, and add a deterministic camera impulse. Ring-outs spawn feedback at the relevant arena edge before the fallen puck is hidden. Feedback has global cooldowns so sustained contacts do not flood the screen.

## BOB Turn Flow

1. Each player owns one mechanically Standard striker. Only the active player's striker is selectable, wherever that striker previously settled.
2. Existing controller selection, drag math, contact preview, and launch impulse handle the shot unchanged.
3. During physics resolution, GameMode checks each active center against the four `AFlickBobArena` pocket radii.
4. Colored pucks are eliminated and reduce their owner's remaining count, regardless of the shooter.
5. A pocketed striker is hidden until settlement and marked for return to its owner's baseline.
6. If the shooter pocketed their own striker, an available pocketed puck belonging to that shooter is also restored as the penalty; pocketing the other striker does not penalize the shooter.
7. `FlickBobRules` checks the win condition. If neither color is cleared, control alternates without moving either non-pocketed striker.

## Elimination

Knockout pucks are eliminated when their actor Z reaches `KillZ`. BOB pucks are eliminated when their centers enter a pocket radius. Both paths use the same one-shot piece operation: mark eliminated, hide, disable collision, stop physics, and update active counts. BOB may reactivate one owner puck when applying a striker penalty.

## Physics Resolution

Resolution is based on linear and angular velocity thresholds across all active pieces. A timeout cleanup exists for tiny jitter; it only sleeps pieces that are already below a loose movement threshold.

Puck bodies are short simulated cylinders with pitch and roll unlocked. Gravity therefore creates real torque as the center of mass moves beyond the arena support edge. Continuous collision detection and elevated position/velocity solver iteration counts reduce tunneling and unstable rim contacts at high launch speeds. Elimination still happens at `KillZ`, after the physical fall is underway. Surface friction uses average combine mode so the Grippy role can create meaningfully stronger contact without changing arena collision logic.

## Multiplayer Boundary

The host is currently an Unreal listen server. Players submit piece identity, aim direction, and normalized power through server RPCs; GameMode validates ownership and turn state before launching the authoritative replicated puck. Match phase, teams, readiness, scores, and lobby mode are replicated. Steam supplies identity, discovery, invitations, and relay transport but does not decide gameplay outcomes. Direct-IP sessions exercise the same replicated lobby and match code through the fallback IP driver.

`UFlickMatchmakingCoordinatorSubsystem` is the production matchmaking boundary. It submits complete parties, polls allocations, retains reconnect reservations, and lets dedicated authorities verify admission and send heartbeats. `UFlickSessionSubsystem` remains responsible for Steam friends, invitations, private parties, browsing, and the development session-matching fallback. Ranked identity and settlement remain server-only through `UFlickRankedBackendSubsystem`; neither Steam lobby metadata nor client travel options are authoritative.

## Why This Shape

The game needs quick iteration on physics feel. The architecture keeps the core loop small, tuneable, and readable by one developer while leaving space for future piece types without building an ability framework prematurely.
