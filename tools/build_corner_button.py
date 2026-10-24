"""Draw a blank round button in the placement cog's dark bronze UI style."""
from pathlib import Path
import hashlib,json,math
from PIL import Image
root=Path(__file__).resolve().parents[1]
size,scale=32,8
image=Image.new('RGBA',(size*scale,size*scale))
for y in range(size*scale):
    for x in range(size*scale):
        dx=(x+.5)/scale-16;dy=(y+.5)/scale-16
        radius=math.hypot(dx,dy)
        if radius>15.5:continue
        if radius>13.5:
            light=max(0,min(1,.5-(dx+dy)/44))
            color=tuple(round(a+(b-a)*light) for a,b in ((54,120),(45,99),(29,58)))+(255,)
        elif radius>12.5:color=(8,8,7,255)
        else:color=(18,17,14,255)
        image.putpixel((x,y),color)
image=image.resize((size,size),Image.Resampling.LANCZOS)
out=root/'addon/SaureksCloset/Textures/CornerButton.tga'
image.save(out,format='TGA',compression=None)
manifest=root/'addon/SaureksCloset/ARTWORK.json'
entries=json.loads(manifest.read_text())
entries=[entry for entry in entries if entry['texture']!='CornerButton.tga']
entries.append(dict(texture='CornerButton.tga',source='tools/build_corner_button.py',generated=True,
    source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),size=[size,size],
    encoding='RGBA8 lossless',gpu_base_bytes=size*size*4,bytes=out.stat().st_size,
    sha256=hashlib.sha256(out.read_bytes()).hexdigest()))
manifest.write_text(json.dumps(entries,indent=2)+'\n')
assert image.getpixel((0,0))[3]==0 and image.getpixel((16,16))[3]==255
print('Exported round corner-button background:',out.stat().st_size,'bytes')
