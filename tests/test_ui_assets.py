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
assert not list(folder.glob('ArmorDecorations*'))
assert not list(folder.glob('GenericTrim*')) and not list(folder.glob('WardrobeBG*'))
assert not list(folder.glob('WardrobeFrame*'))
assert not list(folder.glob('Settings*'))
assert hashlib.sha256((folder / 'Main.blp').read_bytes()).hexdigest() == '1dcd62ccdc06f806bde7be4435f6e2f7f9589d1984ec2ec5075555db51321a3d'
# Verify original artwork byte-for-byte, separately from the newly generated shadows.
for prefix in ['ArmorSlots', 'ArmorShadow']:
    layer = Image.new('RGBA', (512, 512))
    for suffix, x, y in [('TL',0,0), ('TR',256,0), ('BL',0,256), ('BR',256,256)]:
        name = prefix + suffix + '.tga'
        with Image.open(folder / name) as part:
            assert part.size == (256, 256) and part.mode == 'RGBA'
            layer.paste(part, (x, y))
        if prefix == 'ArmorSlots':
            entry = next(e for e in manifest if e['texture'] == name)
            assert entry['restored_from'] == 'v3.4.34'
    # Preserve the original TGA's sub-1% alpha noise rather than editing the artwork.
    assert layer.getchannel('A').crop((150,150,350,350)).getextrema()[1] <= 1
ui = (root / 'addon/SaureksCloset/UI.lua').read_text()
assert 'armorShadowFrame' in ui and 'shadow:SetAlpha(.45)' in ui
assert 'ArmorDecorations.blp' not in ui
print('PASS: texture inventory, payloads, checksums, restored art and separate shadow layers')
