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
# Export compressed runtime textures directly; the original artwork stays external.
from optimize_ui_assets import write_blp
from build_slot_shadow import armor_with_shadow
manifest=root/'addon/SaureksCloset/ARTWORK.json'
report=[v for v in json.loads(manifest.read_text()) if not v['texture'].startswith(('Main.', 'ArmorSlots', 'ArmorShadow', 'ArmorDecorations'))]
for source,name,im,description in [
    (background,'Main.blp',bg.resize((512,512),Image.Resampling.LANCZOS),'Centered crop: 47,0,1206,1254'),
    (slots,'ArmorDecorations.blp',armor_with_shadow(layer),'Combined trim and 45% shadow, 3px right/4px down, drawn at 326x361')]:
    decoded,gpu_bytes,psnr=write_blp(im,out/name,im.mode=='RGBA')
    assert psnr is None or psnr>=30
    report.append({'source':source.name,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'texture':name,'layout':description,'size':[512,512],'bytes':(out/name).stat().st_size,'gpu_base_bytes':gpu_bytes})
manifest.write_text(json.dumps(report,indent=2)+'\n')
print('Exported compressed backdrop and combined armor decoration/shadow.')
