# Classic arena — premium visual deck

Blender-authored static deck for the Switchyard arena used by 1v1, 2v2, and
3v3. The graphite playing surface, satin-metal rim, warm undercut light,
and fine scoring marks are visual-only. The existing divider, socket, and
switch visuals remain modular. The original `AFlickArena::ArenaMesh` cylinder
still owns all collision and physical-material behavior at the same 650 cm
radius.

Regenerate with Blender 5.2:

```powershell
& 'C:/Program Files (x86)/Steam/steamapps/common/Blender/blender.exe' --background --python AssetDevelopment/ClassicArena/scripts/generate_classic_arena.py
```

This writes `ClassicArenaPremium.blend`, an FBX/GLB export, a review render,
and `dimensions.json`.

## Legacy source archive

This deck has been superseded by the modular arena under `AssetDevelopment/Arena`.
Its unused Unreal mesh was removed after a runtime-path
and Asset Registry reference audit. The source files and manual importer are
retained for recovery, but `update-flick-assets.cmd` no longer imports this deck.
The current gameplay arena, collision and lighting are unchanged.
