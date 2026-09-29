"""Export the artist-authored BOB source without regenerating its geometry."""
import json
from pathlib import Path
import bpy
import bmesh
from math import cos, sin, pi, hypot
from mathutils import Vector

root = Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(root / "BobArena.blend"), use_scripts=False)
meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH" and not obj.hide_render]
if len(meshes) != 1 or meshes[0].name != "SM_BobArena_HighDetail":
    raise RuntimeError("Expected one presentation mesh named SM_BobArena_HighDetail")
obj = meshes[0]
evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
corners = [obj.matrix_world @ Vector(corner) for corner in evaluated.bound_box]
size = [max(v[i] for v in corners) - min(v[i] for v in corners) for i in range(3)]
if any(abs(size[i] - 13.08) > .001 for i in (0, 1)):
    raise RuntimeError("Authored board dimensions must remain 13.08 x 13.08 metres")
# The supplied centre-ring faces point downwards. Repair only their winding;
# retain the artist's geometry, dimensions and original Blender source.
obj.data = bpy.data.meshes.new_from_object(evaluated)
obj.modifiers.clear()
# Recess the artist's raised pocket bezels into the tabletop. Keep the pocket
# walls/bottoms intact; a 0.005 cm render offset prevents coplanar z-fighting
# without a perceptible lip. Collision remains exactly at 250 cm below.
flattened_pocket_vertices = 0
for vertex in obj.data.vertices:
    if vertex.co.z > 2.50005 and any(
            hypot(vertex.co.x - cx, vertex.co.y - cy) < .9
            for cx in (-4.7, 4.7) for cy in (-4.7, 4.7)):
        vertex.co.z = 2.50005
        flattened_pocket_vertices += 1
bm = bmesh.new()
bm.from_mesh(obj.data)
center_faces = [face for face in bm.faces
    if obj.data.materials[face.material_index].name == "04_BOB_Red"
    and all(hypot(v.co.x, v.co.y) < 2.5 for v in face.verts)]
if not center_faces:
    raise RuntimeError("Expected the authored centre ring for winding validation")
ring_faces = [face for face in center_faces if face.normal.z < -.9]
bmesh.ops.reverse_faces(bm, faces=ring_faces)
bm.to_mesh(obj.data)
bm.free()
obj.data.update()

# One static compound body: flat coplanar support and 32-sided round openings.
# No artwork triangles, pocket bottoms, or invisible full-board slab collide.
collision = []
def hull(points):
    points = list(dict.fromkeys((round(x, 7), round(y, 7)) for x, y in points))
    if len(points) < 3:
        return
    count = len(points)
    vertices = [(x, y, z) for z in (2.0, 2.5) for x, y in points]
    faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
    faces += [(i, (i + 1) % count, (i + 1) % count + count, i + count) for i in range(count)]
    data = bpy.data.meshes.new("BobFloorHull")
    data.from_pydata(vertices, [], faces)
    data.update()
    part = bpy.data.objects.new(f"UCX_SM_BobArena_HighDetail_{len(collision):03d}", data)
    bpy.context.collection.objects.link(part)
    collision.append(part)

def box(x0, x1, y0, y1):
    hull([(x0, y0), (x1, y0), (x1, y1), (x0, y1)])

extent, center, radius = 6.2, 4.7, .66
low, high = center - radius, center + radius
for x0, x1 in [(-extent, -high), (-low, low), (high, extent)]:
    box(x0, x1, -extent, extent)
for x0 in (-high, low):
    for y0, y1 in [(-extent, -high), (-low, low), (high, extent)]:
        box(x0, x0 + radius * 2, y0, y1)
for cx in (-center, center):
    for cy in (-center, center):
        for i in range(32):
            angles = (2*pi*i/32, 2*pi*(i+1)/32)
            inner = [(cx+radius*cos(a), cy+radius*sin(a)) for a in angles]
            outer = [(cx+radius*cos(a)/max(abs(cos(a)),abs(sin(a))),
                      cy+radius*sin(a)/max(abs(cos(a)),abs(sin(a)))) for a in angles]
            hull([inner[0], outer[0], outer[1], inner[1]])
bpy.ops.object.select_all(action="DESELECT")
obj.select_set(True)
for part in collision:
    part.select_set(True)
bpy.context.view_layer.objects.active = obj
(root / "exports").mkdir(exist_ok=True)
bpy.ops.export_scene.fbx(filepath=str(root / "exports/SM_BobArena_HighDetail.fbx"),
    use_selection=True, object_types={"MESH"}, use_mesh_modifiers=True, mesh_smooth_type="FACE",
    add_leaf_bones=False, bake_anim=False, axis_forward="-Z", axis_up="Y")
mesh = obj.data
mesh.calc_loop_triangles()
report = {"source": "BobArena.blend", "triangles": len(mesh.loop_triangles),
    "dimensions_m": size, "materials": [m.name for m in obj.data.materials],
    "corrected_ring_faces": len(ring_faces), "floor_collision_hulls": len(collision),
    "flattened_pocket_vertices": flattened_pocket_vertices}
(root / "exports/source_report.json").write_text(json.dumps(report, indent=2))
print("FLICK_BOB_EXPORT_COMPLETE " + json.dumps(report))
