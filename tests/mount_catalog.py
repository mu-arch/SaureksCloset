"""Offline catalog consistency: complete pinned mounted-spell mapping and native icons."""
import json
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
metadata=json.loads((root/'addon/SaureksCloset/MOUNTS-SOURCE.json').read_text())
source=(root/'addon/SaureksCloset/MountCatalog.lua').read_text()
rows=re.findall(r'^\{id=(\d+),name=("(?:\\.|[^"\\])*"),icon=("(?:\\.|[^"\\])*")\},$',source,re.M)
entries=metadata['entries']
assert len(rows)==metadata['count']==len(entries)==109
assert len({int(r[0]) for r in rows})==109
assert entries==sorted(entries,key=lambda r:(r['name'].lower(),r['id']))
for row,entry in zip(rows,entries):
    assert (int(row[0]),json.loads(row[1]),json.loads(row[2]))==(entry['id'],entry['name'],entry['icon'])
    assert entry['model'].lower().startswith('creature\\')
    assert entry['icon'].lower().startswith('interface\\icons\\')
    assert entry['spells'] and len(entry['spells'])==len(set(entry['spells']))
spells={spell for entry in entries for spell in entry['spells']}
assert len(spells)==126, 'Every mounted spell in the client must resolve to an appearance'
assert {458,5784,13819,23161,23214,24252,25863,30174}<=spells
assert len(metadata['auraNames'])==117
assert metadata['sha256']=='4f92db520868ab4e566726f68b5b2e380ae781209beaf22237b4f7f04600d0c0'
assert all(icon.lower().startswith('interface\\icons\\ability_mount_') for icon in metadata['exclusiveMountIcons'])
assert all(icon in metadata['candidateMountIcons'] for icon in metadata['exclusiveMountIcons'])
# Common mount must resolve to a creature display, not its creature or spell ID.
assert next(e for e in entries if e['id']==2404)['name']=='Brown Horse'
assert next(e for e in entries if e['id']==2404)['icon']=='Interface\\Icons\\Ability_Mount_RidingHorse'
toc=(root/'addon/SaureksCloset/SaureksCloset.toc').read_text().splitlines()
assert toc.index('Core.lua')<toc.index('MountCatalog.lua')<toc.index('Mounts.lua')<toc.index('UI.lua')
print('PASS: 109 unique mount displays, all 126 client mounted spells, validated names/icons/models, pinned provenance and load ordering')
