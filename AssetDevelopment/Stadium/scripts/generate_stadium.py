"""Generate the high-detail visual stadium surrounding FLICK's Test Arena.

Adapted from the supplied arena_surroundings_blender_v4.py. The central arena
is not part of either export; it is imported only into the Blender preview.
"""

import json
import math
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parent.parent
ARENA_RADIUS_M = 6.50
ARENA_SURFACE_Z_M = 0.0
STADIUM_FLOOR_Z_M = -0.50
OUTER_FLOOR_RADIUS_M = 15.90
WALL_INNER_RADIUS_M = 15.55
WALL_OUTER_RADIUS_M = 16.15
WALL_TOP_Z_M = 4.65
SEGMENTS = 192

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
    if emission > 0.0:
        shader.inputs["Emission Color"].default_value = (*color, 1.0)
        shader.inputs["Emission Strength"].default_value = emission
    return value


# Cool-neutral architectural surfaces keep both team colours equally readable.
# The outer floor is deliberately the brightest large surface in the room.
porcelain = material("01_Porcelain_Composite", (.80, .84, .90), .08, .36)
concrete = material("02_Pale_Concrete", (.58, .62, .68), .04, .56)
graphite = material("03_Graphite_Inset", (.025, .038, .058), .46, .28)
metal = material("04_Brushed_Aluminium", (.50, .59, .68), .93, .22)
warm = material("05_Warm_Architectural_Light", (1.0, .86, .72), .05, .22, 3.0)
cyan = material("06_Team_Cyan_Light", (.0, .46, 1.0), .04, .18, 4.5)
orange = material("07_Team_Orange_Light", (1.0, .18, .015), .04, .18, 4.5)
dark = material("08_Deep_Recess", (.008, .014, .024), .30, .34)


def finish(obj, name, mat, bevel=0.0):
    obj.name = name
    obj.data.materials.append(mat)
    if bevel > 0.0:
        modifier = obj.modifiers.new("Edge chamfer", "BEVEL")
        modifier.width = bevel
        modifier.segments = 3
    for polygon in obj.data.polygons:
        polygon.use_smooth = True
    return obj


def cube(name, location, size, mat, rotation=0.0, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=(0.0, 0.0, rotation))
    obj = bpy.context.object
    obj.scale = Vector(size) * 0.5
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, name, mat, bevel)


def cylinder(name, radius, depth, z, mat, vertices=SEGMENTS, bevel=0.0):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=(0.0, 0.0, z))
    return finish(bpy.context.object, name, mat, bevel)


def sector(name, inner, outer, start_deg, end_deg, z0, z1, mat, segments=40, bevel=0.0):
    start = math.radians(start_deg)
    end = math.radians(end_deg)
    if end <= start:
        end += math.tau
    vertices = []
    faces = []
    for index in range(segments + 1):
        angle = start + (end - start) * index / segments
        c, s = math.cos(angle), math.sin(angle)
        vertices.extend(((inner*c, inner*s, z0), (outer*c, outer*s, z0),
                         (inner*c, inner*s, z1), (outer*c, outer*s, z1)))
    for index in range(segments):
        a, b = index * 4, (index + 1) * 4
        faces.extend(((a, b+1, a+1), (a, b, b+1), (a+2, a+3, b+3),
                      (a+2, b+3, b+2), (a, a+2, b+2), (a, b+2, b),
                      (a+1, b+1, b+3), (a+1, b+3, a+3)))
    faces.extend(((0, 1, 3), (0, 3, 2)))
    end_index = segments * 4
    faces.extend(((end_index, end_index+3, end_index+1),
                  (end_index, end_index+2, end_index+3)))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    return finish(obj, name, mat, bevel)


def ring(name, inner, outer, z0, z1, mat, bevel=0.0):
    return sector(name, inner, outer, 0.0, 360.0, z0, z1, mat, SEGMENTS, bevel)


def radial_cube(name, angle_deg, radius, tangential_width, radial_depth, z, height, mat, bevel=0.0):
    angle = math.radians(angle_deg)
    return cube(name, (radius*math.cos(angle), radius*math.sin(angle), z + height*0.5),
                (radial_depth, tangential_width, height), mat, angle, bevel)


structure = []
lights = []

# Pale architectural floor, recessed arena socket and concentric machining lines.
structure.append(ring("Main stadium floor", ARENA_RADIUS_M + .03, OUTER_FLOOR_RADIUS_M,
                      STADIUM_FLOOR_Z_M - .16, STADIUM_FLOOR_Z_M, porcelain, .018))
structure.append(ring("Arena shadow reveal", ARENA_RADIUS_M + .03, ARENA_RADIUS_M + .16,
                      STADIUM_FLOOR_Z_M - .02, STADIUM_FLOOR_Z_M + .025, dark, .008))
structure.append(ring("Outer floor border", 15.15, 15.34, STADIUM_FLOOR_Z_M,
                      STADIUM_FLOOR_Z_M + .035, metal, .008))
for radius in (9.35, 12.25, 14.75):
    structure.append(ring("Concentric floor seam", radius, radius + .018,
                          STADIUM_FLOOR_Z_M + .004, STADIUM_FLOOR_Z_M + .010, graphite))
for angle_deg in range(0, 360, 15):
    angle = math.radians(angle_deg)
    inner, outer = ARENA_RADIUS_M + .20, 15.12
    midpoint = (inner + outer) * .5
    structure.append(radial_cube("Radial floor seam", angle_deg, midpoint, .012,
                                 outer - inner, STADIUM_FLOOR_Z_M + .003, .008, graphite))

# Symmetrical three-tier spectator benches and inset aisle gaps.
bench_specs = ((18, 70), (110, 162), (198, 250), (290, 342))
for group, (start, end) in enumerate(bench_specs):
    for tier in range(3):
        inner = 11.75 + tier * .52
        outer = 14.55 - tier * .18
        z0 = STADIUM_FLOOR_Z_M + tier * .25
        structure.append(sector(f"Bench {group} tier {tier}", inner, outer, start, end,
                                z0, z0 + .23, concrete, 48, .015))
        lights.append(sector(f"Bench {group} light {tier}", inner + .04, outer - .06,
                             start + .7, end - .7, z0 + .235, z0 + .265, warm, 48))
    for aisle_angle in (start - 2.5, end + 2.5):
        for step in range(5):
            radius = 12.0 + step * .47
            structure.append(radial_cube(f"Aisle {group} step {step}", aisle_angle, radius,
                                         .92, .46, STADIUM_FLOOR_Z_M, .11 * (step + 1), porcelain, .01))
            lights.append(radial_cube(f"Aisle {group} light {step}", aisle_angle, radius - .21,
                                      .62, .025, STADIUM_FLOOR_Z_M + .105 * (step + 1), .025, warm, .003))

# Low guardrails remain open so they frame rather than hide gameplay.
for rail_index, (start, end) in enumerate(((200, 252), (288, 340), (20, 62), (118, 160))):
    for post_index in range(7):
        angle = start + (end - start) * post_index / 6
        structure.append(radial_cube(f"Rail {rail_index} post {post_index}", angle, 14.78,
                                     .055, .055, STADIUM_FLOOR_Z_M + .73, .72, graphite, .004))
    structure.append(sector(f"Rail {rail_index} crown", 14.745, 14.815, start, end,
                            STADIUM_FLOOR_Z_M + 1.42, STADIUM_FLOOR_Z_M + 1.48, metal, 64, .008))

# Segmented wall shell, inset bays, metallic buttresses and ventilation details.
structure.append(ring("Wall foundation", WALL_INNER_RADIUS_M, WALL_OUTER_RADIUS_M,
                      STADIUM_FLOOR_Z_M - .12, STADIUM_FLOOR_Z_M + .34, graphite, .015))
structure.append(ring("Main wall shell", WALL_INNER_RADIUS_M, WALL_OUTER_RADIUS_M,
                      STADIUM_FLOOR_Z_M + .34, WALL_TOP_Z_M, concrete, .018))
structure.append(ring("Upper soffit", 15.18, WALL_OUTER_RADIUS_M, 3.72, 4.18, graphite, .014))
structure.append(ring("Wall crown", 15.40, WALL_OUTER_RADIUS_M + .08,
                      WALL_TOP_Z_M, WALL_TOP_Z_M + .22, metal, .012))
for bay in range(24):
    center = bay * 15.0
    structure.append(sector(f"Wall inset {bay}", 15.46, 15.545, center - 5.4, center + 5.4,
                            .56, 2.72, graphite, 16, .008))
    structure.append(sector(f"Wall upper panel {bay}", 15.47, 15.545, center - 5.2, center + 5.2,
                            2.88, 3.52, porcelain, 14, .006))
    if bay % 2 == 0:
        structure.append(sector(f"Wall buttress {bay}", 15.34, 16.22, center - 1.35, center + 1.35,
                                STADIUM_FLOOR_Z_M, WALL_TOP_Z_M + .10, porcelain, 8, .012))
    for vent in (-2.8, 0.0, 2.8):
        structure.append(sector(f"Vent {bay}", 15.43, 15.455, center + vent - .65, center + vent + .65,
                                3.12, 3.20, dark, 4, .002))

# Four portal compositions give every camera heading an authored focal point.
for portal_index, angle_deg in enumerate((0, 90, 180, 270)):
    structure.append(radial_cube(f"Portal {portal_index} surround", angle_deg, 15.35,
                                 3.35, .36, STADIUM_FLOOR_Z_M, 3.55, concrete, .025))
    structure.append(radial_cube(f"Portal {portal_index} recess", angle_deg, 15.14,
                                 2.45, .10, STADIUM_FLOOR_Z_M + .05, 2.75, dark, .018))
    structure.append(radial_cube(f"Portal {portal_index} door", angle_deg, 15.07,
                                 1.70, .06, STADIUM_FLOOR_Z_M + .08, 2.34, graphite, .012))
    for side in (-1, 1):
        angle = math.radians(angle_deg)
        tangent = Vector((-math.sin(angle), math.cos(angle), 0.0))
        center = Vector((15.02*math.cos(angle), 15.02*math.sin(angle), 1.02)) + tangent * (side * 1.18)
        lights.append(cube(f"Portal {portal_index} light {side}", center,
                           (.04, .10, 2.75), warm, angle, .004))

# Localized architectural/team illumination, kept as a separate export.
lights.append(ring("Arena pedestal light", ARENA_RADIUS_M + .12, ARENA_RADIUS_M + .18,
                   STADIUM_FLOOR_Z_M + .03, STADIUM_FLOOR_Z_M + .075, warm, .006))
lights.append(ring("Lower wall cove", 15.49, 15.535, .02, .12, warm))
lights.append(ring("Upper wall cove", 15.47, 15.535, 3.48, 3.58, warm))
for name, angle_deg, light_material in (("Blue left", 135, cyan), ("Blue front", 225, cyan),
                                         ("Orange right", 45, orange), ("Orange front", 315, orange)):
    lights.append(radial_cube(name, angle_deg, 15.40, .15, .07, .18, 3.24, light_material, .004))


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
    scene.cursor.location = (0.0, 0.0, 0.0)
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
        object_types={"MESH"}, add_leaf_bones=False, axis_forward="-Y", axis_up="Z", mesh_smooth_type="FACE")
    bpy.ops.export_scene.gltf(filepath=str(export_dir / f"{obj.name}.glb"), use_selection=True, export_format="GLB")


structure_asset = join_asset("SM_TestStadium_Structure", structure)
lights_asset = join_asset("SM_TestStadium_Lights", lights)
for asset in (structure_asset, lights_asset):
    export_asset(asset)
    asset.hide_render = True

# Preview the stadium with the current arena art, without including it in exports.
arena_preview_path = ROOT.parent / "Arena" / "exports" / "SM_TestArena_Static.glb"
if arena_preview_path.is_file():
    bpy.ops.import_scene.gltf(filepath=str(arena_preview_path))

preview_collection = bpy.data.collections.new("STADIUM_PREVIEW")
scene.collection.children.link(preview_collection)
for asset in (structure_asset, lights_asset):
    copy = asset.copy()
    copy.data = asset.data.copy()
    copy.hide_render = False
    preview_collection.objects.link(copy)

def area_light(name, location, energy, size, color):
    data = bpy.data.lights.new(name, "AREA")
    data.energy = energy
    data.shape = "DISK"
    data.size = size
    data.color = color
    obj = bpy.data.objects.new(name, data)
    scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (Vector((0.0, 0.0, 0.0)) - obj.location).to_track_quat("-Z", "Y").to_euler()

area_light("Neutral key", (-7.0, -5.0, 11.0), 1500, 7.0, (.82, .90, 1.0))
area_light("Warm fill", (8.0, -2.0, 7.0), 950, 5.0, (1.0, .82, .68))
area_light("Top softbox", (0.0, 0.0, 14.0), 1800, 9.0, (.92, .96, 1.0))
bpy.ops.object.camera_add(location=(0.0, -15.0, 10.8))
camera = bpy.context.object
camera.rotation_euler = (Vector((0.0, 0.0, -.05)) - camera.location).to_track_quat("-Z", "Y").to_euler()
camera.data.lens = 31
scene.camera = camera
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = 1920
scene.render.resolution_y = 1080
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.world.color = (.012, .018, .03)
try:
    scene.view_settings.look = "AgX - Medium High Contrast"
except TypeError:
    pass

(ROOT / "renders").mkdir(exist_ok=True)
scene.render.filepath = str(ROOT / "renders" / "test_stadium_preview.png")
manifest = {
    "arena_radius_cm": ARENA_RADIUS_M * 100.0,
    "arena_surface_z_cm": ARENA_SURFACE_Z_M * 100.0,
    "stadium_floor_z_cm": STADIUM_FLOOR_Z_M * 100.0,
    "outer_floor_radius_cm": OUTER_FLOOR_RADIUS_M * 100.0,
    "wall_inner_radius_cm": WALL_INNER_RADIUS_M * 100.0,
    "wall_top_z_cm": WALL_TOP_Z_M * 100.0,
    "exports": [structure_asset.name, lights_asset.name],
}
(ROOT / "manifest.json").write_text(json.dumps(manifest, indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT / "TestStadium.blend"))
bpy.ops.render.render(write_still=True)
print("FLICK_TEST_STADIUM_GENERATION_COMPLETE", json.dumps(manifest))
