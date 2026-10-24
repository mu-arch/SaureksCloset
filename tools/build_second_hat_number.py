"""Center the visible client-font digit, independent of FontString line metrics.

Usage: python tools/build_second_hat_number.py /path/to/FRIZQT__.TTF
The client font stays local; only the small rendered glyph is included.
"""
from pathlib import Path
import hashlib,json,sys
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
font_path=Path(sys.argv[1]);font=ImageFont.truetype(str(font_path),88)
mask=Image.new('L',(192,192));ImageDraw.Draw(mask).text((32,16),'2',font=font,fill=255)
mask=mask.crop(mask.getbbox())
height=18;width=max(2,round((mask.width*height/mask.height)/2)*2)
mask=mask.resize((width,height),Image.Resampling.LANCZOS)
alpha=Image.new('L',(32,32));alpha.paste(mask,((32-width)//2,(32-height)//2))
image=Image.new('RGBA',(32,32),(210,210,204,0));image.putalpha(alpha)
out=root/'addon/SaureksCloset/Textures/SecondHatNumber.tga';image.save(out,format='TGA',compression=None)
left,top,right,bottom=alpha.getbbox()
assert abs((left+right)/2-16)<=.5 and abs((top+bottom)/2-16)<=.5
manifest=root/'addon/SaureksCloset/ARTWORK.json';entries=json.loads(manifest.read_text())
entries=[entry for entry in entries if entry['texture']!='SecondHatNumber.tga']
entries.append(dict(texture='SecondHatNumber.tga',source='tools/build_second_hat_number.py',generated=True,
    source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),font_sha256=hashlib.sha256(font_path.read_bytes()).hexdigest(),
    size=[32,32],encoding='RGBA8 lossless',gpu_base_bytes=4096,bytes=out.stat().st_size,
    sha256=hashlib.sha256(out.read_bytes()).hexdigest()))
manifest.write_text(json.dumps(entries,indent=2)+'\n')
print('Centered digit bounds:',alpha.getbbox(),'center:',((left+right)/2,(top+bottom)/2))
