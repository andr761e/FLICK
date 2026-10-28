"""Rebuild the five shared cosmetic shaders, retaining production asset paths."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from FlickCosmeticMaterial import build, build_facets

for folder, family in [('Trails', 'Trail'), ('Spawns', 'Spawn'), ('Knockouts', 'Knockout')]:
    build(f'/Game/Cosmetics/{folder}/M_Puck{family}_SoftV1')
build('/Game/Cosmetics/Knockouts/M_PuckAtmosphere', atmosphere=True)
build_facets()
