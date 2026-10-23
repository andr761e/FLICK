# Locker finish cards // near-top-down actual-model renders

54 square 768x768 Blender Cycles product renders, one for each of the nine current puck
FBXs and six existing finishes. This view makes finish colours and the actual face emblem
easy to compare in the locker. The new view does not replace Lineups' angled presentation
renders, other menus' transparent thumbnails, or the live rotatable locker puck.

Rebuild from the project root:

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python Tools/Render-FlickPuckThumbnails.py -- --locker
```

Use `--only Standard --skin 3` to render one combination. A full run records all 54 source
SHA256 entries in `renders.json`; every source mesh is checked before/after rendering.
Nothing exports FBX files or writes gameplay meshes/materials. All source hashes were
verified unchanged after the full locker render. Finish indices remain Blue, Orange,
Emerald, Amethyst, Crimson, Amber (0 through 5).

Import:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' FLICK.uproject -run=pythonscript -script=Tools/Import-FlickPuckThumbnails.py -FlickLockerThumbnails -unattended -nop4 -nullrhi -nosound
```

Imported paths: `/Game/UI/PuckThumbnails/Locker/T_Puck_{Archetype}_{Finish}`. BC7/sRGB,
mips, streamable, maximum 1024 (source 768 square). Existing `/Game/UI` cook coverage
includes these assets. Source PNGs total approximately 34.8 MiB. No live scene capture
is added for the grid. `FLICK.UI.LockerArtwork` checks all 54 imported textures.
