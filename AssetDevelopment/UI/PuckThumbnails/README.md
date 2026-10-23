# Puck thumbnails // real model renders

54 transparent UI-only PNGs: nine archetypes, six existing finishes each. These are offline
Blender studio renders of the optimized `Pucks/ClassicBlue/exports/*.fbx` geometry. No FBX,
gameplay material, collision, or skin preference is written by the renderer.

Finish indices match the game: 0 Classic Blue, 1 Classic Orange, 2 Emerald, 3 Amethyst,
4 Crimson, 5 Amber. Variant palettes are read from the existing puck set JSON files.

Rebuild from the project root (Blender 5.2 tested):

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python Tools/Render-FlickPuckThumbnails.py
```

Optional `-- --only Heavy --skin 3` renders a single thumbnail. `renders.json` records the
source FBX SHA256 for each render; the renderer checks that each source hash is unchanged.
The manifest describes the most recent invocation, so use the full command for a full manifest.

Import after rendering with Unreal Editor Python:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' FLICK.uproject -run=pythonscript -script=Tools/Import-FlickPuckThumbnails.py -unattended -nop4 -nullrhi -nosound
```

Output: `/Game/UI/PuckThumbnails/T_Puck_{Archetype}_{Finish}`. BC7, sRGB, mips, streamable,
512 maximum platform size. The existing `/Game/UI` cook directory includes these assets.
`FLICK.UI.PuckThumbnails` verifies all 54 imports and their size/compression settings.

These small images are not used to replace the actual arena/locker scene. Relative archetype
dimensions and their native emblem geometry are preserved; the shared widget letterboxes the
4:3 image. Total source PNG size is approximately 8.1 MiB, not a per-frame rendering cost.
