"""Validate the generated stadium's centre opening and production exports."""

import json
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent.parent
manifest = json.loads((ROOT / "manifest.json").read_text())
errors = []
for name in manifest["exports"]:
    if not (ROOT / "exports" / f"{name}.fbx").is_file():
        errors.append(f"Missing FBX export: {name}")
    if not (ROOT / "exports" / f"{name}.glb").is_file():
        errors.append(f"Missing GLB export: {name}")
    if bpy.data.objects.get(name) is None:
        errors.append(f"Missing Blender source object: {name}")
if manifest["arena_radius_cm"] < 650.0:
    errors.append("Stadium centre opening is smaller than the Test Arena")
if manifest["stadium_floor_z_cm"] >= manifest["arena_surface_z_cm"]:
    errors.append("Stadium floor must remain below the arena play surface")
report = {"status": "failed" if errors else "passed", "errors": errors, **manifest}
(ROOT / "validation.json").write_text(json.dumps(report, indent=2))
print("FLICK_TEST_STADIUM_VALIDATION", json.dumps(report))
if errors:
    raise RuntimeError("; ".join(errors))
