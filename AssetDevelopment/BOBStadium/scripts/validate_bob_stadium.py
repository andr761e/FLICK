"""Validate Pocket Foundry source objects, opening, and exports."""

import json
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parent.parent
manifest = json.loads((ROOT / "manifest.json").read_text())
errors = []
for name in manifest["exports"]:
    if bpy.data.objects.get(name) is None:
        errors.append("Missing Blender object: " + name)
    for extension in ("fbx", "glb"):
        if not (ROOT / "exports" / f"{name}.{extension}").is_file():
            errors.append(f"Missing {extension.upper()} export: {name}")
if manifest["centre_opening_cm"] < manifest["board_outer_span_cm"]:
    errors.append("Stadium centre opening intersects the BOB arena")
if manifest["floor_z_cm"] >= 0.0:
    errors.append("Surrounding deck must remain below the play surface")
if manifest["nearest_tall_frame_cm"] - manifest["gameplay_camera_radius_cm"] < 250.0:
    errors.append("Tall venue architecture must leave at least 250 cm around the gameplay camera orbit")
report = {"status": "failed" if errors else "passed", "errors": errors, **manifest}
(ROOT / "validation.json").write_text(json.dumps(report, indent=2))
print("FLICK_BOB_STADIUM_VALIDATION", json.dumps(report))
if errors:
    raise RuntimeError("; ".join(errors))
