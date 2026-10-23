"""Import the text-free menu illustrations; safe to rerun after replacing a master PNG."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
names = ("PlayCasual", "PlayCompetitive", "PlayTraining", "PlayPrivate", "PracticeSwitch", "PracticeBob",
         "PlayCasualV2", "PlayCompetitiveV2", "PlayTrainingV2", "PlayPrivateV2", "TrainingTutorialV2",
         "TrainingFreePlayV2", "TrainingBotV2", "TrainingBobV2", "FormatDoublesV2", "FormatTriplesV2", "PlayBackdropV2", "PracticeSwitchV2",
         "SettingsBackdropV2", "SettingsImpactV2", "ProfileBackdropV2", "ProfileRankStageV2", "ProfileCustomizeV2",
         "PracticePrecisionV3", "PracticeKnockoutsV3", "PracticeSwitchV3", "PracticeBobV3",
         "TrainingPracticeWideV3", "ProfileIdentityBannerV3", "ProfileShowcaseFloorV4", "ProfilePlacementV4", "ProfileFinishTrioV4")
if unreal.SystemLibrary.parse_param(unreal.SystemLibrary.get_command_line(), "FlickSettingsArtwork"):
    names = ("SettingsBackdropV2", "SettingsImpactV2")
elif unreal.SystemLibrary.parse_param(unreal.SystemLibrary.get_command_line(), "FlickProfileArtwork"):
    names = ("ProfileBackdropV2", "ProfileRankStageV2", "ProfileCustomizeV2")
elif unreal.SystemLibrary.parse_param(unreal.SystemLibrary.get_command_line(), "FlickPracticeArtwork"):
    names = ("PracticePrecisionV3", "PracticeKnockoutsV3", "PracticeSwitchV3", "PracticeBobV3")
elif unreal.SystemLibrary.parse_param(unreal.SystemLibrary.get_command_line(), "FlickPremiumArtwork"):
    names = ("TrainingPracticeWideV3", "ProfileIdentityBannerV3")
elif unreal.SystemLibrary.parse_param(unreal.SystemLibrary.get_command_line(), "FlickProfileV4Artwork"):
    names = ("ProfileShowcaseFloorV4", "ProfilePlacementV4", "ProfileFinishTrioV4")
tasks = []
for name in names:
    source = root / "AssetDevelopment" / "UI" / "MenuArtwork" / (name + ".png")
    if not source.is_file():
        raise RuntimeError("Missing menu illustration: " + str(source))
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = "/Game/UI/MenuArtwork"
    task.destination_name = "T_Menu_" + name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = False
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in names:
    texture = unreal.load_asset("/Game/UI/MenuArtwork/T_Menu_" + name)
    if not texture:
        raise RuntimeError("Failed to import " + name)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property("power_of_two_mode", unreal.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property("max_texture_size", 2048 if name in ("PlayBackdropV2", "SettingsBackdropV2", "ProfileBackdropV2", "TrainingPracticeWideV3") else 1024)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("never_stream", False)
    texture.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log("FLICK_MENU_ARTWORK_IMPORT_COMPLETE")
