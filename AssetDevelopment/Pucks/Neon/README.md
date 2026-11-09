Historical standalone source: current production is Retro Arcade, covering all nine types. Use ../RetroArcade/ and Tools/Import-FlickRetroArcade.py. Saved appearance index remains 6.

# Neon / Standard

Unique Standard appearance, appended catalog index 6. Only cosmetic geometry/materials are replaced. The existing root cylinder remains authoritative for collision, mass, friction, restitution and motion.

`scripts/build_neon.py` creates the Blender source scene, combined FBX, ten-role material manifest, studio render and five UI portraits. It uses the supplied script as a design starting point and the original reference at `Deliverables/Neon Standard Puck.png`. The supplied script is preserved unchanged under `source/` and is never executed directly.

Run from the project root:

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python AssetDevelopment/Pucks/Neon/scripts/build_neon.py -- --render --portraits
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' FLICK.uproject -run=pythonscript '-script=Tools/Import-FlickNeonPuck.py' -unattended -nop4 -nosplash -nosound -nullrhi
python Tools/Compare-FlickNeon.py
```

Use full project/script paths if running Unreal outside the project directory. Close running editor/game instances before importing assets.

Production mesh: `/Game/Cosmetics/Pucks/Neon/SM_Puck_Standard_Neon`. The existing packaging settings already cook this directory. The importer calibrates the high-radiance authoring materials to the game's approved exposure and shared puck lighting. Vertex LED brightness is retained from the FBX. No additional per-puck lights, texture masks or Blueprint gameplay logic are introduced.

The studio floor and light-linking setup are not exported into the game. The Blender scene is a source presentation, not a replacement for runtime verification. Runtime screenshots, comparisons and verification notes are in `Deliverables/Neon-Standard-2026-11-08/`.

The sculpted crown uses revolved radial sections, a recessed central disc and convex glass lenses. Its approved design is option C from `Deliverables/Neon-Crown-Concepts-2026-11-08/`: twelve broad solid crown panels (eight cyan, four magenta), with the existing yellow accents raised above them. These replace the tiny top LED matrix for readability at gameplay distance. Side windows retain their cyan micro-grid clusters and large magenta checker squares. `Tools/Import-FlickNeonPuck.py` supplies local color bounce and a small analytic reflection environment for the metal, leaving the approved shared puck lighting unchanged. This keeps the machining legible under the game's intentionally subdued specular key. Built-in tangent generation avoids degenerate MikkTSpace bases on the tiny details while retaining the authored normals.

For the actual in-game comparison, capture with `Tools/Capture-FlickUI.ps1 -Screen PuckDetail -MatchPuckSkin 6 -CameraOrbit 180 -CameraElevation 32 -CameraDistance 1.15 -Width 2560 -Height 1440 -CaptureDelaySeconds 4 -HideControlOverview`, then pass that screenshot to `python Tools/Compare-FlickNeon.py --game <screenshot>`. The comparison only crops and uniformly scales the original pixels.
