"""One-command Unreal import pass for FLICK's authored FBX presentation assets."""

from pathlib import Path
import runpy

import unreal as u


project = Path(u.Paths.project_dir())
tools = project / "Tools"
importers = (
    "Import-FlickBlueStandardPrototype.py",
    "Import-FlickHighDetailPucks.py",
    "Import-FlickArena.py",
    "Import-FlickStadium.py",
)

for importer in importers:
    path = tools / importer
    if not path.is_file():
        raise RuntimeError("FLICK asset importer is missing: " + str(path))
    u.log("FLICK_ASSET_UPDATE_START: " + importer)
    runpy.run_path(str(path), run_name="__main__")
    u.log("FLICK_ASSET_UPDATE_FINISH: " + importer)

u.EditorAssetLibrary.save_directory("/Game/TestArena", only_if_is_dirty=True, recursive=True)
u.log("FLICK_ASSET_UPDATE_COMPLETE")
