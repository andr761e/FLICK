"""Blender-only planar cleanup, preserving material/UV boundaries and normals.

Candidates must also pass independent surface, bounds and render checks before
production use. Never ratio-decimate silhouettes or change mesh coordinates.
"""
import math
import bpy
import bmesh

def mesh_stats(mesh):
    mesh.calc_loop_triangles()
    return dict(vertices=len(mesh.vertices), edges=len(mesh.edges),
                faces=len(mesh.polygons), triangles=len(mesh.loop_triangles))

def optimize_planar_mesh(obj):
    if obj.type != 'MESH' or obj.modifiers:
        raise ValueError('Cleanup requires an evaluated mesh without modifiers: ' + obj.name)
    mesh = obj.data
    before = mesh_stats(mesh)
    normals = {}
    corner_normals = mesh.corner_normals
    for face in mesh.polygons:
        for i in face.loop_indices:
            key = (tuple(mesh.vertices[mesh.loops[i].vertex_index].co), face.material_index)
            normals.setdefault(key, []).append((face.normal.copy(), corner_normals[i].vector.copy()))
    bm = bmesh.new()
    bm.from_mesh(mesh)
    # Coincident vertices encode the same position. Corner UVs and normals stay
    # independent, so welding does not erase a hard edge or material seam.
    if len({tuple(v.co) for v in mesh.vertices}) < len(mesh.vertices):
        bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-9)
        bm.normal_update()
    def sub(a,b): return tuple(a[i]-b[i] for i in range(3))
    def dot(a,b): return math.fsum(a[i]*b[i] for i in range(3))
    def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
    planes = {}
    for face in bm.faces:
        points = [tuple(v.co) for v in face.verts]
        origin = points[0]
        normal = max((cross(sub(points[i],origin), sub(points[i+1],origin)) for i in range(1,len(points)-1)),
                     key=lambda n: dot(n,n), default=(0,0,0))
        length = math.sqrt(dot(normal,normal))
        if length > 1e-16:
            normal = tuple(v/length for v in normal)
            if all(abs(dot(normal,sub(p,origin))) < 1e-8 for p in points):
                planes[face] = (origin,normal,points)
    edges = []
    used_faces = set()
    for edge in bm.edges:
        if len(edge.link_faces) != 2: continue
        a,b = edge.link_faces
        if a not in planes or b not in planes: continue
        if a in used_faces or b in used_faces or edge.seam or not edge.smooth: continue
        if a.material_index != b.material_index or a.smooth != b.smooth: continue
        origin,normal,_ = planes[a]
        if dot(normal,planes[b][1]) > .999999999999 and all(
                abs(dot(normal,sub(p,origin))) < 1e-8 for p in planes[b][2]):
            layers = list(bm.loops.layers.uv.values())
            if any(any((next(l for l in a.loops if l.vert == v)[layer].uv
                        - next(l for l in b.loops if l.vert == v)[layer].uv).length > 1e-7
                       for v in edge.verts) for layer in layers): continue
            edges.append(edge)
            used_faces.update((a,b))
    # Disjoint pairs cannot close an annular region into an invalid hole-spanning
    # ngon. Keep all boundary vertices, including every circular/bevel segment.
    bmesh.ops.dissolve_edges(bm, edges=edges, use_verts=False, use_face_split=False)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    restored = []
    current = mesh.corner_normals
    normal_error = 0.0
    for face in mesh.polygons:
        for i in face.loop_indices:
            key = (tuple(mesh.vertices[mesh.loops[i].vertex_index].co), face.material_index)
            choices = normals[key]
            normal = max(choices, key=lambda value: value[0].dot(face.normal))[1]
            error = (normal-current[i].vector).length
            normal_error = max(normal_error, error)
            restored.append(normal if error > 1e-4 else current[i].vector.copy())
    print('VENUE_NORMAL_ERROR', obj.name, normal_error, flush=True)
    # Blender stores custom normals as quantized angular pairs. Re-encoding
    # differences below 0.006 degrees only adds noise and bloats FBX arrays.
    if normal_error > 1e-4:
        mesh.normals_split_custom_set(restored)
    mesh.update()
    return before, mesh_stats(mesh)

def export_compact_fbx(**options):
    """Standard FBX arrays, with lossless zlib level 9 instead of Blender's 1.

    Restoring the compressor immediately keeps this scoped to this one export.
    Geometry, precision and Unreal import settings are unchanged.
    """
    import io_scene_fbx.encode_bin as writer
    compress = writer.zlib.compress
    try:
        writer.zlib.compress = lambda data, level=-1: compress(data, 9)
        bpy.ops.export_scene.fbx(**options)
    finally:
        writer.zlib.compress = compress
