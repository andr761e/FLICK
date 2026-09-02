# FLICK Setup

FLICK is currently a code-first Unreal Engine 5.6 prototype. The vertical slice spawns its camera, light, arena, pucks, HUD, and match state from C++ at runtime.

## Requirements

- Unreal Engine 5.6
- Visual Studio 2022 or Visual Studio Build Tools with the MSVC C++ toolchain
- VS Code is optional but supported by the generated `FLICK.code-workspace`
- Steam installed and running for online sessions
- Two Steam accounts on separate computers for a complete online join/invite test

## Build

From the project root:

```powershell
& 'C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat' FLICKEditor Win64 Development 'C:\Users\andre\Desktop\FLICK\FLICK.uproject' -waitmutex -NoHotReload
```

Or in VS Code, run the generated `FLICKEditor Win64 Development Build` task if available.

## Local Network Test

After building, run this from the project root:

```powershell
.\play-flick-network.cmd
```

Both windows now open at the main menu. In the first window, open `PLAY`, choose a mode,
and select `HOST LOCAL`. In the second window, open `PLAY` and select `JOIN LOCALHOST`.
The host waits in the lobby until the second player joins. Both players must select `READY UP`,
then the host selects `START MATCH`. The host controls the lobby's competition selection.

This opens two independent game instances. After hosting, Player 1 becomes the listen server and controls only the cyan team. Player 2 connects over localhost and controls only the orange team. Matches return to the same connected lobby when the host selects `RETURN TO LOBBY`.

The network match uses server-authoritative turns, launches, physics resolution, eliminations, round scoring, and rematches. The client submits only its selected replicated puck, aim direction, and normalized power. Invalid, out-of-turn, or wrong-team requests are rejected by the server.

The command deliberately passes `-nosteam`, so both local instances can run under one Windows account without competing for a Steam login. Steam sessions use the normal single-window launch described below.

## Steam Online Test

FLICK uses Unreal Engine's bundled `OnlineSubsystemSteam` and `SteamSockets` plugins. No separate Steamworks SDK download is required for the current engine integration. Steam provides identity, lobby discovery, invitations, and relay transport; the existing Unreal listen server remains authoritative for gameplay.

The development configuration currently uses Valve's shared test App ID `480`. It is suitable for integration work, but it must be replaced with FLICK's assigned App ID before wider testing or release. FLICK tags and filters every advertised lobby so unrelated App ID 480 sessions do not appear in the browser.

1. Start Steam and sign in.
2. Launch one normal FLICK window with `play-flick.cmd`. Do not pass `-nosteam`.
3. Open `PLAY`, select a mode, and choose `HOST ONLINE`.
4. From the Steam lobby, choose `INVITE FRIENDS`, or have another account choose `FIND ONLINE` and join the listed lobby.
5. Ready both players and let the host start the match.

Steam does not support a meaningful two-user test by opening two clients under one signed-in account. Use two Steam accounts on separate computers for final discovery, invite acceptance, joining, host-departure, and latency testing. `play-flick-network.cmd` remains the fast single-machine replication test.

The online browser reports Steam initialization, search, join, full-session, address-resolution, travel, and connection failures in the UI. `HOST LOCAL` and `JOIN LOCAL` remain available from that screen when direct-IP development is needed.

## Public Unranked Matchmaking

Open `PLAY`, enter the online browser, choose `1V1`, `2V2`, or `3V3`, then select `FIND MATCH`. FLICK searches Steam for an accepting lobby with the same competition and team size. It joins the compatible result with the lowest reported ping, or creates an advertised queue when none is available. `CANCEL QUEUE` remains available while searching or waiting.

When the roster fills, the Steam lobby stops advertising and becomes a match-found confirmation screen. Every player must ready within 30 seconds; the host starts only after all slots confirm. Searching for an incomplete roster times out after 90 seconds and leaves the player in control of cancellation. Competition cards are locked after queue entry so the advertised rules cannot change underneath connected players.

The listen server owns the match ID, result, completion timestamp, and disconnect-forfeit decision. A finalized result is immutable. Premade members retain their party slots throughout matchmaking and return together after the match; players who joined as opponents leave that host party. Solo players return to the frontend.

Party leaders can queue their connected group for unranked 2v2 or 3v3 from the Social drawer. A persistent FLICK party identity is kept separately from the disposable Steam session used for discovery and gameplay. A premade first searches as a connected group; when it finds a compatible host, every member migrates into that exact match session with the same party ID and reserved team slots. If no match exists, the leader opens the current party as the accepting matchmaking lobby.

After a match, each premade is rebuilt as its own private party and solo players return to the frontend. Party members retry discovery while their leader recreates the party. If the gameplay host drops, the visiting party leader rebuilds its group and the original host party's second slot may take over restoration. Validate migration, restoration, invites, and host loss with separate Steam accounts because one signed-in account cannot represent multiple Steam users correctly.

For a single-machine lifecycle test without Steam discovery, run:

```powershell
.\play-flick-matchmaking-local.cmd 1
.\play-flick-matchmaking-local.cmd 2
.\play-flick-matchmaking-local.cmd 3
```

These commands open the required independent clients, auto-confirm ready, and exercise the same matchmaking roster, lobby lock, and authoritative start. Close a non-host client during play to verify the server-side forfeit result. Real Steam discovery, relay ping, and account identity still require separate Steam accounts on separate computers.

## Dedicated Matchmaking Coordinator

Public matchmaking now has a production-shaped coordinator path in addition to the Steam-session fallback. When configured, a solo player or complete connected party submits one queue ticket, polls for an allocation, and receives a separate opaque reservation for each member. The party leader relays the allocation to every connected member, so the group leaves its party lobby together and joins the same dedicated authority. The game server does not trust travel options: it verifies each reservation with the coordinator before assigning the canonical account, team, or player slot.

Run `play-flick-coordinator-local.cmd [1|2|3] [casual|ranked] [variant]` to exercise the natural queue-to-server flow. The command starts `Tools/FlickCoordinator`, opens the requested number of independent clients, queues them, launches an `UnrealEditor-Cmd.exe -server` authority on an allocated port, and lets each client confirm ready normally. Variant values are `0` for 4v4 Knockout, `1` for BOB, and `2` for 3v3 Knockout. `run-flick-local-coordinator.cmd` starts only the service, while `test-flick-coordinator.cmd` runs a fast HTTP allocation and reservation smoke test without launching Unreal.

The coordinator is enabled locally by `-FlickCoordinatorUrl=http://127.0.0.1:8090`; without that option, existing Steam lobby matchmaking remains unchanged. Shipping builds require the remote HTTPS path. Allocated clients retain their reservation for three reconnect attempts, and the authority waits the configured 45-second grace period before recording a disconnect forfeit. Servers heartbeat while active and report normal or forfeited completion once. The full protocol and production security requirements are in `Docs/MatchmakingCoordinatorApi.md`.

For a deployable single-host online stack, install a source-built Unreal Engine 5.6 checkout and run:

```powershell
$env:FLICK_UNREAL_ENGINE_ROOT = 'D:\UnrealEngine-5.6'
.\package-flick-online-stack.cmd
```

This packages `FLICKServer` and publishes the .NET coordinator under `Builds\OnlineStack`. Use `Deploy\production.env.example` as the host's environment-variable checklist, then start the coordinator with `run-flick-coordinator-production.cmd`. Production startup fails closed unless HTTPS, a packaged server, a public server host, a Steam App ID, and a publisher Web API key are configured. Open the allocated UDP game ports in the host firewall and place the HTTP coordinator behind TLS termination. See `Deploy\README.md` for the boundary of the included single-host deployment.

## Steam Social And Parties

The main menu `SOCIAL` drawer reads the signed-in Steam account's friends list and presence. Online friends and locally remembered recent FLICK opponents can be invited directly. Sending the first invite creates a private three-person Steam party lobby; later invites reuse that lobby. Accepting an invite leaves any existing FLICK lobby first, then connects to the party leader.

Party membership and leader state replicate to every connected player. The leader can remove members or disband the party, guests can leave from their party screen, and connection or host failures return affected players to the frontend. Recent players are stored locally because Unreal's Steam friends interface does not provide Steam recent-player history.

The party leader can queue the group for public unranked 2v2 or 3v3 from the Social drawer. Existing party members stay together on Team 1, the Steam session expands to four or six players, and open positions are filled through matchmaking. Every player receives a replicated team slot and controls only the pucks assigned to that slot. All players must ready before the Team 1 slot-one host can start.

For a single-machine replication test, run either:

```powershell
.\play-flick-team-network.cmd 2
.\play-flick-team-network.cmd 3
```

The command uses `-nosteam` and opens four or six compact clients directly into a private local lobby. Ready every window, then start from the host. A disconnect returns the group to the lobby; reconnecting the same network identity restores its reserved team slot while that host process remains alive.

## Package A Development Build

Run this command from the project root whenever an external Windows test build is needed:

```powershell
.\package-flick-development.cmd
```

The command builds, cooks, stages, and archives the Win64 Development target to `Builds\Development\Windows`. It also copies the shared Steam test App ID beside the packaged launcher and game executable. A successful run ends with the exact output path.

Test `Builds\Development\Windows\FLICK.exe` locally with Steam running before distribution. Share the entire `Windows` directory as one zip archive; individual executables are not standalone. The recipient does not need Unreal Engine or this source project, but does need Windows, Steam running under a separate account, and permission for FLICK through Windows Firewall.

## Launch From Unreal Editor

1. Close Unreal Editor if a previous C++ hot reload feels stale.
2. Build `FLICKEditor`.
3. Open `C:\Users\andre\Desktop\FLICK\FLICK.uproject`.
4. Let Unreal compile modules if it asks.
5. Open the default map if Unreal does not open one automatically.
6. Press Play.

Expected startup:

- An elevated marked tabletop, pedestal, and dark backdrop appear.
- The main menu appears over a live preview that cycles through all three arenas without changing the selected next match.
- Play opens a dedicated 3v3 Knockout, 4v4 Knockout, and BOB mode browser.
- Home and matches use separate cinematic and competitive tabletop camera framing.
- A mode can start with saved lineups, while Lineups remains directly accessible for role changes.
- Knockout matches are Best of 5 (first to three round wins), with simultaneous opening flicks and the first planner alternating each round.
- BOB is one standard-puck board: clear all twelve pucks of your color before the opponent.

No Blueprint setup is currently required.

## Match Modes

| Mode | Pieces per team | Match format |
| --- | ---: | --- |
| 3v3 Knockout | 3 | Best of 5 on the compact, faster circular arena |
| 4v4 Knockout | 4 | Best of 5 on the full tactical circular arena |
| BOB | 12 plus one striker per player | One square-board race to pocket your color |

## Puck Roles

Each active lineup slot can be cycled independently for both players before a match. Lineups persist between launches.

| Role | Strength | Tradeoff |
| --- | --- | --- |
| Standard | Balanced and dependable | No specialized advantage |
| Heavy | High mass, stable impacts, larger body | Slower launch and less bounce |
| Striker | Fast launch, long coast, lively rebounds | Light and easier to knock away |
| Grippy | Precise stops and strong surface grip | Lower speed and less bounce |

These are physical differences, not scripted hit bonuses. Mass changes collision momentum, friction and damping change travel, restitution changes rebound, and geometry changes contact and edge behavior.

Puck roles and lineup editing apply only to Knockout. BOB always spawns the Standard archetype for every colored puck and both strikers; saved Knockout lineups are ignored but preserved while BOB is selected.

## BOB Rules

BOB uses a separate square arena with four inset pockets and colliding raised rails. The inset leaves a playable lane between each pocket and the rail. Twelve Player 1 and twelve Player 2 Standard pucks begin mixed inside the center circle. Each player has a dark team-trimmed striker using the same Standard puck mechanics as every other BOB puck.

Both strikers begin on their owners' baselines. Flick your striker from wherever it settled on your previous turn; ordinary turn changes never teleport it. Any colored puck whose center enters a pocket is removed and counts for that puck's owner, including an opponent puck pocketed by the shooter. Turns alternate after the table settles; this first version does not grant an extra shot for pocketing your own color.

Pocketing your own striker loses the turn and returns that striker to its owner's baseline. If that player has already pocketed one of their colored pucks, one is also returned to a free central position as the penalty. A striker belonging to the other player is returned without penalizing the shooter. Clearing both colors in one collision chain awards the board to the shooter. The first player to clear all twelve of their own color wins the BOB match.

## Controls

- Left mouse press: select one of the current player's pucks
- Hold and drag backwards: aim and choose power
- Left mouse release: flick
- Right mouse or Escape: cancel aim
- Q / E: rotate the gameplay camera one side counter-clockwise / clockwise
- Middle mouse: rotate the gameplay camera clockwise
- R: restart match
- Escape outside an active aim: pause or resume

On controller, D-pad up/down rotates the gameplay camera between the four board sides. D-pad left/right and the shoulder buttons continue to cycle selectable pucks.

Menus are mouse-driven. The pause menu provides Resume, Restart, Settings, and Main Menu. Settings include the aim/contact guide, impact effects, camera shake strength, master volume, physics-effects volume, interface volume, VSync, window mode, and resolution. Gameplay and audio preferences persist between launches; display changes take effect when `Apply Display` is selected.

At the start of each round, the first player aims and releases to lock a kickoff shot; no puck moves yet. The other player then aims and releases, which launches both committed shots together. Once the kickoff settles, normal alternating turns begin with that round's first planner. The first planner alternates between players each round.

In the lineup builder, use each slot's left and right controls to choose its role. Escape or Back returns to mode selection.

While dragging, the pull tether follows the cursor, the archetype-colored guide shows initial launch direction, and the bottom meter shows effective launch speed and relative power. A swept marker identifies the first puck contact inside the guide distance. It is a readability aid, not a physics simulation, and can be disabled in Settings.

Launches, collisions, sliding, ring-outs, turn changes, round results, match victories, and menu actions use runtime-generated audio. No external sound asset import is required.

Pucks use free three-axis Chaos rotation, continuous collision detection, and increased solver iterations. A puck whose center of mass passes the support edge can tip and fall naturally; it no longer has to remain flat until its whole footprint leaves the arena.

For automated presentation checks in non-shipping builds, `-FlickModeSelectPreview` opens the playlist selection, `-FlickOnlineBrowserPreview` opens and refreshes Steam discovery, `-FlickOnlineHostSmokeTest` creates a temporary Steam lobby, `-FlickSocialPreview` opens the main-menu social drawer, `-FlickLoadoutPreview` opens the selected Knockout lineup, `-Flick3v3LoadoutPreview` and `-Flick4v4LoadoutPreview` force each formation, `-FlickLoadoutComparePreview` holds Heavy over Player 1's selected slot for stat-comparison captures, `-FlickItemShopPreview` opens the store placeholder, `-FlickSettingsPreview` opens settings, `-Flick4v4Preview` and `-Flick3v3Preview` open their match views, `-FlickTrainingPreview` starts the standalone free-play training flow, `-FlickPausePreview` opens the match pause overlay without stopping the capture timer, `-FlickArchetypePreview` arranges all nine Knockout puck types on one board without changing saved lineups, `-FlickMenuCyclePreview` accelerates the home carousel, `-FlickAutoStart` starts the selected match, `-FlickAutoKickoff` submits a deterministic Knockout kickoff, `-FlickBobPreview` starts and breaks a BOB rack, `-FlickBobPocketTest` forces one owner-scored pocket, `-FlickBobPersistenceTest` verifies a complete persistent-striker turn and exits, and `-FlickMatchResultPreview` displays a completed match. `-FlickCameraView=0` through `-FlickCameraView=3` selects an exact gameplay board side for deterministic captures. `-FlickCaptureFrame` captures the constructed viewport after a short delay and exits; it can be combined with presentation flags.

## Notes

The current default map is the engine template map. The arena surface is elevated so pucks can be eliminated before hitting any template floor beneath it. A future project-owned empty map is useful, but the current milestone intentionally avoids fabricating binary `.umap` files.
# Ranked Multiplayer Development

FLICK supports separate season-scoped ratings for every game-mode and human-team-size playlist. Ranked queues advertise their playlist MMR through Steam, begin at a `+/-100` search range, and widen through repeated searches before opening a new lobby. The first five completed matches are placements.

Ranked identity, match registration, and result settlement now pass through `UFlickRankedBackendSubsystem` on the authoritative game server. Clients request a Steam WebAPI ticket, the server passes it to the configured authority, and a ranked board cannot begin until every roster slot is verified. The server registers one immutable roster before gameplay and submits one idempotent result after normal completion or a disconnect forfeit. Clients no longer calculate their own result; they only cache progress and rating snapshots returned by the authority.

`Config/DefaultGame.ini` uses `Mode=LocalDevelopment` by default. In that mode ratings are still config-backed, but they are owned and calculated by the game server rather than by each client. This is intentionally an offline development provider, not a secure public service. It validates complete unique rosters, team slots, a configurable maximum party MMR spread, match lifecycle, and duplicate settlement. A disconnected player receives the persisted authoritative profile the next time that account authenticates.

Shipping builds force `RemoteHttp`, HTTPS, Steam ticket authentication, and a dedicated server. The coordinator injects a unique workload credential and stable server ID into every allocated process. Never place a server credential in an `.ini` file or a client package. The remote service contract is:

- `POST /v1/auth/steam`: validate `steam_ticket` for `steam_ticket_audience`, require it to match `claimed_account_id`, and return `authenticated`, `account_id`, plus a `progress` object.
- `POST /v1/ranked/matches`: accept the server-authenticated season, playlist, immutable participant roster, team slots, and authoritative ratings. Return `accepted` and the same `match_id`.
- `POST /v1/ranked/matches/{match_id}/result`: atomically settle the registered roster. Return `accepted`, `duplicate`, `match_id`, and one rating update per participant. Treat `X-Idempotency-Key` as a durable uniqueness key.

The service must validate Steam tickets with Steamworks publisher credentials, derive ratings from its own database rather than trusting request ratings, enforce season and party policy, and make match settlement transactional. The Unreal client contains the integration contract but deliberately does not contain publisher secrets or a pretend production database.

## Dedicated Server Development

Run `build-flick-dedicated-server.cmd` to compile `FLICKServer Win64 Development`. Then run `play-flick-ranked-dedicated-local.cmd 1`, `2`, or `3` to launch a headless authority and the matching number of local clients. `run-flick-dedicated-server-local.cmd [team-size]` starts only the server, and `package-flick-dedicated-server.cmd` creates a deployable server archive. Set `FLICK_UNREAL_ENGINE_ROOT` or pass the source-engine root as the first packaging-command argument.

Epic Launcher engine installations commonly omit the libraries needed to compile/package `TargetType.Server`. If the dedicated target reports that server targets are unsupported, use a source-built Unreal Engine 5.6 installation. Listen-server development remains available, but shipping ranked mode intentionally refuses it.
