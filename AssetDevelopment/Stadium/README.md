# Test stadium asset pipeline

This folder contains the visual-only stadium surrounding the Switchyard Test
Arena. It is derived from `arena_surroundings_blender_v4.py`, with the layout
rebuilt around FLICK's authoritative 650 cm arena radius.

The stadium origin is the arena centre at play-surface height. The surrounding
floor is authored 50 cm below that origin, matching the existing arena deck.
The centre opening is controlled by `ARENA_RADIUS_M` in
`scripts/generate_stadium.py` and must remain at least 6.50 m.

Generated files:

- `TestStadium.blend` - editable Blender workshop.
- `exports/SM_TestStadium_Structure.fbx` - floor, seating, walls and rails.
- `exports/SM_TestStadium_Lights.fbx` - warm and team-colour light fixtures.
- `renders/test_stadium_preview.png` - reference render with the current arena.
- `manifest.json` - dimensions and integration contract.

Regenerate with Blender 5.2, then run `Tools/Import-FlickStadium.py` through the
Unreal Python commandlet. Stadium meshes are presentation-only and must never
participate in puck collision.
