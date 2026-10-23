# Profile cutout portraits

54 transparent 1024 x 768 portraits of the actual optimized game FBXs and palettes. Used only by the expanded profile identity card over its illustrated arena stage; other menus keep their existing portrait sets.

Regenerate with Blender: `blender --background --python Tools/Render-FlickPuckThumbnails.py -- --profile-cutout`.

Import via `Tools/Import-FlickPuckThumbnails.py` with `-FlickProfileCutoutThumbnails` in Unreal. BC7/sRGB, max size 1024, alpha retained, streamable UI mipmaps.

Lower product camera and subtle offline microfinish, transparent film, no game mesh exports or runtime material changes. Source FBX SHA-256 hashes for every rendered type/finish are in `renders.json` and checked after rendering. The separate UI stage and soft contact shadow are presentation only.
