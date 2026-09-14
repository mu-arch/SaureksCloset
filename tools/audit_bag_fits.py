"""Measure pack contact depth from vanilla body surfaces; never copy client meshes.

WOW_DATA points to the local Data directory. --write regenerates the small
numeric fit table. Default mode verifies it against the installed models.
"""
import argparse
import ctypes as C
import struct
from pathlib import Path
import read_client_data as client


BAG_MODEL_SCALE = .45


def contact_depth(vertices, faces, back):
    scale=BAG_MODEL_SCALE;surfaces=[]
    # Sample the central contact patch; the pack has rounded rear corners.
    for dy in [-.35,-.175,0,.175,.35]:
        for dz in [-.30,-.15,0,.15,.30]:
            y=back[1]+(.28+dy)*scale;z=back[2]+(.20+dz)*scale;hits=[]
            for face in faces:
                a,b,c=[vertices[i] for i in face]
                u=(b[1]-a[1],b[2]-a[2]);v=(c[1]-a[1],c[2]-a[2])
                determinant=u[0]*v[1]-u[1]*v[0]
                if abs(determinant)<1e-9:continue
                p=((y-a[1])*v[1]-(z-a[2])*v[0])/determinant
                q=(u[0]*(z-a[2])-u[1]*(y-a[1]))/determinant
                if p>=0 and q>=0 and p+q<=1:hits.append(a[0]+p*(b[0]-a[0])+q*(c[0]-a[0]))
            if hits:surfaces.append(min(hits))
    assert len(surfaces)>=8,'Insufficient back surface coverage'
    # Keep the flat contact face just behind the most prominent body surface.
    return min(surfaces)-.015*scale-back[0]


def fits():
    result=[]
    for race in ['Human','Orc','Dwarf','NightElf','Scourge','Tauren','Gnome','Troll']:
        for sex in ['Male','Female']:
            path=f'Character\\{race}\\{sex}\\{race}{sex}.m2';data=client.read(path)
            assert data[:8]==b'MD20\0\1\0\0'
            n,o=struct.unpack_from('<II',data,0x44)
            vertices=[struct.unpack_from('<3f',data,o+48*i) for i in range(n)]
            view=struct.unpack_from('<I',data,0x50)[0]
            n,o=struct.unpack_from('<II',data,view);indices=struct.unpack_from('<'+'H'*n,data,o)
            n,o=struct.unpack_from('<II',data,view+8);triangles=struct.unpack_from('<'+'H'*n,data,o)
            n,o=struct.unpack_from('<II',data,view+24)
            sections=[struct.unpack_from('<10H3f',data,o+32*i) for i in range(n)]
            groups={}
            for section in sections:
                group=section[0]//100
                if group and group!=15:groups[group]=min(groups.get(group,section[0]),section[0])
            faces=[]
            for section in sections:
                if section[0]==0 or section[0] in groups.values():
                    faces.extend(tuple(indices[triangles[t+j]] for j in range(3)) for t in range(section[4],section[4]+section[5],3))
            lookup=struct.unpack_from('<I',data,0x110)[0];index=struct.unpack_from('<H',data,lookup+56)[0]
            attachments=struct.unpack_from('<I',data,0x108)[0]
            back=struct.unpack_from('<3f',data,attachments+48*index+8)
            result.append((race+' '+sex,back,contact_depth(vertices,faces,back)))
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--write',action='store_true');args=parser.parse_args()
    try:
        for archive in ['patch-2.MPQ','patch.MPQ','model.MPQ']:
            handle=C.c_void_p();assert client.lib.SFileOpenArchive(str(client.game/archive).encode(),0,0x100,C.byref(handle))
            client.archives.append(handle)
        rows=fits()
    finally:
        for handle in client.archives:client.lib.SFileCloseArchive(handle)
    header='// Generated numeric contact fits; no client model geometry.\n// Verify with tools/audit_bag_fits.py against build 5875.\n#pragma once\nstruct BagFit { std::array<float,3> anchor; float depth; };\nstatic constexpr BagFit bagFits[]={\n'
    header=header.replace('struct BagFit',f'static constexpr float bagModelScale={BAG_MODEL_SCALE:.9f}f;\nstruct BagFit')
    for name,back,depth in rows:
        header+='    {{'+','.join(f'{v:.9f}f' for v in back)+'},'+f'{depth:.9f}f'+'}, // '+name+'\n'
    header+='};\n'
    path=Path(__file__).resolve().parents[1]/'native/BagFits.h'
    if args.write:path.write_text(header)
    else:assert path.read_text()==header,'Back contact fits differ; review the installed models.'
    print('PASS: pack contact depth measured against all 16 vanilla race/gender back surfaces.')
