"""Package one addon folder ready to drop into Interface/AddOns."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
# A matching version string is insufficient: the actual release DLL must also
# enable the Body controls, and the shipped artwork must retain its alignment.
subprocess.run([sys.executable, str(root / 'tools/check_release.py')], cwd=root, check=True)
addon = root / 'addon/SaureksCloset'
version = re.search(r'V.VERSION = "([^"]+)"', (addon / 'Core.lua').read_text()).group(1)
assert re.search(r'## Version: ([^\n]+)', (addon / 'SaureksCloset.toc').read_text()).group(1) == version
required = int(re.search(r'V.REQUIRED_RENDERER=(\d+)', (addon / 'Updates.lua').read_text()).group(1))
native_version = int(re.search(r'static int __fastcall version\(void\* L\)\{return result\(L,(\d+)\);\}', (root / 'native/SaureksCloset.cpp').read_text()).group(1))
assert native_version == required
# Preserve the schema-1 wire version so previously installed clients can detect this release.
assert (root / 'update-version.txt').read_text() == f'schema=1\naddon={version[2:] if version.startswith("0.") else version}\ndll={required}\n'
dll = addon / 'Installation instructions/SaureksCloset.dll'
assert dll.read_bytes() == (root / 'native/SaureksCloset.dll').read_bytes(), 'Bundled DLL differs from the current build'
artwork = json.loads((addon / 'ARTWORK.json').read_text())
assert {p.name for p in (addon / 'Textures').iterdir()} == {e['texture'] for e in artwork} | {'ASSETS-LICENSE'}
for icon in re.findall(r'icon="([^"]+)"', (addon / 'BagCatalog.lua').read_text()):
    name = icon.rsplit('\\', 1)[-1]
    assert name in {e['texture'] for e in artwork}, 'Unlisted bag icon: ' + name
    assert (addon / 'Textures' / name).is_file(), 'Missing bag icon: ' + name
for license_copy in [addon / 'LICENSE', root / 'native/LICENSE', addon / 'Installation instructions/LICENSE.txt']:
    assert license_copy.read_bytes() == (root / 'LICENSE').read_bytes()
assert (addon / 'LICENSING.md').read_bytes() == (root / 'LICENSING.md').read_bytes()
assert (addon / 'ASSETS-LICENSE').read_bytes() == (root / 'ASSETS-LICENSE').read_bytes()
bags = json.loads((root / 'native/BAG-ASSETS.json').read_text())
assert bags['schema'] == 2
bag_files = {'ASSETS-LICENSE'}
for bag in bags['bags']:
    for field, checksum in [('model', 'model_sha256'), ('texture', 'blp_sha256')]:
        name = bag[field]
        assert Path(name).name == name, 'Bag manifest must contain basenames only'
        assert hashlib.sha256((addon / 'Models' / name).read_bytes()).hexdigest() == bag[checksum], name
        bag_files.add(name)
assert {p.name for p in (addon / 'Models').iterdir() if p.is_file()} == bag_files

# Keep only the installable addon, its existing documentation/screenshots and DLL.
# Build metadata stays in the repository. Never rewrite the user's README.
payload = {}
for p in addon.rglob('*'):
    if not p.is_file():
        continue
    relative = p.relative_to(addon)
    if relative.parts[0] == 'Textures':
        include = p.name in {e['texture'] for e in artwork} or p.name == 'ASSETS-LICENSE'
    elif relative.parts[0] == 'Screenshots':
        include = p.suffix.lower() in ('.png', '.gif') or p.name == 'ASSETS-LICENSE'
    elif relative.parts[0] == 'CapeMotion':
        include = len(relative.parts) == 2 and (p.name in {f'B{i:02}.m2' for i in range(1,17)} or p.name == 'ASSETS-LICENSE')
    elif relative.parts[0] == 'Models':
        include = p.name in bag_files
    elif relative.parts[0] == 'Installation instructions':
        include = p.suffix.lower() in ('.txt', '.dll')
    else:
        include = len(relative.parts) == 1 and (p.suffix.lower() in ('.lua', '.xml', '.toc', '.md') or p.name in ('LICENSE', 'ASSETS-LICENSE'))
    if include:
        payload['SaureksCloset/' + relative.as_posix()] = p
assert 'SaureksCloset/README.md' in payload
for name in ['LICENSE', 'LICENSING.md', 'Installation instructions/LICENSE.txt', 'Installation instructions/MINHOOK-LICENSE.txt', 'CATALOG-LICENSE.md', 'CATALOG-COPYRIGHT.md', 'ASSETS-LICENSE', 'Textures/ASSETS-LICENSE', 'Screenshots/ASSETS-LICENSE']:
    assert 'SaureksCloset/' + name in payload
for line in (addon / 'SaureksCloset.toc').read_text().splitlines():
    if line.strip() and not line.startswith('#'):
        assert 'SaureksCloset/' + line.strip() in payload, line
for link in re.findall(r'\]\((Screenshots/[^)]+)\)|src="(Screenshots/[^"]+)"', (addon / 'README.md').read_text()):
    link = link[0] or link[1]
    assert 'SaureksCloset/' + link in payload, 'Missing README image: ' + link
assert [name for name in payload if name.endswith('.dll')] == ['SaureksCloset/Installation instructions/SaureksCloset.dll']
checksums = {name: hashlib.sha256(p.read_bytes()).hexdigest() for name, p in payload.items()}
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=root / f'SaureksCloset-{version}.zip')
output = parser.parse_args().output
with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
    for name, p in sorted(payload.items()):
        archive.write(p, name)
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    assert set(archive.namelist()) == set(payload)
    assert {name.split('/')[0] for name in archive.namelist()} == {'SaureksCloset'}
    for name, digest in checksums.items():
        assert hashlib.sha256(archive.read(name)).hexdigest() == digest, name
        assert hashlib.sha256(payload[name].read_bytes()).hexdigest() == digest, 'Source changed during packaging'
print(f'Created and verified {output.name}: one addon folder, {len(payload)} files, {output.stat().st_size:,} bytes.')
