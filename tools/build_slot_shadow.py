"""Export a soft shadow from the complete decoration alpha silhouette."""
from pathlib import Path
from PIL import Image, ImageFilter
root = Path(__file__).resolve().parents[1]
out = root / 'addon/SaureksCloset/Textures'
alpha = Image.open(out / 'ArmorSlots.tga').convert('RGBA').getchannel('A')
alpha = alpha.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(5))
shadow = Image.new('RGBA', (512, 512), (0, 0, 0, 0))
shadow.putalpha(alpha)
for suffix,x,y in [('TL',0,0),('TR',256,0),('BL',0,256),('BR',256,256)]:
    shadow.crop((x,y,x+256,y+256)).save(out / ('ArmorShadow'+suffix+'.tga'), compression=None)
