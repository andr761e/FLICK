# Blue Standard prototype

This folder preserves the supplied high-detail blue Standard puck source and an
isolated Unreal export. It is intentionally used only for Player 1 Standard
pucks in the Switchyard test arena while the art direction is evaluated.

The visual mesh is 90 x 90 x 20 cm and center-originated during export. FLICK's
existing primitive collision, mass, dimensions and Standard puck stats remain
authoritative.

Rebuild the FBX with:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/PrototypeStandard/export_unreal.py
```

Then import it with `Tools/Import-FlickBlueStandardPrototype.py` through the
Unreal Python commandlet.
