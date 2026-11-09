"""Render only UI portraits from saved source scenes. Never exports geometry."""
import argparse,hashlib,json,sys
from pathlib import Path
import bpy
sys.path.insert(0,str(Path(__file__).resolve().parent))
from FlickPuckPortraitViews import apply_view,VIEWS

NAMES=('Standard','Heavy','Striker','Grippy','Slider','Blocker','Compact','Bouncer','Toppler')

def render_portraits(scene,puck,project,name):
    camera=scene.camera
    floor=bpy.data.objects['Studio floor, not exported']
    floor_shader=floor.data.materials[0].node_tree.nodes.get('Principled BSDF')
    floor_shader.inputs['Base Color'].default_value=(.012,.025,.04,1)
    floor_shader.inputs['Metallic'].default_value=.72
    floor_shader.inputs['Roughness'].default_value=.32
    # The classic portrait renderer grounds every model before aiming the camera.
    # Match that coordinate frame, not the centered gameplay mesh's origin.
    bottom=min((puck.matrix_world@v.co).z for v in puck.data.vertices)
    puck.location.z-=bottom;floor.location.z=-.001
    scene.render.image_settings.file_format='PNG';scene.render.image_settings.color_mode='RGBA'
    report=[]
    for view in VIEWS:
        spec=apply_view(scene,camera,view)
        floor.hide_render=spec['transparent']
        scene.cycles.samples=128 if view in ['Hero','ProfileCutout'] else 96 if view!='Original' else 48
        output=project/'AssetDevelopment/UI/PuckThumbnails'
        if view!='Original':output/=view
        output.mkdir(parents=True,exist_ok=True)
        scene.render.filepath=str(output/f'{name}_6.png')
        bpy.ops.render.render(write_still=True)
        report.append(dict(archetype=name,skin=6,view=view,**spec))
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--only',choices=NAMES)
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    project=Path(__file__).resolve().parent.parent
    source=project/'AssetDevelopment/Pucks/RetroArcade'
    report=[]
    for name in NAMES:
        if args.only and name!=args.only:continue
        fbx=source/f'exports/{name}.fbx';before=hashlib.sha256(fbx.read_bytes()).hexdigest()
        bpy.ops.wm.open_mainfile(filepath=str(source/f'RetroArcade_{name}.blend'))
        report.extend(render_portraits(bpy.context.scene,bpy.data.objects[f'SM_Puck_{name}_RetroArcade'],project,name))
        assert hashlib.sha256(fbx.read_bytes()).hexdigest()==before,'Portrait render modified geometry'
        print('RETRO_ARCADE_PORTRAITS_MATCHED',name,flush=True)
    (source/'manifests/portrait_views.json').write_text(json.dumps(report,indent=2))
