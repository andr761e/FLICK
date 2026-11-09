"""Refresh only Retro Arcade UI textures; preserve all gameplay asset bytes."""
import hashlib,json
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
mesh_files=list((root/'Content/Cosmetics/Pucks/RetroArcade').glob('*.uasset'))
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in mesh_files}
names=('Standard','Heavy','Striker','Grippy','Slider','Blocker','Compact','Bouncer','Toppler')
jobs=[]
for view in ['','Locker','Presentation','Hero','ProfileCutout']:
    for name in names:
        source=root/'AssetDevelopment/UI/PuckThumbnails'/view/f'{name}_6.png'
        assert source.is_file(),str(source)
        task=u.AssetImportTask();task.filename=str(source)
        task.destination_path='/Game/UI/PuckThumbnails'+('/'+view if view else '')
        task.destination_name=f'T_Puck_{name}_6';task.automated=True
        task.replace_existing=True;task.replace_existing_settings=True;task.save=False
        jobs.append(task)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(jobs)
for job in jobs:
    texture=u.load_asset(job.destination_path+'/'+job.destination_name)
    assert isinstance(texture,u.Texture2D),job.destination_name
    texture.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_BC7)
    texture.set_editor_property('power_of_two_mode',u.TexturePowerOfTwoSetting.STRETCH_TO_POWER_OF_TWO)
    texture.set_editor_property('mip_gen_settings',u.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE)
    texture.set_editor_property('max_texture_size',512 if job.destination_path=='/Game/UI/PuckThumbnails' else 1024)
    texture.set_editor_property('srgb',True);texture.set_editor_property('never_stream',False)
    u.EditorAssetLibrary.save_loaded_asset(texture,only_if_is_dirty=False)
assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==digest for p,digest in before.items()),'Portrait import changed a game mesh/material'
report=dict(textures=len(jobs),game_assets_unchanged=len(before))
(root/'Saved/RetroArcadePortraitImport.json').write_text(json.dumps(report,indent=2))
u.log('RETRO_ARCADE_PORTRAITS_IMPORTED '+str(report))
