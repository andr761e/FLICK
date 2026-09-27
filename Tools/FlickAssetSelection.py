"""Source selection shared by the command-line asset importers."""

import json
import os
from pathlib import Path


_selection = os.environ.get("FLICK_ASSET_UPDATE_SELECTION")
changed = None if not _selection else set(json.loads(_selection))


def selected(project, source):
    """An unset selection preserves standalone importers' full-import behavior."""
    return changed is None or Path(source).relative_to(project).as_posix() in changed
