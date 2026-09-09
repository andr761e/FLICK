"""Import the supplied high-detail blue Standard puck as an isolated prototype."""
import json
from pathlib import Path
import unreal as u


project = Path(u.Paths.project_dir())
source_root = project / "AssetDevelopment/Pucks/HighDetail"
destination = "/Game/TestArena/Pucks/PrototypeStandard"
u.EditorAssetLibrary.make_directory(destination)
assets = u.AssetToolsHelpers.get_asset_tools()
editing = u.MaterialEditingLibrary
shared_parent = u.load_asset("/Game/TestArena/Pucks/M_PuckSurface")
if not shared_parent:
    raise RuntimeError("Shared puck material M_PuckSurface is missing")

# Give the prototype its own parent so anisotropic brushed-metal highlights can
# be evaluated without changing any of the production workshop pucks.
parent_path = destination + "/M_BlueStandardSurface"
parent = u.load_asset(parent_path)
if not parent:
    parent = assets.duplicate_asset("M_BlueStandardSurface", destination, shared_parent)
    anisotropy = editing.create_material_expression(
        parent, u.MaterialExpressionScalarParameter, -420, 520)
    anisotropy.set_editor_property("parameter_name", "Anisotropy")
    anisotropy.set_editor_property("default_value", 0.0)
    editing.connect_material_property(anisotropy, "", u.MaterialProperty.MP_ANISOTROPY)
    editing.recompile_material(parent)
    u.EditorAssetLibrary.save_loaded_asset(parent)
if not parent:
    raise RuntimeError("Could not create the blue Standard prototype material parent")

# Clean premium hierarchy from the supplied reference: bright machined silver,
# readable reflective graphite, deep recesses, and sharp cyan embedded lights.
# Values are tuned for the test arena's fixed exposure and restrained bloom.
# Tuple: base colour, metallic, roughness, emission, reflection/surface lift,
# anisotropy. The latter stretches highlights around the authored circular grain.
specs = {
    "Graphite anodized housing": ((.050, .066, .086), .72, .27, 0.0, .075, .25),
    "Circular brushed silver": ((.76, .82, .89), .97, .21, 0.0, .46, .72),
    "Recessed dark titanium": ((.026, .038, .052), .76, .31, 0.0, .040, .35),
    "Cyan light diffuser": ((.004, .080, .125), .08, .22, 6.0, 0.0, 0.0),
    "Black gasket and sockets": ((.012, .019, .028), .22, .38, 0.0, .012, 0.0),
    "Machined edge highlights": ((.34, .42, .50), .95, .20, 0.0, .22, .55),
    "Cyan center emblem": ((.003, .065, .10), .10, .23, 6.0, 0.0, 0.0),
}

materials = {}
for index, (source_name, (rgb, metallic, roughness, emission, lift, anisotropy)) in enumerate(specs.items(), 1):
    safe_name = source_name.replace(" ", "_").replace("and", "And")
    name = f"MI_{index:02d}_{safe_name}"
    material = u.load_asset(destination + "/" + name)
    if not material:
        material = assets.create_asset(
            name, destination, u.MaterialInstanceConstant,
            u.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(material, parent)
    editing.set_material_instance_vector_parameter_value(
        material, "BaseColor", u.LinearColor(*rgb, 1))
    editing.set_material_instance_vector_parameter_value(
        material, "TeamColor", u.LinearColor(0.0, .55, 1.0, 1.0))
    for parameter, value in (
        ("Metallic", metallic), ("Roughness", roughness),
        ("Emission", emission), ("SurfaceLift", lift),
        ("Anisotropy", anisotropy)):
        editing.set_material_instance_scalar_parameter_value(material, parameter, value)
    u.EditorAssetLibrary.save_loaded_asset(material)
    materials[source_name] = material

mesh_path = destination + "/SM_Puck_Standard_Blue_Prototype"
source_file = source_root / "exports/Standard.fbx"
if not source_file.is_file():
    raise RuntimeError("Blue Standard source FBX is missing: " + str(source_file))
task = u.AssetImportTask()
task.filename = str(source_file)
task.destination_path = destination
task.destination_name = "SM_Puck_Standard_Blue_Prototype"
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
data.generate_lightmap_u_vs = False
data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
task.options = options
task.factory = u.FbxFactory()
# Keep this asset path stable: it is the production Standard puck despite the
# historical PrototypeStandard folder/name retained for compatibility.
assets.import_asset_tasks([task])
mesh = u.load_asset(mesh_path)

if not isinstance(mesh, u.StaticMesh):
    raise RuntimeError("Blue Standard prototype import failed")
if len(mesh.get_editor_property("static_materials")) != len(specs):
    raise RuntimeError("Blue Standard prototype lost its authored material sections")

mapped = []
for slot_index, slot in enumerate(mesh.get_editor_property("static_materials")):
    slot_name = str(slot.get_editor_property("material_slot_name"))
    normalized_slot = slot_name.replace("_", " ").lower()
    matches = [key for key in materials if key.lower() in normalized_slot]
    if len(matches) != 1:
        raise RuntimeError("Unknown prototype material slot: " + slot_name)
    mesh.set_material(slot_index, materials[matches[0]])
    mapped.append(matches[0])

bounds = mesh.get_bounds()
dimensions = [bounds.box_extent.x * 2.0, bounds.box_extent.y * 2.0, bounds.box_extent.z * 2.0]
if max(abs(dimensions[0] - 90.0), abs(dimensions[1] - 90.0), abs(dimensions[2] - 20.0)) > .25:
    raise RuntimeError("Blue Standard prototype unit mismatch: " + str(dimensions))
if bounds.origin.length() > 1.0:
    raise RuntimeError("Blue Standard prototype pivot is not centered: " + str(bounds.origin))
u.EditorAssetLibrary.save_loaded_asset(mesh)
(project / "Saved/BlueStandardPrototypeImportReport.json").write_text(json.dumps({
    "dimensions_cm": dimensions,
    "materials": mapped,
}, indent=2))
u.log("FLICK_BLUE_STANDARD_PROTOTYPE_IMPORT_COMPLETE")
