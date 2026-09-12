"""Export the supplied artwork as small, power-of-two, uncompressed WoW 1.12 TGA textures."""
from pathlib import Path
import hashlib,json,argparse
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('source_directory',type=Path);p.add_argument('--logo',type=Path);p.add_argument('--mini-logo',type=Path);a=p.parse_args()
out=root/'addon/SaureksCloset/Textures';out.mkdir(exist_ok=True)
# Main crop removes the supplied outer header/footer; native WoW borders remain in the UI.
assets=[('main_bg.png','Main.tga',(12,145,1022,1229),(512,512)),
        ('vanillacloset_logo.png','Logo.tga',(102,88,1152,1138),(128,128))]
manifest=root/'addon/SaureksCloset/ARTWORK.json'
existing=json.loads(manifest.read_text()) if manifest.exists() else []
report=[entry for entry in existing if entry.get('generated') or entry['texture'] in ('SettingsTL.tga','SettingsTR.tga','SettingsBL.tga','SettingsBR.tga')]
if a.mini_logo:
 assets.append((str(a.mini_logo),'MiniLogo.tga',(6,14,1224,1232),(64,64)))
else:
 manifest=root/'addon/SaureksCloset/ARTWORK.json'
 report.extend(entry for entry in existing if entry['texture']=='MiniLogo.tga')
for source,name,crop,size in assets:
 src=a.logo if name=='Logo.tga' and a.logo else a.source_directory/source;im=Image.open(src).convert('RGB');im=im.crop(crop).resize(size,Image.Resampling.LANCZOS)
 if name in ('Logo.tga','MiniLogo.tga'):
  mask=Image.new('L',(512,512));ImageDraw.Draw(mask).ellipse((0,0,511,511),fill=255)
  im=im.convert('RGBA');im.putalpha(mask.resize(size,Image.Resampling.LANCZOS))
 im.save(out/name,format='TGA',compression=None)
 check=Image.open(out/name);assert check.size==size and check.mode==('RGBA' if name in ('Logo.tga','MiniLogo.tga') else 'RGB')
 report.append({'source':src.name,'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'texture':name,'crop':crop,'size':size,'bytes':(out/name).stat().st_size})
(root/'addon/SaureksCloset/ARTWORK.json').write_text(json.dumps(report,indent=2)+'\n')
print('Prepared',len(report),'client-ready textures:',sum(i['bytes'] for i in report),'bytes total')
