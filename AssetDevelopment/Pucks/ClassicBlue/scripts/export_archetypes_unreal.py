"""Export the eight supplied high-detail GLBs as section-preserving Unreal FBXs."""
import bpy
import json
from pathlib import Path


root = Path(__file__).resolve().parent.parent
source_root = root / "source/archetypes/models"
export_root = root / "exports"
export_root.mkdir(parents=True, exist_ok=True)
dimensions = json.loads((root / "manifests/archetype_dimensions.json").read_text())
expected = {row["asset"]: row for row in dimensions}
report = []

for name, row in expected.items():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)
    bpy.ops.import_scene.gltf(filepath=str(source_root / f"{name}.glb"))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"{name}: supplied GLB contains no visual meshes")

    # Deterministic planar UVs provide tangents for the anisotropic metal shader.
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
    puck.name = f"SM_Puck_{name}_HighDetail"
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

    minimum_z = min(vertex.co.z for vertex in puck.data.vertices)
    maximum_z = max(vertex.co.z for vertex in puck.data.vertices)
    center_z = (minimum_z + maximum_z) * 0.5
    for vertex in puck.data.vertices:
        vertex.co.z -= center_z
    puck.data.update()

    measured = [float(value) * 100.0 for value in puck.dimensions]
    wanted = [row["diameter_cm"], row["diameter_cm"], row["visible_height_cm"]]
    if max(abs(actual - target) for actual, target in zip(measured, wanted)) > 0.25:
        raise RuntimeError(f"{name}: dimensions {measured} do not match {wanted}")

    bpy.ops.export_scene.fbx(
        filepath=str(export_root / f"{name}.fbx"), use_selection=True,
        object_types={"MESH"}, add_leaf_bones=False, axis_forward="-Y", axis_up="Z",
        mesh_smooth_type="FACE", use_tspace=False)
    material_names = [slot.name for slot in puck.material_slots]
    if len(material_names) != 7:
        raise RuntimeError(f"{name}: expected seven material families, got {material_names}")
    report.append({
        "asset": name,
        "dimensions_cm": [round(value, 3) for value in measured],
        "triangles": sum(len(poly.vertices) - 2 for poly in puck.data.polygons),
        "materials": material_names,
    })

(root / "manifests/archetype_exports.json").write_text(json.dumps(report, indent=2))
print("FLICK_HIGH_DETAIL_COLLECTION_EXPORT_COMPLETE", len(report))
