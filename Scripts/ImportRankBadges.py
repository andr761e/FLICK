"""Import the approved transparent rank atlas. Run with Unreal Editor Python."""
from pathlib import Path
import unreal

source = Path(unreal.Paths.project_dir()) / "AssetDevelopment/UI/Ranks/exports/RankBadges.png"
task = unreal.AssetImportTask()
task.set_editor_property("filename", str(source))
task.set_editor_property("destination_path", "/Game/UI/Ranks")
task.set_editor_property("destination_name", "RankBadges")
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", False)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.EditorAssetLibrary.load_asset("/Game/UI/Ranks/RankBadges")
if not isinstance(texture, unreal.Texture2D):
    raise RuntimeError("Rank badge atlas import failed")
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
texture.set_editor_property("srgb", True)
texture.set_editor_property("never_stream", True)
texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log("FLICK_RANK_BADGES_IMPORTED")
