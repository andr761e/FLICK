# BOB Pocket Foundry stadium

This is the visual-only venue surrounding the square BOB arena. It is an
original rectangular broadcast-hall concept rather than a variation of the
Switchyard stadium: a sunken chamfered deck, four diagonal corner galleries,
pocket-ring score pylons, and suspended gantries frame the four-corner game.
Tall architecture begins outside the gameplay-camera orbit, leaving a dedicated
clear envelope for Q/E rotation and elevation changes.

The origin is the arena centre at play-surface height. The 13.08 m square BOB
arena is previewed but never included in either stadium export. Gameplay
collision remains entirely owned by `AFlickBobArena`.

Generated files:

- `BOBPocketFoundry.blend` — editable Blender source.
- `exports/SM_BobStadium_Structure.fbx` — architecture and seating.
- `exports/SM_BobStadium_Lights.fbx` — emissive fixtures and score rings.
- `renders/bob_stadium_preview.png` — authored preview.
- `manifest.json` and `validation.json` — integration contract.

Regenerate with Blender 5.2:

```powershell
blender --background --python AssetDevelopment/BOBStadium/scripts/generate_bob_stadium.py
blender --background AssetDevelopment/BOBStadium/BOBPocketFoundry.blend --python AssetDevelopment/BOBStadium/scripts/validate_bob_stadium.py
```

Close Unreal Editor and run `update-flick-assets.cmd` to reimport the venue.
