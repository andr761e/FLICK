# Development packaging and startup checks

Run `./package-flick-development.cmd` from the project checkout. The output to
share is the **entire** `Builds/Development/Windows` folder, not just FLICK.exe.

The command resolves the project relative to itself. It uses Unreal 5.8 from
`C:\Program Files\Epic Games\UE_5.8` unless `FLICK_UNREAL_ENGINE_ROOT` names a
different installation. Unreal must have a compatible C++ toolchain and Windows
SDK available. Packaging builds both the game and its editor module; cooking
must not silently use a stale editor DLL or require another developer's Binaries.
Automation Tool logs are in `Saved/AutomationToolLogs`.

Before reporting the package ready, the command automatically runs the isolated,
offscreen startup checks below. A failed check returns a nonzero exit code and
warns not to distribute the output. Build-only/headless environments can explicitly
set `FLICK_SKIP_PACKAGE_STARTUP_TEST=1`; doing so prints a warning and does not
claim runtime validation.

## Empty bootstrap map

The default client/editor/server map and the cook map are `/Engine/Maps/Entry`.
FLICK spawns its own arena, stadium, lighting and camera in C++; no landscape or
Level Blueprint is required. Network test launchers use this same map. Client
fallback travel reads the configured game default instead of hardcoding a map.

Previously the package loaded `/Engine/Maps/Templates/OpenWorld`. The failing
package's log stopped on the first frame with a handled ensure in
`ULandscapeComponent::CreateSceneProxy` / PSO precaching while World Partition
registered template landscape components. An isolated offscreen run reproduced
the same ensure, although unattended error handling allowed that run to continue.
The template terrain is unnecessary; removing it from startup addresses the
failure without globally disabling PSO precaching or weakening rendering checks.

Main-menu initialization now builds the equipped-puck showcase before the first
rendered frame, rather than leaving the temporary match roster until the first
world tick. The normal intro fade remains enabled.

## Test the package, not just UnrealEditor

After packaging, run:

```powershell
.\Tools\Test-FlickPackagedStartup.ps1
# Also exercise normal Steam initialization on a PC with Steam running:
.\Tools\Test-FlickPackagedStartup.ps1 -WithSteam
```

This runs the actual packaged game offscreen with isolated fresh/saved profiles.
It does **not** pass FlickSkipIntro or a menu/arena preview flag. The C++ client
automation test checks the normal intro completes, the input-blocking overlay
clears, the world advances, and the single high-detail puck matches equipped
type/skin. It exercises the Settings and Play Slate button callbacks and then
exits through the real Quit button callback. It does not simulate an OS mouse
click or replace a live two-PC multiplayer test.

The saved-profile case equips an Amethyst Bouncer with BOB as the last selected
playlist. Test logs, profile settings, JSON pass summaries and a main-menu PNG go
under `Saved/PackageStartupQA/<run>/`. A hang times out and terminates only the
test's own process. Any handled ensure, failed check or unsuccessful exit fails
the script.

The matching editor regression is `FLICK.Frontend.PackagingConfiguration`;
`FLICK.Frontend.PackagedStartup` is a client-context test and needs a running game
viewport (packaged executable or UnrealEditor -game), not a NullRHI commandlet.

## Verification — 16 October 2026

- FLICKEditor and Win64 Development game compiled successfully. The final
  `package-flick-development.cmd` completed with exit code 0, including its two
  automatic startup checks. Unreal's staging step restarted its local Zen store
  after transient connection failures; it recovered and completed successfully.
- Both editor regressions passed (2/2):
  `Saved/Automation/PackagedStartup-Editor/index.json`.
- Final automatic package checks: fresh profile and saved Amethyst Bouncer with
  BOB selected both passed, with no handled ensures or runtime errors:
  `Saved/PackageStartupQA/20261016-220351-795787/`.
- The same two packaged startup cases also passed with Steam connected:
  `Saved/PackageStartupQA/20261016-220026/`. An earlier test-harness request for
  Unreal's absent packaged HTML report template emitted an export-only error;
  the final harness writes its own JSON summaries and no longer requests it.
- An initial saved-profile fixture was reset by Unreal because it omitted the
  engine's settings version. The final fixture inherits a genuine versioned
  settings file; the expected saved puck is explicitly asserted, not assumed.
- These are local automated checks, including actual Slate button callbacks.
  They do not establish behavior on another PC or validate a live online match.
