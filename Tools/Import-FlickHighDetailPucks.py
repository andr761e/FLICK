"""Import the eight high-detail puck meshes and generate blue/orange team variants."""
import json
from pathlib import Path
import unreal as u


project = Path(u.Paths.project_dir())
source_root = project / "AssetDevelopment/Pucks/HighDetail"
destination = "/Game/TestArena/Pucks/HighDetail"
standard_material_root = "/Game/TestArena/Pucks/PrototypeStandard"
u.EditorAssetLibrary.make_directory(destination)
assets = u.AssetToolsHelpers.get_asset_tools()
editing = u.MaterialEditingLibrary

parent = u.load_asset(standard_material_root + "/M_BlueStandardSurface")
if not parent:
    raise RuntimeError("High-detail Standard material parent is missing")

material_assets = {
    "Graphite anodized housing": "MI_01_Graphite_anodized_housing",
    "Circular brushed silver": "MI_02_Circular_brushed_silver",
    "Recessed dark titanium": "MI_03_Recessed_dark_titanium",
    "Cyan light diffuser": "MI_04_Cyan_light_diffuser",
    "Black gasket and sockets": "MI_05_Black_gasket_And_sockets",
    "Machined edge highlights": "MI_06_Machined_edge_highlights",
    "Cyan center emblem": "MI_07_Cyan_center_emblem",
}
materials = {}
for source_name, asset_name in material_assets.items():
    material = u.load_asset(standard_material_root + "/" + asset_name)
    if not material:
        raise RuntimeError("Missing approved Standard material: " + asset_name)
    materials[source_name] = material


def team_material(name, base_color, team_color, roughness):
    material = u.load_asset(destination + "/" + name)
    if not material:
        material = assets.create_asset(
            name, destination, u.MaterialInstanceConstant,
            u.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(material, parent)
    editing.set_material_instance_vector_parameter_value(
        material, "BaseColor", u.LinearColor(*base_color, 1.0))
    editing.set_material_instance_vector_parameter_value(
        material, "TeamColor", u.LinearColor(*team_color, 1.0))
    for parameter, value in (
            ("Metallic", 0.08), ("Roughness", roughness),
            ("Emission", 6.0), ("SurfaceLift", 0.0), ("Anisotropy", 0.0)):
        editing.set_material_instance_scalar_parameter_value(material, parameter, value)
    u.EditorAssetLibrary.save_loaded_asset(material)
    return material


# These are the only orange-specific assets: all geometry and neutral graphite /
# silver materials are intentionally shared with blue.
orange_diffuser = team_material(
    "MI_Orange_Light_Diffuser", (.125, .022, .002), (1.0, .18, .003), .22)
orange_emblem = team_material(
    "MI_Orange_Center_Emblem", (.10, .016, .002), (1.0, .18, .003), .23)

manifest = json.loads((source_root / "manifests/archetype_exports.json").read_text())
report = []
for row in manifest:
    name = row["asset"]
    mesh_path = destination + "/SM_Puck_" + name + "_HighDetail"
    mesh = u.load_asset(mesh_path)
    if not isinstance(mesh, u.StaticMesh) or len(mesh.get_editor_property("static_materials")) != 7:
        if mesh:
            u.EditorAssetLibrary.delete_asset(mesh_path)
        task = u.AssetImportTask()
        task.filename = str(source_root / "exports" / (name + ".fbx"))
        task.destination_path = destination
        task.destination_name = "SM_Puck_" + name + "_HighDetail"
        task.automated = True
        task.replace_existing = False
        task.save = True
        options = u.FbxImportUI()
        options.import_mesh = True
        options.import_materials = False
        options.import_textures = False
        options.import_as_skeletal = False
        options.automated_import_should_detect_type = False
        options.mesh_type_to_import = u.FBXImportType.FBXIT_STATIC_MESH
        data = options.static_mesh_import_data
        data.combine_meshes = True
        data.auto_generate_collision = False
        data.generate_lightmap_u_vs = False
        data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.options = options
        task.factory = u.FbxFactory()
        assets.import_asset_tasks([task])
        mesh = u.load_asset(mesh_path)

    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError(name + ": mesh import failed")
    slots = mesh.get_editor_property("static_materials")
    if len(slots) != 7:
        raise RuntimeError(name + ": authored material sections were not preserved")
    mapped = []
    for slot_index, slot in enumerate(slots):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        normalized = slot_name.replace("_", " ").lower()
        matches = [key for key in materials if key.lower() in normalized]
        if len(matches) != 1:
            raise RuntimeError(name + ": unknown material slot " + slot_name)
        mesh.set_material(slot_index, materials[matches[0]])
        mapped.append(matches[0])

    bounds = mesh.get_bounds()
    measured = [bounds.box_extent.x * 2.0, bounds.box_extent.y * 2.0, bounds.box_extent.z * 2.0]
    expected = row["dimensions_cm"]
    if max(abs(actual - wanted) for actual, wanted in zip(measured, expected)) > .3:
        raise RuntimeError(name + ": Unreal dimensions mismatch " + str(measured))
    if bounds.origin.length() > 1.0:
        raise RuntimeError(name + ": pivot is not centered " + str(bounds.origin))
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    report.append({
        "asset": name,
        "dimensions_cm": measured,
        "materials": mapped,
        "orange_variant_materials": [
            orange_diffuser.get_path_name(), orange_emblem.get_path_name()],
    })

(project / "Saved/HighDetailPuckImportReport.json").write_text(json.dumps(report, indent=2))
u.log("FLICK_HIGH_DETAIL_PUCK_IMPORT_COMPLETE")
