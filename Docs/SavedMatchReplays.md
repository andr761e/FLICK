# Saved match replays

After a completed Casual, Competitive or Private match, enter a title in **REPLAY NAME**, then choose **SAVE REPLAY** on the final scoreboard. Titles are limited to 100 characters; you can rename them later in the library. Each participant saves their own local recording. Practice, tutorial and free-play sessions are excluded. Starting another match discards the previous unsaved recording.

Open **Profile > Saved Replays** to watch, rename, favourite, search, sort or delete matches. The library shows mode, teams, date, result, participants, duration and local storage use. Watching from an online room requires explicitly leaving that room and party; hosts close their room.

## Opening a replay

A loading overlay remains visible while scene resources load asynchronously and the replay arena warms up. Playback opens paused so you can choose a camera before starting. The loading screen has a Cancel button. New recordings retain a small resource manifest; older recordings load the actual replay scene under the overlay, avoiding a preload of the entire cosmetic catalogue. The resources remain resident through seeking and are released when exiting.

Recorded cameras that are not being viewed no longer tick or compete with the theatre camera's lighting. Playing camera transforms interpolate between recorded updates; paused views snap to the requested recorded pose. Light-rig discovery runs periodically and after seeks instead of searching every frame. Replay playback also bypasses the live winner-puck presentation.

## Theatre controls

- Drag the timeline to any point. Coloured ticks mark rounds, shots and recorded events.
- Space: pause/play. Buttons select 0.25x, 0.5x, 1x or 2x.
- Left/right: seek five seconds. Comma/period: step the timeline by 1/30 second while paused.
- Home/end: start/end. Round and shot buttons jump between markers.
- C: cycle Arena, Player, Follow Puck, Free Camera and Cinematic cameras.
- **CHOOSE PLAYER** opens the named list of every player seat. Selecting a name also switches to Player camera. Brackets or target buttons cycle participants or visible pucks.
- Hold right mouse to orbit; mouse wheel zooms the framed cameras.
- In Free Camera, hold right mouse and use WASD/QE to fly; Shift boosts movement. Camera movement works while paused.
- F: hide controls. Escape: return to Saved Replays.

Player cameras use the recorded camera transforms where available. Bots and seats without a recorded view use a team-camera approximation. Replays contain world state and events, not a video recording or the original players' entire HUD.

## Storage and implementation

Replays live under the running build's `Saved/Replays` directory:

- `Matches/<GUID>.flick.replay`: Unreal's versioned native replay stream, with a FLICK filename suffix.
- `Index/<GUID>.json`: versioned library details and timeline markers.
- `Temp/`: the current unsaved recording.

Keep a match?s payload and index together when backing it up. This version does not implement replay import, Steam Cloud synchronization, video export or guarantees across incompatible future game builds. Failed playback returns to the library with an error message.

The C++ game-instance subsystem records replicated state through DemoNetDriver and the native local-file streamer, with checkpoints for seeking. A small DemoNetDriver subclass restores checkpoint actors synchronously, preventing partially populated frames when paused. Dedicated servers do not create unused local recordings. Playback uses a separate controller, disables gameplay input and recorded gameplay-controller RPCs, and bypasses match-authority logic. It does not resimulate a saved match from launch inputs. Existing per-shot cinematic presentation rewinds are excluded from the recording.

## Validation

Development helper: `-FlickReplaySmoke` with `-FlickAutoKickoff`, optionally `-FlickVariant=1` for BOB. Always use an isolated `-UserDir` so smoke-test match results cannot touch a real profile. `-FlickReplaySmokeOutput=<path>` selects logs/screenshots/result output. The helper is excluded from Shipping builds.

Automation tests: `FLICK.Replays.Library` and `FLICK.Replays.CorruptIndex`. Targeted regressions include private spectating, training shot undo and puck presentation. Runtime smoke checks record/save/reopen, actor counts, pause, forward/backward seeking, repeatable poses, paused camera movement, profile match totals and return to the library.

Verified on 2026-11-15:

- FLICKEditor Win64 Development compilation succeeded.
- Six targeted automation tests passed with zero failures.
- Rendered Knockout and BOB end-to-end replay smoke tests passed, including a real kickoff/BOB shot, repeated seeking and camera movement while paused.
- A separate local-network host and joining client each passed recording, saving, leaving the room, playback, scrubbing, profile isolation and library persistence.
- Theatre and library screenshots were visually inspected at 1920x1080.

The initial runtime failures exposed the required native `.replay` extension, synchronous map teardown during exit, and deferred checkpoint actor spawning. These were fixed and the affected flows retested. One concurrent rendered BOB test also crashed inside Unreal's renderer; sequential rendered testing subsequently completed successfully. Steam transport, long matches and cross-machine camera fidelity still require a playtest.

## Loading and usability update

The rendered smoke test now checks named-save persistence, paused startup after preloading, every player-seat camera, forward/backward seeks across a checkpoint and the end of the recording, free-camera movement while paused, and library persistence. Separate host/client tests verified both recorded human POVs against the saved camera transforms.

In short 1920x1080 tests on the development machine, loaded replay frame times were 15.10 ms mean / 16.40 ms p95 for Knockout, and 12.94 ms mean / 15.21 ms p95 for BOB. Live-play samples in those runs were 16.37 ms and 15.66 ms mean respectively. These are short smoke-test samples, not a performance guarantee for long matches or other hardware.

The final update also passed a rendered BOB legacy-metadata test with the resource manifest removed, plus rendered Knockout/BOB seeks across the five-second checkpoint to the recording end and back. The replay-name field and loading/theatre screens were visually inspected. Five targeted automated post-match/private-spectator/library tests passed with zero failures.

## Further replay refinement

- Timeline drags preview the selected time; checkpoint reconstruction happens once on release. The clock shows the preview time.
- Camera interpolation follows wall time, so slow-motion playback does not slow camera responsiveness.
- Visible -5/+5-second buttons, Reset View and Fly Camera supplement the named-player picker. Fly Camera captures the mouse without holding a button. Escape releases the mouse first; Exit Replay exits directly.
- Legacy files load their actual scene under the overlay instead of preloading the entire cosmetic catalogue and compiling unrelated resources.
- A silent hidden playback pass runs the opening motion normally, bounded to ten replay seconds or twenty wall-clock seconds. It then rewinds to the initial moment and warms all camera modes before opening paused. Fast-forwarded or paused-only warm-up did not sufficiently exercise motion-only render paths.
- Experimental global rendering-cache overrides were removed. The game retains its original graphics defaults.

The actual 27-second private recording was tested in the rebuilt Development package with normal graphics-driver cache access and an isolated profile copy. The final 20-second visible playback sample recorded 2,171 frames, 9.21 ms mean, 10.09 ms p95, and no frames above 50 ms. This offscreen 1080p sample is not a guarantee for all hardware or recordings. Startup work occurs behind the loading overlay.

The existing Development package executable and configuration were updated, and the original saved replay payload was verified unchanged by hash. Packaged regression checks exercise named saves, paused startup, checkpoint seeks, drag preview without seeking, seek on release, player POVs, paused camera movement, Escape release, profile isolation and library persistence.

Final verification: the packaged game completed the actual-private-match normal-cache test at 9.21 ms mean / 10.09 ms p95 with zero frames over 50 ms in the measured twenty-second playback section. The final packaged Knockout and BOB regression runs both exited successfully after validating seek-on-release and Escape fly-camera release. Both game and editor builds compiled successfully.

## Recorded-motion and lighting correction

Saved-match playback now treats replicated puck movement as recorded poses. It does not feed those poses back into Chaos's live physics correction loop. A short, bounded interpolation buffer presents intermediate position/quaternion samples; seeks, paused poses, teleports and checkpoint revisions snap to recorded state and clear stale samples. Live and online gameplay keep their existing physics replication. The minimum presentation delay is tuneable with `flick.Replay.PoseBufferSeconds` (default 0.05 replay seconds, clamped to 0-0.2); sparse older files require the source-rate adjustment described below.

Switchyard dividers no longer run client-side motion prediction during saved playback. Their recorded lift fractions drive the replay presentation. The theatre owns one fixed world-space puck-lighting frame; recorded camera pawns cannot move that rig, including immediately after seeking. Changing between player, puck, cinematic and free cameras preserves bloom and exposure. Private Knockout arena scaling also applies to replay lighting and framing.

Taking control of free camera preserves the current overview/player pose, even beyond the gameplay camera's normal bounds. Its replay-only floor follows the arena surface, preventing flight underneath the deck. Gameplay/free-play camera bounds are unchanged.

Validation on 2026-11-15: the updated Development package passed rendered Knockout and BOB record/save/load, repeated checkpoint seeks, player POVs, free-camera movement, no-snap takeover from player/overview, above-deck bounds, unchanged light transforms/bloom/exposure, profile isolation and return-to-library checks. The original 27-second private recording opened successfully. A 20-second motion trace contained 13,280 puck samples with zero physics-simulating samples; 859 of 1,048 moving samples supplied intermediate motion between native recording updates. 11,062 settled samples matched the latest recorded positions exactly at the trace's 0.001 cm precision.

Before/after paused player-to-free-camera image comparisons retained essentially identical scene brightness in both modes (mean absolute RGB differences below 0.31/255 in the arena crop). These image comparisons verify that specific transition, not every possible Lumen reflection at every camera angle. The capture run deliberately saved PNG screenshots, which caused five measurement hitches; its frame-time report should not be used as a clean playback benchmark. Original player replay payload SHA-256 remained unchanged.

Both game and editor Development targets compiled successfully after the recorded-motion correction. Seven targeted editor automation tests passed with zero failures: camera movement/zoom, puck presentation, private spectator view, private team camera reset, post-match rematch flow and both replay library tests.


## Capture-rate and Target transition correction

The earlier DemoNetDriver config entries were not engine properties. Unreal ignored them and recorded at its default maximum of 8 Hz. The actual 41-second private replay confirmed MinHz 0 / MaxHz 8 in its native header; moving-puck updates were approximately 125 ms apart. A 50 ms interpolation buffer ran out of future samples and repeatedly froze, even while the camera and renderer remained smooth.

Recording now uses the real `demo.RecordHz` / `demo.MinRecordHz` console variables at 60 Hz, with adaptive replay update frequency disabled. Each puck also records a replay-only source timestamp in `PreReplicationForReplay`, so playback does not substitute the receiving render frame's time. This timestamp does not travel over the live game connection. The JSON index stores the configured pose sample rate; missing values retain compatibility with existing 8 Hz recordings. Playback covers one source interval plus arrival jitter, while high-rate new files retain the short buffer. Old files can interpolate smoothly but cannot recover collisions or direction changes that were never sampled.

Target cycling in Player View now includes Free Camera. Entering or leaving a target explicitly restores the local lighting rig and camera postprocessing and marks the camera cut. The same camera actor and pose are retained for free-camera takeover.

On the same 41-second saved private match, 11 continuous movement sections previously held their displayed position on 512 of 1,082 sampled frames (47.32%). After correction, the corresponding check found zero held frames in 1,023 sampled frames. This checks actual pose continuity, not average rendering FPS. Both traces intentionally contain screenshot capture stalls and are not clean frame-time benchmarks.

The exact recorded Maurice POV to Free Camera transition was exercised using the Target button method. Camera pose, bloom (0.32), exposure bias (-0.45), postprocess blend weight and light transforms/intensities remained intact. Before/after arena-crop mean RGB difference was 0.90/255 with essentially unchanged mean brightness. Turning free camera retained the visible arena fixtures; two stationary images afterward differed by 0.54/255 on average. These checks cover the tested views and do not establish the absence of every temporal reflection artifact at every angle.

The game and editor Development builds compiled successfully, the package was restaged successfully with the corrected configuration, and all seven targeted editor regression tests passed. A fresh packaged Knockout replay's native header confirmed MinHz 60 / MaxHz 60, and its record/save/load/seek/player-POV/Target-free-camera checks passed. Original player replay payload hashes were unchanged.

The final packaged BOB regression also passed, with MinHz 60 / MaxHz 60 confirmed in its native replay header. Both modes passed Target-to-free-camera lighting continuity and repeated checkpoint seeking. A separate playback of the fresh BOB recording exercised actual moving-puck samples.


New recordings additionally retain a replay-only position at 0.01 cm precision and rotation with each source timestamp. The native live-movement codec rounds location to whole centimetres, which can visibly step during a slow roll even at a higher recording rate. A notification on the source timestamp feeds every new replay sample to interpolation, including samples where the standard movement packet's rounded position is unchanged. Live multiplayer movement encoding remains unchanged. Older native-only replay streams continue through the existing fallback.

Final precision verification: both Development targets compiled successfully, seven editor regression tests passed again, and the final packaged Knockout and BOB record/save/load/seek/camera checks passed. A fresh BOB playback decoded 39,416 sub-centimetre source samples with no physics simulation; nine continuous movement sections contained zero held frames across 416 sampled frames. The original private recording also opened in the final build and retained its lighting through the Target-to-free-camera transition. Both original replay payload hashes remained unchanged.


## View-dependent reflections and 120 Hz target

The subsequent opposite-side screenshot exposed a limitation of the earlier fixed-rig correction: lights retained their intensities but their reflection highlights remained aimed for the original viewer. The replay controller now rotates the authored deck softboxes around the board for the actual camera eye position, preserving the configured local power, colour and reflection settings. Puck studio lights use the ray from the eye to the board consistently in every camera mode; rotating free camera without moving its eye no longer swings the studio frame. The two puck studio sources remain above the deck. Recorded camera actors remain unable to overwrite the local theatre rig.

Local lighting preferences are checked periodically even while paused, and changes reconfigure the rig without changing viewpoint. The exact 24-second private replay was compared with a development-only fixed-rig reference. The corrected bot view restores the cyan/warm deck reflections visible in the original human view, as do the two side free-camera views. All seven local lighting preferences and light intensities were identical across those views. Changing and restoring the key slider in an isolated test profile propagated to the replay lights while paused; the original profile was not edited.

Recording now targets 120 Hz using `demo.RecordHz` and `demo.MinRecordHz`. This matches the fixed physics rate, but native recording samples on the game tick: actual saved frequency is bounded by the game's frame rate. The rendered BOB test on this machine measured approximately 73.7 source updates per second; its 120 Hz header is a target, not proof of 120 independent samples. Nine measured moving sections had zero held display frames in that run. Existing 8 Hz and 60 Hz recordings retain their original source data and playback compatibility.

Both Development targets compiled successfully and the package was restaged. Rendered Knockout and BOB end-to-end replay checks passed, as did seven targeted editor automation tests. Screenshot and light-state evidence is under `Saved/ReplayImplementation/LightingAngles120`; the previous fixed-rig reference is in `LightingAnglesFixedReference`.

Final 120 Hz validation: source timestamps in the high-frame-rate headless capture measured approximately 120.6 updates/second per puck (119.3-121.8 across actors over the measured four-second window). The rendered run measured 73.7, confirming the game-frame-rate limit rather than treating the replay header as the actual rate. BOB opposite-side and side free-camera views were rendered and visually inspected as well; local preference values and light intensities matched across views, and changing/restoring the local key slider restored the same intensities. Original player payload hashes remained unchanged.


## Compact controls and playback timing

The persistent replay UI now uses a roughly 65-pixel bottom dock at 1920x1080, with small title/exit chips replacing the full-width top banner. Camera/player choices, speeds and extra tools are popup menus. Play/pause, five-second skips, target changes, free-camera capture and hide/show remain directly available. Existing keyboard shortcuts are retained. Camera, speed and More menus plus fully hidden controls were rendered and visually inspected.

Playback requests a bounded 15-second streaming window ahead, refreshing it periodically and leaving native seeking in control while a seek is pending. The loading overlay also waits for at least three seconds of data (or the remaining duration) before opening. This avoids relying on the local streamer's normal multi-second chunk-read throttle, particularly at faster playback speeds.

Recorded puck poses are applied once per actor frame rather than repeatedly for each arriving packet. The replay controller updates after those poses, before Unreal builds the camera view, removing the extra frame of follow-camera delay. Camera easing uses an exponential wall-time response. Position interpolation uses recorded endpoint velocities with bounded monotone Hermite tangents, retaining linear fallback where velocities are absent. Paused and seek poses still snap to the recorded state. Live physics and recording frequency remain unchanged.

The first UI build caught a missing Slate bracket; it was corrected, and the game target subsequently compiled successfully. The interpolation regression covers constant velocity, exact endpoints, deceleration, missing velocities and collision tangents staying within recorded bounds. Existing short 1x rendered comparisons were approximately 10 ms at p95 both before and after; no renderer FPS improvement is claimed from those samples. Their five screenshot stalls are deliberate captures. Original low-rate saves remain limited by the motion information they contain.

Final compact/timing validation: game and editor Development builds succeeded after the Slate syntax correction. Eight targeted editor automation tests passed with zero failures, including the new interpolation test. Both rendered Knockout and BOB end-to-end replay regressions passed. The original 41-second private recording completed normal and double-speed playback with no stationary timeline frames in the measured playback windows; double speed advanced 40 replay seconds in 20 wall seconds. Persistent controls, all three popup menus, and hide/show were rendered at 1920x1080 and 1280x720. Original saved replay payload hashes remained unchanged. The Development executable and symbols were updated.

## Named replay clips

In the theatre, choose **More → Create clip**. The temporary panel provides independent start/end sliders, editable `m:ss.sss` timestamps (plain seconds are also accepted), **Use current** buttons, a clip name, and **Loop preview**. The selected interval is highlighted on the existing timeline. Camera/player choices remain available during preview. Dragging either boundary shows footage at that boundary, with coalesced native seeks at most every 120 ms and only one seek in flight. Release queues the final exact boundary. Timestamp edits also preview their boundary. Dragging the normal timeline stops the loop and retains its normal seek-on-release behavior. Escape closes the editor before exiting the theatre.

**Review & Save** first pauses the preview and presents the name, start/end and duration without writing a library entry. **Back to Edit** returns to trimming; **Confirm & Save** writes the clip. A persistent **Clip Saved** panel confirms success and its location, with **Done** or **Another Clip** actions. The library entry is tagged **CLIP**, showing its original match title and selected duration. It opens paused at relative `00:00`; slider seeks, keyboard skips, frame steps and shot/round navigation are bounded by the selected interval. Playback stops at the endpoint, and pressing Play there restarts the clip. Recorded player/bot perspectives, follow-puck, free camera and cinematic views remain available. A clip can itself be clipped again.

The native recording clock remains in source seconds so checkpoint loading, pose interpolation and recorded cameras keep their original timing. Only theatre UI time is rebased. Clips preload the source assets and seek to their start behind the existing loading overlay before opening.

Clips are metadata, not rendered videos or trimmed native payloads. Each index has an optional `SourceId`, `SourceName`, absolute `ClipStart` and `ClipEnd`. `Duration` and event timestamps retain the full recording's clock. Clipping a clip flattens its source reference and adds its original offset. This avoids copying native checkpoint state and preserves all perspectives. The original entry is unchanged; the underlying recording still occupies its full disk size. Library storage totals count each shared recording once.

Deleting an entry first removes its index, then deletes its native payload only if no other valid library entry references it. Deleting the original library entry therefore preserves its clips. Source IDs and finite time ranges are validated on read and write; missing clip fields retain compatibility with full-match indexes. No third-party dependencies, gameplay physics changes, or binary assets are needed.

Validation on 2026-11-16: both Windows Development game and editor targets compiled successfully. Nine targeted editor automation tests passed with zero test failures or warnings. The new clip metadata regression covers invalid ranges/names, unsafe source IDs, independent titles, nested source offsets and deletion of shared recordings only after the last library reference. Rendered clip checks at 1920x1080 and 1280x720 passed loading after original-entry deletion, relative zero, looping preview, camera/player selection, nested saving, bounded seeks, restart from end and automatic stop at 2x. Both editor-panel layouts and the tagged clip library were visually inspected. Existing rendered Knockout and BOB record/save/load/seek/camera/profile regressions passed. The Development executable and symbols were updated; original player payload SHA-256 hashes remained unchanged.

The same rendered clip workflow also passed on a fresh BOB recording. Timestamp fields retain exact endpoints when focused without changes, use integer milliseconds to avoid invalid `:60.000` displays, and prevent saving an invalid edited range.

## Clip editing refinement and visual stability

The save action now has distinct editing, review and saved states. No index is created by **Review & Save**. The success panel remains visible until **Done**, rather than silently closing as soon as the file is written. Both boundary sliders preview their actual native replay footage while dragging; requests are coalesced rather than postponed until dragging stops. The final boundary is requested on release. Normal theatre timeline scrubbing retains its existing release-to-seek behavior.

The current camera/puck motion interpolation and capture settings are unchanged. Narrow Knockout guide strips are wider and use a matte finish to reduce sub-pixel edge/specular shimmer at gameplay distance. Polished, non-emissive metal puck materials receive a small 0.18 roughness floor through the shared live/replay finish, without changing their colours or physics. These are targeted shimmer reductions, not a claim that every temporal reflection artifact is eliminated.

Validation: Windows game and editor Development builds succeeded. All nine targeted editor automation tests passed with zero test failures/warnings. Rendered clip tests passed on a copied current 120 Hz recording at 1920x1080 and a copied legacy recording at 1280x720. They verify footage updates during continuous dragging of both boundaries, settling on release, review without writing, actual Slate confirmation-button activation, exactly one new saved entry, persistent success feedback and Done, plus existing camera/loop/seek/library behavior. Review and saved layouts were rendered and visually inspected at both sizes. The first runtime attempt used an obsolete default fixture no longer present in the player's library; it was rerun with isolated copies. The Development executable and symbols were updated.

Fresh rendered Knockout and BOB record/save/load/seek, player/free camera, lighting and profile-isolation regressions also passed after the visual finish adjustment. The current player replay's index and payload hashes remained unchanged, and the staged executable matches the compiled executable.

Clip handle limits: dragging past the opposite boundary leaves the clamped time unchanged and no longer requests another preview. The preview layer also skips targets that are already queued, in flight, or displayed, including an unchanged release. Dragging back into the valid range resumes preview updates. Returning to an active/displayed target cancels an obsolete queued target from earlier in the drag.

Limit-fix validation: game and editor Development builds succeeded. The rendered clip regression passed holding each boundary against the other with repeated drag events and checking no new seek/revision occurs, unchanged release, and resumed preview when moving away, alongside the existing trim/review/save/playback workflow. The updated Development executable was staged and matched the build hash. The player recording and index remained unchanged.

## Replay lighting stability

Replay-local light actors are now marked rewindable while watching a replay. Native checkpoint seeks previously deleted these non-networked actors, recreating the rig and invalidating lighting history repeatedly. Only FLICK-tagged, non-replicated light fixtures and the local skylight receive this treatment; gameplay actors and recorded state still rewind normally. The controller restores a missing/replaced rig and applies local preference, arena/context or scale changes. Ordinary seeks with the same rig no longer reconfigure every light or declare a camera cut. Large time/view discontinuities retain history resets.

Unreal always considers replay cameras movable, even when the world is paused. A replay-only scene-view extension now allows the actual viewport to use paused temporal sampling after 32 settled frames. Camera movement, look/fly input, replay seeks/revisions, playback, lighting changes and viewport resizing allow convergence again. UI/input keep ticking, and the scene continues to render; this does not substitute a cached screenshot. Other worlds and scene-capture views are excluded.

The retained changes preserve light intensities, colours, exposure, render scale, hardware/software Lumen preference and the existing quality presets. Higher reflection reconstruction, full-resolution reflection rays, higher final-gather sampling, higher TSR history resolution and harder shadows were compared in isolated processes. Their additional cost or visual change did not justify keeping them; default reflection/final-gather quality remains 1.

The controlled 1920x1080 overview comparison used the same recording at source time 24 seconds, twelve paused captures and ten repeated same-time seek captures. Deck luminance temporal standard deviation fell from 0.4313 to approximately 0.0001 after settling. Immediate post-seek variation fell from 1.5769 to 0.6204 (about 61% lower). Paused mean luminance was 118.58 before and 118.52 after on a 0-255 scale. Six-second screenshot-free playback windows measured 8.81 ms versus 9.26 ms mean frame time in the final comparison; these are short scene checks, not a general performance benchmark. Local light identities, transforms and powers remained stable across seeks. Free-camera input wakes temporal rendering; released free-camera and opposite-player views settle again.

One initial rig-refresh trial failed the visual comparison because deleted lights were not restored. It was corrected by explicitly restoring missing rigs and preserving the local fixtures across rewinds. An ordinary paused-world flag trial also had no effect because Unreal overrides that flag for replay worlds; the retained view extension addresses that actual renderer path.

Rollback: `flick.Visual.StableArenaLighting 0` disables both retained changes (reopen the replay to reinitialize the rig). `flick.Visual.PauseStableSampling 0` disables only settled sampling. Exact pre-change source/configuration and the packaged executable are backed up under `Saved/ReplayImplementation/LightingStabilityRollback`; its `Restore.ps1` restores those files without reverting the earlier clip features. Rebuild targets after a source rollback; the old executable is restored immediately. Comparison artifacts are in `Saved/ReplayImplementation/LightingStability`.

Final validation: game and editor Development targets compiled successfully. Nine targeted editor automation tests passed with zero test failures/warnings. The rendered clip workflow passed live dragging, both handle limits, no repeated seeks at limits, review/confirmation, looping, cameras, bounded playback and persistence. A fresh BOB recording passed save/load/seek, paused/free-camera lighting, profile isolation and return to the library. The final render audit checked stable local light identities, camera wake-up and settled free/opposite-player views. A separate isolated preference check lowered the cool-softbox value from 2.997 to 2.250 while paused and restored it: scene RGB mean changed from 87.94 to 85.17 and returned to 87.87, confirming the paused view updates to preference changes. The player recording/index hashes remained unchanged, and the staged executable matches the compiled executable. An analysis-only log-reading command initially assumed UTF-8; Unreal's UTF-16 log output was then decoded correctly.


## Winner-screen transition hitch ? 2026-11-16

Match-end recording previously called the global replay-streaming Flush on the
game thread. The previous rendered BOB log records six consecutive 100 ms sleeps
on the same frame when the recording stopped, blocking the winner-camera transition
for approximately 600 ms. FinishCapture now requests the normal asynchronous stop
and retains the local streamer until its queued writes reach Idle. Save and replay
opening remain unavailable until finalization completes; recording failures do not
expose a partial file. New-match capture also waits before deleting the old temporary
file. Application shutdown still drains pending writes before temporary-file cleanup.

Both Development targets compiled successfully and the packaged executable was
updated. All 12 focused editor tests passed, including post-match arena presentation,
rematch flow and replay tests. Rendered packaged Knockout and BOB recordings passed
record/save/load, puck counts, pause/seek/free-camera and library persistence checks.
Both updated logs contain zero blocking 100 ms flush sleeps. This verifies removal
of the identified recording stall, not an exhaustive benchmark of every cosmetic
or graphics configuration. Logs are under Saved/ReplayImplementation/WinnerHitch*
and the SmokeClassic/SmokeBob folders.
