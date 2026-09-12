"""Draw the matching open-eye UI glyph for the active outfit, with transparent corners."""
from pathlib import Path
import hashlib,json
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
scale=8
image=Image.new('RGBA',(32*scale,32*scale),(0,0,0,0));draw=ImageDraw.Draw(image)
def curve(start,control,end):
 points=[]
 for i in range(65):
  t=i/64
  points.append(tuple(round(((1-t)**2*start[j]+2*(1-t)*t*control[j]+t*t*end[j])*scale) for j in (0,1)))
 return points
outline=curve((4,16),(16,1),(28,16))+curve((28,16),(16,31),(4,16))
grey=(151,157,163,255)
draw.line(outline,fill=(20,23,26,230),width=4*scale,joint='curve')
draw.line(outline,fill=grey,width=2*scale,joint='curve')
draw.ellipse((12*scale,12*scale,20*scale,20*scale),outline=grey,width=2*scale)
image=image.resize((64,64),Image.Resampling.LANCZOS)
out=root/'addon/SaureksCloset/Textures/VisibleSlot.tga';image.save(out,format='TGA',compression=None)
assert out.read_bytes()[2]==2
manifest=root/'addon/SaureksCloset/ARTWORK.json'
report=json.loads(manifest.read_text())
report=[entry for entry in report if entry['texture']!='VisibleSlot.tga']
report.append({'source':'tools/build_visible_slot_icon.py','generated':True,'source_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'texture':'VisibleSlot.tga','size':[64,64],'bytes':out.stat().st_size})
manifest.write_text(json.dumps(report,indent=2)+'\n')
print('Exported visible-slot UI glyph:' ,out.stat().st_size,'bytes')
