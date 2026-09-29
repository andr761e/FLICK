# Puck cosmetic sets

`AssetDevelopment/Pucks/` contains only cosmetic-set folders:

- `ClassicBlue/`: editable art, previews, manifests and nine FBX exports.
- `ClassicOrange/`: independent material configuration in `set.json`, sharing
  Blue geometry and neutral materials instead of duplicating meshes.

Replace supplied FBXs in `AssetDevelopment/Pucks/ClassicBlue/exports/`, close
Unreal Editor and run `.\update-flick-pucks.cmd`. This imports nine meshes and
refreshes Orange materials from its configuration. It does not regenerate
Orange FBXs, player-number variants, arenas or stadiums.

See [the Blue pipeline](../AssetDevelopment/Pucks/ClassicBlue/README.md) and
[the Orange set](../AssetDevelopment/Pucks/ClassicOrange/README.md).
Imported Unreal asset paths remain unchanged so existing references still work.

Cosmetics are independent of teams. The visible blue/orange ownership ring and
immediate team-coloured `Name - Puck type` hover label identify the owner.
Physics, collision, dimensions, mass and archetype behaviour remain in C++.
