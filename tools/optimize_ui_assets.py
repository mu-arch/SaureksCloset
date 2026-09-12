"""Build compressed UI assets from an original addon-image backup.

Usage: python tools/optimize_ui_assets.py backup.zip --prefix installed \
    --output addon/SaureksCloset --review texture-review

Input must be the unoptimized source backup: never recompress a previous output.
Uses the client's BLP2/DXT layout and Pillow's DDS block encoder. Screenshots
keep their pixels exactly. The caller keeps the backup outside the addon.
"""
from pathlib import Path
import argparse
import hashlib
import io
import json
import math
import struct
import zipfile
import numpy as np
from PIL import Image


def write_blp(image, path, alpha):
    """One-mip UI BLP2: BC1 opaque, BC3 smooth alpha; includes standard palette area."""
    image = image.convert('RGBA' if alpha else 'RGB')
    w, h = image.size
    assert w >= 4 and h >= 4 and w & (w - 1) == 0 and h & (h - 1) == 0
    dds = io.BytesIO()
    image.save(dds, format='DDS', pixel_format='DXT5' if alpha else 'DXT1')
    data = dds.getvalue()
    assert data[:4] == b'DDS ' and data[84:88] == (b'DXT5' if alpha else b'DXT1')
    blocks = data[128:]
    assert len(blocks) == w * h // (1 if alpha else 2)
    # BLP2 header (148 bytes), zero palette (1024 bytes), one block-compressed mip.
    header = struct.pack('<4sI4BII', b'BLP2', 1, 2, 8 if alpha else 0,
                         7 if alpha else 0, 0, w, h)
    header += struct.pack('<16I', 1172, *([0] * 15))
    header += struct.pack('<16I', len(blocks), *([0] * 15))
    path.write_bytes(header + bytes(1024) + blocks)
    with Image.open(path) as decoded:
        decoded.load()
        assert decoded.size == image.size and decoded.mode == image.mode
        decoded = decoded.copy()
    # Compare over both dark and light backgrounds so transparent-edge damage counts.
    errors = []
    for color in [(24, 24, 24, 255), (210, 210, 210, 255)]:
        bg = Image.new('RGBA', image.size, color)
        before = np.asarray(Image.alpha_composite(bg, image.convert('RGBA')), dtype=float)
        after = np.asarray(Image.alpha_composite(bg, decoded.convert('RGBA')), dtype=float)
        errors.append(float(np.mean((before[..., :3] - after[..., :3]) ** 2)))
    mse = max(errors)
    return decoded, len(blocks), None if mse == 0 else 10 * math.log10(255 ** 2 / mse)


def joined(images, prefix):
    result = Image.new('RGBA', (512, 512))
    for suffix, x, y in [('TL', 0, 0), ('TR', 256, 0), ('BL', 0, 256), ('BR', 256, 256)]:
        result.paste(images[prefix + suffix + '.tga'], (x, y))
    return result


def bake_armor(images):
    """Composite at 2x UI size, preserving the old 3px/4px shadow offset and clips."""
    decor = Image.new('RGBA', (652, 722))
    shadow = Image.new('RGBA', (652, 722))
    for suffix, x, y, width in [('TL', 0, 0, 328), ('TR', 328, 0, 326),
                                ('BL', 0, 357, 328), ('BR', 328, 357, 326)]:
        art = images['ArmorSlots' + suffix + '.tga'].resize((width, 357), Image.Resampling.LANCZOS)
        shade = images['ArmorShadow' + suffix + '.tga'].resize((width, 357), Image.Resampling.LANCZOS)
        shade.putalpha(shade.getchannel('A').point(lambda a: round(a * .45)))
        if x:
            art = art.crop((0, 0, width - 2, 357))
            shade = shade.crop((0, 0, width - 8, 357))
        decor.alpha_composite(art, (x, y))
        shadow.alpha_composite(shade, (x + 6, y + 8))
    # 512 pixels retains the original detail density and also covers the shadow's
    # four-pixel overhang; draw this combined image at 326x361 UI pixels.
    return Image.alpha_composite(shadow, decor).resize((512, 512), Image.Resampling.LANCZOS)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('backup', type=Path)
    parser.add_argument('--prefix', default='installed')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--review', type=Path, required=True)
    args = parser.parse_args()
    args.review.mkdir(parents=True, exist_ok=True)
    texture_dir = args.output / 'Textures'
    texture_dir.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.backup) as z:
        checksums = json.loads(z.read('SHA256.json'))
        raw = {name[len(args.prefix)+1:]: z.read(name) for name in checksums if name.startswith(args.prefix + '/')}
        for name, data in raw.items():
            assert hashlib.sha256(data).hexdigest() == checksums[args.prefix + '/' + name]
    images = {Path(name).name: Image.open(io.BytesIO(data)).convert('RGBA')
              for name, data in raw.items() if name.startswith('Textures/')}
    assert len(images) == 23, 'Expected the complete original texture set'
    outputs = {name[:-4]: image for name, image in images.items()
               if not name.startswith(('ArmorSlots', 'ArmorShadow'))}
    outputs['ArmorDecorations'] = bake_armor(images)
    # Small icons keep their exact pixels: block compression visibly damages their
    # fine lettering, circular borders and gold edges for very little memory gain.
    lossless_icons = {'Logo', 'MiniLogo', 'HiddenSlot', 'VisibleSlot', 'PortraitBorder'}
    report = {'backup_sha256': hashlib.sha256(args.backup.read_bytes()).hexdigest(),
              'texture_bytes_before': sum(len(v) for k, v in raw.items() if k.startswith('Textures/')),
              'resident_base_rgba_bytes_before': sum(im.width*im.height*4 for n, im in images.items() if n != 'ArmorSlots.tga'),
              'textures': [], 'screenshots': []}
    for name, image in outputs.items():
        alpha = image.getchannel('A').getextrema()[0] < 255
        if name in lossless_icons:
            path = texture_dir / (name + '.tga')
            path.write_bytes(raw['Textures/' + name + '.tga'])
            decoded, payload, psnr = image.copy(), image.width * image.height * 4, None
            encoding = 'RGBA8 lossless'
        else:
            path = texture_dir / (name + '.blp')
            decoded, payload, psnr = write_blp(image, path, alpha)
            encoding = 'DXT5' if alpha else 'DXT1'
            assert psnr is None or psnr >= 30, 'Compression quality regressed: ' + name
        image.save(args.review / (name + '-before.png'))
        decoded.save(args.review / (name + '-after.png'))
        report['textures'].append({'texture': path.name, 'size': list(image.size),
                                   'encoding': encoding,
                                   'gpu_base_bytes': payload, 'bytes': path.stat().st_size,
                                   'worst_background_psnr_db': psnr,
                                   'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
    keep = {t['texture'] for t in report['textures']}
    for path in texture_dir.iterdir():
        if path.suffix in ('.tga', '.blp') and path.name not in keep:
            path.unlink()
    # Strip needless opaque alpha and optimize PNG filters/zlib; compare all pixels.
    for name, data in raw.items():
        if not name.startswith('Screenshots/'):
            continue
        original = Image.open(io.BytesIO(data)).convert('RGBA')
        image = original.convert('RGB') if original.getchannel('A').getextrema() == (255, 255) else original
        encoded = io.BytesIO();image.save(encoded, format='PNG', optimize=True, compress_level=9)
        result = encoded.getvalue() if len(encoded.getvalue()) < len(data) else data
        assert Image.open(io.BytesIO(result)).convert('RGBA').tobytes() == original.tobytes()
        dst = args.output / name;dst.parent.mkdir(parents=True, exist_ok=True);dst.write_bytes(result)
        report['screenshots'].append({'file': name, 'before': len(data), 'after': len(result), 'lossless': True})
    report['texture_bytes_after'] = sum(t['bytes'] for t in report['textures'])
    report['resident_compressed_base_bytes_after'] = sum(t['gpu_base_bytes'] for t in report['textures'])
    report['image_bytes_before'] = report['texture_bytes_before'] + sum(t['before'] for t in report['screenshots'])
    report['image_bytes_after'] = report['texture_bytes_after'] + sum(t['after'] for t in report['screenshots'])
    (args.review / 'optimization-report.json').write_text(json.dumps(report, indent=2) + '\n')
    # Update only the artwork record; Markdown files are never opened or changed.
    old = json.loads(raw['ARTWORK.json'])
    sources = {entry['texture']: entry for entry in old}
    entries = []
    for item in report['textures']:
        base = sources.get(item['texture'].replace('.blp', '.tga'), {})
        entry = dict(base)
        entry.update(item)
        if item['texture'] == 'ArmorDecorations.blp':
            entry['source'] = 'Original ArmorSlots and ArmorShadow panels'
            entry['layout'] = 'Combined trim and 45% shadow, 3px right/4px down, drawn at 326x361'
        entries.append(entry)
    (args.output / 'ARTWORK.json').write_text(json.dumps(entries, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if not isinstance(v, list)}, indent=2))
    print('Lowest composited PSNR:', min(t['worst_background_psnr_db'] or 100 for t in report['textures']))


if __name__ == '__main__':
    main()
