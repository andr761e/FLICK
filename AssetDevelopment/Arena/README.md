# Switchyard arena workshop

High-detail Blender source assets for FLICK's Test > Switchyard arena. The scene
uses the live C++ dimensions as its source of truth and keeps every gameplay-driven
mechanism modular.

## Assets

- `SM_TestArena_Static`: non-colliding visual deck, chassis, rim and markings.
- `SM_TestArena_Divider`: one nominal 146 x 18 x 56 cm divider, pivoted at its
  bottom center. Runtime X scaling can reproduce the existing outer/inset lengths.
- `SM_TestArena_DividerSocket`: separate dormant/active floor socket.
- `SM_TestArena_SwitchHousing`: separate 40 cm-radius switch surround.
- `SM_TestArena_SwitchDot`: separate 8 cm-radius activation dot.
- `SM_TestArena_SignalTrace`: one-meter trace segment for runtime length scaling.

The Blender preview shows all twenty authored sockets and a deterministic set of
eight active mechanisms. It is presentation-only. Divider collision, random layout,
switch detection and deployment remain owned by `AFlickTestArena`.

## Rebuild

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Arena/scripts/generate_arena.py
```

The script writes `TestArenaWorkshop.blend`, `exports/*.fbx`, `exports/*.glb`,
`renders/test_arena_preview.png`, and `dimensions.json`. Blender units are meters;
the manifest records centimeters for direct comparison with Unreal.

Validate the generated workshop and every modular export with:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background AssetDevelopment/Arena/TestArenaWorkshop.blend --python AssetDevelopment/Arena/scripts/validate_arena.py
```

This is high-detail prototype art. The static deck has no gameplay collision, and
the modular divider art must be attached to the existing authoritative collision
components rather than replacing them.

## Unreal integration

The imported assets live under `/Game/TestArena/Arena`. Reimport them after an
authoring rebuild together with the pucks and stadium by closing Unreal Editor
and running from the project root:

```powershell
.\update-flick-assets.cmd
```

`AFlickTestArena` uses the static arena and mechanism meshes only as presentation.
Its original circular floor and simple divider boxes remain the authoritative
collision, so the higher-detail bevels cannot change puck movement.
