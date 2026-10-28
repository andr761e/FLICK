"""Rebuild the production knockout shader through Unreal editor Python."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from FlickCosmeticMaterial import build, build_facets

build('/Game/Cosmetics/Knockouts/M_PuckKnockout_SoftV1')

build('/Game/Cosmetics/Knockouts/M_PuckAtmosphere', atmosphere=True)
build_facets()
