"""Rebuild the production spawn shader through Unreal editor Python."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from FlickCosmeticMaterial import build, build_facets

build('/Game/Cosmetics/Spawns/M_PuckSpawn_SoftV1')
build_facets()
