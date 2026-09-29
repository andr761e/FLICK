"""Import only the nine base pucks and their two starter cosmetic appearances."""
from pathlib import Path
import runpy
import unreal as u

project = Path(u.Paths.project_dir())
u.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
for importer in ("Import-FlickBlueStandardPrototype.py", "Import-FlickHighDetailPucks.py"):
    runpy.run_path(str(project / "Tools" / importer), run_name="__main__")
u.log("FLICK_PUCK_UPDATE_COMPLETE")
