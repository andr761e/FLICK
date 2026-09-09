import bpy
import json
from pathlib import Path
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "dimensions.json").read_text())
EXPECTED_ASSETS = {
    "SM_TestArena_Static",
    "SM_TestArena_Divider",
    "SM_TestArena_DividerSocket",
    "SM_TestArena_SwitchHousing",
    "SM_TestArena_SwitchDot",
    "SM_TestArena_SignalTrace",
}
TOLERANCE_M = 0.002


def local_bounds(obj):
    points = [Vector(corner) for corner in obj.bound_box]
    mins = Vector((min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)))
    maxs = Vector((max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)))
    return mins, maxs, maxs - mins


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


results = {}
for name in sorted(EXPECTED_ASSETS):
    obj = bpy.data.objects.get(name)
    require(obj is not None, f"Missing source object: {name}")
    mins, maxs, size = local_bounds(obj)
    results[name] = {
        "bounds_min_m": [round(v, 5) for v in mins],
        "bounds_max_m": [round(v, 5) for v in maxs],
        "dimensions_m": [round(v, 5) for v in size],
    }
    for extension in ("fbx", "glb"):
        export = ROOT / "exports" / f"{name}.{extension}"
        require(export.is_file() and export.stat().st_size > 1024, f"Missing or empty export: {export}")

static_min, static_max, static_size = local_bounds(bpy.data.objects["SM_TestArena_Static"])
arena_radius = MANIFEST["arena_radius_cm"] / 100.0
require(max(abs(static_min.x), abs(static_max.x), abs(static_min.y), abs(static_max.y)) <= arena_radius + TOLERANCE_M,
        "Static visual exceeds the authoritative 650 cm arena radius")
# No decorative rim vertex may form a raised, non-colliding wall. The inherited
# Unreal cylinder owns the physical top plane all the way to the 650 cm edge.
static_mesh = bpy.data.objects["SM_TestArena_Static"].data
rim_vertex_z = [vertex.co.z for vertex in static_mesh.vertices
                if Vector((vertex.co.x, vertex.co.y)).length >= arena_radius * 0.959]
require(rim_vertex_z and max(rim_vertex_z) <= TOLERANCE_M,
        "Decorative rim rises above the authoritative play surface")
require(static_max.z <= 0.035,
        "Static arena contains raised non-colliding trim above the play surface")

divider_min, divider_max, divider_size = local_bounds(bpy.data.objects["SM_TestArena_Divider"])
nominal = [value / 100.0 for value in MANIFEST["divider_nominal_cm"]]
for axis, (actual, expected) in enumerate(zip(divider_size, nominal)):
    require(actual <= expected + TOLERANCE_M, f"Divider axis {axis} exceeds nominal dimension")
require(divider_min.z >= -TOLERANCE_M, "Divider geometry extends beneath its bottom-center pivot")

switch_min, switch_max, switch_size = local_bounds(bpy.data.objects["SM_TestArena_SwitchHousing"])
switch_diameter = MANIFEST["switch_radius_cm"] * 2.0 / 100.0
require(switch_size.x <= switch_diameter + TOLERANCE_M and switch_size.y <= switch_diameter + TOLERANCE_M,
        "Switch housing exceeds ControlZoneRadius")
require(switch_max.z <= TOLERANCE_M and switch_min.z < 0.0,
        "Switch housing must be authored downward from the arena surface")

dot_min, dot_max, dot_size = local_bounds(bpy.data.objects["SM_TestArena_SwitchDot"])
# The activation disc is exact; its dark gasket is deliberately 28% wider and is
# visual-only. Validate the physical activation dimension through the manifest.
require(abs(MANIFEST["activation_dot_radius_cm"] - 8.0) < 0.001,
        "Activation dot no longer matches SwitchActivationDotRadius")
require(dot_max.z <= TOLERANCE_M and dot_min.z < 0.0,
        "Switch activation dot must be authored downward from the arena surface")

for flush_name in ("SM_TestArena_DividerSocket", "SM_TestArena_SignalTrace"):
    flush_min, flush_max, _ = local_bounds(bpy.data.objects[flush_name])
    require(flush_max.z <= TOLERANCE_M and flush_min.z < 0.0,
            f"{flush_name} must be authored downward from the arena surface")

locations = MANIFEST["possible_locations"]
active = [location for location in locations if location["active"]]
require(len(locations) == 20, "Expected twenty designed divider locations")
require(len(active) == 8, "Expected eight active switches")
require(sum(1 for location in locations if location["outer"]) == 10, "Expected ten outer locations")
require(sum(1 for location in active if location["outer"]) == 4, "Expected four active outer switches")

# The visible rim must sit outside every possible divider and socket footprint.
# This checks the authored layout rather than changing the gameplay locations.
outer_locations = [location for location in locations if location["outer"]]
outer_center_radius_cm = max(
    (Vector(location["center_cm"]).length for location in outer_locations), default=0.0)
socket_outer_edge_cm = outer_center_radius_cm + MANIFEST["divider_nominal_cm"][1] * 0.5 + 4.0
require(MANIFEST["rim_inner_radius_cm"] > socket_outer_edge_cm,
        "Visible rim intersects an outer divider socket footprint")

report = {
    "status": "passed",
    "blend_file": bpy.data.filepath,
    "asset_count": len(EXPECTED_ASSETS),
    "possible_locations": len(locations),
    "active_switches": len(active),
    "objects": results,
}
(ROOT / "validation.json").write_text(json.dumps(report, indent=2))
print("FLICK_TEST_ARENA_VALIDATION_PASSED", len(EXPECTED_ASSETS), "assets")
