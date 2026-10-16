"""Create conservative venue candidates; production inputs remain untouched.

blender --background --python Tools/Optimize-FlickVenueGeometry.py -- [--only NAME]
The explicit scope excludes every puck and the BOB playing arena.
"""
import argparse
import hashlib
import json
import math
import shutil
from pathlib import Path
import sys
import bpy
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

sys.path.insert(0, str(Path(__file__).resolve().parent))
from FlickMeshCleanup import optimize_planar_mesh, mesh_stats, export_compact_fbx

PROJECT = Path(__file__).resolve().parents[1]
WORK = PROJECT / 'Saved/VenueOptimization'
parser = argparse.ArgumentParser()
parser.add_argument('--only')
parser.add_argument('--update-blends', action='store_true')
args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
audit = json.loads((WORK / 'audit.json').read_text())
report = []

if args.update_blends:
    reports = []
    for folder, blend in [('Arena','TestArenaWorkshop.blend'), ('Stadium','TestStadium.blend'), ('BOBStadium','BOBPocketFoundry.blend')]:
        source = PROJECT / 'AssetDevelopment' / folder / blend
        backup = WORK / 'originals' / source.relative_to(PROJECT)
        if backup.exists(): raise RuntimeError('Blender source already backed up/applied: ' + str(source))
        backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, backup)
        bpy.ops.wm.open_mainfile(filepath=str(source))
        counts = []
        for row in audit:
            if Path(row['source']).parts[1] != folder: continue
            name = Path(row['source']).stem
            obj = bpy.data.objects[name]
            before, after = optimize_planar_mesh(obj)
            counts.append(dict(asset=name, before=before, after=after))
        bpy.context.preferences.filepaths.save_version = 0
        bpy.ops.wm.save_as_mainfile(filepath=str(source), compress=True)
        reports.append(dict(source=source.relative_to(PROJECT).as_posix(), bytes_before=backup.stat().st_size,
                            bytes_after=source.stat().st_size, meshes=counts))
    (WORK / 'blends.json').write_text(json.dumps(reports, indent=2))
    raise SystemExit(0)

def surface_error(source, target):
    source.calc_loop_triangles()
    target.calc_loop_triangles()
    # Match surfaces per material, so coincident layers cannot mask a removed face.
    maximum = 0.0
    worst = None
    vertices = [tuple(v.co) for v in target.vertices]
    def sub(a,b): return tuple(a[i]-b[i] for i in range(3))
    def dot(a,b): return sum(a[i]*b[i] for i in range(3))
    def distance2(p, a, b, c):
        # Ericson closest-point test in double precision. Blender's float BVH
        # misses some extremely thin authored bevel triangles even in a self-test.
        ab, ac, ap = sub(b,a), sub(c,a), sub(p,a)
        d1,d2=dot(ab,ap),dot(ac,ap)
        if d1<=0 and d2<=0: return dot(ap,ap)
        bp=sub(p,b); d3,d4=dot(ab,bp),dot(ac,bp)
        if d3>=0 and d4<=d3: return dot(bp,bp)
        vc=d1*d4-d3*d2
        if vc<=0 and d1>=0 and d3<=0 and abs(d1-d3)>1e-30:
            t=d1/(d1-d3); v=sub(p,tuple(a[i]+t*ab[i] for i in range(3))); return dot(v,v)
        cp=sub(p,c); d5,d6=dot(ab,cp),dot(ac,cp)
        if d6>=0 and d5<=d6: return dot(cp,cp)
        vb=d5*d2-d1*d6
        if vb<=0 and d2>=0 and d6<=0 and abs(d2-d6)>1e-30:
            t=d2/(d2-d6); v=sub(p,tuple(a[i]+t*ac[i] for i in range(3))); return dot(v,v)
        va=d3*d6-d5*d4
        if va<=0 and d4-d3>=0 and d5-d6>=0 and abs((d4-d3)+(d5-d6))>1e-30:
            t=(d4-d3)/((d4-d3)+(d5-d6)); v=sub(p,tuple(b[i]+t*(c[i]-b[i]) for i in range(3))); return dot(v,v)
        denom=va+vb+vc
        if abs(denom)<1e-35:
            distances = []
            for start, end in ((a,b),(b,c),(c,a)):
                edge, delta = sub(end,start), sub(p,start)
                size2 = dot(edge,edge)
                t = max(0,min(1,dot(delta,edge)/size2)) if size2 else 0
                distance = sub(p,tuple(start[i]+t*edge[i] for i in range(3)))
                distances.append(dot(distance,distance))
            return min(distances)
        v,w=vb/denom,vc/denom
        delta=sub(p,tuple(a[i]+ab[i]*v+ac[i]*w for i in range(3)))
        return dot(delta,delta)
    for material in {p.material_index for p in source.polygons}:
        triangles = [tuple(t.vertices) for t in target.loop_triangles if t.material_index == material]
        grid = {}
        boxes = []
        for index, triangle in enumerate(triangles):
            points = [vertices[v] for v in triangle]
            low = tuple(min(p[i] for p in points) for i in range(3))
            high = tuple(max(p[i] for p in points) for i in range(3))
            boxes.append((low,high))
            for x in range(math.floor(low[0]-.00001),math.floor(high[0]+.00001)+1):
                for y in range(math.floor(low[1]-.00001),math.floor(high[1]+.00001)+1):
                    for z in range(math.floor(low[2]-.00001),math.floor(high[2]+.00001)+1):
                        grid.setdefault((x,y,z),[]).append(index)
        tree = BVHTree.FromPolygons([v.co for v in target.vertices],
            triangles, all_triangles=True)
        linked = {}
        for index, triangle in enumerate(triangles):
            for vertex in triangle: linked.setdefault(vertex, set()).add(index)
        kd = KDTree(len(linked))
        for vertex in linked: kd.insert(vertices[vertex], vertex)
        kd.balance()
        vertex_ids = {}
        for v in linked: vertex_ids.setdefault(vertices[v], []).append(v)
        exact_vertices = set(vertex_ids)
        for triangle in source.loop_triangles:
            if triangle.material_index != material:
                continue
            a, b, c = [source.vertices[i].co for i in triangle.vertices]
            corner_candidates = set()
            for corner in (a,b,c):
                for vertex in vertex_ids.get(tuple(corner), []): corner_candidates.update(linked[vertex])
            for point in (a, b, c, (a+b+c)/3, (a+b)/2, (b+c)/2, (c+a)/2):
                if tuple(point) in exact_vertices:
                    continue
                result = tree.find_nearest(point)
                if result is None or result[3] is None:
                    raise RuntimeError('Material surface disappeared')
                distance = result[3]
                if distance > .000002:
                    candidates = corner_candidates | {item[2] for item in tree.find_nearest_range(point, max(.005, distance*2))}
                    candidates.update(i for i in grid.get(tuple(math.floor(v) for v in point), [])
                                      if all(boxes[i][0][axis]-.000005 <= point[axis] <= boxes[i][1][axis]+.000005 for axis in range(3)))
                    for _, vertex, _ in kd.find_n(point, min(16, len(linked))): candidates.update(linked[vertex])
                    distance = min(distance, min((distance2(tuple(point), *(vertices[v] for v in triangles[i]))**.5
                                                  for i in candidates), default=distance))
                if distance > maximum:
                    maximum = distance
                    worst = dict(material=material, point=list(point), area=triangle.area,
                                 triangle=[list(a),list(b),list(c)])
    if maximum > .000005: print('VENUE_DISTANCE_DIAGNOSTIC', maximum, worst, flush=True)
    return maximum

for entry in audit:
    source = PROJECT / entry['source']
    if args.only and source.stem != args.only:
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(source), use_custom_normals=True)
    objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if len(objects) != 1:
        raise RuntimeError('Expected one combined mesh: ' + str(source))
    obj = objects[0]
    original = obj.data.copy()
    dimensions = list(obj.dimensions)
    materials = [s.name for s in obj.material_slots]
    before, after = optimize_planar_mesh(obj)
    print('VENUE_CANDIDATE_COUNTS', source.stem, before, after, flush=True)
    if list(obj.dimensions) != dimensions or materials != [s.name for s in obj.material_slots]:
        raise RuntimeError('Dimensions or materials changed: ' + source.stem)
    error = max(surface_error(original, obj.data), surface_error(obj.data, original))
    # Floating-point roundoff only (0.005 mm). No approximation of curved surfaces.
    if error > .000005:
        print('VENUE_SELF_ERROR', source.stem, surface_error(original, original), flush=True)
        raise RuntimeError(f'{source.stem}: surface error {error} m exceeds tolerance')
    output = WORK / 'candidates' / source.relative_to(PROJECT)
    output.parent.mkdir(parents=True, exist_ok=True)
    export_compact_fbx(filepath=str(output), use_selection=True,
        object_types={'MESH'}, add_leaf_bones=False, axis_forward='-Y', axis_up='Z',
        mesh_smooth_type='FACE', use_tspace=False, bake_anim=False)
    bpy.ops.export_scene.gltf(filepath=str(output.with_suffix('.glb')), use_selection=True, export_format='GLB')
    result = dict(source=entry['source'], package=entry['package'], before=before, after=after,
                  bytes_before=source.stat().st_size, bytes_after=output.stat().st_size,
                  original_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  surface_error_metres=error,
                  candidate_sha256=hashlib.sha256(output.read_bytes()).hexdigest())
    report.append(result)
    print('VENUE_OPTIMIZED', json.dumps(result), flush=True)
    (WORK / ('candidate-'+args.only+'.json' if args.only else 'candidates.json')).write_text(json.dumps(report, indent=2))
