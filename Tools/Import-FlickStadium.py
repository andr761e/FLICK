"""Unreal commandlet: import the visual-only Test Arena stadium."""

import json
from pathlib import Path

import unreal as u

project = Path(u.Paths.project_dir())
source_root = project / "AssetDevelopment/Stadium"
destination = "/Game/TestArena/Stadium"
u.EditorAssetLibrary.make_directory(destination)
assets = u.AssetToolsHelpers.get_asset_tools()
editing = u.MaterialEditingLibrary
mesh_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)

parent_path = destination + "/M_StadiumSurface"
parent = u.load_asset(parent_path)
if not parent:
    parent = assets.create_asset("M_StadiumSurface", destination, u.Material, u.MaterialFactoryNew())
    def parameter(expression_type, name, value, prop):
        node = editing.create_material_expression(parent, expression_type)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", value)
        editing.connect_material_property(node, "", prop)
        return node
    base = parameter(u.MaterialExpressionVectorParameter, "BaseColor", u.LinearColor(.2, .24, .3, 1), u.MaterialProperty.MP_BASE_COLOR)
    parameter(u.MaterialExpressionScalarParameter, "Metallic", .2, u.MaterialProperty.MP_METALLIC)
    parameter(u.MaterialExpressionScalarParameter, "Roughness", .4, u.MaterialProperty.MP_ROUGHNESS)
    emission = editing.create_material_expression(parent, u.MaterialExpressionScalarParameter)
    emission.set_editor_property("parameter_name", "Emission")
    emission.set_editor_property("default_value", 0.0)
    multiply = editing.create_material_expression(parent, u.MaterialExpressionMultiply)
    editing.connect_material_expressions(base, "", multiply, "A")
    editing.connect_material_expressions(emission, "", multiply, "B")
    editing.connect_material_property(multiply, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    editing.recompile_material(parent)
    u.EditorAssetLibrary.save_loaded_asset(parent)

specs = {
    # Neutral architectural palette: bright enough to separate the stadium
    # from the arena, without borrowing either team's cyan/orange identity.
    "01_Porcelain_Composite": ((.72, .77, .84), .08, .36, 0.0),
    "02_Pale_Concrete": ((.52, .57, .64), .04, .56, 0.0),
    "03_Graphite_Inset": ((.018, .032, .055), .46, .28, 0.0),
    "04_Brushed_Aluminium": ((.56, .65, .75), .93, .22, 0.0),
    "05_Warm_Architectural_Light": ((1.0, .86, .72), .05, .22, 3.0),
    "06_Team_Cyan_Light": ((.0, .30, 1.0), .04, .18, 4.8),
    "07_Team_Orange_Light": ((1.0, .10, .005), .04, .18, 4.8),
    "08_Deep_Recess": ((.006, .012, .022), .30, .34, 0.0),
}
materials = {}
for key, (rgb, metallic, roughness, emission) in specs.items():
    name = "MI_" + key
    instance = u.load_asset(destination + "/" + name)
    if not instance:
        instance = assets.create_asset(name, destination, u.MaterialInstanceConstant, u.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(instance, parent)
    editing.set_material_instance_vector_parameter_value(instance, "BaseColor", u.LinearColor(*rgb, 1))
    editing.set_material_instance_scalar_parameter_value(instance, "Metallic", metallic)
    editing.set_material_instance_scalar_parameter_value(instance, "Roughness", roughness)
    editing.set_material_instance_scalar_parameter_value(instance, "Emission", emission)
    u.EditorAssetLibrary.save_loaded_asset(instance)
    materials[key] = instance

manifest = json.loads((source_root / "manifest.json").read_text())
report = []
for name in manifest["exports"]:
    task = u.AssetImportTask()
    task.filename = str(source_root / "exports" / f"{name}.fbx")
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
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = False
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    options.static_mesh_import_data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
    task.options = options
    task.factory = u.FbxFactory()
    assets.import_asset_tasks([task])
    mesh = u.load_asset(destination + "/" + name)
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError("Stadium mesh import failed: " + name)
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
            raise RuntimeError(f"Unknown stadium material slot on {name}: {slot_name}")
        mesh.set_material(index, materials[matches[0]])
        mapped.append(matches[0])
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    bounds = mesh.get_bounds()
    report.append({"name": name, "dimensions_cm": [bounds.box_extent.x*2, bounds.box_extent.y*2, bounds.box_extent.z*2], "materials": mapped})

structure = next(row for row in report if row["name"] == "SM_TestStadium_Structure")
if structure["dimensions_cm"][0] < 3100.0 or structure["dimensions_cm"][0] > 3300.0:
    raise RuntimeError("Stadium import scale mismatch: " + str(structure["dimensions_cm"]))
(project / "Saved/StadiumImportReport.json").write_text(json.dumps(report, indent=2))
u.log("FLICK_STADIUM_IMPORT_COMPLETE")
