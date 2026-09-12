"""Verify two-handed sword anchors from the installed 1.12.1 models; no model writes.

Run with --write to regenerate the small C++ regression fixture. Only attachment
metadata is retained; the client model files are never copied into the addon.
"""
import ctypes as C
import hashlib
import json
from pathlib import Path
import struct
import sys
import read_client_data as client

root=Path(__file__).resolve().parents[1]
rows=[]
try:
    for name in ['patch-2.MPQ','patch.MPQ','model.MPQ']:
        handle=C.c_void_p()
        assert client.lib.SFileOpenArchive(str(client.game/name).encode(),0,0x100,C.byref(handle)),name
        client.archives.append(handle)
    for race,name in enumerate(['Human','Orc','Dwarf','NightElf','Scourge','Tauren','Gnome','Troll'],1):
        for sex in ['Male','Female']:
            filename=f'Character\\{name}\\{sex}\\{name}{sex}.m2'
            data=client.read(filename)
            assert data and data[:8]==b'MD20\x00\x01\x00\x00',filename
            count,offset=struct.unpack_from('<II',data,0x104)
            lookup_count,lookup=struct.unpack_from('<II',data,0x10C)
            bone_count=struct.unpack_from('<I',data,0x34)[0]
            assert 28<lookup_count<=512 and 0<count<=512 and 0<bone_count<=2048
            points={}
            for point in [26,27]:
                index=struct.unpack_from('<H',data,lookup+2*point)[0]
                assert index<count
                actual,bone,unused,x,y,z=struct.unpack_from('<IHH3f',data,offset+48*index)
                assert actual==point and bone<bone_count
                points[point]={'index':index,'bone':bone,'position':[x,y,z]}
            rows.append({'race':race,'sex':sex,'file':filename,'sha256':hashlib.sha256(data).hexdigest(),'points':points})
finally:
    for handle in client.archives:client.lib.SFileCloseArchive(handle)

fixture='// Extracted from all 16 installed build-5875 character models.\n// Regenerate with tools/audit_sword_attachments.py. Test data only.\nstruct SwordAttachmentFixture { unsigned race,sex,point,index,bone; std::array<float,3> position; };\nstatic const SwordAttachmentFixture swordFixtures[]={\n'
for row in rows:
    for point_id in [26,27]:
        point=row['points'][point_id]
        fixture+='{%d,%d,%d,%d,%d,{{%s}}},\n'%(row['race'],row['sex']=='Female',point_id,point['index'],point['bone'],','.join('%.9ff'%v for v in point['position']))
fixture+='};\n'
path=root/'tests/sword_attachment_fixtures.h'
if '--write' in sys.argv:path.write_text(fixture)
else:assert path.read_text()==fixture,'Installed models differ from regression fixtures; inspect before regenerating.'
print('PASS: sword attachment IDs, indices, bones and coordinates verified against all 16 installed models.')
if '--json' in sys.argv:print(json.dumps(rows,indent=2))
