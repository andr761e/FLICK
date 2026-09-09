"""Unreal commandlet: import the high-detail, visual-only BOB arena."""
import json
from pathlib import Path

import unreal as u


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
    "01_BOB_Surface": ((0.58, 0.64, 0.68), 0.08, 0.43, 0.50, 0.00, 0.018),
    "02_BOB_Wood": ((0.20, 0.095, 0.035), 0.02, 0.31, 0.50, 0.00, 0.028),
    "03_BOB_InnerWall": ((0.60, 0.66, 0.70), 0.22, 0.28, 0.58, 0.00, 0.012),
    "04_BOB_Red": ((0.66, 0.012, 0.022), 0.12, 0.27, 0.52, 0.08, 0.006),
    "05_BOB_PocketRed": ((0.80, 0.008, 0.015), 0.18, 0.21, 0.58, 0.90, 0.006),
    "06_BOB_PocketVoid": ((0.012, 0.010, 0.014), 0.18, 0.48, 0.38, 0.00, 0.004),
    "07_BOB_Silver": ((0.70, 0.77, 0.82), 0.96, 0.19, 0.70, 0.00, 0.014),
    "08_BOB_DarkMetal": ((0.032, 0.045, 0.062), 0.74, 0.24, 0.62, 0.00, 0.012),
    "09_BOB_PocketBottom": ((0.085, 0.006, 0.011), 0.24, 0.31, 0.52, 0.10, 0.005),
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
data.generate_lightmap_u_vs = True
data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
task.options = options
task.factory = u.FbxFactory()
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
dimensions = [bounds.box_extent.x * 2.0, bounds.box_extent.y * 2.0, bounds.box_extent.z * 2.0]
if abs(dimensions[0] - 1308.0) > 1.0 or abs(dimensions[1] - 1308.0) > 1.0:
    raise RuntimeError("BOB arena scale mismatch: " + str(dimensions))
(project / "Saved/BobArenaImportReport.json").write_text(json.dumps({
    "asset": asset_name,
    "dimensions_cm": dimensions,
    "materials": mapped,
}, indent=2))
u.log("FLICK_BOB_ARENA_IMPORT_COMPLETE")
