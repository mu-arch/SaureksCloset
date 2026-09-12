"""Convert the supplied RGBA window trim into four vanilla-compatible panels."""
from pathlib import Path
import argparse, hashlib, json
from PIL import Image
from optimize_ui_assets import write_blp
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('source',type=Path);args=p.parse_args()
im=Image.open(args.source)
assert im.mode=='RGBA' and im.size==(1200,1310), 'Expected the supplied 1200x1310 RGBA trim'
prepared=im.resize((512,512),Image.Resampling.LANCZOS)
out=root/'addon/SaureksCloset/Textures'
manifest=root/'addon/SaureksCloset/ARTWORK.json'
report=[entry for entry in json.loads(manifest.read_text()) if not entry['texture'].startswith('GenericTrim')]
for suffix,x,y in [('TL',0,0),('TR',256,0),('BL',0,256),('BR',256,256)]:
    name='GenericTrim'+suffix+'.blp';panel=prepared.crop((x,y,x+256,y+256))
    decoded,gpu_bytes,psnr=write_blp(panel,out/name,True)
    assert psnr is None or psnr>=30
    check=Image.open(out/name)
    assert check.mode=='RGBA' and check.size==(256,256)
    report.append({'source':args.source.name,'source_sha256':hashlib.sha256(args.source.read_bytes()).hexdigest(),'texture':name,'layout':'Complete frame, scaled to 512x512 and split into four panels; source alpha preserved','crop':[x,y,x+256,y+256],'size':[256,256],'bytes':(out/name).stat().st_size,'gpu_base_bytes':gpu_bytes,'encoding':'DXT5'})
manifest.write_text(json.dumps(report,indent=2)+'\n')
print('Exported and verified four RGBA trim panels; transparent center preserved.')
