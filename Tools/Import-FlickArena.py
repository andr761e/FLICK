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
materials_only = "-FlickArenaMaterialsOnly" in u.SystemLibrary.get_command_line()


def parameter(material, expression_type, name, value, prop):
    node = editing.create_material_expression(material, expression_type)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    editing.connect_material_property(node, "", prop)
    return node


parent_path = destination + "/M_ArenaSurface_PremiumV2"
parent = u.load_asset(parent_path)
if not parent:
    parent = assets.create_asset(
        "M_ArenaSurface_PremiumV2", destination, u.Material, u.MaterialFactoryNew())
    # Build a versioned parent once. Reusing it on later imports avoids Unreal's
    # unsafe DeleteAllMaterialExpressions path on loaded/rooted expressions.
    base_color = parameter(
        parent, u.MaterialExpressionVectorParameter, "BaseColor",
        u.LinearColor(.02, .03, .04, 1), u.MaterialProperty.MP_BASE_COLOR)
    parameter(parent, u.MaterialExpressionScalarParameter, "Metallic", .4,
              u.MaterialProperty.MP_METALLIC)
    roughness = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter, -700, 80)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", .4)
    texcoord = editing.create_material_expression(parent, u.MaterialExpressionTextureCoordinate, -700, 220)
    texcoord.set_editor_property("u_tiling", 56.0)
    texcoord.set_editor_property("v_tiling", 8.0)
    noise = editing.create_material_expression(parent, u.MaterialExpressionNoise, -480, 220)
    noise.set_editor_property("scale", 1.0)
    noise.set_editor_property("quality", 1)
    noise.set_editor_property("levels", 1)
    noise.set_editor_property("output_min", -.018)
    noise.set_editor_property("output_max", .018)
    editing.connect_material_expressions(texcoord, "", noise, "Position")
    roughness_add = editing.create_material_expression(parent, u.MaterialExpressionAdd, -220, 100)
    editing.connect_material_expressions(roughness, "", roughness_add, "A")
    editing.connect_material_expressions(noise, "", roughness_add, "B")
    editing.connect_material_property(roughness_add, "", u.MaterialProperty.MP_ROUGHNESS)
    parameter(parent, u.MaterialExpressionScalarParameter, "Specular", .5,
              u.MaterialProperty.MP_SPECULAR)
    parameter(parent, u.MaterialExpressionScalarParameter, "Anisotropy", 0.0,
              u.MaterialProperty.MP_ANISOTROPY)
    # Tiny render-only vertical offsets establish a stable order for flush
    # inlays. Unreal 5.6 does not expose PixelDepthOffset to Python, so use the
    # supported World Position Offset input. Authored geometry and collision do
    # not move; only the rendered vertices shift by fractions of a centimetre.
    layer_direction = editing.create_material_expression(parent, u.MaterialExpressionVectorParameter)
    layer_direction.set_editor_property("parameter_name", "LayerDirection")
    layer_direction.set_editor_property("default_value", u.LinearColor(0.0, 0.0, 1.0, 0.0))
    layer_offset = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    layer_offset.set_editor_property("parameter_name", "RenderLayerOffset")
    layer_offset.set_editor_property("default_value", 0.0)
    layer_multiply = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(layer_direction, "", layer_multiply, "A")
    editing.connect_material_expressions(layer_offset, "", layer_multiply, "B")
    editing.connect_material_property(layer_multiply, "", u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    emission = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    emission.set_editor_property("parameter_name", "Emission")
    emission.set_editor_property("default_value", 0.0)
    emissive_multiply = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(base_color, "", emissive_multiply, "A")
    editing.connect_material_expressions(emission, "", emissive_multiply, "B")
    surface_lift = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    surface_lift.set_editor_property("parameter_name", "SurfaceLift")
    surface_lift.set_editor_property("default_value", 0.0)
    surface_multiply = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(base_color, "", surface_multiply, "A")
    editing.connect_material_expressions(surface_lift, "", surface_multiply, "B")
    emissive_add = editing.create_material_expression(parent, u.MaterialExpressionAdd)
    editing.connect_material_expressions(emissive_multiply, "", emissive_add, "A")
    editing.connect_material_expressions(surface_multiply, "", emissive_add, "B")
    editing.connect_material_property(emissive_add, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.recompile_material(parent)
    u.EditorAssetLibrary.save_loaded_asset(parent)


specs = {
    # A diffuse blue-grey deck gives the bright puck metal a mid-value backdrop.
    # Emissive strength is deliberately restrained; localized bloom is handled
    # by the test camera instead of letting the arena create a broad haze.
    # Tuple: colour, metallic, roughness, strip emission, ambient surface lift,
    # specular response, anisotropy, render-only Z offset in centimetres.
    "01_Arena_Graphite": ((.18, .225, .29), .55, .28, 0.0, .12, .62, .28, -.05),
    "02_Arena_Surface": ((.62, .70, .79), .12, .46, 0.0, .24, .55, .06, -.30),
    "03_Rim_Polymer": ((.055, .078, .11), .22, .36, 0.0, .035, .58, .08, -.25),
    "04_Brushed_Titanium": ((.72, .79, .87), .94, .20, 0.0, .22, .72, .68, -.20),
    "05_Deep_Recess": ((.022, .034, .05), .34, .34, 0.0, .008, .48, .12, -.10),
    # Bright embedded rim lenses: strong enough to read as arena lighting, but
    # still below the puck LEDs and restrained enough to preserve a sharp edge.
    "06_Team_Cyan": ((0.0, .32, .76), .06, .25, 2.75, 0.0, .55, 0.0, 0.0),
    "07_Team_Orange": ((.95, .16, .008), .06, .25, 2.75, 0.0, .55, 0.0, 0.0),
    # Marking separation is baked into the non-colliding visual mesh. Avoid
    # relying on WPO here: actual vertex separation is stable under every view.
    "08_Floor_Lines": ((.79, .85, .92), .48, .27, 0.0, .10, .66, .24, 0.0),
    "09_Switch_Accent": ((.025, .48, .72), .12, .32, .72, 0.0, .55, 0.0, 0.0),
    "10_Inner_Field": ((.53, .61, .70), .16, .42, 0.0, .20, .58, .08, -.20),
    "11_Center_Inset": ((.70, .77, .84), .28, .30, 0.0, .18, .64, .16, -.15),
    "12_Accent_Metal": ((.43, .51, .61), .90, .23, 0.0, .18, .70, .56, -.15),
    "13_Dark_Marking": ((.075, .105, .15), .40, .32, 0.0, .025, .56, .18, 0.0),
}
materials = {}
for key, (rgb, metallic, roughness, emission, lift, specular, anisotropy, layer_offset) in specs.items():
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
    editing.set_material_instance_scalar_parameter_value(material, "SurfaceLift", lift)
    editing.set_material_instance_scalar_parameter_value(material, "Specular", specular)
    editing.set_material_instance_scalar_parameter_value(material, "Anisotropy", anisotropy)
    editing.set_material_instance_scalar_parameter_value(material, "RenderLayerOffset", layer_offset)
    u.EditorAssetLibrary.save_loaded_asset(material)
    materials[key] = material

# The flush mechanisms sit directly over the broad arena surface and need a
# wider render-order separation than the rim's adjacent (non-overlapping)
# bands. These variants reuse the exact same colour and surface values.
mechanism_offsets = {
    "12_Accent_Metal": -.05,
    "04_Brushed_Titanium": .05,
    "05_Deep_Recess": .15,
    "01_Arena_Graphite": .25,
    "10_Inner_Field": .35,
    "09_Switch_Accent": .50,
}
mechanism_materials = {}
for key, layer_offset in mechanism_offsets.items():
    rgb, metallic, roughness, emission, lift, specular, anisotropy, _ = specs[key]
    name = "MI_Flush_" + key
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
    editing.set_material_instance_scalar_parameter_value(material, "SurfaceLift", lift)
    editing.set_material_instance_scalar_parameter_value(material, "Specular", specular)
    editing.set_material_instance_scalar_parameter_value(material, "Anisotropy", anisotropy)
    editing.set_material_instance_scalar_parameter_value(material, "RenderLayerOffset", layer_offset)
    u.EditorAssetLibrary.save_loaded_asset(material)
    mechanism_materials[key] = material


manifest = json.loads((source_root / "dimensions.json").read_text())
asset_names = manifest["exports"]
flush_mechanism_names = {
    "SM_TestArena_DividerSocket",
    "SM_TestArena_SwitchHousing",
    "SM_TestArena_SwitchDot",
    "SM_TestArena_SignalTrace",
}
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
    if not materials_only:
        # Reimport in place so every C++ soft reference remains live. Deleting a
        # referenced StaticMesh before import can leave Unreal's asset registry
        # holding a pending-kill object for the rest of the commandlet session.
        assets.import_asset_tasks([task])

    mesh = u.load_asset(destination + "/" + name)
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError("Mesh import failed: " + name)
    if not materials_only:
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
        if not materials_only:
            key = matches[0]
            material = mechanism_materials.get(key, materials[key]) \
                if name in flush_mechanism_names else materials[key]
            mesh.set_material(index, material)
        mapped.append(matches[0])
    if not materials_only:
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
u.log("FLICK_ARENA_IMPORT_COMPLETE: six modular meshes, thirteen base materials and six flush-mechanism variants")
