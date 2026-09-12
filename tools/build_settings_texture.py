"""Export the full 320x354 Settings page as native-sized vanilla WoW panels."""
from pathlib import Path
import argparse
import hashlib
import json
from PIL import Image
from optimize_ui_assets import write_blp

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('image', type=Path)
args = parser.parse_args()
out = root / 'addon/SaureksCloset/Textures'
# Match the actual visible page at one texture pixel per UI pixel. Keep the same
# complete-artwork mapping as the original background, without a square intermediate.
with Image.open(args.image) as image:
    page = image.convert('RGB').resize((320, 354), Image.Resampling.LANCZOS)
manifest = root / 'addon/SaureksCloset/ARTWORK.json'
entries = [entry for entry in json.loads(manifest.read_text())
           if entry['texture'] not in ('Settings.tga', 'About.tga', 'SettingsTL.tga',
                                       'SettingsTR.tga', 'SettingsBL.tga', 'SettingsBR.tga',
                                       'SettingsTL.blp', 'SettingsTR.blp', 'SettingsBL.blp', 'SettingsBR.blp')]
# Native texture dimensions must be powers of two. Bottom panels contain 98 visible rows;
# edge padding fills their remaining 30 rows and the UI crops it with texture UVs.
for suffix, x, y, width, height in [('TL', 0, 0, 256, 256), ('TR', 256, 0, 64, 256),
                                    ('BL', 0, 256, 256, 128), ('BR', 256, 256, 64, 128)]:
    visible_height = min(height, 354 - y)
    panel = Image.new('RGB', (width, height))
    panel.paste(page.crop((x, y, x + width, y + visible_height)), (0, 0))
    if visible_height < height:
        edge = page.crop((x, 353, x + width, 354))
        panel.paste(edge.resize((width, height - visible_height)), (0, visible_height))
    target = out / ('Settings' + suffix + '.blp')
    decoded, gpu_bytes, psnr = write_blp(panel, target, False)
    assert psnr is None or psnr >= 30, 'Settings compression quality regressed'
    with Image.open(target) as exported:
        assert exported.size == (width, height) and exported.mode == 'RGB'
    assert target.read_bytes()[:4] == b'BLP2'
    entries.append({'source': args.image.name,
                    'source_sha256': hashlib.sha256(args.image.read_bytes()).hexdigest(),
                    'texture': target.name, 'size': [width, height],
                    'encoding': 'DXT1', 'gpu_base_bytes': gpu_bytes,
                    'layout': '320x354 full Settings page, one texture pixel per UI pixel',
                    'crop': [x, y, x + width, y + visible_height],
                    'bytes': target.stat().st_size})
manifest.write_text(json.dumps(entries, indent=2) + '\n')
print('Prepared and verified the complete 320x354 Settings background in four native-sized panels.')
