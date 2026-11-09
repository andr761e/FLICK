"""Discard zero-area bevel triangles, preserving art normals and corner data."""
import bpy

def clean_export_mesh(puck):
    old=puck.data;old.calc_loop_triangles()
    triangles=[]
    for triangle in old.loop_triangles:
        a,b,c=[old.vertices[i].co for i in triangle.vertices]
        if (b-a).cross(c-a).length_squared>1e-24:
            triangles.append(triangle)
    normals=[item.vector.copy() for item in old.corner_normals]
    new=bpy.data.meshes.new(old.name+'_ExportClean')
    new.from_pydata([v.co for v in old.vertices],[],[tuple(t.vertices) for t in triangles])
    for material in old.materials:new.materials.append(material)
    for poly,triangle in zip(new.polygons,triangles):
        poly.material_index=triangle.material_index
        poly.use_smooth=old.polygons[triangle.polygon_index].use_smooth
    old_loops=[loop for triangle in triangles for loop in triangle.loops]
    for source in old.uv_layers:
        uv=new.uv_layers.new(name=source.name)
        for dest,index in zip(uv.data,old_loops):dest.uv=source.data[index].uv
    for source in old.color_attributes:
        if source.domain!='CORNER':continue
        color=new.color_attributes.new(name=source.name,type=source.data_type,domain='CORNER')
        for dest,index in zip(color.data,old_loops):dest.color=source.data[index].color
    new.update()
    new.normals_split_custom_set([normals[i] for i in old_loops])
    puck.data=new
    bpy.data.meshes.remove(old)
    return len(old_loops)//3
