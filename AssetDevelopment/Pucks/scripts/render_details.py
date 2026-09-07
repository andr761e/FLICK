"""Render close views from the saved workshop without modifying the .blend."""
import bpy
from pathlib import Path
from mathutils import Vector

root=Path(__file__).resolve().parents[1]
scene=bpy.context.scene
scene.render.resolution_x=1400
scene.render.resolution_y=1000
scene.cycles.samples=64
cam=scene.camera
for name,team in [('Standard','Blue'),('Heavy','Orange')]:
    target=bpy.data.objects['SM_Puck_'+name+('_Orange' if team=='Orange' else '')]
    for obj in scene.objects:
        if obj.name.startswith('SM_Puck_') or obj.type=='FONT':
            obj.hide_render=obj is not target
    cam.location=target.location+Vector((.9,-1.6,1.25))
    cam.rotation_euler=(target.location-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.ortho_scale=target.dimensions.x*1.55
    scene.render.filepath=str(root/'renders'/f'{name.lower()}_{team.lower()}_detail.png')
    bpy.ops.render.render(write_still=True)
print('FLICK_DETAIL_RENDERS_COMPLETE')
