# Puck finish and presentation (2026-10-24)

## Scope

Only puck appearance changes. Existing shapes, sizes, collision, masses,
friction/restitution, shot rules, ownership markers, emblems, cosmetic save
indices, arena materials, camera settings and global exposure are unchanged.
The BOB striker retains its distinct pearl/target face.

## Real-time finish

- The existing `M_BlueStandardSurface` parent is shared by all nine classes and
  the six existing finishes, including the historically named Standard palettes.
- Neutral metal no longer emits a constant white `SurfaceLift`. The graphite
  housing and recessed core have darker albedo; silver/edge highlights remain
  metallic with controlled roughness and restrained anisotropy.
- Fine rest-space machining varies roughness/normal response. Derivative
  filtering removes subpixel grain at gameplay distances. No texture maps,
  displacement, world-position offsets or new geometry are added.
- LED emission uses a bounded idle/selected budget plus a hue-purity response
  that reduces pastel white-out. Existing team/skin colour data is retained.
- A shared, tagged studio key/fill pair now supplies fixed neutral lighting on
  channel 1. Puck visual meshes receive that channel exclusively. The pair has
  zero indirect/volumetric contribution and no shadow passes, and cannot light
  the floor or stadium. Post-match display copies retain channels.
- Reconfiguring the rig reuses the tagged actors. Arena lighting preferences
  do not change studio power or specular response. Dedicated servers do not
  create the visual rig. These are shared lights, not lights per puck.

## Authoring

`AssetDevelopment/Pucks/finish.json` holds neutral finish-role settings.
`Source/FLICK/Core/FlickPuckFinish.h` holds the runtime LED energy budget.
`Tools/Build-FlickPuckFinish.py` updates existing material asset paths only.
`Tools/Update-FlickPucks.py` reapplies that finish after a full puck import.
No new material/texture asset paths are needed.

`Tools/Render-FlickPuckThumbnails.py` renders the original optimized FBXs with
the same finish-role settings. Existing Locker, Hero, Presentation, ProfileCutout
and original portrait assets are refreshed using `Tools/Import-FlickPuckThumbnails.py`.
Each view has 54 class/finish combinations. Renders verify the source FBX hashes
did not change and preserve texture dimensions, compression, save indices and
transparent backgrounds where required. Only the locker portrait camera gets a
slightly lower product angle to reveal the sidewall; gameplay cameras do not move.

## Verification artifacts

Before/after real game captures: `Deliverables/Puck-Finish-2026-10-24`.
Material role report: `Saved/PuckFinishReport.json`.
Per-view render logs: `Saved/Logs/PuckFinishRender-*.log`.
Per-view import logs: `Saved/Logs/PuckFinishImport-*.log`.

All five view sets were regenerated and imported: 270 existing textures, with
54 unique class/finish combinations in each set. No puck FBX/GLB or static-mesh
assets changed. The full regression suite returned 103 successes, 2 successes
with warnings and the same 7 pre-existing failures:
Cosmetics.StarterSkins; Visuals.KnockoutArenaTheme, MenuArenaIsolation,
MenuPresentationIsolation, OneVsOneCameraIsolation, TestArenaExposure,
WorkshopArena. Puck/lighting-isolation and physics tests passed.

FLICKEditor and FLICK Win64 Development builds succeeded. Actual viewport
startup/input and Quit-button smoke checks passed. These are rebuilt development
binaries/assets, not a fresh packaged archive.

After all portrait imports, the focused UI suite passed all 19 tests. Real
viewport captures were inspected for the locker (720p/1080p), class selection,
lineup, profile, live match and BOB striker. A cold-cache class capture was
retaken after its editor texture compilation completed; no blank portrait is
used in the final verification set.

## Camera-angle glare follow-up (2026-10-24)

Bright arena softboxes could reflect nearly white across the dark insert when
the camera orbited, obscuring its emblem. A satin, low-F0 face coating now
separates the insert from the polished silver hardware. The core is dielectric
with roughness 0.44 and specular 0.02. Crown/edge/housing-sidewall finish values,
light rig, exposure, LED emission budgets and gameplay physics are unchanged.
Diffuser/emblem specular is reduced to 0.04 with roughness 0.34, so reflected
white light does not overpower their existing emitted skin colour.

The supplied Standard FBX assigns its large center disc to **Housing**, not
**Core**. Changing only the Core material therefore missed that class. The
shared parent applies the same coating to the housing's upper center using
rest-space coordinates (24.5–25 cm radial transition, upper half only). That
region is inside the silver crown and excludes the sidewall/service plate.
`FinishFaceCoating` enables this only on Housing; its child skin palettes inherit
it. No source mesh, section assignment, collider or additional material asset
is required. The mask moves with the puck and does not depend on camera angle.
The BOB striker's separate pearl target material remains separate.

The thumbnail authoring script mirrors that face treatment on its temporary
in-memory mesh/materials; it never exports those assignments to the source FBX.
Existing production UI portraits were deliberately not replaced for this
real-time orbit fix. One isolated Standard/Amethyst hero render validates the
authoring path with an unchanged source hash.

`Capture-FlickUI.ps1 -CameraOrbit 15` adds a capture-only offset using the actual
gameplay orbit method, with no saved camera/profile changes. Before captures at
0°, 15°, 30° and 45° reproduce the washout; matching after captures plus 90°,
180° and 270° verify readable faces around the board. Artifacts are retained
in `Deliverables/Puck-Reflection-Fix-2026-10-24`, outside disposable Saved data.
`Saved/PuckFinishBindings.json` records all nine meshes' material bindings.
`FLICK.Visuals.PuckFaceGlare` checks all six finish families, including the
Standard housing-center exception, preserved silver and the Orange LED assets.

Follow-up verification: both FLICKEditor and FLICK Win64 Development compiled
successfully. The full suite ran 113 tests: 104 successes, 2 successes with
warnings, and the same 7 pre-existing failures listed above. PuckFaceGlare,
WorkshopPucks, PuckSkins and BOB lighting checks passed. Final 1080p match and
Amethyst locker captures, plus the BOB striker close-up, were visually inspected.
No new packaged archive was made.

## Consistent locker/gameplay lighting (2026-10-25)

The locker appearance is the reference for live pucks. Adjustable arena lights
previously affected the visual mesh directly, and the puck reflection fixture
also followed the arena Highlights preference. That coupling is removed.

The studio pair is camera-relative in azimuth, with broad sources and restrained
specular response. It scales distance, source size and power with arena size,
so larger Knockout formats do not darken the pucks. The existing hidden physics
cylinder supplies the arena contact shadow; elimination disables that shadow.
No collider dimensions, simulation, mass, friction or restitution change.

`PuckEnvironmentResponse` defaults to 0.12 in the shared material parent and
attenuates indirect environment lighting through material AO. Direct arena
lighting is isolated by channels; Lumen reflections and post-processing are
still scene effects, so this is controlled lighting rather than an unlit decal
or a promise of identical pixels under every exposure/quality setting. The
coated dark insert, silver finish and LED emission budgets remain intact.

The dedicated BOB striker face remains separate. Existing static portraits are
unchanged in this follow-up. See `PuckStudioArena.md` for captures and checks.

## Locker matching follow-up (2026-10-26)

Knockout play/replay now share the locker's exposure and bloom. The studio pair
has wider gameplay coverage and follows the view orientation, while locker
retains its original layout. Materials and static portraits are unchanged.
See `PuckLockerMatch.md` for implementation and comparison details.
