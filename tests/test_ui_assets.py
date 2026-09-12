"""Validate the shipped native UI texture headers, payloads and transparent center."""
from pathlib import Path
import hashlib
import json
import struct
from PIL import Image

root = Path(__file__).resolve().parents[1]
folder = root / 'addon/SaureksCloset/Textures'
manifest = json.loads((root / 'addon/SaureksCloset/ARTWORK.json').read_text())
assert {p.name for p in folder.iterdir()} == {entry['texture'] for entry in manifest}
for entry in manifest:
    p = folder / entry['texture'];data = p.read_bytes()
    assert len(data) == entry['bytes']
    if entry.get('sha256'):
        assert hashlib.sha256(data).hexdigest() == entry['sha256']
    with Image.open(p) as image:
        image.load()
        assert list(image.size) == entry['size']
        if p.suffix == '.blp':
            magic, version, encoding, alpha, alpha_encoding, mips, w, h = struct.unpack_from('<4sI4BII', data)
            assert magic == b'BLP2' and version == 1 and encoding == 2 and mips == 0
            assert (alpha, alpha_encoding) in ((0, 0), (8, 7))
            assert w & (w-1) == 0 and h & (h-1) == 0
            offsets = struct.unpack_from('<16I', data, 20)
            lengths = struct.unpack_from('<16I', data, 84)
            assert offsets[0] == 1172 and not any(offsets[1:]) and not any(lengths[1:])
            assert lengths[0] == w*h//(1 if alpha else 2)
            assert offsets[0] + lengths[0] == len(data)
            assert lengths[0] == entry['gpu_base_bytes']
        if p.name == 'ArmorDecorations.blp':
            assert image.convert('RGBA').getchannel('A').crop((150,100,350,440)).getextrema() == (0,0)
assert not list(folder.glob('ArmorShadow*')) and not list(folder.glob('ArmorSlots*'))
ui = (root / 'addon/SaureksCloset/UI.lua').read_text()
assert 'ArmorShadow' not in ui and 'armorShadowFrame' not in ui
assert 'ArmorDecorations.blp' in ui
print('PASS: texture inventory, native BLP headers/block sizes, checksums, transparent preview center and removed separate shadows')
