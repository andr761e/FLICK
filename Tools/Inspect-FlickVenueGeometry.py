"""Read-only Blender audit of the explicitly scoped venue meshes."""
import json
from pathlib import Path
import bpy

PROJECT = Path(__file__).resolve().parents[1]
OUT = PROJECT / 'Saved/VenueOptimization'
OUT.mkdir(parents=True, exist_ok=True)
groups = {
    'Arena': ('TestArenaWorkshop.blend', '/Game/TestArena/Arena'),
    'Stadium': ('TestStadium.blend', '/Game/TestArena/Stadium'),
    'BOBStadium': ('BOBPocketFoundry.blend', '/Game/BOB/Stadium'),
}
def stats(obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    normals = mesh.corner_normals
    constant = sum(all((normals[i].vector - normals[p.loop_start].vector).length < 1e-6
                       for i in p.loop_indices) for p in mesh.polygons)
    return dict(vertices=len(mesh.vertices), edges=len(mesh.edges), faces=len(mesh.polygons),
                triangles=len(mesh.loop_triangles), constant_normal_faces=constant,
                uv_layers=len(mesh.uv_layers), materials=[s.name for s in obj.material_slots],
                dimensions=list(obj.dimensions), modifiers=[m.type for m in obj.modifiers])
report = []
for folder, (blend, destination) in groups.items():
    for source in sorted((PROJECT / 'AssetDevelopment' / folder / 'exports').glob('*.fbx')):
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=str(source), use_custom_normals=True)
        objects = [o for o in bpy.context.scene.objects if o.type == 'MESH']
        row = dict(source=source.relative_to(PROJECT).as_posix(), bytes=source.stat().st_size,
                   package=destination + '/' + source.stem, meshes={o.name:stats(o) for o in objects})
        report.append(row)
        print('VENUE_AUDIT', json.dumps(row), flush=True)
    bpy.ops.wm.open_mainfile(filepath=str(PROJECT / 'AssetDevelopment' / folder / blend))
    print('VENUE_BLEND', folder, json.dumps({o.name:stats(o) for o in bpy.context.scene.objects
          if o.type == 'MESH' and o.name.startswith('SM_')}), flush=True)
(OUT / 'audit.json').write_text(json.dumps(report, indent=2))
