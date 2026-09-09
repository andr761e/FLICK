# Puck asset development

The active puck-art pipeline lives in [`HighDetail`](HighDetail/README.md).
Those are the meshes currently used by the Switchyard test arena.

The retired procedural workshop, its low-quality Blender models, exports and
reference renders have been removed. Do not reintroduce them as import sources;
all puck-art work should begin from `HighDetail/`.

Runtime physics, collision, dimensions, mass and archetype behavior remain in
C++. The files here provide presentation meshes only.
