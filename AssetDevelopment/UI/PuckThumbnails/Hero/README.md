# Premium hero portraits

All 54 portraits are offline Cycles renders of the existing optimized FBXs, not redesigned/generated puck models.

Regenerate with Blender:

    blender --background --python Tools/Render-FlickPuckThumbnails.py -- --hero

Import in Unreal using the absolute script path and `-FlickHeroThumbnails` with `Tools/Import-FlickPuckThumbnails.py`.

- Lower camera, brushed reflective graphite receiver, soft white key light, amber and cyan strip reflections.
- 1024 x 768 masters; compressed BC7/sRGB, streamable UI mipmaps, max size 1024.
- Used by the lineup hero inspector and expanded profile showcase/avatar fallback only.
- The lineup adds a 12-degree, aspect-corrected portrait roll in Slate to match the angled concept; the profile retains the flat camera composition. Both use the same model portrait, without another texture set.
- Actual archetype and equipped finish remain dynamic. The locker preview remains a live, rotatable in-game puck.
- The render-only floor and lights are never exported to the game. No meshes, physics or runtime materials are changed.
- `renders.json` records each source FBX SHA-256; the renderer checks it again after rendering each archetype.
- Original, Presentation and Locker views remain separate and unchanged.
