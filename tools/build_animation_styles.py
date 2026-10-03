"""Offline animation retargeting. Preserve recipient meshes, pivots and clip timing.

Patches contain only bone tracks; cape-only bones are duplicated in each base.
The client composes independent body/cape patches once when a style is selected.
"""
import argparse, collections, hashlib, json, struct, zlib, subprocess, tempfile
from pathlib import Path
import numpy as np
from build_cape_animation import array
from client_mpq import Client
ROOT=Path(__file__).resolve().parents[1]
RACES=['Human','Orc','Dwarf','NightElf','Scourge','Tauren','Gnome','Troll']
SEQUENCE='<HHIIfIhHIII7fhH'
IDENTITY=np.array([0.,0.,0.,1.])
def norm(q):return q/np.maximum(np.linalg.norm(q,axis=-1,keepdims=True),1e-12)
def mul(a,b):
    a,b=np.broadcast_arrays(a,b)
    return np.concatenate((a[...,3:]*b[...,:3]+b[...,3:]*a[...,:3]+np.cross(a[...,:3],b[...,:3]),a[...,3:]*b[...,3:]-np.sum(a[...,:3]*b[...,:3],axis=-1,keepdims=True)),axis=-1)
def inv(q):return q*np.array([-1,-1,-1,1])
def blend(a,b,t):
    b=np.where(np.sum(a*b,axis=-1,keepdims=True)<0,-b,b)
    return norm(a+(b-a)*t)
class Model:
    def __init__(self,data):
        self.data=data;self.n,self.bo=struct.unpack_from('<II',data,52)
        self.records=[data[self.bo+108*i:self.bo+108*(i+1)] for i in range(self.n)]
        self.parents=[struct.unpack_from('<h',r,8)[0] for r in self.records]
        self.pivots=np.array([struct.unpack_from('<3f',r,96) for r in self.records])
        self.keys={struct.unpack_from('<i',r)[0]:i for i,r in enumerate(self.records) if struct.unpack_from('<i',r)[0]>=0}
        self.seqs=array(data,28,SEQUENCE);self.sequence={s[:2]:i for i,s in enumerate(self.seqs)}
        self.stand=next(i for i,s in enumerate(self.seqs) if s[0]==0)
        self.tracks={}
        for i,r in enumerate(self.records):
            for at,dims in [(12,3),(40,4)]:
                interp,glob,nr,ro,nt,to,nk,ko=struct.unpack_from('<Hh6I',r,at)
                self.tracks[i,at]=(interp,glob,np.frombuffer(data,dtype='<u4',count=nr*2,offset=ro).reshape(-1,2),np.frombuffer(data,dtype='<u4',count=nt,offset=to),np.frombuffer(data,dtype='<f4',count=nk*dims,offset=ko).reshape(-1,dims))
        nv,vo=struct.unpack_from('<II',data,68);self.vo=vo
        self.vertices=np.array([struct.unpack_from('<3f',data,vo+48*i) for i in range(nv)])
        self.weights=np.array([struct.unpack_from('<4B',data,vo+48*i+12) for i in range(nv)])
        self.indices=np.array([struct.unpack_from('<4B',data,vo+48*i+16) for i in range(nv)])
        _,v=struct.unpack_from('<II',data,76);lookup=array(data,v,'<H');tri=array(data,v+8,'<H');cape=set();body=set();long=set()
        for sec in array(data,v+24,'<10H3f'):
            ids={lookup[tri[k][0]][0] for k in range(sec[4],sec[4]+sec[5])}
            (cape if 1500<=sec[0]<1600 else body).update(ids)
            if sec[0]==1502:long.update(ids)
        assert not cape&body
        self.cape=cape;self.body=body
        z=self.vertices[list(long),2];hem=[i for i in long if self.vertices[i,2]<min(z)+(max(z)-min(z))*.3]
        scores=collections.Counter()
        for i in hem:
            for b,w in zip(self.indices[i],self.weights[i]):scores[int(b)]+=int(w)
        bone=scores.most_common(1)[0][0];chain=[]
        while bone!=self.keys[5]:
            assert bone>=0
            chain.append(bone);bone=self.parents[bone]
        self.cape_chain=chain[::-1];assert 2<=len(chain)<=4
        self.height=float(self.pivots[self.keys[6],2])
    def sample(self,bone,at,seq,phase):
        interp,glob,ranges,times,values=self.tracks[bone,at];dims=values.shape[1]
        if not len(values):return np.tile(IDENTITY if dims==4 else [0,0,0],(len(phase),1))
        if glob!=-1:raise ValueError('Global animation tracks must stay native')
        lo,hi=ranges[seq] if seq<len(ranges) else (0,len(values)-1)
        times=times[lo:hi+1];values=values[lo:hi+1]
        if not len(times):return np.tile(IDENTITY if dims==4 else [0,0,0],(len(phase),1))
        s=self.seqs[seq];ticks=s[2]+phase*(s[3]-s[2]);a=np.clip(np.searchsorted(times,ticks,side='right')-1,0,len(times)-1);b=np.minimum(a+1,len(times)-1)
        t=np.clip((ticks-times[a])/np.maximum(times[b]-times[a],1),0,1)[:,None] if interp else np.zeros((len(phase),1))
        return blend(norm(values[a]),norm(values[b]),t) if dims==4 else values[a]+(values[b]-values[a])*t
    def neutral(self,bone,at=40):return self.sample(bone,at,self.stand,np.array([0.]))[0]

def semantic(model):
    # Authored key-bone IDs identify head, arms, fingers, pelvis and root.
    # Ancestor paths recover the intervening elbow/wrist/spine joints.
    result={}
    for key,bone in model.keys.items():
        if key not in [4,5,6,7,26]:continue
        depth=0
        while bone>=0:
            result[key,depth]=bone
            if key in (7,12,17,19,20):break
            bone=model.parents[bone];depth+=1
            if bone in model.keys.values():break
    # Hand attachment IDs have consistent left/right semantics across races;
    # arm key-bone ordering does not (Tauren/Troll reverse some identifiers).
    # Preserve recipient finger/grip animation rather than mapping unlike digits.
    count,offset=struct.unpack_from('<II',model.data,0x104)
    attachments={struct.unpack_from('<I',model.data,offset+48*i)[0]:struct.unpack_from('<H',model.data,offset+48*i+4)[0] for i in range(count)}
    for hand in (1,2):
        bone=attachments[hand];depth=0
        while bone!=model.keys[4]:
            assert bone>=0
            result[200+hand,depth]=bone;bone=model.parents[bone];depth+=1
        assert depth==6
    # Legs have no named key-bone IDs in this client. Find their weighted
    # ankle pivots on each side, then follow the authored pelvis hierarchy.
    weighted=set(model.indices[model.weights>0]);pelvis=model.keys[5]
    for side in [-1,1]:
        candidates=[]
        for b in range(model.n):
            chain=[];a=int(b)
            while a>=0 and a!=pelvis:chain.append(a);a=model.parents[a]
            if a==pelvis and len(chain)==3 and model.pivots[b,1]*side>.015 and model.pivots[b,2]<model.height*.45:
                candidates.append((model.pivots[b,2],chain))
        assert candidates,'No ankle chain'
        chain=min(candidates)[1]
        for depth,bone in enumerate(chain):result[100+side,depth]=bone
        # Toe follows ankle; use the weighted lowest child, not knee helpers.
        children=[b for b in weighted if model.parents[b]==chain[0]]
        if children:result[100+side,-1]=min(children,key=lambda b:model.pivots[b,2])
    return result

def correspondence(target,source):
    a,b=semantic(target),semantic(source);mapping={}
    for label,bone in a.items():
        if label in b:
            donor=b[label]
            if bone in mapping and mapping[bone]!=donor:raise ValueError(('Conflicting semantic mapping',label,bone,donor,mapping[bone]))
            mapping[bone]=donor
    # Face details, race-only appendages and cloth chains keep their own tracks.
    for bone in target.cape_chain:mapping.pop(bone,None)
    return mapping

def base_model(model):
    data=bytearray(model.data)
    def block(raw):
        data.extend(bytes(-len(data)%16));off=len(data);data.extend(raw);return off
    mapping={old:model.n+i for i,old in enumerate(model.cape_chain)}
    assert model.n+len(mapping)<256
    records=list(model.records)
    for old in model.cape_chain:
        record=bytearray(model.records[old]);struct.pack_into('<i',record,0,-1);struct.pack_into('<h',record,8,mapping.get(model.parents[old],model.parents[old]));records.append(record)
    struct.pack_into('<II',data,52,len(records),block(b''.join(records)))
    views,vbase=struct.unpack_from('<II',data,76);palettes=array(model.data,0x8c,'<H')
    for view in range(views):
        at=vbase+44*view;sections=[list(s) for s in array(model.data,at+24,'<10H3f')]
        for sec in sections:
            if 1500<=sec[0]<1600:
                remap=[(mapping.get(v[0],v[0]),) for v in palettes[sec[7]:sec[7]+sec[6]]];sec[7]=len(palettes);palettes.extend(remap);sec[9]=mapping.get(sec[9],sec[9])
        struct.pack_into('<II',data,at+24,len(sections),block(b''.join(struct.pack('<10H3f',*s) for s in sections)))
    for v in model.cape:
        for k in range(4):
            at=model.vo+48*v+16+k;data[at]=mapping.get(data[at],data[at])
    struct.pack_into('<II',data,0x8c,len(palettes),block(b''.join(struct.pack('<H',*v) for v in palettes)))
    return bytes(data),mapping

def track_blob(model,bone,at,changed):
    interp,glob,ranges,times,keys=model.tracks[bone,at];assert glob==-1 and interp in (0,1)
    new_ranges=[];new_times=[];new_keys=[]
    for i in range(max(len(ranges),len(model.seqs))):
        if i in changed:t,k=changed[i]
        elif len(keys):
            lo,hi=ranges[i] if i<len(ranges) else (0,len(keys)-1);t=times[lo:hi+1];k=keys[lo:hi+1]
        else:
            t=np.array([model.seqs[min(i,len(model.seqs)-1)][2]]);k=np.array([IDENTITY if at==40 else [0,0,0]])
        new_ranges.append((len(new_keys),len(new_keys)+len(k)-1));new_times.extend(t);new_keys.extend(k)
    ranges_bytes=np.array(new_ranges,dtype='<u4').tobytes();times_bytes=np.array(new_times,dtype='<u4').tobytes();keys_bytes=np.array(new_keys,dtype='<f4').tobytes()
    return struct.pack('<Hh6I',1,-1,len(new_ranges),28,len(new_times),28+len(ranges_bytes),len(new_keys),28+len(ranges_bytes)+len(times_bytes))+ranges_bytes+times_bytes+keys_bytes

def make_patch(target,source,base,mapping,cape=False,calm=False):
    entries=[];matched=0
    transfers={b:source.cape_chain[min(i,len(source.cape_chain)-1)] for i,b in enumerate(target.cape_chain)} if cape else correspondence(target,source)
    edits={(b,40):{} for b in transfers}
    # Translation is transferred only at the skeletal root, scaled for the
    # recipient's proportions. Local joint translation/scale stays untouched.
    if not cape:edits[target.keys[26],12]={}
    for si,seq in enumerate(target.seqs):
        donor=source.sequence.get(seq[:2]);donor=source.sequence.get((seq[0],0)) if donor is None else donor
        if donor is None or seq[3]<=seq[2]:continue
        matched+=1
        count=min(65,max(3,int((seq[3]-seq[2])/40)+1));phase=np.linspace(0,1,count);times=np.round(seq[2]+phase*(seq[3]-seq[2])).astype('<u4')
        if cape:
            # Sample the cumulative donor chain along normalized chain length,
            # then distribute that motion over the recipient's own joints.
            cumulative=[np.tile(IDENTITY,(count,1))]
            for b in source.cape_chain:
                delta=mul(source.sample(b,40,donor,phase),inv(source.neutral(b)))
                cumulative.append(norm(mul(cumulative[-1],delta)))
            previous=cumulative[0]
            for i,b in enumerate(target.cape_chain):
                at=(i+1)*len(source.cape_chain)/len(target.cape_chain);lo=min(int(at),len(source.cape_chain));hi=min(lo+1,len(source.cape_chain));current=blend(cumulative[lo],cumulative[hi],at-lo)
                delta=mul(inv(previous),current);previous=current
                values=norm(mul(delta,target.neutral(b)))
                if calm and seq[0] in (4,5,13):
                    center=norm(values[0]);aligned=np.where(np.sum(values*center,axis=1)[:,None]<0,-values,values);center=norm(aligned.mean(axis=0));values=blend(center,values,.65 if i==0 else .25)
                edits[b,40][si]=times,values
        else:
            for b,donor_b in transfers.items():
                if target.tracks[b,40][1]!=-1 or source.tracks[donor_b,40][1]!=-1:continue
                values=norm(mul(mul(source.sample(donor_b,40,donor,phase),inv(source.neutral(donor_b))),target.neutral(b)))
                edits[b,40][si]=times,values
            b=target.keys[26];db=source.keys[26]
            values=target.neutral(b,12)+(source.sample(db,12,donor,phase)-source.neutral(db,12))*(target.height/source.height)
            edits[b,12][si]=times,values
    for (b,at),changed in edits.items():
        if changed:entries.append((mapping[b] if cape else b,at,track_blob(target,b,at,changed)))
    blob=bytearray(struct.pack('<4sIII',b'SCA1',len(base),zlib.crc32(base),len(entries)))
    for b,at,track in entries:blob.extend(struct.pack('<HHI',b,at,len(track)));blob.extend(track)
    return bytes(blob),dict(tracks=len(entries),matched_clips=matched,mapping={str(b):int(s) for b,s in transfers.items()})

def main():
    p=argparse.ArgumentParser();p.add_argument('--client',type=Path,required=True);p.add_argument('--body-only',action='store_true');args=p.parse_args();client=Client(args.client)
    compressor=Path(tempfile.gettempdir())/'closet-pack-animation-styles'
    subprocess.run(['c++','-std=c++17','-O2',str(ROOT/'tools/pack_animation_styles.cpp'),'-o',str(compressor)],check=True)
    out=ROOT/'addon/SaureksCloset/Animations/Styles';out.mkdir(parents=True,exist_ok=True);models=[];audit=json.loads((ROOT/'assets/cape/animation-styles.json').read_text()) if args.body_only else {'files':{},'models':[]}
    for race in RACES:
        for sex in ['Male','Female']:
            data,archive=client.read(f'Character\\{race}\\{sex}\\{race}{sex}.m2');models.append(Model(data))
            if not args.body_only:audit['models'].append(dict(race=race,sex=sex,source_sha256=hashlib.sha256(data).hexdigest(),archive=archive))
    def write(name,data):
        (out/name).write_bytes(data)
        if name.endswith('.sca'):subprocess.run([str(compressor),str(out/name)],check=True)
        audit['files'][name]=hashlib.sha256((out/name).read_bytes()).hexdigest()
    for i,target in enumerate(models,1):
        base,mapping=base_model(target);write(f'B{i:02}.m2',base);row=audit['models'][i-1];row['cape_chain']=target.cape_chain
        if not args.body_only:row['styles']={}
        for j,donor in enumerate(models,1):
            if i==j:continue
            for cape in ([False] if args.body_only else [False,True]):
                patch,details=make_patch(target,donor,base,mapping,cape);name=f'{"C" if cape else "A"}{i:02}_{j:02}.sca';write(name,patch);row['styles'][name]=details
        print('Baked',i,row['race'],row['sex'],flush=True)
    calm=Model((ROOT/'addon/SaureksCloset/Animations/HumanFemaleCapeCalm.m2').read_bytes())
    base,mapping=base_model(models[1]);patch=bytearray(struct.pack('<4sIII',b'SCA1',len(base),zlib.crc32(base),len(mapping)))
    for b in range(models[1].n,calm.n):
        track=track_blob(calm,b,40,{})
        patch.extend(struct.pack('<HHI',b,40,len(track)));patch.extend(track)
    write('C02_17.sca',bytes(patch))
    (ROOT/'assets/cape/animation-styles.json').write_text(json.dumps(audit,indent=2)+'\n')
if __name__=='__main__':main()
