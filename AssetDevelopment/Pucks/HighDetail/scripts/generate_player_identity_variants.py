"""Generate high-detail P1/P2/P3 puck variants for both team presentations.

The three player meshes are geometry variants; blue/orange continue to share
geometry in Unreal and differ only through the existing team-light material.
Separate Blue and Orange Blender packages are emitted for visual review.
"""
import bpy
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
OUTPUT_ROOT = ROOT.parent / "PlayerIdentity"
ARCHETYPES = ["Standard", "Toppler", "Bouncer", "Compact", "Blocker",
              "Slider", "Grippy", "Striker", "Heavy"]
IDENTITIES = {
    "P1": (0.92, 0.96, 1.00, 1.0),
    "P2": (1.00, 0.06, 0.72, 1.0),
    "P3": (0.58, 1.00, 0.02, 1.0),
}
TEAMS = {
    "Blue": ((0.005, 0.60, 0.83, 1.0), (0.0, 0.50, 1.0, 1.0)),
    "Orange": ((0.82, 0.075, 0.003, 1.0), (1.0, 0.18, 0.003, 1.0)),
}
SHOWCASE_POSITIONS = {
    "Standard": (-2.2, 1.2), "Toppler": (-1.1, 1.2), "Bouncer": (0.0, 1.2),
    "Compact": (1.1, 1.2), "Blocker": (2.2, 1.2), "Slider": (-1.65, 0.0),
    "Grippy": (-0.55, 0.0), "Striker": (0.55, 0.0), "Heavy": (1.65, 0.0),
}


def source_for(name):
    if name == "Standard":
        return ROOT / "source/standard/Standard.glb"
    return ROOT / "source/archetypes/models" / f"{name}.glb"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for collection in list(bpy.data.collections):
        if collection.name != "Collection":
            bpy.data.collections.remove(collection)
    for material in list(bpy.data.materials):
        bpy.data.materials.remove(material)


def set_principled_material(material, base_color, emission_color=None, strength=0.0):
    material.diffuse_color = base_color
    material.use_nodes = True
    shader = material.node_tree.nodes.get("Principled BSDF")
    if not shader:
        return
    shader.inputs["Base Color"].default_value = base_color
    if emission_color:
        emission_input = shader.inputs.get("Emission Color") or shader.inputs.get("Emission")
        if emission_input:
            emission_input.default_value = emission_color
        strength_input = shader.inputs.get("Emission Strength")
        if strength_input:
            strength_input.default_value = strength
    shader.inputs["Metallic"].default_value = 0.2
    shader.inputs["Roughness"].default_value = 0.24


def is_original_emblem(obj):
    return obj.type == "MESH" and any(
        slot.material and "center emblem" in slot.material.name.lower()
        for slot in obj.material_slots)


def add_identity_text(identity, color, z):
    material = bpy.data.materials.get(f"Player identity {identity}")
    if not material:
        material = bpy.data.materials.new(f"Player identity {identity}")
        set_principled_material(material, color, color, 7.0)

    bpy.ops.object.text_add(location=(0.0, 0.0, z - 0.0010))
    text = bpy.context.object
    text.name = f"{identity} top emblem"
    text.data.body = identity
    text.data.align_x = "CENTER"
    text.data.align_y = "CENTER"
    text.data.size = 0.135
    text.data.extrude = 0.00042
    text.data.bevel_depth = 0.00032
    text.data.bevel_resolution = 3
    text.data.resolution_u = 12
    text.data.materials.append(material)
    bpy.ops.object.convert(target="MESH")
    return bpy.context.object


def normalize_team_materials(objects, base_color, emissive_color):
    for obj in objects:
        for slot in obj.material_slots:
            material = slot.material
            if not material or "light diffuser" not in material.name.lower():
                continue
            # Blue exports are the runtime geometry source, so retain the slot
            # name used by Unreal even in the orange Blender review package.
            material.name = "Cyan light diffuser"
            set_principled_material(material, base_color, emissive_color, 6.0)


def build_puck(name, identity, identity_color, team_colors):
    before = set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(source_for(name)))
    imported = [obj for obj in bpy.context.scene.objects if obj not in before and obj.type == "MESH"]
    if not imported:
        raise RuntimeError(f"{name}: no mesh objects imported")

    maximum_z = max((obj.matrix_world @ vertex.co).z
                    for obj in imported for vertex in obj.data.vertices)
    original_emblems = [obj for obj in imported if is_original_emblem(obj)]
    if not original_emblems:
        raise RuntimeError(f"{name}: center emblem material was not found")
    imported = [obj for obj in imported if obj not in original_emblems]
    for obj in original_emblems:
        bpy.data.objects.remove(obj, do_unlink=True)
    normalize_team_materials(imported, *team_colors)
    imported.append(add_identity_text(identity, identity_color, maximum_z))

    bpy.ops.object.select_all(action="DESELECT")
    for obj in imported:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = imported[0]
    bpy.ops.object.convert(target="MESH")
    bpy.ops.object.join()
    puck = bpy.context.object
    puck.name = f"SM_Puck_{name}_{identity}_HighDetail"
    bpy.ops.object.material_slot_remove_unused()
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

    minimum_z = min(vertex.co.z for vertex in puck.data.vertices)
    maximum_z = max(vertex.co.z for vertex in puck.data.vertices)
    center_z = (minimum_z + maximum_z) * 0.5
    for vertex in puck.data.vertices:
        vertex.co.z -= center_z
    puck.data.update()
    return puck


def export_collection(identity, team, identity_color, team_colors):
    clear_scene()
    output = OUTPUT_ROOT / identity / team
    export_root = output / "exports"
    export_root.mkdir(parents=True, exist_ok=True)
    report = []
    pucks = []
    for name in ARCHETYPES:
        puck = build_puck(name, identity, identity_color, team_colors)
        pucks.append((name, puck))
        measured = [round(float(value) * 100.0, 3) for value in puck.dimensions]
        materials = [slot.material.name if slot.material else "" for slot in puck.material_slots]
        if len(materials) != 7 or f"Player identity {identity}" not in materials:
            raise RuntimeError(f"{name} {identity}: invalid material sections {materials}")

        bpy.ops.object.select_all(action="DESELECT")
        puck.select_set(True)
        bpy.context.view_layer.objects.active = puck
        bpy.ops.export_scene.fbx(
            filepath=str(export_root / f"{name}.fbx"), use_selection=True,
            object_types={"MESH"}, add_leaf_bones=False, axis_forward="-Y", axis_up="Z",
            mesh_smooth_type="FACE", use_tspace=False)
        report.append({"asset": name, "identity": identity, "team_preview": team,
                       "dimensions_cm": measured, "materials": materials})

    for name, puck in pucks:
        x, y = SHOWCASE_POSITIONS[name]
        puck.location = (x, y, 0.0)
    bpy.ops.wm.save_as_mainfile(filepath=str(output / f"{identity}_{team}_Pucks.blend"))
    (output / "manifest.json").write_text(json.dumps(report, indent=2))
    return report


all_reports = []
for identity, identity_color in IDENTITIES.items():
    for team, team_colors in TEAMS.items():
        all_reports.extend(export_collection(identity, team, identity_color, team_colors))

(OUTPUT_ROOT / "manifest.json").write_text(json.dumps(all_reports, indent=2))
print("FLICK_PLAYER_IDENTITY_VARIANTS_COMPLETE", len(all_reports))
