"""Import only the nine base pucks and their two starter cosmetic appearances."""
from pathlib import Path
import runpy
import unreal as u

project = Path(u.Paths.project_dir())
u.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
geometry_only = '-FlickPuckGeometryOnly' in u.SystemLibrary.get_command_line()
importers = ("Import-FlickHighDetailPucks.py",) if geometry_only else (
    "Import-FlickBlueStandardPrototype.py", "Import-FlickHighDetailPucks.py")
for importer in importers:
    runpy.run_path(str(project / "Tools" / importer), run_name="__main__")
if not geometry_only:
    runpy.run_path(str(project / "Tools/Build-FlickPuckFinish.py"), run_name="__main__")
u.log("FLICK_PUCK_UPDATE_COMPLETE")
