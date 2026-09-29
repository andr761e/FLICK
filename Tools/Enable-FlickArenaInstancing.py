"""Enable the existing arena shader for 1v1 instanced rim lights; no reimport."""
import unreal as u

material = u.load_asset("/Game/TestArena/Arena/M_ArenaSurface_PremiumV2")
if not material:
    raise RuntimeError("The imported arena surface material is missing.")
if not material.get_editor_property("used_with_instanced_static_meshes"):
    material.set_editor_property("used_with_instanced_static_meshes", True)
    u.MaterialEditingLibrary.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Could not save arena instancing support.")
u.log("FLICK: Arena instancing support ready; existing material colours and parameters unchanged.")
