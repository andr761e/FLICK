"""Generate FLICK's high-detail BOB arena from the supplied v3 design."""
from pathlib import Path
import json
import math

import bpy


ROOT = Path(__file__).resolve().parents[1]
EXPORTS = ROOT / "exports"
RENDERS = ROOT / "renders"
EXPORTS.mkdir(parents=True, exist_ok=True)
RENDERS.mkdir(parents=True, exist_ok=True)

# Authoritative runtime dimensions, in metres.
PLAY_SIZE = 12.40
HALF_PLAY = PLAY_SIZE * 0.5
BOARD_THICKNESS = 0.50
SURFACE_Z = 2.50
BOARD_BOTTOM_Z = SURFACE_Z - BOARD_THICKNESS
RAIL_HEIGHT = 0.42
RAIL_THICKNESS = 0.34
OUTER_SIZE = 13.08
HALF_OUTER = OUTER_SIZE * 0.5
POCKET_RADIUS = 0.66
POCKET_POS = 4.70

SURFACE_THICKNESS = 0.028
LINE_WIDTH = 0.040
LINE_Z = SURFACE_Z + 0.004  # 4 mm separation prevents gameplay-camera flicker.
SQUARE_HALF = 4.10
CORNER_GUIDE_RADIUS = 0.20
CENTER_RING_RADIUS = 1.22
COLLECTION_NAME = "BOB_ARENA_HIGH_DETAIL"


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for collection in list(bpy.data.collections):
        if collection.name != "Collection":
            bpy.data.collections.remove(collection)
    collection = bpy.data.collections.get("Collection")
    collection.name = COLLECTION_NAME
    return collection


COLLECTION = reset_scene()
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.length_unit = "METERS"


def material(name, color, metallic=0.0, roughness=0.5, emission=0.0):
    result = bpy.data.materials.new(name)
    result.use_nodes = True
    shader = result.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1.0)
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    if "Specular IOR Level" in shader.inputs:
        shader.inputs["Specular IOR Level"].default_value = 0.48
    if emission > 0.0:
        socket = "Emission Color" if "Emission Color" in shader.inputs else "Emission"
        shader.inputs[socket].default_value = (*color, 1.0)
        if "Emission Strength" in shader.inputs:
            shader.inputs["Emission Strength"].default_value = emission
    return result


MATS = {
    "01_BOB_Surface": material("01_BOB_Surface", (0.72, 0.77, 0.80), 0.08, 0.46),
    "02_BOB_Wood": material("02_BOB_Wood", (0.23, 0.12, 0.055), 0.0, 0.34),
    "03_BOB_InnerWall": material("03_BOB_InnerWall", (0.64, 0.69, 0.72), 0.18, 0.31),
    "04_BOB_Red": material("04_BOB_Red", (0.72, 0.025, 0.035), 0.1, 0.29, 0.15),
    "05_BOB_PocketRed": material("05_BOB_PocketRed", (0.82, 0.018, 0.028), 0.18, 0.22, 0.55),
    "06_BOB_PocketVoid": material("06_BOB_PocketVoid", (0.008, 0.011, 0.016), 0.1, 0.62),
    "07_BOB_Silver": material("07_BOB_Silver", (0.72, 0.79, 0.84), 0.94, 0.20),
    "08_BOB_DarkMetal": material("08_BOB_DarkMetal", (0.038, 0.050, 0.066), 0.72, 0.25),
    "09_BOB_PocketBottom": material("09_BOB_PocketBottom", (0.11, 0.012, 0.018), 0.24, 0.32, 0.08),
}


def finish_object(obj, mat, bevel_width=0.0, smooth=True):
    obj.data.materials.append(mat)
    if bevel_width > 0.0:
        modifier = obj.modifiers.new("EdgeBevel", "BEVEL")
        modifier.width = bevel_width
        modifier.segments = 3
        modifier.limit_method = "ANGLE"
    if smooth:
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
    return obj


def cube(name, size, location, mat, bevel_width=0.0):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(value * 0.5 for value in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish_object(obj, mat, bevel_width)


def cylinder(name, radius, depth, location, mat, vertices=128, bevel_width=0.0):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    obj = bpy.context.object
    obj.name = name
    return finish_object(obj, mat, bevel_width)


def flat_rect(name, width, height, z, mat, location=(0.0, 0.0)):
    half_width, half_height = width * 0.5, height * 0.5
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(
        [(-half_width, -half_height, z), (half_width, -half_height, z),
         (half_width, half_height, z), (-half_width, half_height, z)],
        [], [(0, 1, 2, 3)])
    obj = bpy.data.objects.new(name, mesh)
    COLLECTION.objects.link(obj)
    obj.location.x, obj.location.y = location
    return finish_object(obj, mat, smooth=False)


def flat_ring(name, radius, width, z, mat, location=(0.0, 0.0), segments=160):
    inner, outer = radius - width * 0.5, radius + width * 0.5
    vertices, faces = [], []
    for index in range(segments):
        angle = 2.0 * math.pi * index / segments
        cosine, sine = math.cos(angle), math.sin(angle)
        vertices.extend(((inner * cosine, inner * sine, z), (outer * cosine, outer * sine, z)))
    for index in range(segments):
        current, following = index * 2, ((index + 1) % segments) * 2
        faces.append((current, following, following + 1, current + 1))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    obj = bpy.data.objects.new(name, mesh)
    COLLECTION.objects.link(obj)
    obj.location.x, obj.location.y = location
    return finish_object(obj, mat, smooth=False)


def solid_ring(name, radius, width, depth, z, mat, location=(0.0, 0.0), segments=160):
    """Create a shallow two-sided annulus that survives FBX back-face culling."""
    inner, outer = radius - width * 0.5, radius + width * 0.5
    bottom, top = z - depth * 0.5, z + depth * 0.5
    vertices, faces = [], []
    for index in range(segments):
        angle = 2.0 * math.pi * index / segments
        cosine, sine = math.cos(angle), math.sin(angle)
        vertices.extend((
            (inner * cosine, inner * sine, bottom),
            (outer * cosine, outer * sine, bottom),
            (inner * cosine, inner * sine, top),
            (outer * cosine, outer * sine, top),
        ))
    for index in range(segments):
        current, following = index * 4, ((index + 1) % segments) * 4
        faces.extend((
            (current + 2, current + 3, following + 3, following + 2),
            (current, following, following + 1, current + 1),
            (current + 1, following + 1, following + 3, current + 3),
            (current, current + 2, following + 2, following),
        ))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    obj = bpy.data.objects.new(name, mesh)
    COLLECTION.objects.link(obj)
    obj.location.x, obj.location.y = location
    return finish_object(obj, mat, bevel_width=min(width, depth) * 0.18, smooth=False)


def boolean_difference(target, cutter):
    # Lock in authored edge bevels before cutting. Applying a Boolean below an
    # unapplied bevel produces order-dependent corner seams in Blender/FBX.
    bpy.context.view_layer.objects.active = target
    target.select_set(True)
    for existing in list(target.modifiers):
        bpy.ops.object.modifier_apply(modifier=existing.name)
    modifier = target.modifiers.new("PocketCut", "BOOLEAN")
    modifier.operation = "DIFFERENCE"
    modifier.solver = "EXACT"
    modifier.object = cutter
    bpy.context.view_layer.objects.active = target
    target.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    target.select_set(False)


objects = []
base = cube("BOB_Base", (OUTER_SIZE, OUTER_SIZE, BOARD_THICKNESS - SURFACE_THICKNESS),
            (0.0, 0.0, BOARD_BOTTOM_Z + (BOARD_THICKNESS - SURFACE_THICKNESS) * 0.5),
            MATS["02_BOB_Wood"], 0.018)
surface = cube("BOB_PlaySurface", (PLAY_SIZE, PLAY_SIZE, SURFACE_THICKNESS),
               (0.0, 0.0, SURFACE_Z - SURFACE_THICKNESS * 0.5), MATS["01_BOB_Surface"], 0.003)
objects.extend((base, surface))

# A dark lower reveal and slim aluminium edge make the silhouette read cleanly.
objects.append(cube("BOB_LowerReveal", (OUTER_SIZE - 0.12, OUTER_SIZE - 0.12, 0.105),
                    (0.0, 0.0, BOARD_BOTTOM_Z + 0.055), MATS["08_BOB_DarkMetal"], 0.014))

rail_z = SURFACE_Z + RAIL_HEIGHT * 0.5
rail_cross = OUTER_SIZE - 2.0 * RAIL_THICKNESS
rails = [
    cube("Rail_Left", (RAIL_THICKNESS, OUTER_SIZE, RAIL_HEIGHT),
         (-HALF_OUTER + RAIL_THICKNESS * 0.5, 0.0, rail_z), MATS["02_BOB_Wood"], 0.014),
    cube("Rail_Right", (RAIL_THICKNESS, OUTER_SIZE, RAIL_HEIGHT),
         (HALF_OUTER - RAIL_THICKNESS * 0.5, 0.0, rail_z), MATS["02_BOB_Wood"], 0.014),
    cube("Rail_Top", (rail_cross, RAIL_THICKNESS, RAIL_HEIGHT),
         (0.0, HALF_OUTER - RAIL_THICKNESS * 0.5, rail_z), MATS["02_BOB_Wood"], 0.014),
    cube("Rail_Bottom", (rail_cross, RAIL_THICKNESS, RAIL_HEIGHT),
         (0.0, -HALF_OUTER + RAIL_THICKNESS * 0.5, rail_z), MATS["02_BOB_Wood"], 0.014),
]
objects.extend(rails)

# Reflective rail caps, inner liners, and red inset strips add premium layering.
cap_z = SURFACE_Z + RAIL_HEIGHT + 0.012
for name, size, location in (
    ("Cap_Left", (0.25, OUTER_SIZE - 0.20, 0.024), (-HALF_OUTER + 0.17, 0.0, cap_z)),
    ("Cap_Right", (0.25, OUTER_SIZE - 0.20, 0.024), (HALF_OUTER - 0.17, 0.0, cap_z)),
    ("Cap_Top", (rail_cross - 0.10, 0.25, 0.024), (0.0, HALF_OUTER - 0.17, cap_z)),
    ("Cap_Bottom", (rail_cross - 0.10, 0.25, 0.024), (0.0, -HALF_OUTER + 0.17, cap_z)),
):
    objects.append(cube(name, size, location, MATS["07_BOB_Silver"], 0.008))

for sx in (-1.0, 1.0):
    for sy in (-1.0, 1.0):
        objects.append(cube("CornerCap", (0.30, 0.30, 0.030),
                            (sx * (HALF_OUTER - 0.17), sy * (HALF_OUTER - 0.17), cap_z + 0.004),
                            MATS["07_BOB_Silver"], 0.012))

liner_thickness = 0.012
liner_face = HALF_PLAY - 0.010
liner_length = PLAY_SIZE - 0.040
for name, size, location in (
    ("Liner_Left", (liner_thickness, liner_length, RAIL_HEIGHT - 0.035), (-liner_face, 0.0, rail_z)),
    ("Liner_Right", (liner_thickness, liner_length, RAIL_HEIGHT - 0.035), (liner_face, 0.0, rail_z)),
    ("Liner_Top", (liner_length, liner_thickness, RAIL_HEIGHT - 0.035), (0.0, liner_face, rail_z)),
    ("Liner_Bottom", (liner_length, liner_thickness, RAIL_HEIGHT - 0.035), (0.0, -liner_face, rail_z)),
):
    objects.append(cube(name, size, location, MATS["03_BOB_InnerWall"], 0.004))

# Cut the four gameplay pockets through the broad visual layers.
cutters = []
for sx in (-1.0, 1.0):
    for sy in (-1.0, 1.0):
        x, y = sx * POCKET_POS, sy * POCKET_POS
        cutter = cylinder("PocketCutter", POCKET_RADIUS, 1.10, (x, y, 2.38), MATS["06_BOB_PocketVoid"], 128)
        cutters.append(cutter)
        for target in (base, surface, *rails):
            boolean_difference(target, cutter)

        liner = cylinder("PocketLiner", POCKET_RADIUS * 0.985, 0.24,
                         (x, y, SURFACE_Z - 0.13), MATS["05_BOB_PocketRed"], 128, 0.006)
        bottom = cylinder("PocketBottom", POCKET_RADIUS * 0.78, 0.030,
                          (x, y, SURFACE_Z - 0.225), MATS["09_BOB_PocketBottom"], 128, 0.004)
        # Cut a true opening through the red sleeve, then retain a recessed dark
        # bottom. This matches the supplied design rather than reading as a flat
        # red target painted over the pocket.
        liner_cutter = cylinder("PocketLinerCutter", POCKET_RADIUS * 0.78, 0.60,
                                (x, y, SURFACE_Z - 0.08), MATS["06_BOB_PocketVoid"], 128)
        boolean_difference(liner, liner_cutter)
        bpy.data.objects.remove(liner_cutter, do_unlink=True)
        objects.extend((liner, bottom))
        objects.append(solid_ring("PocketCollar", POCKET_RADIUS + 0.035, 0.038, 0.010,
                                  LINE_Z + 0.006, MATS["05_BOB_PocketRed"], (x, y), 160))

for cutter in cutters:
    bpy.data.objects.remove(cutter, do_unlink=True)

# Supplied v3 marking construction: straight lines terminate tangentially at
# the complete corner guide circles, with no overlapping rounded-corner arc.
tangent = SQUARE_HALF - CORNER_GUIDE_RADIUS
straight_length = 2.0 * tangent
objects.extend((
    flat_rect("Line_Top", straight_length, LINE_WIDTH, LINE_Z, MATS["04_BOB_Red"], (0.0, SQUARE_HALF)),
    flat_rect("Line_Bottom", straight_length, LINE_WIDTH, LINE_Z, MATS["04_BOB_Red"], (0.0, -SQUARE_HALF)),
    flat_rect("Line_Left", LINE_WIDTH, straight_length, LINE_Z, MATS["04_BOB_Red"], (-SQUARE_HALF, 0.0)),
    flat_rect("Line_Right", LINE_WIDTH, straight_length, LINE_Z, MATS["04_BOB_Red"], (SQUARE_HALF, 0.0)),
    flat_ring("CenterRing", CENTER_RING_RADIUS, LINE_WIDTH, LINE_Z, MATS["04_BOB_Red"]),
    cylinder("CenterMedallion", 0.045, 0.012, (0.0, 0.0, LINE_Z + 0.006), MATS["05_BOB_PocketRed"], 64),
))
for sx in (-1.0, 1.0):
    for sy in (-1.0, 1.0):
        objects.append(solid_ring("CornerGuide", CORNER_GUIDE_RADIUS, LINE_WIDTH, 0.010,
                                  LINE_Z + 0.006, MATS["04_BOB_Red"], (sx * tangent, sy * tangent), 96))

# Small dark-metal fasteners along each rail add scale without changing shape.
for side in (-1.0, 1.0):
    for index in range(-4, 5):
        offset = index * 1.18
        objects.append(cylinder("RailFastener", 0.035, 0.014,
                                (offset, side * (HALF_OUTER - 0.17), cap_z + 0.018),
                                MATS["08_BOB_DarkMetal"], 32, 0.003))
        objects.append(cylinder("RailFastener", 0.035, 0.014,
                                (side * (HALF_OUTER - 0.17), offset, cap_z + 0.018),
                                MATS["08_BOB_DarkMetal"], 32, 0.003))

# Bake transforms, modifiers, and material slots into one visual-only export.
bpy.ops.object.select_all(action="DESELECT")
for obj in objects:
    if obj and obj.name in bpy.data.objects:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        for modifier in list(obj.modifiers):
            bpy.ops.object.modifier_apply(modifier=modifier.name)
bpy.context.view_layer.objects.active = base
bpy.ops.object.join()
arena = bpy.context.object
arena.name = "SM_BobArena_HighDetail"

bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "BobArena.blend"))
bpy.ops.object.select_all(action="DESELECT")
arena.select_set(True)
bpy.context.view_layer.objects.active = arena
bpy.ops.export_scene.fbx(
    filepath=str(EXPORTS / "SM_BobArena_HighDetail.fbx"),
    use_selection=True,
    object_types={"MESH"},
    mesh_smooth_type="FACE",
    use_mesh_modifiers=True,
    add_leaf_bones=False,
    bake_anim=False,
)

# Neutral top-down preview.
world = bpy.data.worlds.new("BOB_World") if bpy.context.scene.world is None else bpy.context.scene.world
bpy.context.scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs[0].default_value = (0.16, 0.19, 0.23, 1.0)
world.node_tree.nodes["Background"].inputs[1].default_value = 0.55
for name, location, energy, size in (
    ("Key", (-7.0, -8.0, 12.0), 2600.0, 8.0),
    ("Fill", (8.0, -2.0, 9.0), 1500.0, 7.0),
    ("Top", (0.0, 0.0, 13.0), 1200.0, 9.0),
):
    data = bpy.data.lights.new(name, "AREA")
    data.energy, data.shape, data.size = energy, "DISK", size
    light = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(light)
    light.location = location
    direction = -light.location
    light.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
camera_data = bpy.data.cameras.new("PreviewCamera")
camera = bpy.data.objects.new("PreviewCamera", camera_data)
bpy.context.scene.collection.objects.link(camera)
camera.location = (10.4, -12.8, 14.8)
camera.rotation_euler = (-camera.location).to_track_quat("-Z", "Y").to_euler()
camera_data.lens = 52
bpy.context.scene.camera = camera
scene = bpy.context.scene
try:
    scene.render.engine = "BLENDER_EEVEE_NEXT"
except TypeError:
    scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 1400
scene.render.resolution_y = 1000
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.filepath = str(RENDERS / "bob_arena_preview.png")
bpy.ops.render.render(write_still=True)

(ROOT / "dimensions.json").write_text(json.dumps({
    "units": "centimeters",
    "play_size": [1240.0, 1240.0],
    "outer_size": [1308.0, 1308.0],
    "board_thickness": 50.0,
    "surface_z": 250.0,
    "rail_height": 42.0,
    "rail_thickness": 34.0,
    "pocket_radius": 66.0,
    "pocket_centers": [[sx * 470.0, sy * 470.0] for sx in (-1, 1) for sy in (-1, 1)],
    "exports": ["SM_BobArena_HighDetail"],
}, indent=2))
print("FLICK BOB arena generated, saved, exported, and rendered.")
