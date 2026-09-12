import unreal


SOURCE = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_content_dir() + "UI/FlickKnockoutWordmark.png"
)
DESTINATION = "/Game/UI"

task = unreal.AssetImportTask()
task.filename = SOURCE
task.destination_path = DESTINATION
task.destination_name = "FlickKnockoutWordmark"
task.automated = True
task.replace_existing = True
task.replace_existing_settings = True
task.save = True

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset("/Game/UI/FlickKnockoutWordmark")
if not texture:
    raise RuntimeError("Failed to import the FLICK: KNOCKOUT wordmark")

texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("never_stream", True)
texture.set_editor_property("filter", unreal.TextureFilter.TF_TRILINEAR)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("srgb", True)
texture.modify()
unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
unreal.log("FLICK_WORDMARK_IMPORT_COMPLETE")
