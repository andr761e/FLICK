"""Unreal commandlet: report content assets with no registry or runtime referencers."""

import json
import re
from pathlib import Path

import unreal as u


project = Path(u.Paths.project_dir())
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()

dependency_options = u.AssetRegistryDependencyOptions()
for property_name in (
    "include_hard_package_references",
    "include_soft_package_references",
    "include_searchable_names",
    "include_soft_management_references",
    "include_hard_management_references",
):
    dependency_options.set_editor_property(property_name, True)

runtime_paths = set()
runtime_patterns = set()
authoring_paths = set()
asset_pattern = re.compile(r"/Game/[A-Za-z0-9_./%+-]+")


def collect_paths(roots, extensions, exact_paths, format_patterns=None):
    """Collect Unreal paths, retaining printf-style runtime path patterns."""
    for root in roots:
        if not root.exists():
            continue
        files = root.glob("*") if root == project else root.rglob("*")
        for path in files:
            if not path.is_file() or path.suffix.lower() not in extensions:
                continue
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except OSError:
                continue
            for match in asset_pattern.findall(text):
                package_path = match.split(".", 1)[0].rstrip("/")
                if "%" in package_path and format_patterns is not None:
                    expression = re.escape(package_path)
                    expression = re.sub(r"%[0-9]*[sd]", r"[^/]+", expression)
                    format_patterns.add("^" + expression + "$")
                else:
                    exact_paths.add(package_path)


collect_paths(
    (project / "Source", project / "Config", project),
    {".cpp", ".h", ".ini", ".uproject", ".cs"},
    runtime_paths,
    runtime_patterns,
)
collect_paths(
    (project / "Tools", project / "AssetDevelopment"),
    {".py", ".md"},
    authoring_paths,
)
compiled_runtime_patterns = tuple(re.compile(value) for value in runtime_patterns)


def referenced_by_runtime(package_name):
    return package_name in runtime_paths or any(
        pattern.fullmatch(package_name) for pattern in compiled_runtime_patterns
    )


rows = []
for asset in registry.get_assets_by_path("/Game", recursive=True):
    package_name = str(asset.package_name)
    referencers = sorted(
        str(value) for value in registry.get_referencers(package_name, dependency_options)
    )
    runtime_reference = referenced_by_runtime(package_name)
    rows.append({
        "package": package_name,
        "asset_class": str(asset.asset_class_path.asset_name),
        "registry_referencers": referencers,
        "referenced_at_runtime": runtime_reference,
        "referenced_by_authoring_tools": package_name in authoring_paths,
        "candidate_unused": (
            not referencers
            and not runtime_reference
            and package_name != "/Game/FLICK"
        ),
    })

report = {
    "asset_count": len(rows),
    "runtime_asset_paths": sorted(runtime_paths),
    "runtime_asset_patterns": sorted(runtime_patterns),
    "authoring_asset_paths": sorted(authoring_paths),
    "candidate_unused": [row for row in rows if row["candidate_unused"]],
    "assets": rows,
}
output = project / "Saved" / "FlickAssetAudit.json"
output.write_text(json.dumps(report, indent=2), encoding="utf-8")
u.log(
    f"FLICK_ASSET_AUDIT_COMPLETE assets={len(rows)} "
    f"candidates={len(report['candidate_unused'])}"
)
