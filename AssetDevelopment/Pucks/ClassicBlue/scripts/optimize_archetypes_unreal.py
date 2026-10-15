"""Conservative, material/UV-boundary-preserving optimization of current FBXs.

Blender --background --python <this file> -- [--only Compact]
Then --review to inspect exported candidates; --apply accepts a full dry run.
Standard is never modified. Backups and reports go to Saved.
"""
import argparse
import json
import math
import shutil
import sys
import hashlib
from pathlib import Path

import bpy
import bmesh
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT.parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--apply', action='store_true')
parser.add_argument('--only')
parser.add_argument('--review', action='store_true', help='Render original/candidate contact sheets only')
args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
review = PROJECT / 'Saved/PuckOptimization'
review.mkdir(parents=True, exist_ok=True)
rows = json.loads((ROOT / 'manifests/archetype_exports.json').read_text())
if args.only:
    rows = [row for row in rows if row['asset'] == args.only]
report = []

if args.apply:
    pending = json.loads((review / 'dry_run.json').read_text())
    if len(pending) != 8 or args.only:
        raise RuntimeError('Apply requires a complete eight-puck dry run')
    for row in pending:
        source = ROOT / 'exports' / (row['asset'] + '.fbx')
        candidate = review / 'candidates' / source.name
        if hashlib.sha256(source.read_bytes()).hexdigest() != row['original_sha256']:
            raise RuntimeError(f'{source.name}: original changed since dry run')
        if candidate.stat().st_size != row['bytes_after']:
            raise RuntimeError(f'{source.name}: candidate changed since dry run')
    backups = review / 'originals'
    backups.mkdir(exist_ok=True)
    for row in pending:
        source = ROOT / 'exports' / (row['asset'] + '.fbx')
        backup = backups / source.name
        if not backup.exists():
            shutil.copy2(source, backup)
        shutil.copy2(review / 'candidates' / source.name, source)
    (review / 'applied.json').write_text(json.dumps(pending, indent=2))
    print('PUCK_OPTIMIZATION_APPLIED', len(pending), flush=True)
    raise SystemExit(0)

if args.review:
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    palette = [
        ('Graphite', (.025, .04, .055), .7, .3),
        ('Machined', (.5, .6, .65), .85, .23),
        ('Black', (.004, .008, .012), .2, .45),
        ('Cyan light', (.005, .65, 1), .1, .25),
        ('Circular', (.55, .65, .7), .9, .28),
        ('Recessed', (.015, .025, .035), .7, .3),
        ('Cyan center', (.005, .65, 1), .1, .25)]
    for index, row in enumerate(rows):
        for side, folder in enumerate((ROOT / 'exports', review / 'candidates')):
            bpy.ops.import_scene.fbx(filepath=str(folder / (row['asset'] + '.fbx')), use_custom_normals=True)
            obj = next(o for o in bpy.context.selected_objects if o.type == 'MESH')
            # Same dimensions, materials, lighting and camera for each pair.
            obj.location = ((index % 2) * 2.5 + side * 1.15 - 1.8, (index // 2) * 1.3 - 1.95, 0)
            for slot in obj.material_slots:
                mat = slot.material
                mat.use_nodes = True
                shader = mat.node_tree.nodes.get('Principled BSDF')
                for token, color, metallic, roughness in palette:
                    if token in mat.name:
                        shader.inputs['Base Color'].default_value = (*color, 1)
                        shader.inputs['Metallic'].default_value = metallic
                        shader.inputs['Roughness'].default_value = roughness
                        shader.inputs['Emission Color'].default_value = (*color, 1)
                        shader.inputs['Emission Strength'].default_value = 2 if 'Cyan' in token else 0
                        break
            text = bpy.data.curves.new('Label', 'FONT')
            text.body = row['asset'] + (' / original' if side == 0 else ' / optimized')
            text.size = .08
            label = bpy.data.objects.new(text.body, text)
            bpy.context.scene.collection.objects.link(label)
            label.location = obj.location + Vector((-.43, -.57, .01))
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE'
    scene.eevee.taa_render_samples = 32
    scene.render.resolution_x, scene.render.resolution_y = 2400, 2000
    scene.render.resolution_percentage = 100
    scene.world.color = (.22, .22, .22)
    for location, power, size in [((-3, -2, 5), 1800, 4), ((4, 3, 5), 2200, 3), ((0, 5, 3), 1000, 3)]:
        light = bpy.data.lights.new('Review softbox', 'AREA')
        light.energy, light.shape, light.size = power, 'DISK', size
        actor = bpy.data.objects.new(light.name, light)
        scene.collection.objects.link(actor)
        actor.location = location
        actor.rotation_euler = (-actor.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.ops.object.camera_add(location=(0, -5.5, 8))
    camera = bpy.context.object
    camera.rotation_euler = (-camera.location).to_track_quat('-Z', 'Y').to_euler()
    camera.data.type, camera.data.ortho_scale = 'ORTHO', 6.8
    scene.camera = camera
    for label, position in [('top', (0, -5.5, 8)), ('low', (0, -8, 4)), ('underside', (0, -5.5, -8))]:
        camera.location = position
        camera.rotation_euler = (-camera.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(review / f'comparison_{label}.png')
        bpy.ops.render.render(write_still=True)
    raise SystemExit(0)

def stats(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    return {'vertices': len(mesh.vertices), 'polygons': len(mesh.polygons),
            'triangles': len(mesh.loop_triangles), 'materials': [s.name for s in obj.material_slots],
            'dimensions': list(obj.dimensions)}

def surface_error(source, target):
    """Check vertices and triangle interiors in both directions, not bounds alone."""
    source.calc_loop_triangles()
    target.calc_loop_triangles()
    tree = BVHTree.FromPolygons([v.co for v in target.vertices],
        [tuple(t.vertices) for t in target.loop_triangles], all_triangles=True)
    maximum = 0.0
    for vertex in source.vertices:
        maximum = max(maximum, tree.find_nearest(vertex.co)[3])
    for triangle in source.loop_triangles:
        a, b, c = [source.vertices[i].co for i in triangle.vertices]
        for point in ((a+b+c)/3, (a+b)/2, (b+c)/2, (c+a)/2):
            maximum = max(maximum, tree.find_nearest(point)[3])
    return maximum

for name in ['Standard'] + [row['asset'] for row in rows]:
    if args.only and name not in ('Standard', args.only):
        continue
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)
    path = ROOT / 'exports' / (name + '.fbx')
    bpy.ops.import_scene.fbx(filepath=str(path), use_custom_normals=True)
    objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if len(objects) != 1:
        raise RuntimeError(f'{name}: expected one combined mesh, got {len(objects)}')
    obj = objects[0]
    bpy.context.view_layer.objects.active = obj
    before = stats(obj)
    if name == 'Standard':
        print('PUCK_REFERENCE', json.dumps(before), flush=True)
        continue
    original = obj.copy()
    original.data = obj.data.copy()
    bpy.context.scene.collection.objects.link(original)
    original.hide_render = True
    original.select_set(False)
    # Match authored corner normals by position/material and face direction.
    # Nearest-face transfer is unsafe on this layered, overlapping artwork:
    # it can pick an underside instead of the visible top surface.
    def position_key(position):
        return tuple(round(value, 6) for value in position)
    normal_lookup = {}
    authored_normals = original.data.corner_normals
    for face in original.data.polygons:
        for loop_index in face.loop_indices:
            vertex = original.data.vertices[original.data.loops[loop_index].vertex_index]
            key = (position_key(vertex.co), face.material_index)
            normal_lookup.setdefault(key, []).append((face.normal.copy(), authored_normals[loop_index].vector.copy()))
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-7)
    bm.to_mesh(obj.data)
    bm.free()
    print('PUCK_WELDED', name, json.dumps(stats(obj)), flush=True)
    modifier = obj.modifiers.new('Planar surface cleanup', 'DECIMATE')
    modifier.decimate_type = 'DISSOLVE'
    modifier.angle_limit = math.radians(.1)
    modifier.delimit = {'MATERIAL', 'UV'}
    modifier.use_dissolve_boundaries = False
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    preserved_normals = [None] * len(obj.data.loops)
    for face in obj.data.polygons:
        for loop_index in face.loop_indices:
            vertex = obj.data.vertices[obj.data.loops[loop_index].vertex_index]
            choices = normal_lookup[(position_key(vertex.co), face.material_index)]
            preserved_normals[loop_index] = max(choices, key=lambda item: item[0].dot(face.normal))[1]
        face.use_smooth = True
    obj.data.normals_split_custom_set(preserved_normals)
    after = stats(obj)
    if before['materials'] != after['materials']:
        raise RuntimeError(f'{name}: material slots changed')
    if max(abs(a-b) for a,b in zip(before['dimensions'], after['dimensions'])) > 1e-6:
        raise RuntimeError(f'{name}: bounds changed')
    if after['triangles'] >= before['triangles']:
        raise RuntimeError(f'{name}: cleanup did not reduce actual triangles')
    error = max(surface_error(obj.data, original.data), surface_error(original.data, obj.data))
    # Less than 0.2 mm on a ~900 mm puck: sub-pixel even in close-up reviews.
    if error > .0002:
        raise RuntimeError(f'{name}: surface deviation {error} metres exceeds 0.2 mm')
    result = {'asset': name, 'before': before, 'after': after, 'surface_error_metres': error,
        'bytes_before': path.stat().st_size, 'original_sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    output = review / 'candidates' / path.name
    output.parent.mkdir(exist_ok=True)
    bpy.ops.export_scene.fbx(filepath=str(output), use_selection=True,
        object_types={'MESH'}, add_leaf_bones=False, axis_forward='-Y', axis_up='Z',
        mesh_smooth_type='FACE', use_tspace=False)
    result['bytes_after'] = output.stat().st_size
    report.append(result)
    print('PUCK_OPTIMIZATION', json.dumps(result), flush=True)
(review / 'dry_run.json').write_text(json.dumps(report, indent=2))
