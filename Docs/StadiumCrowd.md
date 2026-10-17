# Stadium spectators

Spectators are currently disabled in Knockout (1v1, 2v2 and 3v3) and BOB.
Their component, design and seating layouts are retained for later use; both
arena callers pass `false` to `Configure`, so no crowd instances are populated
or rendered. Existing stadium exports, seats, lighting and collision are unchanged.

`UFlickStadiumCrowd` is a presentation-only scene component attached to each
arena. Its five instance batches share clothing/head colours. Rounded cylinder
heads and tiny dark eyes give the figures a readable face at close range. There
are no spectator actors, skeletons, AI, animation ticks, shadows, replicated
instances, or collision bodies. Dedicated servers do not populate the crowd.
Each person uses thirteen cubes and one shared cylinder head.

Seat coordinates follow `generate_stadium.py` and `generate_bob_stadium.py`:

- Knockout: four bench arcs, three rows, eighteen positions per row (216 seats).
- BOB: four corner galleries, four rows, seven existing seats per row (112 seats).

A fixed random seed fills approximately 82% of the seats, consistently across
clients. Density (`Occupancy`) and figure size (`SpectatorScale`) are editable
on the arena's StadiumCrowd component. Changes rebuild instances; ordinary arena
updates only adjust the shared location, scale and visibility. The crowd follows
the same stadium scale and SurfaceZ as the original architecture and hides if
the high-detail stadium assets are unavailable. This is decorative geometry,
not an interactive crowd simulation.

Verification: build FLICKEditor and run `FLICK.Arena.StadiumCrowd`. The test
checks both seating layouts, component registration, non-collision, disabled
tick, complete figures, arena clearance, scaling, visibility and zero occupancy.
