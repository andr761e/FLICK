# Lineup product stages // actual puck geometry

54 offline Blender Cycles renders: the nine current optimized puck FBXs in all six existing
finishes. The native emblems, relative dimensions and silhouettes are retained. These are
not AI-redesigned models or live scene captures. The stage is an offline-only graphite plane,
three softboxes and subtle reflected emission; it is not imported as gameplay geometry.

Rebuild from the project root:

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python Tools/Render-FlickPuckThumbnails.py -- --presentation
```

Optional `--only Heavy --skin 3` renders one combination. `renders.json` records the most
recent invocation's source FBX hashes; a full run records all 54. The renderer checks the
source hash before and after each archetype and never exports a mesh or writes game materials.

Import:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' FLICK.uproject -run=pythonscript -script=Tools/Import-FlickPuckThumbnails.py -FlickPresentationThumbnails -unattended -nop4 -nullrhi -nosound
```

Output is `/Game/UI/PuckThumbnails/Presentation/T_Puck_{Archetype}_{Finish}`: 1024x768,
BC7/sRGB, mips, streamable, maximum 1024. Existing `/Game/UI` cook coverage includes them.
The full source PNG set is about 45.9 MiB; only the displayed finishes are requested at runtime.
The Lineup hero and catalogue opt in to these stages. The old 512x384 transparent images
and other menus' thumbnail behavior are unchanged. `FLICK.UI.LineupPresentation` validates
all 54 imported assets; `FLICK.UI.PuckThumbnails` continues to validate the old set separately.
