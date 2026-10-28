# Replay cosmetics and bot loadouts

Replays present cosmetic effects without running launches, collisions, scoring or elimination logic again.

- Puck trails use recorded shot time during replay, including pucks that were eliminated in the live shot. Their gameplay elimination flag stays unchanged.
- Knockout and arrival events occurring inside the recorded shot retain their original position, direction, colour, strength and equipped style. Pre-shot arrivals are not added to the replay.
- Recorded time controls effect age, including the opening hold and slow motion. Replay actors are reused and explicitly destroyed on finish or skip. Any remaining live burst is removed when playback begins.
- The cosmetic clock is replicated for remote viewers. Normal gameplay and locker previews continue using their existing clocks.

Training opponents and unoccupied private-match seats receive a cached, coordinated skin, trail, spawn effect and knockout effect. Classic bots also receive a built-in valid four-puck lineup. Beginner bots favour Balanced and Control; other difficulties use all four presets. Human-edited lineups do not replace these bot presets.

Assignments last for the whole match, including respawns and subsequent rounds. New matches clear the cache. Human seats, ordinary free play, tutorials and prescribed practice packs retain their existing loadouts. Cosmetics do not change piece physics; bot lineup classes use the existing archetype rules.

Regression coverage: `FLICK.Replay.Cosmetics`, `FLICK.Bots.MatchLoadouts`.

Validation on 2026-10-28:

- Editor and standalone game Development builds completed successfully.
- Replay and bot regression checks passed. Trail, tutorial, practice-pack, undo and render-rate independence checks also passed in the full suite.
- A real offscreen game viewport review passed; screenshots are in `Deliverables/Replay-Cosmetics-2026-10-28`.
- The full suite still reports the seven previously recorded visual-baseline failures: StarterSkins, KnockoutArenaTheme, MenuArenaIsolation, MenuPresentationIsolation, OneVsOneCameraIsolation, TestArenaExposure and WorkshopArena. The initial new test assertion failures were corrected and both feature tests rerun successfully.
- Replication is implemented, but a live multi-machine multiplayer session has not been tested for this change.
