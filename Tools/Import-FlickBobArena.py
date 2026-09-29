"""Unreal commandlet: import the high-detail, visual-only BOB arena."""
import json
from pathlib import Path

import unreal as u

# Legacy FBX handles multiple UCX hulls reliably; Interchange 5.8 can retain
# cooked convex resources during reimport and trigger a Chaos assignment ensure.
u.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")


project = Path(u.Paths.project_dir())
source_root = project / "AssetDevelopment/BOB Arena"
destination = "/Game/BOB/Arena"
asset_name = "SM_BobArena_HighDetail"
u.EditorAssetLibrary.make_directory(destination)
assets = u.AssetToolsHelpers.get_asset_tools()
editing = u.MaterialEditingLibrary
mesh_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)


def parameter(parent, expression_type, name, value, prop):
    node = editing.create_material_expression(parent, expression_type)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    editing.connect_material_property(node, "", prop)
    return node


parent_path = destination + "/M_BobArena_Premium"
parent = u.load_asset(parent_path)
if not parent:
    parent = assets.create_asset("M_BobArena_Premium", destination, u.Material, u.MaterialFactoryNew())
    base = parameter(parent, u.MaterialExpressionVectorParameter, "BaseColor",
                     u.LinearColor(0.5, 0.5, 0.5, 1.0), u.MaterialProperty.MP_BASE_COLOR)
    parameter(parent, u.MaterialExpressionScalarParameter, "Metallic", 0.0,
              u.MaterialProperty.MP_METALLIC)
    roughness = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.45)
    noise_amount = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    noise_amount.set_editor_property("parameter_name", "RoughnessVariation")
    noise_amount.set_editor_property("default_value", 0.012)
    texture_coordinate = editing.create_material_expression(parent, u.MaterialExpressionTextureCoordinate)
    lever = editing.create_material_expression(parent, u.MaterialExpressionNoise)
    lever.set_editor_property("scale", 0.85)
    lever.set_editor_property("quality", 1)
    lever.set_editor_property("levels", 2)
    lever.set_editor_property("output_min", -1.0)
    lever.set_editor_property("output_max", 1.0)
    editing.connect_material_expressions(texture_coordinate, "", lever, "Position")
    variation = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(lever, "", variation, "A")
    editing.connect_material_expressions(noise_amount, "", variation, "B")
    combined_roughness = editing.create_material_expression(parent, u.MaterialExpressionAdd)
    editing.connect_material_expressions(roughness, "", combined_roughness, "A")
    editing.connect_material_expressions(variation, "", combined_roughness, "B")
    editing.connect_material_property(combined_roughness, "", u.MaterialProperty.MP_ROUGHNESS)
    parameter(parent, u.MaterialExpressionScalarParameter, "Specular", 0.5,
              u.MaterialProperty.MP_SPECULAR)
    emission = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    emission.set_editor_property("parameter_name", "Emission")
    emission.set_editor_property("default_value", 0.0)
    emissive = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(base, "", emissive, "A")
    editing.connect_material_expressions(emission, "", emissive, "B")
    editing.connect_material_property(emissive, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.recompile_material(parent)
    u.EditorAssetLibrary.save_loaded_asset(parent)


# Clean neutral surface, warm timber frame, cool metal trim, and restrained red
# artwork. The pocket liner is the only mildly emissive arena element.
specs = {
    "01_BOB_Surface": ((.76, .76, .67), .08, .42, .50, .00, .018),
    "02_BOB_Wood": ((.17, .075, .050), .02, .31, .50, .00, .028),
    "03_BOB_InnerWall": ((.59, .58, .51), .20, .32, .58, .00, .012),
    "04_BOB_Red": ((.56, .018, .038), .12, .27, .52, .08, .006),
    "05_BOB_PocketRed": ((.80, .025, .048), .18, .23, .58, .90, .006),
    "06_BOB_PocketVoid": ((.010, .012, .016), .12, .60, .38, .00, .004),
    "07_BOB_Silver": ((.76, .66, .45), .90, .22, .70, .00, .014),
    "08_BOB_DarkMetal": ((.035, .040, .048), .76, .27, .62, .00, .012),
    "09_BOB_PocketBottom": ((.09, .010, .021), .24, .32, .52, .10, .005),
}
materials = {}
for key, (rgb, metallic, roughness, specular, emission, variation) in specs.items():
    name = "MI_" + key
    instance = u.load_asset(destination + "/" + name)
    if not instance:
        instance = assets.create_asset(name, destination, u.MaterialInstanceConstant,
                                       u.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(instance, parent)
    editing.set_material_instance_vector_parameter_value(instance, "BaseColor", u.LinearColor(*rgb, 1.0))
    editing.set_material_instance_scalar_parameter_value(instance, "Metallic", metallic)
    editing.set_material_instance_scalar_parameter_value(instance, "Roughness", roughness)
    editing.set_material_instance_scalar_parameter_value(instance, "Specular", specular)
    editing.set_material_instance_scalar_parameter_value(instance, "Emission", emission)
    editing.set_material_instance_scalar_parameter_value(instance, "RoughnessVariation", variation)
    u.EditorAssetLibrary.save_loaded_asset(instance)
    materials[key] = instance


source_file = source_root / "exports" / (asset_name + ".fbx")
if not source_file.is_file():
    raise RuntimeError("BOB arena source FBX is missing: " + str(source_file))
task = u.AssetImportTask()
task.filename = str(source_file)
task.destination_path = destination
task.destination_name = asset_name
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
data.one_convex_hull_per_ucx = True
data.generate_lightmap_u_vs = True
data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
task.options = options
task.factory = u.FbxFactory()
# Reimport can otherwise retain previous automatic/full-board hulls in addition
# to the UCX openings. Clear collision before importing the authoritative set.
existing_mesh = u.load_asset(destination + "/" + asset_name)
if existing_mesh:
    mesh_editor.remove_collisions(existing_mesh)
assets.import_asset_tasks([task])

mesh = u.load_asset(destination + "/" + asset_name)
if not isinstance(mesh, u.StaticMesh):
    raise RuntimeError("BOB arena import failed")
settings = mesh_editor.get_lod_build_settings(mesh, 0)
settings.recompute_normals = False
settings.recompute_tangents = True
settings.use_mikk_t_space = True
settings.remove_degenerates = True
mesh_editor.set_lod_build_settings(mesh, 0, settings)
# Use only the authored floor hulls for both physics and traces. Complex artwork
# includes decorative pocket bottoms that must never stop a falling puck.
mesh.get_editor_property("body_setup").set_editor_property(
    "collision_trace_flag", u.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)

mapped = []
for index, slot in enumerate(mesh.get_editor_property("static_materials")):
    slot_name = str(slot.get_editor_property("material_slot_name"))
    matches = [key for key in materials if key in slot_name]
    if len(matches) != 1:
        raise RuntimeError("Unknown BOB arena material slot: " + slot_name)
    mesh.set_material(index, materials[matches[0]])
    mapped.append(matches[0])
u.EditorAssetLibrary.save_loaded_asset(mesh)

bounds = mesh.get_bounds()
if mesh_editor.get_convex_collision_count(mesh) < 9:
    raise RuntimeError("BOB arena is missing its pocket-cutout floor collision")
dimensions = [bounds.box_extent.x * 2.0, bounds.box_extent.y * 2.0, bounds.box_extent.z * 2.0]
if abs(dimensions[0] - 1308.0) > 1.0 or abs(dimensions[1] - 1308.0) > 1.0:
    raise RuntimeError("BOB arena scale mismatch: " + str(dimensions))
(project / "Saved/BobArenaImportReport.json").write_text(json.dumps({
    "asset": asset_name,
    "dimensions_cm": dimensions,
    "materials": mapped,
    "floor_collision_hulls": mesh_editor.get_convex_collision_count(mesh),
}, indent=2))
u.log("FLICK_BOB_ARENA_IMPORT_COMPLETE")
