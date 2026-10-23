"""Import all 54 model/finish thumbnails without touching meshes or gameplay materials."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
presentation = "-FlickPresentationThumbnails" in unreal.SystemLibrary.get_command_line()
locker = "-FlickLockerThumbnails" in unreal.SystemLibrary.get_command_line()
hero = "-FlickHeroThumbnails" in unreal.SystemLibrary.get_command_line()
profile_cutout = "-FlickProfileCutoutThumbnails" in unreal.SystemLibrary.get_command_line()
if sum((presentation, locker, hero, profile_cutout)) > 1:
    raise RuntimeError("Choose only one thumbnail view")
source_dir = root / "AssetDevelopment/UI/PuckThumbnails"
destination = "/Game/UI/PuckThumbnails"
if presentation:
    source_dir = source_dir / "Presentation"
    destination += "/Presentation"
elif locker:
    source_dir = source_dir / "Locker"
    destination += "/Locker"
elif hero:
    source_dir = source_dir / "Hero"
    destination += "/Hero"
elif profile_cutout:
    source_dir = source_dir / "ProfileCutout"
    destination += "/ProfileCutout"
names = ("Standard", "Heavy", "Striker", "Grippy", "Slider", "Blocker", "Compact", "Bouncer", "Toppler")
tasks = []
for name in names:
    for skin in range(6):
        source = source_dir / (name + "_" + str(skin) + ".png")
        if not source.is_file():
            raise RuntimeError("Missing model render: " + str(source))
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = destination
        task.destination_name = "T_Puck_" + name + "_" + str(skin)
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = True
        task.save = False
        tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    texture = unreal.load_asset(task.destination_path + "/" + task.destination_name)
    if not texture:
        raise RuntimeError("Failed to import " + task.destination_name)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("max_texture_size", 1024 if presentation or locker or hero or profile_cutout else 512)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", False)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log("FLICK_PUCK_THUMBNAILS_IMPORTED 54")
