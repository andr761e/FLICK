"""Offline UI renders from the current optimized FBXs; never exports or modifies game meshes.

Blender --background --python Tools/Render-FlickPuckThumbnails.py -- [--only Standard] [--skin 0]
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

root = Path(__file__).resolve().parent.parent
finish_roles = json.loads((root / "AssetDevelopment/Pucks/finish.json").read_text())["roles"]
parser = argparse.ArgumentParser()
parser.add_argument("--only")
parser.add_argument("--skin", type=int)
parser.add_argument("--output", type=Path, help="Isolated authoring QA output; does not replace production portraits")
views = parser.add_mutually_exclusive_group()
views.add_argument("--presentation", action="store_true", help="High-quality lineup product stages; leaves existing transparent thumbnails unchanged")
views.add_argument("--locker", action="store_true", help="Square near-top-down finish cards for the locker")
views.add_argument("--hero", action="store_true", help="Premium lower-angle model portraits on a textured reflective stage")
views.add_argument("--profile-cutout", action="store_true", help="Transparent lower-angle portraits for the profile's illustrated arena stage")
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
product_stage = args.presentation or args.locker or args.hero or args.profile_cutout
names = ("Standard", "Heavy", "Striker", "Grippy", "Slider", "Blocker", "Compact", "Bouncer", "Toppler")
skins = ("ClassicBlue", "ClassicOrange", "Emerald", "Amethyst", "Crimson", "Amber")
output = root / "AssetDevelopment/UI/PuckThumbnails"
if args.presentation:
    output = output / "Presentation"
elif args.locker:
    output = output / "Locker"
elif args.hero:
    output = output / "Hero"
elif args.profile_cutout:
    output = output / "ProfileCutout"
if args.output:
    output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.samples = 48
if product_stage:
    scene.cycles.samples = 96
scene.cycles.use_denoising = True
try:
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "OPTIX"
    prefs.get_devices()
    for device in prefs.devices:
        device.use = device.type == "OPTIX"
    if any(device.use for device in prefs.devices):
        scene.cycles.device = "GPU"
except (RuntimeError, TypeError):
    pass
scene.render.resolution_x, scene.render.resolution_y = 512, 384
if args.presentation or args.hero or args.profile_cutout:
    scene.render.resolution_x, scene.render.resolution_y = 1024, 768
elif args.locker:
    scene.render.resolution_x, scene.render.resolution_y = 768, 768
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.render.film_transparent = True
if product_stage:
    scene.render.film_transparent = False
scene.world.use_nodes = True
scene.world.node_tree.nodes["Background"].inputs[0].default_value = (.06, .08, .11, 1)
scene.world.node_tree.nodes["Background"].inputs[1].default_value = .35
scene.view_settings.view_transform = "Standard"
scene.view_settings.exposure = -.9
if product_stage:
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.exposure = -0.9
    scene.world.node_tree.nodes["Background"].inputs[0].default_value = (.012, .023, .035, 1)
    scene.world.node_tree.nodes["Background"].inputs[1].default_value = .22
for position, power, size, color in [((-2, -2, 3), 180, 1.8, (.83, .92, 1)),
        ((2, .5, 2.5), 220, 1.5, (1, .86, .65)), ((0, 2, 2), 200, 1.4, (.55, .86, 1))]:
    data = bpy.data.lights.new("Thumbnail softbox", "AREA")
    data.energy, data.size, data.color = power, size, color
    light = bpy.data.objects.new(data.name, data)
    scene.collection.objects.link(light)
    light.location = position
    light.rotation_euler = (-light.location).to_track_quat("-Z", "Y").to_euler()
bpy.ops.object.camera_add(location=(1.15, -1.9, 1.75))
camera = bpy.context.object
camera.rotation_euler = (Vector((0, 0, .05)) - camera.location).to_track_quat("-Z", "Y").to_euler()
camera.data.type = "ORTHO"
camera.data.ortho_scale = 1.48
if product_stage:
    camera.location = (1.05, -1.9, 1.85)
    camera.rotation_euler = (Vector((0, 0, .065)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = 1.55
    for light in [item for item in scene.objects if item.type == "LIGHT"]:
        light.data.energy *= .32
        # Narrow softbox reflections reveal the metal instead of filling its
        # entire crown with one flat grey reflection. Offline fixtures only.
        light.data.shape = "RECTANGLE"
        light.data.size = .42
        light.data.size_y = 1.8
    # A real brushed-metal receiver supplies contact, emissive spill and
    # reflections in the offline render. It is never imported into the game.
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, -.001))
    floor = bpy.context.object
    floor.name = "Offline product stage only"
    material = bpy.data.materials.new("Offline brushed graphite")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    shader = nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = (.018, .031, .046, 1)
    shader.inputs["Metallic"].default_value = .72
    shader.inputs["Roughness"].default_value = .3
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 140
    noise.inputs["Detail"].default_value = 2
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = .12
    bump.inputs["Distance"].default_value = .002
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    floor.data.materials.append(material)
if args.locker:
    camera.location = (0, -.95, 2.6)
    camera.rotation_euler = (Vector((0, 0, .06)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = 1.5
if args.hero:
    scene.cycles.samples = 128
    camera.location = (.9, -1.9, 1.45)
    camera.rotation_euler = (Vector((0, 0, .08)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = 1.62
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.exposure = -.9
    shader.inputs["Base Color"].default_value = (.009, .019, .028, 1)
    shader.inputs["Metallic"].default_value = .82
    # Cross-grain, hairline wear and roughness breakup, all offline only.
    tex = nodes.new("ShaderNodeTexCoord")
    mapping = nodes.new("ShaderNodeVectorMath")
    mapping.operation = "MULTIPLY"
    mapping.inputs[1].default_value = (3, 18, 1)
    links.new(tex.outputs["Generated"], mapping.inputs[0])
    links.new(mapping.outputs["Vector"], noise.inputs["Vector"])
    # Object coordinates keep grain independent of the large floor dimensions.
    links.new(tex.outputs["Object"], mapping.inputs[0])
    noise.inputs["Scale"].default_value = 12
    bump.inputs["Strength"].default_value = .22
    bump.inputs["Distance"].default_value = .0014
    roughness = nodes.new("ShaderNodeMapRange")
    roughness.inputs["To Min"].default_value = .19
    roughness.inputs["To Max"].default_value = .32
    links.new(noise.outputs["Fac"], roughness.inputs["Value"])
    links.new(roughness.outputs["Result"], shader.inputs["Roughness"])
    for light in [item for item in scene.objects if item.type == "LIGHT"]:
        light.data.energy *= 1.1
    for position, power, color, width, length in [
            ((-1.3, .8, 1.2), 100, (1, .53, .16), .28, 2.2),
            ((1.1, .6, .8), 65, (.12, .7, 1), .16, 1.8)]:
        data = bpy.data.lights.new("Offline stage strip", "AREA")
        data.energy, data.color, data.shape = power, color, "RECTANGLE"
        data.size, data.size_y = width, length
        light = bpy.data.objects.new(data.name, data)
        scene.collection.objects.link(light)
        light.location = position
        light.rotation_euler = (-light.location).to_track_quat("-Z", "Y").to_euler()
if args.profile_cutout:
    scene.cycles.samples = 128
    scene.render.film_transparent = True
    floor.hide_render = True
    camera.location = (.65, -2, 1.18)
    camera.rotation_euler = (Vector((0, 0, .07)) - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.ortho_scale = 1.42
    scene.view_settings.exposure = -.7
scene.camera = camera
report = []

for name in names:
    if args.only and name != args.only:
        continue
    source = root / "AssetDevelopment/Pucks/ClassicBlue/exports" / (name + ".fbx")
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    bpy.ops.import_scene.fbx(filepath=str(source), use_custom_normals=True)
    meshes = [obj for obj in bpy.context.selected_objects if obj.type == "MESH"]
    if len(meshes) != 1:
        raise RuntimeError(name + ": expected one optimized mesh")
    obj = meshes[0]
    rest_transform = obj.matrix_world.copy()
    # Match the runtime center coating without changing/exporting the FBX.
    # The Standard center happens to share its material with the sidewall.
    for index, mat in enumerate(list(obj.data.materials)):
        if "graphite" not in mat.name.lower() or not finish_roles["Housing"].get("coat_center", 0):
            continue
        center_faces = [p for p in obj.data.polygons if p.material_index == index
                        and (rest_transform @ p.center).z > 0
                        and (rest_transform @ p.center).xy.length < .245 and p.normal.z > .8]
        if center_faces:
            coating = mat.copy()
            coating.name = mat.name + "_FaceCoating"
            coating_index = len(obj.data.materials)
            obj.data.materials.append(coating)
            for polygon in center_faces:
                polygon.material_index = coating_index
    bounds = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    center = Vector(((min(p.x for p in bounds) + max(p.x for p in bounds)) / 2,
                     (min(p.y for p in bounds) + max(p.y for p in bounds)) / 2,
                     min(p.z for p in bounds)))
    obj.location -= center
    for skin, palette_name in enumerate(skins):
        if args.skin is not None and skin != args.skin:
            continue
        palette_path = root / "AssetDevelopment/Pucks" / palette_name / "set.json"
        palette = json.loads(palette_path.read_text()) if palette_path.exists() else {}
        accent = palette.get("accent_color", [1, .18, .003] if skin == 1 else [.005, .65, 1])
        bases = palette.get("base_colors", {})
        for slot in obj.material_slots:
            mat = slot.material
            mat.use_nodes = True
            shader = mat.node_tree.nodes.get("Principled BSDF")
            token = mat.name.lower().replace("_", " ")
            role = "Diffuser" if "light diffuser" in token else "Emblem" if "center emblem" in token else \
                "Housing" if "graphite" in token else "Crown" if "circular" in token else \
                "Edges" if "machined" in token else "Core" if "recessed" in token else "Gasket"
            base = bases.get(role, {"Housing": [.025, .034, .044], "Crown": [.65, .72, .78],
                "Edges": [.45, .51, .57], "Core": [.008, .015, .025], "Gasket": [.003, .006, .009]}.get(role,
                [.125, .022, .002] if skin == 1 else [.004, .08, .125]))
            luminous = role in ("Diffuser", "Emblem")
            shader.inputs["Base Color"].default_value = (*base, 1)
            shader.inputs["Metallic"].default_value = .12 if luminous else .35 if role in ("Core", "Gasket") else .85
            shader.inputs["Roughness"].default_value = .25 if role in ("Crown", "Edges") else .42
            shader.inputs["Emission Color"].default_value = (*(v ** 1.35 for v in accent), 1)
            shader.inputs["Emission Strength"].default_value = 1.5 if luminous else 0
            if product_stage:
                shader.inputs["Roughness"].default_value = .22 if role in ("Crown", "Edges") else .29
                shader.inputs["Emission Strength"].default_value = 2.7 if luminous else 0
            if args.hero or args.profile_cutout:
                shader.inputs["Roughness"].default_value = .18 if role in ("Crown", "Edges") else .31
                shader.inputs["Emission Strength"].default_value = 2.7 if luminous else 0
            # The same optical hierarchy as the runtime puck parent; lighting
            # remains specific to the offline camera, never the geometry.
            finish = dict(finish_roles[role])
            if "_FaceCoating" in mat.name:
                for key in ["metallic", "roughness", "specular", "anisotropy"]:
                    finish[key] = finish_roles["Core"][key]
                finish["color_scale"] *= .5
            shader.inputs["Base Color"].default_value = (*(v * finish["color_scale"] for v in base), 1)
            shader.inputs["Metallic"].default_value = finish["metallic"]
            shader.inputs["Roughness"].default_value = finish["roughness"]
            # Blender's level is relative to the dielectric Fresnel response;
            # keep the same role-specific low-glare face as the runtime shader.
            shader.inputs["Specular IOR Level"].default_value = finish.get("specular", .5)
            shader.inputs["Anisotropic"].default_value = finish["anisotropy"]
            shader.inputs["Emission Strength"].default_value = 2.2 if luminous else 0
            if not luminous:
                # Subtle machining, not a noisy painted silver surface.
                nodes = mat.node_tree.nodes
                links = mat.node_tree.links
                grain = nodes.new("ShaderNodeTexNoise")
                grain.inputs["Scale"].default_value = 230
                grain.inputs["Detail"].default_value = 2
                micro = nodes.new("ShaderNodeBump")
                micro.inputs["Strength"].default_value = .055 * finish["detail"]
                micro.inputs["Distance"].default_value = .00025
                links.new(grain.outputs["Fac"], micro.inputs["Height"])
                links.new(micro.outputs["Normal"], shader.inputs["Normal"])
                roughness = nodes.new("ShaderNodeMapRange")
                roughness.inputs["To Min"].default_value = finish["roughness"] - .016 * finish["detail"]
                roughness.inputs["To Max"].default_value = finish["roughness"] + .016 * finish["detail"]
                links.new(grain.outputs["Fac"], roughness.inputs["Value"])
                links.new(roughness.outputs["Result"], shader.inputs["Roughness"])
        scene.render.filepath = str(output / (name + "_" + str(skin) + ".png"))
        bpy.ops.render.render(write_still=True)
        report.append({"archetype": name, "skin": skin, "source_sha256": source_hash})
        print("FLICK_PUCK_THUMBNAIL", name, skin, flush=True)
    bpy.data.objects.remove(obj, do_unlink=True)
    if hashlib.sha256(source.read_bytes()).hexdigest() != source_hash:
        raise RuntimeError("Source geometry unexpectedly changed: " + name)
(output / "renders.json").write_text(json.dumps(report, indent=2))
print("FLICK_PUCK_THUMBNAILS_COMPLETE", len(report), flush=True)
