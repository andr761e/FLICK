"""Import eight high-detail base meshes and Classic Orange cosmetic materials."""
import json
from pathlib import Path
import unreal as u

asset_selected = globals().get("asset_selected", lambda _project, _source: True)


project = Path(u.Paths.project_dir())
source_root = project / "AssetDevelopment/Pucks/ClassicBlue"
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
orange_set_root = project / "AssetDevelopment/Pucks/ClassicOrange"
orange_set = json.loads((orange_set_root / "set.json").read_text())
if (orange_set_root / orange_set["shared_geometry"]).resolve() != source_root.resolve():
    raise RuntimeError("Classic Orange must reference the Classic Blue geometry")
orange_names = ("MI_Orange_Light_Diffuser", "MI_Orange_Center_Emblem")
if set(orange_set["materials"]) != set(orange_names):
    raise RuntimeError("Classic Orange requires exactly the diffuser and emblem materials")
orange_materials = []
for name in orange_names:
    definition = orange_set["materials"][name]
    orange_materials.append(team_material(
        name, definition["base_color"], definition["emissive_color"], definition["roughness"]))
orange_diffuser, orange_emblem = orange_materials

manifest = json.loads((source_root / "manifests/archetype_exports.json").read_text())
report = []
for row in manifest:
    name = row["asset"]
    mesh_path = destination + "/SM_Puck_" + name + "_HighDetail"
    source_file = source_root / "exports" / (name + ".fbx")
    if not source_file.is_file():
        raise RuntimeError(name + ": source FBX is missing: " + str(source_file))
    if not asset_selected(project, source_file):
        continue
    task = u.AssetImportTask()
    task.filename = str(source_file)
    task.destination_path = destination
    task.destination_name = "SM_Puck_" + name + "_HighDetail"
    task.automated = True
    # Replace the object in place. Its /Game path remains stable, so every C++
    # soft reference and loaded map continues to resolve to the updated mesh.
    task.replace_existing = True
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
