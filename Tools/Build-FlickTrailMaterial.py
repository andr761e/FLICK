"""Rebuild the production trail shader through Unreal editor Python."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from FlickCosmeticMaterial import build

build('/Game/Cosmetics/Trails/M_PuckTrail_SoftV1')
