"""Preserve original v3.4.34 armor art and rebuild only its separate shadows."""
from pathlib import Path
import argparse
import hashlib
import json
from PIL import Image
from build_slot_shadow import separate_shadow

root = Path(__file__).resolve().parents[1]


def build(source_directory=None):
    addon = root / 'addon/SaureksCloset'
    manifest = addon / 'ARTWORK.json'
    entries = json.loads(manifest.read_text())
    corners = [('TL', 0, 0), ('TR', 256, 0), ('BL', 0, 256), ('BR', 256, 256)]
    overlay = Image.new('RGBA', (512, 512))
    for suffix, x, y in corners:
        name = 'ArmorSlots' + suffix + '.tga'
        entry = next(e for e in entries if e['texture'] == name)
        path = addon / 'Textures' / name
        assert entry['restored_from'] == 'v3.4.34'
        assert hashlib.sha256(path.read_bytes()).hexdigest() == entry['sha256']
        with Image.open(path) as part:
            overlay.paste(part.convert('RGBA'), (x, y))
    shadow = separate_shadow(overlay)
    for suffix, x, y in corners:
        name = 'ArmorShadow' + suffix + '.tga'
        path = addon / 'Textures' / name
        shadow.crop((x, y, x+256, y+256)).save(path, compression=None)
        entry = next(e for e in entries if e['texture'] == name)
        entry.pop('restored_from', None)
        entry.update(source='tools/build_slot_shadow.py', generated=True,
                     source_sha256=hashlib.sha256((root / 'tools/build_slot_shadow.py').read_bytes()).hexdigest(),
                     layout='Separate contact and soft shadow of unmodified v3.4.34 armor art',
                     sha256=hashlib.sha256(path.read_bytes()).hexdigest(), bytes=path.stat().st_size)
    manifest.write_text(json.dumps(entries, indent=2) + '\n')
    print('Preserved original v3.4.34 armor art; rebuilt 4 separate shadow textures.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_directory', nargs='?', type=Path)
    parser.add_argument('--slots-only', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    build(args.source_directory)
