"""Unreal commandlet: import the modular Switchyard arena workshop."""
import json
from pathlib import Path
import unreal as u


project = Path(u.Paths.project_dir())
source_root = project / "AssetDevelopment/Arena"
destination = "/Game/TestArena/Arena"
u.EditorAssetLibrary.make_directory(destination)
assets = u.AssetToolsHelpers.get_asset_tools()
editing = u.MaterialEditingLibrary
mesh_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)


def parameter(material, expression_type, name, value, prop):
    node = editing.create_material_expression(material, expression_type)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    editing.connect_material_property(node, "", prop)
    return node


parent = u.load_asset(destination + "/M_ArenaSurface")
if not parent:
    parent = assets.create_asset(
        "M_ArenaSurface", destination, u.Material, u.MaterialFactoryNew())
    base_color = parameter(
        parent, u.MaterialExpressionVectorParameter, "BaseColor",
        u.LinearColor(.02, .03, .04, 1), u.MaterialProperty.MP_BASE_COLOR)
    parameter(parent, u.MaterialExpressionScalarParameter, "Metallic", .4,
              u.MaterialProperty.MP_METALLIC)
    parameter(parent, u.MaterialExpressionScalarParameter, "Roughness", .5,
              u.MaterialProperty.MP_ROUGHNESS)
    emission = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    emission.set_editor_property("parameter_name", "Emission")
    emission.set_editor_property("default_value", 0.0)
    multiply = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(base_color, "", multiply, "A")
    editing.connect_material_expressions(emission, "", multiply, "B")
    editing.connect_material_property(multiply, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.recompile_material(parent)
    u.EditorAssetLibrary.save_loaded_asset(parent)


specs = {
    # The deck and mechanisms deliberately sit in a lighter blue-grey value
    # range than the puck chassis, preserving silhouettes at gameplay distance.
    "01_Arena_Graphite": ((.055, .075, .10), .68, .30, 0.0),
    "02_Arena_Surface": ((.10, .14, .19), .32, .55, 0.0),
    "03_Rim_Polymer": ((.016, .024, .035), .04, .60, 0.0),
    "04_Brushed_Titanium": ((.42, .50, .58), .92, .21, 0.0),
    "05_Deep_Recess": ((.002, .006, .012), .28, .55, 0.0),
    "06_Team_Cyan": ((0.0, .20, .48), .05, .32, 1.2),
    "07_Team_Orange": ((.75, .075, .003), .05, .32, 1.2),
    "08_Floor_Lines": ((.42, .51, .60), .48, .42, 0.0),
    "09_Switch_Accent": ((.02, .48, .72), .12, .42, .65),
}
materials = {}
for key, (rgb, metallic, roughness, emission) in specs.items():
    name = "MI_" + key
    material = u.load_asset(destination + "/" + name)
    if not material:
        material = assets.create_asset(
            name, destination, u.MaterialInstanceConstant,
            u.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(material, parent)
    editing.set_material_instance_vector_parameter_value(
        material, "BaseColor", u.LinearColor(*rgb, 1))
    editing.set_material_instance_scalar_parameter_value(material, "Metallic", metallic)
    editing.set_material_instance_scalar_parameter_value(material, "Roughness", roughness)
    editing.set_material_instance_scalar_parameter_value(material, "Emission", emission)
    u.EditorAssetLibrary.save_loaded_asset(material)
    materials[key] = material


manifest = json.loads((source_root / "dimensions.json").read_text())
asset_names = manifest["exports"]
report = []
for name in asset_names:
    task = u.AssetImportTask()
    task.filename = str(source_root / "exports" / (name + ".fbx"))
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
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
    data.generate_lightmap_u_vs = True
    data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    task.factory = u.FbxFactory()
    assets.import_asset_tasks([task])

    mesh = u.load_asset(destination + "/" + name)
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError("Mesh import failed: " + name)
    settings = mesh_editor.get_lod_build_settings(mesh, 0)
    settings.recompute_normals = False
    settings.recompute_tangents = True
    settings.use_mikk_t_space = True
    settings.remove_degenerates = True
    mesh_editor.set_lod_build_settings(mesh, 0, settings)

    mapped = []
    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        matches = [key for key in materials if key in slot_name]
        if len(matches) != 1:
            raise RuntimeError(f"Unknown material slot on {name}: {slot_name}")
        mesh.set_material(index, materials[matches[0]])
        mapped.append(matches[0])
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    bounds = mesh.get_bounds()
    report.append({
        "name": name,
        "dimensions_cm": [
            bounds.box_extent.x * 2.0,
            bounds.box_extent.y * 2.0,
            bounds.box_extent.z * 2.0,
        ],
        "materials": mapped,
    })


by_name = {row["name"]: row for row in report}
static_size = by_name["SM_TestArena_Static"]["dimensions_cm"]
divider_size = by_name["SM_TestArena_Divider"]["dimensions_cm"]
switch_size = by_name["SM_TestArena_SwitchHousing"]["dimensions_cm"]
if max(abs(static_size[0] - 1300.0), abs(static_size[1] - 1300.0)) > 1.0:
    raise RuntimeError("Arena import scale mismatch: " + str(static_size))
if max(abs(divider_size[0] - 146.0), abs(divider_size[1] - 18.0)) > 1.0:
    raise RuntimeError("Divider import scale mismatch: " + str(divider_size))
if max(abs(switch_size[0] - 80.0), abs(switch_size[1] - 80.0)) > 1.0:
    raise RuntimeError("Switch import scale mismatch: " + str(switch_size))

(project / "Saved/ArenaImportReport.json").write_text(json.dumps(report, indent=2))
u.log("FLICK_ARENA_IMPORT_COMPLETE: six modular meshes and nine native materials")
