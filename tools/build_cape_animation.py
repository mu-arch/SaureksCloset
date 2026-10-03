"""Bake a gentler HumanFemale cape; original body and tabard tracks stay intact.

Only cape influences of lower chain 26 -> 47 get private bones. Walk, run and
walk-backward oscillation is reduced around each clip's authored mean pose.
No simulation, runtime mesh changes, animation timing or root-motion edits.
"""
import argparse, hashlib, json, struct
from pathlib import Path
import numpy as np

ROOT=Path(__file__).resolve().parents[1]
MODEL=r'Character\Human\Female\HumanFemale.m2'
GAINS={26:.65,47:.25}
CLIPS={4,5,13}

def array(data,at,fmt):
    n,off=struct.unpack_from('<II',data,at);size=struct.calcsize(fmt)
    assert off+n*size<=len(data)
    return list(struct.iter_unpack(fmt,data[off:off+n*size]))

def bake(original):
    assert original[:8]==b'MD20\x00\x01\x00\x00'
    data=bytearray(original)
    def block(value):
        data.extend(bytes((-len(data))%16));off=len(data);data.extend(value);return off
    def store(at,fmt,rows):
        raw=b''.join(struct.pack(fmt,*row) for row in rows)
        struct.pack_into('<II',data,at,len(rows),block(raw))
    nb,bo=struct.unpack_from('<II',original,0x34)
    assert nb==117,'Only the audited classic HumanFemale skeleton is supported'
    mapping={old:nb+i for i,old in enumerate(GAINS)}
    bones=[bytearray(original[bo+i*108:bo+(i+1)*108]) for i in range(nb)]
    sequences=array(original,0x1c,'<HHIIfIhHIII7fhH')
    changed=[]
    for old,gain in GAINS.items():
        bone=bytearray(bones[old]);parent=struct.unpack_from('<h',bone,8)[0]
        struct.pack_into('<i',bone,0,-1);struct.pack_into('<h',bone,8,mapping.get(parent,parent))
        for track,dims in [(12,3),(40,4)]:
            interp,glob,nr,ro,nt,to,nk,ko=struct.unpack_from('<Hh6I',bone,track)
            if not nk:continue
            assert interp in (0,1) and glob==-1 and nt==nk and nr>=len(sequences)
            keys=np.frombuffer(original,dtype='<f4',count=nk*dims,offset=ko).reshape(nk,dims).copy()
            ranges=[struct.unpack_from('<II',original,ro+i*8) for i in range(nr)]
            edits=[]
            for i,seq in enumerate(sequences):
                if seq[0] not in CLIPS:continue
                lo,hi=ranges[i];assert 0<=lo<=hi<nk
                values=keys[lo:hi+1].astype(float)
                if np.max(np.abs(values-values[0]))<1e-7:continue
                # Never alter a key range shared with any non-locomotion clip.
                assert all(not(max(lo,a)<=min(hi,b)) for j,(a,b) in enumerate(ranges) if j!=i and (j>=len(sequences) or sequences[j][0] not in CLIPS))
                if dims==4:
                    values/=np.linalg.norm(values,axis=1)[:,None]
                    values[np.sum(values*values[0],axis=1)<0]*=-1
                    center=values.mean(axis=0);center/=np.linalg.norm(center)
                    values=center+(values-center)*gain
                    values/=np.linalg.norm(values,axis=1)[:,None]
                else:
                    center=values.mean(axis=0);values=center+(values-center)*gain
                keys[lo:hi+1]=values;edits.append(dict(sequence=i,animation=seq[0],keys=[lo,hi]))
            if edits:
                struct.pack_into('<I',bone,track+24,block(keys.astype('<f4').tobytes()))
                changed.append(dict(bone=old,copy=mapping[old],track=track,gain=gain,clips=edits))
        bones.append(bone)
    struct.pack_into('<II',data,0x34,len(bones),block(b''.join(bones)))
    nv,vo=struct.unpack_from('<II',data,0x44)
    views,vbase=struct.unpack_from('<II',data,0x4c)
    cape_vertices=set();body_vertices=set();palettes=array(original,0x8c,'<H')
    for view in range(views):
        base=vbase+44*view;lookup=array(original,base,'<H');triangles=array(original,base+8,'<H')
        sections=[list(row) for row in array(original,base+24,'<10H3f')]
        for section in sections:
            cape=1500<=section[0]<1600
            used={lookup[triangles[k][0]][0] for k in range(section[4],section[4]+section[5])}
            (cape_vertices if cape else body_vertices).update(used)
            if cape:
                offset=section[7];palette=[(mapping.get(v[0],v[0]),) for v in palettes[offset:offset+section[6]]]
                section[7]=len(palettes);palettes.extend(palette)
                section[9]=mapping.get(section[9],section[9])
        store(base+24,'<10H3f',sections)
    assert not(cape_vertices&body_vertices),'Shared vertices require splitting before edits'
    for vertex in cape_vertices:
        for i in range(4):
            at=vo+vertex*48+16+i;data[at]=mapping.get(data[at],data[at])
    store(0x8c,'<H',palettes)
    return bytes(data),dict(source_sha256=hashlib.sha256(original).hexdigest(),sha256=hashlib.sha256(data).hexdigest(),original_bones=nb,bones=len(bones),cape_vertices=len(cape_vertices),unchanged_body_vertices=len(body_vertices),tracks=changed)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--client',type=Path,required=True);args=parser.parse_args()
    from client_mpq import Client
    source,archive=Client(args.client).read(MODEL);data,audit=bake(source);audit['archive']=archive
    target=ROOT/'addon/SaureksCloset/Animations/HumanFemaleCapeCalm.m2';target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    (ROOT/'assets/cape').mkdir(parents=True,exist_ok=True)
    (ROOT/'assets/cape/human-female-calm.json').write_text(json.dumps(audit,indent=2)+'\n')
    print('Baked',target,len(data),'bytes;',audit['cape_vertices'],'cape vertices; separate lower cape bones',audit['bones'])
if __name__=='__main__':main()
