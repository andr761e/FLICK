# Physics diagnostics

Available in Editor and Development builds only. Open the Unreal console (`~`)
and enter `flick.Diagnostics 1`. Use `flick.Diagnostics 0` to turn it off.
It is a local, read-only view; it does not alter collision, physics or scoring.

- Cyan: active box/convex collision meshes and procedural floor triangles.
- Yellow: puck velocity arrows, representing 0.15 seconds of travel.
- Green: actual hit normals, retained for two seconds (at most 32 contacts).
- Magenta: BOB pocket openings and the depth at which pocket capture is allowed.

Hover or select a puck to see its ID, position, speed, angular speed, supporting
collider (with actual hit height and normal), and BOB
pocket/capture state in the overlay. The last hit identifies both colliders.
Pocket diagnostics use the same functions as gameplay, not a separate estimate.

Individual layers can be hidden with `flick.Diagnostics.Collision 0`,
`flick.Diagnostics.Velocity 0`, `flick.Diagnostics.Contacts 0`, or
`flick.Diagnostics.Pockets 0`. Use `1` to restore a layer. These debug options
are deliberately not saved as player preferences and are disabled in Shipping.

Collision wireframes cover the primitive types used by FLICK's arenas and
pucks (boxes, convex hulls and procedural floors), not arbitrary engine shapes.
