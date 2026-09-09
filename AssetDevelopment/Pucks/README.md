# Puck asset development

The active puck-art pipeline lives in [`HighDetail`](HighDetail/README.md).
Those are the meshes currently used by the Switchyard test arena.

Multiplayer P1/P2/P3 variants live in `PlayerIdentity/`. Each player folder
contains Blue and Orange Blender review packages. Unreal imports one shared
geometry set per player and applies team lighting at runtime, so identity
colours remain constant across teams without duplicating runtime meshes.

The retired procedural workshop, its low-quality Blender models, exports and
reference renders have been removed. Do not reintroduce them as import sources;
all puck-art work should begin from `HighDetail/`.

Runtime physics, collision, dimensions, mass and archetype behavior remain in
C++. The files here provide presentation meshes only.
