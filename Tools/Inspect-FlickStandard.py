"""Inspect supplied puck geometry without modifying the source FBX."""
import bpy
import json
import sys
from mathutils import Vector
bpy.ops.wm.read_factory_settings(use_empty=True)
args = sys.argv[sys.argv.index('--') + 1:]
bpy.ops.import_scene.fbx(filepath=args[0])
objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
points = [o.matrix_world @ Vector(p) for o in objects for p in o.bound_box]
print('PUCK_INSPECT ' + json.dumps({
    'bounds': [[min(p[i] for p in points), max(p[i] for p in points)] for i in range(3)],
    'materials': sorted({m.name for o in objects for m in o.data.materials if m}),
    'objects': len(objects),
}))
if len(args) > 1:
    center = Vector([(min(p[i] for p in points) + max(p[i] for p in points)) * .5 for i in range(3)])
    for obj in objects:
        obj.location -= center
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.ops.export_scene.fbx(filepath=args[1], use_selection=True,
                             object_types={'MESH'}, add_leaf_bones=False,
                             bake_anim=False, mesh_smooth_type='FACE')
