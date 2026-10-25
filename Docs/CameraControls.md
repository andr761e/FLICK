# Gameplay camera and ambient range (2026-10-25)

Default orbit-camera controls:

| Input | Action |
| --- | --- |
| A / D, held | Orbit left / right |
| W / S, held | Raise / lower the viewing angle |
| Wheel up / down | Move closer / farther from the arena |
| F | Reset orientation, elevation and zoom |
| V | Toggle top view; the wheel still zooms |
| X | Toggle the existing free camera |

Free camera retains WASD translation, mouse look and Space/Ctrl height. Its
movement keys can share bindings with the mutually exclusive orbit-camera
controls; simultaneous actions still reject conflicting keys. Private
spectator A/D seat cycling is suspended while using free camera.

In the training board editor, Q/E now cycles puck types. The wheel zooms there
as it does in normal gameplay. Existing custom bindings are retained; Reset
Controls applies these new defaults.

Orbit and elevation integrate elapsed time. Zoom uses a tunable 8% step,
bounded to 0.65–1.4 times the existing camera distance preference. It scales
camera distance about the arena surface, changes neither elevation nor field
of view, and resets with the existing round/camera reset. Frontend cameras,
replay framing and followed-player cameras keep their own presentation.

Ambient is now a trim over a soft base rather than a complete removal of the
sky/environment contribution. Stored slider values remain 0–3 with the same
save keys. Effective intensity is `1 + 2 * slider / 3`: minimum retains the
previous normal base, default is 1.667, and maximum remains exactly 3.
The ambient readout shows its effective 100–300% intensity rather than calling
the retained base light 0%.
Arena fill, softbox, accent and reflection controls are unchanged. Fixed puck studio
lighting remains independent of all these settings.

All changes are local presentation/input. Collision, mass, friction, launch
rules and physics timing are untouched.

## Verification

Both FLICKEditor and FLICK Win64 Development compiled successfully. The full
suite ran 115 tests: 106 successes, 2 successes with warnings, and the same
7 pre-existing failures (Cosmetics.StarterSkins; Visuals.KnockoutArenaTheme,
MenuArenaIsolation, MenuPresentationIsolation, OneVsOneCameraIsolation,
TestArenaExposure, WorkshopArena).

MovementAndZoom checks equal orbit/elevation changes over equal elapsed time
at 40 and 400 FPS, actual camera-distance reduction, unchanged elevation while
zooming, distance limits, reset, top-view zoom and menu isolation. Control
binding/conflict tests and lighting-response/physics tests pass.

Epic-quality minimum/default/maximum ambient captures and the updated training
camera hints were visually inspected. Evidence is retained under
`Deliverables/Camera-Ambient-2026-10-25`; automation report:
`Saved/Automation/CameraAmbient20261025`.
The final actual-viewport startup/input and Quit-button smoke checks passed.
The lighting-settings capture confirms a 100% base readout at slider minimum.
