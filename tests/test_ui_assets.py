"""Validate the shipped native UI texture headers, payloads and transparent center."""
from pathlib import Path
import hashlib
import json
import struct
from PIL import Image

root = Path(__file__).resolve().parents[1]
folder = root / 'addon/SaureksCloset/Textures'
manifest = json.loads((root / 'addon/SaureksCloset/ARTWORK.json').read_text())
assert {p.name for p in folder.iterdir()} == {entry['texture'] for entry in manifest} | {'ASSETS-LICENSE'}
assert '../ASSETS-LICENSE' in (folder / 'ASSETS-LICENSE').read_text()
assert (folder.parent / 'ASSETS-LICENSE').read_bytes() == (root / 'ASSETS-LICENSE').read_bytes()
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
# Center the visible numeral, not the font's ascender/line box. Test both
# source pixels and the actual 16px control after texture filtering.
number_entry=next(entry for entry in manifest if entry['texture']=='SecondHatNumber.tga')
assert hashlib.sha256((root/number_entry['source']).read_bytes()).hexdigest()==number_entry['source_sha256']
with Image.open(folder/'SecondHatNumber.tga') as number:
    assert number.mode=='RGBA' and number.size==(32,32)
    assert number.getpixel((0,0))[3]==0
    for size in (32,16):
        alpha=number.resize((size,size),Image.Resampling.BILINEAR).getchannel('A')
        left,top,right,bottom=alpha.point(lambda v:255 if v>=16 else 0).getbbox()
        assert abs((left+right)/2-size/2)<=.5 and abs((top+bottom)/2-size/2)<=.5
    assert number.getpixel((16,16))[:3]==(210,210,204)
assert not list(folder.glob('ArmorDecorations*'))
assert not list(folder.glob('GenericTrim*')) and not list(folder.glob('WardrobeBG*'))
assert not list(folder.glob('Settings*'))
signature_entry = next(entry for entry in manifest if entry['texture'] == 'DonationSignature.tga')
signature_source = root / signature_entry['source']
assert hashlib.sha256(signature_source.read_bytes()).hexdigest() == signature_entry['source_sha256']
with Image.open(signature_source) as source, Image.open(folder / 'DonationSignature.tga') as signature:
    assert signature.mode == 'RGBA' and signature.size == (512, 256)
    assert signature.getchannel('A').getextrema() == (0, 255)
    expected = source.convert('RGBA').resize((480, 160), Image.Resampling.LANCZOS)
    assert signature.crop(signature_entry['content_rect']).tobytes() == expected.tobytes()
    assert signature.getchannel('A').crop((0, 0, 512, 48)).getbbox() is None
qr_entry = next(entry for entry in manifest if entry['texture'] == 'CashAppQR.tga')
assert qr_entry['qr_destination'] == 'https://cash.app/$saurek?qr=1'
assert hashlib.sha256((root / qr_entry['source']).read_bytes()).hexdigest() == qr_entry['source_sha256']
qr_data = (folder / 'CashAppQR.tga').read_bytes()
assert qr_data[2] == 2 and qr_data[16] == 32  # Uncompressed true-color TGA.
with Image.open(folder / 'CashAppQR.tga') as qr:
    assert qr.size == (512, 512) and qr.mode == 'RGBA'
    assert qr.getchannel('A').getextrema() == (255, 255)
    quiet = qr_entry['quiet_zone_pixels']
    assert quiet >= 4 * (512 - 2 * quiet) / 37  # Four clear QR modules.
    for box in [(0, 0, 512, quiet), (0, 512 - quiet, 512, 512),
                (0, 0, quiet, 512), (512 - quiet, 0, 512, 512)]:
        assert qr.crop(box).getextrema() == ((0, 0), (0, 0), (0, 0), (255, 255))
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
for prefix in ['WardrobeFrameCrop', 'WardrobeFrameShadowCrop']:
    layer = Image.new('RGBA', (512, 512))
    for suffix, x, y in [('TL',0,0), ('TR',256,0), ('BL',0,256), ('BR',256,256)]:
        with Image.open(folder / (prefix + suffix + '.tga')) as part:
            assert part.size == (256, 256) and part.mode == 'RGBA'
            layer.paste(part, (x, y))
    assert layer.getchannel('A').crop((150,150,350,350)).getextrema()[1] <= 1
ui = (root / 'addon/SaureksCloset/UI.lua').read_text()
assert 'armorShadowFrame' in ui and 'shadow:SetAlpha(.45)' in ui
assert 'wardrobeViewShadowFrame' in ui and 'WardrobeFrameShadowCrop' in ui
assert 'ArmorDecorations.blp' not in ui
print('PASS: texture inventory, payloads, checksums, restored art and separate shadow layers')
