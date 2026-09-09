"""Prepare independent FBX exports with valid per-face planar UVs for tangents.
Run on PuckWorkshop.blend; the authoring scene is not saved or modified on disk.
These UVs support shading, not an atlas for hand-painted textures.
"""
import bpy, json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
out=root/'exports'/'Unreal'; out.mkdir(exist_ok=True)
for entry in json.loads((root/'dimensions.json').read_text()):
    obj=bpy.data.objects['SM_Puck_'+entry['name']]
    obj.data=obj.data.copy()
    obj.location=(0,0,0)
    mesh=obj.data
    uv=mesh.uv_layers.active or mesh.uv_layers.new(name='SurfaceUV')
    for polygon in mesh.polygons:
        dominant=max(range(3),key=lambda i:abs(polygon.normal[i]))
        axes=[i for i in range(3) if i!=dominant]
        for index in polygon.loop_indices:
            position=mesh.vertices[mesh.loops[index].vertex_index].co
            uv.data[index].uv=(position[axes[0]],position[axes[1]])
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    triangulate=obj.modifiers.new(name='Unreal triangulation',type='TRIANGULATE')
    triangulate.keep_custom_normals=True
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    bpy.ops.export_scene.fbx(filepath=str(out/(entry['name']+'.fbx')),use_selection=True,
        object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',
        mesh_smooth_type='FACE',use_tspace=True)
print('FLICK_UNREAL_EXPORT_COMPLETE')
