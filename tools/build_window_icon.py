"""Convert the supplied window portrait to a small original-client TGA asset."""
from pathlib import Path
import hashlib, json
from PIL import Image, ImageDraw
root = Path(__file__).resolve().parents[1]
source = root / "assets/window-icon-source.png"
crop = (102, 89, 1146, 1133)
size = (64, 64)
image = Image.open(source).convert("RGBA").crop(crop)
mask = Image.new("L", image.size)
ImageDraw.Draw(mask).ellipse((0, 0, image.width-1, image.height-1), fill=255)
image.putalpha(mask)
image = image.resize(size, Image.Resampling.LANCZOS)
out = root / "addon/SaureksCloset/Textures/Logo.tga"
image.save(out, format="TGA", compression=None)
manifest = root / "addon/SaureksCloset/ARTWORK.json"
entries = json.loads(manifest.read_text())
entry = next(e for e in entries if e["texture"] == "Logo.tga")
entry.clear()
entry.update(source="assets/window-icon-source.png", source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    texture="Logo.tga", crop=list(crop), size=list(size), bytes=out.stat().st_size,
    encoding="RGBA8 (lossless)", gpu_base_bytes=size[0]*size[1]*4, sha256=hashlib.sha256(out.read_bytes()).hexdigest())
manifest.write_text(json.dumps(entries, indent=2)+"\n")
print("Exported 64x64 RGBA window portrait:", out.stat().st_size, "bytes")
