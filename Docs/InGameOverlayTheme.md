# In-game overlay theme

The pause menu is the visual reference for the session UI: near-black refined
surfaces, restrained cyan angular framing, paper text, and lime primary actions.
Team colours, warning colours and result colours retain their existing meaning.

Applied to round results, post-match controls, scoreboards, match HUD panels,
replay controls, training/tutorial/practice panels, control hints, private role
selection, spectator controls, event notifications and team quick-chat panels.
Compact gameplay panels deliberately omit large branding to preserve visibility.

The private role picker also uses the session brand header, full cyan frame and
native arrow actions. Join Blue and Join Orange retain explicit team accents;
Resume is lime. Camera navigation is a quieter PREV / target / NEXT row separated
from role choices by a hairline. The centred panel scales down to fit the viewport.
`-FlickPrivateRolePreview` is an opt-in visibility override for isolated UI captures.
See `private-role-1280x720.png` in the capture directory below.

Round results remain centred and now have an in-session brand header, a cyan
outer frame, a darker score inset, and a lime action with native play/arrow glyphs.
The layout scales down on smaller viewports. The pause menu remains centred.

This is Slate-only presentation work: no physics, match rules, replication,
timers, camera changes, audio changes or new binary assets are required.
Existing action callbacks, visibility and enabled-state rules are retained.

Visual QA captures: `Deliverables/InGame-Overlays-2026-10-23`.
The capture tool's Result preset now uses a live preview board plus the round
overlay flag; completing an entire series instead opens the winner cinematic.

## Verification (2026-10-23)

- FLICKEditor Win64 Development: compiled successfully.
- FLICK Win64 Development: compiled successfully (not a fresh packaged archive).
- Actual UI startup/input smoke and Quit-button checks: PASS.
- Full automation suite: 103 successes, 2 successes with warnings, 7 failures.
  The failure set matches the prior baseline: Cosmetics.StarterSkins and
  Visuals.KnockoutArenaTheme, MenuArenaIsolation, MenuPresentationIsolation,
  OneVsOneCameraIsolation, TestArenaExposure and WorkshopArena.
- Captured and inspected round results, pause, scoreboard, training, practice,
  spectator controls and post-match summaries at 1280x720.
