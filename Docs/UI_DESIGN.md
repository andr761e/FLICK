# FLICK interface and Switchyard

The interface uses graphite surfaces, warm white text and lime actions. Cyan and
orange continue to identify the two teams. Colors and the 1600 x 900 reference
canvas are in `Source/FLICK/UI/FlickUITheme.h`. Slate scales the reference canvas
to fit the viewport, keeping navigation and match controls within its bounds.
Profile uses a standalone page with the shared palette and instant screen changes.
Match cards retain their contents in an 84-pixel-high reference layout (previously
128 pixels). Settings hides the match HUD and omits the current-ruleset card.

Home uses a code-native wordmark, a prominent Play action and short descriptions.
The opened Social drawer uses an opaque graphite surface, lime selected tabs and
presence indicators, and shared neutral row styling. Its main-menu launcher is
unchanged. Long party names truncate inside their row.
Playlist cards expose keyboard focus; format cards explicitly show selection.
Settings groups game feel, sound and display in a scrollable workspace with a
persistent Back/Apply footer. Match presentation prioritizes the active player,
remaining pucks, turn state and shot power. Existing menu callbacks and game rules
remain in their original owners.

## Switchyard: test arena only

Open **Play > Test > Switchyard**, then confirm your class. Switchyard uses the
existing eight switches, twenty possible sockets and bot match rules.

- Match a switch's color to the same divider.
- Hit the small filled activation dot; the outer ring is a visual locator.
- Colored caps distinguish raised walls from flat socket markings.
- The arena has no divider labels, state summary or divider event notifications.

The matte instrument face, cap strips and switch markings have no collision.
Physics still uses the inherited arena collider and the existing divider bodies.
Divider dimensions, friction, restitution, overlap clearance and deployment
timing are unchanged. The activation dot radius is reduced from 13 to 8 world
units, with its visible size and activation area kept in agreement. The one state-refresh correction clears a
queued visual when an obstructed deployment is cancelled at settlement.

Arena-specific presentation changes are confined to `AFlickTestArena`. Other arena visuals are unchanged.

## Review captures

Build `FLICKEditor`, then run:

```powershell
.\Tools\Capture-FlickUI.ps1 -Screen Home,Profile,Settings,TestArena,TestArenaSettings -Width 1600 -Height 900
.\Tools\Capture-FlickUI.ps1 -Screen TestArenaStates,Settings -Width 1280 -Height 720 -CameraView 2
```

The helper launches Unreal offscreen, captures Slate via `Shot showui`, disables
Steam and isolates profile saves beneath `Saved/UICaptures`. Its default Entry
map is an empty host for the game's runtime C++ arena. Pass
`-Map /Engine/Maps/Templates/OpenWorld` to use the project's normal startup map.
The shader/cache directories stay inside the project. A cold shader cache can
require `-TimeoutSeconds 600` on the first run.

`TestArenaStates` is an explicit non-shipping visual fixture with four raised
dividers and one queued change. It is for reviewing appearance, not evidence of
physics behavior. `-FlickTestArenaSeed=1337` keeps capture layouts reproducible;
normal matches retain their randomized layouts. Camera views are indexed 0â€“3.

## Validation (2026-09-05)

- After the compact-HUD and Profile revision, rebuilt and reran all 22 tests:
  22 passed, zero failures or warnings. Inspected fresh 1280 x 720 captures of
  TestArena, Profile and TestArenaSettings in
  `Saved/UICaptures/20260905-200932-a53d52`. The match settings capture confirms
  that player cards and the ruleset box are absent.
- Unreal 5.6 `FLICKEditor Win64 Development` compilation and linking succeeded.
- All 22 `FLICK` automation tests passed, including `FLICK.Arena.ControlZones`.
  Machine-readable results are in `Saved/UIAutomation/Report/index.json`.
- The existing gamepad smoke test passed in the test arena: controller focus,
  analog drag and release produced an accepted shot. See
  `Saved/UIInputSmoke/smoke.log`.
- Actual Slate captures were inspected at 1600 x 900 and 1280 x 720, including
  the opposite camera side, raised/queued/open divider fixture, settings,
  profile, mode selection, pause and results. The final test-arena capture also
  ran on `/Engine/Maps/Templates/OpenWorld`, the normal startup map.

This verifies build, existing rules, the controller shot path and static visual
layouts. Full manual mouse navigation and extended divider-match playtesting
remain useful for judging feel.

