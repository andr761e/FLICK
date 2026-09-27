"""Import the collision-free Blender presentation for the classic arena."""
from pathlib import Path
import unreal as u

project = Path(u.Paths.project_dir())
source = project / "AssetDevelopment/ClassicArena/exports/SM_ClassicArena_Premium.fbx"
destination = "/Game/ClassicArena"
name = "SM_ClassicArena_Premium"
u.EditorAssetLibrary.make_directory(destination)
assets = u.AssetToolsHelpers.get_asset_tools()
editing = u.MaterialEditingLibrary
parent = u.load_asset("/Game/TestArena/Arena/M_ArenaSurface_PremiumV2")
if not parent:
    raise RuntimeError("Classic arena material parent is missing")

# Values mirror the editable Blender source. The existing material parent adds
# a subtle roughness breakup without bringing in texture dependencies.
specs = {
    "01_Deck_Graphite": ((.105, .145, .170), .42, .36, 0.0, .018),
    "02_Field_Slate": ((.245, .315, .335), .17, .48, 0.0, .022),
    "03_Gunmetal_Rail": ((.024, .039, .051), .78, .27, 0.0, .025),
    "04_Satin_Titanium": ((.67, .73, .72), .86, .24, 0.0, .035),
    "05_Deep_Recess": ((.008, .018, .025), .30, .52, 0.0, .005),
    "06_Floor_Marking": ((.82, .82, .72), .30, .37, 0.0, .015),
    "07_Warm_Light": ((1.0, .57, .18), .04, .22, 1.7, 0.0),
    "08_Warm_Metal": ((.62, .42, .19), .82, .28, 0.0, .025),
}
materials = {}
for key, (rgb, metallic, roughness, emission, lift) in specs.items():
    asset_name = "MI_" + key
    instance = u.load_asset(destination + "/" + asset_name)
    if not instance:
        instance = assets.create_asset(
            asset_name, destination, u.MaterialInstanceConstant,
            u.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(instance, parent)
    editing.set_material_instance_vector_parameter_value(
        instance, "BaseColor", u.LinearColor(*rgb, 1.0))
    for parameter, value in (
        ("Metallic", metallic), ("Roughness", roughness),
        ("Emission", emission), ("SurfaceLift", lift),
        ("RenderLayerOffset", 0.0)):
        editing.set_material_instance_scalar_parameter_value(instance, parameter, value)
    u.EditorAssetLibrary.save_loaded_asset(instance)
    materials[key] = instance

task = u.AssetImportTask()
task.filename = str(source)
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
    raise RuntimeError("Classic arena mesh import failed")
for index, slot in enumerate(mesh.get_editor_property("static_materials")):
    slot_name = str(slot.get_editor_property("material_slot_name"))
    matches = [key for key in specs if key in slot_name]
    if len(matches) != 1:
        raise RuntimeError("Unknown classic arena material slot: " + slot_name)
    mesh.set_material(index, materials[matches[0]])
u.EditorAssetLibrary.save_loaded_asset(mesh)
bounds = mesh.get_bounds().box_extent
if abs(bounds.x - 650.0) > 1.0 or abs(bounds.y - 650.0) > 1.0:
    raise RuntimeError(f"Classic arena radius changed during import: {bounds}")
u.log("FLICK_CLASSIC_ARENA_IMPORT_COMPLETE")
