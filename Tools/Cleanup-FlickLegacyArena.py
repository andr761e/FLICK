"""Remove only the audited, superseded ClassicArena presentation assets."""
import runpy
from pathlib import Path
import unreal as u

project = Path(u.Paths.project_dir())
registry = u.AssetRegistryHelpers.get_asset_registry()
registry.wait_for_completion()
options = u.AssetRegistryDependencyOptions()
for key in ("include_hard_package_references", "include_soft_package_references",
            "include_searchable_names", "include_soft_management_references",
            "include_hard_management_references"):
    options.set_editor_property(key, True)

# Remove leaves first, then any materials orphaned by that removal. Never force
# deletion of a referenced asset, and never touch the active TestArena/BOB paths.
removed = []
while True:
    leaves = [str(asset.package_name) for asset in
              registry.get_assets_by_path("/Game/ClassicArena", recursive=True)
              if not registry.get_referencers(asset.package_name, options)]
    if not leaves:
        break
    for package in leaves:
        if not u.EditorAssetLibrary.delete_asset(package):
            raise RuntimeError("Could not remove legacy asset: " + package)
        removed.append(package)
    registry.scan_paths_synchronous(["/Game/ClassicArena"], force_rescan=True)
u.log("FLICK_LEGACY_CLEANUP_REMOVED: " + ", ".join(removed))
runpy.run_path(str(project / "Tools/Audit-FlickAssets.py"), run_name="__main__")
