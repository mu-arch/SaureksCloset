"""Check the shipped baked cape, including actual skinned lower-edge motion.
No game installation is needed: original records are retained in the variant.
"""
from pathlib import Path
import sys,struct,hashlib,json
import numpy as np
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from build_cape_animation import array,GAINS,CLIPS,bake
p=ROOT/'addon/SaureksCloset/Animations/HumanFemaleCapeCalm.m2';data=p.read_bytes()
audit=json.loads((ROOT/'assets/cape/human-female-calm.json').read_text())
assert hashlib.sha256(data).hexdigest()==audit['sha256']
nb,bo=struct.unpack_from('<II',data,0x34);assert nb==119
mapping={old:117+i for i,old in enumerate(GAINS)}
sequences=array(data,0x1c,'<HHIIfIhHIII7fhH')
nv,vo=struct.unpack_from('<II',data,0x44)
vertices=np.array([struct.unpack_from('<3f',data,vo+48*i) for i in range(nv)])
weights=np.array([struct.unpack_from('<4B',data,vo+48*i+12) for i in range(nv)])/255
bones=np.array([struct.unpack_from('<4B',data,vo+48*i+16) for i in range(nv)])
oldbones=bones.copy()
for old,new in mapping.items():oldbones[bones==new]=old
views,vbase=struct.unpack_from('<II',data,0x4c);cape=set();body=set();long=set()
for view in range(views):
    base=vbase+44*view;lookup=array(data,base,'<H');tri=array(data,base+8,'<H');props=array(data,base+16,'<4B');palette=array(data,0x8c,'<H')
    for sec in array(data,base+24,'<10H3f'):
        used={tri[k][0] for k in range(sec[4],sec[4]+sec[5])};is_cape=1500<=sec[0]<1600
        (cape if is_cape else body).update(lookup[i][0] for i in used)
        if sec[0]==1502:long.update(lookup[i][0] for i in used)
        for index in used:
            vertex=lookup[index][0]
            for k in range(4):
                if weights[vertex,k]:assert palette[sec[7]+props[index][k]][0]==bones[vertex,k], 'GPU palette disagrees with CPU skin'
assert not(cape&body)
assert np.array_equal(bones[list(body)],oldbones[list(body)]),'Body/tabard influences changed'
assert all(not any(b in mapping.values() for b in bones[i]) for i in cape if vertices[i,2]>1.3),'Upper cape attachment changed'
for old,new in mapping.items():
    original=data[bo+old*108:bo+(old+1)*108];copy=data[bo+new*108:bo+(new+1)*108]
    assert original[96:108]==copy[96:108] and original[68:96]==copy[68:96],'Pivot or scale changed'
    for track,dims in [(12,3),(40,4)]:
        before=struct.unpack_from('<Hh6I',original,track);after=struct.unpack_from('<Hh6I',copy,track)
        assert before[:-1]==after[:-1],'Timing or interpolation changed'
        nk,ko=before[-2:];newko=after[-1]
        for i,seq in enumerate(sequences):
            if seq[0] in CLIPS or not nk:continue
            lo,hi=struct.unpack_from('<II',data,before[3]+8*i)
            assert data[ko+lo*dims*4:ko+(hi+1)*dims*4]==data[newko+lo*dims*4:newko+(hi+1)*dims*4],'Unrelated animation changed'

def quat(q):
    x,y,z,w=q
    return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)], [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)], [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
def evaluate(seq,t):
    matrices=[]
    for i in range(nb):
        offset=bo+108*i;_,flags,parent,_=struct.unpack_from('<iIhH',data,offset);pivot=np.array(struct.unpack_from('<3f',data,offset+96))
        base=matrices[parent].copy() if parent>=0 else np.eye(4)
        if flags&7:
            origin=(base@np.r_[pivot,1])[:3]
            if flags&6==6:base[:3,:3]=np.eye(3)
            elif flags&6==4:base[:3,:3]=np.diag(np.linalg.norm(base[:3,:3],axis=0))
            elif flags&6==2:base[:3,:3]/=np.linalg.norm(base[:3,:3],axis=0)
            base[:3,3]=-base[:3,:3]@pivot if flags&1 else origin-base[:3,:3]@pivot
        values=[]
        for track,default in [(12,[0,0,0]),(40,[0,0,0,1]),(68,[1,1,1])]:
            interp,glob,nr,ro,nt,to,nk,ko=struct.unpack_from('<Hh6I',data,offset+track)
            if not nk:values.append(np.array(default));continue
            lo,hi=struct.unpack_from('<II',data,ro+seq*8) if nr else (0,nk-1)
            times=np.frombuffer(data,dtype='<u4',count=nt,offset=to)[lo:hi+1]
            keys=np.frombuffer(data,dtype='<f4',count=nk*len(default),offset=ko).reshape(nk,len(default))[lo:hi+1]
            k=max(0,min(len(times)-1,np.searchsorted(times,t,side='right')-1));v=keys[k].astype(float)
            if interp and k+1<len(times) and times[k+1]>times[k]:v=v+(keys[k+1]-v)*np.clip((t-int(times[k]))/(int(times[k+1])-int(times[k])),0,1)
            values.append(v)
        translation,rotation,scale=values;local=np.eye(4);local[:3,:3]=quat(rotation)@np.diag(scale);local[:3,3]=pivot+translation-local[:3,:3]@pivot
        matrices.append(base@local)
    return np.array(matrices)

def skin(matrices,ids,indices):
    position=np.c_[vertices[ids],np.ones(len(ids))]
    return np.sum(np.einsum('nkij,nj->nki',matrices[indices[ids]],position)*weights[ids,:,None],axis=1)[:,:3]
edge=sorted(i for i in long if vertices[i,2]<min(vertices[list(long),2])+.12)
assert edge
ratios={}
for seq,s in enumerate(sequences):
    if s[0] not in CLIPS:continue
    native=[];calm=[]
    for tick in np.linspace(s[2],s[3],48):
        matrices=evaluate(seq,tick)
        # Compare free-end motion relative to its moving body mount.
        inv=np.linalg.inv(matrices[22])
        for out,indices in [(native,oldbones),(calm,bones)]:
            points=skin(matrices,edge,indices);out.append((inv@np.c_[points,np.ones(len(points))].T).T[:,:3])
    native=np.array(native);calm=np.array(calm)
    before=np.sqrt(np.mean((native-native.mean(axis=0))**2));after=np.sqrt(np.mean((calm-calm.mean(axis=0))**2));ratio=after/before
    assert ratio<.8,(s[0],ratio)
    ratios[str(s[0])]=round(float(ratio),3)
print('PASS: cape-only CPU/GPU weights, pinned upper cape, preserved body/tabard and unrelated tracks; lower-edge RMS ratios:',ratios)
# Optional source check proves all existing character bone records stay byte-identical.
if len(sys.argv)>1:
    source=Path(sys.argv[1]).read_bytes();rebuilt,_=bake(source);assert rebuilt==data
    _,original_bones=struct.unpack_from('<II',source,0x34)
    assert data[bo:bo+117*108]==source[original_bones:original_bones+117*108]
    print('PASS: deterministic bake and every original character bone preserved byte-for-byte')
