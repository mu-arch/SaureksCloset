"""Make transfer and corresponding-source archives without Blizzard assets or executables."""
from pathlib import Path
import hashlib,json,zipfile,re
root=Path(__file__).resolve().parents[1]
version='3.4.34'
addon=root/'addon/SaureksCloset'
assert re.search(r'V.VERSION = "([^"]+)"',(addon/'Core.lua').read_text()).group(1)==version
assert re.search(r'## Version: ([^\n]+)',(addon/'SaureksCloset.toc').read_text()).group(1)==version
dll_version=int(re.search(r'static int __fastcall version\(void\* L\)\{return result\(L,(\d+)\);\}',(root/'native/SaureksCloset.cpp').read_text()).group(1))
required=int(re.search(r'V.REQUIRED_RENDERER=(\d+)',(addon/'Updates.lua').read_text()).group(1))
assert dll_version==required,'Addon requires a different DLL version'
(root/'update-version.txt').write_text('schema=1\naddon='+version+'\ndll='+str(dll_version)+'\n')
payload={f'Interface/AddOns/SaureksCloset/{p.relative_to(addon).as_posix()}':p for p in addon.rglob('*') if p.is_file()}
payload.update({'SaureksCloset.dll':root/'native/SaureksCloset.dll','README.md':root/'addon/SaureksCloset/README.md',
                'LICENSE.txt':root/'native/LICENSE','MINHOOK-LICENSE.txt':root/'native/vendor/minhook/LICENSE.txt'})
payload.update({'install.py':root/'tools/install.py','CLIENT-BUILD.json':root/'native/CLIENT-BUILD.json'})
payload['update-version.txt']=root/'update-version.txt'
payload['Interface/AddOns/SaureksCloset/Installation instructions/SaureksCloset.dll']=root/'native/SaureksCloset.dll'
readme=payload['README.md'].read_text()
for ref in ['Screenshots/', 'Installation%20instructions/', 'CATALOG-COPYRIGHT.md', 'CATALOG-LICENSE.md', 'ARTWORK.json']:
 readme=readme.replace(']('+ref,'](Interface/AddOns/SaureksCloset/'+ref)
readme_bytes=readme.encode()
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
 z.writestr('README.md','Saurek\'s Closet '+version+' corresponding source.\n\nBuild the bridge with sh tools/build_native.sh (i686-w64-mingw32 GCC).\nSee addon/SaureksCloset/README.md and native/RESEARCH.md for behavior and validation limits.\nThe generated headers/signatures needed for compilation are included.\nBlizzard executable, model assets, DBCs and FrameXML references are not redistributed.\nLua tests require the extracted FrameXML references listed in tests/test.lua and Lua 5.0.3.\n')
with zipfile.ZipFile(root/f'SaureksCloset-{version}.zip') as z:
 assert z.testzip() is None
 for name,digest in manifest.items():assert hashlib.sha256(z.read(name)).hexdigest()==digest
 assert 'SaureksCloset.dll' in z.namelist() and not any(name.endswith('WoW.exe') for name in z.namelist())
with zipfile.ZipFile(root/f'SaureksCloset-{version}-source.zip') as z:assert z.testzip() is None
print('Created and verified transfer and source archives for '+version)
