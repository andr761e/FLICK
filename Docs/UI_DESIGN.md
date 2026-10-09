# FLICK interface and Switchyard

The interface uses graphite surfaces, warm white text and lime actions. Cyan and
orange continue to identify the two teams. Colors and the 1600 x 900 reference
canvas are in `Source/FLICK/UI/FlickUITheme.h`. Slate scales the reference canvas
to fit the viewport, keeping navigation and match controls within its bounds.
Profile uses a standalone page with the shared palette and instant screen changes.
Match cards retain their contents in an 84-pixel-high reference layout (previously
128 pixels). Settings hides the match HUD and omits the current-ruleset card.

Home uses a code-native wordmark, a prominent Play action and short descriptions.
The home-specific layout is defined in `UI/FlickMainMenuStyle.h`: a steep,
anti-aliased graphite-glass diagonal, outward-stepping chamfered cards, directional
surface highlights, and restrained lime/cyan edge glow. Slate draws the glow
after the scene, so it does not depend on arena bloom or change world lighting.
PLAY alone has a full luminous surround; secondary actions retain softer right
edge accents and their existing mouse/controller focus transitions. Party tray
placement follows the same diagonal edge, and the display-puck selector remains
centered between QUIT and the bottom-anchored profile. Cosmetic art keeps its
cut-corner crop and all profile/challenge values keep their existing bindings.
The opened Social drawer uses an opaque graphite surface, lime selected tabs and
presence indicators, and shared neutral row styling. Its main-menu launcher is
unchanged. Long party names truncate inside their row.
Playlist cards expose keyboard focus; format cards explicitly show selection.
Settings groups game feel, sound and display in a scrollable workspace with a
persistent Back/Apply footer. Match presentation prioritizes the active player,
remaining pucks, turn state and shot power. Existing menu callbacks and game rules
remain in their original owners.

All pages inherit the main-menu palette from `FlickUITheme.h`: graphite glass,
cool white titles, readable muted subtitles, lime actions, cyan highlights and
orange team/warning cues. Shared panels use eight chamfered corners, a subtle
directional sheen and opposing corner rails. Playlist/header/footer adapters
reuse those panels rather than maintain separate drawing implementations.
Settings uses the quieter treatment, with themed dropdown pop-ups, checkboxes,
sliders, key-binding selectors and focusable tabs. Dense HUD panels retain
their current placement and team colours; no world lighting or gameplay rules
are changed. Full premium framing is reserved for featured main-menu panels.
Cosmetic colours and artwork cropping are preserved. Shared action labels scale
down only when necessary to fit their existing button bounds.
`FLICK.UI.SharedPresentationTheme` checks common palette identity, small/large
panel geometry, readable text contrast, transparency and coloured-state safety.

## Switchyard: test arena only

Open **Play > Test > Switchyard**, then confirm your class. Switchyard uses the
existing eight switches, twenty possible sockets and bot match rules.

- Match a switch's color to the same divider.
- Hit the small filled activation dot; the outer ring is a visual locator.
- Colored caps distinguish raised walls from flat socket markings.
- The arena has no divider labels, state summary or divider event notifications.

The matte instrument face, cap strips and switch markings have no collision.
Physics still uses the inherited arena collider and the existing divider bodies.
Divider dimensions, friction and restitution are unchanged. Switch activation
keeps the existing cue delay, followed by a tuneable physical lift (see
`DividerLift.md`). Occupied dividers now lift pucks instead of cancelling deployment
at settlement. The activation dot radius is reduced from 13 to 8 world units,
with its visible size and activation area kept in agreement.

Arena-specific presentation changes are confined to `AFlickTestArena`. Other arena visuals are unchanged.

The imported 1v1, 2v2 and 3v3 presentations share a satin-silver deck with stronger metallic
response and restrained emissive fill, so the existing softboxes/environment
produce soft reflections rather than a uniformly lit surface. Its switch and
closed-divider metal surrounds use darker gunmetal. These are local material
overrides; coloured state-owned accents, authored depth offsets, geometry and
physics are unchanged. All formats also share the darker graphite stadium,
balanced warm/cyan stadium emitters and flush cyan rim inserts. Existing
layouts, radii, divider counts and component transforms remain format-specific.
`OneVsOneDeckMetallic`, `OneVsOneDeckRoughness` and
`OneVsOneMenuDeckRoughness` tune the finish. The focused
`FLICK.Visuals.KnockoutArenaTheme` test covers format transitions, menu
inheritance, bezel depth offsets and preservation of switch-state colours.

Knockout lighting reuses the existing fixtures: stronger neutral environment
light and a larger, higher diffuse-only central fill keep the metallic floor readable across
the board. Gameplay softboxes sit higher with broader sources and longer
falloff, avoiding a single bright edge against a dark centre. The cyan rim and
warm highlights remain. `OneVsOneSkyLightMultiplier` and
`OneVsOneFillLightIntensity` tune the base coverage;
`FLICK.Visuals.KnockoutLightingTheme` verifies menu/gameplay coverage,
larger-format scaling, BOB preference application and reuse of the same light actors.
Fixture source sizes, falloff and height above the deck scale with arena radius;
local light power scales with radius squared to retain comparable illumination.
No additional shadow-casting lights or imported asset copies are needed. Historical
`OneVsOne` tuning property/subobject names remain for serialized compatibility.

Settings > Lighting exposes separate Main Menu and Gameplay presets for
ambient light, broad diffuse fill, cool/warm softboxes, team accents, overhead
light and direct-light reflections. Values are relative to the existing C++
fixture intensities (0–300%, reflections 0–100%) and persist in the local
GameUserSettings.ini under `FLICK.LightingSettings`. Defaults retain menu fill,
increase gameplay fill to 120%, and reduce direct-light reflections to 35% in
the menu and 75% in play. Materials, emissive strips and physics are unchanged.
The two menu softboxes track the camera at a fixed off-axis offset, avoiding
the old startup mirror alignment without changing the floor's roughness.
Reset Lighting restores both presets without touching other settings.

Preview Arena replaces the full settings overlay with a compact right-hand
panel. Open settings from the main menu to preview menu lighting, or pause a
Knockout or BOB match to preview gameplay lighting. Editing the other preset is allowed,
but its values only affect that scene. Menu camera, materials and lighting
remain in menu presentation while settings are open. The shared
`FlickArenaLighting` helper reuses tagged, non-replicated fixtures on both host
and client; preferences never send an RPC. The same saved Gameplay preset serves
all three Knockout formats and BOB; existing player preferences remain valid. BOB
keeps its own warmer baseline, with broad adjustable softboxes and diffuse-only
fill. Its visual-only optimized mesh replaces the old asset in place without
changing the board dimensions. Its hidden compound floor has physical round
pocket openings; scoring waits until pucks fall below the tabletop. The supplied
centre ring's downward face winding is repaired during export. `FLICK.Settings.Lighting` covers bounded values, independent
presets, persistence and reset; the isolation test also checks live updates.

## Review captures

Build `FLICKEditor`, then run:

```powershell
.\Tools\Capture-FlickUI.ps1 -Screen Home,Profile,Settings,TestArena,TestArenaSettings -Width 1600 -Height 900
.\Tools\Capture-FlickUI.ps1 -Screen TestArenaStates,Settings -Width 1280 -Height 720 -CameraView 2
.\Tools\Capture-FlickUI.ps1 -Screen Home,Party,Invite -Width 1920 -Height 1080 -CaptureDelaySeconds 8
.\Tools\Capture-FlickUI.ps1 -Screen Home -Width 3440 -Height 1440 -CaptureDelaySeconds 8
.\Tools\Capture-FlickUI.ps1 -Screen Lighting,LightingPreview -Width 1280 -Height 720
.\Tools\Capture-FlickUI.ps1 -Screen TestArenaStates,TestArenaSettings -SettingsTab 7 -PlayersPerTeam 2 -Width 1920 -Height 1080 -Map /Engine/Maps/Templates/OpenWorld
.\Tools\Capture-FlickUI.ps1 -Screen TestArenaStates,TestArenaSettings -SettingsTab 7 -PlayersPerTeam 3 -Width 1920 -Height 1080 -Map /Engine/Maps/Templates/OpenWorld
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

`FLICK.UI.MainMenuPresentationLayout` checks card convexity, row spacing, room
for the selector/profile, and containment inside the diagonal at 720p, 1080p,
ultrawide, 4:3, portrait and extreme-wide ratios. Inspect captures as well: these
geometry assertions do not substitute for judging glow, typography, or input.

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

