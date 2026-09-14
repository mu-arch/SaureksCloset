"""Validate the shipped vanilla model, source topology, atlas and all mip levels."""
import hashlib
import io
import math
from pathlib import Path
import struct as S
import sys
import tempfile
from PIL import Image

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'tools'))
from build_bag_model import read_glb, write_m2, write_blp

folder=root/'addon/SaureksCloset/Models'
data=(folder/'DarkSchoolbag.m2').read_bytes()
assert data[:8]==b'MD20\0\1\0\0'

def array(offset,stride):
    count,start=S.unpack_from('<II',data,offset)
    assert count>0 and start>=0x144 and start+count*stride<=len(data)
    return count,start

count,offset=array(0x34,108)
assert count==1 and S.unpack_from('<h',data,offset+8)[0]==-1
n,vertex=array(0x44,48)
positions=[]
for i in range(n):
    values=S.unpack_from('<3f8B3f4f',data,vertex+48*i)
    assert all(math.isfinite(v) for v in values)
    assert values[3:11]==(255,0,0,0,0,0,0,0)
    assert abs(sum(v*v for v in values[11:14])-1)<.001
    assert all(0<=uv<=1 for uv in values[14:16])
    positions.append(values[:3])
low=S.unpack_from('<3f',data,0xb4);high=S.unpack_from('<3f',data,0xc0)
assert low==tuple(min(v[i] for v in positions) for i in range(3))
assert high==tuple(max(v[i] for v in positions) for i in range(3))
assert high[0]==0 and low[0]<0 and abs(low[1]+high[1])<.00001 and abs(low[2]+high[2])<.00001
views,view=array(0x4c,44);assert views==1
index_count,index=array(view,2);assert index_count==n
indices=S.unpack_from('<'+'H'*n,data,index);assert max(indices)<n
tri_count,tri=array(view+8,2);assert tri_count%3==0
assert max(S.unpack_from('<'+'H'*tri_count,data,tri))<n
section_count,section=array(view+24,32);assert section_count==1
record=S.unpack_from('<10H3f',data,section)
assert record[2:10]==(0,n,0,tri_count,1,0,1,0)
batch_count,batch=array(view+32,24);assert batch_count==1
tex_count,tex=array(0x5c,16);assert tex_count==1
kind,flags,length,name=S.unpack_from('<4I',data,tex)
assert kind==2 and data[name:name+length]==b'Interface\\AddOns\\SaureksCloset\\Models\\DarkSchoolbag.blp\0'
vertices,triangles,atlas=read_glb(root/'assets/DarkSchoolbag.glb')
assert len(vertices)==n and len(triangles)==tri_count
with tempfile.TemporaryDirectory() as d:
    m2=Path(d)/'test.m2';blp=Path(d)/'test.blp'
    write_m2(vertices,triangles,m2);write_blp(atlas,blp)
    assert m2.read_bytes()==data
    assert blp.read_bytes()==(folder/'DarkSchoolbag.blp').read_bytes()
blp=(folder/'DarkSchoolbag.blp').read_bytes()
assert S.unpack_from('<4sI4BII',blp)==(b'BLP2',1,2,0,0,1,128,128)
offsets=S.unpack_from('<16I',blp,20);sizes=S.unpack_from('<16I',blp,84)
end=1172
for i in range(8):
    side=max(1,128>>i);length=max(1,(side+3)//4)**2*8
    assert offsets[i]==end and sizes[i]==length;end+=length
assert end==len(blp) and not any(offsets[8:]) and not any(sizes[8:])
source=Image.open(io.BytesIO(atlas)).convert('RGB').resize((128,128),Image.Resampling.LANCZOS)
decoded=Image.open(folder/'DarkSchoolbag.blp').convert('RGB');assert source.size==decoded.size
error=sum((a-b)**2 for p,q in zip(source.get_flattened_data(),decoded.get_flattened_data()) for a,b in zip(p,q))/(128*128*3)
psnr=10*math.log10(255**2/error)
assert psnr>35,psnr
assert '../ASSETS-LICENSE' in (folder/'ASSETS-LICENSE').read_text()
print(f'PASS: vanilla M2 structure, {n} original vertices, {tri_count//3} triangles, 128px texture with 8 mip levels, {psnr:.1f} dB texture fidelity; deterministic rebuild.')
