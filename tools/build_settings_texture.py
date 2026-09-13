"""Verify Settings shares the wardrobe background; no separate artwork is generated."""
from pathlib import Path
import argparse
import hashlib
import json

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('image', nargs='?', type=Path, help=argparse.SUPPRESS)
parser.parse_args()
addon = root / 'addon/SaureksCloset'
entries = json.loads((addon / 'ARTWORK.json').read_text())
entry = next(e for e in entries if e['texture'] == 'Main.blp')
assert hashlib.sha256((addon / 'Textures/Main.blp').read_bytes()).hexdigest() == entry['sha256']
assert not list((addon / 'Textures').glob('Settings*'))
print('Settings shares the existing wardrobe background; no texture rebuilt.')
