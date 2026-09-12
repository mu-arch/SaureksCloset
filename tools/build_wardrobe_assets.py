"""Export separate wardrobe backdrop and alpha slot layer for the 328x355 content area."""
from pathlib import Path
import argparse,hashlib,json
from PIL import Image
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('source_directory',type=Path);a=p.parse_args()
out=root/'addon/SaureksCloset/Textures'
# Fit the backdrop to the content aspect; retain the decoration's complete visible frame.
background=a.source_directory/'BG4.png';slots=a.source_directory/'armor_slot_decorations.png'
bg=Image.open(background).convert('RGB');overlay=Image.open(slots).convert('RGBA')
assert bg.size==(1254,1254) and overlay.size==(1200,1310)
bg=bg.crop((47,0,1206,1254))
# Keep the complete decoration, including its new bottom-center ornament.
layer=overlay.crop((0,0,1200,1310))
layer.putalpha(layer.getchannel('A').point(lambda value:255 if value>=240 else value))
manifest=root/'addon/SaureksCloset/ARTWORK.json'
report=[v for v in json.loads(manifest.read_text()) if v['texture'] not in ('Main.tga','ArmorSlots.tga','ArmorSlotsTL.tga','ArmorSlotsTR.tga','ArmorSlotsBL.tga','ArmorSlotsBR.tga')]
for source,name,im,description in [(background,'Main.tga',bg,'Centered crop: 47,0,1206,1254'),(slots,'ArmorSlots.tga',layer,'Complete decoration frame: 0,0,1200,1310')]:
 im.resize((512,512),Image.Resampling.LANCZOS).save(out/name,format='TGA',compression=None)
 check=Image.open(out/name);assert check.mode==im.mode and check.size==(512,512)
 report.append({'source':source.name,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'texture':name,'layout':description,'size':[512,512],'bytes':(out/name).stat().st_size})
# Vanilla UI artwork normally uses 256-pixel panels. Split the finished overlay
# without resampling again, so the four pieces meet on identical pixel boundaries.
prepared=Image.open(out/'ArmorSlots.tga').convert('RGBA')
for suffix,x,y in [('TL',0,0),('TR',256,0),('BL',0,256),('BR',256,256)]:
 name='ArmorSlots'+suffix+'.tga'
 prepared.crop((x,y,x+256,y+256)).save(out/name,format='TGA',compression=None)
 report.append({'source':slots.name,'source_sha256':hashlib.sha256(slots.read_bytes()).hexdigest(),'texture':name,'layout':'Quadrant of ArmorSlots.tga','crop':[x,y,x+256,y+256],'size':[256,256],'bytes':(out/name).stat().st_size})
manifest.write_text(json.dumps(report,indent=2)+'\n')
print('Exported separate RGB backdrop and RGBA slot overlay; transparent center preserved.')

# Keep the decoration silhouette shadow in sync whenever the supplied artwork changes.
import runpy
runpy.run_path(str(root/'tools/build_slot_shadow.py'))
