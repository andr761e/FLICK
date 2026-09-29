"""Compatibility entry point: export the authored arena; never overwrite it."""
import runpy
from pathlib import Path

runpy.run_path(str(Path(__file__).with_name("export_bob_arena.py")), run_name="__main__")
