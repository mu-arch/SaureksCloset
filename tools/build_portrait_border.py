"""Build a thin, centered circular UI border at the addon's standard icon size."""
from pathlib import Path
from PIL import Image, ImageDraw
root = Path(__file__).resolve().parents[1]
scale = 4
im = Image.new('RGBA', (64 * scale, 64 * scale))
draw = ImageDraw.Draw(im)
# At 40 UI pixels this produces a one-pixel gold rim and a soft outer edge.
draw.ellipse(tuple(v * scale for v in (3, 3, 61, 61)), outline=(35, 28, 15, 160), width=3 * scale)
draw.ellipse(tuple(v * scale for v in (4, 4, 60, 60)), outline=(175, 148, 86, 255), width=round(1.6 * scale))
im.resize((64, 64), Image.Resampling.LANCZOS).save(root / 'addon/SaureksCloset/Textures/PortraitBorder.tga', compression=None)
