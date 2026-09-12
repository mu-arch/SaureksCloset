"""Make transfer and corresponding-source archives without Blizzard assets or executables."""
from pathlib import Path
import hashlib,json,zipfile,re
root=Path(__file__).resolve().parents[1]
version='3.4.35'
addon=root/'addon/SaureksCloset'
assert re.search(r'V.VERSION = "([^"]+)"',(addon/'Core.lua').read_text()).group(1)==version
assert re.search(r'## Version: ([^\n]+)',(addon/'SaureksCloset.toc').read_text()).group(1)==version
dll_version=int(re.search(r'static int __fastcall version\(void\* L\)\{return result\(L,(\d+)\);\}',(root/'native/SaureksCloset.cpp').read_text()).group(1))
required=int(re.search(r'V.REQUIRED_RENDERER=(\d+)',(addon/'Updates.lua').read_text()).group(1))
assert dll_version==required,'Addon requires a different DLL version'
(root/'update-version.txt').write_text('schema=1\naddon='+version+'\ndll='+str(dll_version)+'\n')
expected_textures={entry['texture'] for entry in json.loads((addon/'ARTWORK.json').read_text())}
actual_textures={p.name for p in (addon/'Textures').iterdir() if p.suffix in ('.tga','.blp')}
assert actual_textures==expected_textures,'Texture directory and artwork record differ; do not ship unused source textures'
payload={f'Interface/AddOns/SaureksCloset/{p.relative_to(addon).as_posix()}':p for p in addon.rglob('*') if p.is_file()}
payload.update({'SaureksCloset.dll':root/'native/SaureksCloset.dll','README.md':(root/'README.md' if (root/'README.md').exists() else addon/'README.md'),
                'LICENSE.txt':root/'native/LICENSE','MINHOOK-LICENSE.txt':root/'native/vendor/minhook/LICENSE.txt'})
payload.update({'install.py':root/'tools/install.py','CLIENT-BUILD.json':root/'native/CLIENT-BUILD.json'})
payload['update-version.txt']=root/'update-version.txt'
payload['Interface/AddOns/SaureksCloset/Installation instructions/SaureksCloset.dll']=root/'native/SaureksCloset.dll'
# Publish the existing README exactly as written by its owner.
readme_bytes=payload['README.md'].read_bytes()
manifest={name:hashlib.sha256(readme_bytes if name=='README.md' else p.read_bytes()).hexdigest() for name,p in payload.items()}
with zipfile.ZipFile(root/f'SaureksCloset-{version}.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name,p in sorted(payload.items()):
  if name=='README.md':z.writestr(name,readme_bytes)
  else:z.write(p,name)
 z.writestr('FILES-SHA256.json',json.dumps(manifest,indent=2)+'\n')
source={'update-version.txt':root/'update-version.txt'}
for folder in ['addon','native','tools','tests']:
 for p in (root/folder).rglob('*'):
  if not p.is_file() or any(part in ('build','__pycache__','.git') for part in p.parts):continue
  if p.suffix in ('.dll','.o','.pyc') or p.name in ('BodyState.h','body_state.cpp'):continue
  source[str(p.relative_to(root))]=p
with zipfile.ZipFile(root/f'SaureksCloset-{version}-source.zip','w',zipfile.ZIP_DEFLATED) as z:
 for name,p in sorted(source.items()):z.write(p,name)
 z.writestr('README.md',readme_bytes)

with zipfile.ZipFile(root/f'SaureksCloset-{version}.zip') as z:
 assert z.testzip() is None
 for name,digest in manifest.items():assert hashlib.sha256(z.read(name)).hexdigest()==digest
 assert 'SaureksCloset.dll' in z.namelist() and not any(name.endswith('WoW.exe') for name in z.namelist())
with zipfile.ZipFile(root/f'SaureksCloset-{version}-source.zip') as z:assert z.testzip() is None
print('Created and verified transfer and source archives for '+version)
