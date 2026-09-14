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


foundry_black = material("01_Foundry_Black", (.028, .052, .061), .48, .34)
blue_steel = material("02_Blue_Steel", (.12, .20, .24), .76, .28)
gunmetal = material("03_Gunmetal", (.27, .34, .37), .84, .23)
concrete = material("04_Smoked_Concrete", (.34, .39, .40), .04, .58)
brass = material("05_Aged_Brass", (.62, .37, .09), .82, .29)
warm = material("06_Warm_Pocket_Light", (1.0, .68, .28), .04, .18, 6.5)
cyan = material("07_Cyan_Score_Light", (.02, .72, 1.0), .03, .14, 7.5)
orange = material("08_Orange_Score_Light", (1.0, .25, .035), .03, .14, 7.5)
light_composite = material("09_Light_Composite", (.58, .66, .68), .16, .36)


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


def gallery_cube(name, angle, radius, tangent_offset, size, z, mat, bevel=.0):
    radial = Vector((math.cos(angle), math.sin(angle), 0.0))
    tangent = Vector((-math.sin(angle), math.cos(angle), 0.0))
    location = radial * radius + tangent * tangent_offset
    location.z = z + size[2] * .5
    return cube(name, location, size, mat, angle, bevel)


def beam_between(name, start, end, width, mat, bevel=.0):
    start = Vector(start)
    end = Vector(end)
    direction = end - start
    obj = cube(name, (start + end) * .5, (direction.length, width, width), mat, bevel=bevel)
    obj.rotation_euler = direction.to_track_quat("X", "Z").to_euler()
    return obj


structure = []
lights = []

# A square sunken deck follows BOB's board instead of the circular Knockout bowl.
structure.append(chamfered_ring("Pocket foundry deck", DECK_INNER_HALF_M, DECK_INNER_HALF_M,
                               DECK_OUTER_X_M, DECK_OUTER_Y_M, FLOOR_Z_M - .22, FLOOR_Z_M, foundry_black))
structure.append(chamfered_ring("Board shadow socket", BOARD_OUTER_HALF_M + .02, BOARD_OUTER_HALF_M + .02,
                               DECK_INNER_HALF_M, DECK_INNER_HALF_M, FLOOR_Z_M - .03, FLOOR_Z_M + .04, gunmetal))
structure.append(cube("Arena undercroft", (0, 0, FLOOR_Z_M - .62), (14.2, 14.2, 1.0), blue_steel, bevel=.12))
structure.append(chamfered_ring("Light service apron", 7.15, 7.15, 14.8, 14.2,
                               FLOOR_Z_M + .01, FLOOR_Z_M + .075, light_composite))
structure.append(chamfered_ring("Service apron outer frame", 14.8, 14.2, 15.05, 14.45,
                               FLOOR_Z_M + .02, FLOOR_Z_M + .12, gunmetal))
lights.append(chamfered_ring("Service apron light rail", 14.57, 13.97, 14.68, 14.08,
                            FLOOR_Z_M + .078, FLOOR_Z_M + .105, warm))
structure.append(chamfered_ring("Board service frame", 6.86, 6.86, 7.18, 7.18,
                               FLOOR_Z_M + .076, FLOOR_Z_M + .14, blue_steel))

# Fine panel breaks, access plates, and fasteners keep the brighter apron from
# reading as a single empty slab.
for offset in (-11.7, -9.25, 9.25, 11.7):
    structure.append(cube("Apron longitudinal seam", (offset, 0, FLOOR_Z_M + .086),
                          (.028, 24.7, .022), gunmetal))
for offset in (-10.9, -8.35, 8.35, 10.9):
    structure.append(cube("Apron cross seam", (0, offset, FLOOR_Z_M + .087),
                          (25.6, .028, .023), gunmetal))
for side in (-1, 1):
    for offset in (-11.3, -8.65, 0.0, 8.65, 11.3):
        structure.append(cube("Apron access plate", (offset, side * 11.9, FLOOR_Z_M + .105),
                              (1.65, .72, .06), blue_steel, bevel=.035))
        for fastener_x in (-.62, .62):
            structure.append(cylinder("Apron fastener",
                                      (offset + fastener_x, side * 11.9, FLOOR_Z_M + .148),
                                      .045, .028, brass, 24, .006))
    for offset in (-10.3, -6.8, 0.0, 6.8, 10.3):
        structure.append(cube("Side apron access plate", (side * 12.5, offset, FLOOR_Z_M + .105),
                              (.72, 1.65, .06), blue_steel, bevel=.035))
        for fastener_y in (-.62, .62):
            structure.append(cylinder("Side apron fastener",
                                      (side * 12.5, offset + fastener_y, FLOOR_Z_M + .148),
                                      .045, .028, brass, 24, .006))
lights.append(cube("Cyan board service rail", (-7.12, 0.0, FLOOR_Z_M + .151),
                   (.035, 7.6, .028), cyan, bevel=.006))
lights.append(cube("Orange board service rail", (7.12, 0.0, FLOOR_Z_M + .151),
                   (.035, 7.6, .028), orange, bevel=.006))

# Etched square lanes and brass capture paths lead toward all four pockets.
for offset in (-16.8, -13.2, 13.2, 16.8):
    structure.append(cube("Long deck seam", (offset, 0, FLOOR_Z_M + .012), (.025, 34.0, .018), gunmetal))
for offset in (-15.2, 15.2):
    structure.append(cube("Cross deck seam", (0, offset, FLOOR_Z_M + .014), (35.0, .025, .02), gunmetal))
for sx in (-1, 1):
    for sy in (-1, 1):
        angle = math.degrees(math.atan2(sy, sx))
        structure.append(radial_cube("Pocket approach rail", angle, 10.4, 6.2, .11,
                                     FLOOR_Z_M + .078, .05, brass, .012))

# Machined service hatches and low light lanes fill the apron without entering
# the camera volume above it.
for side in (-1, 1):
    for offset in (-11.5, -7.8, 7.8, 11.5):
        structure.append(cube("Deck service hatch", (offset, side * 16.7, FLOOR_Z_M + .055),
                              (2.55, 1.35, .07), blue_steel, bevel=.045))
        structure.append(cube("Deck hatch inset", (offset, side * 16.7, FLOOR_Z_M + .094),
                              (2.05, .82, .018), foundry_black, bevel=.025))
        light_mat = cyan if offset < 0 else orange
        lights.append(cube("Deck hatch status", (offset, side * 16.25, FLOOR_Z_M + .108),
                           (1.15, .045, .025), light_mat, bevel=.006))
    for offset in (-12.2, -7.4, 7.4, 12.2):
        structure.append(cube("Side service hatch", (side * 17.7, offset, FLOOR_Z_M + .055),
                              (1.35, 2.55, .07), blue_steel, bevel=.045))
        structure.append(cube("Side hatch inset", (side * 17.7, offset, FLOOR_Z_M + .094),
                              (.82, 2.05, .018), foundry_black, bevel=.025))
        lights.append(cube("Side hatch status", (side * 17.25, offset, FLOOR_Z_M + .108),
                           (.045, 1.15, .025), cyan if side < 0 else orange, bevel=.006))

# Four diagonal corner galleries make every pocket a focal point.
for gallery, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    angle = math.atan2(sy, sx)
    for tier in range(4):
        radius = 19.0 + tier * .62
        z = FLOOR_Z_M + tier * .34
        structure.append(radial_cube(f"Gallery {gallery} tier {tier}", math.degrees(angle), radius,
                                     1.18, 4.5 - tier * .18, z, .30,
                                     light_composite if tier % 2 == 0 else concrete, .035))
        structure.append(radial_cube(f"Gallery {gallery} fascia {tier}", math.degrees(angle), radius - .54,
                                     .08, 4.15 - tier * .18, z + .27, .16, blue_steel, .018))
        lights.append(radial_cube(f"Gallery {gallery} step light {tier}", math.degrees(angle), radius - .60,
                                  .035, 3.65 - tier * .16, z + .34, .045, warm, .006))
        for seat in range(-3, 4):
            seat_offset = seat * .54
            structure.append(gallery_cube(f"Gallery {gallery} seat {tier}", angle,
                                          radius + .04, seat_offset, (.38, .42, .13),
                                          z + .31, blue_steel, .025))
            structure.append(gallery_cube(f"Gallery {gallery} seat back {tier}", angle,
                                          radius + .24, seat_offset, (.10, .42, .38),
                                          z + .42, gunmetal, .025))
    rail_radius = 18.35
    structure.append(radial_cube(f"Gallery {gallery} safety rail", math.degrees(angle), rail_radius,
                                 .07, 4.75, FLOOR_Z_M + .88, .075, gunmetal, .015))
    for post in (-2.25, -1.12, 0.0, 1.12, 2.25):
        structure.append(gallery_cube(f"Gallery {gallery} rail post", angle, rail_radius,
                                      post, (.075, .075, .82), FLOOR_Z_M + .12, gunmetal, .012))
    # Tall pocket-score pylons carry an upright ring facing the board centre.
    tower_x, tower_y = sx * 15.4, sy * 14.1
    structure.append(cube(f"Pocket pylon {gallery}", (tower_x, tower_y, 2.15),
                          (.42, .42, 5.4), gunmetal, math.radians(45), .055))
    tower_angle = math.atan2(tower_y, tower_x)
    tower_radius = math.hypot(tower_x, tower_y)
    for fin in (-.38, .38):
        structure.append(gallery_cube(f"Pocket pylon fin {gallery}", tower_angle, tower_radius,
                                      fin, (.24, .14, 4.55), .0, blue_steel, .035))
    for collar_z in (.25, 2.0, 4.65):
        structure.append(gallery_cube(f"Pocket pylon collar {gallery}", tower_angle, tower_radius,
                                      0.0, (.72, 1.15, .16), collar_z, brass, .025))
    structure.append(torus_facing_center(f"Pocket halo frame {gallery}", (tower_x, tower_y, 4.35),
                                        .82, .12, brass))
    light_mat = cyan if sx < 0 else orange
    lights.append(torus_facing_center(f"Pocket halo light {gallery}",
                                     (tower_x - sx * .03, tower_y - sy * .03, 4.35), .82, .045, light_mat))
    lights.append(cylinder(f"Pocket halo core {gallery}",
                           (tower_x - sx * .05, tower_y - sy * .05, 4.35), .14, .08, light_mat, 48, .01))
    lights.append(gallery_cube(f"Pocket pylon score bar {gallery}", tower_angle,
                               tower_radius - .25, 0.0, (.045, .82, .075), 1.15, light_mat, .008))

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

# Layered broadcast wall bays replace the previous broad, empty slabs.
for index, x in enumerate((-14.7, -9.8, -4.9, 0.0, 4.9, 9.8, 14.7)):
    structure.append(cube("Rear wall bay", (x, 18.67, 2.55), (4.35, .16, 3.25),
                          light_composite, bevel=.055))
    structure.append(cube("Rear wall bay screen", (x, 18.56, 2.58), (3.72, .07, 2.55),
                          foundry_black, bevel=.045))
    structure.append(cube("Rear wall bay sill", (x, 18.46, 1.18), (4.1, .22, .18),
                          brass, bevel=.025))
    bay_light = cyan if index < 3 else orange if index > 3 else warm
    lights.append(cube("Rear wall bay light", (x, 18.50, 3.68), (2.7, .035, .065),
                       bay_light, bevel=.008))

for side in (-1, 1):
    panel_x = side * 19.98
    for index, y in enumerate((-13.4, -8.0, -2.6, 2.8, 8.2, 13.6)):
        structure.append(cube("Side wall bay", (panel_x, y, 2.35), (.16, 4.65, 3.0),
                              light_composite, bevel=.055))
        structure.append(cube("Side wall bay screen", (panel_x - side * .11, y, 2.38),
                              (.07, 3.96, 2.34), foundry_black, bevel=.045))
        structure.append(cube("Side wall equipment plinth", (panel_x - side * .34, y, .44),
                              (.68, 3.45, .88), blue_steel, bevel=.06))
        for vent in (-1.05, -.52, 0.0, .52, 1.05):
            structure.append(cube("Equipment cooling fin", (panel_x - side * .70, y + vent, .5),
                                  (.10, .27, .52), gunmetal, bevel=.018))
        bay_light = cyan if side < 0 else orange
        lights.append(cube("Side wall vertical light", (panel_x - side * .17, y, 3.05),
                           (.035, .07, 1.18), bay_light, bevel=.008))

# A light crown beam and diagonal lattice give the rear wall a readable
# foundry/truss silhouette from every gameplay angle.
structure.append(cube("Rear crown beam", (0.0, 18.95, 5.48), (35.4, .34, .32),
                      gunmetal, bevel=.045))
structure.append(cube("Rear crown cap", (0.0, 18.93, 5.72), (34.6, .24, .12),
                      light_composite, bevel=.025))
for bay_x in (-14.7, -9.8, -4.9, 0.0, 4.9, 9.8, 14.7):
    structure.append(beam_between("Rear lattice brace", (bay_x - 1.65, 18.72, 4.18),
                                  (bay_x + 1.65, 18.72, 5.25), .10, blue_steel, .015))
    structure.append(beam_between("Rear lattice brace", (bay_x + 1.65, 18.69, 4.18),
                                  (bay_x - 1.65, 18.69, 5.25), .10, blue_steel, .015))
lights.append(cube("Rear crown light", (0.0, 18.72, 5.67), (28.5, .035, .055), warm, bevel=.008))

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

# -----------------------------------------------------------------------------
# DETAIL PASS — this is deliberately concentrated outside the gameplay camera
# volume. The goal is a venue that feels authored rather than just enclosed.
# -----------------------------------------------------------------------------

# Layered ceiling perimeter. It gives the room a strong silhouette from low
# cameras without putting a roof directly over the board.
for ring_index, (radius, z, beam_depth, tangent_width) in enumerate((
    (19.3, 6.15, .30, 3.2),
    (20.08, 6.78, .22, 2.75),
)):
    for angle_deg in range(0, 360, 45):
        structure.append(radial_cube(f"Ceiling ring {ring_index}", angle_deg, radius,
                                     beam_depth, tangent_width, z, .24,
                                     gunmetal if ring_index == 0 else blue_steel, .035))
        if ring_index == 0:
            lights.append(radial_cube("Ceiling ring marker", angle_deg, radius - .17,
                                      .035, 1.55, z + .05, .045, warm, .008))

# Diagonal hangers tie the upper ring into the outer wall massing. They are
# intentionally sparse so the silhouette reads as structural rather than noisy.
for angle_deg in range(0, 360, 45):
    a = math.radians(angle_deg)
    radial = Vector((math.cos(a), math.sin(a), 0.0))
    p0 = radial * 18.95 + Vector((0, 0, 5.58))
    p1 = radial * 20.08 + Vector((0, 0, 6.88))
    structure.append(beam_between("Upper ring hanger", p0, p1, .13, blue_steel, .018))

# Four corner utility towers make the arena feel like an operating industrial
# venue. They sit farther out than the score pylons and never enter play space.
for tower_index, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    base_x, base_y = sx * 18.85, sy * 17.55
    tower_angle = math.atan2(base_y, base_x)
    tower_radius = math.hypot(base_x, base_y)
    structure.append(cube(f"Utility tower core {tower_index}", (base_x, base_y, 2.4),
                          (.92, .92, 5.7), foundry_black, math.radians(45), .08))
    for local_z in (.35, 2.15, 4.0, 5.05):
        structure.append(gallery_cube(f"Utility tower collar {tower_index}", tower_angle, tower_radius,
                                      0.0, (1.42, 1.42, .16), local_z, gunmetal, .03))
    for fin in (-.47, .47):
        structure.append(gallery_cube(f"Utility tower vertical fin {tower_index}", tower_angle,
                                      tower_radius, fin, (.18, .16, 4.45), .42, blue_steel, .025))
    # Equipment pods give each tower a believable function.
    for local_z in (1.05, 3.05):
        structure.append(gallery_cube(f"Utility tower equipment pod {tower_index}", tower_angle,
                                      tower_radius - .60, 0.0, (.62, 1.05, .62), local_z,
                                      light_composite, .055))
        structure.append(gallery_cube(f"Utility tower pod inset {tower_index}", tower_angle,
                                      tower_radius - .92, 0.0, (.10, .72, .34), local_z + .14,
                                      foundry_black, .025))
    tower_light = cyan if sx < 0 else orange
    lights.append(gallery_cube(f"Utility tower beacon {tower_index}", tower_angle,
                               tower_radius - .90, 0.0, (.05, .58, .09), 4.62, tower_light, .008))

# Ventilation drums and pipework add large-scale industrial detail high on the
# walls. Their visual mass sits above normal sight lines instead of cluttering
# the floor.
for side in (-1, 1):
    x = side * 19.45
    for idx, y in enumerate((-11.2, -5.6, 5.6, 11.2)):
        bpy.ops.mesh.primitive_cylinder_add(vertices=48, radius=.52, depth=1.05,
                                            location=(x - side * .34, y, 4.45),
                                            rotation=(0.0, math.radians(90), 0.0))
        drum = finish(bpy.context.object, f"Wall ventilation drum {side}_{idx}",
                      gunmetal, .045, True)
        structure.append(drum)
        structure.append(cube(f"Vent drum backplate {side}_{idx}",
                              (x, y, 4.45), (.18, 1.42, 1.42), blue_steel, bevel=.05))
        # Radial grille bars are cheap geometry but read strongly at distance.
        for bar_rot in (0.0, math.radians(45), math.radians(90), math.radians(135)):
            structure.append(cube(f"Vent grille bar {side}_{idx}",
                                  (x - side * .91, y, 4.45), (.07, 1.02, .08),
                                  light_composite, bar_rot, .01))

# Rear wall service pipes: multiple heights, with clamps and occasional valve
# housings. These break up the otherwise broad rear architectural surface.
for pipe_index, z in enumerate((.62, .96, 4.42)):
    structure.append(cube(f"Rear service pipe {pipe_index}", (0.0, 18.20, z),
                          (31.4, .12, .12), brass if pipe_index == 1 else blue_steel, bevel=.04))
    for x in (-13.6, -9.1, -4.55, 0.0, 4.55, 9.1, 13.6):
        structure.append(cube(f"Rear pipe clamp {pipe_index}", (x, 18.18, z),
                              (.18, .22, .34), gunmetal, bevel=.025))
for x in (-11.35, 11.35):
    structure.append(cylinder("Rear line valve", (x, 18.03, .96), .24, .18, brass, 40, .025))
    structure.append(cube("Rear valve handle", (x, 17.91, 1.20), (.62, .08, .08), brass, bevel=.018))

# Control consoles on the side walls. These are silhouettes first; the emissive
# strips provide visual hierarchy without relying on textures.
for side in (-1, 1):
    console_x = side * 18.86
    for console_index, y in enumerate((-10.6, -3.5, 3.5, 10.6)):
        structure.append(cube(f"Control console body {side}_{console_index}",
                              (console_x, y, .60), (1.05, 2.15, 1.20),
                              foundry_black, bevel=.09))
        structure.append(cube(f"Control console face {side}_{console_index}",
                              (console_x - side * .57, y, .92), (.08, 1.65, .56),
                              gunmetal, bevel=.035))
        structure.append(cube(f"Control console screen {side}_{console_index}",
                              (console_x - side * .62, y, 1.02), (.025, 1.16, .28),
                              foundry_black, bevel=.018))
        console_light = cyan if side < 0 else orange
        lights.append(cube(f"Control console status {side}_{console_index}",
                           (console_x - side * .645, y, .78), (.018, .72, .045),
                           console_light, bevel=.005))

# Repeated wall bolt patterns and inspection plates add close-up richness. The
# placement is regular enough to feel designed, not random.
for x in (-16.9, -12.7, -8.45, -4.2, 4.2, 8.45, 12.7, 16.9):
    structure.append(cube("Rear inspection plate", (x, 18.34, .42),
                          (1.24, .08, .56), blue_steel, bevel=.035))
    for bolt_x in (-.46, .46):
        for bolt_z in (.24, .60):
            structure.append(cylinder("Rear inspection bolt",
                                      (x + bolt_x, 18.28, bolt_z),
                                      .04, .025, brass, 20, .005))

# Gallery underlighting and support legs make the spectator tiers feel anchored
# instead of floating.
for gallery, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    angle = math.atan2(sy, sx)
    for support in (-1.65, 0.0, 1.65):
        structure.append(gallery_cube(f"Gallery underframe post {gallery}", angle, 19.55,
                                      support, (.22, .22, 1.45), FLOOR_Z_M - .02,
                                      gunmetal, .025))
        structure.append(gallery_cube(f"Gallery underframe foot {gallery}", angle, 19.55,
                                      support, (.62, .62, .13), FLOOR_Z_M - .08,
                                      blue_steel, .035))
    lights.append(gallery_cube(f"Gallery underglow {gallery}", angle, 19.05, 0.0,
                               (.035, 3.55, .07), FLOOR_Z_M + .12,
                               cyan if sx < 0 else orange, .008))

# Pocket marker chevrons visually point toward the four scoring corners. They
# are kept low and outside the board, acting like embedded venue wayfinding.
for marker_index, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    angle = math.atan2(sy, sx)
    for j, radius in enumerate((8.25, 8.85, 9.45)):
        structure.append(radial_cube(f"Pocket chevron base {marker_index}_{j}",
                                     math.degrees(angle), radius, .46, .62,
                                     FLOOR_Z_M + .082, .04, gunmetal, .008))
        lights.append(radial_cube(f"Pocket chevron light {marker_index}_{j}",
                                  math.degrees(angle), radius - .03, .18, .34,
                                  FLOOR_Z_M + .126, .025,
                                  cyan if sx < 0 else orange, .004))

# A pair of broad scoreboard banners on the rear wall gives the architecture a
# recognisable broadcast identity even before any text/UI is added in Unreal.
for side, x in enumerate((-7.0, 7.0)):
    structure.append(cube(f"Rear scoreboard housing {side}", (x, 18.12, 4.72),
                          (5.6, .34, 1.12), foundry_black, bevel=.075))
    structure.append(cube(f"Rear scoreboard frame {side}", (x, 17.92, 4.72),
                          (5.05, .12, .78), gunmetal, bevel=.05))
    lights.append(cube(f"Rear scoreboard accent {side}", (x, 17.84, 4.35),
                       (3.75, .035, .055), cyan if x < 0 else orange, bevel=.008))

# Small floor bollards around the far apron add depth cues and make the board
# feel like an installation inside a larger venue.
for angle_deg in range(0, 360, 30):
    if angle_deg in (90, 270):
        continue
    a = math.radians(angle_deg)
    radius = 14.95
    x, y = radius * math.cos(a), radius * math.sin(a)
    structure.append(cylinder("Apron safety bollard", (x, y, FLOOR_Z_M + .28),
                              .10, .56, gunmetal, 28, .02))
    structure.append(cylinder("Apron bollard cap", (x, y, FLOOR_Z_M + .58),
                              .13, .08, brass, 28, .018))


# Front wall closes the venue so the stadium now reads as a complete room rather
# than a U-shaped shell.
structure.append(cube("Front wall", (0, -19.3, 2.45), (34.5, .72, 5.3), concrete, bevel=.08))
structure.append(cube("Front wall inset", (0, -18.92, 2.35), (34.0, .12, 1.05), foundry_black, bevel=.025))
for x in (-14.2, -8.6, -3.0, 3.0, 8.6, 14.2):
    structure.append(cube("Front wall rib", (x, -18.87, 2.65), (.18, .42, 5.15), gunmetal, bevel=.025))
    lights.append(cube("Front inspection light", (x, -18.63, 3.7), (1.22, .06, .075), warm, bevel=.01))

# The front wall gets a different treatment from the rear so it does not feel
# like a copy-pasted backdrop. Think entry concourse / production wall.
for index, x in enumerate((-14.7, -9.8, -4.9, 0.0, 4.9, 9.8, 14.7)):
    structure.append(cube("Front wall bay", (x, -18.67, 2.55), (4.35, .16, 3.25),
                          light_composite, bevel=.055))
    structure.append(cube("Front wall recess", (x, -18.53, 2.58), (3.72, .09, 2.55),
                          foundry_black, bevel=.045))
    structure.append(cube("Front wall sill", (x, -18.42, 1.18), (4.1, .22, .18),
                          brass, bevel=.025))
    bay_light = orange if index < 3 else cyan if index > 3 else warm
    lights.append(cube("Front wall bay light", (x, -18.49, 3.68), (2.7, .035, .065),
                       bay_light, bevel=.008))

# A larger central entry / hero bay gives the new front wall a recognisable face.
structure.append(cube("Front hero portal frame", (0.0, -18.56, 2.45), (8.25, .32, 3.85),
                      blue_steel, bevel=.08))
structure.append(cube("Front hero portal recess", (0.0, -18.40, 2.45), (7.25, .14, 3.05),
                      foundry_black, bevel=.05))
structure.append(cube("Front hero canopy", (0.0, -18.98, 4.46), (9.15, 1.05, .26),
                      gunmetal, bevel=.04))
lights.append(cube("Front hero lintel light", (0.0, -18.31, 3.82), (5.45, .035, .065),
                   warm, bevel=.008))
for side in (-1, 1):
    structure.append(cube("Front hero totem", (side * 4.28, -18.42, 2.10), (.44, .28, 3.05),
                          gunmetal, bevel=.05))
    lights.append(cube("Front hero totem light", (side * 4.10, -18.32, 2.72), (.05, .04, 1.55),
                       cyan if side < 0 else orange, bevel=.006))

# Front wall service routing mirrors the rear wall so the enclosure feels built,
# not just sealed off.
for pipe_index, z in enumerate((.62, .96, 4.42)):
    structure.append(cube(f"Front service pipe {pipe_index}", (0.0, -18.20, z),
                          (31.4, .12, .12), brass if pipe_index == 1 else blue_steel, bevel=.04))
    for x in (-13.6, -9.1, -4.55, 0.0, 4.55, 9.1, 13.6):
        structure.append(cube(f"Front pipe clamp {pipe_index}", (x, -18.18, z),
                              (.18, .22, .34), gunmetal, bevel=.025))
for x in (-11.35, 11.35):
    structure.append(cylinder("Front line valve", (x, -18.03, .96), .24, .18, brass, 40, .025))
    structure.append(cube("Front valve handle", (x, -17.91, 1.20), (.62, .08, .08), brass, bevel=.018))

# Continuous upper perimeter beams finally tie all four sides together.
structure.append(cube("Front crown beam", (0.0, -18.95, 5.48), (35.4, .34, .32),
                      gunmetal, bevel=.045))
structure.append(cube("Front crown cap", (0.0, -18.93, 5.72), (34.6, .24, .12),
                      light_composite, bevel=.025))
for bay_x in (-14.7, -9.8, -4.9, 0.0, 4.9, 9.8, 14.7):
    structure.append(beam_between("Front lattice brace", (bay_x - 1.65, -18.72, 4.18),
                                  (bay_x + 1.65, -18.72, 5.25), .10, blue_steel, .015))
    structure.append(beam_between("Front lattice brace", (bay_x + 1.65, -18.69, 4.18),
                                  (bay_x - 1.65, -18.69, 5.25), .10, blue_steel, .015))
lights.append(cube("Front crown light", (0.0, -18.72, 5.67), (28.5, .035, .055), warm, bevel=.008))

for side in (-1, 1):
    structure.append(cube("Side crown beam", (side * 20.24, 0.0, 5.15), (.34, 34.4, .32),
                          gunmetal, bevel=.045))
    structure.append(cube("Side crown cap", (side * 20.18, 0.0, 5.40), (.24, 33.6, .12),
                          light_composite, bevel=.025))
    lights.append(cube("Side crown light", (side * 20.08, 0.0, 5.30), (.035, 27.8, .055),
                       cyan if side < 0 else orange, bevel=.008))
    for y in (-13.4, -8.0, -2.6, 2.8, 8.2, 13.6):
        structure.append(beam_between("Side crown brace",
                                      (side * 20.04, y - 1.65, 4.05),
                                      (side * 20.04, y + 1.65, 5.02), .10, blue_steel, .015))
        structure.append(beam_between("Side crown brace",
                                      (side * 19.97, y + 1.65, 4.05),
                                      (side * 19.97, y - 1.65, 5.02), .10, blue_steel, .015))

# Corner wall piers bridge front/back and side walls so the enclosure reads as a
# single stadium shell, not separate wall cards.
for corner_index, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    structure.append(cube(f"Corner wall pier {corner_index}", (sx * 20.04, sy * 18.88, 2.55),
                          (.86, .86, 5.15), gunmetal, math.radians(45), .07))
    structure.append(cube(f"Corner wall collar {corner_index}", (sx * 20.04, sy * 18.88, 5.10),
                          (1.42, 1.42, .16), brass, math.radians(45), .028))
    lights.append(cube(f"Corner pier light {corner_index}", (sx * 19.79, sy * 18.63, 3.15),
                       (.05, .05, 1.85), cyan if sx < 0 else orange, bevel=.006))

# Short corner catwalks sit high enough that they don't crowd gameplay but they
# connect the walls and ceiling language nicely.
for corner_index, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    corner_angle = math.atan2(sy, sx)
    corner_radius = 21.05
    structure.append(gallery_cube(f"Corner catwalk deck {corner_index}", corner_angle, corner_radius,
                                  0.0, (3.55, 1.24, .16), 4.88, blue_steel, .03))
    structure.append(gallery_cube(f"Corner catwalk rail {corner_index}", corner_angle, corner_radius - .28,
                                  0.0, (3.30, .08, .16), 5.56, gunmetal, .02))
    for offset in (-1.25, 0.0, 1.25):
        structure.append(gallery_cube(f"Corner catwalk post {corner_index}", corner_angle, corner_radius - .28,
                                      offset, (.08, .08, .72), 4.92, gunmetal, .015))
    lights.append(gallery_cube(f"Corner catwalk light {corner_index}", corner_angle, corner_radius - .42,
                               0.0, (2.15, .04, .05), 5.02, warm, .006))

# Extra side-wall service trunks and floor cabinets add more readable detail on
# the newly enclosed perimeter.
for side in (-1, 1):
    for y in (-14.0, -7.0, 0.0, 7.0, 14.0):
        structure.append(cube("Side wall floor cabinet", (side * 19.15, y, .56),
                              (.72, 1.42, 1.12), blue_steel, bevel=.055))
        structure.append(cube("Side wall cabinet inset", (side * 18.84, y, .72),
                              (.09, .92, .44), foundry_black, bevel=.02))
        lights.append(cube("Side wall cabinet status", (side * 18.78, y, .50),
                           (.025, .58, .04), cyan if side < 0 else orange, bevel=.004))
    for y in (-12.6, -4.2, 4.2, 12.6):
        structure.append(cube("Side vertical service trunk", (side * 19.42, y, 2.64),
                              (.24, .36, 4.62), gunmetal, bevel=.035))
        structure.append(cube("Side service trunk cap", (side * 19.30, y, 4.92),
                              (.48, .86, .16), brass, bevel=.02))

# A second set of suspended technical pods increases ceiling richness without
# putting heavy geometry directly above the board centre.
for angle_deg in (22.5, 67.5, 112.5, 157.5, 202.5, 247.5, 292.5, 337.5):
    a = math.radians(angle_deg)
    center = Vector((20.35 * math.cos(a), 20.35 * math.sin(a), 5.95))
    structure.append(cube("Suspended tech pod", center, (1.25, .68, .52), blue_steel, a, bevel=.04))
    lights.append(cube("Suspended tech pod light", center + Vector((0.0, 0.0, -.22)),
                       (.72, .08, .05), warm, a, bevel=.006))
    p_top = center + Vector((0.0, 0.0, .62))
    p_anchor = Vector((20.9 * math.cos(a), 20.9 * math.sin(a), 6.82))
    structure.append(beam_between("Tech pod hanger", p_top, p_anchor, .07, gunmetal, .012))

# Small front-row standing rails make the front/rear aprons feel intentionally
# spectator-facing instead of blank dead space.
for y in (-16.2, 16.2):
    structure.append(cube("Apron standing rail", (0.0, y, FLOOR_Z_M + .82), (11.4, .08, .09), gunmetal, bevel=.015))
    for x in (-5.1, -2.55, 0.0, 2.55, 5.1):
        structure.append(cube("Apron standing post", (x, y, FLOOR_Z_M + .44), (.08, .08, .84), gunmetal, bevel=.012))
    lights.append(cube("Apron rail light", (0.0, y + (.08 if y < 0 else -.08), FLOOR_Z_M + .88),
                       (8.4, .03, .04), warm, bevel=.005))

# -----------------------------------------------------------------------------
# ENCLOSED VENUE ENRICHMENT — a second authored pass of practical fixtures,
# maintenance infrastructure, and close-range fabrication detail. All tall
# elements remain beyond the protected gameplay-camera orbit.
# -----------------------------------------------------------------------------

# Raised maintenance walks run behind the arena-facing wall equipment. Their
# layered deck, toe rail, posts, and underslung practicals give the enclosed
# front and rear walls useful depth when viewed from the board.
for side in (-1, 1):
    y = side * 18.82
    structure.append(cube("Long maintenance walk", (0.0, y, 4.82),
                          (31.8, .62, .16), blue_steel, bevel=.035))
    structure.append(cube("Maintenance walk toe rail", (0.0, y - side * .38, 5.02),
                          (31.2, .08, .34), gunmetal, bevel=.018))
    structure.append(cube("Maintenance walk upper rail", (0.0, y - side * .48, 5.68),
                          (31.2, .07, .08), light_composite, bevel=.014))
    for x in (-14.8, -11.1, -7.4, -3.7, 0.0, 3.7, 7.4, 11.1, 14.8):
        structure.append(cube("Maintenance walk post", (x, y - side * .48, 5.34),
                              (.075, .075, .72), gunmetal, bevel=.012))
    for x in (-12.8, -6.4, 0.0, 6.4, 12.8):
        light_color = cyan if x < 0 else orange if x > 0 else warm
        lights.append(cube("Maintenance walk practical", (x, y - side * .50, 4.68),
                           (1.65, .045, .07), light_color, bevel=.008))

# Wall-facing floodlight banks provide a clear ceiling rhythm and make the
# light-colored composite visible in-game without filling the centre overhead.
for angle_deg in range(0, 360, 30):
    a = math.radians(angle_deg)
    radial = Vector((math.cos(a), math.sin(a), 0.0))
    tangent = Vector((-math.sin(a), math.cos(a), 0.0))
    center = radial * 19.25 + Vector((0.0, 0.0, 5.92))
    structure.append(cube("Perimeter floodlight housing", center,
                          (.52, 1.42, .34), gunmetal, a, bevel=.055))
    for lamp_offset in (-.43, 0.0, .43):
        lamp_position = center - radial * .29 + tangent * lamp_offset + Vector((0.0, 0.0, -.04))
        lights.append(cube("Perimeter floodlight lens", lamp_position,
                           (.055, .29, .16), light_composite, a, bevel=.025))
    structure.append(beam_between("Floodlight hanger", center + Vector((0.0, 0.0, .14)),
                                  radial * 19.72 + Vector((0.0, 0.0, 6.68)),
                                  .075, blue_steel, .012))

# Segmented clerestory strips visually lift each wall. Warm separators keep the
# pale panels from reading as one uninterrupted bright band.
for side in (-1, 1):
    y = side * 18.48
    for index, x in enumerate((-13.8, -9.2, -4.6, 0.0, 4.6, 9.2, 13.8)):
        structure.append(cube("Front rear clerestory frame", (x, y, 4.17),
                              (3.62, .15, .54), gunmetal, bevel=.035))
        structure.append(cube("Front rear clerestory panel", (x, y - side * .09, 4.17),
                              (3.18, .035, .34), light_composite, bevel=.022))
        lights.append(cube("Front rear clerestory marker", (x, y - side * .12, 3.91),
                           (1.48, .025, .035), cyan if index < 3 else orange if index > 3 else warm,
                           bevel=.005))
for side in (-1, 1):
    x = side * 19.68
    for index, y in enumerate((-13.0, -8.65, -4.3, 0.0, 4.3, 8.65, 13.0)):
        structure.append(cube("Side clerestory frame", (x, y, 4.12),
                              (.15, 3.35, .50), gunmetal, bevel=.035))
        structure.append(cube("Side clerestory panel", (x - side * .09, y, 4.12),
                              (.035, 2.92, .30), light_composite, bevel=.022))
        lights.append(cube("Side clerestory marker", (x - side * .12, y, 3.88),
                           (.025, 1.25, .035), cyan if side < 0 else orange, bevel=.005))

# Low drain channels and numbered-looking brass tabs give the floor a finer
# manufacturing scale. These are deliberately shallow and cannot obstruct the
# free-camera envelope.
for side in (-1, 1):
    for offset in (-12.4, -6.2, 0.0, 6.2, 12.4):
        structure.append(cube("Apron drain frame", (offset, side * 14.75, FLOOR_Z_M + .105),
                              (2.45, .42, .045), gunmetal, bevel=.018))
        for slot in (-.78, -.39, 0.0, .39, .78):
            structure.append(cube("Apron drain slot", (offset + slot, side * 14.75, FLOOR_Z_M + .132),
                                  (.08, .30, .018), foundry_black, bevel=.006))
        structure.append(cube("Apron index tab", (offset, side * 14.49, FLOOR_Z_M + .142),
                              (.54, .08, .025), brass, bevel=.006))

# Four high observation pods anchor the corners of the complete enclosure and
# echo the square playfield without duplicating the existing score pylons.
for pod_index, (sx, sy) in enumerate(((-1, -1), (1, -1), (1, 1), (-1, 1))):
    angle = math.atan2(sy, sx)
    radius = 25.55
    structure.append(gallery_cube("Corner observation pod", angle, radius, 0.0,
                                  (2.45, 1.22, 1.18), 4.72, foundry_black, .09))
    structure.append(gallery_cube("Corner observation window", angle, radius - .64, 0.0,
                                  (.06, 1.82, .48), 5.05, light_composite, .035))
    structure.append(gallery_cube("Corner observation brow", angle, radius - .72, 0.0,
                                  (.44, 2.28, .14), 5.72, gunmetal, .025))
    pod_light = cyan if sx < 0 else orange
    lights.append(gallery_cube("Corner observation status", angle, radius - .69, 0.0,
                               (.035, 1.24, .055), 4.91, pod_light, .006))


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
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=.004)
    bpy.ops.object.mode_set(mode="OBJECT")
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


area_light("Foundry key", (-7.5, -8.0, 13.0), 2400, 8.0, (.72, .86, 1.0))
area_light("Foundry warm fill", (8.0, -2.0, 9.0), 1650, 6.0, (1.0, .70, .38))
area_light("Board softbox", (0, 0, 15.0), 2700, 9.0, (.88, .95, 1.0))
area_light("Rear architecture wash", (0, 13.5, 10.5), 1850, 8.0, (.64, .82, 1.0))
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
scene.world.color = (.015, .024, .032)
try:
    scene.view_settings.look = "AgX - Medium High Contrast"
except TypeError:
    pass

(ROOT / "renders").mkdir(exist_ok=True)
scene.render.filepath = str(ROOT / "renders" / "bob_stadium_preview.png")
manifest = {
    "concept": "Pocket Foundry Enclosed",
    "board_outer_span_cm": BOARD_OUTER_HALF_M * 200.0,
    "centre_opening_cm": DECK_INNER_HALF_M * 200.0,
    "outer_dimensions_cm": [DECK_OUTER_X_M * 200.0, DECK_OUTER_Y_M * 200.0],
    "floor_z_cm": FLOOR_Z_M * 100.0,
    "wall_top_z_cm": WALL_TOP_M * 100.0,
    "gameplay_camera_radius_cm": GAMEPLAY_CAMERA_RADIUS_M * 100.0,
    "nearest_tall_frame_cm": NEAREST_TALL_FRAME_M * 100.0,
    "camera_clearance_cm": (NEAREST_TALL_FRAME_M - GAMEPLAY_CAMERA_RADIUS_M) * 100.0,
    "enclosure_sides": 4,
    "detail_pass": "enclosed_venue_enrichment",
    "exports": [structure_asset.name, lights_asset.name],
}
(ROOT / "manifest.json").write_text(json.dumps(manifest, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "BOBPocketFoundry.blend"))
bpy.ops.render.render(write_still=True)
print("FLICK_BOB_STADIUM_GENERATION_COMPLETE", json.dumps(manifest))
