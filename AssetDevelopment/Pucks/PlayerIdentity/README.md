# Multiplayer player-identity pucks

The editable review packages are organized by player identity and team:

- `P1/Blue` and `P1/Orange`: ice-white P1 emblem
- `P2/Blue` and `P2/Orange`: magenta P2 emblem
- `P3/Blue` and `P3/Orange`: lime P3 emblem

Each team folder contains one Blender file, nine individual FBX exports, and a
dimension/material manifest. The P-number colour is identical on both teams;
only the embedded puck lights change between cyan and orange.

Unreal imports the Blue geometry once per player identity and swaps only the
team-light material at runtime. This avoids duplicate runtime meshes while the
separate Orange Blender packages remain available for art review.

Regenerate all packages from the current high-detail source pucks with:

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python AssetDevelopment/Pucks/HighDetail/scripts/generate_player_identity_variants.py
```

These are cosmetic meshes. Existing C++ physics bodies remain authoritative.
