"""Bake the armor's offset soft shadow into its decoration texture."""
from PIL import Image, ImageFilter
from optimize_ui_assets import bake_armor


def armor_with_shadow(overlay):
    overlay = overlay.convert('RGBA').resize((512, 512), Image.Resampling.LANCZOS)
    alpha = overlay.getchannel('A').filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(5))
    shadow = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
    shadow.putalpha(alpha)
    images = {}
    for suffix, x, y in [('TL', 0, 0), ('TR', 256, 0), ('BL', 0, 256), ('BR', 256, 256)]:
        crop = (x, y, x + 256, y + 256)
        images['ArmorSlots' + suffix + '.tga'] = overlay.crop(crop)
        images['ArmorShadow' + suffix + '.tga'] = shadow.crop(crop)
    return bake_armor(images)
