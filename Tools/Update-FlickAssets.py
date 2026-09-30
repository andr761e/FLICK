"""One-command Unreal import pass for FLICK's authored FBX presentation assets."""

from pathlib import Path
import importlib.util
import runpy

import unreal as u


project = Path(u.Paths.project_dir())
tools = project / "Tools"
spec = importlib.util.spec_from_file_location("FlickAssetSelection", tools / "FlickAssetSelection.py")
selection = importlib.util.module_from_spec(spec)
spec.loader.exec_module(selection)
importers = (
    "Import-FlickBlueStandardPrototype.py",
    "Import-FlickHighDetailPucks.py",
    "Import-FlickArena.py",
    "Import-FlickStadium.py",
    "Import-FlickBobArena.py",
    "Import-FlickBobStadium.py",
)

for importer in importers:
    path = tools / importer
    if not path.is_file():
        raise RuntimeError("FLICK asset importer is missing: " + str(path))
    # The Standard prototype provides shared materials used by the other pucks.
    # Import it first whenever any high-detail puck needs updating.
    prefixes = {
        "Import-FlickBlueStandardPrototype.py": ("AssetDevelopment/Pucks/ClassicBlue/exports/Standard.fbx", "Tools/Import-FlickBlueStandardPrototype.py"),
        "Import-FlickHighDetailPucks.py": ("AssetDevelopment/Pucks/ClassicBlue/", "AssetDevelopment/Pucks/ClassicOrange/", "Tools/Import-FlickHighDetailPucks.py", "Tools/Import-FlickBlueStandardPrototype.py"),
        "Import-FlickArena.py": ("AssetDevelopment/Arena/", "Tools/Import-FlickArena.py"),
        "Import-FlickStadium.py": ("AssetDevelopment/Stadium/", "Tools/Import-FlickStadium.py"),
        "Import-FlickBobArena.py": ("AssetDevelopment/BOB Arena/", "Tools/Import-FlickBobArena.py"),
        "Import-FlickBobStadium.py": ("AssetDevelopment/BOBStadium/", "Tools/Import-FlickBobStadium.py"),
    }[importer]
    if selection.changed is not None and not any(
            name.startswith(prefix) for name in selection.changed for prefix in prefixes):
        continue
    u.log("FLICK_ASSET_UPDATE_START: " + importer)
    force_group = selection.changed is None or "Tools/" + importer in selection.changed or any(
        name.endswith(".json") and name.startswith(prefix)
        and not name.endswith("/ClassicOrange/set.json")
        for name in selection.changed for prefix in prefixes)
    runpy.run_path(str(path), run_name="__main__", init_globals={
        "asset_selected": (lambda _project, _source: True) if force_group else selection.selected})
    u.log("FLICK_ASSET_UPDATE_FINISH: " + importer)

u.EditorAssetLibrary.save_directory("/Game/TestArena", only_if_is_dirty=True, recursive=True)
u.EditorAssetLibrary.save_directory("/Game/BOB", only_if_is_dirty=True, recursive=True)
u.log("FLICK_ASSET_UPDATE_COMPLETE")
