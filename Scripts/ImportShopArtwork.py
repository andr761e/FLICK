"""Import fictional storefront product renders; does not touch real cosmetics."""
from pathlib import Path
import unreal

NAMES = ("IonShift", "SolarWake", "ZeroPoint", "Supernova", "Nightfall", "PrismHalo")
root = Path(unreal.Paths.project_dir()) / "AssetDevelopment/UI/Shop/exports"
tasks = []
for name in NAMES:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(root / f"{name}.png"))
    task.set_editor_property("destination_path", "/Game/UI/Shop")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in NAMES:
    texture = unreal.EditorAssetLibrary.load_asset(f"/Game/UI/Shop/{name}")
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Shop artwork import failed: {name}")
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SHARPEN2)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_TRILINEAR)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log("FLICK_SHOP_ARTWORK_IMPORTED: six product renders")
