"""Build the visual-only 1v1/2v2/3v3 arena; the Unreal cylinder owns collision."""
import bpy
import json
import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parents[1]
GAME_MODE = (PROJECT / "Source/FLICK/Game/FlickGameMode.h").read_text()
MATCH = re.search(r"float\s+ArenaRadius\s*=\s*([\d.]+)f", GAME_MODE)
RADIUS = (float(MATCH.group(1)) if MATCH else 650.0) / 100.0
SEGMENTS = 192

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.length_unit = "METERS"


def material(name, color, metallic, roughness, emission=0.0):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1.0)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Emission Color"].default_value = (*color, 1.0)
    bsdf.inputs["Emission Strength"].default_value = emission
    return mat


graphite = material("01_Deck_Graphite", (0.310, 0.320, 0.330), .38, .46)
field = material("02_Field_Slate", (0.400, 0.410, 0.420), .22, .53)
rail = material("03_Gunmetal_Rail", (0.13, 0.14, 0.15), .72, .31)
titanium = material("04_Satin_Titanium", (0.48, 0.49, 0.50), .84, .24)
recess = material("05_Deep_Recess", (0.018, 0.022, 0.027), .26, .59)
mark = material("06_Floor_Marking", (0.47, 0.49, 0.51), .25, .57)
warm = material("07_Warm_Light", (1.0, .55, .23), .03, .20, 2.2)
muted_warm = material("08_Warm_Metal", (.42, .31, .23), .72, .30)

parts = []


def finish(obj, name, mat, bevel=0.0):
    obj.name = name
    obj.data.materials.append(mat)
    if bevel:
        modifier = obj.modifiers.new("Machined edge", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    modifier = obj.modifiers.new("Weighted normals", "WEIGHTED_NORMAL")
    modifier.keep_sharp = True
    parts.append(obj)
    return obj


def cylinder(name, radius, bottom, top, mat, bevel=0.004):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=SEGMENTS, radius=radius, depth=top-bottom,
        location=(0, 0, (top+bottom)/2))
    return finish(bpy.context.object, name, mat, min(bevel, (top-bottom)*.2))


def sector(name, inner, outer, bottom, top, start, end, mat, steps=20):
    vertices = []
    for z in (bottom, top):
        for radius in (inner, outer):
            for index in range(steps+1):
                angle = start + (end-start)*index/steps
                vertices.append((radius*math.cos(angle), radius*math.sin(angle), z))
    n = steps+1
    faces = []
    for index in range(steps):
        faces.extend((
            (index, index+1, n+index+1, n+index),
            (2*n+index, 3*n+index, 3*n+index+1, 2*n+index+1),
            (index, 2*n+index, 2*n+index+1, index+1),
            (n+index, n+index+1, 3*n+index+1, 3*n+index)))
    faces.extend(((0, n, 3*n, 2*n), (steps, 2*n+steps, 3*n+steps, n+steps)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    return finish(obj, name, mat)


def ring(name, inner, outer, bottom, top, mat):
    return sector(name, inner, outer, bottom, top, 0, math.tau, mat, SEGMENTS)


def line(name, angle, inner, outer, width, z, mat):
    return sector(name, inner, outer, z-.001, z, angle-width/2/outer,
                  angle+width/2/outer, mat, 2)


# The entire upper playing surface stays within 7 mm of the existing Z=0
# collision plane. The 650 cm exterior radius is unchanged and never collides.
cylinder("Monolithic lower chassis", RADIUS, -.50, -.10, recess, .018)
cylinder("Graphite deck", RADIUS*.985, -.11, .002, graphite, .010)
cylinder("Dark tactical field", RADIUS*.900, .002, .003, field, .0002)
ring("Outer shadow channel", RADIUS*.958, RADIUS*.986, -.025, -.012, recess)
ring("Machined outer collar", RADIUS*.968, RADIUS, -.045, -.014, rail)
ring("Satin inner edge", RADIUS*.950, RADIUS*.958, -.011, -.006, titanium)
ring("Warm undercut diffuser", RADIUS*.980, RADIUS*.990, -.078, -.058, warm)
ring("Warm inset perimeter", RADIUS*.973, RADIUS*.977, .0023, .0042, warm)
ring("Lower amber reflection", RADIUS*.955, RADIUS*.960, -.087, -.070, muted_warm)

# Measured seams and restrained tournament markings; the center ring is the
# visual anchor, with no false pocket or raised obstacle on the playing field.
for index in range(20):
    angle = math.tau*index/20
    line("Precision radial seam", angle, RADIUS*.16, RADIUS*.94,
         .011 if index % 5 == 0 else .006, .0040, recess)
for fraction, count, coverage in ((.36, 48, .54), (.54, 48, .57), (.72, 60, .52)):
    r = RADIUS*fraction
    for index in range(count):
        mid = math.tau*index/count
        half = math.pi/count*coverage
        sector("Fine dashed scoring arc", r-.008, r+.008,
               .0037, .0046, mid-half, mid+half, mark, 3)
ring("Center dark well", .76, .91, .0032, .0045, recess)
cylinder("Center field", .78, .0035, .0048, graphite, .0002)
ring("Center warm halo", .80, .83, .0050, .0064, warm)
cylinder("Center point", .035, .0050, .0067, warm, .0002)

# The actual divider sockets are positioned by AFlickTestArena, not baked into
# this shared deck. Avoid decorative recesses that imply false divider sites.
for index in range(32):
    mid = math.tau*index/32
    half = math.pi/32*.78
    sector("Warm rim light window", RADIUS*.972, RADIUS*.983,
           .0030, .0050, mid-half, mid+half, warm, 6)
    sector("Segmented metal crown", RADIUS*.987, RADIUS*.996,
           -.013, -.005, mid-half*.94, mid+half*.94, titanium, 6)
    sector("Crown reveal", RADIUS*.963, RADIUS*.967,
           -.011, -.005, mid-half*.88, mid+half*.88, recess, 6)

for index in range(8):
    angle = math.tau*(index+.5)/8
    sector("Direction lozenge", RADIUS*.775, RADIUS*.804,
           .0042, .0054, angle-.013, angle+.013, muted_warm, 2)

bpy.ops.object.select_all(action="DESELECT")
for obj in parts:
    obj.select_set(True)
bpy.context.view_layer.objects.active = parts[0]
bpy.ops.object.convert(target="MESH")
bpy.ops.object.join()
arena = bpy.context.object
arena.name = "SM_ClassicArena_Premium"
arena.data.name = "SM_ClassicArena_Premium_Mesh"
scene.cursor.location = (0, 0, 0)
bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

exports = ROOT / "exports"
renders = ROOT / "renders"
exports.mkdir(exist_ok=True)
renders.mkdir(exist_ok=True)
bpy.ops.export_scene.fbx(
    filepath=str(exports / "SM_ClassicArena_Premium.fbx"), use_selection=True,
    object_types={"MESH"}, add_leaf_bones=False, axis_forward="-Y",
    axis_up="Z", mesh_smooth_type="FACE")
bpy.ops.export_scene.gltf(
    filepath=str(exports / "SM_ClassicArena_Premium.glb"), use_selection=True,
    export_format="GLB")

# Save an editable scene and an art-review render; neither is required at runtime.
world = bpy.data.worlds.new("Premium arena studio") if not bpy.data.worlds else bpy.data.worlds[0]
scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (.13, .15, .18, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = .18
bpy.ops.object.camera_add(location=(0, -12.8, 12.0))
camera = bpy.context.object
direction = -camera.location
camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
camera.data.type = "ORTHO"
camera.data.ortho_scale = 17.0
scene.camera = camera
for location, power, size, color in (
    ((-6, -4, 11), 1800, 9, (1.0, .82, .64)),
    ((6, 4, 8), 1300, 8, (.70, .83, 1.0)),
):
    bpy.ops.object.light_add(type="AREA", location=location)
    light = bpy.context.object
    light.data.energy = power
    light.data.shape = "DISK"
    light.data.size = size
    light.data.color = color
    light.rotation_euler = (-light.location).to_track_quat("-Z", "Y").to_euler()
scene.render.engine = "BLENDER_EEVEE"
scene.view_settings.view_transform = "Standard"
scene.render.resolution_x = 1600
scene.render.resolution_y = 900
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.filepath = str(renders / "classic_arena_preview.png")
bpy.context.preferences.filepaths.save_version = 0
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "ClassicArenaPremium.blend"))
bpy.ops.render.render(write_still=True)

vertices = [arena.matrix_world @ vertex.co for vertex in arena.data.vertices]
xs = [vertex.x for vertex in vertices]
ys = [vertex.y for vertex in vertices]
zs = [vertex.z for vertex in vertices]
(ROOT / "dimensions.json").write_text(json.dumps({
    "collision_radius_cm": RADIUS*100,
    "visual_bounds_min_m": [min(xs), min(ys), min(zs)],
    "visual_bounds_max_m": [max(xs), max(ys), max(zs)],
    "visual_vertices": len(vertices),
    "runtime_collision": "unchanged AFlickArena::ArenaMesh cylinder",
}, indent=2))
print("CLASSIC_ARENA_GENERATION_COMPLETE", len(vertices))
