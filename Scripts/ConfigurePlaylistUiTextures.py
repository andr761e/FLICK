import unreal

PLAYLIST_TEXTURES = (
    "/Game/UI/1v1PlaylistSymbol",
    "/Game/UI/2v2PlaylistSymbol",
    "/Game/UI/3v3PlaylistSymbol",
    "/Game/UI/BobPlaylistSymbol",
    "/Game/UI/CasualPlaylistSymbol",
    "/Game/UI/CompetitivePlaylistSymbol",
    "/Game/UI/TrainingPlaylistSymbol",
    "/Game/UI/PrivateMatchPlaylistSymbol",
    "/Game/UI/TutorialPlaylist",
    "/Game/UI/FreePlayPlaylist",
    "/Game/UI/1v1vsBotPlaylist",
    "/Game/UI/BOBvsBotPlaylist",
)


for asset_path in PLAYLIST_TEXTURES:
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(texture, unreal.Texture2D):
        raise RuntimeError(f"Missing playlist texture: {asset_path}")

    texture.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_EDITOR_ICON,
    )
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_TRILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_SHARPEN2)
    texture.set_editor_property("lod_bias", 0)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False)
