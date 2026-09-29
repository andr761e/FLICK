"""Build one combined Unreal FBX from the supplied blue Standard puck GLB."""
import bpy
import json
from pathlib import Path


root = Path(__file__).resolve().parent.parent
source = root / "source/standard/Standard.glb"
output = root / "exports/Standard.fbx"

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(source))

meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
if not meshes:
    raise RuntimeError("The supplied GLB did not contain any puck meshes")

# Add deterministic per-face planar UVs. The prototype currently uses native
# scalar materials, but these coordinates also make later brushed-metal detail
# and tangent-based normal maps possible without changing the mesh again.
for obj in meshes:
    mesh = obj.data
    uv = mesh.uv_layers.active or mesh.uv_layers.new(name="SurfaceUV")
    for polygon in mesh.polygons:
        dominant = max(range(3), key=lambda axis: abs(polygon.normal[axis]))
        axes = [axis for axis in range(3) if axis != dominant]
        for loop_index in polygon.loop_indices:
            position = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            uv.data[loop_index].uv = (position[axes[0]], position[axes[1]])

bpy.ops.object.select_all(action="DESELECT")
for obj in meshes:
    obj.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.convert(target="MESH")
bpy.ops.object.join()
puck = bpy.context.object
puck.name = "SM_Puck_Standard_Blue_Prototype"
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

# The supplied art is authored on the floor (Z 0..20 cm), while FLICK's
# authoritative physics body and existing visual meshes are center-originated.
# Recenter mesh data without resizing or changing the rendered silhouette.
minimum_z = min(vertex.co.z for vertex in puck.data.vertices)
maximum_z = max(vertex.co.z for vertex in puck.data.vertices)
center_z = (minimum_z + maximum_z) * .5
for vertex in puck.data.vertices:
    vertex.co.z -= center_z
puck.data.update()

dimensions = tuple(float(value) for value in puck.dimensions)
if max(abs(dimensions[0] - .90), abs(dimensions[1] - .90), abs(dimensions[2] - .20)) > .002:
    raise RuntimeError(f"Unexpected prototype dimensions in metres: {dimensions}")

bpy.ops.export_scene.fbx(
    filepath=str(output), use_selection=True, object_types={"MESH"},
    add_leaf_bones=False, axis_forward="-Y", axis_up="Z",
    mesh_smooth_type="FACE", use_tspace=False)

(root / "manifests/standard_export.json").write_text(json.dumps({
    "asset": puck.name,
    "dimensions_cm": [round(value * 100.0, 3) for value in dimensions],
    "triangles": sum(len(polygon.vertices) - 2 for polygon in puck.data.polygons),
    "materials": [slot.name for slot in puck.material_slots],
}, indent=2))
print("FLICK_BLUE_STANDARD_EXPORT_COMPLETE", dimensions, len(puck.material_slots))
