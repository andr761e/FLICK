"""Build the visual-only BOB Pocket Foundry venue and production exports."""

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parent.parent
BOARD_OUTER_HALF_M = 6.54
DECK_INNER_HALF_M = 6.82
DECK_OUTER_X_M = 21.2
DECK_OUTER_Y_M = 20.2
FLOOR_Z_M = -0.54
WALL_TOP_M = 5.6
BOB_SOURCE_SURFACE_Z_M = 2.5
GAMEPLAY_CAMERA_RADIUS_M = 15.8
NEAREST_TALL_FRAME_M = 18.4

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for datablocks in (bpy.data.meshes, bpy.data.materials, bpy.data.cameras, bpy.data.lights):
    for datablock in list(datablocks):
        datablocks.remove(datablock)

scene = bpy.context.scene
scene.unit_settings.system = "METRIC"
scene.unit_settings.length_unit = "METERS"


def material(name, color, metallic, roughness, emission=0.0):
    value = bpy.data.materials.new(name)
    value.diffuse_color = (*color, 1.0)
    value.use_nodes = True
    shader = value.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (*color, 1.0)
    shader.inputs["Metallic"].default_value = metallic
    shader.inputs["Roughness"].default_value = roughness
    if emission:
        shader.inputs["Emission Color"].default_value = (*color, 1.0)
        shader.inputs["Emission Strength"].default_value = emission
    return value


foundry_black = material("01_Foundry_Black", (.012, .022, .029), .55, .29)
blue_steel = material("02_Blue_Steel", (.075, .12, .145), .82, .24)
gunmetal = material("03_Gunmetal", (.19, .24, .27), .90, .20)
concrete = material("04_Smoked_Concrete", (.20, .23, .24), .04, .62)
brass = material("05_Aged_Brass", (.48, .25, .055), .86, .27)
warm = material("06_Warm_Pocket_Light", (1.0, .58, .16), .05, .20, 5.0)
cyan = material("07_Cyan_Score_Light", (.0, .58, .88), .04, .16, 5.8)
orange = material("08_Orange_Score_Light", (1.0, .16, .018), .04, .16, 5.8)


def finish(obj, name, mat, bevel=0.0, smooth=False):
    obj.name = name
    obj.data.name = name + "_Mesh"
    obj.data.materials.append(mat)
    if bevel:
        modifier = obj.modifiers.new("Machined edge", "BEVEL")
        modifier.width = bevel
        modifier.segments = 3
    if smooth:
        for polygon in obj.data.polygons:
            polygon.use_smooth = True
    return obj


def cube(name, location, size, mat, rotation=0.0, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=(0.0, 0.0, rotation))
    obj = bpy.context.object
    obj.scale = Vector(size) * 0.5
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, name, mat, bevel)


def cylinder(name, location, radius, depth, mat, vertices=64, bevel=0.0):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location)
    return finish(bpy.context.object, name, mat, bevel, True)


def torus_facing_center(name, location, major, minor, mat):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
                                    major_segments=64, minor_segments=12, location=location)
    obj = bpy.context.object
    direction = -Vector((location[0], location[1], 0.0)).normalized()
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return finish(obj, name, mat, 0.0, True)


def chamfered_points(half_x, half_y, cut):
    return [(-half_x + cut, -half_y), (half_x - cut, -half_y),
            (half_x, -half_y + cut), (half_x, half_y - cut),
            (half_x - cut, half_y), (-half_x + cut, half_y),
            (-half_x, half_y - cut), (-half_x, -half_y + cut)]


def chamfered_ring(name, inner_x, inner_y, outer_x, outer_y, z0, z1, mat):
    outer = chamfered_points(outer_x, outer_y, 1.35)
    inner = chamfered_points(inner_x, inner_y, .38)
    verts = [(x, y, z) for z in (z0, z1) for ring in (outer, inner) for x, y in ring]
    faces = []
    for i in range(8):
        n = (i + 1) % 8
        ob, ib, ot, it = i, 8 + i, 16 + i, 24 + i
        obn, ibn, otn, itn = n, 8 + n, 16 + n, 24 + n
        faces.extend(((ot, otn, itn, it), (obn, ob, ib, ibn),
                      (ob, obn, otn, ot), (ibn, ib, it, itn)))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    return finish(obj, name, mat, .025)


def radial_cube(name, angle_deg, radius, radial_depth, width, z, height, mat, bevel=.0):
    angle = math.radians(angle_deg)
    return cube(name, (radius * math.cos(angle), radius * math.sin(angle), z + height * .5),
                (radial_depth, width, height), mat, angle, bevel)


structure = []
lights = []

# A square sunken deck follows BOB's board instead of the circular Knockout bowl.
structure.append(chamfered_ring("Pocket foundry deck", DECK_INNER_HALF_M, DECK_INNER_HALF_M,
                               DECK_OUTER_X_M, DECK_OUTER_Y_M, FLOOR_Z_M - .22, FLOOR_Z_M, foundry_black))
structure.append(chamfered_ring("Board shadow socket", BOARD_OUTER_HALF_M + .02, BOARD_OUTER_HALF_M + .02,
                               DECK_INNER_HALF_M, DECK_INNER_HALF_M, FLOOR_Z_M - .03, FLOOR_Z_M + .04, gunmetal))
structure.append(cube("Arena undercroft", (0, 0, FLOOR_Z_M - .62), (14.2, 14.2, 1.0), blue_steel, bevel=.12))

# Etched square lanes and brass capture paths lead toward all four pockets.
for offset in (-16.8, -13.2, 13.2, 16.8):
    structure.append(cube("Long deck seam", (offset, 0, FLOOR_Z_M + .012), (.025, 34.0, .018), gunmetal))
for offset in (-15.2, 15.2):
    structure.append(cube("Cross deck seam", (0, offset, FLOOR_Z_M + .014), (35.0, .025, .02), gunmetal))
for sx in (-1, 1):
    for sy in (-1, 1):
        angle = math.degrees(math.atan2(sy, sx))
        structure.append(radial_cube("Pocket approach rail", angle, 10.4, 6.2, .11,
                                     FLOOR_Z_M + .025, .05, brass, .012))

# Four diagonal corner galleries make every pocket a focal point.
for gallery, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    angle = math.atan2(sy, sx)
    for tier in range(4):
        radius = 19.0 + tier * .62
        z = FLOOR_Z_M + tier * .34
        structure.append(radial_cube(f"Gallery {gallery} tier {tier}", math.degrees(angle), radius,
                                     1.18, 4.5 - tier * .18, z, .30, concrete, .035))
        structure.append(radial_cube(f"Gallery {gallery} fascia {tier}", math.degrees(angle), radius - .54,
                                     .08, 4.15 - tier * .18, z + .27, .16, blue_steel, .018))
        lights.append(radial_cube(f"Gallery {gallery} step light {tier}", math.degrees(angle), radius - .60,
                                  .035, 3.65 - tier * .16, z + .34, .045, warm, .006))
    # Tall pocket-score pylons carry an upright ring facing the board centre.
    tower_x, tower_y = sx * 15.4, sy * 14.1
    structure.append(cube(f"Pocket pylon {gallery}", (tower_x, tower_y, 2.15),
                          (.42, .42, 5.4), gunmetal, math.radians(45), .055))
    structure.append(torus_facing_center(f"Pocket halo frame {gallery}", (tower_x, tower_y, 4.35),
                                        .82, .12, brass))
    light_mat = cyan if sx < 0 else orange
    lights.append(torus_facing_center(f"Pocket halo light {gallery}",
                                     (tower_x - sx * .03, tower_y - sy * .03, 4.35), .82, .045, light_mat))
    lights.append(cylinder(f"Pocket halo core {gallery}",
                           (tower_x - sx * .05, tower_y - sy * .05, 4.35), .14, .08, light_mat, 48, .01))

# Rectangular wall modules and open broadcast portals keep the room readable.
for side, (loc, size) in enumerate((((0, 19.3, 2.45), (34.5, .72, 5.3)),
                                    ((-20.45, 0, 2.15), (.72, 32.8, 4.7)),
                                    ((20.45, 0, 2.15), (.72, 32.8, 4.7)))):
    structure.append(cube(f"Foundry wall {side}", loc, size, concrete, bevel=.08))
    inset_size = (size[0] - .5, .12, 1.05) if side == 0 else (.12, size[1] - .5, 1.05)
    inset_loc = (loc[0], loc[1] - .38, 2.35) if side == 0 else (loc[0] + (-.38 if side == 2 else .38), loc[1], 2.15)
    structure.append(cube(f"Foundry wall inset {side}", inset_loc, inset_size, foundry_black, bevel=.025))

for x in (-14.2, -8.6, -3.0, 3.0, 8.6, 14.2):
    structure.append(cube("Rear wall rib", (x, 18.87, 2.65), (.18, .42, 5.15), gunmetal, bevel=.025))
    lights.append(cube("Rear inspection light", (x, 18.63, 3.7), (1.22, .06, .075), warm, bevel=.01))

# Two overhead bridge gantries cross the rear corners, a signature foundry silhouette.
for index, angle in enumerate((32.0, 148.0)):
    structure.append(radial_cube(f"Suspended gantry {index}", angle, 21.0, 5.2, .24,
                                 5.15, .24, blue_steel, .035))
    for step in (-2.5, 0.0, 2.5):
        a = math.radians(angle)
        center = Vector((21.0 * math.cos(a), 21.0 * math.sin(a), 5.02))
        tangent = Vector((-math.sin(a), math.cos(a), 0))
        p = center + tangent * step
        lights.append(cube(f"Gantry lamp {index}", p, (.07, .95, .07), warm, a, .008))


def join_asset(name, objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.convert(target="MESH")
    bpy.ops.object.join()
    result = bpy.context.object
    result.name = name
    result.data.name = name + "_Mesh"
    scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return result


def export_asset(obj):
    export_dir = ROOT / "exports"
    export_dir.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(export_dir / f"{obj.name}.fbx"), use_selection=True,
                             object_types={"MESH"}, add_leaf_bones=False,
                             axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE")
    bpy.ops.export_scene.gltf(filepath=str(export_dir / f"{obj.name}.glb"),
                              use_selection=True, export_format="GLB")


structure_asset = join_asset("SM_BobStadium_Structure", structure)
lights_asset = join_asset("SM_BobStadium_Lights", lights)
for asset in (structure_asset, lights_asset):
    export_asset(asset)
    asset.hide_render = True

# Bring the board into the render only; it remains absent from production exports.
arena_preview = ROOT.parent / "BOB Arena" / "exports" / "SM_BobArena_HighDetail.fbx"
if arena_preview.is_file():
    existing = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(arena_preview))
    for obj in set(bpy.data.objects) - existing:
        obj.location.z -= BOB_SOURCE_SURFACE_Z_M

preview = bpy.data.collections.new("POCKET_FOUNDRY_PREVIEW")
scene.collection.children.link(preview)
for asset in (structure_asset, lights_asset):
    copy = asset.copy()
    copy.data = asset.data.copy()
    copy.hide_render = False
    preview.objects.link(copy)


def area_light(name, location, energy, size, color):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector((0, 0, 0)) - obj.location).to_track_quat("-Z", "Y").to_euler()


area_light("Foundry key", (-7.5, -8.0, 13.0), 1700, 7.0, (.72, .86, 1.0))
area_light("Foundry warm fill", (8.0, -2.0, 9.0), 1100, 5.0, (1.0, .66, .32))
area_light("Board softbox", (0, 0, 15.0), 1900, 8.0, (.86, .93, 1.0))
bpy.ops.object.camera_add(location=(0.0, -28.0, 17.5))
camera = bpy.context.object
camera.rotation_euler = (Vector((0, 0, -.15)) - camera.location).to_track_quat("-Z", "Y").to_euler()
camera.data.lens = 34
scene.camera = camera
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.world.color = (.004, .008, .012)
try:
    scene.view_settings.look = "AgX - Medium High Contrast"
except TypeError:
    pass

(ROOT / "renders").mkdir(exist_ok=True)
scene.render.filepath = str(ROOT / "renders" / "bob_stadium_preview.png")
manifest = {
    "concept": "Pocket Foundry",
    "board_outer_span_cm": BOARD_OUTER_HALF_M * 200.0,
    "centre_opening_cm": DECK_INNER_HALF_M * 200.0,
    "outer_dimensions_cm": [DECK_OUTER_X_M * 200.0, DECK_OUTER_Y_M * 200.0],
    "floor_z_cm": FLOOR_Z_M * 100.0,
    "wall_top_z_cm": WALL_TOP_M * 100.0,
    "gameplay_camera_radius_cm": GAMEPLAY_CAMERA_RADIUS_M * 100.0,
    "nearest_tall_frame_cm": NEAREST_TALL_FRAME_M * 100.0,
    "exports": [structure_asset.name, lights_asset.name],
}
(ROOT / "manifest.json").write_text(json.dumps(manifest, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "BOBPocketFoundry.blend"))
bpy.ops.render.render(write_still=True)
print("FLICK_BOB_STADIUM_GENERATION_COMPLETE", json.dumps(manifest))
